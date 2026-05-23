/*
 * JsonAdapter.cpp, part of VCMI engine
 *
 * JSON-over-TCP ingress for vcmiserver. Listens on a separate port, accepts
 * length-prefixed JSON frames, dispatches via the pack codec registry so
 * adding new pack types means adding a new file under server/json/packs/
 * rather than editing this one.
 */
#include "StdInc.h"

#include "JsonAdapter.h"
#include "CVCMIServer.h"
#include "CGameHandler.h"
#include "json/PackCodec.h"
#include "json/PackCodecRegistry.h"

#include "../lib/json/JsonNode.h"
#include "../lib/network/NetworkInterface.h"
#include "../lib/networkPacks/PacksForLobby.h"
#include "../lib/networkPacks/PacksForServer.h"
#include "../lib/networkPacks/PacksForClient.h"
#include "../lib/gameState/CGameState.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapping/TerrainTile.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/mapObjects/CGTownInstance.h"
#include "../lib/entities/faction/CTown.h"
#include "../lib/CCreatureHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/pathfinder/CGPathNode.h"
#include "../lib/pathfinder/PathfinderOptions.h"
#include "queries/QueriesProcessor.h"
#include "queries/CQuery.h"
#include "processors/TurnOrderProcessor.h"
#include "../lib/serializer/GameConnection.h"

JsonAdapter::JsonAdapter(CVCMIServer & srv) : server(srv) {}
JsonAdapter::~JsonAdapter() = default;

uint16_t JsonAdapter::start(uint16_t port)
{
	listener = server.getNetworkHandler().createServerTCP(*this);
	auto bound = listener->start(port);
	logNetwork->info("[JsonAdapter] Listening for JSON connections on port %d", bound);
	return bound;
}

bool JsonAdapter::ownsConnection(const std::shared_ptr<GameConnection> & game) const
{
	for (const auto & pair : jsonConnections)
		if (pair.second == game)
			return true;
	return false;
}

void JsonAdapter::onNewConnection(const std::shared_ptr<INetworkConnection> & connection)
{
	logNetwork->info("[JsonAdapter] New JSON client connected");
	auto game = std::make_shared<GameConnection>(connection);
	game->enterLobbyConnectionMode();
	jsonConnections.emplace_back(connection, game);
	server.activeConnections.push_back(game);
}

void JsonAdapter::onDisconnected(const std::shared_ptr<INetworkConnection> & connection, const std::string & errorMessage)
{
	logNetwork->info("[JsonAdapter] JSON client disconnected: %s", errorMessage);
	for (auto it = jsonConnections.begin(); it != jsonConnections.end(); ++it)
	{
		if (it->first == connection)
		{
			auto game = it->second;
			jsonConnections.erase(it);
			LobbyClientDisconnected lcd;
			lcd.clientId = game->connectionID;
			server.handleReceivedPack(game, lcd);
			return;
		}
	}
}

void JsonAdapter::onPacketReceived(const std::shared_ptr<INetworkConnection> & connection, const std::vector<std::byte> & message)
{
	std::shared_ptr<GameConnection> game;
	for (auto & pair : jsonConnections)
		if (pair.first == connection) { game = pair.second; break; }

	if (!game)
	{
		logNetwork->error("[JsonAdapter] packet from unknown connection; dropping");
		connection->close();
		return;
	}

	try
	{
		JsonNode root(reinterpret_cast<const std::byte *>(message.data()), message.size(), "<json-adapter>");
		if (!root.isStruct() || !root["type"].isString())
			throw std::runtime_error("JSON message missing 'type' field");

		const std::string & packType = root["type"].String();
		logNetwork->info("[JsonAdapter] received pack type='%s'", packType);

		// Wrapper-namespace queries: not real engine packs. The wrapper uses
		// these to ask for engine-state info (map size, terrain regions, etc.).
		// We build a response and send it back, never forwarding to the engine.
		if (packType.rfind("Wrapper", 0) == 0)
		{
			handleWrapperQuery(connection, packType, root);
			return;
		}

		const PackCodec * codec = PackCodecRegistry::instance().findByTypeName(packType);
		if (codec == nullptr)
			throw std::runtime_error("No codec registered for pack type: " + packType);

		std::unique_ptr<CPack> rawPack = codec->fromJson(root);

		// Dispatch by pack family, matching the binary-peer flow in
		// CVCMIServer::onPacketReceived (which uses ICPackVisitor).
		if (auto * lobbyPack = dynamic_cast<CPackForLobby *>(rawPack.get()))
		{
			server.handleReceivedPack(game, *lobbyPack);
		}
		else if (auto * serverPack = dynamic_cast<CPackForServer *>(rawPack.get()))
		{
			if (server.gh)
				server.gh->handleReceivedPack(game->connectionID, *serverPack);
			else
				throw std::runtime_error("Pack '" + packType + "' is a CPackForServer but no CGameHandler is running yet (still in lobby?)");
		}
		else
		{
			throw std::runtime_error("Pack type '" + packType + "' is neither CPackForLobby nor CPackForServer");
		}

		logNetwork->info("[JsonAdapter] dispatch complete: type='%s'", packType);
	}
	catch (const std::exception & e)
	{
		logNetwork->error("[JsonAdapter] error handling packet: %s. Dropping connection.", e.what());
		try { connection->close(); } catch (...) {}
	}
}

void JsonAdapter::sendPackToJsonClient(const std::shared_ptr<GameConnection> & game, CPackForLobby & pack)
{
	sendPackToJsonClientImpl(game, pack);
}

void JsonAdapter::sendPackToJsonClient(const std::shared_ptr<GameConnection> & game, CPackForClient & pack)
{
	sendPackToJsonClientImpl(game, pack);
}

void JsonAdapter::sendPackToJsonClientImpl(const std::shared_ptr<GameConnection> & game, CPack & pack)
{
	std::shared_ptr<INetworkConnection> sock;
	for (const auto & pair : jsonConnections)
		if (pair.second == game) { sock = pair.first; break; }
	if (!sock)
		return;

	JsonNode out;
	out.Struct(); // ensure struct type

	const PackCodec * codec = PackCodecRegistry::instance().findByPack(pack);
	if (codec != nullptr)
	{
		codec->toJson(pack, out);
	}
	else
	{
		// No outbound codec for this pack subtype yet. Emit a minimal envelope
		// so the JSON client knows something happened.
		out["type"].String() = "Unsupported";
		out["cppType"].String() = typeid(pack).name();
	}

	// Pack-specific outbound enrichment: decorate the JSON with engine-state info
	// the codec can't access (codecs are stateless). Wrapped defensively so a
	// throw can't abort the pack send — wrapper clients miss enrichment but
	// still see the base pack.
	try { enrichOutbound(game, pack, out); }
	catch (const std::exception & e) {
		logNetwork->warn("[JsonAdapter] enrichOutbound threw: %s", e.what());
	}

	std::string body = out.toCompactString();
	std::vector<std::byte> payload(body.size());
	std::memcpy(payload.data(), body.data(), body.size());
	logNetwork->info("[JsonAdapter] outbound: %s", body);
	sock->sendPacket(payload); // NetworkConnection::sendPacket prepends the 4-byte size header itself
}

void JsonAdapter::sendRawJson(const std::shared_ptr<INetworkConnection> & sock, const JsonNode & json)
{
	std::string body = json.toCompactString();
	std::vector<std::byte> payload(body.size());
	std::memcpy(payload.data(), body.data(), body.size());
	logNetwork->info("[JsonAdapter] outbound (raw): %s", body);
	sock->sendPacket(payload);
}

void JsonAdapter::handleWrapperQuery(const std::shared_ptr<INetworkConnection> & sock, const std::string & queryType, const JsonNode & req)
{
	if (!server.gh || !server.gh->gs)
	{
		JsonNode err;
		err["type"].String() = "WrapperQueryError";
		err["queryType"].String() = queryType;
		err["error"].String() = "no game running yet";
		sendRawJson(sock, err);
		return;
	}
	const auto & map = server.gh->gs->getMap();

	if (queryType == "WrapperQueryMap")
	{
		// Map metadata: size, level count, and a summary of known objects.
		JsonNode resp;
		resp["type"].String() = "WrapperMapMeta";
		resp["width"].Integer() = map.width;
		resp["height"].Integer() = map.height;
		resp["levels"].Integer() = map.levels();
		sendRawJson(sock, resp);
		return;
	}

	if (queryType == "WrapperListHeroes")
	{
		// Returns heroes the engine currently knows about. Useful at game start
		// where the wrapper only learns about heroes via HeroVisit packs, which
		// fire only for the active player's heroes initially. This lets the
		// joining player see RED's hero (and vice versa) before turn cycles.
		JsonNode resp;
		resp["type"].String() = "WrapperHeroes";
		JsonNode & arr = resp["heroes"];
		arr.Vector();
		for (const auto & heroId : server.gh->gs->getMap().getHeroesOnMap())
		{
			const auto * h = server.gh->gs->getHero(heroId);
			if (!h) continue;
			JsonNode entry;
			entry["id"].Integer() = h->id.getNum();
			if (h->getOwner().isValidPlayer())
				entry["owner"].Integer() = h->getOwner().getNum();
			entry["position"]["x"].Integer() = h->pos.x;
			entry["position"]["y"].Integer() = h->pos.y;
			entry["position"]["z"].Integer() = h->pos.z;
			entry["movePoints"].Integer() = h->movementPointsRemaining();
			arr.Vector().push_back(entry);
		}
		sendRawJson(sock, resp);
		return;
	}

	if (queryType == "WrapperQueryHeroSpells")
	{
		// Returns a hero's spellbook + mana with engine-authoritative metadata
		// (name, level, this hero's actual cost, castability). The wrapper needs
		// this to drive `battle-cast` — spell IDs alone are useless without
		// names/costs/mana, and cost depends on the hero's skills.
		const int heroId = static_cast<int>(req["heroId"].Integer());
		const auto * h = server.gh->gs->getHero(ObjectInstanceID(heroId));
		JsonNode resp;
		resp["type"].String() = "WrapperHeroSpells";
		resp["heroId"].Integer() = heroId;
		if (!h)
		{
			resp["error"].String() = "no such hero";
			sendRawJson(sock, resp);
			return;
		}
		resp["mana"].Integer() = h->mana;
		resp["manaLimit"].Integer() = h->manaLimit();
		resp["hasSpellbook"].Bool() = h->hasSpellbook();
		JsonNode & arr = resp["spells"];
		arr.Vector();
		for (const SpellID & sid : h->getSpellsInSpellbook())
		{
			const CSpell * sp = sid.toSpell();
			if (!sp) continue;
			JsonNode entry;
			entry["id"].Integer() = sid.getNum();
			entry["name"].String() = sp->getNameTranslated();
			entry["level"].Integer() = sp->getLevel();
			entry["cost"].Integer() = h->getSpellCost(sp);
			entry["combat"].Bool() = sp->isCombat();
			entry["adventure"].Bool() = sp->isAdventure();
			entry["canCast"].Bool() = h->canCastThisSpell(sp) && h->mana >= h->getSpellCost(sp);
			arr.Vector().push_back(entry);
		}
		sendRawJson(sock, resp);
		return;
	}

	if (queryType == "WrapperQueryTown")
	{
		// Returns a town's built buildings + dwellings with the creatures
		// recruitable RIGHT NOW (id, name, available count, gold cost). The
		// wrapper learns town changes from NewStructures/SetAvailableCreatures
		// packs, but those only fire on CHANGE — the INITIAL town state isn't
		// broadcast, so a fresh client has no idea what its town can recruit.
		// This reads it straight from the engine. Drives the `recruit` verb.
		const int townId = static_cast<int>(req["townId"].Integer());
		JsonNode resp;
		resp["type"].String() = "WrapperTown";
		resp["townId"].Integer() = townId;
		const auto * obj = map.getObject(ObjectInstanceID(townId));
		const auto * town = dynamic_cast<const CGTownInstance *>(obj);
		if (!town)
		{
			resp["error"].String() = "no such town";
			sendRawJson(sock, resp);
			return;
		}
		resp["name"].String() = town->getNameTranslated();
		if (town->getOwner().isValidPlayer())
			resp["owner"].Integer() = town->getOwner().getNum();
		JsonNode & blds = resp["buildings"];
		blds.Vector();
		for (const BuildingID & b : town->getBuildings())
		{
			JsonNode e; e.Integer() = b.getNum();
			blds.Vector().push_back(e);
		}
		// Dwellings: town->creatures[level] = { availableCount, [creatureIds] }.
		JsonNode & dwellings = resp["dwellings"];
		dwellings.Vector();
		for (size_t level = 0; level < town->creatures.size(); level++)
		{
			const auto & slot = town->creatures[level];
			const ui32 available = slot.first;
			if (slot.second.empty()) continue;
			JsonNode d;
			d["level"].Integer() = static_cast<int64_t>(level);
			d["available"].Integer() = available;
			JsonNode & cres = d["creatures"];
			cres.Vector();
			for (const CreatureID & cid : slot.second)
			{
				const CCreature * cre = cid.toCreature();
				if (!cre) continue;
				JsonNode c;
				c["id"].Integer() = cid.getNum();
				c["name"].String() = cre->getNamePluralTranslated();
				c["goldCost"].Integer() = cre->getRecruitCost(GameResID::GOLD);
				cres.Vector().push_back(c);
			}
			dwellings.Vector().push_back(d);
		}
		sendRawJson(sock, resp);
		return;
	}

	if (queryType == "WrapperSetAutoResolve")
	{
		// Toggle full server-side auto-resolve: when on, the server's battle AI
		// plays EVERY side (including the wrapper's own), so battles resolve
		// without tactical input. When off (default), only neutral stacks are
		// driven and the wrapper plays its own side.
		const bool enabled = req["enabled"].Bool();
		server.gh->battleAutoResolve = enabled;
		JsonNode resp;
		resp["type"].String() = "WrapperAutoResolve";
		resp["enabled"].Bool() = enabled;
		sendRawJson(sock, resp);
		return;
	}

	if (queryType == "WrapperForceAIAttack")
	{
		// DEBUG/TEST: arm a one-shot — the next hosted-AI player's turn forces an
		// attack on the human's hero through the real startBattle path, so the
		// cross-player battle deferral fires deterministically (Phase 4 testing).
		server.gh->debugForceAIAttack = true;
		JsonNode resp;
		resp["type"].String() = "WrapperForceAIAttackArmed";
		resp["armed"].Bool() = true;
		sendRawJson(sock, resp);
		return;
	}

	if (queryType == "WrapperPopQuery")
	{
		// Hard escape: forcibly remove a query from the QueriesProcessor.
		// Used when a player disconnects mid-query (especially CBattleQuery)
		// and the game is stuck waiting on them. Pops the query unconditionally.
		// NOT a clean way to end battles — the engine may be in an
		// inconsistent state afterward — but it unblocks games that would
		// otherwise be lost. Treat as an admin override.
		const int qidInt = static_cast<int>(req["queryID"].Integer());
		JsonNode resp;
		resp["type"].String() = "WrapperPopQueryResult";
		resp["queryID"].Integer() = qidInt;
		auto q = server.gh->queries->getQuery(QueryID(qidInt));
		if (!q)
		{
			resp["found"].Bool() = false;
			sendRawJson(sock, resp);
			return;
		}
		resp["found"].Bool() = true;
		resp["description"].String() = q->toString();
		try {
			server.gh->queries->popQuery(q);
			resp["popped"].Bool() = true;
		}
		catch (const std::exception & e)
		{
			resp["popped"].Bool() = false;
			resp["error"].String() = e.what();
		}
		sendRawJson(sock, resp);
		return;
	}

	if (queryType == "WrapperListQueries")
	{
		// Surface all pending engine-side queries (battle, dialog, level-up, etc.)
		// so agents can see what's blocking them and respond via QueryReply.
		JsonNode resp;
		resp["type"].String() = "WrapperQueries";
		JsonNode & arr = resp["queries"];
		arr.Vector();
		const auto all = server.gh->queries->allQueries();
		for (const auto & q : all)
		{
			if (!q) continue;
			JsonNode entry;
			entry["queryID"].Integer() = q->queryID.getNum();
			JsonNode & players = entry["players"];
			players.Vector();
			for (const auto & p : q->players)
			{
				JsonNode pn;
				pn.Integer() = p.getNum();
				players.Vector().push_back(pn);
			}
			entry["description"].String() = q->toString();
			arr.Vector().push_back(entry);
		}
		sendRawJson(sock, resp);
		return;
	}

	if (queryType == "WrapperQueryPath")
	{
		const int heroId = static_cast<int>(req["heroId"].Integer());
		const int dx = static_cast<int>(req["x"].Integer());
		const int dy = static_cast<int>(req["y"].Integer());
		const int dz = req["z"].isNumber() ? static_cast<int>(req["z"].Integer()) : 0;
		const int3 dest(dx, dy, dz);

		JsonNode resp;
		resp["type"].String() = "WrapperPath";
		resp["heroId"].Integer() = heroId;
		resp["dest"]["x"].Integer() = dx;
		resp["dest"]["y"].Integer() = dy;
		resp["dest"]["z"].Integer() = dz;

		const auto * hero = server.gh->gs->getHero(ObjectInstanceID(heroId));
		if (!hero)
		{
			resp["reachable"].Bool() = false;
			resp["error"].String() = "hero not found";
			sendRawJson(sock, resp);
			return;
		}
		// Bounds-check the destination BEFORE pathfinding. getPath indexes the
		// paths array by coordinate; an out-of-bounds dest (e.g. negative coords
		// from a wrapper client scouting "off the edge") is an out-of-range
		// access that SEGFAULTS the server (not a catchable C++ exception). This
		// crashed a live game when an agent did `goto to=-3,1,0`.
		if (!map.isInTheMap(dest))
		{
			resp["reachable"].Bool() = false;
			resp["error"].String() = "destination out of map bounds";
			sendRawJson(sock, resp);
			return;
		}
		resp["start"]["x"].Integer() = hero->pos.x;
		resp["start"]["y"].Integer() = hero->pos.y;
		resp["start"]["z"].Integer() = hero->pos.z;

		try
		{
			CPathsInfo pathsInfo(int3(map.width, map.height, map.levels()), hero);
			auto config = std::make_shared<SingleHeroPathfinderConfig>(pathsInfo, server.gh->gameInfo(), hero);
			// Default pathfinder stops at guards (treats them as endpoints).
			// For wrapper queries we want to see the full path so the agent can
			// see what they'd cross. The agent decides whether to actually step.
			config->options.ignoreGuards = true;
			server.gh->gs->calculatePaths(config);

			CGPath path;
			const bool reachable = pathsInfo.getPath(path, dest);
			logNetwork->info("[Pathfinder] hero=%d at (%d,%d,%d) -> (%d,%d,%d): reachable=%d, nodes=%d",
				heroId, hero->pos.x, hero->pos.y, hero->pos.z, dx, dy, dz,
				reachable ? 1 : 0, (int)path.nodes.size());
			resp["reachable"].Bool() = reachable;
			if (reachable && !path.nodes.empty())
			{
				// path.nodes is in reverse (dest -> start); front() is destination.
				const auto & destNode = path.nodes.front();
				resp["cost"].Float() = destNode.cost;
				resp["turnsToReach"].Integer() = destNode.turns;
				resp["movePointsAfter"].Integer() = destNode.moveRemains;
				JsonNode & tiles = resp["tiles"];
				tiles.Vector();
				static const char * actionNames[] = {
					"UNKNOWN", "EMBARK", "DISEMBARK", "NORMAL", "BATTLE",
					"VISIT", "BLOCKING_VISIT",
					"TELEPORT_NORMAL", "TELEPORT_BLOCKING_VISIT", "TELEPORT_BATTLE",
				};
				static const char * layerNames[] = {
					"WRONG", "AUTO", "LAND", "SAIL", "WATER", "AIR",
				};
				// Path nodes use visitablePos coordinates (the tile the
				// PATHFINDER reasons about). The engine's MoveHero handler
				// expects coordinates in the hero's actual `pos` space, which
				// differs by 1 in some axis for objects with a footprint > 1
				// (heroes, towns, boats). Convert per upstream's
				// HeroMovementController: int3 coord = h->convertFromVisitablePos(node.coord).
				for (auto it = path.nodes.rbegin(); it != path.nodes.rend(); ++it)
				{
					if (it->coord == hero->visitablePos()) continue; // skip start (matches upstream)
					int3 movePos = hero->convertFromVisitablePos(it->coord);
					JsonNode entry;
					entry["x"].Integer() = movePos.x;
					entry["y"].Integer() = movePos.y;
					entry["z"].Integer() = movePos.z;
					// Also expose the visitable position so agents can correlate
					// with object positions (e.g., the tile a monster guards
					// from is its visitablePos, not its convertFromVisitablePos).
					entry["visitablePos"]["x"].Integer() = it->coord.x;
					entry["visitablePos"]["y"].Integer() = it->coord.y;
					entry["visitablePos"]["z"].Integer() = it->coord.z;
					entry["turn"].Integer() = it->turns;
					entry["movePointsAfter"].Integer() = it->moveRemains;
					const int actionIdx = static_cast<int>(it->action);
					const int actionMax = static_cast<int>(sizeof(actionNames) / sizeof(actionNames[0]));
					entry["action"].String() = (actionIdx >= 0 && actionIdx < actionMax)
						? actionNames[actionIdx] : "UNKNOWN";
					const int layerIdx = static_cast<int>(it->layer);
					const int layerMax = static_cast<int>(sizeof(layerNames) / sizeof(layerNames[0]));
					entry["layer"].String() = (layerIdx >= 0 && layerIdx < layerMax)
						? layerNames[layerIdx] : "WRONG";
					tiles.Vector().push_back(entry);
				}
			}
		}
		catch (const std::exception & e)
		{
			resp["reachable"].Bool() = false;
			resp["error"].String() = std::string("pathfinder failed: ") + e.what();
		}
		sendRawJson(sock, resp);
		return;
	}

	if (queryType == "WrapperQueryRegion")
	{
		const int x0 = static_cast<int>(req["x0"].Integer());
		const int y0 = static_cast<int>(req["y0"].Integer());
		const int x1 = static_cast<int>(req["x1"].Integer());
		const int y1 = static_cast<int>(req["y1"].Integer());
		const int z = req["z"].isNumber() ? static_cast<int>(req["z"].Integer()) : 0;

		JsonNode resp;
		resp["type"].String() = "WrapperMapRegion";
		resp["x0"].Integer() = x0;
		resp["y0"].Integer() = y0;
		resp["x1"].Integer() = x1;
		resp["y1"].Integer() = y1;
		resp["z"].Integer() = z;
		JsonNode & arr = resp["tiles"];
		arr.Vector();

		for (int y = y0; y <= y1; y++)
		{
			for (int x = x0; x <= x1; x++)
			{
				const int3 t(x, y, z);
				if (!map.isInTheMap(t)) continue;
				const TerrainTile & tile = map.getTile(t);
				JsonNode entry;
				entry["x"].Integer() = x;
				entry["y"].Integer() = y;
				entry["z"].Integer() = z;
				entry["terrain"].Integer() = tile.getTerrainID().getNum();
				entry["blocked"].Bool() = tile.blocked();
				entry["visitable"].Bool() = tile.visitable();
				const auto topObj = tile.topVisitableObj();
				if (topObj.getNum() >= 0)
				{
					entry["topObject"].Integer() = topObj.getNum();
					if (const auto * obj = server.gh->gs->getObj(topObj))
					{
						entry["objType"].Integer() = obj->ID.getNum();
						entry["objSubType"].Integer() = obj->subID.getNum();
						if (obj->getOwner().isValidPlayer())
							entry["objOwner"].Integer() = obj->getOwner().getNum();
					}
				}
				const int3 guard = map.guardingCreaturePosition(t);
				if (map.isInTheMap(guard))
				{
					JsonNode & g = entry["guardedBy"];
					g.Struct();
					g["x"].Integer() = guard.x;
					g["y"].Integer() = guard.y;
					g["z"].Integer() = guard.z;
				}
				arr.Vector().push_back(entry);
			}
		}
		sendRawJson(sock, resp);
		return;
	}

	JsonNode err;
	err["type"].String() = "WrapperQueryError";
	err["queryType"].String() = queryType;
	err["error"].String() = "unknown query type";
	sendRawJson(sock, err);
}

void JsonAdapter::enrichOutbound(const std::shared_ptr<GameConnection> & game, const CPack & pack, JsonNode & out)
{
	// HeroVisit: add the hero's current position. The engine emits these at
	// game start for each starting hero (with `starting=true`), which is how
	// wrapper clients first learn where their heroes are on day 1.
	if (auto * hv = dynamic_cast<const HeroVisit *>(&pack))
	{
		if (server.gh && server.gh->gs)
		{
			const auto * hero = server.gh->gs->getHero(hv->heroId);
			if (hero)
			{
				JsonNode & pos = out["position"];
				pos.Struct();
				pos["x"].Integer() = hero->pos.x;
				pos["y"].Integer() = hero->pos.y;
				pos["z"].Integer() = hero->pos.z;
			}
		}
	}

	// LobbyStartGame echo: tell THIS connection which player slots it owns,
	// plus the current turn state (matters for LOAD_GAME — the engine doesn't
	// re-broadcast NewTurn/PlayerStartsTurn after load, so without these
	// fields a freshly-loaded client doesn't know whose turn it is).
	if (dynamic_cast<const LobbyStartGame *>(&pack))
	{
		const auto players = server.getAllClientPlayers(game->connectionID);
		JsonNode & arr = out["yourPlayers"];
		arr.Vector();
		for (PlayerColor color : players)
		{
			JsonNode entry;
			entry.Integer() = color.getNum();
			arr.Vector().push_back(entry);
		}

		// Current turn-state. server.gh->gs->day exists by the time
		// LobbyStartGame echoes (gs has been loaded). actingPlayers is the
		// set of players currently making turns — single for normal play,
		// multiple under simturns. TurnOrderProcessor's set is private, so
		// we probe each PlayerColor via isPlayerMakingTurn().
		try
		{
			if (server.gh && server.gh->gs)
			{
				out["day"].Integer() = server.gh->gs->day;
				JsonNode & acting = out["actingPlayers"];
				acting.Vector();
				if (server.gh->turnOrder)
				{
					for (int i = 0; i < PlayerColor::PLAYER_LIMIT_I; i++)
					{
						const PlayerColor pc(i);
						if (server.gh->turnOrder->isPlayerMakingTurn(pc))
						{
							JsonNode entry;
							entry.Integer() = i;
							acting.Vector().push_back(entry);
						}
					}
				}
			}
		}
		catch (const std::exception & e)
		{
			logNetwork->warn("[JsonAdapter] LobbyStartGame turn-state enrichment threw: %s", e.what());
		}
	}

	// FoWChange: when tiles get revealed, attach terrain + top-object info
	// for each tile. This is how the wrapper builds up a tile-by-tile picture
	// of the map as the player explores. Wrapped in try/catch since the
	// reveal can fire very early in game-start and gameState may not be fully
	// settled yet — better to lose enrichment than abort the whole pack.
	if (auto * fow = dynamic_cast<const FoWChange *>(&pack))
	{
		logNetwork->info("[JsonAdapter] enriching FoWChange (tiles=%d, gh=%d, gs=%d)",
			(int)fow->tiles.size(), server.gh ? 1 : 0, (server.gh && server.gh->gs) ? 1 : 0);
		try
		{
			if (server.gh && server.gh->gs)
			{
				const auto & map = server.gh->gs->getMap();
				JsonNode & arr = out["tileInfo"];
				arr.Vector();
				for (const int3 & t : fow->tiles)
				{
					if (!map.isInTheMap(t)) continue;
					const TerrainTile & tile = map.getTile(t);
					JsonNode entry;
					entry["x"].Integer() = t.x;
					entry["y"].Integer() = t.y;
					entry["z"].Integer() = t.z;
					entry["terrain"].Integer() = tile.getTerrainID().getNum();
					entry["blocked"].Bool() = tile.blocked();
					entry["visitable"].Bool() = tile.visitable();
					const auto topObj = tile.topVisitableObj();
					if (topObj.getNum() >= 0)
					{
						entry["topObject"].Integer() = topObj.getNum();
						// Object type — agents need this to distinguish
						// monsters, towns, mines, artifacts, resource piles…
						if (const auto * obj = server.gh->gs->getObj(topObj))
						{
							entry["objType"].Integer() = obj->ID.getNum();
							entry["objSubType"].Integer() = obj->subID.getNum();
							if (obj->getOwner().isValidPlayer())
								entry["objOwner"].Integer() = obj->getOwner().getNum();
						}
					}
					// Guard zone: if a monster guards this tile, entering it
					// triggers combat. Expose the guard's position so agents
					// can avoid or knowingly engage.
					const int3 guard = map.guardingCreaturePosition(t);
					if (map.isInTheMap(guard))
					{
						JsonNode & g = entry["guardedBy"];
						g.Struct();
						g["x"].Integer() = guard.x;
						g["y"].Integer() = guard.y;
						g["z"].Integer() = guard.z;
					}
					arr.Vector().push_back(entry);
				}
			}
		}
		catch (const std::exception & e)
		{
			logNetwork->warn("[JsonAdapter] FoWChange enrichment failed: %s", e.what());
			// Strip any partial enrichment to keep wire format consistent.
			if (out.Struct().count("tileInfo"))
				out.Struct().erase("tileInfo");
		}
	}
}

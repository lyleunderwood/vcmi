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
#include "../lib/pathfinder/CGPathNode.h"
#include "../lib/pathfinder/PathfinderOptions.h"
#include "queries/QueriesProcessor.h"
#include "queries/CQuery.h"
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
				for (auto it = path.nodes.rbegin(); it != path.nodes.rend(); ++it)
				{
					if (it->coord == hero->pos) continue; // skip start
					JsonNode entry;
					entry["x"].Integer() = it->coord.x;
					entry["y"].Integer() = it->coord.y;
					entry["z"].Integer() = it->coord.z;
					entry["turn"].Integer() = it->turns;
					entry["movePointsAfter"].Integer() = it->moveRemains;
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

	// LobbyStartGame echo: tell THIS connection which player slots it owns.
	// Each connection sees a different list because the engine assigns slots
	// per-connection during prepareToStartGame.
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

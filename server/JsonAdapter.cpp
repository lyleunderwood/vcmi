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
#include "battles/BattleProcessor.h"
#include "json/PackCodec.h"
#include "json/PackCodecRegistry.h"

#include "../lib/json/JsonNode.h"
#include "../lib/network/NetworkInterface.h"
#include "../lib/networkPacks/PacksForLobby.h"
#include "../lib/networkPacks/PacksForServer.h"
#include "../lib/networkPacks/PacksForClient.h"
#include "../lib/gameState/CGameState.h"
#include "../lib/CPlayerState.h"
#include "../lib/battle/BattleInfo.h"
#include "../lib/CStack.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapping/TerrainTile.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/mapObjects/CGTownInstance.h"
#include "../lib/mapObjects/IMarket.h"
#include "../lib/gameState/UpgradeInfo.h"
#include "../lib/mapObjects/ObjectTemplate.h"
#include "../lib/entities/faction/CTown.h"
#include "../lib/entities/building/CBuilding.h"
#include "../lib/CCreatureHandler.h"
#include "../lib/GameLibrary.h"
#include "../lib/entities/artifact/CArtHandler.h"
#include "../lib/entities/artifact/CArtifact.h"
#include "../lib/entities/artifact/CArtifactInstance.h"
#include "../lib/entities/artifact/CArtifactSet.h"
#include "../lib/mapObjects/army/CStackInstance.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/pathfinder/CGPathNode.h"
#include "../lib/pathfinder/PathfinderOptions.h"
#include "queries/QueriesProcessor.h"
#include "queries/CQuery.h"
#include "queries/BattleQueries.h"
#include "processors/TurnOrderProcessor.h"
#include "../lib/serializer/GameConnection.h"

// homam-web fork: defined in server/json/packs/BattleStart.cpp — serializes a
// BattleInfo into the same JSON shape the BattleStart codec emits, so we can
// synthesize a resume payload the wrapper's existing BattleStart handler accepts.
void emitBattleInfo(const BattleInfo & bi, JsonNode & out);

// homam-web fork: [{slot, creatureId, count}] for any army (town garrison or a
// hero). Used by WrapperQueryTown's full-state fields.
static void emitCreatureSet(const CCreatureSet & army, JsonNode & out)
{
	out.Vector();
	for (const auto & slotPair : army.Slots())
	{
		const SlotID slot = slotPair.first;
		const CCreature * cre = army.getCreature(slot);
		JsonNode e;
		e["slot"].Integer() = slot.getNum();
		e["creatureId"].Integer() = cre ? cre->getId().getNum() : -1;
		e["count"].Integer() = army.getStackCount(slot);
		out.Vector().push_back(e);
	}
}

// homam-web fork: per-slot creature upgrade options for an army (hero or town).
// Emits [{slot, to:[creatureIds], cost:[ResourceSet per target]}] for stacks that
// can upgrade RIGHT NOW (engine fillUpgradeInfo accounts for the town's upgrade
// dwellings / Hill Fort / visited upgrader). Drives the client's Upgrade button.
static void emitUpgrades(const CGameState & gs, const CArmedInstance & army, JsonNode & out)
{
	out.Vector();
	for (const auto & slotPair : army.Slots())
	{
		const SlotID slot = slotPair.first;
		const CCreature * cre = army.getCreature(slot);
		if (!cre)
			continue;
		UpgradeInfo info(cre->getId());
		gs.fillUpgradeInfo(&army, slot, info);
		if (!info.canUpgrade())
			continue;
		JsonNode e;
		e["slot"].Integer() = slot.getNum();
		e["from"].Integer() = cre->getId().getNum();
		const auto & ids = info.getAvailableUpgrades();
		const auto & costs = info.getAvailableUpgradeCosts();
		JsonNode & toArr = e["to"];     toArr.Vector();
		JsonNode & costArr = e["cost"]; costArr.Vector();
		for (size_t i = 0; i < ids.size(); i++)
		{
			JsonNode t; t.Integer() = ids[i].getNum();
			toArr.Vector().push_back(t);
			const ResourceSet rs = (i < costs.size()) ? costs[i] : ResourceSet();
			JsonNode c;
			c["gold"].Integer() = rs[GameResID::GOLD];
			c["wood"].Integer() = rs[GameResID::WOOD];
			c["ore"].Integer() = rs[GameResID::ORE];
			c["mercury"].Integer() = rs[GameResID::MERCURY];
			c["sulfur"].Integer() = rs[GameResID::SULFUR];
			c["crystal"].Integer() = rs[GameResID::CRYSTAL];
			c["gems"].Integer() = rs[GameResID::GEMS];
			costArr.Vector().push_back(c);
		}
		out.Vector().push_back(e);
	}
}

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

			// homam-web fork (#90): a player signalling their adventure interface
			// is ready (on connect/load) auto-resumes any live battle that was
			// orphaned by a reload — so a defender who disconnected mid-battle
			// keeps playing without a manual resume-battle. No-op if none.
			if (dynamic_cast<AdvInterfaceReady *>(rawPack.get()))
				resumeOrphanedBattles(connection, serverPack->player.getNum());
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

std::vector<int> JsonAdapter::resumeOrphanedBattles(const std::shared_ptr<INetworkConnection> & sock, int onlyPlayerNum)
{
	std::vector<int> resumed;
	if (!server.gh || !server.gh->gs)
		return resumed;

	// battleIDs that still have a live CBattleQuery are in-progress on a running
	// server (not a reload) — leave them alone. A reloaded battle has none.
	std::set<int> hasQuery;
	for (const auto & q : server.gh->queries->allQueries())
		if (auto bq = std::dynamic_pointer_cast<CBattleQuery>(q))
			hasQuery.insert(bq->battleID.getNum());

	// Snapshot the ids first — resumeLoadedBattle mutates engine state.
	std::vector<BattleID> ids;
	for (const auto & bi : server.gh->gs->currentBattles)
		if (bi) ids.push_back(bi->battleID);

	for (const BattleID & id : ids)
	{
		const BattleInfo * bi = server.gh->gs->getBattle(id);
		if (!bi) continue;
		if (hasQuery.count(id.getNum())) continue; // live battle, not orphaned

		if (onlyPlayerNum >= 0)
		{
			const PlayerColor want(onlyPlayerNum);
			if (bi->getSide(BattleSide::ATTACKER).color != want
				&& bi->getSide(BattleSide::DEFENDER).color != want)
				continue; // this player isn't a participant
		}

		// (1) push the battle model so the wrapper rebuilds it via its BattleStart
		// codec, BEFORE re-activating (so it has the model when BattleSetActiveStack
		// arrives). We synthesize the JSON rather than send a real BattleStart pack
		// (whose apply would duplicate the battle in gs).
		JsonNode bs;
		bs["type"].String() = "BattleStart";
		bs["battleID"].Integer() = bi->battleID.getNum();
		JsonNode info;
		emitBattleInfo(*bi, info);
		bs["info"] = info;
		sendRawJson(sock, bs);

		// (2) recreate query + re-activate flow (may emit BattleSetActiveStack).
		server.gh->battles->resumeLoadedBattle(id);
		resumed.push_back(id.getNum());
	}
	return resumed;
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
		std::set<int> seenHeroes;
		const auto emitListHero = [&](const CGHeroInstance * h, const char * inTown)
		{
			if (!h || seenHeroes.count(h->id.getNum()))
				return;
			seenHeroes.insert(h->id.getNum());
			JsonNode entry;
			entry["id"].Integer() = h->id.getNum();
			if (h->getOwner().isValidPlayer())
				entry["owner"].Integer() = h->getOwner().getNum();
			entry["position"]["x"].Integer() = h->pos.x;
			entry["position"]["y"].Integer() = h->pos.y;
			entry["position"]["z"].Integer() = h->pos.z;
			entry["movePoints"].Integer() = h->movementPointsRemaining();
			if (inTown)
				entry["inTown"].String() = inTown; // "visiting" or "garrison"
			arr.Vector().push_back(entry);
		};
		for (const auto & heroId : server.gh->gs->getMap().getHeroesOnMap())
			emitListHero(server.gh->gs->getHero(heroId), nullptr);
		// homam-web fork: also include heroes sitting IN a town. A freshly-hired
		// hero becomes the town's VISITING hero and is NOT in getHeroesOnMap(),
		// so without this it never surfaces (the tavern-hire gap).
		for (const auto & townId : server.gh->gs->getMap().getAllTowns())
		{
			const auto * t = dynamic_cast<const CGTownInstance *>(server.gh->gs->getMap().getObject(townId));
			if (!t) continue;
			emitListHero(t->getVisitingHero(), "visiting");
			emitListHero(t->getGarrisonHero(), "garrison");
		}
		sendRawJson(sock, resp);
		return;
	}

	if (queryType == "WrapperListTowns")
	{
		// homam-web fork: enumerate every town on the map (any owner, incl. neutral).
		// Mirrors WrapperListHeroes. A thin client can't enumerate towns itself —
		// town packs only fire on CHANGE, so a fresh client's town list is empty.
		// This reads the engine's precomputed town index. Drives town discovery,
		// markers, and the kingdom screen. Shape matches WrapperListHeroes:
		// position is nested {x,y,z} (the town object's anchor tile, like hero pos).
		JsonNode resp;
		resp["type"].String() = "WrapperTowns";
		JsonNode & arr = resp["towns"];
		arr.Vector();
		for (const auto & townId : map.getAllTowns())
		{
			const auto * t = dynamic_cast<const CGTownInstance *>(map.getObject(townId));
			if (!t) continue;
			JsonNode entry;
			entry["id"].Integer() = t->id.getNum();
			if (t->getOwner().isValidPlayer())
				entry["owner"].Integer() = t->getOwner().getNum();
			entry["faction"].Integer() = t->getFactionID().getNum();
			entry["name"].String() = t->getNameTranslated();
			entry["position"]["x"].Integer() = t->pos.x;
			entry["position"]["y"].Integer() = t->pos.y;
			entry["position"]["z"].Integer() = t->pos.z;
			arr.Vector().push_back(entry);
		}
		sendRawJson(sock, resp);
		return;
	}

	if (queryType == "WrapperCreatureNames")
	{
		// homam-web fork: static {id: name} table for ALL creatures, so a thin
		// client can show names for enemy/neutral stacks (and any creature id)
		// instead of #id. Never changes during a game — fetch once and cache.
		JsonNode resp;
		resp["type"].String() = "WrapperCreatureNames";
		JsonNode & names = resp["names"];
		for (const auto & c : LIBRARY->creh->objects)
			if (c)
				names[std::to_string(c->getId().getNum())].String() = c->getNameSingularTranslated();
		sendRawJson(sock, resp);
		return;
	}

	if (queryType == "WrapperSpellNames")
	{
		// homam-web fork: static {id: name} table for ALL spells (mage guild +
		// spellbook display). Never changes during a game — fetch once and cache.
		JsonNode resp;
		resp["type"].String() = "WrapperSpellNames";
		JsonNode & names = resp["names"];
		for (const auto & sp : LIBRARY->spellh->objects)
			if (sp)
				names[std::to_string(sp->getId().getNum())].String() = sp->getNameTranslated();
		sendRawJson(sock, resp);
		return;
	}

	if (queryType == "WrapperArtifactNames")
	{
		// homam-web fork: static {id: name} table for ALL artifacts, to label the
		// artifact ids in WrapperQueryHero.artifacts. Fetch once and cache.
		JsonNode resp;
		resp["type"].String() = "WrapperArtifactNames";
		JsonNode & names = resp["names"];
		for (const auto & a : LIBRARY->arth->objects)
			if (a)
				names[std::to_string(a->getId().getNum())].String() = a->getNameTranslated();
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

		// homam-web fork: full town state for the town screen (additive — the
		// fields above are unchanged). See docs/screens/town.md.
		resp["faction"].Integer() = town->getFactionID().getNum();
		resp["fortLevel"].Integer() = static_cast<int>(town->fortLevel());
		resp["hallLevel"].Integer() = town->hallLevel();
		resp["builtThisTurn"].Bool() = town->built > 0;
		resp["goldIncome"].Integer() = town->dailyIncome()[GameResID::GOLD];

		// Town's own garrison stacks (a garrisoned hero's army is reported
		// separately under garrisonHero).
		JsonNode garrison;
		emitCreatureSet(*town, garrison);
		resp["garrison"] = garrison;

		// homam-web fork: per-slot upgrade options for the town's own garrison
		// (a garrisoned/visiting hero's upgrades come via WrapperQueryHero).
		JsonNode garrisonUpgrades;
		emitUpgrades(*server.gh->gs, *town, garrisonUpgrades);
		resp["garrisonUpgrades"] = garrisonUpgrades;

		const auto emitTownHero = [&](const CGHeroInstance * h, JsonNode & out)
		{
			if (!h)
				return; // leave field absent (null)
			out["id"].Integer() = h->id.getNum();
			out["name"].String() = h->getNameTranslated();
			if (h->getOwner().isValidPlayer())
				out["owner"].Integer() = h->getOwner().getNum();
			JsonNode army;
			emitCreatureSet(*h, army);
			out["army"] = army;
		};
		emitTownHero(town->getGarrisonHero(), resp["garrisonHero"]);
		emitTownHero(town->getVisitingHero(), resp["visitingHero"]);

		// Mage guild: level + the spells available at each guild level.
		JsonNode & mageGuild = resp["mageGuild"];
		mageGuild["level"].Integer() = town->mageGuildLevel();
		JsonNode & guildSpells = mageGuild["spells"];
		guildSpells.Vector();
		for (const auto & levelSpells : town->spells)
		{
			JsonNode lvl;
			lvl.Vector();
			for (const SpellID & sp : levelSpells)
			{
				JsonNode s;
				s.Integer() = sp.getNum();
				lvl.Vector().push_back(s);
			}
			guildSpells.Vector().push_back(lvl);
		}
		// homam-web fork: `buildable` — for every building this town's faction can
		// have that ISN'T already built, its id + name + cost + build-state
		// (ALLOWED = buildable now). Ends the build-ID guess-and-check: the client
		// reads which structure ids to pass to `build`.
		static const char * buildStateNames[] = {
			"HAVE_CAPITAL", "NO_WATER", "FORBIDDEN", "ADD_MAGES_GUILD",
			"ALREADY_PRESENT", "CANT_BUILD_TODAY", "NO_RESOURCES", "ALLOWED",
			"PREREQUIRES", "MISSING_BASE", "BUILDING_ERROR", "TOWN_NOT_OWNED",
		};
		JsonNode & buildable = resp["buildable"];
		buildable.Vector();
		for (const auto & bp : town->getTown()->buildings)
		{
			const BuildingID bid = bp.first;
			const EBuildingState st = server.gh->gs->canBuildStructure(town, bid);
			if (st == EBuildingState::ALREADY_PRESENT)
				continue; // only report what's NOT built yet
			JsonNode e;
			e["id"].Integer() = bid.getNum();
			e["name"].String() = bp.second ? bp.second->getNameTranslated() : "";
			const int si = static_cast<int>(st);
			e["state"].String() = (si >= 0 && si < static_cast<int>(sizeof(buildStateNames)/sizeof(buildStateNames[0])))
				? buildStateNames[si] : "UNKNOWN";
			e["allowed"].Bool() = (st == EBuildingState::ALLOWED);
			const ResourceSet cost = town->getBuildingCost(bid);
			JsonNode & c = e["cost"];
			c["gold"].Integer() = cost[GameResID::GOLD];
			c["wood"].Integer() = cost[GameResID::WOOD];
			c["ore"].Integer() = cost[GameResID::ORE];
			c["mercury"].Integer() = cost[GameResID::MERCURY];
			c["sulfur"].Integer() = cost[GameResID::SULFUR];
			c["crystal"].Integer() = cost[GameResID::CRYSTAL];
			c["gems"].Integer() = cost[GameResID::GEMS];
			buildable.Vector().push_back(e);
		}

		// homam-web fork: marketplace exchange rates — present only when this town
		// can trade (a Marketplace is built). Rates depend on the number of
		// marketplaces the owner has, which the engine folds into
		// getMarketEfficiency(). Each entry: "give `give` units of resource `from`
		// to receive `get` units of resource `to`" (resource ids 0..6 =
		// wood,mercury,ore,sulfur,crystal,gems,gold). Drives the trade UI.
		if (town->allowsTrade(EMarketMode::RESOURCE_RESOURCE))
		{
			JsonNode & mr = resp["marketRates"];
			mr["efficiency"].Integer() = town->getMarketEfficiency();
			JsonNode & rr = mr["resourceResource"];
			rr.Vector();
			for (int from = 0; from < 7; from++)
			{
				for (int to = 0; to < 7; to++)
				{
					if (from == to)
						continue;
					int give = 0, get = 0;
					if (!town->getOffer(from, to, give, get, EMarketMode::RESOURCE_RESOURCE))
						continue;
					JsonNode e;
					e["from"].Integer() = from;
					e["to"].Integer() = to;
					e["give"].Integer() = give;
					e["get"].Integer() = get;
					rr.Vector().push_back(e);
				}
			}
		}

		sendRawJson(sock, resp);
		return;
	}

	if (queryType == "WrapperQueryHero")
	{
		// homam-web fork: full hero state — army, primary/secondary skills, mana,
		// movement, experience/level, spellbook. The pack stream only carries
		// CHANGES, so a fresh client has no hero detail; this reads it from the
		// engine (analogous to WrapperQueryTown). Artifacts deferred to a follow-up.
		const int heroId = static_cast<int>(req["heroId"].Integer());
		JsonNode resp;
		resp["type"].String() = "WrapperHero";
		resp["heroId"].Integer() = heroId;
		const auto * h = dynamic_cast<const CGHeroInstance *>(map.getObject(ObjectInstanceID(heroId)));
		if (!h)
		{
			resp["error"].String() = "no such hero";
			sendRawJson(sock, resp);
			return;
		}
		resp["name"].String() = h->getNameTranslated();
		if (h->getOwner().isValidPlayer())
			resp["owner"].Integer() = h->getOwner().getNum();
		resp["class"].Integer() = h->getHeroClassID().getNum();
		const int3 hp = h->visitablePos();
		resp["position"]["x"].Integer() = hp.x;
		resp["position"]["y"].Integer() = hp.y;
		resp["position"]["z"].Integer() = hp.z;
		resp["level"].Integer() = h->level;
		resp["experience"].Integer() = static_cast<int64_t>(h->exp);
		resp["mana"].Integer() = h->mana;
		resp["manaLimit"].Integer() = h->manaLimit();
		resp["movePoints"].Integer() = h->movementPointsRemaining();
		resp["movePointsLimit"].Integer() = h->movementPointsLimit();

		JsonNode & prim = resp["primary"];
		prim["attack"].Integer() = h->getPrimSkillLevel(PrimarySkill::ATTACK);
		prim["defence"].Integer() = h->getPrimSkillLevel(PrimarySkill::DEFENSE);
		prim["spellPower"].Integer() = h->getPrimSkillLevel(PrimarySkill::SPELL_POWER);
		prim["knowledge"].Integer() = h->getPrimSkillLevel(PrimarySkill::KNOWLEDGE);

		JsonNode & sec = resp["secondarySkills"];
		sec.Vector();
		for (const auto & sk : h->secSkills)
		{
			JsonNode e;
			e["skill"].Integer() = sk.first.getNum();
			e["level"].Integer() = sk.second;
			sec.Vector().push_back(e);
		}

		JsonNode & spellbook = resp["spellbook"];
		spellbook.Vector();
		for (const SpellID & sp : h->getSpellsInSpellbook())
		{
			JsonNode e;
			e.Integer() = sp.getNum();
			spellbook.Vector().push_back(e);
		}

		JsonNode army;
		emitCreatureSet(*h, army);
		resp["army"] = army;

		// homam-web fork: per-slot upgrade options (drives the Upgrade button).
		JsonNode upgrades;
		emitUpgrades(*server.gh->gs, *h, upgrades);
		resp["upgrades"] = upgrades;

		// homam-web fork: artifacts — worn (keyed by ArtifactPosition slot) + backpack.
		// `artifactId` is the artifact TYPE id (label via WrapperArtifactNames). A
		// `locked` worn slot is a sub-part of an equipped COMBINED artifact (its main
		// slot holds the same artifactId). Worn slot ids: 0 head,1 shoulders,2 neck,
		// 3 rHand,4 lHand,5 torso,6 rRing,7 lRing,8 feet,9-12 misc,13-16 machines,
		// 17 spellbook,18 misc5.
		JsonNode & arts = resp["artifacts"];
		JsonNode & worn = arts["worn"];
		worn.Vector();
		for (const auto & wp : h->artifactsWorn)
		{
			const CArtifactInstance * ai = wp.second.getArt();
			if (!ai)
				continue;
			JsonNode e;
			e["slot"].Integer() = wp.first.getNum();
			e["artifactId"].Integer() = ai->getTypeId().getNum();
			if (wp.second.locked)
				e["locked"].Bool() = true;
			worn.Vector().push_back(e);
		}
		JsonNode & backpack = arts["backpack"];
		backpack.Vector();
		for (const auto & slotInfo : h->artifactsInBackpack)
		{
			const CArtifactInstance * ai = slotInfo.getArt();
			JsonNode e;
			e.Integer() = ai ? ai->getTypeId().getNum() : -1;
			backpack.Vector().push_back(e);
		}

		sendRawJson(sock, resp);
		return;
	}

	if (queryType == "WrapperQueryKingdom")
	{
		// homam-web fork: a player's kingdom overview — towns, heroes, resources,
		// and total daily income. Closes the "no way to enumerate my towns/heroes
		// with state at game start" gap (the pack stream only carries changes).
		// `player` defaults to the first human player.
		JsonNode resp;
		resp["type"].String() = "WrapperKingdom";
		PlayerColor player = PlayerColor::CANNOT_DETERMINE;
		if (req["player"].isNumber())
			player = PlayerColor(static_cast<int32_t>(req["player"].Integer()));
		else
		{
			for (const auto & pi : server.gh->gameInfo().getStartInfo()->playerInfos)
			{
				const auto * ps = server.gh->gameInfo().getPlayerState(pi.first, false);
				if (ps && ps->isHuman()) { player = pi.first; break; }
			}
		}
		const auto * ps = server.gh->gameInfo().getPlayerState(player, false);
		if (!ps)
		{
			resp["error"].String() = "no such player";
			sendRawJson(sock, resp);
			return;
		}
		resp["player"].Integer() = player.getNum();

		JsonNode & townsArr = resp["towns"];
		townsArr.Vector();
		for (const auto * t : ps->getTowns())
		{
			JsonNode e;
			e["id"].Integer() = t->id.getNum();
			e["name"].String() = t->getNameTranslated();
			e["faction"].Integer() = t->getFactionID().getNum();
			e["position"]["x"].Integer() = t->visitablePos().x;
			e["position"]["y"].Integer() = t->visitablePos().y;
			e["position"]["z"].Integer() = t->visitablePos().z;
			townsArr.Vector().push_back(e);
		}

		// Daily income: sum every ownable object (towns + mines + gold-producing
		// heroes), matching the engine's own Statistic::getIncome loop — but for
		// the full ResourceSet, not just gold. Town-only accumulation previously
		// dropped all mine resource income (wood/ore/etc.).
		ResourceSet income;
		for (const auto * obj : ps->getOwnedObjects())
			if (const auto * own = obj->asOwnable())
				income += own->dailyIncome();

		JsonNode & heroesArr = resp["heroes"];
		heroesArr.Vector();
		for (const auto * hh : ps->getHeroes())
		{
			JsonNode e;
			e["id"].Integer() = hh->id.getNum();
			e["name"].String() = hh->getNameTranslated();
			e["level"].Integer() = hh->level;
			e["position"]["x"].Integer() = hh->visitablePos().x;
			e["position"]["y"].Integer() = hh->visitablePos().y;
			e["position"]["z"].Integer() = hh->visitablePos().z;
			heroesArr.Vector().push_back(e);
		}

		const auto emitResources = [](const ResourceSet & rs, JsonNode & out)
		{
			out["wood"].Integer() = rs[GameResID::WOOD];
			out["mercury"].Integer() = rs[GameResID::MERCURY];
			out["ore"].Integer() = rs[GameResID::ORE];
			out["sulfur"].Integer() = rs[GameResID::SULFUR];
			out["crystal"].Integer() = rs[GameResID::CRYSTAL];
			out["gems"].Integer() = rs[GameResID::GEMS];
			out["gold"].Integer() = rs[GameResID::GOLD];
		};
		emitResources(ps->resources, resp["resources"]);
		emitResources(income, resp["dailyIncome"]);

		sendRawJson(sock, resp);
		return;
	}

	if (queryType == "WrapperQueryNav")
	{
		// homam-web fork: a NAVIGATION GRAPH for a hero — far easier for an agent
		// to reason about than a raw tile map. Returns (1) POIs: objects the hero's
		// owner has seen (towns/heroes/mines/resources/artifacts/monsters/
		// dwellings/garrisons), and (2) FRONTIERS: explored tiles adjacent to fog,
		// clustered to bucket centers (explore-here nodes). Every node carries
		// reachability + turns from the engine pathfinder, so the agent can just
		// pick a node and `march` to it. FoW-faithful (only visible objects /
		// explored frontier tiles).
		const int heroId = static_cast<int>(req["heroId"].Integer());
		JsonNode resp;
		resp["type"].String() = "WrapperNav";
		resp["heroId"].Integer() = heroId;
		const auto * hero = server.gh->gs->getHero(ObjectInstanceID(heroId));
		if (!hero)
		{
			resp["error"].String() = "no such hero";
			sendRawJson(sock, resp);
			return;
		}
		const PlayerColor owner = hero->getOwner();
		const auto & nmap = server.gh->gs->getMap();
		// The hero's own army strength, so a caller can compare against a POI's
		// guardStrength and judge whether to engage.
		resp["heroArmyStrength"].Integer() = static_cast<int64_t>(hero->getArmyStrength());

		CPathsInfo navPaths(int3(nmap.width, nmap.height, nmap.levels()), hero);
		auto navCfg = std::make_shared<SingleHeroPathfinderConfig>(navPaths, server.gh->gameInfo(), hero);
		navCfg->options.ignoreGuards = true;
		server.gh->gs->calculatePaths(navCfg);

		// Reachability of a tile, or (for blocked object tiles) its nearest
		// reachable neighbour. Returns reachable + turns (-1 if not).
		const auto reachOf = [&](const int3 & t, bool & reachable, int & turns)
		{
			reachable = false; turns = -1;
			const CGPathNode * n = navPaths.getNode(t);
			if (n && n->theNodeBefore) { reachable = true; turns = n->turns; return; }
			int best = std::numeric_limits<int>::max();
			for (const int3 & d : int3::getDirs())
			{
				const int3 c = t + d;
				if (!nmap.isInTheMap(c)) continue;
				const CGPathNode * nn = navPaths.getNode(c);
				if (nn && nn->theNodeBefore) { reachable = true; best = std::min(best, static_cast<int>(nn->turns)); }
			}
			if (reachable) turns = best;
		};

		// (1) POIs — visible objects of interest.
		JsonNode & pois = resp["pois"];
		pois.Vector();

		// homam-web fork: FAITHFUL enemy intel (no cheating). A human only learns
		// an enemy/guard army as an APPROXIMATE quantity BAND per stack — the
		// right-click quick-info popup ("Pack"=10-19, "Lots"=20-49, … via
		// CGCreature::getPopupText → CStackInstance::getQuantityID). Exact
		// getArmyStrength is info the player cannot see, so we emit only the bands
		// the engine itself surfaces. `out` must already be a Vector().
		const auto appendArmyBands = [](const CArmedInstance * army, JsonNode & out)
		{
			if (!army) return;
			for (const auto & slot : army->Slots())
			{
				const auto & st = slot.second;
				if (!st || !st->getCreature()) continue;
				JsonNode e;
				e["creature"].String() = st->getCreature()->getNamePluralTranslated();
				e["count"].String() = CCreature::getQuantityRangeStringForId(st->getQuantityID());
				out.Vector().push_back(e);
			}
		};
		for (const auto * obj : nmap.getObjects())
		{
			if (!obj || obj->id == hero->id) continue;
			const auto oid = obj->ID;
			const bool interesting =
				oid == Obj::TOWN || oid == Obj::HERO || oid == Obj::MINE ||
				oid == Obj::RESOURCE || oid == Obj::ARTIFACT || oid == Obj::MONSTER ||
				oid == Obj::CREATURE_GENERATOR1 || oid == Obj::CREATURE_GENERATOR4 ||
				oid == Obj::GARRISON;
			if (!interesting) continue;
			if (!server.gh->gs->isVisibleFor(obj, owner)) continue;
			const int3 p = obj->visitablePos();
			bool r; int t; reachOf(p, r, t);
			JsonNode e;
			e["id"].Integer() = obj->id.getNum();
			e["type"].Integer() = oid.getNum();
			e["name"].String() = obj->getObjectName();
			const PlayerColor objOwner = obj->getOwner();
			if (objOwner.isValidPlayer())
				e["owner"].Integer() = objOwner.getNum();
			e["x"].Integer() = p.x;
			e["y"].Integer() = p.y;
			e["z"].Integer() = p.z;
			e["reachable"].Bool() = r;
			if (r) e["turns"].Integer() = t;
			// homam-web fork: surface GUARDS so the agent isn't blind-marched into
			// a fight. A POI is "contested" if (a) a wandering monster guards its
			// approach (getGuardingCreatures) or (b) it's an enemy-owned object
			// (taking it can trigger combat with its defenders). Report guard
			// strength when known so the agent can judge whether to engage.
			const auto guards = server.gh->gameInfo().getGuardingCreatures(p);
			const bool enemyOwned = objOwner.isValidPlayer() && objOwner != owner;
			if (!guards.empty() || enemyOwned)
			{
				e["guarded"].Bool() = true;
				if (!guards.empty())
				{
					e["guardName"].String() = guards.front()->getObjectName();
					// Approximate per-stack bands of the wandering monster(s) guarding
					// the approach — faithful to the right-click quick-info popup.
					JsonNode & ga = e["guardArmy"]; ga.Vector();
					for (const auto * g : guards)
						appendArmyBands(dynamic_cast<const CArmedInstance *>(g), ga);
				}
				if (enemyOwned)
					e["enemyOwned"].Bool() = true;
			}
			// The defender's OWN army (enemy hero/town garrison, or a wandering
			// monster as a POI) — approximate bands only.
			if (const auto * armed = dynamic_cast<const CArmedInstance *>(obj))
				if (enemyOwned || oid == Obj::MONSTER)
				{
					JsonNode & da = e["army"]; da.Vector();
					appendArmyBands(armed, da);
				}
			pois.Vector().push_back(e);
		}

		// (2) FRONTIERS — explored tiles adjacent to fog, one representative per
		// bucket (coalesced explore-here nodes), reachable only.
		const int B = 6;
		std::map<std::pair<int,int>, int3> bucketRep;
		std::map<std::pair<int,int>, int> bucketBestDist;
		for (int z = 0; z < nmap.levels(); z++)
			for (int x = 0; x < nmap.width; x++)
				for (int y = 0; y < nmap.height; y++)
				{
					const int3 t(x, y, z);
					if (!server.gh->gs->isVisibleFor(t, owner)) continue;
					bool isFrontier = false;
					for (const int3 & d : int3::getDirs())
					{
						const int3 c = t + d;
						if (nmap.isInTheMap(c) && !server.gh->gs->isVisibleFor(c, owner)) { isFrontier = true; break; }
					}
					if (!isFrontier) continue;
					bool r; int tn; reachOf(t, r, tn);
					if (!r) continue;
					const int cx = (x / B) * B + B / 2;
					const int cy = (y / B) * B + B / 2;
					const auto key = std::make_pair(cx * 1000 + z, cy);
					const int dist = std::abs(x - cx) + std::abs(y - cy);
					auto it = bucketBestDist.find(key);
					if (it == bucketBestDist.end() || dist < it->second)
					{
						bucketBestDist[key] = dist;
						bucketRep[key] = t;
					}
				}
		JsonNode & frontiers = resp["frontiers"];
		frontiers.Vector();
		for (const auto & kv : bucketRep)
		{
			const int3 t = kv.second;
			bool r; int tn; reachOf(t, r, tn);
			JsonNode e;
			e["x"].Integer() = t.x;
			e["y"].Integer() = t.y;
			e["z"].Integer() = t.z;
			e["reachable"].Bool() = r;
			if (r) e["turns"].Integer() = tn;
			frontiers.Vector().push_back(e);
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
		// attack on the human (their town if {town:true}, else their hero) through
		// the real visit path, so the cross-player battle deferral fires
		// deterministically (Phase 4 testing).
		server.gh->debugForceAIAttack = true;
		server.gh->debugForceAIAttackTown = req["town"].Bool(); // false if absent
		JsonNode resp;
		resp["type"].String() = "WrapperForceAIAttackArmed";
		resp["armed"].Bool() = true;
		resp["town"].Bool() = server.gh->debugForceAIAttackTown;
		sendRawJson(sock, resp);
		return;
	}

	if (queryType == "WrapperQueryBattle")
	{
		// homam-web fork: a join/bootstrap SNAPSHOT of the current battle for the
		// battle screen. Deltas already arrive as normal battle packs
		// (BattleStackMoved/BattleAttack/…); this is just what a fresh client needs
		// to render before the first delta. Base shape (battleId, round, sides,
		// obstacles, siege, battlefield, tile, tactics) reuses emitBattleInfo; the
		// `stacks` array is REPLACED with live per-unit runtime that the
		// BattleStart codec (starting-state only) doesn't carry. See
		// docs/screens/battle.md. Optional `battleId` selects among multiple.
		JsonNode resp;
		resp["type"].String() = "WrapperBattle";
		const BattleInfo * bi = nullptr;
		if (server.gh && server.gh->gs)
		{
			if (req["battleId"].isNumber())
				bi = server.gh->gs->getBattle(BattleID(static_cast<int32_t>(req["battleId"].Integer())));
			else if (!server.gh->gs->currentBattles.empty() && server.gh->gs->currentBattles.front())
				bi = server.gh->gs->currentBattles.front().get();
		}
		if (!bi)
		{
			resp["error"].String() = "no active battle";
			sendRawJson(sock, resp);
			return;
		}

		// Base shape (reuses the BattleStart serializer); then enrich `stacks`.
		JsonNode info;
		emitBattleInfo(*bi, info);
		info["activeUnit"].Integer() = bi->activeStack;

		// homam-web fork: ACTION AFFORDANCES for the active stack so the wrapper can
		// offer `battle-attack target=<id>` with NO hand-computed from-hex (the #1
		// manual-combat pain — VCMI's offset-hex adjacency is unintuitive). BattleInfo
		// is itself a CBattleInfoCallback, so we use the engine's own reachability.
		// `activeReachable` = hexes the active stack can move to; per-enemy
		// `attackFromHex` (set in the stacks loop) = a reachable hex adjacent to it.
		const CStack * activeStk = nullptr;
		std::set<int> reachSet;
		for (const auto & sp : bi->stacks)
			if (sp && static_cast<int32_t>(sp->unitId()) == bi->activeStack) { activeStk = sp.get(); break; }
		if (activeStk && activeStk->alive())
		{
			JsonNode & rj = info["activeReachable"]; rj.Vector();
			for (const auto & h : bi->battleGetAvailableHexes(activeStk, false))
			{
				reachSet.insert(h.toInt());
				JsonNode e; e.Integer() = h.toInt(); rj.Vector().push_back(e);
			}
		}

		// Build a fresh stacks array with live state. NB: JsonNode::Vector() does
		// NOT clear an already-populated vector (emitBattleInfo filled the
		// starting-state shape), so build a new node and overwrite the key.
		JsonNode stacks;
		stacks.Vector();
		for (const auto & stPtr : bi->stacks)
		{
			if (!stPtr) continue;
			const CStack & st = *stPtr;
			JsonNode e;
			e["unitId"].Integer() = static_cast<int64_t>(st.unitId());
			e["side"].Integer() = (st.unitSide() == BattleSide::DEFENDER) ? 1 : 0;
			e["creatureId"].Integer() = (st.unitType() ? st.unitType()->getId().getNum() : -1);
			e["count"].Integer() = st.getCount();
			e["baseCount"].Integer() = static_cast<int64_t>(st.unitBaseAmount());
			e["firstHPleft"].Integer() = st.getFirstHPleft();
			e["position"].Integer() = st.getPosition().toInt();
			e["alive"].Bool() = st.alive();
			e["doubleWide"].Bool() = st.doubleWide();
			e["canShoot"].Bool() = st.canShoot();
			e["shots"].Integer() = st.shots.available();
			e["defended"].Bool() = st.defended();
			e["movedThisRound"].Bool() = st.moved();
			e["retaliationsLeft"].Integer() = st.counterAttacks.available();
			e["isActive"].Bool() = (bi->activeStack == static_cast<int32_t>(st.unitId()));
			// homam-web fork: for ENEMY stacks (opposite side from the active stack),
			// surface what the active stack can do to it THIS turn — no hex math needed:
			//   meleeable     : can the active stack reach an adjacent hex and attack?
			//   attackFromHex : that hex (feed straight to `battle-attack from=`)
			//   shootableByActive : active is a shooter with ammo (ranged option)
			if (activeStk && st.alive() && st.unitSide() != activeStk->unitSide())
			{
				int attackFrom = -1;
				const BattleHex stPos = st.getPosition();
				const int activePos = activeStk->getPosition().toInt();
				const auto & nbrs = stPos.getNeighbouringTiles();
				// already adjacent? attack from current hex (no move).
				for (const auto & nb : nbrs)
					if (nb.toInt() == activePos) { attackFrom = activePos; break; }
				if (attackFrom < 0)
					for (const auto & nb : nbrs)
						if (reachSet.count(nb.toInt())) { attackFrom = nb.toInt(); break; }
				e["meleeable"].Bool() = (attackFrom >= 0);
				if (attackFrom >= 0) e["attackFromHex"].Integer() = attackFrom;
				e["shootableByActive"].Bool() = activeStk->canShoot() && activeStk->shots.available() > 0;
			}
			stacks.Vector().push_back(e);
		}
		info["stacks"] = stacks; // overwrite the starting-state shape

		// Fold the enriched battle info into the response.
		resp["battle"] = info;
		sendRawJson(sock, resp);
		return;
	}

	if (queryType == "WrapperResumeBattle")
	{
		// homam-web fork (#90): a battle restored from a save lives in gs->currentBattles
		// but has no CBattleQuery and was never re-broadcast — the human who
		// disconnected mid-defense reconnects ACTIVE_FREE with no battle model.
		// For each orphaned loaded battle: (1) send a BattleStart-shaped payload so
		// the wrapper rebuilds the battle model via its existing BattleStart handler,
		// then (2) recreate the query + re-activate the flow on the server. Order
		// matters: the wrapper must have the model before BattleSetActiveStack arrives.
		JsonNode resp;
		resp["type"].String() = "WrapperResumeBattleResult";
		JsonNode & arr = resp["resumed"];
		arr.Vector();
		for (int id : resumeOrphanedBattles(sock, -1))
		{
			JsonNode entry;
			entry.Integer() = id;
			arr.Vector().push_back(entry);
		}
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
			if (!reachable)
			{
				// homam-web fork: the pathfinder is FoW-limited — it will NOT route
				// to a tile the hero's owner hasn't explored yet, even if the
				// wrapper's omniscient map (WrapperQueryRegion reads engine truth)
				// shows it as open. Distinguish that from a genuinely blocked/
				// out-of-range tile so the caller isn't misled by "not reachable".
				resp["reason"].String() = server.gh->gs->isVisibleFor(dest, hero->getOwner())
					? "blocked or out of reach (tile is explored but no path exists)"
					: "fog of war — tile not explored yet (move closer to reveal it)";

				// homam-web fork: best-effort navigation. The exact target is
				// unreachable (FoW / blocked), but a caller "marching toward" a
				// distant goal wants to move as far in that direction as possible
				// this turn. Scan all reachable nodes (the pathfinder already
				// computed them) for the one nearest the target and return it as
				// `bestReachable` (in visitablePos space, a valid goto target).
				const CGPathNode * bestNode = nullptr;
				int3 bestTile(-1, -1, -1);
				int bestDist = std::numeric_limits<int>::max();
				for (int z2 = 0; z2 < map.levels(); z2++)
					for (int x2 = 0; x2 < map.width; x2++)
						for (int y2 = 0; y2 < map.height; y2++)
						{
							const int3 t2(x2, y2, z2);
							const CGPathNode * n = pathsInfo.getNode(t2);
							if (!n || !n->theNodeBefore)
								continue; // unreachable, or the start node itself
							const int dd = std::abs(x2 - dx) + std::abs(y2 - dy) + std::abs(z2 - dz) * 1000;
							if (dd < bestDist) { bestDist = dd; bestNode = n; bestTile = t2; }
						}
				if (bestNode)
				{
					JsonNode & br = resp["bestReachable"];
					br["x"].Integer() = bestTile.x;
					br["y"].Integer() = bestTile.y;
					br["z"].Integer() = bestTile.z;
					br["cost"].Float() = bestNode->cost;
					br["turnsToReach"].Integer() = bestNode->turns;
					br["distanceToTarget"].Integer() = bestDist;
				}
			}
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
					// homam-web fork: flag tiles in a (visible) monster's zone of
					// control — stepping onto one STARTS combat. The pathfinder uses
					// ignoreGuards=true so it routes THROUGH guards without marking a
					// BATTLE action; this lets goto stop before a guarded tile rather
					// than blind-engaging. Gated on visibility (faithful: unseen
					// guards remain a surprise, as in-game).
					const auto tileGuards = server.gh->gameInfo().getGuardingCreatures(it->coord);
					if (!tileGuards.empty())
					{
						bool guardVisible = false;
						JsonNode guardArmy; guardArmy.Vector();
						for (const auto * g : tileGuards)
						{
							if (!server.gh->gs->isVisibleFor(g, hero->getOwner())) continue;
							guardVisible = true;
							if (const auto * ai = dynamic_cast<const CArmedInstance *>(g))
								for (const auto & slot : ai->Slots())
								{
									const auto & st = slot.second;
									if (!st || !st->getCreature()) continue;
									JsonNode b;
									b["creature"].String() = st->getCreature()->getNamePluralTranslated();
									b["count"].String() = CCreature::getQuantityRangeStringForId(st->getQuantityID());
									guardArmy.Vector().push_back(b);
								}
						}
						if (guardVisible)
						{
							entry["guarded"].Bool() = true;
							entry["guardArmy"] = guardArmy;
						}
					}
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

	if (queryType == "WrapperQueryObjects")
	{
		// Full object list (decorations + gameplay), each once, at its anchor (bottom-right) tile.
		JsonNode resp;
		resp["type"].String() = "WrapperObjects";
		JsonNode & arr = resp["objects"];
		arr.Vector();
		for (const auto & obj : map.objects)
		{
			if (!obj) continue;
			const int3 p = obj->anchorPos();
			JsonNode e;
			e["id"].Integer() = obj->id.getNum();
			e["x"].Integer() = p.x;
			e["y"].Integer() = p.y;
			e["z"].Integer() = p.z;
			e["objType"].Integer() = obj->ID.getNum();
			e["objSubType"].Integer() = obj->subID.getNum();
			if (obj->getOwner().isValidPlayer()) e["objOwner"].Integer() = obj->getOwner().getNum();
			if (obj->appearance) e["objDef"].String() = obj->appearance->animationFile.getName();
			if (obj->appearance) e["printPriority"].Integer() = obj->appearance->printPriority;
			arr.Vector().push_back(e);
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
		// homam-web fork: optional FoW filter. When `fogForPlayer` is set, each
		// tile is tagged with `visible` (gs->isVisibleFor) so the client can
		// distinguish explored tiles from omniscient terrain it shouldn't act on.
		const bool haveFog = req["fogForPlayer"].isNumber();
		const PlayerColor fogPlayer = haveFog
			? PlayerColor(static_cast<int32_t>(req["fogForPlayer"].Integer()))
			: PlayerColor::CANNOT_DETERMINE;

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
				entry["terView"].Integer() = tile.terView;
				entry["extTileFlags"].Integer() = tile.extTileFlags;
				if (tile.hasRiver()) { entry["riverType"].Integer() = tile.getRiverID().getNum(); entry["riverDir"].Integer() = tile.riverDir; }
				if (tile.hasRoad())  { entry["roadType"].Integer() = tile.getRoadID().getNum();  entry["roadDir"].Integer() = tile.roadDir; }
				entry["blocked"].Bool() = tile.blocked();
				entry["visitable"].Bool() = tile.visitable();
				if (haveFog)
					entry["visible"].Bool() = server.gh->gs->isVisibleFor(t, fogPlayer);
				const auto topObj = tile.topVisitableObj();
				if (topObj.getNum() >= 0)
				{
					entry["topObject"].Integer() = topObj.getNum();
					if (const auto * obj = server.gh->gs->getObj(topObj))
					{
						entry["objType"].Integer() = obj->ID.getNum();
						entry["objSubType"].Integer() = obj->subID.getNum();
						if (obj->appearance) entry["objDef"].String() = obj->appearance->animationFile.getName();
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
			// homam-web fork: a JSON client only drives the HUMAN players. On
			// LOAD the engine assigns the sole connection every player slot
			// (incl. AI players and a bogus -1), but the AI players are driven
			// in-process by ServerAdventureAI — reporting them as "yours" makes
			// the wrapper treat their pending dialogs/turns as the human's
			// (e.g. a false ACTIVE_DIALOG from an AI's BlockingDialog).
			const auto * ps = (server.gh && server.gh->gs) ? server.gh->gs->getPlayerState(color, false) : nullptr;
			if (!ps || !ps->isHuman())
				continue;
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
	// TryMoveHero: the FoW reveal during movement rides inside the pack
	// (fowRevealed), NOT a separate FoWChange. Attach terrain per revealed tile
	// as fowRevealedInfo (non-breaking; codec keeps the bare fowRevealed list).
	if (auto * tmh = dynamic_cast<const TryMoveHero *>(&pack))
	{
		if (server.gh && server.gh->gs && !tmh->fowRevealed.empty())
		{
			const auto & map = server.gh->gs->getMap();
			JsonNode & arr = out["fowRevealedInfo"];
			arr.Vector();
			for (const int3 & t : tmh->fowRevealed)
			{
				if (!map.isInTheMap(t)) continue;
				const TerrainTile & tile = map.getTile(t);
				JsonNode e;
				e["x"].Integer() = t.x;
				e["y"].Integer() = t.y;
				e["z"].Integer() = t.z;
				e["terrain"].Integer() = tile.getTerrainID().getNum();
				e["terView"].Integer() = tile.terView;
				e["extTileFlags"].Integer() = tile.extTileFlags;
				e["blocked"].Bool() = tile.blocked();
				arr.Vector().push_back(e);
			}
		}
	}
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
					entry["terView"].Integer() = tile.terView;
					entry["extTileFlags"].Integer() = tile.extTileFlags;
					if (tile.hasRiver()) { entry["riverType"].Integer() = tile.getRiverID().getNum(); entry["riverDir"].Integer() = tile.riverDir; }
					if (tile.hasRoad())  { entry["roadType"].Integer() = tile.getRoadID().getNum();  entry["roadDir"].Integer() = tile.roadDir; }
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
							if (obj->appearance) entry["objDef"].String() = obj->appearance->animationFile.getName();
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

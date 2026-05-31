/*
 * LobbySetMap.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbySetMap. Canonical example for new pack codec
 * implementations.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbySetMap.ts
 *
 * Two-mode inbound (mutually exclusive):
 *
 *   mode "file" — load a known map / save by URI:
 *     { "mapInfo": { "fileURI": "Maps/X.h3m", "kind": "map" } }
 *     { "mapInfo": { "fileURI": "Saves/X",   "kind": "save" } }
 *   The server constructs a real CMapInfo by calling mapInit() / saveInit().
 *
 *   mode "random" — generate a map from CMapGenOptions:
 *     { "mapGenOpts": { ...CMapGenOptions::serializeJson shape... } }
 *   The server deserializes mapGenOpts via JsonDeserializer and synthesizes
 *   a random-map CMapInfo (isRandomMap=true + populated mapHeader), mirroring
 *   the upstream Qt client's RandomMapTab::updateMapInfoByHost. The actual
 *   tile generation runs later, in CGameState::initNewGame, triggered by
 *   StartInfo::createRandomMap() returning true (mapGenOptions != nullptr).
 *
 * Outbound mirrors the inbound shape (fileURI for "file"; opaque {} for the
 * mapGenOpts presence flag — the wrapper already has the full RMG selection
 * in Postgres, so a wire echo of the blob is not needed today).
 *
 * Seed reproducibility (#122): the fork adds two optional fields to
 * CMapGenOptions::serializeJson — {hasCustomSeed:bool, customSeed:int} — and
 * the fork-edited CGameState::initNewGame uses customSeed for the CMapGenerator
 * RNG when hasCustomSeed is true. The wrapper sends these fields when the
 * persisted #111 rmgSeed is non-null, so the lobby's #114 seed picker produces
 * reproducible maps (within a single engine build).
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForLobby.h"
#include "../../../lib/mapping/CMapInfo.h"
#include "../../../lib/mapping/CMapHeader.h"
#include "../../../lib/mapping/MapFormat.h"
#include "../../../lib/filesystem/ResourcePath.h"
#include "../../../lib/rmg/CMapGenOptions.h"
#include "../../../lib/rmg/CRmgTemplate.h"
#include "../../../lib/serializer/JsonDeserializer.h"
#include "../../../lib/texts/MetaString.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class LobbySetMapCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbySetMap"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbySetMap *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbySetMap>();

		const JsonNode & miNode = json["mapInfo"];
		if (miNode.isStruct() && miNode["fileURI"].isString())
		{
			const std::string & fileURI = miNode["fileURI"].String();
			const std::string kind = miNode["kind"].isString() ? miNode["kind"].String() : "map";

			auto mi = std::make_shared<CMapInfo>();
			try
			{
				if (kind == "save")
					mi->saveInit(ResourcePath(fileURI, EResType::SAVEGAME));
				else
					mi->mapInit(fileURI);
				pack->mapInfo = mi;
			}
			catch (const std::exception & e)
			{
				logNetwork->error("[LobbySetMap codec] failed to load %s '%s': %s",
					kind, fileURI, e.what());
				// Leave mapInfo as nullptr; downstream will refuse to start.
			}
		}

		const JsonNode & mgoNode = json["mapGenOpts"];
		if (mgoNode.isStruct() && !mgoNode.Struct().empty())
		{
			try
			{
				// 1. Deserialize CMapGenOptions from JSON via the canonical
				//    serializeJson handler (round-trips with saves).
				auto opts = std::make_shared<CMapGenOptions>();
				JsonDeserializer handler(nullptr, mgoNode);
				opts->serializeJson(handler);

				// 2. Synthesize a random-map CMapInfo, mirroring
				//    RandomMapTab::updateMapInfoByHost. Generation is gated in
				//    CVCMIServer::setMapInfo on `mi->isRandomMap && mapGenOpts`.
				auto mi = std::make_shared<CMapInfo>();
				mi->isRandomMap = true;
				mi->mapHeader = std::make_unique<CMapHeader>();
				mi->mapHeader->version = EMapFormat::VCMI;
				mi->mapHeader->name.appendLocalString(EMetaText::GENERAL_TXT, 740);
				mi->mapHeader->description.appendLocalString(EMetaText::GENERAL_TXT, 741);

				if (opts->getWaterContent() != EWaterContent::RANDOM)
					mi->mapHeader->banWaterHeroes(opts->getWaterContent() != EWaterContent::NONE);

				if (const auto * tpl = opts->getMapTemplate())
				{
					const auto desc = tpl->getDescription();
					if (!desc.empty())
						mi->mapHeader->description.appendRawString(std::string("\n\n") + desc);
					for (const auto & hero : tpl->getBannedHeroes())
						mi->mapHeader->allowedHeroes.erase(hero);
					for (const auto & hero : tpl->getEnabledHeroes())
						mi->mapHeader->allowedHeroes.insert(hero);
				}

				mi->mapHeader->difficulty = EMapDifficulty::NORMAL;
				mi->mapHeader->height = opts->getHeight();
				mi->mapHeader->width = opts->getWidth();
				mi->mapHeader->mapLayers.clear();
				for (int i = 0; i < opts->getLevels(); ++i)
				{
					if (i == 0)
						mi->mapHeader->mapLayers.push_back(MapLayerId::SURFACE);
					else if (i == 1)
						mi->mapHeader->mapLayers.push_back(MapLayerId::UNDERGROUND);
					else
						mi->mapHeader->mapLayers.push_back(MapLayerId::UNKNOWN);
				}

				const int playersToGen = opts->getMaxPlayersCount();
				mi->mapHeader->howManyTeams = playersToGen;
				for (int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
				{
					mi->mapHeader->players[i].canComputerPlay = false;
					mi->mapHeader->players[i].canHumanPlay = false;
				}
				for (const auto & player : opts->getPlayersSettings())
				{
					PlayerInfo pi;
					pi.isFactionRandom = (player.second.getStartingTown() == FactionID::RANDOM);
					pi.canComputerPlay = (player.second.getPlayerType() != EPlayerType::HUMAN);
					pi.canHumanPlay = (player.second.getPlayerType() != EPlayerType::COMP_ONLY);
					pi.team = player.second.getTeam();
					pi.hasMainTown = true;
					pi.generateHeroAtMainTown = true;
					mi->mapHeader->players[player.first.getNum()] = pi;
				}

				pack->mapInfo = mi;
				pack->mapGenOpts = opts;
			}
			catch (const std::exception & e)
			{
				logNetwork->error("[LobbySetMap codec] failed to deserialize mapGenOpts: %s", e.what());
				// Leave both mapInfo + mapGenOpts as nullptr; downstream refuses to start.
			}
		}

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbySetMap &>(pack);

		out["type"].String() = typeName();

		if (p.mapInfo)
		{
			JsonNode & mi = out["mapInfo"];
			mi.Struct();
			mi["fileURI"].String() = p.mapInfo->fileURI;
			// Signal which kind this is so the wrapper can disambiguate echoes.
			mi["kind"].String() = p.mapInfo->scenarioOptionsOfSave ? "save" : "map";
		}

		if (p.mapGenOpts)
			out["mapGenOpts"].Struct(); // opaque presence flag
	}
};

REGISTER_PACK_CODEC(LobbySetMapCodec)

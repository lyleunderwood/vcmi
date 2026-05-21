/*
 * LobbySetMap.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbySetMap. Canonical example for new pack codec
 * implementations.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbySetMap.ts
 *
 * On inbound we accept a structured `mapInfo` hint:
 *   { "fileURI": "Maps/X.h3m",  "kind": "map" }   -- new game (default)
 *   { "fileURI": "Saves/X",     "kind": "save" }  -- load saved game (.vsgm1 implied)
 *
 * Server-side we then construct a real CMapInfo by calling either mapInit()
 * or saveInit(). TypeScript clients can't construct a CMapInfo from JSON
 * (it's a heavyweight C++ engine type); the server must load it from a
 * known filename.
 *
 * Outbound shape mirrors what we accept on inbound (just fileURI for now;
 * kind is omitted since the wrapper can infer from si->mode).
 *
 * mapGenOpts (random-map generator options) remains opaque-only.
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForLobby.h"
#include "../../../lib/mapping/CMapInfo.h"
#include "../../../lib/filesystem/ResourcePath.h"

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

		// mapGenOpts: still opaque-only.
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
			out["mapGenOpts"].Struct(); // still opaque
	}
};

REGISTER_PACK_CODEC(LobbySetMapCodec)

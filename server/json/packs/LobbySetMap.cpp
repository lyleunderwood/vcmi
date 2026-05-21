/*
 * LobbySetMap.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbySetMap. Canonical example for new pack codec
 * implementations.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbySetMap.ts
 *
 * On inbound we accept either an opaque mapInfo placeholder (legacy /
 * passthrough behavior) OR a `{ "fileURI": "Maps/X.h3m" }` hint that
 * causes us to construct a real CMapInfo server-side via mapInit().
 * The hint path is required because TypeScript clients can't construct
 * a CMapInfo from JSON (it's a heavyweight C++ engine type); the
 * server has to load it from a known map filename.
 *
 * mapGenOpts (random-map generator options) remains opaque-only for now.
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForLobby.h"
#include "../../../lib/mapping/CMapInfo.h"

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

		// mapInfo: if a fileURI string is supplied, load a real CMapInfo
		// server-side via mapInit(). Otherwise leave the pointer nullptr
		// (legacy opaque-placeholder behavior).
		const JsonNode & miNode = json["mapInfo"];
		if (miNode.isStruct() && miNode["fileURI"].isString())
		{
			auto mi = std::make_shared<CMapInfo>();
			try
			{
				mi->mapInit(miNode["fileURI"].String());
				pack->mapInfo = mi;
			}
			catch (const std::exception & e)
			{
				logNetwork->error("[LobbySetMap codec] failed to load map '%s': %s",
					miNode["fileURI"].String(), e.what());
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
			// Emit a structured placeholder including fileURI so the
			// wrapper can correlate state changes with the map it asked for.
			JsonNode & mi = out["mapInfo"];
			mi.Struct();
			mi["fileURI"].String() = p.mapInfo->fileURI;
		}

		if (p.mapGenOpts)
			out["mapGenOpts"].Struct(); // still opaque
	}
};

REGISTER_PACK_CODEC(LobbySetMapCodec)

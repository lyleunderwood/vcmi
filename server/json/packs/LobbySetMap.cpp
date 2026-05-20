/*
 * LobbySetMap.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbySetMap.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbySetMap.ts
 *
 * NOTE: `mapInfo` (std::shared_ptr<CMapInfo>) and `mapGenOpts`
 * (std::shared_ptr<CMapGenOptions>) are complex VCMI engine types whose
 * full field models are not in scope for this codec. They are treated as
 * opaque JSON blobs on the wire: toJson emits a placeholder null when the
 * pointer is null (or an empty struct when not), and fromJson leaves the
 * pointers as nullptr. A future codec pass should promote CMapInfo and
 * CMapGenOptions to shared codecs.
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForLobby.h"

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
		// mapInfo and mapGenOpts are opaque on the wire; leave as nullptr.
		// (Future work: deserialize from json["mapInfo"] / json["mapGenOpts"].)
		(void)json;
		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbySetMap &>(pack);

		out["type"].String() = typeName();

		if (p.mapInfo)
			out["mapInfo"].Struct(); // opaque placeholder
		// else: leave field absent / null

		if (p.mapGenOpts)
			out["mapGenOpts"].Struct(); // opaque placeholder
		// else: leave field absent / null
	}
};

REGISTER_PACK_CODEC(LobbySetMapCodec)

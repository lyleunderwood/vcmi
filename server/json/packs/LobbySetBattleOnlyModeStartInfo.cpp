/*
 * LobbySetBattleOnlyModeStartInfo.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbySetBattleOnlyModeStartInfo.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbySetBattleOnlyModeStartInfo.ts
 *
 * NOTE: `startInfo` (std::shared_ptr<BattleOnlyModeStartInfo>) is a complex
 * VCMI engine type whose full field model is not in scope for this codec.
 * It is treated as an opaque JSON blob on the wire: toJson emits an empty
 * struct placeholder when the pointer is non-null (omitted when null), and
 * fromJson leaves the pointer as nullptr. A future codec pass should
 * promote BattleOnlyModeStartInfo to a shared codec.
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForLobby.h"

class LobbySetBattleOnlyModeStartInfoCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbySetBattleOnlyModeStartInfo"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbySetBattleOnlyModeStartInfo *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbySetBattleOnlyModeStartInfo>();
		// startInfo is opaque on the wire; leave as nullptr.
		// (Future work: deserialize from json["startInfo"].)
		(void)json;
		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbySetBattleOnlyModeStartInfo &>(pack);

		out["type"].String() = typeName();

		if (p.startInfo)
			out["startInfo"].Struct(); // opaque placeholder
		// else: leave field absent / null
	}
};

REGISTER_PACK_CODEC(LobbySetBattleOnlyModeStartInfoCodec)

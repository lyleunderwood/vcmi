/*
 * BattleEnded.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BattleEnded (server -> client battle state update:
 * signals the end of a battle, identifying the victor and loser).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClientBattle.h
 * TypeScript twin:      wrapper/src/codecs/battle/BattleEnded.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClientBattle.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class BattleEndedCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BattleEnded"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BattleEnded *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BattleEnded>();

		if (json["battleID"].isNumber())
			pack->battleID = BattleID(static_cast<int32_t>(json["battleID"].Integer()));

		if (json["victor"].isNumber())
			pack->victor = homamweb::shared::playerColorFromJson(json["victor"]);

		if (json["loser"].isNumber())
			pack->loser = homamweb::shared::playerColorFromJson(json["loser"]);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BattleEnded &>(pack);

		out["type"].String() = typeName();
		out["battleID"].Integer() = static_cast<int64_t>(p.battleID.getNum());
		out["victor"] = homamweb::shared::playerColorToJson(p.victor);
		out["loser"] = homamweb::shared::playerColorToJson(p.loser);
	}
};

REGISTER_PACK_CODEC(BattleEndedCodec)

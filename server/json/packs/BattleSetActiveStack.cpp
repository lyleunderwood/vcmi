/*
 * BattleSetActiveStack.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BattleSetActiveStack (server -> client battle state
 * update: announces which stack is now active, and why).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClientBattle.h
 * TypeScript twin:      wrapper/src/codecs/battle/BattleSetActiveStack.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClientBattle.h"
#include "../../../lib/constants/EntityIdentifiers.h"
#include "../../../lib/battle/BattleUnitTurnReason.h"

namespace
{
std::string reasonToString(BattleUnitTurnReason r)
{
	switch (r)
	{
		case BattleUnitTurnReason::TURN_QUEUE:        return "TURN_QUEUE";
		case BattleUnitTurnReason::MORALE:            return "MORALE";
		case BattleUnitTurnReason::HERO_SPELLCAST:    return "HERO_SPELLCAST";
		case BattleUnitTurnReason::UNIT_SPELLCAST:    return "UNIT_SPELLCAST";
		case BattleUnitTurnReason::AUTOMATIC_ACTION:  return "AUTOMATIC_ACTION";
		default:                                      return "TURN_QUEUE";
	}
}

BattleUnitTurnReason reasonFromString(const std::string & s)
{
	if (s == "MORALE")           return BattleUnitTurnReason::MORALE;
	if (s == "HERO_SPELLCAST")   return BattleUnitTurnReason::HERO_SPELLCAST;
	if (s == "UNIT_SPELLCAST")   return BattleUnitTurnReason::UNIT_SPELLCAST;
	if (s == "AUTOMATIC_ACTION") return BattleUnitTurnReason::AUTOMATIC_ACTION;
	return BattleUnitTurnReason::TURN_QUEUE;
}
} // namespace

class BattleSetActiveStackCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BattleSetActiveStack"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BattleSetActiveStack *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BattleSetActiveStack>();

		if (json["battleID"].isNumber())
			pack->battleID = BattleID(static_cast<int32_t>(json["battleID"].Integer()));

		if (json["stack"].isNumber())
			pack->stack = static_cast<uint32_t>(json["stack"].Integer());

		if (json["reason"].isString())
			pack->reason = reasonFromString(json["reason"].String());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BattleSetActiveStack &>(pack);

		out["type"].String() = typeName();
		out["battleID"].Integer() = static_cast<int64_t>(p.battleID.getNum());
		out["stack"].Integer() = static_cast<int64_t>(p.stack);
		out["reason"].String() = reasonToString(p.reason);
	}
};

REGISTER_PACK_CODEC(BattleSetActiveStackCodec)

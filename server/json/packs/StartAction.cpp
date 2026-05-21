/*
 * StartAction.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for StartAction (server -> client battle state update:
 * announces the BattleAction that the active stack/hero is starting).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClientBattle.h
 * BattleAction inlined: vcmi/lib/battle/BattleAction.h
 * TypeScript twin:      wrapper/src/codecs/battle/StartAction.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClientBattle.h"
#include "../../../lib/battle/BattleAction.h"
#include "../../../lib/battle/BattleHex.h"
#include "../../../lib/battle/BattleSide.h"
#include "../../../lib/constants/EntityIdentifiers.h"
#include "../../../lib/constants/Enumerations.h"

namespace
{
std::string actionTypeToString(EActionType t)
{
	switch (t)
	{
		case EActionType::NO_ACTION:        return "NO_ACTION";
		case EActionType::END_TACTIC_PHASE: return "END_TACTIC_PHASE";
		case EActionType::RETREAT:          return "RETREAT";
		case EActionType::SURRENDER:        return "SURRENDER";
		case EActionType::HERO_SPELL:       return "HERO_SPELL";
		case EActionType::WALK:             return "WALK";
		case EActionType::WAIT:             return "WAIT";
		case EActionType::DEFEND:           return "DEFEND";
		case EActionType::WALK_AND_ATTACK:  return "WALK_AND_ATTACK";
		case EActionType::SHOOT:            return "SHOOT";
		case EActionType::CATAPULT:         return "CATAPULT";
		case EActionType::MONSTER_SPELL:    return "MONSTER_SPELL";
		case EActionType::BAD_MORALE:       return "BAD_MORALE";
		case EActionType::STACK_HEAL:       return "STACK_HEAL";
		case EActionType::WALK_AND_CAST:    return "WALK_AND_CAST";
		default:                            return "NO_ACTION";
	}
}

EActionType actionTypeFromString(const std::string & s)
{
	if (s == "END_TACTIC_PHASE") return EActionType::END_TACTIC_PHASE;
	if (s == "RETREAT")          return EActionType::RETREAT;
	if (s == "SURRENDER")        return EActionType::SURRENDER;
	if (s == "HERO_SPELL")       return EActionType::HERO_SPELL;
	if (s == "WALK")             return EActionType::WALK;
	if (s == "WAIT")             return EActionType::WAIT;
	if (s == "DEFEND")           return EActionType::DEFEND;
	if (s == "WALK_AND_ATTACK")  return EActionType::WALK_AND_ATTACK;
	if (s == "SHOOT")            return EActionType::SHOOT;
	if (s == "CATAPULT")         return EActionType::CATAPULT;
	if (s == "MONSTER_SPELL")    return EActionType::MONSTER_SPELL;
	if (s == "BAD_MORALE")       return EActionType::BAD_MORALE;
	if (s == "STACK_HEAL")       return EActionType::STACK_HEAL;
	if (s == "WALK_AND_CAST")    return EActionType::WALK_AND_CAST;
	return EActionType::NO_ACTION;
}

std::string sideToString(BattleSide s)
{
	switch (s)
	{
		case BattleSide::NONE:        return "NONE";
		case BattleSide::INVALID:     return "INVALID";
		case BattleSide::ALL_KNOWING: return "ALL_KNOWING";
		case BattleSide::ATTACKER:    return "ATTACKER";
		case BattleSide::DEFENDER:    return "DEFENDER";
		default:                      return "NONE";
	}
}

BattleSide sideFromString(const std::string & s)
{
	if (s == "INVALID")     return BattleSide::INVALID;
	if (s == "ALL_KNOWING") return BattleSide::ALL_KNOWING;
	if (s == "ATTACKER")    return BattleSide::ATTACKER;
	if (s == "DEFENDER")    return BattleSide::DEFENDER;
	return BattleSide::NONE;
}

void encodeBattleAction(const BattleAction & ba, JsonNode & out)
{
	out["side"].String() = sideToString(ba.side);
	out["stackNumber"].Integer() = static_cast<int64_t>(ba.stackNumber);
	out["actionType"].String() = actionTypeToString(ba.actionType);
	out["spell"].Integer() = static_cast<int64_t>(ba.spell.getNum());

	JsonNode & target = out["target"];
	target.Vector(); // ensure array shape
	for (const auto & d : ba.target)
	{
		JsonNode entry;
		entry["unitValue"].Integer() = static_cast<int64_t>(d.unitValue);
		entry["hexValue"].Integer() = static_cast<int64_t>(d.hexValue.toInt());
		target.Vector().push_back(entry);
	}
}

BattleAction decodeBattleAction(const JsonNode & json)
{
	BattleAction ba;

	if (json["side"].isString())
		ba.side = sideFromString(json["side"].String());

	if (json["stackNumber"].isNumber())
		ba.stackNumber = static_cast<uint32_t>(json["stackNumber"].Integer());

	if (json["actionType"].isString())
		ba.actionType = actionTypeFromString(json["actionType"].String());

	if (json["spell"].isNumber())
		ba.spell = SpellID(static_cast<int32_t>(json["spell"].Integer()));

	if (json["target"].isVector())
	{
		for (const auto & e : json["target"].Vector())
		{
			BattleAction::DestinationInfo d;
			d.unitValue = static_cast<int32_t>(e["unitValue"].Integer());
			d.hexValue = BattleHex(static_cast<int16_t>(e["hexValue"].Integer()));
			ba.target.push_back(d);
		}
	}

	return ba;
}
} // namespace

class StartActionCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "StartAction"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const StartAction *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<StartAction>();

		if (json["battleID"].isNumber())
			pack->battleID = BattleID(static_cast<int32_t>(json["battleID"].Integer()));

		if (json["ba"].isStruct())
			pack->ba = decodeBattleAction(json["ba"]);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const StartAction &>(pack);

		out["type"].String() = typeName();
		out["battleID"].Integer() = static_cast<int64_t>(p.battleID.getNum());
		encodeBattleAction(p.ba, out["ba"]);
	}
};

REGISTER_PACK_CODEC(StartActionCodec)

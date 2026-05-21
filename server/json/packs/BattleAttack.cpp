/*
 * BattleAttack.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BattleAttack (server -> client battle state update:
 * one stack attacks one or more target stacks; carries a list of
 * BattleStackAttacked entries plus a BattleUnitsChanged-style patch of
 * attacker-side changes).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClientBattle.h
 * TypeScript twin:      wrapper/src/codecs/battle/BattleAttack.ts
 *
 * Nested sub-types are inlined here (not promoted to shared codecs):
 *   - BattleStackAttacked   (from PacksForClientBattle.h)
 *   - UnitChanges           (from BattleChanges.h) — used inside both
 *                           BattleStackAttacked::newState and the
 *                           attacker BattleUnitsChanged::changedStacks
 *   - BattleUnitsChanged    (attackerChanges field)
 *
 * The `flags` bitfields on both BattleAttack and BattleStackAttacked are
 * emitted as plain integers (EFlags bitwise-ORed).
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClientBattle.h"
#include "../../../lib/networkPacks/BattleChanges.h"
#include "../../../lib/constants/EntityIdentifiers.h"
#include "../../../lib/battle/BattleHex.h"

namespace
{

// ---- BattleChanges::EOperation -----------------------------------------
std::string operationToString(BattleChanges::EOperation op)
{
	switch (op)
	{
		case BattleChanges::EOperation::ADD:    return "ADD";
		case BattleChanges::EOperation::UPDATE: return "UPDATE";
		case BattleChanges::EOperation::REMOVE: return "REMOVE";
	}
	return "UPDATE";
}

BattleChanges::EOperation operationFromString(const std::string & s)
{
	if (s == "ADD")    return BattleChanges::EOperation::ADD;
	if (s == "UPDATE") return BattleChanges::EOperation::UPDATE;
	if (s == "REMOVE") return BattleChanges::EOperation::REMOVE;
	throw std::runtime_error("BattleAttack: unknown EOperation '" + s + "'");
}

// ---- UnitChanges entry --------------------------------------------------
JsonNode unitChangesToJson(const UnitChanges & c)
{
	JsonNode out;
	out.Struct();
	out["id"].Integer() = static_cast<int64_t>(c.id);
	out["healthDelta"].Integer() = c.healthDelta;
	// data: opaque passthrough — copy the JsonNode subtree verbatim.
	out["data"] = c.data;
	out["operation"].String() = operationToString(c.operation);
	return out;
}

UnitChanges unitChangesFromJson(const JsonNode & json)
{
	UnitChanges c;
	if (json["id"].isNumber())
		c.id = static_cast<uint32_t>(json["id"].Integer());
	if (json["healthDelta"].isNumber())
		c.healthDelta = json["healthDelta"].Integer();
	// data: opaque passthrough — copy the JsonNode subtree verbatim.
	c.data = json["data"];
	if (json["operation"].isString())
		c.operation = operationFromString(json["operation"].String());
	return c;
}

// ---- BattleStackAttacked entry -----------------------------------------
JsonNode battleStackAttackedToJson(const BattleStackAttacked & s)
{
	JsonNode out;
	out.Struct();
	out["stackAttacked"].Integer() = static_cast<int64_t>(s.stackAttacked);
	out["attackerID"].Integer() = static_cast<int64_t>(s.attackerID);
	out["newState"] = unitChangesToJson(s.newState);
	out["flags"].Integer() = static_cast<int64_t>(s.flags);
	out["killedAmount"].Integer() = static_cast<int64_t>(s.killedAmount);
	out["damageAmount"].Integer() = s.damageAmount;
	out["spellID"].Integer() = static_cast<int64_t>(s.spellID.getNum());
	return out;
}

BattleStackAttacked battleStackAttackedFromJson(const JsonNode & json)
{
	BattleStackAttacked s;
	if (json["stackAttacked"].isNumber())
		s.stackAttacked = static_cast<uint32_t>(json["stackAttacked"].Integer());
	if (json["attackerID"].isNumber())
		s.attackerID = static_cast<uint32_t>(json["attackerID"].Integer());
	s.newState = unitChangesFromJson(json["newState"]);
	if (json["flags"].isNumber())
		s.flags = static_cast<uint32_t>(json["flags"].Integer());
	if (json["killedAmount"].isNumber())
		s.killedAmount = static_cast<uint32_t>(json["killedAmount"].Integer());
	if (json["damageAmount"].isNumber())
		s.damageAmount = json["damageAmount"].Integer();
	if (json["spellID"].isNumber())
		s.spellID = SpellID(static_cast<int32_t>(json["spellID"].Integer()));
	return s;
}

// ---- BattleUnitsChanged inlined as attackerChanges ----------------------
JsonNode attackerChangesToJson(const BattleUnitsChanged & a)
{
	JsonNode out;
	out.Struct();
	out["battleID"].Integer() = static_cast<int64_t>(a.battleID.getNum());
	JsonNode & changed = out["changedStacks"];
	changed.Vector();
	for (const auto & c : a.changedStacks)
		changed.Vector().push_back(unitChangesToJson(c));
	return out;
}

void attackerChangesFromJson(const JsonNode & json, BattleUnitsChanged & a)
{
	if (json["battleID"].isNumber())
		a.battleID = BattleID(static_cast<int32_t>(json["battleID"].Integer()));
	if (json["changedStacks"].isVector())
	{
		for (const auto & entry : json["changedStacks"].Vector())
			a.changedStacks.push_back(unitChangesFromJson(entry));
	}
}

} // namespace

class BattleAttackCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BattleAttack"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BattleAttack *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BattleAttack>();

		if (json["battleID"].isNumber())
			pack->battleID = BattleID(static_cast<int32_t>(json["battleID"].Integer()));

		if (json["bsa"].isVector())
		{
			for (const auto & entry : json["bsa"].Vector())
				pack->bsa.push_back(battleStackAttackedFromJson(entry));
		}

		if (json["stackAttacking"].isNumber())
			pack->stackAttacking = static_cast<uint32_t>(json["stackAttacking"].Integer());

		if (json["flags"].isNumber())
			pack->flags = static_cast<uint32_t>(json["flags"].Integer());

		if (json["tile"].isNumber())
			pack->tile = BattleHex(static_cast<int16_t>(json["tile"].Integer()));

		if (json["spellID"].isNumber())
			pack->spellID = SpellID(static_cast<int32_t>(json["spellID"].Integer()));

		attackerChangesFromJson(json["attackerChanges"], pack->attackerChanges);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BattleAttack &>(pack);

		out["type"].String() = typeName();
		out["battleID"].Integer() = static_cast<int64_t>(p.battleID.getNum());

		JsonNode & bsa = out["bsa"];
		bsa.Vector();
		for (const auto & s : p.bsa)
			bsa.Vector().push_back(battleStackAttackedToJson(s));

		out["stackAttacking"].Integer() = static_cast<int64_t>(p.stackAttacking);
		out["flags"].Integer() = static_cast<int64_t>(p.flags);
		out["tile"].Integer() = static_cast<int64_t>(p.tile.toInt());
		out["spellID"].Integer() = static_cast<int64_t>(p.spellID.getNum());
		out["attackerChanges"] = attackerChangesToJson(p.attackerChanges);
	}
};

REGISTER_PACK_CODEC(BattleAttackCodec)

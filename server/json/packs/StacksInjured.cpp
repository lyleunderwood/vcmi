/*
 * StacksInjured.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for StacksInjured (server -> client battle state update:
 * a batch of `BattleStackAttacked` entries describing damage / kills /
 * resurrections / spell effects applied to stacks within a single battle).
 *
 * C++ pack definition:   vcmi/lib/networkPacks/PacksForClientBattle.h
 * C++ entry definition:  vcmi/lib/networkPacks/PacksForClientBattle.h
 *                        (BattleStackAttacked — inlined here, not shared)
 * C++ nested struct:     UnitChanges from lib/networkPacks/BattleChanges.h
 *                        (inlined here for the per-entry `newState`)
 * TypeScript twin:       wrapper/src/codecs/battle/StacksInjured.ts
 *
 * `BattleStackAttacked::newState.data` is a free-form `JsonNode` (opaque
 * engine-side patch). It is passed through verbatim — no structured
 * representation.
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClientBattle.h"
#include "../../../lib/networkPacks/BattleChanges.h"
#include "../../../lib/constants/EntityIdentifiers.h"

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
	throw std::runtime_error("StacksInjured: unknown EOperation '" + s + "'");
}

// ---- UnitChanges (nested, inlined; mirrors BattleUnitsChanged.cpp) -----
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

// ---- BattleStackAttacked (nested, inlined) -----------------------------
JsonNode battleStackAttackedToJson(const BattleStackAttacked & b)
{
	JsonNode out;
	out.Struct();
	out["stackAttacked"].Integer() = static_cast<int64_t>(b.stackAttacked);
	out["attackerID"].Integer() = static_cast<int64_t>(b.attackerID);
	out["newState"] = unitChangesToJson(b.newState);
	out["flags"].Integer() = static_cast<int64_t>(b.flags);
	out["killedAmount"].Integer() = static_cast<int64_t>(b.killedAmount);
	out["damageAmount"].Integer() = b.damageAmount;
	out["spellID"].Integer() = static_cast<int64_t>(b.spellID.getNum());
	return out;
}

BattleStackAttacked battleStackAttackedFromJson(const JsonNode & json)
{
	BattleStackAttacked b;
	if (json["stackAttacked"].isNumber())
		b.stackAttacked = static_cast<uint32_t>(json["stackAttacked"].Integer());
	if (json["attackerID"].isNumber())
		b.attackerID = static_cast<uint32_t>(json["attackerID"].Integer());
	b.newState = unitChangesFromJson(json["newState"]);
	if (json["flags"].isNumber())
		b.flags = static_cast<uint32_t>(json["flags"].Integer());
	if (json["killedAmount"].isNumber())
		b.killedAmount = static_cast<uint32_t>(json["killedAmount"].Integer());
	if (json["damageAmount"].isNumber())
		b.damageAmount = json["damageAmount"].Integer();
	if (json["spellID"].isNumber())
		b.spellID = SpellID(static_cast<int32_t>(json["spellID"].Integer()));
	return b;
}

} // namespace

class StacksInjuredCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "StacksInjured"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const StacksInjured *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<StacksInjured>();

		if (json["battleID"].isNumber())
			pack->battleID = BattleID(static_cast<int32_t>(json["battleID"].Integer()));

		if (json["stacks"].isVector())
		{
			for (const auto & entry : json["stacks"].Vector())
				pack->stacks.push_back(battleStackAttackedFromJson(entry));
		}

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const StacksInjured &>(pack);

		out["type"].String() = typeName();
		out["battleID"].Integer() = static_cast<int64_t>(p.battleID.getNum());

		JsonNode & stacks = out["stacks"];
		stacks.Vector();
		for (const auto & b : p.stacks)
			stacks.Vector().push_back(battleStackAttackedToJson(b));
	}
};

REGISTER_PACK_CODEC(StacksInjuredCodec)

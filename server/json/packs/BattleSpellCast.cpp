/*
 * BattleSpellCast.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BattleSpellCast (server -> client battle state update:
 * a spell has been cast in a battle, carrying caster info, target tile,
 * mana gained from channeling, affected/resisted/reflected stack ids,
 * and the casting actor).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClientBattle.h
 * TypeScript twin:      wrapper/src/codecs/battle/BattleSpellCast.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClientBattle.h"
#include "../../../lib/battle/BattleSide.h"
#include "../../../lib/battle/BattleHex.h"
#include "../../../lib/constants/EntityIdentifiers.h"

namespace
{

std::string battleSideToString(BattleSide s)
{
	switch (s)
	{
		case BattleSide::NONE:        return "NONE";
		case BattleSide::INVALID:     return "INVALID";
		case BattleSide::ALL_KNOWING: return "ALL_KNOWING";
		case BattleSide::ATTACKER:    return "ATTACKER";
		case BattleSide::DEFENDER:    return "DEFENDER";
	}
	return "NONE";
}

BattleSide battleSideFromString(const std::string & s)
{
	if (s == "NONE")        return BattleSide::NONE;
	if (s == "INVALID")     return BattleSide::INVALID;
	if (s == "ALL_KNOWING") return BattleSide::ALL_KNOWING;
	if (s == "ATTACKER")    return BattleSide::ATTACKER;
	if (s == "DEFENDER")    return BattleSide::DEFENDER;
	throw std::runtime_error("BattleSpellCast: unknown BattleSide '" + s + "'");
}

void decodeStackSet(const JsonNode & json, std::set<ui32> & out)
{
	if (!json.isVector())
		return;
	for (const auto & v : json.Vector())
		out.insert(static_cast<ui32>(v.Integer()));
}

JsonNode encodeStackSet(const std::set<ui32> & set)
{
	JsonNode arr;
	arr.Vector();
	for (auto id : set)
	{
		JsonNode entry;
		entry.Integer() = static_cast<int64_t>(id);
		arr.Vector().push_back(entry);
	}
	return arr;
}

} // namespace

class BattleSpellCastCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BattleSpellCast"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BattleSpellCast *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BattleSpellCast>();

		if (json["battleID"].isNumber())
			pack->battleID = BattleID(static_cast<int32_t>(json["battleID"].Integer()));

		if (json["activeCast"].isBool())
			pack->activeCast = json["activeCast"].Bool();

		if (json["side"].isString())
			pack->side = battleSideFromString(json["side"].String());

		if (json["spellID"].isNumber())
			pack->spellID = SpellID(static_cast<int32_t>(json["spellID"].Integer()));

		if (json["manaGained"].isNumber())
			pack->manaGained = static_cast<ui8>(json["manaGained"].Integer());

		if (json["tile"].isNumber())
			pack->tile = BattleHex(static_cast<si16>(json["tile"].Integer()));

		decodeStackSet(json["affectedCres"], pack->affectedCres);
		decodeStackSet(json["resistedCres"], pack->resistedCres);
		decodeStackSet(json["reflectedCres"], pack->reflectedCres);

		if (json["casterStack"].isNumber())
			pack->casterStack = static_cast<si32>(json["casterStack"].Integer());

		if (json["castByHero"].isBool())
			pack->castByHero = json["castByHero"].Bool();

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BattleSpellCast &>(pack);

		out["type"].String() = typeName();
		out["battleID"].Integer() = static_cast<int64_t>(p.battleID.getNum());
		out["activeCast"].Bool() = p.activeCast;
		out["side"].String() = battleSideToString(p.side);
		out["spellID"].Integer() = static_cast<int64_t>(p.spellID.getNum());
		out["manaGained"].Integer() = static_cast<int64_t>(p.manaGained);
		out["tile"].Integer() = static_cast<int64_t>(p.tile.toInt());
		out["affectedCres"] = encodeStackSet(p.affectedCres);
		out["resistedCres"] = encodeStackSet(p.resistedCres);
		out["reflectedCres"] = encodeStackSet(p.reflectedCres);
		out["casterStack"].Integer() = static_cast<int64_t>(p.casterStack);
		out["castByHero"].Bool() = p.castByHero;
	}
};

REGISTER_PACK_CODEC(BattleSpellCastCodec)

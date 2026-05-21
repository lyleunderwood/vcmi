/*
 * CatapultAttack.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for CatapultAttack (server -> client battle state update:
 * a catapult shot resolved against one or more wall parts, optionally
 * tied to an attacker stack -- attacker == -1 means a spell caused it).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClientBattle.h
 * TypeScript twin:      wrapper/src/codecs/battle/CatapultAttack.ts
 *
 * Nested AttackInfo sub-struct is inlined here (not promoted to shared).
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClientBattle.h"
#include "../../../lib/constants/EntityIdentifiers.h"
#include "../../../lib/constants/Enumerations.h"
#include "../../../lib/battle/BattleHex.h"

namespace
{

// ---- EWallPart ----------------------------------------------------------
std::string wallPartToString(EWallPart p)
{
	switch (p)
	{
		case EWallPart::INDESTRUCTIBLE_PART_OF_GATE: return "INDESTRUCTIBLE_PART_OF_GATE";
		case EWallPart::INDESTRUCTIBLE_PART:         return "INDESTRUCTIBLE_PART";
		case EWallPart::INVALID:                     return "INVALID";
		case EWallPart::KEEP:                        return "KEEP";
		case EWallPart::BOTTOM_TOWER:                return "BOTTOM_TOWER";
		case EWallPart::BOTTOM_WALL:                 return "BOTTOM_WALL";
		case EWallPart::BELOW_GATE:                  return "BELOW_GATE";
		case EWallPart::OVER_GATE:                   return "OVER_GATE";
		case EWallPart::UPPER_WALL:                  return "UPPER_WALL";
		case EWallPart::UPPER_TOWER:                 return "UPPER_TOWER";
		case EWallPart::GATE:                        return "GATE";
		case EWallPart::PARTS_COUNT:                 return "PARTS_COUNT";
	}
	return "INVALID";
}

EWallPart wallPartFromString(const std::string & s)
{
	if (s == "INDESTRUCTIBLE_PART_OF_GATE") return EWallPart::INDESTRUCTIBLE_PART_OF_GATE;
	if (s == "INDESTRUCTIBLE_PART")         return EWallPart::INDESTRUCTIBLE_PART;
	if (s == "INVALID")                     return EWallPart::INVALID;
	if (s == "KEEP")                        return EWallPart::KEEP;
	if (s == "BOTTOM_TOWER")                return EWallPart::BOTTOM_TOWER;
	if (s == "BOTTOM_WALL")                 return EWallPart::BOTTOM_WALL;
	if (s == "BELOW_GATE")                  return EWallPart::BELOW_GATE;
	if (s == "OVER_GATE")                   return EWallPart::OVER_GATE;
	if (s == "UPPER_WALL")                  return EWallPart::UPPER_WALL;
	if (s == "UPPER_TOWER")                 return EWallPart::UPPER_TOWER;
	if (s == "GATE")                        return EWallPart::GATE;
	if (s == "PARTS_COUNT")                 return EWallPart::PARTS_COUNT;
	throw std::runtime_error("CatapultAttack: unknown EWallPart '" + s + "'");
}

// ---- AttackInfo entry ---------------------------------------------------
JsonNode attackInfoToJson(const CatapultAttack::AttackInfo & a)
{
	JsonNode out;
	out.Struct();
	out["destinationTile"].Integer() = static_cast<int64_t>(BattleHex(a.destinationTile).toInt());
	out["attackedPart"].String() = wallPartToString(a.attackedPart);
	out["damageDealt"].Integer() = static_cast<int64_t>(a.damageDealt);
	return out;
}

CatapultAttack::AttackInfo attackInfoFromJson(const JsonNode & json)
{
	CatapultAttack::AttackInfo a;
	if (json["destinationTile"].isNumber())
		a.destinationTile = static_cast<si16>(json["destinationTile"].Integer());
	if (json["attackedPart"].isString())
		a.attackedPart = wallPartFromString(json["attackedPart"].String());
	if (json["damageDealt"].isNumber())
		a.damageDealt = static_cast<ui8>(json["damageDealt"].Integer());
	return a;
}

} // namespace

class CatapultAttackCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "CatapultAttack"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const CatapultAttack *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<CatapultAttack>();

		if (json["battleID"].isNumber())
			pack->battleID = BattleID(static_cast<int32_t>(json["battleID"].Integer()));

		if (json["attackedParts"].isVector())
		{
			for (const auto & entry : json["attackedParts"].Vector())
				pack->attackedParts.push_back(attackInfoFromJson(entry));
		}

		if (json["attacker"].isNumber())
			pack->attacker = static_cast<int>(json["attacker"].Integer());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const CatapultAttack &>(pack);

		out["type"].String() = typeName();
		out["battleID"].Integer() = static_cast<int64_t>(p.battleID.getNum());

		JsonNode & parts = out["attackedParts"];
		parts.Vector();
		for (const auto & a : p.attackedParts)
			parts.Vector().push_back(attackInfoToJson(a));

		out["attacker"].Integer() = static_cast<int64_t>(p.attacker);
	}
};

REGISTER_PACK_CODEC(CatapultAttackCodec)

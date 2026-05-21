/*
 * BattleResultAccepted.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BattleResultAccepted (server -> client: announces that the
 * end-of-battle result has been applied — propagates per-side hero/army XP
 * and the winning side).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClientBattle.h
 * TypeScript twin:      wrapper/src/codecs/battle/BattleResultAccepted.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClientBattle.h"
#include "../../../lib/battle/BattleSide.h"
#include "../../../lib/constants/EntityIdentifiers.h"

namespace
{

// ---- BattleSide enum ----------------------------------------------------
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
	throw std::runtime_error("BattleResultAccepted: unknown BattleSide '" + s + "'");
}

// ---- HeroBattleResults --------------------------------------------------
JsonNode heroBattleResultsToJson(const BattleResultAccepted::HeroBattleResults & h)
{
	JsonNode out;
	out.Struct();
	out["heroID"].Integer() = static_cast<int64_t>(h.heroID.getNum());
	out["armyID"].Integer() = static_cast<int64_t>(h.armyID.getNum());
	out["exp"].Integer() = static_cast<int64_t>(h.exp);
	return out;
}

BattleResultAccepted::HeroBattleResults heroBattleResultsFromJson(const JsonNode & json)
{
	BattleResultAccepted::HeroBattleResults h;
	if (json["heroID"].isNumber())
		h.heroID = ObjectInstanceID(static_cast<int32_t>(json["heroID"].Integer()));
	if (json["armyID"].isNumber())
		h.armyID = ObjectInstanceID(static_cast<int32_t>(json["armyID"].Integer()));
	if (json["exp"].isNumber())
		h.exp = static_cast<TExpType>(json["exp"].Integer());
	return h;
}

} // namespace

class BattleResultAcceptedCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BattleResultAccepted"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BattleResultAccepted *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BattleResultAccepted>();

		if (json["battleID"].isNumber())
			pack->battleID = BattleID(static_cast<int32_t>(json["battleID"].Integer()));

		const JsonNode & hr = json["heroResult"];
		if (hr.isStruct())
		{
			pack->heroResult[BattleSide::ATTACKER] = heroBattleResultsFromJson(hr["attacker"]);
			pack->heroResult[BattleSide::DEFENDER] = heroBattleResultsFromJson(hr["defender"]);
		}

		if (json["winnerSide"].isString())
			pack->winnerSide = battleSideFromString(json["winnerSide"].String());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BattleResultAccepted &>(pack);

		out["type"].String() = typeName();
		out["battleID"].Integer() = static_cast<int64_t>(p.battleID.getNum());

		JsonNode & hr = out["heroResult"];
		hr.Struct();
		hr["attacker"] = heroBattleResultsToJson(p.heroResult[BattleSide::ATTACKER]);
		hr["defender"] = heroBattleResultsToJson(p.heroResult[BattleSide::DEFENDER]);

		out["winnerSide"].String() = battleSideToString(p.winnerSide);
	}
};

REGISTER_PACK_CODEC(BattleResultAcceptedCodec)

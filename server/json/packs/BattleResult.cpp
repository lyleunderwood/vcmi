/*
 * BattleResult.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BattleResult (server -> client: query announcing the outcome
 * of a battle — winning side / type, per-side casualties, per-side experience).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClientBattle.h
 * TypeScript twin:      wrapper/src/codecs/battle/BattleResult.ts
 *
 * BattleResult derives from Query (carries queryID).
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClientBattle.h"
#include "../../../lib/battle/BattleSide.h"
#include "../../../lib/constants/EntityIdentifiers.h"
#include "../../../lib/constants/Enumerations.h"

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
	throw std::runtime_error("BattleResult: unknown BattleSide '" + s + "'");
}

// ---- EBattleResult enum -------------------------------------------------
std::string battleResultTypeToString(EBattleResult r)
{
	switch (r)
	{
		case EBattleResult::NORMAL:    return "NORMAL";
		case EBattleResult::ESCAPE:    return "ESCAPE";
		case EBattleResult::SURRENDER: return "SURRENDER";
	}
	return "NORMAL";
}

EBattleResult battleResultTypeFromString(const std::string & s)
{
	if (s == "NORMAL")    return EBattleResult::NORMAL;
	if (s == "ESCAPE")    return EBattleResult::ESCAPE;
	if (s == "SURRENDER") return EBattleResult::SURRENDER;
	throw std::runtime_error("BattleResult: unknown EBattleResult '" + s + "'");
}

// ---- casualties (std::map<CreatureID, si32>) ----------------------------
JsonNode casualtiesMapToJson(const std::map<CreatureID, si32> & m)
{
	JsonNode out;
	out.Vector();
	for (const auto & kv : m)
	{
		JsonNode entry;
		entry.Struct();
		entry["creatureID"].Integer() = static_cast<int64_t>(kv.first.getNum());
		entry["count"].Integer()      = static_cast<int64_t>(kv.second);
		out.Vector().push_back(entry);
	}
	return out;
}

std::map<CreatureID, si32> casualtiesMapFromJson(const JsonNode & json)
{
	std::map<CreatureID, si32> out;
	if (!json.isVector())
		return out;
	for (const auto & entry : json.Vector())
	{
		CreatureID cid;
		si32 count = 0;
		if (entry["creatureID"].isNumber())
			cid = CreatureID(static_cast<int32_t>(entry["creatureID"].Integer()));
		if (entry["count"].isNumber())
			count = static_cast<si32>(entry["count"].Integer());
		out[cid] = count;
	}
	return out;
}

} // namespace

class BattleResultCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BattleResult"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BattleResult *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BattleResult>();

		if (json["battleID"].isNumber())
			pack->battleID = BattleID(static_cast<int32_t>(json["battleID"].Integer()));

		// Inherited from Query.
		if (json["queryID"].isNumber())
			pack->queryID = QueryID(static_cast<int32_t>(json["queryID"].Integer()));

		if (json["result"].isString())
			pack->result = battleResultTypeFromString(json["result"].String());

		if (json["winner"].isString())
			pack->winner = battleSideFromString(json["winner"].String());

		const JsonNode & cas = json["casualties"];
		if (cas.isStruct())
		{
			pack->casualties[BattleSide::ATTACKER] = casualtiesMapFromJson(cas["attacker"]);
			pack->casualties[BattleSide::DEFENDER] = casualtiesMapFromJson(cas["defender"]);
		}

		const JsonNode & exp = json["exp"];
		if (exp.isStruct())
		{
			if (exp["attacker"].isNumber())
				pack->exp[BattleSide::ATTACKER] = static_cast<TExpType>(exp["attacker"].Integer());
			if (exp["defender"].isNumber())
				pack->exp[BattleSide::DEFENDER] = static_cast<TExpType>(exp["defender"].Integer());
		}

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BattleResult &>(pack);

		out["type"].String() = typeName();
		out["battleID"].Integer() = static_cast<int64_t>(p.battleID.getNum());

		// Inherited from Query.
		out["queryID"].Integer() = static_cast<int64_t>(p.queryID.getNum());

		out["result"].String() = battleResultTypeToString(p.result);
		out["winner"].String() = battleSideToString(p.winner);

		JsonNode & cas = out["casualties"];
		cas.Struct();
		cas["attacker"] = casualtiesMapToJson(p.casualties[BattleSide::ATTACKER]);
		cas["defender"] = casualtiesMapToJson(p.casualties[BattleSide::DEFENDER]);

		JsonNode & exp = out["exp"];
		exp.Struct();
		exp["attacker"].Integer() = static_cast<int64_t>(p.exp[BattleSide::ATTACKER]);
		exp["defender"].Integer() = static_cast<int64_t>(p.exp[BattleSide::DEFENDER]);
	}
};

REGISTER_PACK_CODEC(BattleResultCodec)

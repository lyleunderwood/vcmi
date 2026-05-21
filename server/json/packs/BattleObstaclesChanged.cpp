/*
 * BattleObstaclesChanged.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BattleObstaclesChanged (server -> client battle state update:
 * batch of obstacle changes within a battle — each entry tags an
 * existing/new/removed obstacle and carries an opaque JsonNode patch).
 *
 * C++ pack definition:   vcmi/lib/networkPacks/PacksForClientBattle.h
 * C++ entry definition:  vcmi/lib/networkPacks/BattleChanges.h (ObstacleChanges)
 * TypeScript twin:       wrapper/src/codecs/battle/BattleObstaclesChanged.ts
 *
 * The per-entry `data` member is a free-form `JsonNode` (engine-side state
 * patch). It is passed through verbatim — no structured representation.
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
	throw std::runtime_error("BattleObstaclesChanged: unknown EOperation '" + s + "'");
}

// ---- ObstacleChanges entry ----------------------------------------------
JsonNode obstacleChangesToJson(const ObstacleChanges & c)
{
	JsonNode out;
	out.Struct();
	out["id"].Integer() = static_cast<int64_t>(c.id);
	// data: opaque passthrough — copy the JsonNode subtree verbatim.
	out["data"] = c.data;
	out["operation"].String() = operationToString(c.operation);
	return out;
}

ObstacleChanges obstacleChangesFromJson(const JsonNode & json)
{
	ObstacleChanges c;
	if (json["id"].isNumber())
		c.id = static_cast<uint32_t>(json["id"].Integer());
	// data: opaque passthrough — copy the JsonNode subtree verbatim.
	c.data = json["data"];
	if (json["operation"].isString())
		c.operation = operationFromString(json["operation"].String());
	return c;
}

} // namespace

class BattleObstaclesChangedCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BattleObstaclesChanged"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BattleObstaclesChanged *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BattleObstaclesChanged>();

		if (json["battleID"].isNumber())
			pack->battleID = BattleID(static_cast<int32_t>(json["battleID"].Integer()));

		if (json["changes"].isVector())
		{
			for (const auto & entry : json["changes"].Vector())
				pack->changes.push_back(obstacleChangesFromJson(entry));
		}

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BattleObstaclesChanged &>(pack);

		out["type"].String() = typeName();
		out["battleID"].Integer() = static_cast<int64_t>(p.battleID.getNum());

		JsonNode & changes = out["changes"];
		changes.Vector();
		for (const auto & c : p.changes)
			changes.Vector().push_back(obstacleChangesToJson(c));
	}
};

REGISTER_PACK_CODEC(BattleObstaclesChangedCodec)

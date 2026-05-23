/*
 * BattleStart.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BattleStart (server -> client). Carries the BattleInfo
 * snapshot describing the starting state of an engagement.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClientBattle.h
 * TypeScript twin:      wrapper/src/codecs/battle/BattleStart.ts
 *
 * Minimal first-pass field set per docs/protocol-map.md § BattleInfo:
 *   battleID, round, activeStack, townID, tile, replayAllowed,
 *   sides[], stacks[], obstacles[] (USUAL+ABSOLUTE_OBSTACLE only),
 *   siegeInfo, battlefieldType, terrainType, tacticsSide, tacticDistance.
 *
 * Deferred (follow-up codec passes):
 *   - Bonus system contents (CBonusSystemNode base)
 *   - SpellCreatedObstacle's 11 extra fields
 *   - BattleField/TerrainId as game-id strings (currently raw ints)
 *
 * fromJson is best-effort; the engine never receives a constructed
 * BattleStart from a client (it's an outbound-only pack in normal flow),
 * but the codec is symmetric for tests and reflection.
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClientBattle.h"
#include "../../../lib/battle/BattleInfo.h"
#include "../../../lib/battle/SideInBattle.h"
#include "../../../lib/battle/CObstacleInstance.h"
#include "../../../lib/battle/SiegeInfo.h"
#include "../../../lib/CStack.h"
#include "../../../lib/constants/EntityIdentifiers.h"

static const char * battleSideName(BattleSide s)
{
	switch (s)
	{
	case BattleSide::NONE: return "NONE";
	case BattleSide::INVALID: return "INVALID";
	case BattleSide::ALL_KNOWING: return "ALL_KNOWING";
	case BattleSide::ATTACKER: return "ATTACKER";
	case BattleSide::DEFENDER: return "DEFENDER";
	default: return "NONE";
	}
}

static const char * obstacleKindName(CObstacleInstance::EObstacleType t)
{
	switch (t)
	{
	case CObstacleInstance::USUAL: return "USUAL";
	case CObstacleInstance::ABSOLUTE_OBSTACLE: return "ABSOLUTE_OBSTACLE";
	case CObstacleInstance::SPELL_CREATED: return "SPELL_CREATED";
	case CObstacleInstance::MOAT: return "MOAT";
	default: return "USUAL";
	}
}

static const char * wallPartName(EWallPart p)
{
	switch (p)
	{
	case EWallPart::INVALID: return "INVALID";
	case EWallPart::INDESTRUCTIBLE_PART_OF_GATE: return "INDESTRUCTIBLE_PART_OF_GATE";
	case EWallPart::INDESTRUCTIBLE_PART: return "INDESTRUCTIBLE_PART";
	case EWallPart::KEEP: return "KEEP";
	case EWallPart::BOTTOM_TOWER: return "BOTTOM_TOWER";
	case EWallPart::BOTTOM_WALL: return "BOTTOM_WALL";
	case EWallPart::BELOW_GATE: return "BELOW_GATE";
	case EWallPart::OVER_GATE: return "OVER_GATE";
	case EWallPart::UPPER_WALL: return "UPPER_WALL";
	case EWallPart::UPPER_TOWER: return "UPPER_TOWER";
	case EWallPart::GATE: return "GATE";
	case EWallPart::PARTS_COUNT: return "PARTS_COUNT";
	default: return "INVALID";
	}
}

static const char * wallStateName(EWallState s)
{
	switch (s)
	{
	case EWallState::NONE: return "NONE";
	case EWallState::DESTROYED: return "DESTROYED";
	case EWallState::DAMAGED: return "DAMAGED";
	case EWallState::INTACT: return "INTACT";
	case EWallState::REINFORCED: return "REINFORCED";
	default: return "NONE";
	}
}

static const char * gateStateName(EGateState g)
{
	switch (g)
	{
	case EGateState::NONE: return "NONE";
	case EGateState::CLOSED: return "CLOSED";
	case EGateState::BLOCKED: return "BLOCKED";
	case EGateState::OPENED: return "OPENED";
	case EGateState::DESTROYED: return "DESTROYED";
	default: return "NONE";
	}
}

static void emitSide(const SideInBattle & s, JsonNode & out)
{
	out["color"] = homamweb::shared::playerColorToJson(s.color);
	out["heroID"].Integer() = s.heroID.getNum();
	out["armyObjectID"].Integer() = s.armyObjectID.getNum();
	out["castSpellsCount"].Integer() = static_cast<int64_t>(s.castSpellsCount);
	JsonNode & spellsArr = out["usedSpellsHistory"];
	spellsArr.Vector();
	for (const SpellID & sp : s.usedSpellsHistory)
	{
		JsonNode entry;
		entry.Integer() = sp.getNum();
		spellsArr.Vector().push_back(entry);
	}
	out["enchanterCounter"].Integer() = s.enchanterCounter;
	out["initialMana"].Integer() = s.initialMana;
	out["additionalMana"].Integer() = s.additionalMana;
}

static void emitStack(const CStack & st, JsonNode & out)
{
	out["id"].Integer() = static_cast<int64_t>(st.unitId());
	out["typeID"].Integer() = (st.unitType() ? st.unitType()->getId().getNum() : -1);
	out["baseAmount"].Integer() = static_cast<int64_t>(st.unitBaseAmount());
	out["owner"] = homamweb::shared::playerColorToJson(st.unitOwner());
	out["slot"].Integer() = st.unitSlot().getNum();
	out["side"].String() = battleSideName(st.unitSide());
	out["initialPosition"].Integer() = st.initialPosition.toInt();
}

static void emitObstacle(const CObstacleInstance & o, JsonNode & out)
{
	out["uniqueID"].Integer() = o.uniqueID;
	out["typeId"].Integer() = o.ID;
	out["pos"].Integer() = o.pos.toInt();
	out["kind"].String() = obstacleKindName(o.obstacleType);
}

static void emitSiegeInfo(const SiegeInfo & s, JsonNode & out)
{
	JsonNode & wallArr = out["wallState"];
	wallArr.Vector();
	for (const auto & kv : s.wallState)
	{
		JsonNode entry;
		entry["part"].String() = wallPartName(kv.first);
		entry["state"].String() = wallStateName(kv.second);
		wallArr.Vector().push_back(entry);
	}
	out["gateState"].String() = gateStateName(s.gateState);
}

// homam-web fork: non-static so the JsonAdapter can synthesize a BattleStart
// payload to resume a loaded battle on the wrapper (WrapperResumeBattle).
void emitBattleInfo(const BattleInfo & bi, JsonNode & out)
{
	out["battleID"].Integer() = bi.battleID.getNum();
	out["round"].Integer() = bi.round;
	out["activeStack"].Integer() = bi.activeStack;
	out["townID"].Integer() = bi.townID.getNum();
	out["tile"]["x"].Integer() = bi.tile.x;
	out["tile"]["y"].Integer() = bi.tile.y;
	out["tile"]["z"].Integer() = bi.tile.z;
	out["replayAllowed"].Bool() = bi.replayAllowed;
	out["battlefieldType"].Integer() = bi.battlefieldType.getNum();
	out["terrainType"].Integer() = bi.terrainType.getNum();
	out["tacticsSide"].String() = battleSideName(bi.tacticsSide);
	out["tacticDistance"].Integer() = bi.tacticDistance;

	JsonNode & sidesArr = out["sides"];
	sidesArr.Vector();
	for (const SideInBattle & s : bi.sides)
	{
		JsonNode entry;
		emitSide(s, entry);
		sidesArr.Vector().push_back(entry);
	}

	JsonNode & stacksArr = out["stacks"];
	stacksArr.Vector();
	for (const auto & stPtr : bi.stacks)
	{
		if (!stPtr) continue;
		JsonNode entry;
		emitStack(*stPtr, entry);
		stacksArr.Vector().push_back(entry);
	}

	JsonNode & obstArr = out["obstacles"];
	obstArr.Vector();
	for (const auto & oPtr : bi.obstacles)
	{
		if (!oPtr) continue;
		JsonNode entry;
		emitObstacle(*oPtr, entry);
		obstArr.Vector().push_back(entry);
	}

	JsonNode siege;
	emitSiegeInfo(bi.si, siege);
	out["siegeInfo"] = siege;
}

class BattleStartCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BattleStart"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BattleStart *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BattleStart>();
		if (json["battleID"].isNumber())
			pack->battleID = BattleID(static_cast<int32_t>(json["battleID"].Integer()));
		// fromJson does NOT reconstruct BattleInfo — that's a deep object
		// graph the engine constructs internally during BattleProcessor::startBattle.
		// The wrapper never sends BattleStart back to the engine in practice.
		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BattleStart &>(pack);
		out["type"].String() = typeName();
		out["battleID"].Integer() = static_cast<int64_t>(p.battleID.getNum());
		if (p.info)
		{
			JsonNode info;
			emitBattleInfo(*p.info, info);
			out["info"] = info;
		}
		// else: leave field absent.
	}
};

REGISTER_PACK_CODEC(BattleStartCodec)

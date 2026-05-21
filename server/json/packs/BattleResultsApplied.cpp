/*
 * BattleResultsApplied.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BattleResultsApplied (server -> client battle state update:
 * after a battle ends, applies its consequences: spells learned, artifacts
 * moved between heroes, growing/discharging artifacts, and a raised stack).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClientBattle.h
 * TypeScript twin:      wrapper/src/codecs/battle/BattleResultsApplied.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"
#include "../shared/ArtifactLocation.h"

#include "../../../lib/networkPacks/PacksForClientBattle.h"
#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/networkPacks/ArtifactLocation.h"
#include "../../../lib/mapObjects/army/CStackBasicDescriptor.h"
#include "../../../lib/constants/EntityIdentifiers.h"

namespace
{

ChangeSpells changeSpellsFromJson(const JsonNode & json)
{
	ChangeSpells cs;
	if (json["learn"].isNumber())
		cs.learn = static_cast<ui8>(json["learn"].Integer());
	if (json["hid"].isNumber())
		cs.hid = ObjectInstanceID(static_cast<int32_t>(json["hid"].Integer()));
	if (json["spells"].isVector())
	{
		for (const auto & s : json["spells"].Vector())
			cs.spells.insert(SpellID(static_cast<int32_t>(s.Integer())));
	}
	return cs;
}

void changeSpellsToJson(const ChangeSpells & cs, JsonNode & out)
{
	out["learn"].Integer() = static_cast<int64_t>(cs.learn);
	out["hid"].Integer() = static_cast<int64_t>(cs.hid.getNum());
	JsonNode & spells = out["spells"];
	spells.Vector();
	for (const auto & s : cs.spells)
	{
		JsonNode entry;
		entry.Integer() = static_cast<int64_t>(s.getNum());
		spells.Vector().push_back(entry);
	}
}

MoveArtifactInfo moveArtifactInfoFromJson(const JsonNode & json)
{
	MoveArtifactInfo mi;
	if (json["srcPos"].isNumber())
		mi.srcPos = ArtifactPosition(static_cast<si32>(json["srcPos"].Integer()));
	if (json["dstPos"].isNumber())
		mi.dstPos = ArtifactPosition(static_cast<si32>(json["dstPos"].Integer()));
	if (json["askAssemble"].isBool())
		mi.askAssemble = json["askAssemble"].Bool();
	return mi;
}

void moveArtifactInfoToJson(const MoveArtifactInfo & mi, JsonNode & out)
{
	out["srcPos"].Integer() = static_cast<int64_t>(mi.srcPos.getNum());
	out["dstPos"].Integer() = static_cast<int64_t>(mi.dstPos.getNum());
	out["askAssemble"].Bool() = mi.askAssemble;
}

BulkMoveArtifacts bulkMoveArtifactsFromJson(const JsonNode & json)
{
	BulkMoveArtifacts bma;
	if (json["interfaceOwner"].isNumber())
		bma.interfaceOwner = homamweb::shared::playerColorFromJson(json["interfaceOwner"]);
	if (json["srcArtHolder"].isNumber())
		bma.srcArtHolder = ObjectInstanceID(static_cast<int32_t>(json["srcArtHolder"].Integer()));
	if (json["dstArtHolder"].isNumber())
		bma.dstArtHolder = ObjectInstanceID(static_cast<int32_t>(json["dstArtHolder"].Integer()));
	if (json["srcCreature"].isNumber())
		bma.srcCreature = SlotID(static_cast<int32_t>(json["srcCreature"].Integer()));
	if (json["dstCreature"].isNumber())
		bma.dstCreature = SlotID(static_cast<int32_t>(json["dstCreature"].Integer()));
	if (json["artsPack0"].isVector())
	{
		for (const auto & e : json["artsPack0"].Vector())
			bma.artsPack0.push_back(moveArtifactInfoFromJson(e));
	}
	if (json["artsPack1"].isVector())
	{
		for (const auto & e : json["artsPack1"].Vector())
			bma.artsPack1.push_back(moveArtifactInfoFromJson(e));
	}
	return bma;
}

void bulkMoveArtifactsToJson(const BulkMoveArtifacts & bma, JsonNode & out)
{
	out["interfaceOwner"] = homamweb::shared::playerColorToJson(bma.interfaceOwner);
	out["srcArtHolder"].Integer() = static_cast<int64_t>(bma.srcArtHolder.getNum());
	out["dstArtHolder"].Integer() = static_cast<int64_t>(bma.dstArtHolder.getNum());
	if (bma.srcCreature.has_value())
		out["srcCreature"].Integer() = static_cast<int64_t>(bma.srcCreature->getNum());
	if (bma.dstCreature.has_value())
		out["dstCreature"].Integer() = static_cast<int64_t>(bma.dstCreature->getNum());

	JsonNode & p0 = out["artsPack0"];
	p0.Vector();
	for (const auto & e : bma.artsPack0)
	{
		JsonNode entry;
		moveArtifactInfoToJson(e, entry);
		p0.Vector().push_back(entry);
	}

	JsonNode & p1 = out["artsPack1"];
	p1.Vector();
	for (const auto & e : bma.artsPack1)
	{
		JsonNode entry;
		moveArtifactInfoToJson(e, entry);
		p1.Vector().push_back(entry);
	}
}

GrowUpArtifact growUpArtifactFromJson(const JsonNode & json)
{
	GrowUpArtifact g;
	if (json["id"].isNumber())
		g.id = ArtifactInstanceID(static_cast<int32_t>(json["id"].Integer()));
	return g;
}

void growUpArtifactToJson(const GrowUpArtifact & g, JsonNode & out)
{
	out["id"].Integer() = static_cast<int64_t>(g.id.getNum());
}

DischargeArtifact dischargeArtifactFromJson(const JsonNode & json)
{
	DischargeArtifact d;
	if (json["id"].isNumber())
		d.id = ArtifactInstanceID(static_cast<int32_t>(json["id"].Integer()));
	if (json["charges"].isNumber())
		d.charges = static_cast<uint16_t>(json["charges"].Integer());
	if (json["artLoc"].isStruct())
		d.artLoc = homamweb::shared::artifactLocationFromJson(json["artLoc"]);
	return d;
}

void dischargeArtifactToJson(const DischargeArtifact & d, JsonNode & out)
{
	out["id"].Integer() = static_cast<int64_t>(d.id.getNum());
	out["charges"].Integer() = static_cast<int64_t>(d.charges);
	if (d.artLoc.has_value())
		out["artLoc"] = homamweb::shared::artifactLocationToJson(*d.artLoc);
}

CStackBasicDescriptor raisedStackFromJson(const JsonNode & json)
{
	CreatureID typeID = CreatureID::NONE;
	TQuantity count = -1;
	if (json["typeID"].isNumber())
		typeID = CreatureID(static_cast<int32_t>(json["typeID"].Integer()));
	if (json["count"].isNumber())
		count = static_cast<TQuantity>(json["count"].Integer());
	return CStackBasicDescriptor(typeID, count);
}

void raisedStackToJson(const CStackBasicDescriptor & rs, JsonNode & out)
{
	out["typeID"].Integer() = static_cast<int64_t>(rs.getId().getNum());
	out["count"].Integer() = static_cast<int64_t>(rs.getCount());
}

} // namespace

class BattleResultsAppliedCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BattleResultsApplied"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BattleResultsApplied *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BattleResultsApplied>();

		if (json["battleID"].isNumber())
			pack->battleID = BattleID(static_cast<int32_t>(json["battleID"].Integer()));

		if (json["victor"].isNumber())
			pack->victor = homamweb::shared::playerColorFromJson(json["victor"]);

		if (json["loser"].isNumber())
			pack->loser = homamweb::shared::playerColorFromJson(json["loser"]);

		if (json["learnedSpells"].isStruct())
			pack->learnedSpells = changeSpellsFromJson(json["learnedSpells"]);

		if (json["movingArtifacts"].isVector())
		{
			for (const auto & e : json["movingArtifacts"].Vector())
				pack->movingArtifacts.push_back(bulkMoveArtifactsFromJson(e));
		}

		if (json["growingArtifacts"].isVector())
		{
			for (const auto & e : json["growingArtifacts"].Vector())
				pack->growingArtifacts.push_back(growUpArtifactFromJson(e));
		}

		if (json["dischargingArtifacts"].isVector())
		{
			for (const auto & e : json["dischargingArtifacts"].Vector())
				pack->dischargingArtifacts.push_back(dischargeArtifactFromJson(e));
		}

		if (json["raisedStack"].isStruct())
			pack->raisedStack = raisedStackFromJson(json["raisedStack"]);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BattleResultsApplied &>(pack);

		out["type"].String() = typeName();
		out["battleID"].Integer() = static_cast<int64_t>(p.battleID.getNum());
		out["victor"] = homamweb::shared::playerColorToJson(p.victor);
		out["loser"] = homamweb::shared::playerColorToJson(p.loser);

		changeSpellsToJson(p.learnedSpells, out["learnedSpells"]);

		JsonNode & moving = out["movingArtifacts"];
		moving.Vector();
		for (const auto & e : p.movingArtifacts)
		{
			JsonNode entry;
			bulkMoveArtifactsToJson(e, entry);
			moving.Vector().push_back(entry);
		}

		JsonNode & growing = out["growingArtifacts"];
		growing.Vector();
		for (const auto & e : p.growingArtifacts)
		{
			JsonNode entry;
			growUpArtifactToJson(e, entry);
			growing.Vector().push_back(entry);
		}

		JsonNode & discharging = out["dischargingArtifacts"];
		discharging.Vector();
		for (const auto & e : p.dischargingArtifacts)
		{
			JsonNode entry;
			dischargeArtifactToJson(e, entry);
			discharging.Vector().push_back(entry);
		}

		raisedStackToJson(p.raisedStack, out["raisedStack"]);
	}
};

REGISTER_PACK_CODEC(BattleResultsAppliedCodec)

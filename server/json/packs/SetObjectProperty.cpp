/*
 * SetObjectProperty.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for SetObjectProperty (server -> client state update: set
 * an arbitrary property on a map object).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/SetObjectProperty.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/networkPacks/ObjProperty.h"
#include "../../../lib/constants/EntityIdentifiers.h"

namespace
{
std::string objPropertyToString(ObjProperty p)
{
	switch (p)
	{
		case ObjProperty::INVALID: return "INVALID";
		case ObjProperty::OWNER: return "OWNER";
		case ObjProperty::UNUSED: return "UNUSED";
		case ObjProperty::PRIMARY_STACK_COUNT: return "PRIMARY_STACK_COUNT";
		case ObjProperty::VISITORS: return "VISITORS";
		case ObjProperty::VISITED: return "VISITED";
		case ObjProperty::ID: return "ID";
		case ObjProperty::AVAILABLE_CREATURE: return "AVAILABLE_CREATURE";
		case ObjProperty::MONSTER_COUNT: return "MONSTER_COUNT";
		case ObjProperty::MONSTER_POWER: return "MONSTER_POWER";
		case ObjProperty::MONSTER_EXP: return "MONSTER_EXP";
		case ObjProperty::MONSTER_RESTORE_TYPE: return "MONSTER_RESTORE_TYPE";
		case ObjProperty::MONSTER_REFUSED_JOIN: return "MONSTER_REFUSED_JOIN";
		case ObjProperty::STRUCTURE_ADD_VISITING_HERO: return "STRUCTURE_ADD_VISITING_HERO";
		case ObjProperty::STRUCTURE_CLEAR_VISITORS: return "STRUCTURE_CLEAR_VISITORS";
		case ObjProperty::STRUCTURE_ADD_GARRISONED_HERO: return "STRUCTURE_ADD_GARRISONED_HERO";
		case ObjProperty::BONUS_VALUE_FIRST: return "BONUS_VALUE_FIRST";
		case ObjProperty::BONUS_VALUE_SECOND: return "BONUS_VALUE_SECOND";
		case ObjProperty::SEERHUT_VISITED: return "SEERHUT_VISITED";
		case ObjProperty::SEERHUT_COMPLETE: return "SEERHUT_COMPLETE";
		case ObjProperty::OBELISK_VISITED: return "OBELISK_VISITED";
		case ObjProperty::BANK_DAYCOUNTER: return "BANK_DAYCOUNTER";
		case ObjProperty::BANK_CLEAR: return "BANK_CLEAR";
		case ObjProperty::REWARD_SELECT: return "REWARD_SELECT";
		case ObjProperty::REWARD_CLEARED: return "REWARD_CLEARED";
		default: return "INVALID";
	}
}

ObjProperty objPropertyFromString(const std::string & s)
{
	if (s == "OWNER") return ObjProperty::OWNER;
	if (s == "UNUSED") return ObjProperty::UNUSED;
	if (s == "PRIMARY_STACK_COUNT") return ObjProperty::PRIMARY_STACK_COUNT;
	if (s == "VISITORS") return ObjProperty::VISITORS;
	if (s == "VISITED") return ObjProperty::VISITED;
	if (s == "ID") return ObjProperty::ID;
	if (s == "AVAILABLE_CREATURE") return ObjProperty::AVAILABLE_CREATURE;
	if (s == "MONSTER_COUNT") return ObjProperty::MONSTER_COUNT;
	if (s == "MONSTER_POWER") return ObjProperty::MONSTER_POWER;
	if (s == "MONSTER_EXP") return ObjProperty::MONSTER_EXP;
	if (s == "MONSTER_RESTORE_TYPE") return ObjProperty::MONSTER_RESTORE_TYPE;
	if (s == "MONSTER_REFUSED_JOIN") return ObjProperty::MONSTER_REFUSED_JOIN;
	if (s == "STRUCTURE_ADD_VISITING_HERO") return ObjProperty::STRUCTURE_ADD_VISITING_HERO;
	if (s == "STRUCTURE_CLEAR_VISITORS") return ObjProperty::STRUCTURE_CLEAR_VISITORS;
	if (s == "STRUCTURE_ADD_GARRISONED_HERO") return ObjProperty::STRUCTURE_ADD_GARRISONED_HERO;
	if (s == "BONUS_VALUE_FIRST") return ObjProperty::BONUS_VALUE_FIRST;
	if (s == "BONUS_VALUE_SECOND") return ObjProperty::BONUS_VALUE_SECOND;
	if (s == "SEERHUT_VISITED") return ObjProperty::SEERHUT_VISITED;
	if (s == "SEERHUT_COMPLETE") return ObjProperty::SEERHUT_COMPLETE;
	if (s == "OBELISK_VISITED") return ObjProperty::OBELISK_VISITED;
	if (s == "BANK_DAYCOUNTER") return ObjProperty::BANK_DAYCOUNTER;
	if (s == "BANK_CLEAR") return ObjProperty::BANK_CLEAR;
	if (s == "REWARD_SELECT") return ObjProperty::REWARD_SELECT;
	if (s == "REWARD_CLEARED") return ObjProperty::REWARD_CLEARED;
	return ObjProperty::INVALID;
}
} // namespace

class SetObjectPropertyCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "SetObjectProperty"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const SetObjectProperty *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<SetObjectProperty>();

		if (json["id"].isNumber())
			pack->id = ObjectInstanceID(static_cast<int32_t>(json["id"].Integer()));

		if (json["what"].isString())
			pack->what = objPropertyFromString(json["what"].String());

		if (json["identifier"].isNumber())
			pack->identifier = ObjPropertyID(NumericID(static_cast<int32_t>(json["identifier"].Integer())));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const SetObjectProperty &>(pack);

		out["type"].String() = typeName();
		out["id"].Integer() = static_cast<int64_t>(p.id.getNum());
		out["what"].String() = objPropertyToString(p.what);
		out["identifier"].Integer() = static_cast<int64_t>(p.identifier.getNum());
	}
};

REGISTER_PACK_CODEC(SetObjectPropertyCodec)

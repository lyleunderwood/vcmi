/*
 * SetCommanderProperty.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for SetCommanderProperty (server -> client state update: change
 * a commander's property — alive flag, bonus, secondary skill, experience,
 * or special-skill choice).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/SetCommanderProperty.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

namespace
{
std::string commanderPropertyToString(SetCommanderProperty::ECommanderProperty v)
{
	switch (v)
	{
		case SetCommanderProperty::ALIVE: return "ALIVE";
		case SetCommanderProperty::BONUS: return "BONUS";
		case SetCommanderProperty::SECONDARY_SKILL: return "SECONDARY_SKILL";
		case SetCommanderProperty::EXPERIENCE: return "EXPERIENCE";
		case SetCommanderProperty::SPECIAL_SKILL: return "SPECIAL_SKILL";
	}
	return "ALIVE";
}

SetCommanderProperty::ECommanderProperty commanderPropertyFromString(const std::string & s)
{
	if (s == "ALIVE") return SetCommanderProperty::ALIVE;
	if (s == "BONUS") return SetCommanderProperty::BONUS;
	if (s == "SECONDARY_SKILL") return SetCommanderProperty::SECONDARY_SKILL;
	if (s == "EXPERIENCE") return SetCommanderProperty::EXPERIENCE;
	if (s == "SPECIAL_SKILL") return SetCommanderProperty::SPECIAL_SKILL;
	return SetCommanderProperty::ALIVE;
}
}

class SetCommanderPropertyCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "SetCommanderProperty"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const SetCommanderProperty *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<SetCommanderProperty>();

		if (json["heroid"].isNumber())
			pack->heroid = ObjectInstanceID(static_cast<int32_t>(json["heroid"].Integer()));

		if (json["which"].isString())
			pack->which = commanderPropertyFromString(json["which"].String());

		if (json["amount"].isNumber())
			pack->amount = static_cast<TExpType>(json["amount"].Integer());

		if (json["additionalInfo"].isNumber())
			pack->additionalInfo = static_cast<si32>(json["additionalInfo"].Integer());

		// accumulatedBonus is an opaque Bonus struct; left default-constructed.
		// See "Tricky bits" in the codec report.

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const SetCommanderProperty &>(pack);

		out["type"].String() = typeName();
		out["heroid"].Integer() = static_cast<int64_t>(p.heroid.getNum());
		out["which"].String() = commanderPropertyToString(p.which);
		out["amount"].Integer() = static_cast<int64_t>(p.amount);
		out["additionalInfo"].Integer() = static_cast<int64_t>(p.additionalInfo);
		// accumulatedBonus: opaque, emit empty object placeholder.
		out["accumulatedBonus"].Struct();
	}
};

REGISTER_PACK_CODEC(SetCommanderPropertyCodec)

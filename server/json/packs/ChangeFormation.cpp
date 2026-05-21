/*
 * ChangeFormation.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for ChangeFormation (server -> client state update:
 * change a hero's army formation between LOOSE and TIGHT).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/ChangeFormation.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"
#include "../../../lib/constants/Enumerations.h"

namespace
{
std::string formationToString(EArmyFormation f)
{
	switch (f)
	{
		case EArmyFormation::LOOSE: return "LOOSE";
		case EArmyFormation::TIGHT: return "TIGHT";
		default: return "LOOSE";
	}
}

EArmyFormation formationFromString(const std::string & s)
{
	if (s == "TIGHT") return EArmyFormation::TIGHT;
	return EArmyFormation::LOOSE;
}
} // namespace

class ChangeFormationCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "ChangeFormation"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const ChangeFormation *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<ChangeFormation>();

		if (json["hid"].isNumber())
			pack->hid = ObjectInstanceID(static_cast<int32_t>(json["hid"].Integer()));

		if (json["formation"].isString())
			pack->formation = formationFromString(json["formation"].String());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const ChangeFormation &>(pack);

		out["type"].String() = typeName();
		out["hid"].Integer() = static_cast<int64_t>(p.hid.getNum());
		out["formation"].String() = formationToString(p.formation);
	}
};

REGISTER_PACK_CODEC(ChangeFormationCodec)

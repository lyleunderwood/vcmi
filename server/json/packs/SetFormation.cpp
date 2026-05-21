/*
 * SetFormation.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for SetFormation (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/SetFormation.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
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

class SetFormationCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "SetFormation"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const SetFormation *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<SetFormation>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["hid"].isNumber())
			pack->hid = ObjectInstanceID(static_cast<int32_t>(json["hid"].Integer()));

		if (json["formation"].isString())
			pack->formation = formationFromString(json["formation"].String());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const SetFormation &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["hid"].Integer() = static_cast<int64_t>(p.hid.getNum());
		out["formation"].String() = formationToString(p.formation);
	}
};

REGISTER_PACK_CODEC(SetFormationCodec)

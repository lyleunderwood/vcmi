/*
 * BattleSetStackProperty.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BattleSetStackProperty (server -> client battle state
 * update: a property of a stack (casts left, enchanter counter, etc.)
 * is being set or modified).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClientBattle.h
 * TypeScript twin:      wrapper/src/codecs/battle/BattleSetStackProperty.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClientBattle.h"
#include "../../../lib/constants/EntityIdentifiers.h"

namespace
{
	std::string propertyToString(BattleSetStackProperty::BattleStackProperty p)
	{
		switch (p)
		{
		case BattleSetStackProperty::CASTS: return "CASTS";
		case BattleSetStackProperty::ENCHANTER_COUNTER: return "ENCHANTER_COUNTER";
		case BattleSetStackProperty::UNBIND: return "UNBIND";
		case BattleSetStackProperty::CLONED: return "CLONED";
		case BattleSetStackProperty::HAS_CLONE: return "HAS_CLONE";
		}
		return "CASTS";
	}

	BattleSetStackProperty::BattleStackProperty propertyFromString(const std::string & s)
	{
		if (s == "CASTS") return BattleSetStackProperty::CASTS;
		if (s == "ENCHANTER_COUNTER") return BattleSetStackProperty::ENCHANTER_COUNTER;
		if (s == "UNBIND") return BattleSetStackProperty::UNBIND;
		if (s == "CLONED") return BattleSetStackProperty::CLONED;
		if (s == "HAS_CLONE") return BattleSetStackProperty::HAS_CLONE;
		return BattleSetStackProperty::CASTS;
	}
}

class BattleSetStackPropertyCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BattleSetStackProperty"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BattleSetStackProperty *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BattleSetStackProperty>();

		if (json["battleID"].isNumber())
			pack->battleID = BattleID(static_cast<int32_t>(json["battleID"].Integer()));

		if (json["stackID"].isNumber())
			pack->stackID = static_cast<int>(json["stackID"].Integer());

		if (json["which"].isString())
			pack->which = propertyFromString(json["which"].String());

		if (json["val"].isNumber())
			pack->val = static_cast<int>(json["val"].Integer());

		if (json["absolute"].isNumber())
			pack->absolute = static_cast<int>(json["absolute"].Integer());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BattleSetStackProperty &>(pack);

		out["type"].String() = typeName();
		out["battleID"].Integer() = static_cast<int64_t>(p.battleID.getNum());
		out["stackID"].Integer() = static_cast<int64_t>(p.stackID);
		out["which"].String() = propertyToString(p.which);
		out["val"].Integer() = static_cast<int64_t>(p.val);
		out["absolute"].Integer() = static_cast<int64_t>(p.absolute);
	}
};

REGISTER_PACK_CODEC(BattleSetStackPropertyCodec)

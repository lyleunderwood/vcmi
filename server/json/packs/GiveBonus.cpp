/*
 * GiveBonus.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for GiveBonus (server -> client state update: attach a Bonus to
 * an object, player, battle, or hero-commander).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/GiveBonus.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

namespace
{
std::string giveBonusTargetToString(GiveBonus::ETarget v)
{
	switch (v)
	{
		case GiveBonus::ETarget::OBJECT: return "OBJECT";
		case GiveBonus::ETarget::PLAYER: return "PLAYER";
		case GiveBonus::ETarget::BATTLE: return "BATTLE";
		case GiveBonus::ETarget::HERO_COMMANDER: return "HERO_COMMANDER";
	}
	return "OBJECT";
}

GiveBonus::ETarget giveBonusTargetFromString(const std::string & s)
{
	if (s == "OBJECT") return GiveBonus::ETarget::OBJECT;
	if (s == "PLAYER") return GiveBonus::ETarget::PLAYER;
	if (s == "BATTLE") return GiveBonus::ETarget::BATTLE;
	if (s == "HERO_COMMANDER") return GiveBonus::ETarget::HERO_COMMANDER;
	return GiveBonus::ETarget::OBJECT;
}

GiveBonus::VariantType makeVariantId(GiveBonus::ETarget who, int32_t num)
{
	switch (who)
	{
		case GiveBonus::ETarget::OBJECT:
		case GiveBonus::ETarget::HERO_COMMANDER:
			return GiveBonus::VariantType(ObjectInstanceID(num));
		case GiveBonus::ETarget::PLAYER:
			return GiveBonus::VariantType(PlayerColor(num));
		case GiveBonus::ETarget::BATTLE:
			return GiveBonus::VariantType(BattleID(num));
	}
	return GiveBonus::VariantType(ObjectInstanceID(num));
}
}

class GiveBonusCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "GiveBonus"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const GiveBonus *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<GiveBonus>();

		if (json["who"].isString())
			pack->who = giveBonusTargetFromString(json["who"].String());

		if (json["id"].isNumber())
			pack->id = makeVariantId(pack->who, static_cast<int32_t>(json["id"].Integer()));

		// bonus is an opaque Bonus struct; left default-constructed.
		// See "Tricky bits" in the codec report.

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const GiveBonus &>(pack);

		out["type"].String() = typeName();
		out["who"].String() = giveBonusTargetToString(p.who);
		out["id"].Integer() = static_cast<int64_t>(p.id.getNum());
		// bonus: opaque, emit empty object placeholder.
		out["bonus"].Struct();
	}
};

REGISTER_PACK_CODEC(GiveBonusCodec)

/*
 * RemoveBonus.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for RemoveBonus (server -> client state update: remove a
 * previously-attached Bonus from an object, player, battle, or hero).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/RemoveBonus.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"
#include "../../../lib/bonuses/BonusEnum.h"
#include "../../../lib/bonuses/BonusCustomTypes.h"

namespace
{
std::string removeBonusTargetToString(GiveBonus::ETarget v)
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

GiveBonus::ETarget removeBonusTargetFromString(const std::string & s)
{
	if (s == "OBJECT") return GiveBonus::ETarget::OBJECT;
	if (s == "PLAYER") return GiveBonus::ETarget::PLAYER;
	if (s == "BATTLE") return GiveBonus::ETarget::BATTLE;
	if (s == "HERO_COMMANDER") return GiveBonus::ETarget::HERO_COMMANDER;
	return GiveBonus::ETarget::OBJECT;
}

// whoID is VariantIdentifier<HeroTypeID, PlayerColor, BattleID, ObjectInstanceID>.
// Reconstruct the underlying variant based on the `who` target.
using WhoIDVariant = VariantIdentifier<HeroTypeID, PlayerColor, BattleID, ObjectInstanceID>;

WhoIDVariant makeWhoID(GiveBonus::ETarget who, int32_t num)
{
	switch (who)
	{
		case GiveBonus::ETarget::OBJECT:
			return WhoIDVariant(ObjectInstanceID(num));
		case GiveBonus::ETarget::PLAYER:
			return WhoIDVariant(PlayerColor(num));
		case GiveBonus::ETarget::BATTLE:
			return WhoIDVariant(BattleID(num));
		case GiveBonus::ETarget::HERO_COMMANDER:
			return WhoIDVariant(HeroTypeID(num));
	}
	return WhoIDVariant(ObjectInstanceID(num));
}
}

class RemoveBonusCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "RemoveBonus"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const RemoveBonus *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<RemoveBonus>();

		if (json["who"].isString())
			pack->who = removeBonusTargetFromString(json["who"].String());

		if (json["whoID"].isNumber())
			pack->whoID = makeWhoID(pack->who, static_cast<int32_t>(json["whoID"].Integer()));

		if (json["source"].isNumber())
			pack->source = static_cast<BonusSource>(json["source"].Integer());

		// `id` (BonusSourceID) is a VariantIdentifier whose active alternative
		// depends on `source`. The runtime cannot recover the alternative from
		// just `source` without a switch — left default-constructed for now;
		// the raw int is preserved on the JSON side for round-tripping wire
		// data. See "Tricky bits" in the codec report.
		// bonus is a locally-used copy of the removed Bonus; not serialized.

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const RemoveBonus &>(pack);

		out["type"].String() = typeName();
		out["source"].Integer() = static_cast<int64_t>(p.source);
		out["id"].Integer() = static_cast<int64_t>(p.id.getNum());
		out["who"].String() = removeBonusTargetToString(p.who);
		out["whoID"].Integer() = static_cast<int64_t>(p.whoID.getNum());
	}
};

REGISTER_PACK_CODEC(RemoveBonusCodec)

/*
 * SetAvailableHero.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for SetAvailableHero (server -> client update setting a
 * single tavern hero slot — slotID, role, hero, accompanying army,
 * replenish-points flag — for the given player's tavern pool).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/SetAvailableHero.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/gameState/TavernSlot.h"
#include "../../../lib/mapObjects/army/CSimpleArmy.h"
#include "../../../lib/constants/EntityIdentifiers.h"

namespace
{

std::string tavernHeroSlotToString(TavernHeroSlot s)
{
	switch (s)
	{
		case TavernHeroSlot::NATIVE: return "NATIVE";
		case TavernHeroSlot::RANDOM: return "RANDOM";
		case TavernHeroSlot::NONE:
		default:                     return "NONE";
	}
}

TavernHeroSlot tavernHeroSlotFromString(const std::string & s)
{
	if (s == "NATIVE") return TavernHeroSlot::NATIVE;
	if (s == "RANDOM") return TavernHeroSlot::RANDOM;
	return TavernHeroSlot::NONE;
}

std::string tavernSlotRoleToString(TavernSlotRole r)
{
	switch (r)
	{
		case TavernSlotRole::SINGLE_UNIT: return "SINGLE_UNIT";
		case TavernSlotRole::FULL_ARMY:   return "FULL_ARMY";
		case TavernSlotRole::RETREATED:   return "RETREATED";
		case TavernSlotRole::SURRENDERED: return "SURRENDERED";
		case TavernSlotRole::NONE:
		default:                          return "NONE";
	}
}

TavernSlotRole tavernSlotRoleFromString(const std::string & s)
{
	if (s == "SINGLE_UNIT") return TavernSlotRole::SINGLE_UNIT;
	if (s == "FULL_ARMY")   return TavernSlotRole::FULL_ARMY;
	if (s == "RETREATED")   return TavernSlotRole::RETREATED;
	if (s == "SURRENDERED") return TavernSlotRole::SURRENDERED;
	return TavernSlotRole::NONE;
}

} // namespace

class SetAvailableHeroCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "SetAvailableHero"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const SetAvailableHero *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<SetAvailableHero>();

		if (json["slotID"].isString())
			pack->slotID = tavernHeroSlotFromString(json["slotID"].String());

		if (json["roleID"].isString())
			pack->roleID = tavernSlotRoleFromString(json["roleID"].String());

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		if (json["hid"].isNumber())
			pack->hid = HeroTypeID(static_cast<int32_t>(json["hid"].Integer()));

		pack->army.clearSlots();
		if (json["army"].isVector())
		{
			for (const auto & entry : json["army"].Vector())
			{
				if (!entry.isStruct())
					continue;
				SlotID slot(static_cast<int32_t>(entry["slot"].Integer()));
				CreatureID creature(static_cast<int32_t>(entry["creature"].Integer()));
				TQuantity count = static_cast<TQuantity>(entry["count"].Integer());
				pack->army.setCreature(slot, creature, count);
			}
		}

		if (json["replenishPoints"].isBool())
			pack->replenishPoints = json["replenishPoints"].Bool();

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const SetAvailableHero &>(pack);

		out["type"].String() = typeName();
		out["slotID"].String() = tavernHeroSlotToString(p.slotID);
		out["roleID"].String() = tavernSlotRoleToString(p.roleID);
		out["player"] = homamweb::shared::playerColorToJson(p.player);
		out["hid"].Integer() = static_cast<int64_t>(p.hid.getNum());

		JsonNode & army = out["army"];
		army.Vector(); // ensure vector type even when empty
		for (const auto & slot : p.army.army)
		{
			JsonNode entry;
			entry["slot"].Integer() = static_cast<int64_t>(slot.first.getNum());
			entry["creature"].Integer() = static_cast<int64_t>(slot.second.first.getNum());
			entry["count"].Integer() = static_cast<int64_t>(slot.second.second);
			army.Vector().push_back(entry);
		}

		out["replenishPoints"].Bool() = p.replenishPoints;
	}
};

REGISTER_PACK_CODEC(SetAvailableHeroCodec)

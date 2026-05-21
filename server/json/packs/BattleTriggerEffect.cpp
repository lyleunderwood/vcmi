/*
 * BattleTriggerEffect.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BattleTriggerEffect (server -> client battle state update:
 * a bonus-driven effect triggers on a stack at the beginning of its turn,
 * e.g. mana drain, regeneration tick, poison damage).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClientBattle.h
 * TypeScript twin:      wrapper/src/codecs/battle/BattleTriggerEffect.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClientBattle.h"
#include "../../../lib/constants/EntityIdentifiers.h"
#include "../../../lib/bonuses/BonusEnum.h"

class BattleTriggerEffectCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BattleTriggerEffect"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BattleTriggerEffect *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BattleTriggerEffect>();

		if (json["battleID"].isNumber())
			pack->battleID = BattleID(static_cast<int32_t>(json["battleID"].Integer()));

		if (json["stackID"].isNumber())
			pack->stackID = static_cast<int>(json["stackID"].Integer());

		if (json["effect"].isNumber())
			pack->effect = static_cast<BonusType>(json["effect"].Integer());

		if (json["val"].isNumber())
			pack->val = static_cast<int>(json["val"].Integer());

		if (json["additionalInfo"].isNumber())
			pack->additionalInfo = static_cast<int>(json["additionalInfo"].Integer());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BattleTriggerEffect &>(pack);

		out["type"].String() = typeName();
		out["battleID"].Integer() = static_cast<int64_t>(p.battleID.getNum());
		out["stackID"].Integer() = static_cast<int64_t>(p.stackID);
		out["effect"].Integer() = static_cast<int64_t>(p.effect);
		out["val"].Integer() = static_cast<int64_t>(p.val);
		out["additionalInfo"].Integer() = static_cast<int64_t>(p.additionalInfo);
	}
};

REGISTER_PACK_CODEC(BattleTriggerEffectCodec)

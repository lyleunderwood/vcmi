/*
 * BattleCancelled.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BattleCancelled (server -> client). Signals that an
 * in-progress battle has been cancelled (e.g., replay declined). Carries
 * only the battleID.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClientBattle.h
 * TypeScript twin:      wrapper/src/codecs/battle/BattleCancelled.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClientBattle.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class BattleCancelledCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BattleCancelled"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BattleCancelled *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BattleCancelled>();

		if (json["battleID"].isNumber())
			pack->battleID = BattleID(static_cast<int32_t>(json["battleID"].Integer()));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BattleCancelled &>(pack);

		out["type"].String() = typeName();
		out["battleID"].Integer() = static_cast<int64_t>(p.battleID.getNum());
	}
};

REGISTER_PACK_CODEC(BattleCancelledCodec)

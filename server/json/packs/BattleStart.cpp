/*
 * BattleStart.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BattleStart (server -> client battle state update:
 * notifies clients that a new battle has begun and carries the full
 * BattleInfo snapshot describing it).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClientBattle.h
 * TypeScript twin:      wrapper/src/codecs/battle/BattleStart.ts
 *
 * NOTE: `info` (std::unique_ptr<BattleInfo>) is a heavyweight VCMI engine
 * struct whose full field model is not in scope for this codec. It is
 * treated as an opaque JSON blob on the wire: toJson emits an empty struct
 * placeholder when the pointer is non-null (and omits the field when null),
 * and fromJson leaves the pointer as nullptr. A future codec pass should
 * promote BattleInfo to a shared codec.
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClientBattle.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class BattleStartCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BattleStart"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BattleStart *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BattleStart>();

		if (json["battleID"].isNumber())
			pack->battleID = BattleID(static_cast<int32_t>(json["battleID"].Integer()));

		// `info` is opaque on the wire; leave as nullptr.
		// (Future work: deserialize from json["info"].)

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BattleStart &>(pack);

		out["type"].String() = typeName();
		out["battleID"].Integer() = static_cast<int64_t>(p.battleID.getNum());

		if (p.info)
			out["info"].Struct(); // opaque placeholder
		// else: leave field absent / null
	}
};

REGISTER_PACK_CODEC(BattleStartCodec)

/*
 * SetRewardableConfiguration.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for SetRewardableConfiguration (server -> client: replaces the
 * rewardable configuration on a given map object or town building).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/SetRewardableConfiguration.h
 * TypeScript twin:      wrapper/src/codecs/client/SetRewardableConfiguration.ts
 *
 * NOTE: `configuration` is a `Rewardable::Configuration` value containing
 * deeply-nested engine types (Limiter, Reward, VisitInfo with MetaString,
 * Variables with embedded JsonNode preset values, etc.). A full structural
 * codec is far out of scope for a single-pack codec, so it is treated as an
 * opaque JSON blob: fromJson leaves the field default-constructed and
 * toJson emits an empty struct placeholder. A future codec pass should
 * promote Rewardable::Configuration to a shared codec.
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/SetRewardableConfiguration.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class SetRewardableConfigurationCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "SetRewardableConfiguration"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const SetRewardableConfiguration *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<SetRewardableConfiguration>();

		if (json["objectID"].isNumber())
			pack->objectID = ObjectInstanceID(static_cast<int32_t>(json["objectID"].Integer()));

		if (json["buildingID"].isNumber())
			pack->buildingID = BuildingID(static_cast<int32_t>(json["buildingID"].Integer()));

		// configuration: opaque on the wire; leave default-constructed.
		// (Future work: deserialize from json["configuration"].)

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const SetRewardableConfiguration &>(pack);

		out["type"].String() = typeName();
		out["objectID"].Integer() = static_cast<int64_t>(p.objectID.getNum());
		out["buildingID"].Integer() = static_cast<int64_t>(p.buildingID.getNum());
		out["configuration"].Struct(); // opaque placeholder
	}
};

REGISTER_PACK_CODEC(SetRewardableConfigurationCodec)

/*
 * GiveStackExperience.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for GiveStackExperience (server -> client state update: grant
 * experience to specific creature stacks belonging to a hero/garrison).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/GiveStackExperience.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class GiveStackExperienceCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "GiveStackExperience"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const GiveStackExperience *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<GiveStackExperience>();

		if (json["id"].isNumber())
			pack->id = ObjectInstanceID(static_cast<int32_t>(json["id"].Integer()));

		if (json["val"].isVector())
		{
			for (const auto & entry : json["val"].Vector())
			{
				SlotID slot;
				if (entry["slot"].isNumber())
					slot = SlotID(static_cast<int32_t>(entry["slot"].Integer()));
				si64 value = 0;
				if (entry["value"].isNumber())
					value = static_cast<si64>(entry["value"].Integer());
				pack->val[slot] = value;
			}
		}

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const GiveStackExperience &>(pack);

		out["type"].String() = typeName();
		out["id"].Integer() = static_cast<int64_t>(p.id.getNum());

		JsonNode & val = out["val"];
		val.Vector();
		for (const auto & kv : p.val)
		{
			JsonNode entry;
			entry.Struct();
			entry["slot"].Integer() = static_cast<int64_t>(kv.first.getNum());
			entry["value"].Integer() = static_cast<int64_t>(kv.second);
			val.Vector().push_back(entry);
		}
	}
};

REGISTER_PACK_CODEC(GiveStackExperienceCodec)

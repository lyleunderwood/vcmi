/*
 * SetHeroExperience.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for SetHeroExperience (server -> client state update: set a
 * hero's experience points, either as a delta or as a replacement).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/SetHeroExperience.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/ChangeValueMode.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class SetHeroExperienceCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "SetHeroExperience"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const SetHeroExperience *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<SetHeroExperience>();

		pack->mode = homamweb::shared::changeValueModeFromJson(json["mode"]);

		if (json["id"].isNumber())
			pack->id = ObjectInstanceID(static_cast<int32_t>(json["id"].Integer()));

		if (json["val"].isNumber())
			pack->val = static_cast<si64>(json["val"].Integer());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const SetHeroExperience &>(pack);

		out["type"].String() = typeName();
		out["mode"] = homamweb::shared::changeValueModeToJson(p.mode);
		out["id"].Integer() = static_cast<int64_t>(p.id.getNum());
		out["val"].Integer() = static_cast<int64_t>(p.val);
	}
};

REGISTER_PACK_CODEC(SetHeroExperienceCodec)

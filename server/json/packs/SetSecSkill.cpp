/*
 * SetSecSkill.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for SetSecSkill (server -> client state update: set a
 * hero's level in a secondary skill, either relative or absolute).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/SetSecSkill.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/ChangeValueMode.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class SetSecSkillCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "SetSecSkill"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const SetSecSkill *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<SetSecSkill>();

		pack->mode = homamweb::shared::changeValueModeFromJson(json["mode"]);

		if (json["id"].isNumber())
			pack->id = ObjectInstanceID(static_cast<int32_t>(json["id"].Integer()));

		if (json["which"].isNumber())
			pack->which = SecondarySkill(static_cast<int32_t>(json["which"].Integer()));

		if (json["val"].isNumber())
			pack->val = static_cast<ui16>(json["val"].Integer());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const SetSecSkill &>(pack);

		out["type"].String() = typeName();
		out["mode"] = homamweb::shared::changeValueModeToJson(p.mode);
		out["id"].Integer() = static_cast<int64_t>(p.id.getNum());
		out["which"].Integer() = static_cast<int64_t>(p.which.getNum());
		out["val"].Integer() = static_cast<int64_t>(p.val);
	}
};

REGISTER_PACK_CODEC(SetSecSkillCodec)

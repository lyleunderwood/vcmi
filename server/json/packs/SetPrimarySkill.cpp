/*
 * SetPrimarySkill.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for SetPrimarySkill (server -> client state update: change a
 * hero's primary skill value, either RELATIVE delta or ABSOLUTE replacement).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/SetPrimarySkill.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/ChangeValueMode.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

namespace
{
// Hand-rolled PrimarySkill enum mapping. We avoid PrimarySkill::encode /
// PrimarySkill::decode because those go through the engine's identifier
// resolver (and indexing NPrimarySkill::names), which is unsafe at codec
// time and doesn't round-trip NONE. Promote to a shared codec if reused.
std::string primarySkillToString(PrimarySkill which)
{
	const si32 i = which.getNum();
	if (i == PrimarySkill::ATTACK.getNum()) return "ATTACK";
	if (i == PrimarySkill::DEFENSE.getNum()) return "DEFENSE";
	if (i == PrimarySkill::SPELL_POWER.getNum()) return "SPELL_POWER";
	if (i == PrimarySkill::KNOWLEDGE.getNum()) return "KNOWLEDGE";
	return "NONE";
}

PrimarySkill primarySkillFromString(const std::string & s)
{
	if (s == "ATTACK") return PrimarySkill::ATTACK;
	if (s == "DEFENSE") return PrimarySkill::DEFENSE;
	if (s == "SPELL_POWER") return PrimarySkill::SPELL_POWER;
	if (s == "KNOWLEDGE") return PrimarySkill::KNOWLEDGE;
	return PrimarySkill::NONE;
}
} // namespace

class SetPrimarySkillCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "SetPrimarySkill"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const SetPrimarySkill *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<SetPrimarySkill>();

		pack->mode = homamweb::shared::changeValueModeFromJson(json["mode"]);

		if (json["id"].isNumber())
			pack->id = ObjectInstanceID(static_cast<int32_t>(json["id"].Integer()));

		if (json["which"].isString())
			pack->which = primarySkillFromString(json["which"].String());

		if (json["val"].isNumber())
			pack->val = static_cast<si64>(json["val"].Integer());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const SetPrimarySkill &>(pack);

		out["type"].String() = typeName();
		out["mode"] = homamweb::shared::changeValueModeToJson(p.mode);
		out["id"].Integer() = static_cast<int64_t>(p.id.getNum());
		out["which"].String() = primarySkillToString(p.which);
		out["val"].Integer() = static_cast<int64_t>(p.val);
	}
};

REGISTER_PACK_CODEC(SetPrimarySkillCodec)

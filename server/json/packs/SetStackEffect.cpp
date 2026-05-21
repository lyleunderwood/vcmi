/*
 * SetStackEffect.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for SetStackEffect (server -> client battle state update:
 * add/update/remove sets of Bonus effects on battle stacks identified by
 * unit id).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/SetStackEffect.h
 * TypeScript twin:      wrapper/src/codecs/battle/SetStackEffect.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/SetStackEffect.h"
#include "../../../lib/constants/EntityIdentifiers.h"

namespace
{

using BonusGroup = std::vector<std::pair<ui32, std::vector<Bonus>>>;

void decodeBonusGroup(const JsonNode & json, BonusGroup & out)
{
	if (!json.isVector())
		return;
	for (const auto & entry : json.Vector())
	{
		ui32 stackID = 0;
		if (entry["stackID"].isNumber())
			stackID = static_cast<ui32>(entry["stackID"].Integer());
		// bonuses is opaque; inbound list is left empty.
		// See "Tricky bits" in the codec report.
		out.emplace_back(stackID, std::vector<Bonus>{});
	}
}

JsonNode encodeBonusGroup(const BonusGroup & group)
{
	JsonNode arr;
	arr.Vector();
	for (const auto & pair : group)
	{
		JsonNode entry;
		entry["stackID"].Integer() = static_cast<int64_t>(pair.first);
		// bonuses: opaque, emit empty array placeholder (count preserved separately).
		entry["bonuses"].Vector();
		arr.Vector().push_back(entry);
	}
	return arr;
}

} // namespace

class SetStackEffectCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "SetStackEffect"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const SetStackEffect *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<SetStackEffect>();

		if (json["battleID"].isNumber())
			pack->battleID = BattleID(static_cast<int32_t>(json["battleID"].Integer()));

		decodeBonusGroup(json["toAdd"], pack->toAdd);
		decodeBonusGroup(json["toUpdate"], pack->toUpdate);
		decodeBonusGroup(json["toRemove"], pack->toRemove);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const SetStackEffect &>(pack);

		out["type"].String() = typeName();
		out["battleID"].Integer() = static_cast<int64_t>(p.battleID.getNum());
		out["toAdd"] = encodeBonusGroup(p.toAdd);
		out["toUpdate"] = encodeBonusGroup(p.toUpdate);
		out["toRemove"] = encodeBonusGroup(p.toRemove);
	}
};

REGISTER_PACK_CODEC(SetStackEffectCodec)

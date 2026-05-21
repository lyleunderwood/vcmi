/*
 * SetMana.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for SetMana (server -> client state update: set a hero's
 * current mana value, either as a delta or as a replacement depending on
 * ChangeValueMode).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/SetMana.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/ChangeValueMode.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"
#include "../../../lib/constants/Enumerations.h"

class SetManaCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "SetMana"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const SetMana *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<SetMana>();

		if (json["hid"].isNumber())
			pack->hid = ObjectInstanceID(static_cast<int32_t>(json["hid"].Integer()));

		if (json["val"].isNumber())
			pack->val = static_cast<si32>(json["val"].Integer());

		pack->mode = homamweb::shared::changeValueModeFromJson(json["mode"]);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const SetMana &>(pack);

		out["type"].String() = typeName();
		out["hid"].Integer() = static_cast<int64_t>(p.hid.getNum());
		out["val"].Integer() = static_cast<int64_t>(p.val);
		out["mode"] = homamweb::shared::changeValueModeToJson(p.mode);
	}
};

REGISTER_PACK_CODEC(SetManaCodec)

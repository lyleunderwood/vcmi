/*
 * ChangeTactics.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for ChangeTactics (server -> client state update:
 * toggle whether a hero has tactics enabled in battle).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/ChangeTactics.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class ChangeTacticsCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "ChangeTactics"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const ChangeTactics *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<ChangeTactics>();

		if (json["hid"].isNumber())
			pack->hid = ObjectInstanceID(static_cast<int32_t>(json["hid"].Integer()));

		if (json["enabled"].isBool())
			pack->enabled = json["enabled"].Bool();

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const ChangeTactics &>(pack);

		out["type"].String() = typeName();
		out["hid"].Integer() = static_cast<int64_t>(p.hid.getNum());
		out["enabled"].Bool() = p.enabled;
	}
};

REGISTER_PACK_CODEC(ChangeTacticsCodec)

/*
 * SetTactics.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for SetTactics (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/SetTactics.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class SetTacticsCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "SetTactics"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const SetTactics *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<SetTactics>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["hid"].isNumber())
			pack->hid = ObjectInstanceID(static_cast<int32_t>(json["hid"].Integer()));

		if (json["enabled"].isBool())
			pack->enabled = json["enabled"].Bool();

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const SetTactics &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["hid"].Integer() = static_cast<int64_t>(p.hid.getNum());
		out["enabled"].Bool() = p.enabled;
	}
};

REGISTER_PACK_CODEC(SetTacticsCodec)

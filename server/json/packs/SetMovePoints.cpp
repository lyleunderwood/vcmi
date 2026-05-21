/*
 * SetMovePoints.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for SetMovePoints (server -> client state update: set a
 * hero's remaining movement points).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/SetMovePoints.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class SetMovePointsCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "SetMovePoints"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const SetMovePoints *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<SetMovePoints>();

		if (json["hid"].isNumber())
			pack->hid = ObjectInstanceID(static_cast<int32_t>(json["hid"].Integer()));

		if (json["val"].isNumber())
			pack->val = static_cast<si32>(json["val"].Integer());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const SetMovePoints &>(pack);

		out["type"].String() = typeName();
		out["hid"].Integer() = static_cast<int64_t>(p.hid.getNum());
		out["val"].Integer() = static_cast<int64_t>(p.val);
	}
};

REGISTER_PACK_CODEC(SetMovePointsCodec)

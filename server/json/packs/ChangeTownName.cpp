/*
 * ChangeTownName.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for ChangeTownName (server -> client state update: rename a
 * town instance).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/ChangeTownName.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class ChangeTownNameCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "ChangeTownName"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const ChangeTownName *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<ChangeTownName>();

		if (json["tid"].isNumber())
			pack->tid = ObjectInstanceID(static_cast<int32_t>(json["tid"].Integer()));

		if (json["name"].isString())
			pack->name = json["name"].String();

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const ChangeTownName &>(pack);

		out["type"].String() = typeName();
		out["tid"].Integer() = static_cast<int64_t>(p.tid.getNum());
		out["name"].String() = p.name;
	}
};

REGISTER_PACK_CODEC(ChangeTownNameCodec)

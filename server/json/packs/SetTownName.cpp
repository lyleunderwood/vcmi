/*
 * SetTownName.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for SetTownName (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/SetTownName.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class SetTownNameCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "SetTownName"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const SetTownName *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<SetTownName>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["tid"].isNumber())
			pack->tid = ObjectInstanceID(static_cast<int32_t>(json["tid"].Integer()));

		if (json["name"].isString())
			pack->name = json["name"].String();

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const SetTownName &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["tid"].Integer() = static_cast<int64_t>(p.tid.getNum());
		out["name"].String() = p.name;
	}
};

REGISTER_PACK_CODEC(SetTownNameCodec)

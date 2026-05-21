/*
 * BuildBoat.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BuildBoat (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/BuildBoat.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class BuildBoatCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BuildBoat"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BuildBoat *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BuildBoat>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["objid"].isNumber())
			pack->objid = ObjectInstanceID(static_cast<int32_t>(json["objid"].Integer()));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BuildBoat &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["objid"].Integer() = static_cast<int64_t>(p.objid.getNum());
	}
};

REGISTER_PACK_CODEC(BuildBoatCodec)

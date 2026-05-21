/*
 * BuildStructure.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BuildStructure (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/BuildStructure.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class BuildStructureCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BuildStructure"; }

	bool matches(const CPack & pack) const override
	{
		// Use typeid to avoid matching the RazeStructure subclass.
		return typeid(pack) == typeid(BuildStructure);
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BuildStructure>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["tid"].isNumber())
			pack->tid = ObjectInstanceID(static_cast<int32_t>(json["tid"].Integer()));

		if (json["bid"].isNumber())
			pack->bid = BuildingID(static_cast<int32_t>(json["bid"].Integer()));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BuildStructure &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["tid"].Integer() = static_cast<int64_t>(p.tid.getNum());
		out["bid"].Integer() = static_cast<int64_t>(p.bid.getNum());
	}
};

REGISTER_PACK_CODEC(BuildStructureCodec)

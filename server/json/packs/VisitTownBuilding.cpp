/*
 * VisitTownBuilding.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for VisitTownBuilding (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/VisitTownBuilding.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class VisitTownBuildingCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "VisitTownBuilding"; }

	bool matches(const CPack & pack) const override
	{
		return typeid(pack) == typeid(VisitTownBuilding);
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<VisitTownBuilding>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["tid"].isNumber())
			pack->tid = ObjectInstanceID(static_cast<int32_t>(json["tid"].Integer()));

		if (json["bid"].isNumber())
			pack->bid = BuildingID(static_cast<int32_t>(json["bid"].Integer()));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const VisitTownBuilding &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["tid"].Integer() = static_cast<int64_t>(p.tid.getNum());
		out["bid"].Integer() = static_cast<int64_t>(p.bid.getNum());
	}
};

REGISTER_PACK_CODEC(VisitTownBuildingCodec)

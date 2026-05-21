/*
 * RazeStructures.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for RazeStructures (server -> client state update: announce
 * the set of buildings newly razed in a town this turn).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/RazeStructures.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class RazeStructuresCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "RazeStructures"; }

	bool matches(const CPack & pack) const override
	{
		// RazeStructures and NewStructures are siblings (both derive directly
		// from CPackForClient), so dynamic_cast is unambiguous here.
		return dynamic_cast<const RazeStructures *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<RazeStructures>();

		if (json["tid"].isNumber())
			pack->tid = ObjectInstanceID(static_cast<int32_t>(json["tid"].Integer()));

		if (json["bid"].isVector())
		{
			for (const auto & b : json["bid"].Vector())
				pack->bid.insert(BuildingID(static_cast<int32_t>(b.Integer())));
		}

		if (json["destroyed"].isNumber())
			pack->destroyed = static_cast<si16>(json["destroyed"].Integer());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const RazeStructures &>(pack);

		out["type"].String() = typeName();
		out["tid"].Integer() = static_cast<int64_t>(p.tid.getNum());

		JsonNode & bid = out["bid"];
		bid.Vector();
		for (const auto & b : p.bid)
		{
			JsonNode entry;
			entry.Integer() = static_cast<int64_t>(b.getNum());
			bid.Vector().push_back(entry);
		}

		out["destroyed"].Integer() = static_cast<int64_t>(p.destroyed);
	}
};

REGISTER_PACK_CODEC(RazeStructuresCodec)

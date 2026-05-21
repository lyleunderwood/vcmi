/*
 * NewStructures.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for NewStructures (server -> client state update: announce
 * the set of buildings newly built in a town this turn).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/NewStructures.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class NewStructuresCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "NewStructures"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const NewStructures *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<NewStructures>();

		if (json["tid"].isNumber())
			pack->tid = ObjectInstanceID(static_cast<int32_t>(json["tid"].Integer()));

		if (json["bid"].isVector())
		{
			for (const auto & b : json["bid"].Vector())
				pack->bid.insert(BuildingID(static_cast<int32_t>(b.Integer())));
		}

		if (json["built"].isNumber())
			pack->built = static_cast<si16>(json["built"].Integer());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const NewStructures &>(pack);

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

		out["built"].Integer() = static_cast<int64_t>(p.built);
	}
};

REGISTER_PACK_CODEC(NewStructuresCodec)

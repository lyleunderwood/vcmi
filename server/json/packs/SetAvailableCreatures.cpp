/*
 * SetAvailableCreatures.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for SetAvailableCreatures (server -> client: updates the
 * creature roster available for recruitment at a dwelling/town object).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 *                       (SetAvailableCreatures)
 * TypeScript twin:      wrapper/src/codecs/client/SetAvailableCreatures.ts
 *
 * The `creatures` field is a nested vector<pair<ui32, vector<CreatureID>>>
 * encoded as an array of `{level, ids}` objects.
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class SetAvailableCreaturesCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "SetAvailableCreatures"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const SetAvailableCreatures *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<SetAvailableCreatures>();

		if (json["tid"].isNumber())
			pack->tid = ObjectInstanceID(static_cast<int32_t>(json["tid"].Integer()));

		if (json["creatures"].isVector())
		{
			for (const auto & entry : json["creatures"].Vector())
			{
				ui32 level = 0;
				if (entry["level"].isNumber())
					level = static_cast<ui32>(entry["level"].Integer());
				std::vector<CreatureID> ids;
				if (entry["ids"].isVector())
				{
					for (const auto & v : entry["ids"].Vector())
						ids.emplace_back(static_cast<int32_t>(v.Integer()));
				}
				pack->creatures.emplace_back(level, ids);
			}
		}

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const SetAvailableCreatures &>(pack);

		out["type"].String() = typeName();
		out["tid"].Integer() = static_cast<int64_t>(p.tid.getNum());

		JsonNode & creatures = out["creatures"];
		creatures.Vector();
		for (const auto & pair : p.creatures)
		{
			JsonNode entry;
			entry.Struct();
			entry["level"].Integer() = static_cast<int64_t>(pair.first);
			JsonNode & ids = entry["ids"];
			ids.Vector();
			for (const auto & cid : pair.second)
			{
				JsonNode v;
				v.Integer() = static_cast<int64_t>(cid.getNum());
				ids.Vector().push_back(v);
			}
			creatures.Vector().push_back(entry);
		}
	}
};

REGISTER_PACK_CODEC(SetAvailableCreaturesCodec)

/*
 * TeleportDialog.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for TeleportDialog (server -> client query offering the player
 * a choice of exits when a hero steps onto a teleport channel with multiple
 * destinations).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/TeleportDialog.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/Int3.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class TeleportDialogCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "TeleportDialog"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const TeleportDialog *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<TeleportDialog>();

		// Inherited from Query.
		if (json["queryID"].isNumber())
			pack->queryID = QueryID(static_cast<int32_t>(json["queryID"].Integer()));

		if (json["hero"].isNumber())
			pack->hero = ObjectInstanceID(static_cast<int32_t>(json["hero"].Integer()));

		if (json["channel"].isNumber())
			pack->channel = TeleportChannelID(static_cast<int32_t>(json["channel"].Integer()));

		if (json["exits"].isVector())
		{
			for (const auto & entry : json["exits"].Vector())
			{
				ObjectInstanceID exitObj;
				if (entry["id"].isNumber())
					exitObj = ObjectInstanceID(static_cast<int32_t>(entry["id"].Integer()));
				int3 pos = homamweb::shared::int3FromJson(entry["pos"]);
				pack->exits.emplace_back(exitObj, pos);
			}
		}

		if (json["impassable"].isBool())
			pack->impassable = json["impassable"].Bool();

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const TeleportDialog &>(pack);

		out["type"].String() = typeName();

		// Inherited from Query.
		out["queryID"].Integer() = static_cast<int64_t>(p.queryID.getNum());

		out["hero"].Integer() = static_cast<int64_t>(p.hero.getNum());
		out["channel"].Integer() = static_cast<int64_t>(p.channel.getNum());

		JsonNode & exits = out["exits"];
		exits.Vector(); // ensure vector type even when empty
		for (const auto & entry : p.exits)
		{
			JsonNode node;
			node["id"].Integer() = static_cast<int64_t>(entry.first.getNum());
			node["pos"] = homamweb::shared::int3ToJson(entry.second);
			exits.Vector().push_back(node);
		}

		out["impassable"].Bool() = p.impassable;
	}
};

REGISTER_PACK_CODEC(TeleportDialogCodec)

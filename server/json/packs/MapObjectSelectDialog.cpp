/*
 * MapObjectSelectDialog.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for MapObjectSelectDialog (server -> client query asking the
 * player to pick one map object from a candidate list, e.g. choosing which
 * town to teleport to via the Town Portal spell).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/MapObjectSelectDialog.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/Component.h"
#include "../shared/MetaString.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/texts/MetaString.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class MapObjectSelectDialogCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "MapObjectSelectDialog"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const MapObjectSelectDialog *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<MapObjectSelectDialog>();

		// Inherited from Query.
		if (json["queryID"].isNumber())
			pack->queryID = QueryID(static_cast<int32_t>(json["queryID"].Integer()));

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		pack->icon = homamweb::shared::componentFromJson(json["icon"]);
		pack->title = homamweb::shared::metaStringFromJson(json["title"]);
		pack->description = homamweb::shared::metaStringFromJson(json["description"]);

		if (json["objects"].isVector())
		{
			for (const auto & entry : json["objects"].Vector())
			{
				if (entry.isNumber())
					pack->objects.emplace_back(static_cast<int32_t>(entry.Integer()));
			}
		}

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const MapObjectSelectDialog &>(pack);

		out["type"].String() = typeName();

		// Inherited from Query.
		out["queryID"].Integer() = static_cast<int64_t>(p.queryID.getNum());

		out["player"] = homamweb::shared::playerColorToJson(p.player);
		out["icon"] = homamweb::shared::componentToJson(p.icon);
		out["title"] = homamweb::shared::metaStringToJson(p.title);
		out["description"] = homamweb::shared::metaStringToJson(p.description);

		JsonNode & objects = out["objects"];
		objects.Vector(); // ensure vector type even when empty
		for (const auto & id : p.objects)
		{
			JsonNode node;
			node.Integer() = static_cast<int64_t>(id.getNum());
			objects.Vector().push_back(node);
		}
	}
};

REGISTER_PACK_CODEC(MapObjectSelectDialogCodec)

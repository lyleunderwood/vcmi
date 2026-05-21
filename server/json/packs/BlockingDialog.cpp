/*
 * BlockingDialog.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BlockingDialog (server -> client query asking the player
 * to confirm/cancel/select a component).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/BlockingDialog.ts
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

class BlockingDialogCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BlockingDialog"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BlockingDialog *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BlockingDialog>();

		// Inherited from Query.
		if (json["queryID"].isNumber())
			pack->queryID = QueryID(static_cast<int32_t>(json["queryID"].Integer()));

		pack->text = homamweb::shared::metaStringFromJson(json["text"]);
		pack->components = homamweb::shared::componentsFromJson(json["components"]);

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		if (json["flags"].isNumber())
			pack->flags = static_cast<ui8>(json["flags"].Integer());
		if (json["soundID"].isNumber())
			pack->soundID = static_cast<ui16>(json["soundID"].Integer());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BlockingDialog &>(pack);

		out["type"].String() = typeName();

		// Inherited from Query.
		out["queryID"].Integer() = static_cast<int64_t>(p.queryID.getNum());

		out["text"] = homamweb::shared::metaStringToJson(p.text);
		out["components"] = homamweb::shared::componentsToJson(p.components);
		out["player"] = homamweb::shared::playerColorToJson(p.player);
		out["flags"].Integer() = static_cast<int64_t>(p.flags);
		out["soundID"].Integer() = static_cast<int64_t>(p.soundID);
	}
};

REGISTER_PACK_CODEC(BlockingDialogCodec)

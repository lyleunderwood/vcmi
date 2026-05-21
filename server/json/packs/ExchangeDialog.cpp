/*
 * ExchangeDialog.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for ExchangeDialog (server -> client query: open the
 * hero-to-hero exchange screen between two heroes owned by `player`).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/ExchangeDialog.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class ExchangeDialogCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "ExchangeDialog"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const ExchangeDialog *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<ExchangeDialog>();

		// Inherited from Query.
		if (json["queryID"].isNumber())
			pack->queryID = QueryID(static_cast<int32_t>(json["queryID"].Integer()));

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		if (json["hero1"].isNumber())
			pack->hero1 = ObjectInstanceID(static_cast<int32_t>(json["hero1"].Integer()));
		if (json["hero2"].isNumber())
			pack->hero2 = ObjectInstanceID(static_cast<int32_t>(json["hero2"].Integer()));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const ExchangeDialog &>(pack);

		out["type"].String() = typeName();

		// Inherited from Query.
		out["queryID"].Integer() = static_cast<int64_t>(p.queryID.getNum());

		out["player"] = homamweb::shared::playerColorToJson(p.player);
		out["hero1"].Integer() = static_cast<int64_t>(p.hero1.getNum());
		out["hero2"].Integer() = static_cast<int64_t>(p.hero2.getNum());
	}
};

REGISTER_PACK_CODEC(ExchangeDialogCodec)

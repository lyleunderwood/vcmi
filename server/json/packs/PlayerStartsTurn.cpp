/*
 * PlayerStartsTurn.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for PlayerStartsTurn (server -> client; a query asking the
 * client to acknowledge the start of a player's turn).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/PlayerStartsTurn.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class PlayerStartsTurnCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "PlayerStartsTurn"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const PlayerStartsTurn *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<PlayerStartsTurn>();

		// Inherited from Query.
		if (json["queryID"].isNumber())
			pack->queryID = QueryID(static_cast<int32_t>(json["queryID"].Integer()));

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const PlayerStartsTurn &>(pack);

		out["type"].String() = typeName();

		// Inherited from Query.
		out["queryID"].Integer() = static_cast<int64_t>(p.queryID.getNum());

		out["player"] = homamweb::shared::playerColorToJson(p.player);
	}
};

REGISTER_PACK_CODEC(PlayerStartsTurnCodec)

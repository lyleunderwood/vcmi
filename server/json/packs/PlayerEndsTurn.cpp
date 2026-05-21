/*
 * PlayerEndsTurn.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for PlayerEndsTurn (server -> client notification that a
 * player has finished their turn).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/PlayerEndsTurn.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"

class PlayerEndsTurnCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "PlayerEndsTurn"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const PlayerEndsTurn *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<PlayerEndsTurn>();

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const PlayerEndsTurn &>(pack);

		out["type"].String() = typeName();
		out["player"] = homamweb::shared::playerColorToJson(p.player);
	}
};

REGISTER_PACK_CODEC(PlayerEndsTurnCodec)

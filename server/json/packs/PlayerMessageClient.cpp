/*
 * PlayerMessageClient.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for PlayerMessageClient (server -> client broadcast of a
 * chat-style message attributed to a player).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/PlayerMessageClient.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"

class PlayerMessageClientCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "PlayerMessageClient"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const PlayerMessageClient *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<PlayerMessageClient>();

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		if (json["text"].isString())
			pack->text = json["text"].String();

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const PlayerMessageClient &>(pack);

		out["type"].String() = typeName();
		out["player"] = homamweb::shared::playerColorToJson(p.player);
		out["text"].String() = p.text;
	}
};

REGISTER_PACK_CODEC(PlayerMessageClientCodec)

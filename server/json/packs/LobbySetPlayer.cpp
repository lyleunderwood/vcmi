/*
 * LobbySetPlayer.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbySetPlayer.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbySetPlayer.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForLobby.h"

class LobbySetPlayerCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbySetPlayer"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbySetPlayer *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbySetPlayer>();
		if (json["clickedColor"].isNumber())
			pack->clickedColor = homamweb::shared::playerColorFromJson(json["clickedColor"]);
		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbySetPlayer &>(pack);
		out["type"].String() = typeName();
		out["clickedColor"] = homamweb::shared::playerColorToJson(p.clickedColor);
	}
};

REGISTER_PACK_CODEC(LobbySetPlayerCodec)

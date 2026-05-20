/*
 * LobbySetPlayerName.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbySetPlayerName.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbySetPlayerName.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForLobby.h"

class LobbySetPlayerNameCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbySetPlayerName"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbySetPlayerName *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbySetPlayerName>();
		if (json["color"].isNumber())
			pack->color = homamweb::shared::playerColorFromJson(json["color"]);
		if (json["name"].isString())
			pack->name = json["name"].String();
		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbySetPlayerName &>(pack);
		out["type"].String() = typeName();
		out["color"] = homamweb::shared::playerColorToJson(p.color);
		out["name"].String() = p.name;
	}
};

REGISTER_PACK_CODEC(LobbySetPlayerNameCodec)

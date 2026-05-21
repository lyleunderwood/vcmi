/*
 * LobbyForceSetPlayer.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbyForceSetPlayer.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbyForceSetPlayer.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForLobby.h"
#include "../../../lib/serializer/PlayerConnectionID.h"

class LobbyForceSetPlayerCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbyForceSetPlayer"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbyForceSetPlayer *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbyForceSetPlayer>();
		if (json["targetConnectedPlayer"].isNumber())
			pack->targetConnectedPlayer = PlayerConnectionID(static_cast<int8_t>(json["targetConnectedPlayer"].Integer()));
		if (json["targetPlayerColor"].isNumber())
			pack->targetPlayerColor = homamweb::shared::playerColorFromJson(json["targetPlayerColor"]);
		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbyForceSetPlayer &>(pack);
		out["type"].String() = typeName();
		out["targetConnectedPlayer"].Integer() = static_cast<int64_t>(p.targetConnectedPlayer);
		out["targetPlayerColor"] = homamweb::shared::playerColorToJson(p.targetPlayerColor);
	}
};

REGISTER_PACK_CODEC(LobbyForceSetPlayerCodec)

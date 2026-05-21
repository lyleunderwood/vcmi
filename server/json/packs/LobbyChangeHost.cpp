/*
 * LobbyChangeHost.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbyChangeHost.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbyChangeHost.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForLobby.h"

class LobbyChangeHostCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbyChangeHost"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbyChangeHost *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbyChangeHost>();

		if (json["newHostConnectionId"].isNumber())
			pack->newHostConnectionId = static_cast<GameConnectionID>(json["newHostConnectionId"].Integer());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbyChangeHost &>(pack);

		out["type"].String() = typeName();
		out["newHostConnectionId"].Integer() = static_cast<int64_t>(p.newHostConnectionId);
	}
};

REGISTER_PACK_CODEC(LobbyChangeHostCodec)

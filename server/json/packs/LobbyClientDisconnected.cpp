/*
 * LobbyClientDisconnected.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbyClientDisconnected.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbyClientDisconnected.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForLobby.h"

class LobbyClientDisconnectedCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbyClientDisconnected"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbyClientDisconnected *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbyClientDisconnected>();
		// clientId is server-assigned in our flow; JsonAdapter fills it from the
		// originating GameConnection. shutdownServer is honored if provided.
		pack->shutdownServer = json["shutdownServer"].Bool();
		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbyClientDisconnected &>(pack);
		out["type"].String() = typeName();
		out["clientId"].Integer() = static_cast<int64_t>(p.clientId);
		out["shutdownServer"].Bool() = p.shutdownServer;
	}
};

REGISTER_PACK_CODEC(LobbyClientDisconnectedCodec)

/*
 * LobbyPrepareStartGame.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbyPrepareStartGame. The pack carries no serialized
 * fields (its serialize() template body is empty), so the codec only
 * round-trips the type discriminator.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbyPrepareStartGame.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForLobby.h"

class LobbyPrepareStartGameCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbyPrepareStartGame"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbyPrepareStartGame *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		(void)json;
		return std::make_unique<LobbyPrepareStartGame>();
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		(void)dynamic_cast<const LobbyPrepareStartGame &>(pack); // type check
		out["type"].String() = typeName();
	}
};

REGISTER_PACK_CODEC(LobbyPrepareStartGameCodec)

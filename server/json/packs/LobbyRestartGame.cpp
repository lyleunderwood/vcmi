/*
 * LobbyRestartGame.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbyRestartGame. The pack carries no serialized
 * fields (its serialize() template body is empty), so the codec only
 * round-trips the type discriminator.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbyRestartGame.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForLobby.h"

class LobbyRestartGameCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbyRestartGame"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbyRestartGame *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		(void)json;
		return std::make_unique<LobbyRestartGame>();
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		(void)dynamic_cast<const LobbyRestartGame &>(pack); // type check
		out["type"].String() = typeName();
	}
};

REGISTER_PACK_CODEC(LobbyRestartGameCodec)

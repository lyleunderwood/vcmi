/*
 * LobbySetDifficulty.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbySetDifficulty.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbySetDifficulty.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForLobby.h"

class LobbySetDifficultyCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbySetDifficulty"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbySetDifficulty *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbySetDifficulty>();

		if (json["difficulty"].isNumber())
			pack->difficulty = static_cast<ui8>(json["difficulty"].Integer());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbySetDifficulty &>(pack);

		out["type"].String() = typeName();
		out["difficulty"].Integer() = static_cast<int64_t>(p.difficulty);
	}
};

REGISTER_PACK_CODEC(LobbySetDifficultyCodec)

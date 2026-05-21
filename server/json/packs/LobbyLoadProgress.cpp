/*
 * LobbyLoadProgress.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbyLoadProgress.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbyLoadProgress.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForLobby.h"

class LobbyLoadProgressCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbyLoadProgress"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbyLoadProgress *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbyLoadProgress>();

		if (json["progress"].isNumber())
			pack->progress = static_cast<unsigned char>(json["progress"].Integer());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbyLoadProgress &>(pack);

		out["type"].String() = typeName();
		out["progress"].Integer() = static_cast<int64_t>(p.progress);
	}
};

REGISTER_PACK_CODEC(LobbyLoadProgressCodec)

/*
 * LobbyQuickLoadGame.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbyQuickLoadGame.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbyQuickLoadGame.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForLobby.h"

class LobbyQuickLoadGameCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbyQuickLoadGame"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbyQuickLoadGame *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbyQuickLoadGame>();
		if (json["saveFilePath"].isString())
			pack->saveFilePath = json["saveFilePath"].String();
		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbyQuickLoadGame &>(pack);
		out["type"].String() = typeName();
		out["saveFilePath"].String() = p.saveFilePath;
	}
};

REGISTER_PACK_CODEC(LobbyQuickLoadGameCodec)

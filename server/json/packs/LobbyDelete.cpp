/*
 * LobbyDelete.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbyDelete.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbyDelete.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForLobby.h"

namespace
{
std::string typeToString(LobbyDelete::EType t)
{
	switch (t)
	{
		case LobbyDelete::EType::SAVEGAME: return "SAVEGAME";
		case LobbyDelete::EType::SAVEGAME_FOLDER: return "SAVEGAME_FOLDER";
		case LobbyDelete::EType::RANDOMMAP: return "RANDOMMAP";
	}
	return "SAVEGAME";
}

LobbyDelete::EType typeFromString(const std::string & s)
{
	if (s == "SAVEGAME") return LobbyDelete::EType::SAVEGAME;
	if (s == "SAVEGAME_FOLDER") return LobbyDelete::EType::SAVEGAME_FOLDER;
	if (s == "RANDOMMAP") return LobbyDelete::EType::RANDOMMAP;
	return LobbyDelete::EType::SAVEGAME;
}
} // namespace

class LobbyDeleteCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbyDelete"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbyDelete *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbyDelete>();

		if (json["deleteType"].isString())
			pack->type = typeFromString(json["deleteType"].String());

		if (json["name"].isString())
			pack->name = json["name"].String();

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbyDelete &>(pack);

		out["type"].String() = typeName();
		out["deleteType"].String() = typeToString(p.type);
		out["name"].String() = p.name;
	}
};

REGISTER_PACK_CODEC(LobbyDeleteCodec)

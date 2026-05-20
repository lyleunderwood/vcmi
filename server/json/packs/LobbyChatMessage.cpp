/*
 * LobbyChatMessage.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbyChatMessage.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbyChatMessage.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/MetaString.h"

#include "../../../lib/networkPacks/PacksForLobby.h"
#include "../../../lib/texts/MetaString.h"

class LobbyChatMessageCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbyChatMessage"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbyChatMessage *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbyChatMessage>();

		if (json["playerName"].isString())
			pack->playerName = json["playerName"].String();

		pack->message = homamweb::shared::metaStringFromJson(json["message"]);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbyChatMessage &>(pack);

		out["type"].String() = typeName();
		out["playerName"].String() = p.playerName;
		out["message"] = homamweb::shared::metaStringToJson(p.message);
	}
};

REGISTER_PACK_CODEC(LobbyChatMessageCodec)

/*
 * LobbyShowMessage.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbyShowMessage.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbyShowMessage.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/MetaString.h"

#include "../../../lib/networkPacks/PacksForLobby.h"
#include "../../../lib/texts/MetaString.h"

class LobbyShowMessageCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbyShowMessage"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbyShowMessage *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbyShowMessage>();
		pack->message = homamweb::shared::metaStringFromJson(json["message"]);
		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbyShowMessage &>(pack);

		out["type"].String() = typeName();
		out["message"] = homamweb::shared::metaStringToJson(p.message);
	}
};

REGISTER_PACK_CODEC(LobbyShowMessageCodec)

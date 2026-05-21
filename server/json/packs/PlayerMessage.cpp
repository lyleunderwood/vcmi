/*
 * PlayerMessage.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for PlayerMessage (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/PlayerMessage.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class PlayerMessageCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "PlayerMessage"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const PlayerMessage *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<PlayerMessage>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["text"].isString())
			pack->text = json["text"].String();

		if (json["currObj"].isNumber())
			pack->currObj = ObjectInstanceID(static_cast<int32_t>(json["currObj"].Integer()));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const PlayerMessage &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["text"].String() = p.text;
		out["currObj"].Integer() = static_cast<int64_t>(p.currObj.getNum());
	}
};

REGISTER_PACK_CODEC(PlayerMessageCodec)

/*
 * SaveGame.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for SaveGame (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/SaveGame.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForServer.h"

class SaveGameCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "SaveGame"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const SaveGame *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<SaveGame>();

		// Inherited from CPackForServer.
		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);
		if (json["requestID"].isNumber())
			pack->requestID = static_cast<uint32_t>(json["requestID"].Integer());

		if (json["notifySuccess"].isBool())
			pack->notifySuccess = json["notifySuccess"].Bool();

		if (json["fname"].isString())
			pack->fname = json["fname"].String();

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const SaveGame &>(pack);

		out["type"].String() = typeName();

		// Inherited from CPackForServer.
		out["player"] = homamweb::shared::playerColorToJson(p.player);
		out["requestID"].Integer() = static_cast<int64_t>(p.requestID);

		out["notifySuccess"].Bool() = p.notifySuccess;
		out["fname"].String() = p.fname;
	}
};

REGISTER_PACK_CODEC(SaveGameCodec)

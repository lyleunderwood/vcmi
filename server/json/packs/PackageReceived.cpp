/*
 * PackageReceived.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for PackageReceived (a server -> client acknowledgment pack).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/PackageReceived.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class PackageReceivedCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "PackageReceived"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const PackageReceived *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<PackageReceived>();

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);
		if (json["requestID"].isNumber())
			pack->requestID = static_cast<uint32_t>(json["requestID"].Integer());
		if (json["packType"].isNumber())
			pack->packType = static_cast<uint16_t>(json["packType"].Integer());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const PackageReceived &>(pack);

		out["type"].String() = typeName();
		out["player"] = homamweb::shared::playerColorToJson(p.player);
		out["requestID"].Integer() = static_cast<int64_t>(p.requestID);
		out["packType"].Integer() = static_cast<int64_t>(p.packType);
	}
};

REGISTER_PACK_CODEC(PackageReceivedCodec)

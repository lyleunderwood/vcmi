/*
 * PackageApplied.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for PackageApplied (server -> client acknowledgement of
 * a previously sent request).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/PackageApplied.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"

class PackageAppliedCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "PackageApplied"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const PackageApplied *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<PackageApplied>();

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		if (json["requestID"].isNumber())
			pack->requestID = static_cast<uint32_t>(json["requestID"].Integer());

		if (json["packType"].isNumber())
			pack->packType = static_cast<uint16_t>(json["packType"].Integer());

		if (json["result"].isBool())
			pack->result = json["result"].Bool();

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const PackageApplied &>(pack);

		out["type"].String() = typeName();
		out["player"] = homamweb::shared::playerColorToJson(p.player);
		out["requestID"].Integer() = static_cast<int64_t>(p.requestID);
		out["packType"].Integer() = static_cast<int64_t>(p.packType);
		out["result"].Bool() = p.result;
	}
};

REGISTER_PACK_CODEC(PackageAppliedCodec)

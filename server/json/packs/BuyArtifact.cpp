/*
 * BuyArtifact.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BuyArtifact (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/BuyArtifact.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class BuyArtifactCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BuyArtifact"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BuyArtifact *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BuyArtifact>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["hid"].isNumber())
			pack->hid = ObjectInstanceID(static_cast<int32_t>(json["hid"].Integer()));

		if (json["aid"].isNumber())
			pack->aid = ArtifactID(static_cast<int32_t>(json["aid"].Integer()));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BuyArtifact &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["hid"].Integer() = static_cast<int64_t>(p.hid.getNum());
		out["aid"].Integer() = static_cast<int64_t>(p.aid.getNum());
	}
};

REGISTER_PACK_CODEC(BuyArtifactCodec)

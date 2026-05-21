/*
 * BulkExchangeArtifacts.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BulkExchangeArtifacts (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/BulkExchangeArtifacts.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class BulkExchangeArtifactsCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BulkExchangeArtifacts"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BulkExchangeArtifacts *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BulkExchangeArtifacts>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["srcHero"].isNumber())
			pack->srcHero = ObjectInstanceID(static_cast<int32_t>(json["srcHero"].Integer()));

		if (json["dstHero"].isNumber())
			pack->dstHero = ObjectInstanceID(static_cast<int32_t>(json["dstHero"].Integer()));

		if (json["swap"].isBool())
			pack->swap = json["swap"].Bool();

		if (json["equipped"].isBool())
			pack->equipped = json["equipped"].Bool();

		if (json["backpack"].isBool())
			pack->backpack = json["backpack"].Bool();

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BulkExchangeArtifacts &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["srcHero"].Integer() = static_cast<int64_t>(p.srcHero.getNum());
		out["dstHero"].Integer() = static_cast<int64_t>(p.dstHero.getNum());
		out["swap"].Bool() = p.swap;
		out["equipped"].Bool() = p.equipped;
		out["backpack"].Bool() = p.backpack;
	}
};

REGISTER_PACK_CODEC(BulkExchangeArtifactsCodec)

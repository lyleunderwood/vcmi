/*
 * ExchangeArtifacts.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for ExchangeArtifacts (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/ExchangeArtifacts.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"
#include "../shared/ArtifactLocation.h"

#include "../../../lib/networkPacks/PacksForServer.h"

class ExchangeArtifactsCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "ExchangeArtifacts"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const ExchangeArtifacts *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<ExchangeArtifacts>();
		homamweb::shared::readServerPackBase(json, *pack);

		pack->src = homamweb::shared::artifactLocationFromJson(json["src"]);
		pack->dst = homamweb::shared::artifactLocationFromJson(json["dst"]);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const ExchangeArtifacts &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["src"] = homamweb::shared::artifactLocationToJson(p.src);
		out["dst"] = homamweb::shared::artifactLocationToJson(p.dst);
	}
};

REGISTER_PACK_CODEC(ExchangeArtifactsCodec)

/*
 * EraseArtifactByClient.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for EraseArtifactByClient (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/EraseArtifactByClient.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"
#include "../shared/ArtifactLocation.h"

#include "../../../lib/networkPacks/PacksForServer.h"

class EraseArtifactByClientCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "EraseArtifactByClient"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const EraseArtifactByClient *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<EraseArtifactByClient>();
		homamweb::shared::readServerPackBase(json, *pack);

		pack->al = homamweb::shared::artifactLocationFromJson(json["al"]);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const EraseArtifactByClient &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["al"] = homamweb::shared::artifactLocationToJson(p.al);
	}
};

REGISTER_PACK_CODEC(EraseArtifactByClientCodec)

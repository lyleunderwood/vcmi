/*
 * AssembleArtifacts.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for AssembleArtifacts (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/AssembleArtifacts.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class AssembleArtifactsCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "AssembleArtifacts"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const AssembleArtifacts *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<AssembleArtifacts>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["heroID"].isNumber())
			pack->heroID = ObjectInstanceID(static_cast<int32_t>(json["heroID"].Integer()));

		if (json["artifactSlot"].isNumber())
			pack->artifactSlot = ArtifactPosition(static_cast<int32_t>(json["artifactSlot"].Integer()));

		if (json["assemble"].isBool())
			pack->assemble = json["assemble"].Bool();

		if (json["assembleTo"].isNumber())
			pack->assembleTo = ArtifactID(static_cast<int32_t>(json["assembleTo"].Integer()));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const AssembleArtifacts &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["heroID"].Integer() = static_cast<int64_t>(p.heroID.getNum());
		out["artifactSlot"].Integer() = static_cast<int64_t>(p.artifactSlot.getNum());
		out["assemble"].Bool() = p.assemble;
		out["assembleTo"].Integer() = static_cast<int64_t>(p.assembleTo.getNum());
	}
};

REGISTER_PACK_CODEC(AssembleArtifactsCodec)

/*
 * NewArtifact.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for NewArtifact (server -> client state update: a new artifact
 * has been added to an artifact-holder object, occupying a given position
 * and optionally bound to a spell).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/NewArtifact.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class NewArtifactCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "NewArtifact"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const NewArtifact *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<NewArtifact>();

		if (json["artHolder"].isNumber())
			pack->artHolder = ObjectInstanceID(static_cast<int32_t>(json["artHolder"].Integer()));

		if (json["artId"].isNumber())
			pack->artId = ArtifactID(static_cast<int32_t>(json["artId"].Integer()));

		if (json["spellId"].isNumber())
			pack->spellId = SpellID(static_cast<int32_t>(json["spellId"].Integer()));

		if (json["pos"].isNumber())
			pack->pos = ArtifactPosition(static_cast<int32_t>(json["pos"].Integer()));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const NewArtifact &>(pack);

		out["type"].String() = typeName();
		out["artHolder"].Integer() = static_cast<int64_t>(p.artHolder.getNum());
		out["artId"].Integer() = static_cast<int64_t>(p.artId.getNum());
		out["spellId"].Integer() = static_cast<int64_t>(p.spellId.getNum());
		out["pos"].Integer() = static_cast<int64_t>(p.pos.getNum());
	}
};

REGISTER_PACK_CODEC(NewArtifactCodec)

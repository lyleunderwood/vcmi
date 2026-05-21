/*
 * SetAvailableArtifacts.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for SetAvailableArtifacts (server -> client state update: set
 * the pool of available artifacts for either town Artifact Merchants
 * (id < 0) or an adventure-map Black Market instance (id >= 0)).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/SetAvailableArtifacts.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class SetAvailableArtifactsCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "SetAvailableArtifacts"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const SetAvailableArtifacts *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<SetAvailableArtifacts>();

		if (json["id"].isNumber())
			pack->id = ObjectInstanceID(static_cast<int32_t>(json["id"].Integer()));

		if (json["arts"].isVector())
		{
			for (const auto & art : json["arts"].Vector())
				pack->arts.push_back(ArtifactID(static_cast<int32_t>(art.Integer())));
		}

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const SetAvailableArtifacts &>(pack);

		out["type"].String() = typeName();
		out["id"].Integer() = static_cast<int64_t>(p.id.getNum());

		JsonNode & arts = out["arts"];
		arts.Vector(); // ensure vector type even when empty
		for (const auto & art : p.arts)
		{
			JsonNode node;
			node.Integer() = static_cast<int64_t>(art.getNum());
			arts.Vector().push_back(node);
		}
	}
};

REGISTER_PACK_CODEC(SetAvailableArtifactsCodec)

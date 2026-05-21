/*
 * ChangeArtifactsCostume.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for ChangeArtifactsCostume (server -> client state update:
 * replace the saved artifact-costume slot `costumeIdx` for `player` with
 * the position->artifact mapping in `costumeSet`).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/ChangeArtifactsCostume.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class ChangeArtifactsCostumeCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "ChangeArtifactsCostume"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const ChangeArtifactsCostume *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<ChangeArtifactsCostume>();

		if (json["costumeSet"].isVector())
		{
			for (const auto & entry : json["costumeSet"].Vector())
			{
				ArtifactPosition pos(static_cast<si32>(entry["position"].Integer()));
				ArtifactID art(static_cast<si32>(entry["artifact"].Integer()));
				pack->costumeSet[pos] = art;
			}
		}

		if (json["costumeIdx"].isNumber())
			pack->costumeIdx = static_cast<uint32_t>(json["costumeIdx"].Integer());

		if (json["player"].isNumber())
		{
			// `player` is declared const; the network deserializer uses the
			// same pattern (it writes via `h & player`). Use const_cast on
			// inbound construction only.
			const_cast<PlayerColor &>(pack->player) =
				homamweb::shared::playerColorFromJson(json["player"]);
		}

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const ChangeArtifactsCostume &>(pack);

		out["type"].String() = typeName();

		JsonNode & costumeSet = out["costumeSet"];
		costumeSet.Vector();
		for (const auto & kv : p.costumeSet)
		{
			JsonNode entry;
			entry["position"].Integer() = static_cast<int64_t>(kv.first.getNum());
			entry["artifact"].Integer() = static_cast<int64_t>(kv.second.getNum());
			costumeSet.Vector().push_back(entry);
		}

		out["costumeIdx"].Integer() = static_cast<int64_t>(p.costumeIdx);
		out["player"] = homamweb::shared::playerColorToJson(p.player);
	}
};

REGISTER_PACK_CODEC(ChangeArtifactsCostumeCodec)

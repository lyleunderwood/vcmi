/*
 * LobbySetPlayerHandicap.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbySetPlayerHandicap.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbySetPlayerHandicap.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForLobby.h"
#include "../../../lib/StartInfo.h"
#include "../../../lib/ResourceSet.h"

class LobbySetPlayerHandicapCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbySetPlayerHandicap"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbySetPlayerHandicap *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbySetPlayerHandicap>();

		if (json["color"].isNumber())
			pack->color = homamweb::shared::playerColorFromJson(json["color"]);

		const JsonNode & h = json["handicap"];
		Handicap handicap;
		if (h["percentIncome"].isNumber())
			handicap.percentIncome = static_cast<int>(h["percentIncome"].Integer());
		if (h["percentGrowth"].isNumber())
			handicap.percentGrowth = static_cast<int>(h["percentGrowth"].Integer());
		if (h["startBonus"].isVector())
		{
			const auto & vec = h["startBonus"].Vector();
			for (size_t i = 0; i < vec.size(); ++i)
			{
				if (vec[i].isNumber())
					handicap.startBonus[i] = static_cast<TResource>(vec[i].Integer());
			}
		}
		pack->handicap = handicap;

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbySetPlayerHandicap &>(pack);
		out["type"].String() = typeName();
		out["color"] = homamweb::shared::playerColorToJson(p.color);

		JsonNode & h = out["handicap"];
		h.Struct();
		h["percentIncome"].Integer() = static_cast<int64_t>(p.handicap.percentIncome);
		h["percentGrowth"].Integer() = static_cast<int64_t>(p.handicap.percentGrowth);

		JsonNode & startBonus = h["startBonus"];
		startBonus.Vector();
		for (const auto & v : p.handicap.startBonus)
		{
			JsonNode entry;
			entry.Integer() = static_cast<int64_t>(v);
			startBonus.Vector().push_back(entry);
		}
	}
};

REGISTER_PACK_CODEC(LobbySetPlayerHandicapCodec)

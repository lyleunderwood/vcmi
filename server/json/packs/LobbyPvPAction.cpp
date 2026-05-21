/*
 * LobbyPvPAction.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbyPvPAction.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbyPvPAction.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForLobby.h"
#include "../../../lib/constants/EntityIdentifiers.h"

namespace
{
std::string actionToString(LobbyPvPAction::EAction a)
{
	switch (a)
	{
		case LobbyPvPAction::COIN: return "COIN";
		case LobbyPvPAction::RANDOM_TOWN: return "RANDOM_TOWN";
		case LobbyPvPAction::RANDOM_TOWN_VS: return "RANDOM_TOWN_VS";
		case LobbyPvPAction::NONE:
		default: return "NONE";
	}
}

LobbyPvPAction::EAction actionFromString(const std::string & s)
{
	if (s == "COIN") return LobbyPvPAction::COIN;
	if (s == "RANDOM_TOWN") return LobbyPvPAction::RANDOM_TOWN;
	if (s == "RANDOM_TOWN_VS") return LobbyPvPAction::RANDOM_TOWN_VS;
	return LobbyPvPAction::NONE;
}
} // namespace

class LobbyPvPActionCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbyPvPAction"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbyPvPAction *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbyPvPAction>();

		if (json["action"].isString())
			pack->action = actionFromString(json["action"].String());

		if (json["bannedTowns"].isVector())
		{
			for (const auto & entry : json["bannedTowns"].Vector())
			{
				if (entry.isNumber())
					pack->bannedTowns.emplace_back(static_cast<int32_t>(entry.Integer()));
			}
		}

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbyPvPAction &>(pack);

		out["type"].String() = typeName();
		out["action"].String() = actionToString(p.action);

		JsonNode & bannedTowns = out["bannedTowns"];
		bannedTowns.Vector(); // ensure vector type even when empty
		for (const auto & id : p.bannedTowns)
		{
			JsonNode node;
			node.Integer() = static_cast<int64_t>(id.getNum());
			bannedTowns.Vector().push_back(node);
		}
	}
};

REGISTER_PACK_CODEC(LobbyPvPActionCodec)

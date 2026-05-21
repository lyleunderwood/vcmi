/*
 * LobbySetCampaignBonus.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbySetCampaignBonus.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbySetCampaignBonus.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForLobby.h"

class LobbySetCampaignBonusCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbySetCampaignBonus"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbySetCampaignBonus *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbySetCampaignBonus>();

		if (json["bonusId"].isNumber())
			pack->bonusId = static_cast<int>(json["bonusId"].Integer());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbySetCampaignBonus &>(pack);

		out["type"].String() = typeName();
		out["bonusId"].Integer() = static_cast<int64_t>(p.bonusId);
	}
};

REGISTER_PACK_CODEC(LobbySetCampaignBonusCodec)

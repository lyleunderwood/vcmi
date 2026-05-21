/*
 * LobbySetCampaignMap.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbySetCampaignMap.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbySetCampaignMap.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForLobby.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class LobbySetCampaignMapCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbySetCampaignMap"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbySetCampaignMap *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbySetCampaignMap>();

		if (json["mapId"].isNumber())
			pack->mapId = CampaignScenarioID(static_cast<int32_t>(json["mapId"].Integer()));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbySetCampaignMap &>(pack);

		out["type"].String() = typeName();
		out["mapId"].Integer() = static_cast<int64_t>(p.mapId.getNum());
	}
};

REGISTER_PACK_CODEC(LobbySetCampaignMapCodec)

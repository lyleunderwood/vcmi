/*
 * LobbySetCampaign.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbySetCampaign.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbySetCampaign.ts
 *
 * NOTE: `ourCampaign` (std::shared_ptr<CampaignState>) is a complex VCMI
 * engine type whose full field model is not in scope for this codec. It is
 * treated as an opaque JSON blob on the wire: toJson emits a placeholder
 * empty struct when the pointer is non-null (or omits the field when null),
 * and fromJson leaves the pointer as nullptr. A future codec pass should
 * promote CampaignState to a shared codec.
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForLobby.h"

class LobbySetCampaignCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbySetCampaign"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbySetCampaign *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbySetCampaign>();
		// ourCampaign is opaque on the wire; leave as nullptr.
		// (Future work: deserialize from json["ourCampaign"].)
		(void)json;
		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbySetCampaign &>(pack);

		out["type"].String() = typeName();

		if (p.ourCampaign)
			out["ourCampaign"].Struct(); // opaque placeholder
		// else: leave field absent / null
	}
};

REGISTER_PACK_CODEC(LobbySetCampaignCodec)

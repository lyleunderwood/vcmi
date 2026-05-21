/*
 * PlayerBlocked.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for PlayerBlocked (server -> client notification that a
 * player has been blocked from acting, e.g. due to an upcoming battle
 * or another player's ongoing movement).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/PlayerBlocked.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"

namespace
{
std::string reasonToString(PlayerBlocked::EReason r)
{
	switch (r)
	{
		case PlayerBlocked::UPCOMING_BATTLE: return "UPCOMING_BATTLE";
		case PlayerBlocked::ONGOING_MOVEMENT: return "ONGOING_MOVEMENT";
		default: return "UPCOMING_BATTLE";
	}
}

PlayerBlocked::EReason reasonFromString(const std::string & s)
{
	if (s == "ONGOING_MOVEMENT") return PlayerBlocked::ONGOING_MOVEMENT;
	return PlayerBlocked::UPCOMING_BATTLE;
}

std::string modeToString(PlayerBlocked::EMode m)
{
	switch (m)
	{
		case PlayerBlocked::BLOCKADE_STARTED: return "BLOCKADE_STARTED";
		case PlayerBlocked::BLOCKADE_ENDED: return "BLOCKADE_ENDED";
		default: return "BLOCKADE_STARTED";
	}
}

PlayerBlocked::EMode modeFromString(const std::string & s)
{
	if (s == "BLOCKADE_ENDED") return PlayerBlocked::BLOCKADE_ENDED;
	return PlayerBlocked::BLOCKADE_STARTED;
}
} // namespace

class PlayerBlockedCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "PlayerBlocked"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const PlayerBlocked *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<PlayerBlocked>();

		if (json["reason"].isString())
			pack->reason = reasonFromString(json["reason"].String());

		if (json["startOrEnd"].isString())
			pack->startOrEnd = modeFromString(json["startOrEnd"].String());

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const PlayerBlocked &>(pack);

		out["type"].String() = typeName();
		out["reason"].String() = reasonToString(p.reason);
		out["startOrEnd"].String() = modeToString(p.startOrEnd);
		out["player"] = homamweb::shared::playerColorToJson(p.player);
	}
};

REGISTER_PACK_CODEC(PlayerBlockedCodec)

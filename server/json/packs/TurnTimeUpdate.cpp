/*
 * TurnTimeUpdate.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for TurnTimeUpdate (server -> client notification with the
 * latest TurnTimerInfo for a player).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/TurnTimeUpdate.ts
 *
 * NOTE: TurnTimerInfo is not (yet) promoted to a shared codec. Its fields
 * are inlined here, matching the pattern in LobbySetTurnTime.cpp. If a
 * third pack ever references TurnTimerInfo on the wire, promote to
 * vcmi/server/json/shared/TurnTimerInfo.{h,cpp}.
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/TurnTimerInfo.h"

class TurnTimeUpdateCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "TurnTimeUpdate"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const TurnTimeUpdate *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<TurnTimeUpdate>();

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		const JsonNode & t = json["turnTimer"];
		auto & info = pack->turnTimer;
		if (t["turnTimer"].isNumber())
			info.turnTimer = static_cast<int>(t["turnTimer"].Integer());
		if (t["baseTimer"].isNumber())
			info.baseTimer = static_cast<int>(t["baseTimer"].Integer());
		if (t["battleTimer"].isNumber())
			info.battleTimer = static_cast<int>(t["battleTimer"].Integer());
		if (t["unitTimer"].isNumber())
			info.unitTimer = static_cast<int>(t["unitTimer"].Integer());
		if (t["accumulatingTurnTimer"].isBool())
			info.accumulatingTurnTimer = t["accumulatingTurnTimer"].Bool();
		if (t["accumulatingUnitTimer"].isBool())
			info.accumulatingUnitTimer = t["accumulatingUnitTimer"].Bool();
		if (t["isActive"].isBool())
			info.isActive = t["isActive"].Bool();
		if (t["isBattle"].isBool())
			info.isBattle = t["isBattle"].Bool();
		if (t["remainingMovementPointsPercent"].isNumber())
			info.remainingMovementPointsPercent = static_cast<int>(t["remainingMovementPointsPercent"].Integer());
		if (t["isTurnStart"].isBool())
			info.isTurnStart = t["isTurnStart"].Bool();
		if (t["isTurnEnded"].isBool())
			info.isTurnEnded = t["isTurnEnded"].Bool();

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const TurnTimeUpdate &>(pack);
		const auto & info = p.turnTimer;

		out["type"].String() = typeName();
		out["player"] = homamweb::shared::playerColorToJson(p.player);

		JsonNode & t = out["turnTimer"];
		t.Struct();
		t["turnTimer"].Integer() = static_cast<int64_t>(info.turnTimer);
		t["baseTimer"].Integer() = static_cast<int64_t>(info.baseTimer);
		t["battleTimer"].Integer() = static_cast<int64_t>(info.battleTimer);
		t["unitTimer"].Integer() = static_cast<int64_t>(info.unitTimer);
		t["accumulatingTurnTimer"].Bool() = info.accumulatingTurnTimer;
		t["accumulatingUnitTimer"].Bool() = info.accumulatingUnitTimer;
		t["isActive"].Bool() = info.isActive;
		t["isBattle"].Bool() = info.isBattle;
		t["remainingMovementPointsPercent"].Integer() = static_cast<int64_t>(info.remainingMovementPointsPercent);
		t["isTurnStart"].Bool() = info.isTurnStart;
		t["isTurnEnded"].Bool() = info.isTurnEnded;
	}
};

REGISTER_PACK_CODEC(TurnTimeUpdateCodec)

/*
 * LobbyUpdateState.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbyUpdateState. The pack carries a LobbyState struct
 * which is itself composite:
 *   - si: shared_ptr<StartInfo>   → opaque (see shared/StartInfo.h)
 *   - mi: shared_ptr<CMapInfo>    → opaque (matches LobbySetMap precedent)
 *   - playerNames: map<PlayerConnectionID, ClientPlayer> → structured
 *   - hostClientId: GameConnectionID → int
 *   - campaignMap: CampaignScenarioID → int
 *   - campaignBonus: int
 *
 * Wire serializes both `state` and `refreshList` from the pack's own
 * serialize() template. The hostChanged field is "client-side only" and not
 * serialized.
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/StartInfo.h"

#include "../../../lib/networkPacks/PacksForLobby.h"
#include "../../../lib/StartInfo.h"

namespace
{
JsonNode lobbyStateToJson(const LobbyState & st)
{
	JsonNode out;
	out.Struct();

	if (auto si = homamweb::shared::opaqueStartInfoToJson(st.si))
		out["si"] = *si;
	// mi (CMapInfo) is opaque too — emit empty placeholder if non-null.
	if (st.mi)
		out["mi"].Struct();

	JsonNode & names = out["playerNames"];
	names.Vector(); // array of { connectionId, name, playerId }
	for (const auto & entry : st.playerNames)
	{
		JsonNode item;
		item.Struct();
		item["playerId"].Integer() = static_cast<int64_t>(entry.first);
		item["connection"].Integer() = static_cast<int64_t>(entry.second.connection);
		item["name"].String() = entry.second.name;
		names.Vector().push_back(item);
	}

	out["hostClientId"].Integer() = static_cast<int64_t>(st.hostClientId);
	out["campaignMap"].Integer() = static_cast<int64_t>(st.campaignMap);
	out["campaignBonus"].Integer() = static_cast<int64_t>(st.campaignBonus);

	return out;
}

LobbyState lobbyStateFromJson(const JsonNode & json)
{
	LobbyState st;
	st.si = homamweb::shared::opaqueStartInfoFromJson(json["si"]);
	// mi stays nullptr (opaque inbound)
	st.mi = nullptr;

	if (json["playerNames"].isVector())
	{
		for (const auto & item : json["playerNames"].Vector())
		{
			ClientPlayer cp;
			cp.connection = GameConnectionID(static_cast<int32_t>(item["connection"].Integer()));
			cp.name = item["name"].String();
			PlayerConnectionID pid = PlayerConnectionID(static_cast<int32_t>(item["playerId"].Integer()));
			st.playerNames.emplace(pid, cp);
		}
	}

	if (json["hostClientId"].isNumber())
		st.hostClientId = GameConnectionID(static_cast<int32_t>(json["hostClientId"].Integer()));
	if (json["campaignMap"].isNumber())
		st.campaignMap = CampaignScenarioID(static_cast<int32_t>(json["campaignMap"].Integer()));
	if (json["campaignBonus"].isNumber())
		st.campaignBonus = static_cast<int>(json["campaignBonus"].Integer());

	return st;
}
} // namespace

class LobbyUpdateStateCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbyUpdateState"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbyUpdateState *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbyUpdateState>();
		if (json["state"].isStruct())
			pack->state = lobbyStateFromJson(json["state"]);
		if (json["refreshList"].isBool())
			pack->refreshList = json["refreshList"].Bool();
		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbyUpdateState &>(pack);
		out["type"].String() = typeName();
		out["state"] = lobbyStateToJson(p.state);
		out["refreshList"].Bool() = p.refreshList;
	}
};

REGISTER_PACK_CODEC(LobbyUpdateStateCodec)

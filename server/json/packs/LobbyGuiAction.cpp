/*
 * LobbyGuiAction.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbyGuiAction.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbyGuiAction.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForLobby.h"

namespace
{
std::string actionToString(ui8 a)
{
	switch (a)
	{
		case LobbyGuiAction::NO_TAB: return "NO_TAB";
		case LobbyGuiAction::OPEN_OPTIONS: return "OPEN_OPTIONS";
		case LobbyGuiAction::OPEN_SCENARIO_LIST: return "OPEN_SCENARIO_LIST";
		case LobbyGuiAction::OPEN_RANDOM_MAP_OPTIONS: return "OPEN_RANDOM_MAP_OPTIONS";
		case LobbyGuiAction::OPEN_TURN_OPTIONS: return "OPEN_TURN_OPTIONS";
		case LobbyGuiAction::OPEN_EXTRA_OPTIONS: return "OPEN_EXTRA_OPTIONS";
		case LobbyGuiAction::BATTLE_MODE: return "BATTLE_MODE";
		case LobbyGuiAction::NONE:
		default: return "NONE";
	}
}

ui8 actionFromString(const std::string & s)
{
	if (s == "NO_TAB") return LobbyGuiAction::NO_TAB;
	if (s == "OPEN_OPTIONS") return LobbyGuiAction::OPEN_OPTIONS;
	if (s == "OPEN_SCENARIO_LIST") return LobbyGuiAction::OPEN_SCENARIO_LIST;
	if (s == "OPEN_RANDOM_MAP_OPTIONS") return LobbyGuiAction::OPEN_RANDOM_MAP_OPTIONS;
	if (s == "OPEN_TURN_OPTIONS") return LobbyGuiAction::OPEN_TURN_OPTIONS;
	if (s == "OPEN_EXTRA_OPTIONS") return LobbyGuiAction::OPEN_EXTRA_OPTIONS;
	if (s == "BATTLE_MODE") return LobbyGuiAction::BATTLE_MODE;
	return LobbyGuiAction::NONE;
}
} // namespace

class LobbyGuiActionCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbyGuiAction"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbyGuiAction *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbyGuiAction>();

		if (json["action"].isString())
			pack->action = static_cast<LobbyGuiAction::EAction>(actionFromString(json["action"].String()));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbyGuiAction &>(pack);

		out["type"].String() = typeName();
		out["action"].String() = actionToString(static_cast<ui8>(p.action));
	}
};

REGISTER_PACK_CODEC(LobbyGuiActionCodec)

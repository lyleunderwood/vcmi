/*
 * LobbyChangePlayerOption.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbyChangePlayerOption.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbyChangePlayerOption.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForLobby.h"

namespace
{
std::string whatToString(ui8 w)
{
	switch (w)
	{
		case LobbyChangePlayerOption::TOWN: return "TOWN";
		case LobbyChangePlayerOption::HERO: return "HERO";
		case LobbyChangePlayerOption::BONUS: return "BONUS";
		case LobbyChangePlayerOption::TOWN_ID: return "TOWN_ID";
		case LobbyChangePlayerOption::HERO_ID: return "HERO_ID";
		case LobbyChangePlayerOption::BONUS_ID: return "BONUS_ID";
		case LobbyChangePlayerOption::UNKNOWN:
		default: return "UNKNOWN";
	}
}

ui8 whatFromString(const std::string & s)
{
	if (s == "TOWN") return LobbyChangePlayerOption::TOWN;
	if (s == "HERO") return LobbyChangePlayerOption::HERO;
	if (s == "BONUS") return LobbyChangePlayerOption::BONUS;
	if (s == "TOWN_ID") return LobbyChangePlayerOption::TOWN_ID;
	if (s == "HERO_ID") return LobbyChangePlayerOption::HERO_ID;
	if (s == "BONUS_ID") return LobbyChangePlayerOption::BONUS_ID;
	return LobbyChangePlayerOption::UNKNOWN;
}
} // namespace

class LobbyChangePlayerOptionCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbyChangePlayerOption"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbyChangePlayerOption *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbyChangePlayerOption>();

		if (json["what"].isString())
			pack->what = whatFromString(json["what"].String());

		if (json["value"].isNumber())
			pack->value = static_cast<int32_t>(json["value"].Integer());

		if (json["color"].isNumber())
			pack->color = homamweb::shared::playerColorFromJson(json["color"]);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbyChangePlayerOption &>(pack);

		out["type"].String() = typeName();
		out["what"].String() = whatToString(p.what);
		out["value"].Integer() = static_cast<int64_t>(p.value);
		out["color"] = homamweb::shared::playerColorToJson(p.color);
	}
};

REGISTER_PACK_CODEC(LobbyChangePlayerOptionCodec)

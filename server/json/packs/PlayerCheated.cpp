/*
 * PlayerCheated.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for PlayerCheated (server -> client notification that a
 * player entered a cheat code).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/PlayerCheated.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/Enumerations.h"

namespace
{

std::string colorSchemeToString(ColorScheme s)
{
	switch (s)
	{
		case ColorScheme::NONE:      return "NONE";
		case ColorScheme::GRAYSCALE: return "GRAYSCALE";
		case ColorScheme::H2_SCHEME: return "H2_SCHEME";
		case ColorScheme::KEEP:
		default:                     return "KEEP";
	}
}

ColorScheme colorSchemeFromString(const std::string & s)
{
	if (s == "NONE")      return ColorScheme::NONE;
	if (s == "GRAYSCALE") return ColorScheme::GRAYSCALE;
	if (s == "H2_SCHEME") return ColorScheme::H2_SCHEME;
	return ColorScheme::KEEP;
}

} // namespace

class PlayerCheatedCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "PlayerCheated"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const PlayerCheated *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<PlayerCheated>();

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		if (json["localOnlyCheat"].isBool())
			pack->localOnlyCheat = json["localOnlyCheat"].Bool();

		if (json["losingCheatCode"].isBool())
			pack->losingCheatCode = json["losingCheatCode"].Bool();

		if (json["winningCheatCode"].isBool())
			pack->winningCheatCode = json["winningCheatCode"].Bool();

		if (json["colorScheme"].isString())
			pack->colorScheme = colorSchemeFromString(json["colorScheme"].String());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const PlayerCheated &>(pack);

		out["type"].String() = typeName();
		out["player"] = homamweb::shared::playerColorToJson(p.player);
		out["localOnlyCheat"].Bool() = p.localOnlyCheat;
		out["losingCheatCode"].Bool() = p.losingCheatCode;
		out["winningCheatCode"].Bool() = p.winningCheatCode;
		out["colorScheme"].String() = colorSchemeToString(p.colorScheme);
	}
};

REGISTER_PACK_CODEC(PlayerCheatedCodec)

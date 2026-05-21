/*
 * FoWChange.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for FoWChange (server -> client fog-of-war state update).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/FoWChange.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/Int3.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/Enumerations.h"

namespace
{
std::string tileVisibilityToString(ETileVisibility m)
{
	switch (m)
	{
		case ETileVisibility::HIDDEN:   return "HIDDEN";
		case ETileVisibility::REVEALED: return "REVEALED";
		default: return "HIDDEN";
	}
}

ETileVisibility tileVisibilityFromString(const std::string & s)
{
	if (s == "REVEALED") return ETileVisibility::REVEALED;
	return ETileVisibility::HIDDEN;
}
} // namespace

class FoWChangeCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "FoWChange"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const FoWChange *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<FoWChange>();

		if (json["tiles"].isVector())
		{
			for (const auto & tile : json["tiles"].Vector())
				pack->tiles.insert(homamweb::shared::int3FromJson(tile));
		}

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		if (json["mode"].isString())
			pack->mode = tileVisibilityFromString(json["mode"].String());

		if (json["waitForDialogs"].isBool())
			pack->waitForDialogs = json["waitForDialogs"].Bool();

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const FoWChange &>(pack);

		out["type"].String() = typeName();

		JsonNode & tiles = out["tiles"];
		tiles.Vector(); // ensure vector type even when empty
		for (const auto & tile : p.tiles)
			tiles.Vector().push_back(homamweb::shared::int3ToJson(tile));

		out["player"] = homamweb::shared::playerColorToJson(p.player);
		out["mode"].String() = tileVisibilityToString(p.mode);
		out["waitForDialogs"].Bool() = p.waitForDialogs;
	}
};

REGISTER_PACK_CODEC(FoWChangeCodec)

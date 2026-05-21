/*
 * GiveHero.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for GiveHero (a server -> client update emitted when a hero
 * object is granted to a player, e.g. on map start or via Prison/Map event).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/GiveHero.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class GiveHeroCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "GiveHero"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const GiveHero *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<GiveHero>();

		if (json["id"].isNumber())
			pack->id = ObjectInstanceID(static_cast<int32_t>(json["id"].Integer()));

		if (json["boatId"].isNumber())
			pack->boatId = ObjectInstanceID(static_cast<int32_t>(json["boatId"].Integer()));

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const GiveHero &>(pack);

		out["type"].String() = typeName();
		out["id"].Integer() = static_cast<int64_t>(p.id.getNum());
		out["boatId"].Integer() = static_cast<int64_t>(p.boatId.getNum());
		out["player"] = homamweb::shared::playerColorToJson(p.player);
	}
};

REGISTER_PACK_CODEC(GiveHeroCodec)

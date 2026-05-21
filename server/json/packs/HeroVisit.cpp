/*
 * HeroVisit.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for HeroVisit (server -> client state update: a hero
 * is starting or ending a visit to a map object).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/HeroVisit.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class HeroVisitCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "HeroVisit"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const HeroVisit *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<HeroVisit>();

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		if (json["heroId"].isNumber())
			pack->heroId = ObjectInstanceID(static_cast<int32_t>(json["heroId"].Integer()));

		if (json["objId"].isNumber())
			pack->objId = ObjectInstanceID(static_cast<int32_t>(json["objId"].Integer()));

		if (json["starting"].isBool())
			pack->starting = json["starting"].Bool();

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const HeroVisit &>(pack);

		out["type"].String() = typeName();
		out["player"] = homamweb::shared::playerColorToJson(p.player);
		out["heroId"].Integer() = static_cast<int64_t>(p.heroId.getNum());
		out["objId"].Integer() = static_cast<int64_t>(p.objId.getNum());
		out["starting"].Bool() = p.starting;
	}
};

REGISTER_PACK_CODEC(HeroVisitCodec)

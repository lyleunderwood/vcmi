/*
 * HireHero.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for HireHero (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/HireHero.ts
 *
 * Note: HireHero declares its own `player` field (PlayerColor) which
 * shadows `CPackForServer::player`. Both are independently serialized
 * by the binary template. To round-trip cleanly through JSON we expose
 * the base-class player under the inherited "player" key and the
 * HireHero-specific player under "hirePlayer".
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class HireHeroCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "HireHero"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const HireHero *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<HireHero>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["hid"].isNumber())
			pack->hid = HeroTypeID(static_cast<int32_t>(json["hid"].Integer()));

		if (json["nhid"].isNumber())
			pack->nhid = HeroTypeID(static_cast<int32_t>(json["nhid"].Integer()));

		if (json["tid"].isNumber())
			pack->tid = ObjectInstanceID(static_cast<int32_t>(json["tid"].Integer()));

		if (json["hirePlayer"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["hirePlayer"]);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const HireHero &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["hid"].Integer() = static_cast<int64_t>(p.hid.getNum());
		out["nhid"].Integer() = static_cast<int64_t>(p.nhid.getNum());
		out["tid"].Integer() = static_cast<int64_t>(p.tid.getNum());
		out["hirePlayer"] = homamweb::shared::playerColorToJson(p.player);
	}
};

REGISTER_PACK_CODEC(HireHeroCodec)

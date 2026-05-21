/*
 * HeroVisitCastle.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for HeroVisitCastle (server -> client state update: a hero
 * is entering or leaving a town/castle).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/HeroVisitCastle.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class HeroVisitCastleCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "HeroVisitCastle"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const HeroVisitCastle *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<HeroVisitCastle>();

		if (json["startVisit"].isBool())
			pack->startVisit = json["startVisit"].Bool();

		if (json["tid"].isNumber())
			pack->tid = ObjectInstanceID(static_cast<int32_t>(json["tid"].Integer()));

		if (json["hid"].isNumber())
			pack->hid = ObjectInstanceID(static_cast<int32_t>(json["hid"].Integer()));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const HeroVisitCastle &>(pack);

		out["type"].String() = typeName();
		out["startVisit"].Bool() = p.startVisit;
		out["tid"].Integer() = static_cast<int64_t>(p.tid.getNum());
		out["hid"].Integer() = static_cast<int64_t>(p.hid.getNum());
	}
};

REGISTER_PACK_CODEC(HeroVisitCastleCodec)

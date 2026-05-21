/*
 * CastleTeleportHero.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for CastleTeleportHero (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/CastleTeleportHero.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class CastleTeleportHeroCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "CastleTeleportHero"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const CastleTeleportHero *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<CastleTeleportHero>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["dest"].isNumber())
			pack->dest = ObjectInstanceID(static_cast<int32_t>(json["dest"].Integer()));

		if (json["hid"].isNumber())
			pack->hid = ObjectInstanceID(static_cast<int32_t>(json["hid"].Integer()));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const CastleTeleportHero &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["dest"].Integer() = static_cast<int64_t>(p.dest.getNum());
		out["hid"].Integer() = static_cast<int64_t>(p.hid.getNum());
	}
};

REGISTER_PACK_CODEC(CastleTeleportHeroCodec)

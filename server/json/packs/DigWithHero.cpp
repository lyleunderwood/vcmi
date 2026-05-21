/*
 * DigWithHero.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for DigWithHero (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/DigWithHero.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class DigWithHeroCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "DigWithHero"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const DigWithHero *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<DigWithHero>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["id"].isNumber())
			pack->id = ObjectInstanceID(static_cast<int32_t>(json["id"].Integer()));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const DigWithHero &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["id"].Integer() = static_cast<int64_t>(p.id.getNum());
	}
};

REGISTER_PACK_CODEC(DigWithHeroCodec)

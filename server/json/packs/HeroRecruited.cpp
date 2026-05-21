/*
 * HeroRecruited.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for HeroRecruited (a server -> client update emitted when
 * a hero is recruited from a town tavern).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/HeroRecruited.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/Int3.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class HeroRecruitedCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "HeroRecruited"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const HeroRecruited *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<HeroRecruited>();

		if (json["hid"].isNumber())
			pack->hid = HeroTypeID(static_cast<int32_t>(json["hid"].Integer()));

		if (json["tid"].isNumber())
			pack->tid = ObjectInstanceID(static_cast<int32_t>(json["tid"].Integer()));

		if (json["boatId"].isNumber())
			pack->boatId = ObjectInstanceID(static_cast<int32_t>(json["boatId"].Integer()));

		if (json["tile"].isStruct())
			pack->tile = homamweb::shared::int3FromJson(json["tile"]);

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const HeroRecruited &>(pack);

		out["type"].String() = typeName();
		out["hid"].Integer() = static_cast<int64_t>(p.hid.getNum());
		out["tid"].Integer() = static_cast<int64_t>(p.tid.getNum());
		out["boatId"].Integer() = static_cast<int64_t>(p.boatId.getNum());
		out["tile"] = homamweb::shared::int3ToJson(p.tile);
		out["player"] = homamweb::shared::playerColorToJson(p.player);
	}
};

REGISTER_PACK_CODEC(HeroRecruitedCodec)

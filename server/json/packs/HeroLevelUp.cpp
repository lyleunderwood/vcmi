/*
 * HeroLevelUp.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for HeroLevelUp (server -> client query: a hero gained a level,
 * the player must pick a secondary skill from the offered set).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/HeroLevelUp.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class HeroLevelUpCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "HeroLevelUp"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const HeroLevelUp *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<HeroLevelUp>();

		// Inherited from Query.
		if (json["queryID"].isNumber())
			pack->queryID = QueryID(static_cast<int32_t>(json["queryID"].Integer()));

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		if (json["heroId"].isNumber())
			pack->heroId = ObjectInstanceID(static_cast<int32_t>(json["heroId"].Integer()));

		if (json["primskill"].isNumber())
			pack->primskill = PrimarySkill(static_cast<int32_t>(json["primskill"].Integer()));

		if (json["skills"].isVector())
		{
			for (const auto & s : json["skills"].Vector())
				pack->skills.push_back(SecondarySkill(static_cast<int32_t>(s.Integer())));
		}

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const HeroLevelUp &>(pack);

		out["type"].String() = typeName();

		// Inherited from Query.
		out["queryID"].Integer() = static_cast<int64_t>(p.queryID.getNum());

		out["player"] = homamweb::shared::playerColorToJson(p.player);
		out["heroId"].Integer() = static_cast<int64_t>(p.heroId.getNum());
		out["primskill"].Integer() = static_cast<int64_t>(p.primskill.getNum());

		JsonNode & skills = out["skills"];
		skills.Vector();
		for (const auto & s : p.skills)
		{
			JsonNode entry;
			entry.Integer() = static_cast<int64_t>(s.getNum());
			skills.Vector().push_back(entry);
		}
	}
};

REGISTER_PACK_CODEC(HeroLevelUpCodec)

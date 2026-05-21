/*
 * CommanderLevelUp.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for CommanderLevelUp (server -> client query offering a
 * commander a choice between secondary/special skills on level up).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/CommanderLevelUp.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class CommanderLevelUpCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "CommanderLevelUp"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const CommanderLevelUp *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<CommanderLevelUp>();

		// Inherited from Query.
		if (json["queryID"].isNumber())
			pack->queryID = QueryID(static_cast<int32_t>(json["queryID"].Integer()));

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		if (json["heroId"].isNumber())
			pack->heroId = ObjectInstanceID(static_cast<int32_t>(json["heroId"].Integer()));

		if (json["skills"].isVector())
		{
			for (const auto & s : json["skills"].Vector())
				pack->skills.push_back(static_cast<ui32>(s.Integer()));
		}

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const CommanderLevelUp &>(pack);

		out["type"].String() = typeName();

		// Inherited from Query.
		out["queryID"].Integer() = static_cast<int64_t>(p.queryID.getNum());

		out["player"] = homamweb::shared::playerColorToJson(p.player);
		out["heroId"].Integer() = static_cast<int64_t>(p.heroId.getNum());

		JsonNode & skills = out["skills"];
		skills.Vector();
		for (const auto & s : p.skills)
		{
			JsonNode entry;
			entry.Integer() = static_cast<int64_t>(s);
			skills.Vector().push_back(entry);
		}
	}
};

REGISTER_PACK_CODEC(CommanderLevelUpCodec)

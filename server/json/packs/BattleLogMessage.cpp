/*
 * BattleLogMessage.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BattleLogMessage (server -> client battle log: a list of
 * localized message lines to append to the battle log).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClientBattle.h
 * TypeScript twin:      wrapper/src/codecs/battle/BattleLogMessage.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/MetaString.h"

#include "../../../lib/networkPacks/PacksForClientBattle.h"
#include "../../../lib/constants/EntityIdentifiers.h"
#include "../../../lib/texts/MetaString.h"

class BattleLogMessageCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BattleLogMessage"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BattleLogMessage *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BattleLogMessage>();

		if (json["battleID"].isNumber())
			pack->battleID = BattleID(static_cast<int32_t>(json["battleID"].Integer()));

		if (json["lines"].isVector())
		{
			for (const auto & line : json["lines"].Vector())
				pack->lines.push_back(homamweb::shared::metaStringFromJson(line));
		}

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BattleLogMessage &>(pack);

		out["type"].String() = typeName();
		out["battleID"].Integer() = static_cast<int64_t>(p.battleID.getNum());

		JsonNode & lines = out["lines"];
		lines.Vector(); // ensure array
		for (const auto & line : p.lines)
			lines.Vector().push_back(homamweb::shared::metaStringToJson(line));
	}
};

REGISTER_PACK_CODEC(BattleLogMessageCodec)

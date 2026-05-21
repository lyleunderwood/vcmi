/*
 * BattleStackMoved.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BattleStackMoved (server -> client battle state update:
 * a stack has moved along a sequence of battlefield tiles, optionally
 * teleporting).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClientBattle.h
 * TypeScript twin:      wrapper/src/codecs/battle/BattleStackMoved.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClientBattle.h"
#include "../../../lib/constants/EntityIdentifiers.h"
#include "../../../lib/battle/BattleHex.h"
#include "../../../lib/battle/BattleHexArray.h"

class BattleStackMovedCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BattleStackMoved"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BattleStackMoved *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BattleStackMoved>();

		if (json["battleID"].isNumber())
			pack->battleID = BattleID(static_cast<int32_t>(json["battleID"].Integer()));

		if (json["stack"].isNumber())
			pack->stack = static_cast<ui32>(json["stack"].Integer());

		if (json["tilesToMove"].isVector())
		{
			for (const auto & h : json["tilesToMove"].Vector())
				pack->tilesToMove.insert(BattleHex(static_cast<si16>(h.Integer())));
		}

		if (json["distance"].isNumber())
			pack->distance = static_cast<int>(json["distance"].Integer());

		if (json["teleporting"].isBool())
			pack->teleporting = json["teleporting"].Bool();

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BattleStackMoved &>(pack);

		out["type"].String() = typeName();
		out["battleID"].Integer() = static_cast<int64_t>(p.battleID.getNum());
		out["stack"].Integer() = static_cast<int64_t>(p.stack);

		JsonNode & tiles = out["tilesToMove"];
		tiles.Vector();
		for (const auto & h : p.tilesToMove)
		{
			JsonNode entry;
			entry.Integer() = static_cast<int64_t>(h.toInt());
			tiles.Vector().push_back(entry);
		}

		out["distance"].Integer() = static_cast<int64_t>(p.distance);
		out["teleporting"].Bool() = p.teleporting;
	}
};

REGISTER_PACK_CODEC(BattleStackMovedCodec)

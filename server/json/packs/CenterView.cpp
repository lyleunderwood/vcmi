/*
 * CenterView.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for CenterView (a server -> client update requesting that the
 * client camera centre on a specific tile for a given player).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/CenterView.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/Int3.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"

class CenterViewCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "CenterView"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const CenterView *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<CenterView>();

		if (json["pos"].isStruct())
			pack->pos = homamweb::shared::int3FromJson(json["pos"]);

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		if (json["focusTime"].isNumber())
			pack->focusTime = static_cast<ui32>(json["focusTime"].Integer());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const CenterView &>(pack);

		out["type"].String() = typeName();
		out["pos"] = homamweb::shared::int3ToJson(p.pos);
		out["player"] = homamweb::shared::playerColorToJson(p.player);
		out["focusTime"].Integer() = static_cast<int64_t>(p.focusTime);
	}
};

REGISTER_PACK_CODEC(CenterViewCodec)

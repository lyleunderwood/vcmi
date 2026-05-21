/*
 * DaysWithoutTown.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for DaysWithoutTown (server -> client notification tracking how
 * many days a player has been without a town).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/DaysWithoutTown.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"

class DaysWithoutTownCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "DaysWithoutTown"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const DaysWithoutTown *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<DaysWithoutTown>();

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		if (json["daysWithoutCastle"].isNumber())
			pack->daysWithoutCastle = static_cast<int32_t>(json["daysWithoutCastle"].Integer());
		else
			pack->daysWithoutCastle = std::nullopt;

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const DaysWithoutTown &>(pack);

		out["type"].String() = typeName();
		out["player"] = homamweb::shared::playerColorToJson(p.player);

		if (p.daysWithoutCastle.has_value())
			out["daysWithoutCastle"].Integer() = static_cast<int64_t>(*p.daysWithoutCastle);
	}
};

REGISTER_PACK_CODEC(DaysWithoutTownCodec)

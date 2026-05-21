/*
 * ResponseStatistic.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for ResponseStatistic (server -> client reply carrying a
 * statistic snapshot for the requesting player).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/ResponseStatistic.ts
 *
 * NOTE: the `statistic` field (StatisticDataSet) is a heavyweight VCMI
 * gameState type with ~40 fields per entry plus a per-player accumulator
 * map. It is treated as an opaque blob on the wire: toJson emits a
 * placeholder empty struct, fromJson leaves it default-constructed.
 * Promote to a shared codec if/when the web client needs the stats.
 * Status: partial. See PlayerEndsGame.cpp for the same precedent.
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class ResponseStatisticCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "ResponseStatistic"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const ResponseStatistic *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<ResponseStatistic>();

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		// statistic is opaque on the wire; leave default-constructed.
		// (Future work: promote StatisticDataSet to a shared codec.)

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const ResponseStatistic &>(pack);

		out["type"].String() = typeName();
		out["player"] = homamweb::shared::playerColorToJson(p.player);
		out["statistic"].Struct(); // opaque placeholder; see header comment
	}
};

REGISTER_PACK_CODEC(ResponseStatisticCodec)

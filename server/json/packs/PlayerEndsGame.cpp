/*
 * PlayerEndsGame.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for PlayerEndsGame (server -> client notification that a
 * player has won or lost the game).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/PlayerEndsGame.ts
 *
 * NOTE: the `statistic` field (StatisticDataSet) is a heavyweight VCMI
 * gameState type with ~40 fields per entry plus a per-player accumulator
 * map. It is treated as an opaque blob on the wire: toJson emits a
 * placeholder empty struct, fromJson leaves it default-constructed.
 * Promote to a shared codec if/when the web client needs the stats.
 *
 * The nested EVictoryLossCheckResult is inlined here (intValue +
 * messageToSelf/messageToOthers). Its `intValue` is private; on inbound
 * we reconstruct via the public static factories victory()/defeat() for
 * +1/-1 and the default constructor for 0 (INGAME).
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/MetaString.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/gameState/EVictoryLossCheckResult.h"
#include "../../../lib/gameState/GameStatistics.h"
#include "../../../lib/texts/MetaString.h"
#include "../../../lib/constants/EntityIdentifiers.h"

namespace {

JsonNode victoryLossResultToJson(const EVictoryLossCheckResult & r)
{
	JsonNode out;
	out.Struct();
	int64_t intValue = 0;
	if (r.victory()) intValue = 1;
	else if (r.loss()) intValue = -1;
	out["intValue"].Integer() = intValue;
	out["messageToSelf"] = homamweb::shared::metaStringToJson(r.messageToSelf);
	out["messageToOthers"] = homamweb::shared::metaStringToJson(r.messageToOthers);
	return out;
}

EVictoryLossCheckResult victoryLossResultFromJson(const JsonNode & json)
{
	int64_t intValue = 0;
	if (json["intValue"].isNumber())
		intValue = json["intValue"].Integer();

	MetaString toSelf;
	MetaString toOthers;
	if (json["messageToSelf"].isString())
		toSelf = homamweb::shared::metaStringFromJson(json["messageToSelf"]);
	if (json["messageToOthers"].isString())
		toOthers = homamweb::shared::metaStringFromJson(json["messageToOthers"]);

	if (intValue > 0)
		return EVictoryLossCheckResult::victory(toSelf, toOthers);
	if (intValue < 0)
		return EVictoryLossCheckResult::defeat(toSelf, toOthers);
	return EVictoryLossCheckResult();
}

} // namespace

class PlayerEndsGameCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "PlayerEndsGame"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const PlayerEndsGame *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<PlayerEndsGame>();

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		if (json["victoryLossCheckResult"].isStruct())
			pack->victoryLossCheckResult = victoryLossResultFromJson(json["victoryLossCheckResult"]);

		// statistic is opaque on the wire; leave default-constructed.
		// (Future work: promote StatisticDataSet to a shared codec.)

		if (json["silentEnd"].isBool())
			pack->silentEnd = json["silentEnd"].Bool();

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const PlayerEndsGame &>(pack);

		out["type"].String() = typeName();
		out["player"] = homamweb::shared::playerColorToJson(p.player);
		out["victoryLossCheckResult"] = victoryLossResultToJson(p.victoryLossCheckResult);
		out["statistic"].Struct(); // opaque placeholder; see header comment
		out["silentEnd"].Bool() = p.silentEnd;
	}
};

REGISTER_PACK_CODEC(PlayerEndsGameCodec)

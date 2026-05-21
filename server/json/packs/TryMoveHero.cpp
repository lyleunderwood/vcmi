/*
 * TryMoveHero.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for TryMoveHero (a server -> client movement-result update).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/TryMoveHero.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/Int3.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

namespace
{
std::string resultToString(TryMoveHero::EResult r)
{
	switch (r)
	{
		case TryMoveHero::FAILED:         return "FAILED";
		case TryMoveHero::SUCCESS:        return "SUCCESS";
		case TryMoveHero::TELEPORTATION:  return "TELEPORTATION";
		case TryMoveHero::BLOCKING_VISIT: return "BLOCKING_VISIT";
		case TryMoveHero::EMBARK:         return "EMBARK";
		case TryMoveHero::DISEMBARK:      return "DISEMBARK";
		default:                          return "FAILED";
	}
}

TryMoveHero::EResult resultFromString(const std::string & s)
{
	if (s == "FAILED")         return TryMoveHero::FAILED;
	if (s == "SUCCESS")        return TryMoveHero::SUCCESS;
	if (s == "TELEPORTATION")  return TryMoveHero::TELEPORTATION;
	if (s == "BLOCKING_VISIT") return TryMoveHero::BLOCKING_VISIT;
	if (s == "EMBARK")         return TryMoveHero::EMBARK;
	if (s == "DISEMBARK")      return TryMoveHero::DISEMBARK;
	return TryMoveHero::FAILED;
}
} // namespace

class TryMoveHeroCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "TryMoveHero"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const TryMoveHero *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<TryMoveHero>();

		if (json["id"].isNumber())
			pack->id = ObjectInstanceID(static_cast<int32_t>(json["id"].Integer()));

		if (json["movePoints"].isNumber())
			pack->movePoints = static_cast<ui32>(json["movePoints"].Integer());

		if (json["result"].isString())
			pack->result = resultFromString(json["result"].String());

		if (json["start"].isStruct())
			pack->start = homamweb::shared::int3FromJson(json["start"]);

		if (json["end"].isStruct())
			pack->end = homamweb::shared::int3FromJson(json["end"]);

		if (json["fowRevealed"].isVector())
		{
			for (const auto & tile : json["fowRevealed"].Vector())
				pack->fowRevealed.insert(homamweb::shared::int3FromJson(tile));
		}

		if (json["attackedFrom"].isStruct())
			pack->attackedFrom = homamweb::shared::int3FromJson(json["attackedFrom"]);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const TryMoveHero &>(pack);

		out["type"].String() = typeName();
		out["id"].Integer() = static_cast<int64_t>(p.id.getNum());
		out["movePoints"].Integer() = static_cast<int64_t>(p.movePoints);
		out["result"].String() = resultToString(p.result);
		out["start"] = homamweb::shared::int3ToJson(p.start);
		out["end"] = homamweb::shared::int3ToJson(p.end);

		JsonNode & fow = out["fowRevealed"];
		fow.Vector();
		for (const auto & tile : p.fowRevealed)
			fow.Vector().push_back(homamweb::shared::int3ToJson(tile));

		out["attackedFrom"] = homamweb::shared::int3ToJson(p.attackedFrom);
	}
};

REGISTER_PACK_CODEC(TryMoveHeroCodec)

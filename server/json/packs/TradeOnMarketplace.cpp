/*
 * TradeOnMarketplace.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for TradeOnMarketplace (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/TradeOnMarketplace.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/networkPacks/TradeItem.h"
#include "../../../lib/constants/EntityIdentifiers.h"
#include "../../../lib/constants/Enumerations.h"

namespace
{
std::string marketModeToString(EMarketMode m)
{
	switch (m)
	{
		case EMarketMode::RESOURCE_RESOURCE: return "RESOURCE_RESOURCE";
		case EMarketMode::RESOURCE_PLAYER:   return "RESOURCE_PLAYER";
		case EMarketMode::CREATURE_RESOURCE: return "CREATURE_RESOURCE";
		case EMarketMode::RESOURCE_ARTIFACT: return "RESOURCE_ARTIFACT";
		case EMarketMode::ARTIFACT_RESOURCE: return "ARTIFACT_RESOURCE";
		case EMarketMode::ARTIFACT_EXP:      return "ARTIFACT_EXP";
		case EMarketMode::CREATURE_EXP:      return "CREATURE_EXP";
		case EMarketMode::CREATURE_UNDEAD:   return "CREATURE_UNDEAD";
		case EMarketMode::RESOURCE_SKILL:    return "RESOURCE_SKILL";
		case EMarketMode::MARKET_AFTER_LAST_PLACEHOLDER: return "MARKET_AFTER_LAST_PLACEHOLDER";
		default: return "RESOURCE_RESOURCE";
	}
}

EMarketMode marketModeFromString(const std::string & s)
{
	if (s == "RESOURCE_RESOURCE") return EMarketMode::RESOURCE_RESOURCE;
	if (s == "RESOURCE_PLAYER")   return EMarketMode::RESOURCE_PLAYER;
	if (s == "CREATURE_RESOURCE") return EMarketMode::CREATURE_RESOURCE;
	if (s == "RESOURCE_ARTIFACT") return EMarketMode::RESOURCE_ARTIFACT;
	if (s == "ARTIFACT_RESOURCE") return EMarketMode::ARTIFACT_RESOURCE;
	if (s == "ARTIFACT_EXP")      return EMarketMode::ARTIFACT_EXP;
	if (s == "CREATURE_EXP")      return EMarketMode::CREATURE_EXP;
	if (s == "CREATURE_UNDEAD")   return EMarketMode::CREATURE_UNDEAD;
	if (s == "RESOURCE_SKILL")    return EMarketMode::RESOURCE_SKILL;
	if (s == "MARKET_AFTER_LAST_PLACEHOLDER") return EMarketMode::MARKET_AFTER_LAST_PLACEHOLDER;
	return EMarketMode::RESOURCE_RESOURCE;
}
} // namespace

class TradeOnMarketplaceCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "TradeOnMarketplace"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const TradeOnMarketplace *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<TradeOnMarketplace>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["marketId"].isNumber())
			pack->marketId = ObjectInstanceID(static_cast<int32_t>(json["marketId"].Integer()));

		if (json["heroId"].isNumber())
			pack->heroId = ObjectInstanceID(static_cast<int32_t>(json["heroId"].Integer()));

		if (json["mode"].isString())
			pack->mode = marketModeFromString(json["mode"].String());

		if (json["r1"].isVector())
		{
			for (const auto & v : json["r1"].Vector())
				pack->r1.push_back(TradeItemSell(GameResID(static_cast<int32_t>(v.Integer()))));
		}

		if (json["r2"].isVector())
		{
			for (const auto & v : json["r2"].Vector())
				pack->r2.push_back(TradeItemBuy(GameResID(static_cast<int32_t>(v.Integer()))));
		}

		if (json["val"].isVector())
		{
			for (const auto & v : json["val"].Vector())
				pack->val.push_back(static_cast<ui32>(v.Integer()));
		}

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const TradeOnMarketplace &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["marketId"].Integer() = static_cast<int64_t>(p.marketId.getNum());
		out["heroId"].Integer() = static_cast<int64_t>(p.heroId.getNum());
		out["mode"].String() = marketModeToString(p.mode);

		JsonNode & r1 = out["r1"];
		r1.Vector();
		for (const auto & item : p.r1)
		{
			JsonNode entry;
			entry.Integer() = static_cast<int64_t>(item.getNum());
			r1.Vector().push_back(entry);
		}

		JsonNode & r2 = out["r2"];
		r2.Vector();
		for (const auto & item : p.r2)
		{
			JsonNode entry;
			entry.Integer() = static_cast<int64_t>(item.getNum());
			r2.Vector().push_back(entry);
		}

		JsonNode & val = out["val"];
		val.Vector();
		for (const auto & v : p.val)
		{
			JsonNode entry;
			entry.Integer() = static_cast<int64_t>(v);
			val.Vector().push_back(entry);
		}
	}
};

REGISTER_PACK_CODEC(TradeOnMarketplaceCodec)

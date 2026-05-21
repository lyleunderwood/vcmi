/*
 * MoveHero.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for MoveHero (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/MoveHero.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"
#include "../shared/Int3.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

namespace
{
std::string layerToString(const EPathfindingLayer & l)
{
	switch (l.getNum())
	{
		case EPathfindingLayer::LAND:   return "LAND";
		case EPathfindingLayer::SAIL:   return "SAIL";
		case EPathfindingLayer::WATER:  return "WATER";
		case EPathfindingLayer::AVIATE: return "AVIATE";
		case EPathfindingLayer::AIR:    return "AIR";
		case EPathfindingLayer::WRONG:  return "WRONG";
		case EPathfindingLayer::AUTO:   return "AUTO";
		default: return "WRONG";
	}
}

EPathfindingLayer layerFromString(const std::string & s)
{
	if (s == "LAND")   return EPathfindingLayer(EPathfindingLayer::LAND);
	if (s == "SAIL")   return EPathfindingLayer(EPathfindingLayer::SAIL);
	if (s == "WATER")  return EPathfindingLayer(EPathfindingLayer::WATER);
	if (s == "AVIATE") return EPathfindingLayer(EPathfindingLayer::AVIATE);
	if (s == "AIR")    return EPathfindingLayer(EPathfindingLayer::AIR);
	if (s == "AUTO")   return EPathfindingLayer(EPathfindingLayer::AUTO);
	return EPathfindingLayer(EPathfindingLayer::WRONG);
}
} // namespace

class MoveHeroCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "MoveHero"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const MoveHero *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<MoveHero>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["path"].isVector())
		{
			for (const auto & step : json["path"].Vector())
				pack->path.push_back(homamweb::shared::int3FromJson(step));
		}

		if (json["layer"].isString())
			pack->layer = layerFromString(json["layer"].String());

		if (json["hid"].isNumber())
			pack->hid = ObjectInstanceID(static_cast<int32_t>(json["hid"].Integer()));

		if (json["transit"].isBool())
			pack->transit = json["transit"].Bool();

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const MoveHero &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		JsonNode & path = out["path"];
		path.Vector();
		for (const auto & step : p.path)
			path.Vector().push_back(homamweb::shared::int3ToJson(step));

		out["layer"].String() = layerToString(p.layer);
		out["hid"].Integer() = static_cast<int64_t>(p.hid.getNum());
		out["transit"].Bool() = p.transit;
	}
};

REGISTER_PACK_CODEC(MoveHeroCodec)

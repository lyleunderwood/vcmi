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
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/int3.h"
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

JsonNode int3ToJson(const int3 & p)
{
	JsonNode out;
	out["x"].Integer() = static_cast<int64_t>(p.x);
	out["y"].Integer() = static_cast<int64_t>(p.y);
	out["z"].Integer() = static_cast<int64_t>(p.z);
	return out;
}

int3 int3FromJson(const JsonNode & json)
{
	int3 out;
	if (json["x"].isNumber()) out.x = static_cast<si32>(json["x"].Integer());
	if (json["y"].isNumber()) out.y = static_cast<si32>(json["y"].Integer());
	if (json["z"].isNumber()) out.z = static_cast<si32>(json["z"].Integer());
	return out;
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

		// Inherited from CPackForServer.
		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);
		if (json["requestID"].isNumber())
			pack->requestID = static_cast<uint32_t>(json["requestID"].Integer());

		if (json["path"].isVector())
		{
			for (const auto & step : json["path"].Vector())
				pack->path.push_back(int3FromJson(step));
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

		// Inherited from CPackForServer.
		out["player"] = homamweb::shared::playerColorToJson(p.player);
		out["requestID"].Integer() = static_cast<int64_t>(p.requestID);

		JsonNode & path = out["path"];
		path.Vector(); // ensure vector type
		for (const auto & step : p.path)
			path.Vector().push_back(int3ToJson(step));

		out["layer"].String() = layerToString(p.layer);
		out["hid"].Integer() = static_cast<int64_t>(p.hid.getNum());
		out["transit"].Bool() = p.transit;
	}
};

REGISTER_PACK_CODEC(MoveHeroCodec)

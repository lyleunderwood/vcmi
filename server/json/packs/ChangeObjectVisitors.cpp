/*
 * ChangeObjectVisitors.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for ChangeObjectVisitors (server -> client state update:
 * mark hero/player/team as having visited or scouted an object, or
 * clear all visitors).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/ChangeObjectVisitors.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

namespace
{
std::string visitModeToString(ChangeObjectVisitors::VisitMode m)
{
	switch (m)
	{
		case ChangeObjectVisitors::VISITOR_ADD_HERO:   return "VISITOR_ADD_HERO";
		case ChangeObjectVisitors::VISITOR_ADD_PLAYER: return "VISITOR_ADD_PLAYER";
		case ChangeObjectVisitors::VISITOR_SCOUTED:    return "VISITOR_SCOUTED";
		case ChangeObjectVisitors::VISITOR_CLEAR:      return "VISITOR_CLEAR";
		default: return "VISITOR_CLEAR";
	}
}

ChangeObjectVisitors::VisitMode visitModeFromString(const std::string & s)
{
	if (s == "VISITOR_ADD_HERO")   return ChangeObjectVisitors::VISITOR_ADD_HERO;
	if (s == "VISITOR_ADD_PLAYER") return ChangeObjectVisitors::VISITOR_ADD_PLAYER;
	if (s == "VISITOR_SCOUTED")    return ChangeObjectVisitors::VISITOR_SCOUTED;
	return ChangeObjectVisitors::VISITOR_CLEAR;
}
} // namespace

class ChangeObjectVisitorsCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "ChangeObjectVisitors"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const ChangeObjectVisitors *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<ChangeObjectVisitors>();

		if (json["mode"].isString())
			pack->mode = visitModeFromString(json["mode"].String());

		if (json["object"].isNumber())
			pack->object = ObjectInstanceID(static_cast<int32_t>(json["object"].Integer()));

		if (json["hero"].isNumber())
			pack->hero = ObjectInstanceID(static_cast<int32_t>(json["hero"].Integer()));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const ChangeObjectVisitors &>(pack);

		out["type"].String() = typeName();
		out["mode"].String() = visitModeToString(p.mode);
		out["object"].Integer() = static_cast<int64_t>(p.object.getNum());
		out["hero"].Integer() = static_cast<int64_t>(p.hero.getNum());
	}
};

REGISTER_PACK_CODEC(ChangeObjectVisitorsCodec)

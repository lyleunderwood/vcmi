/*
 * EntitiesChanged.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for EntitiesChanged (server -> client: batch of per-entity
 * runtime data overrides, each one tagged by Metatype + entity index plus
 * an opaque JSON config blob).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h (EntitiesChanged)
 * C++ entry definition: vcmi/lib/networkPacks/EntityChanges.h  (EntityChanges)
 * TypeScript twin:      wrapper/src/codecs/client/EntitiesChanged.ts
 *
 * The per-entry `data` member is a free-form `JsonNode` (engine-side config
 * patch). It is passed through verbatim — no structured representation.
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/networkPacks/EntityChanges.h"

#include <vcmi/Metatype.h>

namespace
{

// ---- Metatype -----------------------------------------------------------
std::string metatypeToString(Metatype m)
{
	switch (m)
	{
		case Metatype::UNKNOWN:              return "UNKNOWN";
		case Metatype::ARTIFACT:             return "ARTIFACT";
		case Metatype::ARTIFACT_INSTANCE:    return "ARTIFACT_INSTANCE";
		case Metatype::CREATURE:             return "CREATURE";
		case Metatype::CREATURE_INSTANCE:    return "CREATURE_INSTANCE";
		case Metatype::FACTION:              return "FACTION";
		case Metatype::HERO_CLASS:           return "HERO_CLASS";
		case Metatype::HERO_TYPE:            return "HERO_TYPE";
		case Metatype::HERO_INSTANCE:        return "HERO_INSTANCE";
		case Metatype::MAP_OBJECT_GROUP:     return "MAP_OBJECT_GROUP";
		case Metatype::MAP_OBJECT_TYPE:      return "MAP_OBJECT_TYPE";
		case Metatype::MAP_OBJECT_INSTANCE:  return "MAP_OBJECT_INSTANCE";
		case Metatype::SKILL:                return "SKILL";
		case Metatype::SPELL:                return "SPELL";
	}
	return "UNKNOWN";
}

Metatype metatypeFromString(const std::string & s)
{
	if (s == "UNKNOWN")             return Metatype::UNKNOWN;
	if (s == "ARTIFACT")            return Metatype::ARTIFACT;
	if (s == "ARTIFACT_INSTANCE")   return Metatype::ARTIFACT_INSTANCE;
	if (s == "CREATURE")            return Metatype::CREATURE;
	if (s == "CREATURE_INSTANCE")   return Metatype::CREATURE_INSTANCE;
	if (s == "FACTION")             return Metatype::FACTION;
	if (s == "HERO_CLASS")          return Metatype::HERO_CLASS;
	if (s == "HERO_TYPE")           return Metatype::HERO_TYPE;
	if (s == "HERO_INSTANCE")       return Metatype::HERO_INSTANCE;
	if (s == "MAP_OBJECT_GROUP")    return Metatype::MAP_OBJECT_GROUP;
	if (s == "MAP_OBJECT_TYPE")     return Metatype::MAP_OBJECT_TYPE;
	if (s == "MAP_OBJECT_INSTANCE") return Metatype::MAP_OBJECT_INSTANCE;
	if (s == "SKILL")               return Metatype::SKILL;
	if (s == "SPELL")               return Metatype::SPELL;
	throw std::runtime_error("EntitiesChanged: unknown Metatype '" + s + "'");
}

// ---- EntityChanges entry ------------------------------------------------
JsonNode entityChangesToJson(const EntityChanges & c)
{
	JsonNode out;
	out.Struct();
	out["metatype"].String() = metatypeToString(c.metatype);
	out["entityIndex"].Integer() = static_cast<int64_t>(c.entityIndex);
	// data: opaque passthrough — copy the JsonNode subtree verbatim.
	out["data"] = c.data;
	return out;
}

EntityChanges entityChangesFromJson(const JsonNode & json)
{
	EntityChanges c;
	if (json["metatype"].isString())
		c.metatype = metatypeFromString(json["metatype"].String());
	if (json["entityIndex"].isNumber())
		c.entityIndex = static_cast<int32_t>(json["entityIndex"].Integer());
	// data: opaque passthrough — copy the JsonNode subtree verbatim.
	c.data = json["data"];
	return c;
}

} // namespace

class EntitiesChangedCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "EntitiesChanged"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const EntitiesChanged *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<EntitiesChanged>();

		if (json["changes"].isVector())
		{
			for (const auto & entry : json["changes"].Vector())
				pack->changes.push_back(entityChangesFromJson(entry));
		}

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const EntitiesChanged &>(pack);

		out["type"].String() = typeName();
		JsonNode & changes = out["changes"];
		changes.Vector();
		for (const auto & c : p.changes)
			changes.Vector().push_back(entityChangesToJson(c));
	}
};

REGISTER_PACK_CODEC(EntitiesChangedCodec)

/*
 * Component.cpp, part of homam-web fork of VCMI engine.
 *
 * TODO: Phase-1 limitation. ComponentSubType is a VariantIdentifier<...>
 * — its concrete variant tag determines how the integer subType should
 * be interpreted (CreatureID vs ArtifactID vs SpellID, etc.). We emit
 * only the integer and lose the tag. Outbound is therefore lossy and
 * inbound cannot reconstruct the original variant; we use a default
 * VariantIdentifier. This is fine for InfoWindow/BlockingDialog (server
 * → client) where the client only needs to display, but would need
 * upgrading if we ever ingest Components from outside.
 */
#include "StdInc.h"

#include "Component.h"

namespace homamweb {
namespace shared {

namespace {
std::string componentTypeToString(ComponentType t)
{
	switch (t)
	{
		case ComponentType::NONE: return "NONE";
		case ComponentType::PRIM_SKILL: return "PRIM_SKILL";
		case ComponentType::SEC_SKILL: return "SEC_SKILL";
		case ComponentType::RESOURCE: return "RESOURCE";
		case ComponentType::RESOURCE_PER_DAY: return "RESOURCE_PER_DAY";
		case ComponentType::CREATURE: return "CREATURE";
		case ComponentType::ARTIFACT: return "ARTIFACT";
		case ComponentType::SPELL_SCROLL: return "SPELL_SCROLL";
		case ComponentType::MANA: return "MANA";
		case ComponentType::EXPERIENCE: return "EXPERIENCE";
		case ComponentType::LEVEL: return "LEVEL";
		case ComponentType::SPELL: return "SPELL";
		case ComponentType::MORALE: return "MORALE";
		case ComponentType::LUCK: return "LUCK";
		case ComponentType::BUILDING: return "BUILDING";
		case ComponentType::HERO_PORTRAIT: return "HERO_PORTRAIT";
		case ComponentType::FLAG: return "FLAG";
	}
	return "UNKNOWN";
}

ComponentType componentTypeFromString(const std::string & s)
{
	if (s == "NONE") return ComponentType::NONE;
	if (s == "PRIM_SKILL") return ComponentType::PRIM_SKILL;
	if (s == "SEC_SKILL") return ComponentType::SEC_SKILL;
	if (s == "RESOURCE") return ComponentType::RESOURCE;
	if (s == "RESOURCE_PER_DAY") return ComponentType::RESOURCE_PER_DAY;
	if (s == "CREATURE") return ComponentType::CREATURE;
	if (s == "ARTIFACT") return ComponentType::ARTIFACT;
	if (s == "SPELL_SCROLL") return ComponentType::SPELL_SCROLL;
	if (s == "MANA") return ComponentType::MANA;
	if (s == "EXPERIENCE") return ComponentType::EXPERIENCE;
	if (s == "LEVEL") return ComponentType::LEVEL;
	if (s == "SPELL") return ComponentType::SPELL;
	if (s == "MORALE") return ComponentType::MORALE;
	if (s == "LUCK") return ComponentType::LUCK;
	if (s == "BUILDING") return ComponentType::BUILDING;
	if (s == "HERO_PORTRAIT") return ComponentType::HERO_PORTRAIT;
	if (s == "FLAG") return ComponentType::FLAG;
	throw std::runtime_error("Component: unknown type '" + s + "'");
}
} // namespace

JsonNode componentToJson(const Component & c)
{
	JsonNode out;
	out.Struct();
	out["type"].String() = componentTypeToString(c.type);
	out["subType"].Integer() = static_cast<int64_t>(c.subType.getNum());
	if (c.value.has_value())
		out["value"].Integer() = static_cast<int64_t>(*c.value);
	else
		out["value"].setType(JsonNode::JsonType::DATA_NULL);
	return out;
}

Component componentFromJson(const JsonNode & json)
{
	Component c;
	c.type = componentTypeFromString(json["type"].String());
	// subType variant tag is lost on the wire; left as default VariantIdentifier.
	if (json["value"].isNumber())
		c.value = static_cast<int32_t>(json["value"].Integer());
	return c;
}

JsonNode componentsToJson(const std::vector<Component> & cs)
{
	JsonNode out;
	out.Vector();
	for (const auto & c : cs)
		out.Vector().push_back(componentToJson(c));
	return out;
}

std::vector<Component> componentsFromJson(const JsonNode & json)
{
	std::vector<Component> result;
	if (!json.isVector())
		return result;
	for (const auto & item : json.Vector())
		result.push_back(componentFromJson(item));
	return result;
}

} // namespace shared
} // namespace homamweb

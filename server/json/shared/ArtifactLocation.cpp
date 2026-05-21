/*
 * ArtifactLocation.cpp, part of homam-web fork of VCMI engine.
 */
#include "StdInc.h"

#include "ArtifactLocation.h"

namespace homamweb {
namespace shared {

JsonNode artifactLocationToJson(const ArtifactLocation & loc)
{
	JsonNode out;
	out["artHolder"].Integer() = static_cast<int64_t>(loc.artHolder.getNum());
	out["slot"].Integer() = static_cast<int64_t>(loc.slot.getNum());
	if (loc.creature.has_value())
		out["creature"].Integer() = static_cast<int64_t>(loc.creature->getNum());
	// else: omit (decoder treats absent/null as nullopt)
	return out;
}

ArtifactLocation artifactLocationFromJson(const JsonNode & json)
{
	ArtifactLocation out;
	if (json["artHolder"].isNumber())
		out.artHolder = ObjectInstanceID(static_cast<int32_t>(json["artHolder"].Integer()));
	if (json["slot"].isNumber())
		out.slot = ArtifactPosition(static_cast<int32_t>(json["slot"].Integer()));
	if (json["creature"].isNumber())
		out.creature = SlotID(static_cast<int32_t>(json["creature"].Integer()));
	return out;
}

} // namespace shared
} // namespace homamweb

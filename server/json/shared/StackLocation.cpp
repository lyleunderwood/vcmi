/*
 * StackLocation.cpp, part of homam-web fork of VCMI engine.
 */
#include "StdInc.h"

#include "StackLocation.h"

namespace homamweb {
namespace shared {

JsonNode stackLocationToJson(const StackLocation & loc)
{
	JsonNode out;
	out["army"].Integer() = static_cast<int64_t>(loc.army.getNum());
	out["slot"].Integer() = static_cast<int64_t>(loc.slot.getNum());
	return out;
}

StackLocation stackLocationFromJson(const JsonNode & json)
{
	StackLocation out;
	if (json["army"].isNumber())
		out.army = ObjectInstanceID(static_cast<int32_t>(json["army"].Integer()));
	if (json["slot"].isNumber())
		out.slot = SlotID(static_cast<int32_t>(json["slot"].Integer()));
	return out;
}

} // namespace shared
} // namespace homamweb

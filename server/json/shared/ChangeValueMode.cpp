/*
 * ChangeValueMode.cpp, part of homam-web fork of VCMI engine.
 */
#include "StdInc.h"

#include "ChangeValueMode.h"

namespace homamweb {
namespace shared {

JsonNode changeValueModeToJson(ChangeValueMode m)
{
	JsonNode out;
	switch (m)
	{
		case ChangeValueMode::ABSOLUTE: out.String() = "ABSOLUTE"; break;
		case ChangeValueMode::RELATIVE:
		default:                        out.String() = "RELATIVE"; break;
	}
	return out;
}

ChangeValueMode changeValueModeFromJson(const JsonNode & json)
{
	if (json.isString() && json.String() == "ABSOLUTE")
		return ChangeValueMode::ABSOLUTE;
	return ChangeValueMode::RELATIVE;
}

} // namespace shared
} // namespace homamweb

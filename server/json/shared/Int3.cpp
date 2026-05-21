/*
 * Int3.cpp, part of homam-web fork of VCMI engine.
 */
#include "StdInc.h"

#include "Int3.h"

namespace homamweb {
namespace shared {

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

} // namespace shared
} // namespace homamweb

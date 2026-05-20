/*
 * PlayerColor.cpp, part of homam-web fork of VCMI engine.
 */
#include "StdInc.h"

#include "PlayerColor.h"

namespace homamweb {
namespace shared {

JsonNode playerColorToJson(const PlayerColor & c)
{
	JsonNode out;
	out.Integer() = static_cast<int64_t>(c.getNum());
	return out;
}

PlayerColor playerColorFromJson(const JsonNode & json)
{
	if (!json.isNumber())
		throw std::runtime_error("PlayerColor: expected JSON integer");
	return PlayerColor(static_cast<int32_t>(json.Integer()));
}

} // namespace shared
} // namespace homamweb

/*
 * StartInfo.cpp, part of homam-web fork of VCMI engine.
 */
#include "StdInc.h"

#include "StartInfo.h"

#include "../../../lib/StartInfo.h"
#include "../../../lib/gameState/CGameState.h"

namespace homamweb {
namespace shared {

std::optional<JsonNode> opaqueStartInfoToJson(const std::shared_ptr<StartInfo> & p)
{
	if (!p)
		return std::nullopt;
	JsonNode out;
	out.Struct();
	return out;
}

std::optional<JsonNode> opaqueGameStateToJson(const std::shared_ptr<CGameState> & p)
{
	if (!p)
		return std::nullopt;
	JsonNode out;
	out.Struct();
	return out;
}

std::shared_ptr<StartInfo> opaqueStartInfoFromJson(const JsonNode &)
{
	return nullptr;
}

std::shared_ptr<CGameState> opaqueGameStateFromJson(const JsonNode &)
{
	return nullptr;
}

} // namespace shared
} // namespace homamweb

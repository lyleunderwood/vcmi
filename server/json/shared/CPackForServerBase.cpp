/*
 * CPackForServerBase.cpp, part of homam-web fork of VCMI engine.
 */
#include "StdInc.h"

#include "CPackForServerBase.h"
#include "PlayerColor.h"

#include "../../../lib/networkPacks/PacksForServer.h"

namespace homamweb {
namespace shared {

void readServerPackBase(const JsonNode & json, CPackForServer & pack)
{
	if (json["player"].isNumber())
		pack.player = playerColorFromJson(json["player"]);
	if (json["requestID"].isNumber())
		pack.requestID = static_cast<uint32_t>(json["requestID"].Integer());
}

void writeServerPackBase(const CPackForServer & pack, JsonNode & out)
{
	out["player"] = playerColorToJson(pack.player);
	out["requestID"].Integer() = static_cast<int64_t>(pack.requestID);
}

} // namespace shared
} // namespace homamweb

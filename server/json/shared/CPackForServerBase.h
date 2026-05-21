/*
 * CPackForServerBase.h, part of homam-web fork of VCMI engine.
 *
 * Shared helpers for the two fields every CPackForServer-derived pack
 * inherits: `player` (PlayerColor) and `requestID` (uint32_t).
 *
 * Every PacksForServer codec should call readServerPackBase / writeServerPackBase
 * to deal with these fields uniformly. The strict-decoder pattern means each
 * codec must also include "player" and "requestID" in its KNOWN_KEYS set
 * (see SERVER_PACK_BASE_KEYS in the TS twin).
 */
#pragma once

#include "../../../lib/json/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN
struct CPackForServer;
VCMI_LIB_NAMESPACE_END

namespace homamweb {
namespace shared {

void readServerPackBase(const JsonNode & json, CPackForServer & pack);
void writeServerPackBase(const CPackForServer & pack, JsonNode & out);

} // namespace shared
} // namespace homamweb

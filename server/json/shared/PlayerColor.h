/*
 * PlayerColor.h, part of homam-web fork of VCMI engine.
 *
 * Shared sub-type codec for PlayerColor (a StaticIdentifier<PlayerColor>
 * int32 wrapper). Used by many lobby and gameplay packs.
 *
 * On the wire we use the integer form (.getNum()). Future improvement
 * could map symbolic names (RED, BLUE, ..., NEUTRAL=255) to strings, but
 * for now integer fidelity is what callers actually need.
 */
#pragma once

#include "../../../lib/json/JsonNode.h"
#include "../../../lib/constants/EntityIdentifiers.h"

namespace homamweb {
namespace shared {

JsonNode playerColorToJson(const PlayerColor & c);
PlayerColor playerColorFromJson(const JsonNode & json);

} // namespace shared
} // namespace homamweb

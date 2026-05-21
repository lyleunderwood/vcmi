/*
 * Int3.h, part of homam-web fork of VCMI engine.
 *
 * Shared codec for int3 — the 3D map coordinate (x, y, z) that appears in
 * many gameplay packs (MoveHero.path, TryMoveHero, NewObject, ChangeObjPos,
 * etc.).
 *
 * Wire form: { "x": int, "y": int, "z": int }.
 */
#pragma once

#include "../../../lib/json/JsonNode.h"
#include "../../../lib/int3.h"

namespace homamweb {
namespace shared {

JsonNode int3ToJson(const int3 & p);
int3 int3FromJson(const JsonNode & json);

} // namespace shared
} // namespace homamweb

/*
 * StartInfo.h, part of homam-web fork of VCMI engine.
 *
 * Opaque-passthrough shared codec for StartInfo and CGameState.
 *
 * StartInfo is a large struct (mode, difficulty, playerInfos, simturnsInfo,
 * turnTimerInfo, extraOptionsInfo, mapname, mapGenOptions, campState).
 * Modelling all of those fields faithfully is significant work — many
 * sub-fields are themselves engine types with their own pointers and
 * variants. For phase 1 we treat StartInfo and CGameState as opaque
 * placeholders on the wire, matching the pattern established by
 * CMapInfo / CMapGenOptions in LobbySetMap:
 *
 *   outbound: emit an empty object `{}` if the pointer is non-null,
 *             otherwise omit the field.
 *   inbound:  leave the pointer as nullptr.
 *
 * This is enough for transport-layer plumbing and discriminator round-tripping
 * but does not let the wrapper introspect the actual game-start configuration.
 *
 * TODO: when the wrapper needs a real lobby UI, promote StartInfo to a
 * structured codec at this level. PlayerSettings, SimturnsInfo, etc. will
 * each need their own shared sub-codec.
 */
#pragma once

#include "../../../lib/json/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN
struct StartInfo;
class CGameState;
VCMI_LIB_NAMESPACE_END

namespace homamweb {
namespace shared {

/// Emit an opaque placeholder if the pointer is non-null; the JsonNode argument
/// is populated, then the caller writes it under a field name with `out[...] = ...`.
/// Returns nullopt if the pointer is null (caller may omit the field).
std::optional<JsonNode> opaqueStartInfoToJson(const std::shared_ptr<StartInfo> & p);
std::optional<JsonNode> opaqueGameStateToJson(const std::shared_ptr<CGameState> & p);

/// Inbound: we do not currently reconstruct StartInfo / CGameState from JSON.
/// These are kept for symmetry but always return nullptr.
std::shared_ptr<StartInfo> opaqueStartInfoFromJson(const JsonNode & json);
std::shared_ptr<CGameState> opaqueGameStateFromJson(const JsonNode & json);

} // namespace shared
} // namespace homamweb

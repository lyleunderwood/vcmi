/*
 * ChangeValueMode.h, part of homam-web fork of VCMI engine.
 *
 * Shared codec for ChangeValueMode (RELATIVE | ABSOLUTE). Used by many
 * server→client state-update packs that mutate a value either as a delta
 * or as a replacement.
 */
#pragma once

#include "../../../lib/json/JsonNode.h"
#include "../../../lib/constants/Enumerations.h"

namespace homamweb {
namespace shared {

JsonNode changeValueModeToJson(ChangeValueMode m);
ChangeValueMode changeValueModeFromJson(const JsonNode & json);

} // namespace shared
} // namespace homamweb

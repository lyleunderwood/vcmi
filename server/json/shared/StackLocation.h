/*
 * StackLocation.h, part of homam-web fork of VCMI engine.
 *
 * Shared codec for StackLocation (army ObjectInstanceID + slot SlotID).
 * Used by ArrangeStacks, BulkMoveArmy, BulkSplitStack, BulkMergeStacks,
 * BulkSplitAndRebalanceStack, DisbandCreature, and similar army packs.
 */
#pragma once

#include "../../../lib/json/JsonNode.h"
#include "../../../lib/networkPacks/StackLocation.h"

namespace homamweb {
namespace shared {

JsonNode stackLocationToJson(const StackLocation & loc);
StackLocation stackLocationFromJson(const JsonNode & json);

} // namespace shared
} // namespace homamweb

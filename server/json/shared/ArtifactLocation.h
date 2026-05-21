/*
 * ArtifactLocation.h, part of homam-web fork of VCMI engine.
 *
 * Shared codec for ArtifactLocation (artHolder + slot + optional creature).
 * Used by ExchangeArtifacts, BulkExchangeArtifacts, ManageBackpackArtifacts,
 * ManageEquippedArtifacts, EraseArtifactByClient, and similar artifact packs.
 */
#pragma once

#include "../../../lib/json/JsonNode.h"
#include "../../../lib/networkPacks/ArtifactLocation.h"

namespace homamweb {
namespace shared {

JsonNode artifactLocationToJson(const ArtifactLocation & loc);
ArtifactLocation artifactLocationFromJson(const JsonNode & json);

} // namespace shared
} // namespace homamweb

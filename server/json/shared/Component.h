/*
 * Component.h, part of homam-web fork of VCMI engine.
 *
 * Shared sub-type codec for Component (used by InfoWindow, BlockingDialog,
 * and other notification packs). Component is a small struct: a type enum,
 * a numeric subtype identifier (opaque from our perspective), and an
 * optional integer value.
 */
#pragma once

#include "../../../lib/json/JsonNode.h"
#include "../../../lib/networkPacks/Component.h"

namespace homamweb {
namespace shared {

JsonNode componentToJson(const Component & c);
Component componentFromJson(const JsonNode & json);

/// Convenience for fields that are vectors of Components.
JsonNode componentsToJson(const std::vector<Component> & cs);
std::vector<Component> componentsFromJson(const JsonNode & json);

} // namespace shared
} // namespace homamweb

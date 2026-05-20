/*
 * MetaString.h, part of homam-web fork of VCMI engine.
 *
 * Shared sub-type codec for MetaString. Phase 1 simplification: we send
 * the fully-rendered string and accept raw strings on inbound. We lose
 * the structured form (i18n placeholders, EMessage opcodes) but the
 * homam-web project doesn't need localization.
 */
#pragma once

#include "../../../lib/json/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN
class MetaString;
VCMI_LIB_NAMESPACE_END

namespace homamweb {
namespace shared {

/// Render the MetaString to a plain JSON string. Lossy for localization;
/// see TODO in MetaString.cpp.
JsonNode metaStringToJson(const MetaString & m);

/// Construct a MetaString from a plain JSON string (treated as raw text).
MetaString metaStringFromJson(const JsonNode & json);

} // namespace shared
} // namespace homamweb

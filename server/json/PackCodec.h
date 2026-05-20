/*
 * PackCodec.h, part of homam-web fork of VCMI engine.
 *
 * Per-pack JSON codec interface. Each network pack subtype that the JSON
 * adapter supports has exactly one PackCodec subclass implementing
 * bidirectional translation between JsonNode and the C++ pack struct.
 *
 * Codecs self-register at static-init time via REGISTER_PACK_CODEC; the
 * JsonAdapter discovers them via PackCodecRegistry at request time.
 */
#pragma once

#include "../../lib/json/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN
struct CPack;
VCMI_LIB_NAMESPACE_END

class PackCodec
{
public:
	virtual ~PackCodec() = default;

	/// The "type" string used on the wire (matches the C++ struct name).
	virtual std::string typeName() const = 0;

	/// Construct a new pack from a JSON object. Caller owns the result.
	/// Throws std::runtime_error on missing required fields or type mismatches.
	virtual std::unique_ptr<CPack> fromJson(const JsonNode & json) const = 0;

	/// Populate the given JsonNode (must be of struct type) with this pack's fields.
	/// Sets the "type" field automatically.
	virtual void toJson(const CPack & pack, JsonNode & out) const = 0;

	/// True iff `pack` is a value this codec knows how to serialize.
	/// Used by the registry to find the right outbound codec for a pack instance.
	virtual bool matches(const CPack & pack) const = 0;
};

/*
 * MetaString.cpp, part of homam-web fork of VCMI engine.
 *
 * TODO: Phase-1 lossy: we emit only the rendered string and lose the
 * structured form (placeholder slots, EMessage opcodes, indexed strings).
 * If we ever support multiple languages in the web client we'll need to
 * upgrade this to serialize the full MetaString contents.
 */
#include "StdInc.h"

#include "MetaString.h"

#include "../../../lib/texts/MetaString.h"

namespace homamweb {
namespace shared {

JsonNode metaStringToJson(const MetaString & m)
{
	JsonNode out;
	out.String() = m.toString();
	return out;
}

MetaString metaStringFromJson(const JsonNode & json)
{
	if (!json.isString())
		throw std::runtime_error("MetaString: expected JSON string");
	return MetaString::createFromRawString(json.String());
}

} // namespace shared
} // namespace homamweb

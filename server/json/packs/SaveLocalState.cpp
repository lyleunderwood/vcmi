/*
 * SaveLocalState.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for SaveLocalState (a client -> server push: persist arbitrary
 * client-side per-game state. The `data` field is a free-form JsonNode and
 * is treated as opaque — round-tripped verbatim with no structural validation.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/SaveLocalState.h
 * TypeScript twin:      wrapper/src/codecs/server/SaveLocalState.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/SaveLocalState.h"

class SaveLocalStateCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "SaveLocalState"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const SaveLocalState *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<SaveLocalState>();
		homamweb::shared::readServerPackBase(json, *pack);

		// data: opaque passthrough — copy the JsonNode subtree verbatim.
		pack->data = json["data"];

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const SaveLocalState &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		// data: opaque passthrough — copy the JsonNode subtree verbatim.
		out["data"] = p.data;
	}
};

REGISTER_PACK_CODEC(SaveLocalStateCodec)

/*
 * BulkSplitStack.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BulkSplitStack (a client -> server gameplay action:
 * split a creature stack into a new slot, transferring `amount` creatures).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/BulkSplitStack.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class BulkSplitStackCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BulkSplitStack"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BulkSplitStack *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BulkSplitStack>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["src"].isNumber())
			pack->src = SlotID(static_cast<int32_t>(json["src"].Integer()));

		if (json["srcOwner"].isNumber())
			pack->srcOwner = ObjectInstanceID(static_cast<int32_t>(json["srcOwner"].Integer()));

		if (json["amount"].isNumber())
			pack->amount = static_cast<si32>(json["amount"].Integer());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BulkSplitStack &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["src"].Integer() = static_cast<int64_t>(p.src.getNum());
		out["srcOwner"].Integer() = static_cast<int64_t>(p.srcOwner.getNum());
		out["amount"].Integer() = static_cast<int64_t>(p.amount);
	}
};

REGISTER_PACK_CODEC(BulkSplitStackCodec)

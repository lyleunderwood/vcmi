/*
 * BulkSplitAndRebalanceStack.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BulkSplitAndRebalanceStack (a client -> server gameplay
 * action: split a creature stack and rebalance across empty slots in the
 * same army).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/BulkSplitAndRebalanceStack.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class BulkSplitAndRebalanceStackCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BulkSplitAndRebalanceStack"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BulkSplitAndRebalanceStack *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BulkSplitAndRebalanceStack>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["src"].isNumber())
			pack->src = SlotID(static_cast<int32_t>(json["src"].Integer()));

		if (json["srcOwner"].isNumber())
			pack->srcOwner = ObjectInstanceID(static_cast<int32_t>(json["srcOwner"].Integer()));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BulkSplitAndRebalanceStack &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["src"].Integer() = static_cast<int64_t>(p.src.getNum());
		out["srcOwner"].Integer() = static_cast<int64_t>(p.srcOwner.getNum());
	}
};

REGISTER_PACK_CODEC(BulkSplitAndRebalanceStackCodec)

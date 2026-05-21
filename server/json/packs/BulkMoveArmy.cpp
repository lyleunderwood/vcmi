/*
 * BulkMoveArmy.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BulkMoveArmy (a client -> server gameplay action:
 * move a whole army between two objects, starting from a given slot).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/BulkMoveArmy.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class BulkMoveArmyCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BulkMoveArmy"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BulkMoveArmy *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BulkMoveArmy>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["srcSlot"].isNumber())
			pack->srcSlot = SlotID(static_cast<int32_t>(json["srcSlot"].Integer()));

		if (json["srcArmy"].isNumber())
			pack->srcArmy = ObjectInstanceID(static_cast<int32_t>(json["srcArmy"].Integer()));

		if (json["destArmy"].isNumber())
			pack->destArmy = ObjectInstanceID(static_cast<int32_t>(json["destArmy"].Integer()));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BulkMoveArmy &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["srcSlot"].Integer() = static_cast<int64_t>(p.srcSlot.getNum());
		out["srcArmy"].Integer() = static_cast<int64_t>(p.srcArmy.getNum());
		out["destArmy"].Integer() = static_cast<int64_t>(p.destArmy.getNum());
	}
};

REGISTER_PACK_CODEC(BulkMoveArmyCodec)

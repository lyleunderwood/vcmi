/*
 * UpgradeCreature.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for UpgradeCreature (a client -> server gameplay action:
 * upgrade a creature stack in a hero's/town's army to a higher tier).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/UpgradeCreature.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class UpgradeCreatureCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "UpgradeCreature"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const UpgradeCreature *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<UpgradeCreature>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["pos"].isNumber())
			pack->pos = SlotID(static_cast<int32_t>(json["pos"].Integer()));

		if (json["id"].isNumber())
			pack->id = ObjectInstanceID(static_cast<int32_t>(json["id"].Integer()));

		if (json["cid"].isNumber())
			pack->cid = CreatureID(static_cast<int32_t>(json["cid"].Integer()));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const UpgradeCreature &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["pos"].Integer() = static_cast<int64_t>(p.pos.getNum());
		out["id"].Integer() = static_cast<int64_t>(p.id.getNum());
		out["cid"].Integer() = static_cast<int64_t>(p.cid.getNum());
	}
};

REGISTER_PACK_CODEC(UpgradeCreatureCodec)

/*
 * DisbandCreature.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for DisbandCreature (a client -> server gameplay action:
 * disband a creature stack in a hero's/town's army).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/DisbandCreature.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class DisbandCreatureCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "DisbandCreature"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const DisbandCreature *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<DisbandCreature>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["pos"].isNumber())
			pack->pos = SlotID(static_cast<int32_t>(json["pos"].Integer()));

		if (json["id"].isNumber())
			pack->id = ObjectInstanceID(static_cast<int32_t>(json["id"].Integer()));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const DisbandCreature &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["pos"].Integer() = static_cast<int64_t>(p.pos.getNum());
		out["id"].Integer() = static_cast<int64_t>(p.id.getNum());
	}
};

REGISTER_PACK_CODEC(DisbandCreatureCodec)

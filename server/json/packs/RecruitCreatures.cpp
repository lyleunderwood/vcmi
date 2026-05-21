/*
 * RecruitCreatures.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for RecruitCreatures (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/RecruitCreatures.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class RecruitCreaturesCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "RecruitCreatures"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const RecruitCreatures *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<RecruitCreatures>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["tid"].isNumber())
			pack->tid = ObjectInstanceID(static_cast<int32_t>(json["tid"].Integer()));

		if (json["dst"].isNumber())
			pack->dst = ObjectInstanceID(static_cast<int32_t>(json["dst"].Integer()));

		if (json["crid"].isNumber())
			pack->crid = CreatureID(static_cast<int32_t>(json["crid"].Integer()));

		if (json["amount"].isNumber())
			pack->amount = static_cast<uint32_t>(json["amount"].Integer());

		if (json["level"].isNumber())
			pack->level = static_cast<int32_t>(json["level"].Integer());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const RecruitCreatures &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["tid"].Integer() = static_cast<int64_t>(p.tid.getNum());
		out["dst"].Integer() = static_cast<int64_t>(p.dst.getNum());
		out["crid"].Integer() = static_cast<int64_t>(p.crid.getNum());
		out["amount"].Integer() = static_cast<int64_t>(p.amount);
		out["level"].Integer() = static_cast<int64_t>(p.level);
	}
};

REGISTER_PACK_CODEC(RecruitCreaturesCodec)

/*
 * ArrangeStacks.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for ArrangeStacks (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/ArrangeStacks.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class ArrangeStacksCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "ArrangeStacks"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const ArrangeStacks *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<ArrangeStacks>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["what"].isNumber())
			pack->what = static_cast<ui8>(json["what"].Integer());

		if (json["p1"].isNumber())
			pack->p1 = SlotID(static_cast<si32>(json["p1"].Integer()));

		if (json["p2"].isNumber())
			pack->p2 = SlotID(static_cast<si32>(json["p2"].Integer()));

		if (json["id1"].isNumber())
			pack->id1 = ObjectInstanceID(static_cast<int32_t>(json["id1"].Integer()));

		if (json["id2"].isNumber())
			pack->id2 = ObjectInstanceID(static_cast<int32_t>(json["id2"].Integer()));

		if (json["val"].isNumber())
			pack->val = static_cast<si32>(json["val"].Integer());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const ArrangeStacks &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["what"].Integer() = static_cast<int64_t>(p.what);
		out["p1"].Integer() = static_cast<int64_t>(p.p1.getNum());
		out["p2"].Integer() = static_cast<int64_t>(p.p2.getNum());
		out["id1"].Integer() = static_cast<int64_t>(p.id1.getNum());
		out["id2"].Integer() = static_cast<int64_t>(p.id2.getNum());
		out["val"].Integer() = static_cast<int64_t>(p.val);
	}
};

REGISTER_PACK_CODEC(ArrangeStacksCodec)

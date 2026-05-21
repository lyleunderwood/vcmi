/*
 * CastAdvSpell.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for CastAdvSpell (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/CastAdvSpell.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"
#include "../shared/Int3.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class CastAdvSpellCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "CastAdvSpell"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const CastAdvSpell *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<CastAdvSpell>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["hid"].isNumber())
			pack->hid = ObjectInstanceID(static_cast<int32_t>(json["hid"].Integer()));

		if (json["sid"].isNumber())
			pack->sid = SpellID(static_cast<int32_t>(json["sid"].Integer()));

		if (json["pos"].isStruct())
			pack->pos = homamweb::shared::int3FromJson(json["pos"]);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const CastAdvSpell &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["hid"].Integer() = static_cast<int64_t>(p.hid.getNum());
		out["sid"].Integer() = static_cast<int64_t>(p.sid.getNum());
		out["pos"] = homamweb::shared::int3ToJson(p.pos);
	}
};

REGISTER_PACK_CODEC(CastAdvSpellCodec)

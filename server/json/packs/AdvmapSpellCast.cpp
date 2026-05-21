/*
 * AdvmapSpellCast.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for AdvmapSpellCast (a server -> client notification that a
 * hero has cast an adventure-map spell).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/AdvmapSpellCast.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class AdvmapSpellCastCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "AdvmapSpellCast"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const AdvmapSpellCast *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<AdvmapSpellCast>();

		if (json["casterID"].isNumber())
			pack->casterID = ObjectInstanceID(static_cast<int32_t>(json["casterID"].Integer()));

		if (json["spellID"].isNumber())
			pack->spellID = SpellID(static_cast<int32_t>(json["spellID"].Integer()));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const AdvmapSpellCast &>(pack);

		out["type"].String() = typeName();
		out["casterID"].Integer() = static_cast<int64_t>(p.casterID.getNum());
		out["spellID"].Integer() = static_cast<int64_t>(p.spellID.getNum());
	}
};

REGISTER_PACK_CODEC(AdvmapSpellCastCodec)

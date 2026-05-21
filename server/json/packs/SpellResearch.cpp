/*
 * SpellResearch.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for SpellResearch (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/SpellResearch.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class SpellResearchCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "SpellResearch"; }

	bool matches(const CPack & pack) const override
	{
		return typeid(pack) == typeid(SpellResearch);
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<SpellResearch>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["tid"].isNumber())
			pack->tid = ObjectInstanceID(static_cast<int32_t>(json["tid"].Integer()));

		if (json["spellAtSlot"].isNumber())
			pack->spellAtSlot = SpellID(static_cast<int32_t>(json["spellAtSlot"].Integer()));

		if (json["accepted"].isBool())
			pack->accepted = json["accepted"].Bool();

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const SpellResearch &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["tid"].Integer() = static_cast<int64_t>(p.tid.getNum());
		out["spellAtSlot"].Integer() = static_cast<int64_t>(p.spellAtSlot.getNum());
		out["accepted"].Bool() = p.accepted;
	}
};

REGISTER_PACK_CODEC(SpellResearchCodec)

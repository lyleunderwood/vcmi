/*
 * ChangeSpells.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for ChangeSpells (server -> client state update: give or take
 * a set of spells from a hero).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/ChangeSpells.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class ChangeSpellsCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "ChangeSpells"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const ChangeSpells *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<ChangeSpells>();

		if (json["learn"].isNumber())
			pack->learn = static_cast<ui8>(json["learn"].Integer());

		if (json["hid"].isNumber())
			pack->hid = ObjectInstanceID(static_cast<int32_t>(json["hid"].Integer()));

		if (json["spells"].isVector())
		{
			for (const auto & s : json["spells"].Vector())
				pack->spells.insert(SpellID(static_cast<int32_t>(s.Integer())));
		}

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const ChangeSpells &>(pack);

		out["type"].String() = typeName();
		out["learn"].Integer() = static_cast<int64_t>(p.learn);
		out["hid"].Integer() = static_cast<int64_t>(p.hid.getNum());

		JsonNode & spells = out["spells"];
		spells.Vector();
		for (const auto & s : p.spells)
		{
			JsonNode entry;
			entry.Integer() = static_cast<int64_t>(s.getNum());
			spells.Vector().push_back(entry);
		}
	}
};

REGISTER_PACK_CODEC(ChangeSpellsCodec)

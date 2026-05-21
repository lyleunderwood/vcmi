/*
 * SetResearchedSpells.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for SetResearchedSpells (server -> client state update: set the
 * list of spells researched in a town's mage guild for the current week).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/SetResearchedSpells.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class SetResearchedSpellsCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "SetResearchedSpells"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const SetResearchedSpells *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<SetResearchedSpells>();

		if (json["level"].isNumber())
			pack->level = static_cast<ui8>(json["level"].Integer());

		if (json["tid"].isNumber())
			pack->tid = ObjectInstanceID(static_cast<int32_t>(json["tid"].Integer()));

		if (json["spells"].isVector())
		{
			for (const auto & spell : json["spells"].Vector())
				pack->spells.push_back(SpellID(static_cast<int32_t>(spell.Integer())));
		}

		if (json["accepted"].isBool())
			pack->accepted = json["accepted"].Bool();

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const SetResearchedSpells &>(pack);

		out["type"].String() = typeName();
		out["level"].Integer() = static_cast<int64_t>(p.level);
		out["tid"].Integer() = static_cast<int64_t>(p.tid.getNum());

		JsonNode & spells = out["spells"];
		spells.Vector(); // ensure vector type even when empty
		for (const auto & spell : p.spells)
		{
			JsonNode node;
			node.Integer() = static_cast<int64_t>(spell.getNum());
			spells.Vector().push_back(node);
		}

		out["accepted"].Bool() = p.accepted;
	}
};

REGISTER_PACK_CODEC(SetResearchedSpellsCodec)

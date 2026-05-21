/*
 * AddQuest.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for AddQuest (server -> client: a quest is granted to a
 * player; the player's quest log gets a new entry).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/AddQuest.ts
 *
 * NOTE: QuestInfo only carries a single ObjectInstanceID on the wire in
 * the NO_RAW_POINTERS_IN_SERIALIZER branch. We mirror that minimal shape
 * here ({ obj: int }) — the heavy CQuest object lookup happens client-side
 * via IGameInfoCallback, not over the wire.
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/gameState/QuestInfo.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class AddQuestCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "AddQuest"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const AddQuest *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<AddQuest>();

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		if (json["quest"].isStruct())
		{
			const JsonNode & q = json["quest"];
			if (q["obj"].isNumber())
				pack->quest.obj = ObjectInstanceID(static_cast<int32_t>(q["obj"].Integer()));
		}

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const AddQuest &>(pack);

		out["type"].String() = typeName();
		out["player"] = homamweb::shared::playerColorToJson(p.player);
		out["quest"]["obj"].Integer() = static_cast<int64_t>(p.quest.obj.getNum());
	}
};

REGISTER_PACK_CODEC(AddQuestCodec)

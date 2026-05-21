/*
 * NewObject.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for NewObject (server -> client state update notifying that a
 * new map object has been created).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/NewObject.ts
 *
 * NOTE: `newObject` is a std::shared_ptr<CGObjectInstance> — a heavyweight
 * VCMI engine type whose full field model is not in scope for this codec.
 * It is treated as opaque on the wire: toJson emits a placeholder empty
 * struct when the pointer is non-null (or omits the field when null), and
 * fromJson leaves the pointer as nullptr. A future codec pass should
 * promote CGObjectInstance to a shared codec. Status: partial.
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"

class NewObjectCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "NewObject"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const NewObject *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<NewObject>();

		// newObject is opaque on the wire; leave as nullptr.
		// (Future work: promote CGObjectInstance to a shared codec.)
		(void)json;

		if (json["initiator"].isNumber())
			pack->initiator = homamweb::shared::playerColorFromJson(json["initiator"]);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const NewObject &>(pack);

		out["type"].String() = typeName();

		if (p.newObject)
			out["newObject"].Struct(); // opaque placeholder
		// else: leave field absent

		out["initiator"] = homamweb::shared::playerColorToJson(p.initiator);
	}
};

REGISTER_PACK_CODEC(NewObjectCodec)

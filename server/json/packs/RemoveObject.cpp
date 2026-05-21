/*
 * RemoveObject.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for RemoveObject (server -> client state update).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/RemoveObject.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"

class RemoveObjectCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "RemoveObject"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const RemoveObject *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<RemoveObject>();

		if (json["objectID"].isNumber())
			pack->objectID = ObjectInstanceID(static_cast<int32_t>(json["objectID"].Integer()));

		if (json["initiator"].isNumber())
			pack->initiator = homamweb::shared::playerColorFromJson(json["initiator"]);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const RemoveObject &>(pack);

		out["type"].String() = typeName();
		out["objectID"].Integer() = static_cast<int64_t>(p.objectID.getNum());
		out["initiator"] = homamweb::shared::playerColorToJson(p.initiator);
	}
};

REGISTER_PACK_CODEC(RemoveObjectCodec)

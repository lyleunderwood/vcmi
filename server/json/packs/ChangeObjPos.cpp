/*
 * ChangeObjPos.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for ChangeObjPos (a server -> client update relocating an
 * existing map object to a new tile).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/ChangeObjPos.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/Int3.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class ChangeObjPosCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "ChangeObjPos"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const ChangeObjPos *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<ChangeObjPos>();

		if (json["objid"].isNumber())
			pack->objid = ObjectInstanceID(static_cast<int32_t>(json["objid"].Integer()));

		if (json["nPos"].isStruct())
			pack->nPos = homamweb::shared::int3FromJson(json["nPos"]);

		if (json["initiator"].isNumber())
			pack->initiator = homamweb::shared::playerColorFromJson(json["initiator"]);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const ChangeObjPos &>(pack);

		out["type"].String() = typeName();
		out["objid"].Integer() = static_cast<int64_t>(p.objid.getNum());
		out["nPos"] = homamweb::shared::int3ToJson(p.nPos);
		out["initiator"] = homamweb::shared::playerColorToJson(p.initiator);
	}
};

REGISTER_PACK_CODEC(ChangeObjPosCodec)

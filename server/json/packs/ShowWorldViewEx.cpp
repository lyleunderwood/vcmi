/*
 * ShowWorldViewEx.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for ShowWorldViewEx (server -> client: opens an extended world-
 * view window for the given player, showing terrain plus a set of map object
 * positions/icons).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h (ShowWorldViewEx)
 * TypeScript twin:      wrapper/src/codecs/client/ShowWorldViewEx.ts
 *
 * Nested sub-struct ObjectPosInfo (lib/spells/ViewSpellInt.h) is inlined here
 * as a field sub-codec. If reused by other packs, promote to shared/.
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/Int3.h"
#include "../shared/PlayerColor.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/spells/ViewSpellInt.h"
#include "../../../lib/constants/EntityIdentifiers.h"

namespace
{

// ---- ObjectPosInfo ------------------------------------------------------
JsonNode objectPosInfoToJson(const ObjectPosInfo & p)
{
	JsonNode out;
	out.Struct();
	out["pos"] = homamweb::shared::int3ToJson(p.pos);
	out["id"].Integer() = static_cast<int64_t>(p.id.getNum());
	out["subId"].Integer() = static_cast<int64_t>(p.subId);
	out["owner"] = homamweb::shared::playerColorToJson(p.owner);
	return out;
}

ObjectPosInfo objectPosInfoFromJson(const JsonNode & json)
{
	ObjectPosInfo p;
	if (json["pos"].isStruct())
		p.pos = homamweb::shared::int3FromJson(json["pos"]);
	if (json["id"].isNumber())
		p.id = Obj(static_cast<int32_t>(json["id"].Integer()));
	if (json["subId"].isNumber())
		p.subId = static_cast<si32>(json["subId"].Integer());
	if (json["owner"].isNumber())
		p.owner = homamweb::shared::playerColorFromJson(json["owner"]);
	return p;
}

} // namespace

class ShowWorldViewExCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "ShowWorldViewEx"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const ShowWorldViewEx *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<ShowWorldViewEx>();

		if (json["player"].isNumber())
			pack->player = homamweb::shared::playerColorFromJson(json["player"]);

		if (json["showTerrain"].isBool())
			pack->showTerrain = json["showTerrain"].Bool();

		if (json["objectPositions"].isVector())
		{
			for (const auto & entry : json["objectPositions"].Vector())
				pack->objectPositions.push_back(objectPosInfoFromJson(entry));
		}

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const ShowWorldViewEx &>(pack);

		out["type"].String() = typeName();
		out["player"] = homamweb::shared::playerColorToJson(p.player);
		out["showTerrain"].Bool() = p.showTerrain;

		JsonNode & positions = out["objectPositions"];
		positions.Vector();
		for (const auto & e : p.objectPositions)
			positions.Vector().push_back(objectPosInfoToJson(e));
	}
};

REGISTER_PACK_CODEC(ShowWorldViewExCodec)

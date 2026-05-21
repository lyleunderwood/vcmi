/*
 * GarrisonDialog.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for GarrisonDialog (server -> client query opening the
 * garrison/hero exchange UI for a town or garrison object).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/GarrisonDialog.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/MetaString.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/texts/MetaString.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class GarrisonDialogCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "GarrisonDialog"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const GarrisonDialog *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<GarrisonDialog>();

		// Inherited from Query.
		if (json["queryID"].isNumber())
			pack->queryID = QueryID(static_cast<int32_t>(json["queryID"].Integer()));

		if (json["objid"].isNumber())
			pack->objid = ObjectInstanceID(static_cast<int32_t>(json["objid"].Integer()));
		if (json["hid"].isNumber())
			pack->hid = ObjectInstanceID(static_cast<int32_t>(json["hid"].Integer()));

		if (json["removableUnits"].isBool())
			pack->removableUnits = json["removableUnits"].Bool();

		pack->customTitle = homamweb::shared::metaStringFromJson(json["customTitle"]);

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const GarrisonDialog &>(pack);

		out["type"].String() = typeName();

		// Inherited from Query.
		out["queryID"].Integer() = static_cast<int64_t>(p.queryID.getNum());

		out["objid"].Integer() = static_cast<int64_t>(p.objid.getNum());
		out["hid"].Integer() = static_cast<int64_t>(p.hid.getNum());
		out["removableUnits"].Bool() = p.removableUnits;
		out["customTitle"] = homamweb::shared::metaStringToJson(p.customTitle);
	}
};

REGISTER_PACK_CODEC(GarrisonDialogCodec)

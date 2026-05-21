/*
 * ManageEquippedArtifacts.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for ManageEquippedArtifacts (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/ManageEquippedArtifacts.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class ManageEquippedArtifactsCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "ManageEquippedArtifacts"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const ManageEquippedArtifacts *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<ManageEquippedArtifacts>();
		homamweb::shared::readServerPackBase(json, *pack);

		if (json["artHolder"].isNumber())
			pack->artHolder = ObjectInstanceID(static_cast<int32_t>(json["artHolder"].Integer()));

		if (json["costumeIdx"].isNumber())
			pack->costumeIdx = static_cast<uint32_t>(json["costumeIdx"].Integer());

		if (json["saveCostume"].isBool())
			pack->saveCostume = json["saveCostume"].Bool();

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const ManageEquippedArtifacts &>(pack);

		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);

		out["artHolder"].Integer() = static_cast<int64_t>(p.artHolder.getNum());
		out["costumeIdx"].Integer() = static_cast<int64_t>(p.costumeIdx);
		out["saveCostume"].Bool() = p.saveCostume;
	}
};

REGISTER_PACK_CODEC(ManageEquippedArtifactsCodec)

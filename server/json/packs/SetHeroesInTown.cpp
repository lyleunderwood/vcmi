/*
 * SetHeroesInTown.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for SetHeroesInTown (server -> client state update: sets
 * the visiting and garrison hero slots for a town).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClient.h
 * TypeScript twin:      wrapper/src/codecs/client/SetHeroesInTown.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClient.h"
#include "../../../lib/constants/EntityIdentifiers.h"

class SetHeroesInTownCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "SetHeroesInTown"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const SetHeroesInTown *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<SetHeroesInTown>();

		if (json["tid"].isNumber())
			pack->tid = ObjectInstanceID(static_cast<int32_t>(json["tid"].Integer()));

		if (json["visiting"].isNumber())
			pack->visiting = ObjectInstanceID(static_cast<int32_t>(json["visiting"].Integer()));

		if (json["garrison"].isNumber())
			pack->garrison = ObjectInstanceID(static_cast<int32_t>(json["garrison"].Integer()));

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const SetHeroesInTown &>(pack);

		out["type"].String() = typeName();
		out["tid"].Integer() = static_cast<int64_t>(p.tid.getNum());
		out["visiting"].Integer() = static_cast<int64_t>(p.visiting.getNum());
		out["garrison"].Integer() = static_cast<int64_t>(p.garrison.getNum());
	}
};

REGISTER_PACK_CODEC(SetHeroesInTownCodec)

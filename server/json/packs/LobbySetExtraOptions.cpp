/*
 * LobbySetExtraOptions.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbySetExtraOptions.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbySetExtraOptions.ts
 *
 * NOTE: ExtraOptionsInfo is not (yet) promoted to a shared codec. Its fields
 * are inlined here. If a second pack ever references ExtraOptionsInfo on the
 * wire, promote to vcmi/server/json/shared/ExtraOptionsInfo.{h,cpp}.
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForLobby.h"
#include "../../../lib/ExtraOptionsInfo.h"

class LobbySetExtraOptionsCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbySetExtraOptions"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbySetExtraOptions *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbySetExtraOptions>();
		const JsonNode & e = json["extraOptionsInfo"];
		auto & info = pack->extraOptionsInfo;
		if (e["cheatsAllowed"].isBool())
			info.cheatsAllowed = e["cheatsAllowed"].Bool();
		if (e["unlimitedReplay"].isBool())
			info.unlimitedReplay = e["unlimitedReplay"].Bool();
		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbySetExtraOptions &>(pack);
		const auto & info = p.extraOptionsInfo;

		out["type"].String() = typeName();

		JsonNode & e = out["extraOptionsInfo"];
		e.Struct();
		e["cheatsAllowed"].Bool() = info.cheatsAllowed;
		e["unlimitedReplay"].Bool() = info.unlimitedReplay;
	}
};

REGISTER_PACK_CODEC(LobbySetExtraOptionsCodec)

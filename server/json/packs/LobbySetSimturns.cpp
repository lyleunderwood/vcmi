/*
 * LobbySetSimturns.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbySetSimturns.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbySetSimturns.ts
 *
 * NOTE: SimturnsInfo is not (yet) promoted to a shared codec. Its fields
 * are inlined here. If a second pack ever references SimturnsInfo on the
 * wire, promote to vcmi/server/json/shared/SimturnsInfo.{h,cpp}.
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForLobby.h"
#include "../../../lib/StartInfo.h"

class LobbySetSimturnsCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbySetSimturns"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbySetSimturns *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbySetSimturns>();
		const JsonNode & s = json["simturnsInfo"];
		auto & info = pack->simturnsInfo;
		if (s["requiredTurns"].isNumber())
			info.requiredTurns = static_cast<int>(s["requiredTurns"].Integer());
		if (s["optionalTurns"].isNumber())
			info.optionalTurns = static_cast<int>(s["optionalTurns"].Integer());
		if (s["allowHumanWithAI"].isBool())
			info.allowHumanWithAI = s["allowHumanWithAI"].Bool();
		if (s["ignoreAlliedContacts"].isBool())
			info.ignoreAlliedContacts = s["ignoreAlliedContacts"].Bool();
		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbySetSimturns &>(pack);
		const auto & info = p.simturnsInfo;

		out["type"].String() = typeName();

		JsonNode & s = out["simturnsInfo"];
		s.Struct();
		s["requiredTurns"].Integer() = static_cast<int64_t>(info.requiredTurns);
		s["optionalTurns"].Integer() = static_cast<int64_t>(info.optionalTurns);
		s["allowHumanWithAI"].Bool() = info.allowHumanWithAI;
		s["ignoreAlliedContacts"].Bool() = info.ignoreAlliedContacts;
	}
};

REGISTER_PACK_CODEC(LobbySetSimturnsCodec)

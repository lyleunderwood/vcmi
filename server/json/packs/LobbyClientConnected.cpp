/*
 * LobbyClientConnected.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbyClientConnected. Canonical example for new pack
 * codec implementations.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbyClientConnected.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForLobby.h"
#include "../../../lib/StartInfo.h"

namespace
{
std::string modeToString(EStartMode m)
{
	switch (m)
	{
		case EStartMode::NEW_GAME: return "NEW_GAME";
		case EStartMode::LOAD_GAME: return "LOAD_GAME";
		case EStartMode::CAMPAIGN: return "CAMPAIGN";
		default: return "INVALID";
	}
}

EStartMode modeFromString(const std::string & s)
{
	if (s == "NEW_GAME") return EStartMode::NEW_GAME;
	if (s == "LOAD_GAME") return EStartMode::LOAD_GAME;
	if (s == "CAMPAIGN") return EStartMode::CAMPAIGN;
	return EStartMode::INVALID;
}
} // namespace

class LobbyClientConnectedCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbyClientConnected"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbyClientConnected *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbyClientConnected>();

		if (json["uuid"].isString())
			pack->uuid = json["uuid"].String();
		else
			pack->uuid = "json-client";

		if (json["names"].isVector())
			for (const auto & n : json["names"].Vector())
				pack->names.push_back(n.String());
		if (pack->names.empty())
			pack->names.push_back("JsonPlayer");

		pack->mode = json["mode"].isString()
			? modeFromString(json["mode"].String())
			: EStartMode::NEW_GAME;

		// clientId, hostClientId, version are server-assigned; ignored on inbound.
		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbyClientConnected &>(pack);

		out["type"].String() = typeName();
		out["uuid"].String() = p.uuid;

		JsonNode & names = out["names"];
		names.Vector(); // ensure vector type
		for (const auto & n : p.names)
		{
			JsonNode v;
			v.String() = n;
			names.Vector().push_back(v);
		}

		out["mode"].String() = modeToString(p.mode);
		out["clientId"].Integer() = static_cast<int64_t>(p.clientId);
		out["hostClientId"].Integer() = static_cast<int64_t>(p.hostClientId);
		out["version"].Integer() = static_cast<int64_t>(p.version);
	}
};

REGISTER_PACK_CODEC(LobbyClientConnectedCodec)

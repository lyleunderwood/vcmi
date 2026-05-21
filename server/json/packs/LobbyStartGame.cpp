/*
 * LobbyStartGame.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbyStartGame.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbyStartGame.ts
 *
 * `initializedStartInfo` and `initializedGameState` use the shared opaque
 * helpers from shared/StartInfo.h. See that header for the phase-1
 * lossiness note.
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/StartInfo.h"

#include "../../../lib/networkPacks/PacksForLobby.h"

class LobbyStartGameCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "LobbyStartGame"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const LobbyStartGame *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<LobbyStartGame>();
		pack->initializedStartInfo = homamweb::shared::opaqueStartInfoFromJson(json["initializedStartInfo"]);
		pack->initializedGameState = homamweb::shared::opaqueGameStateFromJson(json["initializedGameState"]);
		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbyStartGame &>(pack);

		out["type"].String() = typeName();

		if (auto si = homamweb::shared::opaqueStartInfoToJson(p.initializedStartInfo))
			out["initializedStartInfo"] = *si;
		if (auto gs = homamweb::shared::opaqueGameStateToJson(p.initializedGameState))
			out["initializedGameState"] = *gs;
	}
};

REGISTER_PACK_CODEC(LobbyStartGameCodec)

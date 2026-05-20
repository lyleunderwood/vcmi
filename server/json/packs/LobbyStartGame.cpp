/*
 * LobbyStartGame.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for LobbyStartGame.
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForLobby.h
 * TypeScript twin:      wrapper/src/codecs/lobby/LobbyStartGame.ts
 *
 * NOTE: `initializedStartInfo` (std::shared_ptr<StartInfo>) and
 * `initializedGameState` (std::shared_ptr<CGameState>) are complex VCMI
 * engine types whose full field models are not in scope for this codec.
 * They are treated as opaque JSON blobs on the wire: toJson emits an empty
 * struct placeholder when the pointer is non-null and omits the field when
 * the pointer is null; fromJson leaves the pointers as nullptr. A future
 * codec pass should promote StartInfo and CGameState to shared codecs.
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

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
		// initializedStartInfo and initializedGameState are opaque on the
		// wire; leave as nullptr. (Future work: deserialize from
		// json["initializedStartInfo"] / json["initializedGameState"].)
		(void)json;
		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const LobbyStartGame &>(pack);

		out["type"].String() = typeName();

		if (p.initializedStartInfo)
			out["initializedStartInfo"].Struct(); // opaque placeholder
		// else: leave field absent / null

		if (p.initializedGameState)
			out["initializedGameState"].Struct(); // opaque placeholder
		// else: leave field absent / null
	}
};

REGISTER_PACK_CODEC(LobbyStartGameCodec)

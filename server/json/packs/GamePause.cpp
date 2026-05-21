/*
 * GamePause.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for GamePause (a client -> server gameplay action).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForServer.h
 * TypeScript twin:      wrapper/src/codecs/server/GamePause.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"
#include "../shared/CPackForServerBase.h"

#include "../../../lib/networkPacks/PacksForServer.h"

class GamePauseCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "GamePause"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const GamePause *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<GamePause>();
		homamweb::shared::readServerPackBase(json, *pack);
		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const GamePause &>(pack);
		out["type"].String() = typeName();
		homamweb::shared::writeServerPackBase(p, out);
	}
};

REGISTER_PACK_CODEC(GamePauseCodec)

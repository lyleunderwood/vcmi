/*
 * BattleUpdateGateState.cpp, part of homam-web fork of VCMI engine.
 *
 * JSON codec for BattleUpdateGateState (server -> client battle state update:
 * updates the state of the town gate during a siege battle).
 *
 * C++ pack definition:  vcmi/lib/networkPacks/PacksForClientBattle.h
 * TypeScript twin:      wrapper/src/codecs/battle/BattleUpdateGateState.ts
 */
#include "StdInc.h"

#include "../PackCodec.h"
#include "../PackCodecRegistry.h"

#include "../../../lib/networkPacks/PacksForClientBattle.h"
#include "../../../lib/constants/EntityIdentifiers.h"
#include "../../../lib/constants/Enumerations.h"

namespace
{
std::string gateStateToString(EGateState s)
{
	switch (s)
	{
		case EGateState::NONE: return "NONE";
		case EGateState::CLOSED: return "CLOSED";
		case EGateState::BLOCKED: return "BLOCKED";
		case EGateState::OPENED: return "OPENED";
		case EGateState::DESTROYED: return "DESTROYED";
		default: return "NONE";
	}
}

EGateState gateStateFromString(const std::string & s)
{
	if (s == "CLOSED") return EGateState::CLOSED;
	if (s == "BLOCKED") return EGateState::BLOCKED;
	if (s == "OPENED") return EGateState::OPENED;
	if (s == "DESTROYED") return EGateState::DESTROYED;
	return EGateState::NONE;
}
} // namespace

class BattleUpdateGateStateCodec final : public PackCodec
{
public:
	std::string typeName() const override { return "BattleUpdateGateState"; }

	bool matches(const CPack & pack) const override
	{
		return dynamic_cast<const BattleUpdateGateState *>(&pack) != nullptr;
	}

	std::unique_ptr<CPack> fromJson(const JsonNode & json) const override
	{
		auto pack = std::make_unique<BattleUpdateGateState>();

		if (json["battleID"].isNumber())
			pack->battleID = BattleID(static_cast<int32_t>(json["battleID"].Integer()));

		if (json["state"].isString())
			pack->state = gateStateFromString(json["state"].String());

		return pack;
	}

	void toJson(const CPack & pack, JsonNode & out) const override
	{
		const auto & p = dynamic_cast<const BattleUpdateGateState &>(pack);

		out["type"].String() = typeName();
		out["battleID"].Integer() = static_cast<int64_t>(p.battleID.getNum());
		out["state"].String() = gateStateToString(p.state);
	}
};

REGISTER_PACK_CODEC(BattleUpdateGateStateCodec)

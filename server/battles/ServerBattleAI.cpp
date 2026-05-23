/*
 * ServerBattleAI.cpp, part of homam-web fork of VCMI engine.
 *
 * See ServerBattleAI.h and docs/server-side-ai.md.
 */
#include "StdInc.h"
#include "ServerBattleAI.h"

#include "../CGameHandler.h"
#include "../ServerAdventureAI.h"

#include "../../lib/CStack.h"
#include "../../lib/battle/BattleAction.h"
#include "../../lib/battle/CBattleInfoCallback.h"
#include "../../lib/battle/IBattleState.h"
#include "../../lib/callback/CDynLibHandler.h"
#include "../../lib/callback/CBattleCallback.h"
#include "../../lib/callback/CBattleGameInterface.h"

#include <vcmi/Environment.h>

// A CBattleCallback that CAPTURES the AI's chosen action instead of sending it
// over a network (the base class would sendRequest via an IClient). The
// overridden battleMake* methods never touch the IClient, so it's safe to
// construct the base with a null client. The base class still provides full
// battle-state access to the AI via getBattle()/onBattleStarted().
class CapturingBattleCallback : public CBattleCallback
{
public:
	std::optional<BattleAction> captured;

	explicit CapturingBattleCallback(PlayerColor player)
		: CBattleCallback(player, nullptr)
	{}

	void battleMakeUnitAction(const BattleID &, const BattleAction & a) override { captured = a; }
	void battleMakeSpellAction(const BattleID &, const BattleAction & a) override { captured = a; }
	void battleMakeTacticAction(const BattleID &, const BattleAction & a) override { captured = a; }
};

ServerBattleAI::ServerBattleAI(CGameHandler * gameHandler)
	: gameHandler(gameHandler)
	, battleAIName("StupidAI")
{
}

ServerBattleAI::~ServerBattleAI() = default;

bool ServerBattleAI::shouldDrive(const CBattleInfoCallback & battle, const CStack * stack) const
{
	return shouldDrivePlayer(stack->unitOwner());
}

bool ServerBattleAI::shouldDrivePlayer(PlayerColor player) const
{
	// driveAllSides is a direct override (tests); battleAutoResolve is the
	// wrapper-toggled session preference. Either makes us drive every side.
	if (driveAllSides || gameHandler->battleAutoResolve)
		return true;
	if (player == PlayerColor::NEUTRAL)
		return true;
	// homam-web fork: a player whose adventure turn is hosted in-process
	// (ServerAdventureAI) has no client to run a battle AI, so the server plays
	// its battle stacks too — leaving only real human players to play manually.
	if (gameHandler->adventureAI && gameHandler->adventureAI->isDriven(player))
		return true;
	return false;
}

ServerBattleAI::PlayerAI & ServerBattleAI::getOrCreate(const CBattleInfoCallback & battle, PlayerColor owner)
{
	const BattleID battleID = battle.getBattle()->getBattleID();
	auto & perBattle = cache[battleID];

	auto it = perBattle.find(owner);
	if (it != perBattle.end())
		return it->second;

	PlayerAI entry;
	entry.cb = std::make_shared<CapturingBattleCallback>(owner);
	entry.cb->onBattleStarted(battle.getBattle());

	entry.ai = CDynLibHandler::getNewBattleAI(battleAIName);
	if (entry.ai)
	{
		// StupidAI/BattleAI make no env-> calls of consequence; CGameHandler IS
		// an Environment, handed over via a non-owning aliasing shared_ptr.
		std::shared_ptr<Environment> env(std::shared_ptr<void>(), static_cast<Environment*>(gameHandler));
		entry.ai->initBattleInterface(env, entry.cb);
	}
	else
	{
		logGlobal->error("[ServerBattleAI] failed to load battle AI '%s'", battleAIName);
	}

	auto result = perBattle.emplace(owner, std::move(entry));
	return result.first->second;
}

std::optional<BattleAction> ServerBattleAI::computeAction(const CBattleInfoCallback & battle, const CStack * stack)
{
	const BattleID battleID = battle.getBattle()->getBattleID();
	PlayerAI & entry = getOrCreate(battle, stack->unitOwner());
	if (!entry.ai)
		return std::nullopt;

	entry.cb->captured.reset();
	entry.ai->activeStack(battleID, stack);

	if (!entry.cb->captured)
	{
		logGlobal->warn("[ServerBattleAI] AI produced no action for stack %d (player %d)",
			stack->unitId(), stack->unitOwner().getNum());
		return std::nullopt;
	}
	return entry.cb->captured;
}

void ServerBattleAI::onBattleEnded(const BattleID & battleID)
{
	cache.erase(battleID);
}

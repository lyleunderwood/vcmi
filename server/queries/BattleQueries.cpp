/*
 * BattleQueries.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleQueries.h"
#include "MapQueries.h"
#include "QueriesProcessor.h"

#include "../CGameHandler.h"
#include "../battles/BattleProcessor.h"

#include "../../lib/battle/IBattleState.h"
#include "../../lib/battle/BattleLayout.h"
#include "../../lib/battle/SideInBattle.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/mapObjects/CGObjectInstance.h"
#include "../../lib/networkPacks/PacksForServer.h"

void CBattleQuery::notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero) const
{
	// homam-web fork (#302): aborted = rollback path. The battle was cancelled
	// without a result; nothing to apply to the visited object. The visit query
	// above us still pops itself cleanly via its own popIfTop.
	if(aborted)
		return;

	assert(result);

	if(result)
		visitedObject->battleFinished(*gh, visitingHero, *result);
}

CBattleQuery::CBattleQuery(CGameHandler * owner, const IBattleInfo * bi):
	CQuery(owner),
	battleID(bi->getBattleID())
{
	belligerents[BattleSide::ATTACKER] = bi->getSideArmy(BattleSide::ATTACKER);
	belligerents[BattleSide::DEFENDER] = bi->getSideArmy(BattleSide::DEFENDER);

	addPlayer(bi->getSidePlayer(BattleSide::ATTACKER));
	addPlayer(bi->getSidePlayer(BattleSide::DEFENDER));
}

CBattleQuery::CBattleQuery(CGameHandler * owner):
	CQuery(owner)
{
	belligerents[BattleSide::ATTACKER] = nullptr;
	belligerents[BattleSide::DEFENDER] = nullptr;
}

bool CBattleQuery::blocksPack(const CPackForServer * pack) const
{
	if(dynamic_cast<const MakeAction*>(pack) != nullptr)
		return false;

	if(dynamic_cast<const GamePause*>(pack) != nullptr)
		return false;

	// homam-web fork: SaveGame is read-only — allow it mid-battle so an
	// async game can be persisted while a battle is paused awaiting a player.
	if(dynamic_cast<const SaveGame*>(pack) != nullptr)
		return false;

	// homam-web fork: AdvInterfaceReady is a UI-readiness signal (sets
	// uiReadyForDialogs, kicks the top query) — harmless mid-battle. Allowing it
	// matters when a loaded battle auto-resumes during the load handshake: a
	// non-participant player's ready pack arrives after the battle query exists
	// and would otherwise be rejected. See JsonAdapter::resumeOrphanedBattles.
	if(dynamic_cast<const AdvInterfaceReady*>(pack) != nullptr)
		return false;

	return true;
}

void CBattleQuery::onRemoval(PlayerColor color)
{
	// homam-web fork (#302): aborted path — battle was cancelled by
	// WrapperRollbackLiveBattle. Skip battleFinalize (no casualties / exp /
	// artifacts to apply); the rollback caller already erased BattleInfo from
	// gs->currentBattles and re-queued a PendingBattle.
	if(aborted)
		return;

	assert(result);

	if(result)
		gh->battles->battleFinalize(battleID, *result);
}

void CBattleQuery::onExposure(QueryPtr topQuery)
{
	// this method may be called in two cases:
	// 1) when requesting battle replay (but before replay starts -> no valid result)
	// 2) when aswering on levelup queries after accepting battle result -> valid result
	if(result)
		owner->popQuery(*this);
}

CBattleDialogQuery::CBattleDialogQuery(CGameHandler * owner, const IBattleInfo * bi, const std::optional<BattleResult> & Br):
	CDialogQuery(owner),
	bi(bi),
	result(Br)
{
	addPlayer(bi->getSidePlayer(BattleSide::ATTACKER));
	addPlayer(bi->getSidePlayer(BattleSide::DEFENDER));
}

void CBattleDialogQuery::onRemoval(PlayerColor color)
{
	// answer to this query was already processed when handling 1st player
	// this removal call for 2nd player which can be safely ignored
	if (resultProcessed)
		return;

	assert(answer);
	if(*answer == 1)
	{
		gh->battles->restartBattle(
			bi->getBattleID(),
			bi->getSideArmy(BattleSide::ATTACKER),
			bi->getSideArmy(BattleSide::DEFENDER),
			bi->getLocation(),
			bi->getSideHero(BattleSide::ATTACKER),
			bi->getSideHero(BattleSide::DEFENDER),
			bi->getLayout(),
			bi->getDefendedTown()
		);
	}
	else
	{
		gh->battles->endBattleConfirm(bi->getBattleID());
	}
	resultProcessed = true;
}

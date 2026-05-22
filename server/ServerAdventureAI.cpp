/*
 * ServerAdventureAI.cpp, part of VCMI engine
 *
 * homam-web fork — see ServerAdventureAI.h.
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"
#include "ServerAdventureAI.h"

#include "CGameHandler.h"
#include "IGameServer.h"

#include "../lib/callback/CCallback.h"
#include "../lib/callback/CGlobalAI.h"
#include "../lib/callback/CDynLibHandler.h"
#include "../lib/gameState/CGameState.h"
#include "../lib/networkPacks/PacksForServer.h"
#include "../lib/networkPacks/PacksForClient.h"
#include "../lib/StartInfo.h"
#include "../lib/CPlayerState.h"
#include "../lib/battle/BattleAction.h"

#include <vcmi/Environment.h>

// EmptyAI = stable (just ends the turn). Nullkiller2 PLAYS (moves heroes,
// answers level-ups + blocking dialogs via the query routing in
// CGameHandler::showBlockingDialog/heroLevelUp, visits & picks up objects) but
// its turn still doesn't COMPLETE: after a HeroVisit/RemoveObject/PlayerBlocked
// it hangs — NK2's status.waitTillFree() blocks on ongoingHeroMovement /
// objectsBeingVisited, which only clear via IGameEventsReceiver callbacks
// (heroMoved/heroVisit/...) we don't yet forward to the hosted AI (event
// forwarding, the remaining Phase 3b work), and its hero's battles need
// interaction coordination with ServerBattleAI. Flip to "Nullkiller2" to
// resume. See docs/server-side-ai.md Phase 3b.
static const std::string SERVER_ADVENTURE_AI = "EmptyAI";

std::optional<BattleAction> ServerAiClient::makeSurrenderRetreatDecision(PlayerColor, const BattleID &, const BattleStateInfoForRetreat &)
{
	return std::nullopt; // server-hosted AI never surrenders/retreats; ServerBattleAI fights the battle
}

int ServerAiClient::sendRequest(const CPackForServer & request, PlayerColor player, bool /*waitTillRealize*/)
{
	const int requestID = requestCounter++;
	// requestID/player are mutable on CPackForServer (CClient::sendRequest sets
	// them on a const ref too); handleReceivedPack takes a non-const ref but
	// does not retain it.
	auto & pack = const_cast<CPackForServer &>(request);
	pack.requestID = requestID;
	pack.player = player;

	const GameConnectionID conn = gameHandler->gameServer().getConnectionForPlayer(player);
	if(conn == GameConnectionID::INVALID)
	{
		logGlobal->error("[ServerAdventureAI] no owning connection for player %s; dropping AI command", player.toString());
		return requestID;
	}

	gameHandler->handleReceivedPack(conn, pack);
	return requestID;
}

ServerAdventureAI::ServerAdventureAI(CGameHandler * gh)
	: gameHandler(gh)
	, client(std::make_unique<ServerAiClient>(gh))
{
}

ServerAdventureAI::~ServerAdventureAI() = default;

void ServerAdventureAI::installForAiPlayers()
{
	for(const auto & pi : gameHandler->gameInfo().getStartInfo()->playerInfos)
	{
		const PlayerColor color = pi.first;
		const auto * state = gameHandler->gameInfo().getPlayerState(color, false);
		if(!state || state->isHuman())
			continue; // only AI players
		if(ais.count(color))
			continue; // already installed
		if(gameHandler->gameServer().getConnectionForPlayer(color) == GameConnectionID::INVALID)
			continue; // need a connection to route this player's command packs through

		auto ai = CDynLibHandler::getNewAI(SERVER_ADVENTURE_AI);
		auto cb = std::make_shared<CCallback>(gameHandler->gs, color, client.get());
		// CGameHandler IS an Environment (same aliasing-shared_ptr trick as
		// ServerBattleAI). Sufficient for EmptyAI; Nullkiller2 may want a
		// player-specific CPlayerEnvironment — revisit at Phase 3b-5.
		std::shared_ptr<Environment> env(std::shared_ptr<void>(), static_cast<Environment *>(gameHandler));
		ai->initGameInterface(env, cb);

		ais[color] = ai;
		callbacks[color] = cb;
		logGlobal->info("[ServerAdventureAI] hosting %s for player %s", SERVER_ADVENTURE_AI, color.toString());
	}
}

bool ServerAdventureAI::isDriven(PlayerColor player) const
{
	return ais.count(player) != 0;
}

void ServerAdventureAI::deliverRealized(const PackageApplied & pa)
{
	auto it = ais.find(pa.player);
	if(it != ais.end())
		it->second->requestRealized(const_cast<PackageApplied *>(&pa));
}

std::shared_ptr<CGlobalAI> ServerAdventureAI::aiFor(PlayerColor player) const
{
	auto it = ais.find(player);
	return it == ais.end() ? nullptr : it->second;
}

bool ServerAdventureAI::driveTurn(PlayerColor player, QueryID turnQuery)
{
	auto it = ais.find(player);
	if(it == ais.end())
		return false;

	// NON-BLOCKING: just kick off the AI's turn. EmptyAI ends it inline;
	// Nullkiller2 spawns makeTurn on a TBB worker and returns immediately. We do
	// NOT block the IO thread here — the worker's commands flow through
	// handleReceivedPack -> gameServer().sendPack, which needs the single
	// io_context thread free (parking it deadlocks the worker's network sends).
	// The worker advances the turn order itself when it ends its turn (its
	// EndTurn -> doEndPlayerTurn -> resumeTurnOrder -> next player's yourTurn,
	// also non-blocking), and deliverRealized() feeds back the PackageApplied so
	// NK2's endTurn-confirmation loop exits.
	it->second->yourTurn(turnQuery);
	return true;
}

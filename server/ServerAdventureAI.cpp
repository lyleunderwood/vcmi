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
#include "CVCMIServer.h"

#include "../lib/network/NetworkInterface.h"
#include "../lib/callback/CCallback.h"
#include "../lib/callback/CGlobalAI.h"
#include "../lib/callback/CDynLibHandler.h"
#include "../lib/gameState/CGameState.h"
#include "../lib/networkPacks/PacksForServer.h"
#include "../lib/networkPacks/PacksForClient.h"
#include "../lib/StartInfo.h"
#include "../lib/CPlayerState.h"
#include "../lib/battle/BattleAction.h"
#include "../lib/mapObjects/CGHeroInstance.h"

#include <vcmi/Environment.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/dispatch.hpp>
#include <future>

// Which adventure AI to host. EmptyAI = no-op (auto-pass). Nullkiller2 = real
// play. The AI's commands are applied on the io thread (see sendRequest) so the
// async NK2 worker doesn't deadlock on re-entrant callbacks.
static const std::string SERVER_ADVENTURE_AI = "Nullkiller2";

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

	// Mirror CClient::sendRequest: notify the AI synchronously, BEFORE applying.
	// NK2 uses requestSent to record a QueryReply's requestID->queryID mapping
	// (attemptedAnsweringQuery); without it the later PackageApplied confirmation
	// can't clear remainingQueries and status.waitTillFree() hangs forever.
	if(auto ai = gameHandler->adventureAI->aiFor(player))
		ai->requestSent(&pack, requestID);

	if(!ioResolved)
	{
		ioResolved = true;
		if(auto * srv = dynamic_cast<CVCMIServer *>(&gameHandler->gameServer()))
			ioContext = &srv->getNetworkHandler().getContext();
	}

	if(!ioContext)
	{
		// No io_context (shouldn't happen with CVCMIServer) — apply inline.
		gameHandler->handleReceivedPack(conn, pack);
		return requestID;
	}

	// Apply on the io thread, blocking this (possibly AI-worker) thread until
	// done. boost::asio::dispatch runs the lambda INLINE if we're already on the
	// io thread (EmptyAI's synchronous yourTurn, or any io-thread caller), and
	// POSTS it to the io thread otherwise (Nullkiller2's makeTurn worker). Either
	// way the apply + the IGameEventsReceiver event callbacks (onPackApplied) run
	// on the io thread — never re-entrantly inside the AI's makeTurn stack, which
	// is what deadlocked NK2 against its own async query-answer tasks. We block
	// until the apply completes so the AI sees the effect (matches waitTillRealize).
	std::promise<void> done;
	auto fut = done.get_future();
	boost::asio::dispatch(*ioContext, [this, conn, &pack, &done]() {
		gameHandler->handleReceivedPack(conn, pack);
		done.set_value();
	});
	fut.wait();
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

void ServerAdventureAI::onPackApplied(const CPackForClient & pack)
{
	if(ais.empty())
		return;

	// NOTE: we deliberately do NOT forward PlayerBlocked. The server only sends it
	// with reason UPCOMING_BATTLE, which would set NK2's `battle` flag and gate
	// status.waitTillFree() until a matching battleStart/battleEnd cycle clears it.
	// But the hosted AI never plays its own battles — ServerBattleAI resolves them
	// synchronously inside the triggering move's apply (the battle is over before
	// sendRequest returns), so makeTurn can proceed immediately and must not block
	// waiting on a battle it isn't driving. Forwarding it would hang the turn.

	// HeroVisit start/end: push/pop NK2's objectsBeingVisited. objId is only
	// meaningful on the START pack — heroVisit(end) just pops, so null is fine.
	if(const auto * hv = dynamic_cast<const HeroVisit *>(&pack))
	{
		if(auto ai = aiFor(hv->player))
		{
			const auto * hero = gameHandler->gs->getHero(hv->heroId);
			const auto * obj = hv->starting ? gameHandler->gs->getObjInstance(hv->objId) : nullptr;
			ai->heroVisit(hero, obj, hv->starting);
		}
		return;
	}

	// TryMoveHero: NK2 invalidates its pathfinder cache + tracks teleports/boats.
	if(const auto * tmh = dynamic_cast<const TryMoveHero *>(&pack))
	{
		const auto * hero = gameHandler->gs->getHero(tmh->id);
		if(hero)
			if(auto ai = aiFor(hero->getOwner()))
				ai->heroMoved(*tmh, true);
		return;
	}
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

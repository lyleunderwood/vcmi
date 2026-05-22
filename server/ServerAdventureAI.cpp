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
#include "../lib/StartInfo.h"
#include "../lib/CPlayerState.h"
#include "../lib/battle/BattleAction.h"

#include <vcmi/Environment.h>

// Which adventure AI to host. EmptyAI = wiring spike (just ends the turn +
// auto-answers queries). Swap to "Nullkiller2" once the worker-thread/lock
// coordination (Phase 3b-5) is in place.
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
	it->second->yourTurn(turnQuery);
	return true;
}

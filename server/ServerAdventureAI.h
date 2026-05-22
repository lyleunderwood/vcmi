/*
 * ServerAdventureAI.h, part of VCMI engine
 *
 * homam-web fork: host VCMI's adventure AI (Nullkiller2 / EmptyAI) INSIDE the
 * headless server, so AI players actually play instead of just auto-passing.
 *
 * VCMI normally delegates adventure AI to clients; a headless server runs
 * none. This module installs an AI per AI player and drives its turn. The AI
 * issues commands through a CCallback bound to a server-side IClient
 * (ServerAiClient) that forwards each pack straight to
 * CGameHandler::handleReceivedPack — the same path incoming client packs take.
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#pragma once

#include "../lib/callback/IClient.h"
#include "../lib/constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN
class CGlobalAI;
class CCallback;
VCMI_LIB_NAMESPACE_END

class CGameHandler;

/// Server-side IClient: routes a hosted AI's commands into the game handler
/// instead of over the network. The single seam between AI and engine.
class ServerAiClient : public IClient
{
public:
	explicit ServerAiClient(CGameHandler * gh) : gameHandler(gh) {}
	int sendRequest(const CPackForServer & request, PlayerColor player, bool waitTillRealize) override;
	std::optional<BattleAction> makeSurrenderRetreatDecision(PlayerColor player, const BattleID & battleID, const BattleStateInfoForRetreat & battleState) override;

private:
	CGameHandler * gameHandler;
	int requestCounter = 1;
};

class ServerAdventureAI
{
public:
	explicit ServerAdventureAI(CGameHandler * gh);
	~ServerAdventureAI();

	/// Install a hosted AI for every non-human player that has an owning
	/// connection. Call once at game start.
	void installForAiPlayers();

	/// True if a hosted AI drives this player.
	bool isDriven(PlayerColor player) const;

	/// Drive the player's adventure turn (calls the AI's yourTurn). Returns
	/// false if no AI is installed for the player (caller should fall back to
	/// the bare auto-pass). NOTE: with EmptyAI this is synchronous; Nullkiller2
	/// runs makeTurn on a worker thread (Phase 3b-5 will handle the wait).
	bool driveTurn(PlayerColor player, QueryID turnQuery);

	/// The hosted AI for a player (for routing query callbacks), or nullptr.
	std::shared_ptr<CGlobalAI> aiFor(PlayerColor player) const;

private:
	CGameHandler * gameHandler;
	std::unique_ptr<ServerAiClient> client;
	std::map<PlayerColor, std::shared_ptr<CGlobalAI>> ais;
	std::map<PlayerColor, std::shared_ptr<CCallback>> callbacks;
};

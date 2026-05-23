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

#include <condition_variable>
#include <mutex>

namespace boost::asio { class io_context; }

VCMI_LIB_NAMESPACE_BEGIN
class CGlobalAI;
class CCallback;
struct PackageApplied;
struct CPackForClient;
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
	// The server's single io_context. A hosted async AI (Nullkiller2) runs
	// makeTurn on a TBB worker thread; we apply its commands ON the io thread
	// (boost::asio::dispatch) and block the worker until done, so apply + event
	// callbacks never fire re-entrantly inside makeTurn (which deadlocks NK2).
	// Resolved lazily; null => fall back to a synchronous apply.
	boost::asio::io_context * ioContext = nullptr;
	bool ioResolved = false;
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

	/// Deliver a PackageApplied confirmation to the hosted AI (if any) so it
	/// learns its command was applied — NK2 needs the EndTurn confirmation
	/// (requestRealized -> status.madeTurn) to stop re-requesting end-of-turn.
	void deliverRealized(const PackageApplied & pa);

	/// Forward an applied client-pack to the hosted AIs as the matching
	/// IGameEventsReceiver event (heroMoved/heroVisit/playerBlocked). NK2 tracks
	/// ongoingHeroMovement/objectsBeingVisited/battle from these and blocks its
	/// turn (status.waitTillFree) until they clear. Safe now that the AI's
	/// commands apply on the io thread, so these fire there too (not re-entrant).
	void onPackApplied(const CPackForClient & pack);

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

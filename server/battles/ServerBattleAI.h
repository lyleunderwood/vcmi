/*
 * ServerBattleAI.h, part of homam-web fork of VCMI engine.
 *
 * Server-side battle AI driver. Lets a headless server play stacks that no
 * client drives (neutral wandering monsters, and optionally any side under a
 * full auto-resolve policy). Without this, battles involving neutral stacks
 * stall forever — the neutral stack becomes active and nothing acts for it.
 *
 * See docs/server-side-ai.md for the why and the design.
 *
 * Usage (from BattleFlowProcessor):
 *   - shouldDrive(battle, stack)  -> is this stack server-driven?
 *   - computeAction(battle, stack) -> run the AI, return its chosen BattleAction
 *     (the caller applies it via the normal makeAutomaticAction path)
 *   - onBattleEnded(battleID)     -> drop cached AI state for the battle
 */
#pragma once

#include "../../lib/constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN
class CStack;
class BattleAction;
class BattleID;
class CBattleInfoCallback;
class CBattleGameInterface;
VCMI_LIB_NAMESPACE_END

class CGameHandler;
class CapturingBattleCallback;

class ServerBattleAI
{
	struct PlayerAI
	{
		std::shared_ptr<CBattleGameInterface> ai;
		std::shared_ptr<CapturingBattleCallback> cb;
	};

	CGameHandler * gameHandler;

	// Cached AI + callback per (battleID, owning player). Built lazily on first
	// activation and reused for the rest of the battle; cleared on battle end.
	std::map<BattleID, std::map<PlayerColor, PlayerAI>> cache;

	// Name of the battle-AI library to load for server-driven sides.
	std::string battleAIName;

	// Policy: when false, drive only NEUTRAL stacks (wandering monsters) and
	// leave client-controlled sides to their connection. When true, drive
	// every side (full server-side auto-resolve).
	bool driveAllSides = false;

	PlayerAI & getOrCreate(const CBattleInfoCallback & battle, PlayerColor owner);

public:
	explicit ServerBattleAI(CGameHandler * gameHandler);
	~ServerBattleAI();

	void setDriveAllSides(bool value) { driveAllSides = value; }
	void setBattleAIName(const std::string & name) { battleAIName = name; }

	/// True if the server should play this stack (no client will).
	bool shouldDrive(const CBattleInfoCallback & battle, const CStack * stack) const;

	/// True if the server should drive this player's side (used for the
	/// tactics phase, which is per-side not per-stack).
	bool shouldDrivePlayer(PlayerColor player) const;

	/// Run the AI for one activation and return the action it chose.
	/// std::nullopt if the AI produced nothing (caller should defend).
	std::optional<BattleAction> computeAction(const CBattleInfoCallback & battle, const CStack * stack);

    /// Drop cached AI state for a finished battle.
	void onBattleEnded(const BattleID & battleID);
};

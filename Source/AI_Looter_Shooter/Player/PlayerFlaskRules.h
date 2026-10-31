#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"

/** Why the flask's count changed, or why a press was refused (UPlayerFlaskComponent::OnFlaskChanged), so the HUD can say so. */
enum class EFlaskChange : uint8
{
	/** A press started a drink: one charge went (Delta -1). */
	Drunk,
	/** Soul completed a charge (Delta +1, once for each charge filled). */
	Filled,
	/** Soul went in without completing a charge (Delta 0): the next pip's level rose. */
	Soul,
	/** Every charge put back at once (a respawn; Delta what came back). */
	Refilled,
	/** Put back from a saved session, or a fresh body's full flask (no fanfare). */
	Restored,
	/** Pressed with no charge left (Delta 0): the HUD's pips flash red and shake. */
	DeniedEmpty,
	/** Pressed at full health (Delta 0): the HUD says "Health full". */
	DeniedFull
};

/** Why a press can't start a drink right now (FFlaskRules::WhyBlocked: weighed Dead, Scene, Menu, Moving, Busy, Empty, FullHealth). */
enum class EFlaskBlock : uint8
{
	None,
	/** No charge left. */
	Empty,
	/** Health is full (within half a point, as the soul-motes count it). */
	FullHealth,
	/** The hands are taken: a drink running or just ended (RepeatSeconds), a throw, a strike, an ember power's cast, a swim. */
	Busy,
	/** Dead, or no health to heal. */
	Dead,
	/** A scene plays or holds the player. */
	Scene,
	/** A menu covers the game, or the game is paused. */
	Menu,
	/** A mantle or a vault carries the body. */
	Moving
};

/** What a drink's start depends on, gathered by UPlayerFlaskComponent at each press (plain data the tests set). */
struct FFlaskGateInput
{
	/** The world's seconds now, and when the last drink ended (far in the past before the first). */
	double Now = 0.0;
	double LastDrinkEnd = -1000.0;
	bool bDrinking = false;
	bool bAlive = true;
	bool bInScene = false;
	/** A menu is open or the game is paused. */
	bool bMenuOpen = false;
	/** A mantle or a vault carries the body (UPlayerLocomotionComponent::IsTraversing). */
	bool bTraversing = false;
	/** Something else has the hands: a throw, a strike, an ember power's cast, a swim's strokes. */
	bool bHandsBusy = false;
	int32 Charges = 0;
	/** Health is at its most (within half a point). */
	bool bFullHealth = false;
};

/**
 * The soul flask (Docs/Archive/Work/2026-10-30-SoulFlask/CONTRACT.md, the user's playtest note 2: "add a way to heal during
 * combat"), as plain numbers with no engine state: UPlayerFlaskComponent plays them, and the tests drive them. Every number
 * of the design lives here and only here.
 *
 *  - Three charges, carried from the start of every game, full again at each respawn.
 *  - A drink: 1.1 s from the press; the heal starts 0.35 s in and gives 35% of the most health over 0.8 s (a gulp, not a
 *    switch); the player walks at 0.75 meanwhile; the next press works 0.4 s after a drink ends. Once started it finishes.
 *  - Soul refills it: 100 a charge, from kills by rank (KillSoul), a boss's whole flask, at least 25 for any kill in a boss
 *    fight, and 34 from a soul-mote touched at full health. Overflow carries into the next charge; a full flask keeps none.
 */
struct AI_LOOTER_SHOOTER_API FFlaskRules
{
	static constexpr int32 MaxCharges = 3;
	static constexpr float SoulPerCharge = 100.f;
	static constexpr float DrinkSeconds = 1.1f;
	static constexpr float SipAt = 0.35f;
	static constexpr float HealSeconds = 0.8f;
	static constexpr float HealShare = 0.35f;
	static constexpr float RepeatSeconds = 0.4f;
	static constexpr float MoveScale = 0.75f;
	static constexpr float MoteSoul = 34.f;
	static constexpr float BossFightKillSoul = 25.f;

	/** A boss's kill: the whole flask. */
	static constexpr float BossKillSoul = SoulPerCharge * MaxCharges;

	/** Soul one kill gives: Basic 15, Restless 30, Gravebound 45, Hallowed and Ethereal 60, Soulfed 100, a boss 300; in a boss fight at least 25. */
	static float KillSoul(ECreatureRank Rank, bool bBoss, bool bBossFight);

	/** Share of the drink's heal given by Seconds into the drink (0 before SipAt, 1 from SipAt + HealSeconds; smooth). */
	static float HealShareAt(float Seconds);

	/** The heal's last moment (seconds into the drink): SipAt + HealSeconds, a touch past the drink's own end. */
	static constexpr float HealEndSeconds = SipAt + HealSeconds;

	/**
	 * Soul poured into a flask holding Charges full charges and Soul toward the next: InOutCharges and InOutSoul after it
	 * (overflow carried on, nothing kept once all are full). Returns the charges it completed.
	 */
	static int32 Pour(int32& InOutCharges, float& InOutSoul, float Added);

	/** Why a press now starts no drink (None: it does). */
	static EFlaskBlock WhyBlocked(const FFlaskGateInput& In);
};

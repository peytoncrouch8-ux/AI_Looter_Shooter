#pragma once

#include "CoreMinimal.h"
#include "Audio/LooterSoundCues.h"
#include "Creatures/CreatureRank.h"

/**
 * The grave-salt grenade's sound cues (Throw, Bounce, Fuse, Burst, Sear, Pickup): the Throwable section of
 * Audio/LooterSoundCues.h, so the names live in one place with the rest.
 */
namespace ThrowCue = LooterSoundCue::Throwable;

/** Why a throw can't start right now (FThrowRules::WhyBlocked), weighed in this order. */
enum class EThrowBlock : uint8
{
	None,
	/** No living player in control of the pawn (dead, or not possessed by a local player). */
	NoPlayer,
	/** A scene plays or holds the player. */
	Scene,
	/** A menu covers the game, or the game is paused. */
	Menu,
	/** A mantle or a vault carries the body (UPlayerLocomotionComponent::IsTraversing). */
	Traversing,
	/** A melee strike is swinging: the hands are on the gun. */
	Busy,
	/** The last throw is still going, or its cooldown hasn't run out. */
	Cooldown,
	/** No grenade left to throw. */
	Empty
};

/** What a throw's start depends on, gathered by UPlayerThrowComponent at each press (plain data the tests set). */
struct FThrowGateInput
{
	/** The world's seconds now, and when the last throw started (far in the past before the first). */
	double Now = 0.0;
	double LastThrow = -1000.0;
	bool bThrowing = false;
	bool bAlive = true;
	bool bTraversing = false;
	bool bInScene = false;
	/** A menu is open or the game is paused. */
	bool bMenuOpen = false;
	/** A melee strike is swinging (UPlayerMeleeComponent::IsSwinging). */
	bool bMeleeing = false;
	/** Grenades carried. */
	int32 Grenades = 0;
};

/**
 * The grave-salt grenade's throw (Docs/Polish/BorderlandsComparison.md, item 16: the crowd answer and the panic button),
 * as plain numbers with no engine state: UPlayerThrowComponent plays them, and the tests drive them. The burst's numbers
 * are FGraveSaltRules' (Combat/GraveSaltRules.h).
 *
 *  - The throw: 0.5 s from the press, the jar leaving the hand 0.16 s in; another can start 0.8 s after the last one did,
 *    so the three a player can carry go in under two seconds when it's all going wrong.
 *  - The toss: 15 m/s along the look, lifted 8 degrees (an underarm lob reads as a throw, not a shot), with half the
 *    thrower's own speed on top, so a throw on the run lands ahead of them: a level throw from the eye comes down
 *    about 11 m out before it bounces; at 45 degrees it carries about 23 m.
 *  - The count: three at most; two once the tutorial puts the first gun in the player's hands (or the first one found).
 *  - Loot (the orchestrator's hook in Loot/LootDropComponent and Loot/ChestLoot): a kill drops one by rank (DropChance),
 *    twice as likely when the player has none, never when they're full.
 */
struct AI_LOOTER_SHOOTER_API FThrowRules
{
	// --- The throw (seconds) ---
	static constexpr float ThrowSeconds = 0.5f;
	static constexpr float ReleaseSeconds = 0.16f;
	static constexpr float CooldownSeconds = 0.8f;

	// --- The count ---
	static constexpr int32 MaxGrenades = 3;
	static constexpr int32 StartingGrenades = 2;

	/**
	 * Count plus up to Amount, held to MaxGrenades (and never under 0). OutTaken: how many of Amount went in (negative
	 * when Amount takes some away).
	 */
	static int32 Add(int32 Count, int32 Amount, int32* OutTaken = nullptr);

	// --- The toss ---
	static constexpr float ThrowSpeed = 1500.f;
	static constexpr float LiftDegrees = 8.f;
	/** The share of the thrower's own speed the jar keeps. */
	static constexpr float InheritShare = 0.5f;
	/**
	 * Where the jar leaves the hand, from the eye along the look, its right and its up (cm): low and to the left, the
	 * off hand's, since the right one keeps the gun.
	 */
	static constexpr float ReleaseForward = 30.f;
	static constexpr float ReleaseRight = -14.f;
	static constexpr float ReleaseUp = -12.f;

	/** The jar's launch along Look (any length), lifted LiftDegrees, with InheritShare of ThrowerVelocity. */
	static FVector LaunchVelocity(const FVector& Look, const FVector& ThrowerVelocity);

	/** Where the jar leaves the hand for an eye at Eye looking along Look (before any wall in the way). */
	static FVector ReleasePoint(const FVector& Eye, const FVector& Look);

	// --- Loot (for the orchestrator's drop hook) ---

	/** The chance a kill of Rank drops one grenade for a player carrying Carried: doubled at none, nothing when full. */
	static float DropChance(ECreatureRank Rank, int32 Carried);

	/** The chance a loot chest gives one (bStrongbox: the gang's Strongbox, always one). Nothing when full. */
	static float ChestChance(bool bStrongbox, int32 Carried);

	// --- The rules ---

	/** Why a press now starts no throw (None: it does). */
	static EThrowBlock WhyBlocked(const FThrowGateInput& In);
};

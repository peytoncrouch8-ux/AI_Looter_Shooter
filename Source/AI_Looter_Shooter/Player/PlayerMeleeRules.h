#pragma once

#include "CoreMinimal.h"
#include "Audio/LooterSoundCues.h"
#include "Creatures/CreatureRank.h"

/**
 * The melee strike's sound cues (Swing, Hit, HitFlesh, HitShell, HitGel): the Melee section of Audio/LooterSoundCues.h,
 * so the names live in one place with the rest.
 */
namespace MeleeCue = LooterSoundCue::Melee;

/** Why a strike can't start right now (FMeleeRules::WhyBlocked), weighed in this order. */
enum class EMeleeBlock : uint8
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
	/** The last strike is still swinging, or its cooldown hasn't run out. */
	Cooldown
};

/** What a strike's start depends on, gathered by UPlayerMeleeComponent at each press (plain data the tests set). */
struct FMeleeGateInput
{
	/** The world's seconds now, and when the last strike started (far in the past before the first). */
	double Now = 0.0;
	double LastStrike = -1000.0;
	bool bSwinging = false;
	bool bAlive = true;
	bool bTraversing = false;
	bool bInScene = false;
	/** A menu is open or the game is paused. */
	bool bMenuOpen = false;
};

/** Where a strike comes from: the player's eye, the way they look, and the height of their feet (world cm). */
struct FMeleeAim
{
	FVector Eye = FVector::ZeroVector;
	FVector Forward = FVector::ForwardVector;
	double FeetZ = 0.0;
};

/** A body as the strike sees it: an upright capsule (a creature's own, or the box round anything else). */
struct FMeleeBody
{
	FVector Center = FVector::ZeroVector;
	float HalfHeight = 50.f;
	float Radius = 30.f;
};

/** Where a strike meets a body it can reach. */
struct FMeleeContact
{
	/** The point it lands on: the body's near side, at about the striker's chest height (or the body's top, if lower). */
	FVector Point = FVector::ZeroVector;
	/** Across the ground from the striker's eye to the body's near side (cm). */
	float Distance = 0.f;
	/** How far off the strike's line the body's nearest edge is, across the ground (degrees). */
	float Angle = 0.f;
	/** Lower is the better target: nearer and more in line. */
	float Score = 0.f;
};

/**
 * The melee strike's rules (Docs/Polish/BorderlandsComparison.md, item 16), as plain numbers with no engine state:
 * UPlayerMeleeComponent plays them, and the tests drive them.
 *
 *  - The swing: 0.45 s from the press, the blow landing 0.12 s in; another can start 0.6 s after the last one did.
 *  - The reach: 1.7 m across the ground from the eye's line to the body's near side (2 m at full size, times the player's
 *    0.85), within a 50 degree cone; a body pressed against the player (35 cm) is hit anywhere in front.
 *  - The damage: 60 at level 1, a tenth of a Common rifle's 30-round magazine (20 a round), growing with the player's
 *    level as enemies and guns do (FLevelRules::EnemyScale), so a strike at equal levels always takes the same share: a
 *    Basic Unpaid (160) in 3, a Meadow Slime (120) in 2, a spider (300) in 5. +/-10% like every hit, never critical.
 *    That's 100 damage a second at most, half a rifle's: the strike buys space, it doesn't replace the gun.
 *  - The knockback: a hop away (330 cm/s out, 190 up: about 1.3 m for a full-size Basic creature), less for bigger
 *    bodies and tougher ranks, a quarter more for a light slime; none for a Soulfed monster or a boss (damage only).
 */
struct AI_LOOTER_SHOOTER_API FMeleeRules
{
	// --- The swing (seconds) ---
	static constexpr float SwingSeconds = 0.45f;
	static constexpr float ContactSeconds = 0.12f;
	static constexpr float CooldownSeconds = 0.6f;
	/** The swing's own clock holds this long as the blow lands (the striker's half of the hit-stop). */
	static constexpr float AttackerHitStop = 0.05f;

	// --- The reach ---
	/** At full size; the player's reach is this times their size (Reach()). */
	static constexpr float FullSizeReach = 200.f;
	static constexpr float HalfConeDegrees = 25.f;
	/** A body this close (cm, across the ground) is hit anywhere in front of the player, not only in the cone. */
	static constexpr float HugDistance = 35.f;
	/** How far above or below the way the player looks the blow can land (degrees); a body pressed close, HugPitchOff. */
	static constexpr float MaxPitchOff = 70.f;
	static constexpr float HugPitchOff = 85.f;
	/** The height the strike reaches: from this far under the feet to this far over the eye (cm). */
	static constexpr float ReachUnderFeet = 30.f;
	static constexpr float ReachOverEye = 50.f;
	/** The blow lands this far under the eye (about the chest), or on the body's top if that's lower. */
	static constexpr float StrikeUnderEye = 35.f;

	/** The player's reach: FullSizeReach times their size (170 cm). */
	static float Reach();

	// --- The damage ---
	static constexpr float BaseDamage = 60.f;

	/** One strike's damage: BaseDamage times LevelScale (the player's level's), at Roll (0-1) in the +/-10% range. */
	static float StrikeDamage(float LevelScale, float Roll);

	// --- The knockback ---
	static constexpr float KnockSpeed = 330.f;
	static constexpr float KnockLift = 190.f;
	/** Slimes are light: they go this much harder. */
	static constexpr float LightKnockScale = 1.25f;

	/**
	 * The launch a strike gives a creature of Size (its size scale) and Rank, struck along Away (only its way across the
	 * ground counts); bLight for a slime. Zero for a Soulfed monster or a boss, or with no way to go.
	 */
	static FVector KnockVelocity(const FVector& Away, float Size, ECreatureRank Rank, bool bLight);

	// --- The rules ---

	/** Why a press now starts no strike (None: it does). */
	static EMeleeBlock WhyBlocked(const FMeleeGateInput& In);

	/** Whether a strike from Aim reaches Body within Reach; if so, where and how well (OutContact). */
	static bool FindContact(const FMeleeAim& Aim, const FMeleeBody& Body, float Reach, FMeleeContact& OutContact);
};

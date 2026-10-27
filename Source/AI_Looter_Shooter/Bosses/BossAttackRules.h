#pragma once

#include "CoreMinimal.h"

/**
 * The bosses' shared attack rules (round 5, 2026-10-27; the user's pick from a boss-fight video: "boss attack rules and a
 * hitbox debug view"). The Gravemother, the Foreman and Abel each still choose among their own moves in their own order;
 * these rules sit over that choice so every fight reads fair:
 *  - no long idle gaps: a boss that has stood MaxIdle seconds free to attack without attacking readies a move it has (the
 *    cap), so it never just circles;
 *  - not too fast: a new attack starts no sooner than MinGap seconds after the last one ended (the recovery is the
 *    player's window, then a breath), whichever of its moves or moments it is;
 *  - never the same attack MaxRepeats + 1 times in a row: a third of the same is skipped for another, and a boss with
 *    nothing else in reach makes its run-breaker (the Gravemother's stomp, the Foreman's stamp, Abel's flare and buckshot);
 *  - an attack starts only when its reach and facing allow: its target within its distances, and in front of it for a
 *    frontal one (FrontDegrees each side; AimedDegrees for a lob or a shot it turns to aim through its wind-up);
 *  - each attack's hurt zones are live only in its window: grey through the wind-up (the tell), red while it can hurt,
 *    nothing in the recovery. Looter.Boss.HurtZones 1 draws them (Dev/BossHurtZoneDebug) so fairness can be tuned.
 * Every number is a cvar (Looter.Boss.Attack.*); Looter.Boss.Attack.Rules 0 puts every boss back on its own choice, as before.
 */

/** Where a boss attack is in its frame. */
enum class EBossAttackWindow : uint8
{
	None,
	/** Its tell: it can't hurt yet (drawn grey). */
	Windup,
	/** It can hurt now (drawn red). */
	Active,
	/** Its recovery: the player's window, nothing live. */
	Recovery,
};

/** How a hurt zone is shaped. */
enum class EBossHurtShape : uint8
{
	/** Round Center, Radius wide, Height tall (a quake's ring, a burst, a stomp). */
	Disc,
	/** A slice in front: from Center along Direction, Radius long, HalfAngle each side (a bite, a swing, a shot's cone). */
	Wedge,
	/** A band from Center to End, Radius each side (a furrow's line, a lunge's path). */
	Lane,
};

/** One place a boss's attack can hurt the player, as it stands this frame. */
struct AI_LOOTER_SHOOTER_API FBossHurtZone
{
	/** The attack it belongs to (FBossAttackLog's ids). */
	FName Attack;
	EBossHurtShape Shape = EBossHurtShape::Disc;
	EBossAttackWindow Window = EBossAttackWindow::Windup;
	/** Its middle on the ground (a disc), its apex (a wedge) or its start (a lane), world. */
	FVector Center = FVector::ZeroVector;
	/** A lane's end, world. */
	FVector End = FVector::ZeroVector;
	/** A wedge's way (flat). */
	FVector Direction = FVector::ForwardVector;
	/** Its reach (cm): a disc's or wedge's radius, a lane's half width. */
	float Radius = 0.f;
	/** A wedge's half angle (degrees). */
	float HalfAngle = 180.f;
	/** How tall it reaches over its ground (cm): drawn as its height. */
	float Height = 200.f;

	static FBossHurtZone Disc(FName InAttack, EBossAttackWindow InWindow, const FVector& InCenter, float InRadius, float InHeight = 200.f);
	static FBossHurtZone Wedge(FName InAttack, EBossAttackWindow InWindow, const FVector& InApex, const FVector& InDirection, float InRadius,
		float InHalfAngle, float InHeight = 200.f);
	static FBossHurtZone Lane(FName InAttack, EBossAttackWindow InWindow, const FVector& InStart, const FVector& InEnd, float InHalfWidth,
		float InHeight = 200.f);

	/** Whether a point (world) is inside it, flat, and within its height over its ground (the tests; a zone's fairness). */
	bool Contains(const FVector& Point) const;
};

/** One of a boss's attacks as the rules see it: where its target may be for it to start. Distances at the boss's size. */
struct AI_LOOTER_SHOOTER_API FBossAttackSpec
{
	FName Id;
	/** Its target this far off at least, and at most (cm, flat); MaxReach 0: any distance. */
	float MinReach = 0.f;
	float MaxReach = 0.f;
	/** Its target within this many degrees of its front, each side; 180: any way (a ring round it, a roar). */
	float FacingDegrees = 180.f;

	FBossAttackSpec() = default;
	FBossAttackSpec(FName InId, float InMinReach, float InMaxReach, float InFacingDegrees)
		: Id(InId), MinReach(InMinReach), MaxReach(InMaxReach), FacingDegrees(InFacingDegrees) {}
};

/**
 * A boss's memory of its attacks, which the rules read (one per boss: UBossComponent::AttackLog, reset as each fight starts
 * and starts over): the last attack and how many times in a row, whether one is under way, the time since the last ended,
 * and how long it has stood free to attack without attacking.
 */
struct AI_LOOTER_SHOOTER_API FBossAttackLog
{
	void Reset();
	/** Moves its clocks on. bIdle: the boss could attack now and isn't (fighting, free, nothing of its own under way). */
	void Advance(float DeltaSeconds, bool bIdle);
	/** An attack began (its wind-up), and the one under way is over (its recovery done). */
	void NoteStarted(FName Id);
	void NoteEnded();
	/** Its idle clock starts again: the boss was nudged out of an idle gap (so it isn't nudged every frame). */
	void NoteNudged() { Idle = 0.f; }

	bool IsAttacking() const { return bAttacking; }
	FName GetLast() const { return Last; }
	int32 GetStreak() const { return Streak; }
	int32 GetStarted() const { return Started; }
	float GetSinceEnd() const { return SinceEnd; }
	float GetIdle() const { return Idle; }

private:
	FName Last;
	int32 Streak = 0;
	int32 Started = 0;
	bool bAttacking = false;
	/** Since the last attack ended (s): long ago at the start, so the first attack is never held. */
	float SinceEnd = 1.0e4f;
	float Idle = 0.f;
};

namespace BossAttackRules
{
	/** The cvars' starting values (the tests read them). */
	inline constexpr float DefaultMinGap = 0.6f;
	inline constexpr float DefaultMaxIdle = 5.f;
	inline constexpr int32 DefaultMaxRepeats = 2;
	inline constexpr float DefaultFrontDegrees = 70.f;
	inline constexpr float DefaultAimedDegrees = 120.f;
	inline constexpr float DefaultActiveSeconds = 0.15f;

	/** The rules on (Looter.Boss.Attack.Rules 1); off, each boss chooses as it did before them. */
	AI_LOOTER_SHOOTER_API bool IsOn();
	/** Looter.Boss.Attack.MinGap, .MaxIdle, .MaxRepeats, .FrontDegrees, .AimedDegrees, .ActiveSeconds as they stand. */
	AI_LOOTER_SHOOTER_API float MinGap();
	AI_LOOTER_SHOOTER_API float MaxIdle();
	AI_LOOTER_SHOOTER_API int32 MaxRepeats();
	AI_LOOTER_SHOOTER_API float FrontDegrees();
	AI_LOOTER_SHOOTER_API float AimedDegrees();
	/** How long a one-frame blow (a bite, a slam) is drawn live after it lands: it lands in a frame, too short to see. */
	AI_LOOTER_SHOOTER_API float ActiveSeconds();

	// --- The rules on plain numbers (the tests) ---

	/** Distance (cm, flat) within the attack's reach. */
	AI_LOOTER_SHOOTER_API bool IsInReach(const FBossAttackSpec& Spec, float Distance);
	/** ToTarget within FacingDegrees of Forward (flat); right on top of it, or 180 and over, always. */
	AI_LOOTER_SHOOTER_API bool IsFacing(float FacingDegrees, const FVector& Forward, const FVector& ToTarget);
	/** Starting Id now would make more than MaxRepeats of it in a row (0 or less: no limit). */
	AI_LOOTER_SHOOTER_API bool WouldRepeat(FName Id, FName Last, int32 Streak, int32 InMaxRepeats);
	/** Nothing under way and the last one ended Gap seconds ago or more. */
	AI_LOOTER_SHOOTER_API bool IsGapOpen(float SinceEnd, bool bAttacking, float Gap);
	/** Idle this long is past the cap (Cap 0 or less: never). */
	AI_LOOTER_SHOOTER_API bool IsOverdue(float Idle, float Cap);
	/** Where an attack Time seconds in is: its wind-up, Active seconds after it, then its recovery (to Windup + Recovery). */
	AI_LOOTER_SHOOTER_API EBossAttackWindow WindowAt(float Time, float Windup, float Active, float Recovery);
	/** Degrees off a creature's front its own strike still lands at, from the dot it checks (the bite's 0.4: 66 degrees). */
	AI_LOOTER_SHOOTER_API float DegreesForDot(float Dot);

	// --- With the cvars, on a boss's log ---

	/** The rules forbid Id now (another of the same in a row); never while they're off. */
	AI_LOOTER_SHOOTER_API bool IsBlocked(FName Id, const FBossAttackLog& Log);
	/** The gap since its last attack is open (always while the rules are off). */
	AI_LOOTER_SHOOTER_API bool IsGapOpen(const FBossAttackLog& Log);
	/** Its idle gap is past the cap (never while the rules are off). */
	AI_LOOTER_SHOOTER_API bool IsOverdue(const FBossAttackLog& Log);
	/** Its target's place lets Spec start: in reach and in front as it asks (always while the rules are off). */
	AI_LOOTER_SHOOTER_API bool IsPlaced(const FBossAttackSpec& Spec, const FVector& Forward, const FVector& ToTarget);
	/** All of them: not blocked, the gap open, the target placed for it. */
	AI_LOOTER_SHOOTER_API bool MayStart(const FBossAttackSpec& Spec, const FBossAttackLog& Log, const FVector& Forward, const FVector& ToTarget);
}

#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/SlimeFightRules.h"
#include "SlimeCreature.generated.h"

/**
 * The meadow slime: a knee-high dome of green gel with a dark core, living in small groups. 120 health; the core is the
 * critical spot, and since it sits inside the gel a shot counts as a crit when its line runs on through the core (it
 * stops at the gel's surface, so the core's own hull is never hit first).
 *
 * It only moves by hopping, like a classic bouncy slime: it squashes down, springs up stretched, goes round at the top
 * of the hop and splats on landing, then wobbles back to shape. Its attack is a telegraphed deep squash and a leap at the
 * player that hurts on landing. Hurting one sets its whole group on the attacker (PackAlertRadius).
 *
 * The body is SK_Slime (Art/Models/Creatures/Slime.py), two bones: "body" at the ground, which every part but the core
 * is weighted to, so scaling it squashes the slime; and "core". All motion is code (UCreaturePoseAnimInstance applies it):
 * the squash is a spring, volume kept (sides scale by 1 / sqrt of the height's scale), and the core lags behind the
 * body's moves on a spring of its own. The pose is in the mesh's space, so it holds at any size (GetSizeScale); the hops,
 * the leap and the core's crit radius are the full-size slime's times its size.
 *
 * The fight pass (FIGHTS.md section 3, 2026-10-28; SlimeCreatureSplit.cpp, numbers in SlimeFightRules.h): a Restless or
 * better slime dies into two slimelets (this class at BodyScale 0.55, 0.3 of the health, half the damage, a quarter of the
 * experience; Basic; they never split, and count toward the level's caps); the pack's leaps begin 0.45 s apart (GetMeleeGap),
 * and a slime waiting its turn that takes three hits in 0.8 s may hop 250 cm aside.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ASlimeCreature : public ACreatureBase
{
	GENERATED_BODY()

public:
	ASlimeCreature();

	virtual void Tick(float DeltaSeconds) override;
	virtual void Landed(const FHitResult& Hit) override;

	/** Its gel's foot, round (SlimeCreatureBody.cpp). */
	virtual FFootprint GetFootprint() const override;

	// Crits: the core's hull, or a shot whose line runs on through the core.
	virtual bool IsCriticalSpot(const FHitResult& Hit) const override;

	/** Its leaps at one target begin at least this far apart (Looter.Slime.MeleeGap, 0.45 s): the pack's drumbeat (the fight director's per-kind gap). */
	virtual float GetMeleeGap() const override;

	/** The launch velocity of a hop that covers Length (cm, flat ground) along Direction, rising Height at the top. */
	static FVector HopVelocity(const FVector& Direction, float Length, float Height, float Gravity);

	/** The body's scale for a height scale of Z, keeping its volume. */
	static FVector SquashScale(float Z);

	/** How far from the core's middle a shot's line may pass and still count as hitting it (cm, times the slime's size). */
	static constexpr float CoreRadius = 12.f;

	/** The current height scale of the body (1 at rest), for tests and effects. */
	float GetSquash() const { return Squash.Value; }

	/** The squash: a spring toward Target, or a quick forced move (Ramp) for the snappy beats of a hop. */
	struct FSquashSpring
	{
		float Value = 1.f;
		float Velocity = 0.f;
		float Target = 1.f;
		float From = 1.f;
		float To = 1.f;
		float RampTime = 0.f;
		float RampLength = 0.f;

		void Ramp(float NewTo, float Seconds);
		bool IsRamping() const { return RampTime < RampLength; }
		void Tick(float DeltaSeconds);
	};

	/**
	 * Moves the squash and the core's lag behind the body (cm in the mesh's space, and its speed) on by DeltaSeconds, in
	 * short even steps: a distant slime updates only a few times a second, and its springs must go as far in one long
	 * update as in many short ones, without blowing up. An update longer than FCreatureUpdateRate::MaxInterval (a hitch)
	 * moves them that far.
	 */
	static void StepSprings(FSquashSpring& SquashSpring, FVector& CoreLag, FVector& CoreLagSpeed, float DeltaSeconds);

	/** The gel's half-width at its foot at rest (Slime.py: 60 cm, spread 1.3 at the foot), in the mesh's space. */
	static constexpr float FootRadius = 78.f;

	/** The most the body leans to lie along a slope (degrees): steeper ground leaves its foot a little off it. */
	static constexpr float MaxGroundTilt = 32.f;

	/**
	 * The turn that lays a body standing up along ground with this normal (both in the same space), at most MaxGroundTilt:
	 * its up turned onto the normal the shortest way.
	 */
	static FQuat TiltForGround(const FVector& GroundNormal);

	/** The ground's tilt the body wears now (the mesh's space), and how far below the mesh's origin it sits (cm, the mesh's space). */
	const FQuat& GetGroundTilt() const { return GroundTilt; }
	float GetGroundDrop() const { return GroundDrop; }

	/** Its jelly's lean now (degrees: X forward, Y to its right): away from a hit, swaying at rest. For the tests and tools. */
	FVector2D GetLean() const { return Lean; }

	// --- The hit-feel pass (HITS lane D, 2026-10-28; SlimeCreatureReact.cpp) ---

	/** How its death goes, by the killing hit: today's splat, a core crit's pop, a blast's splatter, an overkill's over-inflate and burst. */
	enum class EDeathStyle : uint8 { Body, Popped, Splattered, Overkill };

	/** The style a killing Hit gives (an overkill wins, then a blast, then a crit through the core). */
	static EDeathStyle DeathStyleFor(const FCreatureHit& Hit);
	EDeathStyle GetDeathStyle() const { return DeathStyle; }

	/** An overkill's gel swells to this height scale on the killing frame and holds it through the hit-stop. */
	static constexpr float OverInflate = 1.25f;

	/** A shot's push on a hop in the air (cm/s along the shot, flat): 150 + 400 x the burst's share of its health, at most MaxAirKnock, times sqrt of its size. */
	static FVector AirKnockFor(const FVector& ShotWay, float BurstShare, float SizeScale);
	static constexpr float MaxAirKnock = 450.f;

	/**
	 * Deflects a hop it is in the air for (the hop's ground rules: it never lands past a ledge, so the push is halved until
	 * its landing is on ground, else dropped). Returns the change in its velocity (zero: none made).
	 */
	FVector KnockInAir(const FVector& ShotWay, float BurstShare);

	/** Starts a hop of 200 cm at once along Direction (the tests and the dev tools; its brain does it on its own). */
	void HopNow(const FVector& Direction);
	bool IsInAir() const { return Hop == EHop::Air; }

	/** The wounded gel's size now (1 at full health, easing down to 0.88 at none left: the wound stages' shrink). */
	float GetWoundShrink() const { return WoundScale; }
	/** Its stagger's share now (0 to 1), from the hit reaction; 0 with the stagger pose off. */
	float GetStaggerShare() const;

	// --- The fight pass (SlimeCreatureSplit.cpp) ---

	/** A slimelet: a Restless or better slime's piece. It shares the slime's bestiary page, never splits, and hits and pays less. */
	bool IsSlimelet() const { return bSlimelet; }

	/**
	 * Makes this slime a slimelet: named so, its damage and experience cut to the tuning's shares (kept through rank changes).
	 * Called by the split on the slime it has just spawned.
	 */
	void MarkSlimelet(const FSlimeFightTuning& Tuning);

	/** Throws it Length cm along Direction in a low hop (the split's pieces; a side hop). The hop's height is a third of its length. */
	void ThrowNow(const FVector& Direction, float Length);

protected:
	virtual void BeginPlay() override;
	virtual void OnAttackStarted() override;
	virtual void Strike() override;
	virtual bool IsStuck(float Speed) const override;
	virtual bool CanStartAttack() const override;
	/** A leap called off in its telegraph (hit off its fight turn): it lets go of the squash. */
	virtual void OnAttackCalledOff() override;
	// --- Hits, death, coming back, the notice beat, its entrance (SlimeCreatureReact.cpp) ---
	virtual void OnHurt(bool bCritical, const FVector& HitLocation) override;
	virtual void OnDied() override;
	virtual void OnRespawned() override;
	virtual void OnPoseThawed() override;
	virtual void SetHitVolumesEnabled(bool bEnabled) override;
	virtual void OnNoticed() override;
	virtual void PlayEntrance() override;
	/** A slimelet's damage and experience shares are put on again after every rank application (SlimeCreatureSplit.cpp). */
	virtual void OnRankChanged() override;

private:
	enum class EHop : uint8
	{
		Ground,
		/** Squashing down before a hop. */
		Crouch,
		Air
	};

	void TickHops(const FVector& Wanted, float DeltaSeconds);
	void Launch();
	void AnimateBody(float DeltaSeconds);
	/**
	 * On the ground (or lying dead), measures the ground under its foot when it has moved, and eases the body's tilt and
	 * seat toward it; in the air, back upright over the capsule's foot (SlimeCreatureBody.cpp).
	 */
	void FitToGround(float DeltaSeconds);

	// --- The fight pass (SlimeCreatureSplit.cpp) ---
	/** On its death, a Restless or better slime bursts into slimelets when the caps allow. */
	void SplitOnDeath();
	/** A hit landed: three in the dodge window while it waits its turn make it hop aside (Looter.Slime.Dodge). */
	void NoteHitForDodge();
	/** Puts the slimelet's damage and experience shares on the stats the rank just set. */
	void ApplySlimeletStats();

	bool bSlimelet = false;
	float SlimeletDamageShare = 1.f;
	float SlimeletXPShare = 1.f;
	FSlimeDodgeTracker DodgeTracker;

	// --- Reactions and idle life (SlimeCreatureReact.cpp) ---
	/** Its lean's spring, its idle bubbles and sway move on. */
	void TickReactions(float DeltaSeconds);
	/** The wound drips of a slime under a quarter of its health, the stagger's wobble cue, and an overkill's burst once its hit-stop is over. */
	void TickWounds(float DeltaSeconds);
	/** Lets go of an overkill held inflated through its hit-stop: the kill's layers and its splat, the gel bursting into its puddle. */
	void ReleaseOverkill();
	/** Where on its gel a hit landed, 0 at the ground to 1 at the top. */
	float HitHeightShare(const FVector& HitLocation) const;
	/** What its notice beat does to the squash's rest: down low, shuddering. */
	float ReactSquash() const;
	/** Its lean as a turn about its foot (the mesh's space). */
	FQuat ReactLean() const;
	/** It landed from a hop: a hunting hop or a leap kicks up a ring of dust. */
	void NoteLanding();

	/** Its lean (degrees), the kick a hit gives it, and the spring's speed. */
	FVector2D Lean = FVector2D::ZeroVector;
	FVector2D LeanKick = FVector2D::ZeroVector;
	FVector2D LeanSpeed = FVector2D::ZeroVector;
	/** Seconds to its next bubble rising through it, while it rests. */
	float NextBubble = 3.f;
	bool bBubbleClockSet = false;
	/** Its notice beat's shudder: seconds to the next kick of its squash, and which way it goes. */
	float ShudderClock = 0.f;
	float ShudderSign = 1.f;

	// --- The hit-feel pass ---
	/** A core crit's ripple: seconds since the hit (past RippleSeconds it is over), and its size in squash. */
	float RippleAge = 100.f;
	float RippleAmount = 0.f;
	/** A stagger's quiver: seconds to the next kick of its squash, and which way. */
	float QuiverClock = 0.f;
	float QuiverSign = 1.f;
	/** Seconds to its next gel drip while hurt to a quarter of its health, and whether its stagger's cue has played. */
	float DripClock = 0.f;
	bool bStaggerCued = false;
	/** The wounded gel's size (eased toward lerp(0.88, 1, health share)). */
	float WoundScale = 1.f;
	/** How it died, and the hop's launch velocity (for a deflection before its movement has taken it up). */
	EDeathStyle DeathStyle = EDeathStyle::Body;
	FVector HopLaunchVelocity = FVector::ZeroVector;
	/** An overkill waiting on its hit-stop: the killing hit, and whether it still holds (ReleaseOverkill lets it burst). */
	FCreatureHit OverkillHit;
	bool bOverkillHeld = false;

	FName BodyBone = TEXT("body");
	FName CoreBone = TEXT("core");
	/** Rest poses in the mesh's space. */
	FTransform BodyRest;
	FTransform CoreRest;
	bool bRigReady = false;

	EHop Hop = EHop::Ground;
	float HopTime = 0.f;
	float NextHopDelay = 0.5f;
	FVector HopDirection = FVector::ForwardVector;
	/** The hop it's crouching for is a small shuffle aside from a neighbour (it was idle), not a stroll's hop. */
	bool bShuffleHop = false;
	FVector LaunchedFrom = FVector::ZeroVector;
	float PlannedLength = 0.f;
	/** Hops in a row that got less than a third of the way (something in the way). */
	int32 BlockedHops = 0;
	bool bLeaping = false;
	/** The attack's deep squash is held until the leap. */
	bool bTelegraph = false;

	FSquashSpring Squash;
	/** The core's lag behind the body (cm, mesh space) and its speed. */
	FVector CoreOffset = FVector::ZeroVector;
	FVector CoreVelocity = FVector::ZeroVector;
	float AnimTime = 0.f;
	float IdlePhase = 0.f;

	/** The body's fit to the ground (FitToGround): its tilt and drop now, where they head, and where it last measured. */
	FQuat GroundTilt = FQuat::Identity;
	float GroundDrop = 0.f;
	FQuat GroundTiltTarget = FQuat::Identity;
	float GroundDropTarget = 0.f;
	FVector GroundMeasuredAt = FVector(1.0e9);
	bool bGroundFitStarted = false;
};

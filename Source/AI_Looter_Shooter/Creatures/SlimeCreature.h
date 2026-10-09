#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureBase.h"
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

protected:
	virtual void BeginPlay() override;
	virtual void OnAttackStarted() override;
	virtual void Strike() override;
	virtual void OnHurt(bool bCritical, const FVector& HitLocation) override;
	virtual void OnDied() override;
	virtual void OnRespawned() override;
	virtual void OnPoseThawed() override;
	virtual void SetHitVolumesEnabled(bool bEnabled) override;
	virtual bool IsStuck(float Speed) const override;
	virtual bool CanStartAttack() const override;

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

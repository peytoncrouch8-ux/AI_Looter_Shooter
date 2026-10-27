#pragma once

#include "CoreMinimal.h"
#include "Combat/GrenadeKinds.h"
#include "GameFramework/Actor.h"
#include "GraveSaltGrenade.generated.h"

class APawn;
class UAudioComponent;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
struct FGraveSaltBurstResult;
struct FHitResult;

/**
 * A grave-salt grenade in flight (UPlayerThrowComponent throws it): the stoppered tin (SM_SaltGrenade, from
 * Art/Models/Throwables/SaltGrenade.py, 15 cm across since 2026-10-27) tumbling on its own arc, its waxed fuse fizzing
 * and spitting sparks. It moves itself (FGraveSaltRules: no physics body, no projectile component), sweeping a 9 cm ball
 * through what bullets meet: off the ground and walls it bounces and clinks, damped, then rolls and lies still; a
 * creature it touches bursts it at once. Otherwise the fuse bursts it 1.5 s after it left the hand (GraveSaltBurst does
 * the damage, the look, the sound and the jolt). After the burst it lingers for its flash of light (FGraveSaltTuning's
 * Flash*), then goes.
 *
 * Its kind (GrenadeKinds, round 4) changes its tin (Art/Models/Throwables/GrenadeJars.py), its burst and what follows
 * (GraveSaltGrenadeKinds.cpp):
 *  - Pitch: sticks to the first thing it touches (a creature carries it, and it bursts within Pitch.StuckFuse), and tars
 *    what its burst doesn't kill (UMovementSlowComponent; never a boss).
 *  - Scatter: its burst flings a brood of small jars (this class too, IsBrood: half the size, a 5 cm ball, a short fuse,
 *    their own small burst, no fizz loop) that bounce out and pop one after another.
 *  - Cinder: its burst leaves a bed of burning cinders (AGraveSaltCinderBed).
 *
 * Nothing collides with it: it never blocks the player, a creature or a shot.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AGraveSaltGrenade : public AActor
{
	GENERATED_BODY()

public:
	AGraveSaltGrenade();

	/**
	 * Throws a grenade from Start at StartVelocity for By (never hurt by it; its controller gets the kills), its burst at
	 * ByLevelScale (the thrower's level's FLevelRules::EnemyScale). Null without a world.
	 */
	static AGraveSaltGrenade* Throw(UWorld* World, const FVector& Start, const FVector& StartVelocity, APawn* By, float ByLevelScale);

	/** The same for a jar of Kind. */
	static AGraveSaltGrenade* Throw(UWorld* World, const FVector& Start, const FVector& StartVelocity, APawn* By, float ByLevelScale, EGrenadeKind Kind);

	/** One of a scatter jar's brood, flung from Start at StartVelocity, bursting Fuse seconds on. */
	static AGraveSaltGrenade* ThrowBrood(UWorld* World, const FVector& Start, const FVector& StartVelocity, APawn* By, float ByLevelScale, float Fuse);

	/** The tin (SM_SaltGrenade), or null before it's been imported (then the engine's cylinder stands in). */
	static UStaticMesh* FindJarMesh();

	/** A kind's tin (GrenadeKinds::MeshPath), the salt tin until it's imported; null when neither is. */
	static UStaticMesh* FindJarMesh(EGrenadeKind Kind);

	/** The tin's socket at the fuse's tip, where the sparks and the fizz come from. */
	static FName FuseSocket() { return TEXT("Fuse"); }

	/** Sends it off at Velocity (what Throw does after spawning it). */
	void Launch(const FVector& Velocity, APawn* InThrower, float InLevelScale);

	/** Makes it a jar of Kind (bInBrood: one of a scatter jar's small jars); before Launch. Its tin follows. */
	void SetKind(EGrenadeKind InKind, bool bInBrood = false);

	/** One step of its life: flight, fuse and, after the burst, its light (Tick runs it; tests call it in their levels). */
	void Step(float DeltaSeconds);

	/** Bursts it here and now (the fuse, or a creature touched). Nothing if it already has. */
	void Detonate();

	bool HasBurst() const { return bBurst; }
	bool IsResting() const { return bResting; }
	FVector GetVelocity() const override { return Velocity; }
	/** Seconds of fuse left. */
	float GetFuseLeft() const;
	/** Times it bounced off something hard enough to be heard. */
	int32 GetBounces() const { return Bounces; }
	APawn* GetThrower() const { return Thrower.Get(); }

	EGrenadeKind GetKind() const { return Kind; }
	/** One of a scatter jar's small jars. */
	bool IsBrood() const { return bBrood; }
	/** A pitch jar that has stuck (to a creature: GetStuckTo). */
	bool IsStuck() const { return bStuck; }
	AActor* GetStuckTo() const { return StuckTo.Get(); }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	/** The flight for Dt seconds: arcs, sweeps, bounces, rolls, or a creature touched (which bursts it). */
	void Fly(float Dt);
	/** Something the sweep met that bursts it on contact: anything that can be hurt that isn't a player or the thrower. */
	bool BurstsOn(const FHitResult& Hit) const;
	/** The clink of a bounce at Speed (cm/s into the surface), quieter the softer. */
	void Clink(const FVector& Where, float Speed);
	/** The fuse's sparks this frame, at its socket. */
	void DrawFuse();
	void TumbleJar(float Dt);
	void StopFuseSound();

	// --- GraveSaltGrenadeKinds.cpp ---

	/** Its tin: the kind's model (a small jar: the salt tin at half size), or the engine's cylinder before any is imported. */
	void ApplyJarMesh();
	/** A pitch jar meets Hit: it stops there and holds on (to a creature's bone, or anything that moves). */
	void StickTo(const FHitResult& Hit);
	/** What a kind does after its burst: the pitch's tar on who's left, the scatter's brood, the cinder's bed. */
	void AfterBurst(const FGraveSaltBurstResult& Result, const FVector& Up);
	/** The way its burst faces: the surface it stuck to, else world up. */
	FVector BurstUp() const;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Jar;

	/** The burst's flash on what's round it: a moment of warm light, no shadows. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPointLightComponent> Flash;

	TWeakObjectPtr<APawn> Thrower;
	TWeakObjectPtr<UAudioComponent> FuseSound;
	/** The creature (or moving thing) a pitch jar holds on to. */
	TWeakObjectPtr<AActor> StuckTo;

	FVector Velocity = FVector::ZeroVector;
	/** The surface a pitch jar stuck to faces this way (world up on a creature or before it sticks). */
	FVector StickNormal = FVector::UpVector;
	/** The tumble: about this axis, at this many degrees a second. */
	FVector SpinAxis = FVector::RightVector;
	float SpinRate = 0.f;
	float Age = 0.f;
	float LevelScale = 1.f;
	/** Its fuse (s from the hand): FGraveSaltRules::FuseSeconds; a small jar's own; cut short when a pitch jar sticks to a creature. */
	float FuseSeconds = 1.5f;
	/** The ball its flight sweeps (cm): FGraveSaltRules::CollisionRadius, a small jar's smaller. */
	float BallRadius = 9.f;
	float SinceBurst = 0.f;
	/** The burst's flash as the tuning had it when it went off: its candelas and its fade (s). */
	float FlashPeak = 0.f;
	float FlashFade = 0.25f;
	/** When the last clink played (Age), so a skitter isn't a rattle of them. */
	float LastClink = -1.f;
	int32 Bounces = 0;
	EGrenadeKind Kind = EGrenadeKind::Salt;
	bool bBrood = false;
	bool bStuck = false;
	bool bLaunched = false;
	bool bResting = false;
	bool bBurst = false;

	FRandomStream Random{ 0x5a17 };
};

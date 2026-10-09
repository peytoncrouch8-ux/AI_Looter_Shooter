#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GraveSaltGrenade.generated.h"

class APawn;
class UAudioComponent;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
struct FHitResult;

/**
 * A grave-salt grenade in flight (UPlayerThrowComponent throws it): the stoppered tin (SM_SaltGrenade, from
 * Art/Models/Throwables/SaltGrenade.py) tumbling on its own arc, its waxed fuse fizzing and spitting sparks. It moves
 * itself (FGraveSaltRules: no physics body, no projectile component), sweeping a 6 cm ball through what bullets meet:
 * off the ground and walls it bounces and clinks, damped, then rolls and lies still; a creature it touches bursts it at
 * once. Otherwise the fuse bursts it 1.5 s after it left the hand (GraveSaltBurst does the damage, the look, the sound
 * and the jolt). After the burst it lingers a quarter second for its flash of light, then goes.
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

	/** The tin (SM_SaltGrenade), or null before it's been imported (then the engine's cylinder stands in). */
	static UStaticMesh* FindJarMesh();

	/** The tin's socket at the fuse's tip, where the sparks and the fizz come from. */
	static FName FuseSocket() { return TEXT("Fuse"); }

	/** Sends it off at Velocity (what Throw does after spawning it). */
	void Launch(const FVector& Velocity, APawn* InThrower, float InLevelScale);

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

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Jar;

	/** The burst's flash on what's round it: a moment of warm light, no shadows. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPointLightComponent> Flash;

	TWeakObjectPtr<APawn> Thrower;
	TWeakObjectPtr<UAudioComponent> FuseSound;

	FVector Velocity = FVector::ZeroVector;
	/** The tumble: about this axis, at this many degrees a second. */
	FVector SpinAxis = FVector::RightVector;
	float SpinRate = 0.f;
	float Age = 0.f;
	float LevelScale = 1.f;
	float SinceBurst = 0.f;
	/** When the last clink played (Age), so a skitter isn't a rattle of them. */
	float LastClink = -1.f;
	int32 Bounces = 0;
	bool bLaunched = false;
	bool bResting = false;
	bool bBurst = false;

	FRandomStream Random{ 0x5a17 };
};

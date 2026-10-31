#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
#include "Creatures/StoneSentinelMoveRules.h"
#include "GameFramework/Actor.h"
#include "StoneSentinelBoulder.generated.h"

class AController;
class AGroundCrack;
class AStoneSentinelCreature;
class UDecalComponent;
class UHealthComponent;
class USphereComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * A Stone Sentinel's hurled boulder (the user's pick, 2026-10-31: "a hurled boulder at range"; StoneSentinelCreatureBoulder.cpp
 * throws it): a granite rock of the level's own (SM_Rock_A or SM_Rock_B, sized to FSentinelBoulderTuning::Size) torn out of
 * the ground and carried in its grip, then lobbed at where its target will be.
 *  - In its grip (Hold, its code moves it every frame): it can be shot (a sphere of HitRadius on the Weapon channel, its own
 *    health HealthShare of the sentinel's). Broken there, it bursts over the sentinel's head (CounterBoulder): its counter.
 *  - Thrown (Throw): an arc under its own gravity, turning over as it goes, grit streaming off it, its whoom following it.
 *    Where it will come down is found along its arc as it leaves, and marked there: the landing shadow (MI_Decal_LandingShadow)
 *    growing and darkening as it falls, and a ring of warning light the size of its burst (FWeaponFX::AddGroundRing). It meets
 *    the world as the Gravemother's clod does (world-static geometry; players by their capsules, never a creature).
 *  - Landing (StoneSentinelBoulderLanding.cpp): it bursts, rock and dust thrown out, the ground cracked round it, an impact
 *    scar (MI_Decal_ImpactScar) fading after; every player within SplashRadius hurt (less at the edge) and shoved, the view shaken.
 *  - Dropped (its throw called off, its thrower dead): it falls from the grip and bursts harmlessly.
 *  - Kept (its thrower holds it: Retear): done, it lies hidden and idle, its mesh, sphere, health and decals reused for the next
 *    throw; a spare one, or one whose thrower is gone, removes itself. Its dust stays off the lens (StoneSentinelFX).
 * Until the rocks are imported it's a granite ball of the engine sphere; until the decals are, a ring of dark cracks marks the spot.
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API AStoneSentinelBoulder : public AActor
{
	GENERATED_BODY()

public:
	AStoneSentinelBoulder();

	/** Tears a rock for Thrower out of the ground at Where (its middle, sunk), in its grip from now; its health HealthShare of
	 *  the thrower's most. */
	static AStoneSentinelBoulder* Spawn(AStoneSentinelCreature& InThrower, const FVector& Where, const FSentinelBoulderTuning& InTuning,
		bool bInKeep = false);

	/** Tears this rock up again at Where (a kept rock, idle: its mesh, sphere, health and decals reused, nothing new made). */
	void Retear(const FVector& Where, const FSentinelBoulderTuning& InTuning);

	/** Done and hidden, waiting to be torn up again (a kept rock). */
	bool IsIdle() const { return bIdle; }

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** In its grip: its middle at Where, turned to Yaw (the thrower's). */
	void Hold(const FVector& Where, float Yaw);

	/**
	 * Lobs it at LaunchVelocity: InDamage at the burst's middle (already its thrower's, times the fight's difficulty), credited
	 * to By and From. Where it will come down is found along its arc now and marked there.
	 */
	void Throw(const FVector& LaunchVelocity, float InDamage, AController* By, AActor* From);

	/** Its grip lost: it falls and bursts harmlessly. */
	void Drop();

	/** Moves it on (its tick; the tests call it). */
	void Advance(float DeltaSeconds);

	bool IsHeld() const { return Phase == EPhase::Held; }
	bool IsFlying() const { return Phase == EPhase::Flying || Phase == EPhase::Dropping; }
	bool HasLanded() const { return Phase == EPhase::Landed; }
	bool WasBroken() const { return bBroken; }
	/** Where it was found to come down as it was thrown, how long that would take, and where it did. */
	FVector GetPlannedLanding() const { return PlannedLanding; }
	float GetPlannedSeconds() const { return PlannedSeconds; }
	FVector GetLandedAt() const { return LandedAt; }
	int32 GetPlayersHit() const { return PlayersHit; }
	UHealthComponent* GetHealthComponent() const { return Health; }
	USphereComponent* GetHitZone() const { return HitZone; }

	/** The rocks it's made of (the level's granite; the engine sphere stands in until they're imported). */
	static const TCHAR* const RockPathA;
	static const TCHAR* const RockPathB;
	/** The warning ring's colour (a fuse's orange, read against grass, dirt and stone). */
	static const FLinearColor MarkColor;

private:
	enum class EPhase : uint8 { Held, Flying, Dropping, Landed };

	UFUNCTION()
	void HandleDeath(AController* Killer);

	UFUNCTION()
	void HandleDamaged(float InDamage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser);

	void Fly(float DeltaSeconds);
	/** Grit and a ribbon of dust off it in flight. */
	void Trail(float DeltaSeconds);
	/** The landing's mark: the shadow growing and darkening, the warning ring brightening, as it comes down. */
	void UpdateMark();

	// --- Landing (StoneSentinelBoulderLanding.cpp) ---
	/** It comes down at Where (the world's Normal there; none when on a body): burst, hurt; or harmless when it was dropped. */
	void Land(const FVector& Where, const FVector& Normal);
	/** Every player near Where hurt and shoved by the burst; how many. */
	int32 HurtNear(const FVector& Where);
	/** Its shadow and stand-in ring cleared away. */
	void ClearMark();
	/** Done: hidden, out of the way, idle for its thrower to use again (or gone, if nobody keeps it). */
	void GoIdle();
	/** After landing: the impact scar fading, then it's idle. */
	void TickScar(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, Category = "Boulder")
	TObjectPtr<USphereComponent> HitZone;
	UPROPERTY(VisibleAnywhere, Category = "Boulder")
	TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere, Category = "Boulder")
	TObjectPtr<UHealthComponent> Health;
	UPROPERTY(Transient)
	TObjectPtr<UDecalComponent> Shadow;
	UPROPERTY(Transient)
	TObjectPtr<UDecalComponent> Scar;
	/** Its two rocks (constructor-found, so they're cooked with it). */
	UPROPERTY()
	TObjectPtr<UStaticMesh> RockA;
	UPROPERTY()
	TObjectPtr<UStaticMesh> RockB;

	TWeakObjectPtr<AStoneSentinelCreature> Thrower;
	TWeakObjectPtr<AController> ThrowInstigator;
	TWeakObjectPtr<AActor> ThrowCauser;
	/** The shadow's stand-in: a ring of dark cracks bursting out over the flight. */
	TWeakObjectPtr<AGroundCrack> MarkCrack;
	FSentinelBoulderTuning Tuning;
	FCollisionQueryParams WorldQuery;

	EPhase Phase = EPhase::Held;
	bool bBroken = false;
	bool bDropped = false;
	/** Its thrower keeps it to tear up again; idle between throws; its scar showing now. */
	bool bKeep = false;
	bool bIdle = false;
	bool bScarShown = false;
	FVector Position = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	FVector SpinAxis = FVector::UpVector;
	float SpinSpeed = 2.f;
	float Damage = 0.f;
	float FlightTime = 0.f;
	FVector PlannedLanding = FVector::ZeroVector;
	FVector PlannedNormal = FVector::UpVector;
	float PlannedSeconds = 1.f;
	FVector LandedAt = FVector::ZeroVector;
	int32 PlayersHit = 0;
	float TrailIn = 0.f;
	float GritIn = 0.f;
	float ScarAge = 0.f;
	double LastChipSound = -10.0;
};

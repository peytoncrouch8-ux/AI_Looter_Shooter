#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
#include "Creatures/GravemotherTypes.h"
#include "GameFramework/Actor.h"
#include "GravemotherClod.generated.h"

class ACharacter;
class AController;
class AGravemotherCreature;
class AGravemotherRubble;
class AGroundCrack;
class UDecalComponent;
class UHealthComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * The Gravemother's Grave Clod (the user's call, 2026-10-15/19): a clod of grave earth (SM_GraveClod_L) she tears out of the
 * floor and lifts overhead in her front legs, then lobs at where her target stands.
 *  - Held (her code moves it every frame, Hold): it can be shot, generously (a sphere of HitRadius on the Weapon channel).
 *    Shot, it bursts on her: dirt rains on her head and back, she shrieks, and she's stunned (her CounterClod).
 *  - Thrown (Throw): a lob under its own gravity, its landing shadow (MI_Decal_LandingShadow) growing on the ground where it
 *    will come down, a ribbon of earth dust and crumbs behind it, its whoosh following it. It meets the world as the
 *    Spitter's glob does (world-static geometry, berms and rubble too; players by their capsules, never a creature).
 *  - Landing (GravemotherClodLanding.cpp): it bursts into dust and debris, hurts and shoves every player near, and leaves a
 *    rubble pile (AGravemotherRubble: cover for 10 s).
 *  - Dropped (her tell broken off by a stagger or her death): it falls from her grip and bursts harmlessly.
 * Until SM_GraveClod_L is imported it's an earth-coloured ball of the engine sphere, and its shadow a ring of dark cracks.
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API AGravemotherClod : public AActor
{
	GENERATED_BODY()

public:
	AGravemotherClod();

	/** Tears a clod up out of the ground at Where for Mother (it starts small; Hold grows it), begun. */
	static AGravemotherClod* Spawn(AGravemotherCreature& InMother, const FVector& Where, const FGravemotherClodSettings& InSettings);

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** In her grip: its middle at Where, GrownShare of its full size (0-1). */
	void Hold(const FVector& Where, float GrownShare);

	/**
	 * Lobs it at LaunchVelocity: InDamage at a burst's middle (her bite times Strength, rolled as hers), credited to By and
	 * From. Where it will come down is found along its arc now, and its shadow laid there.
	 */
	void Throw(const FVector& LaunchVelocity, float InDamage, AController* By, AActor* From);

	/** Her grip lost: it falls and bursts harmlessly. */
	void Drop();

	/** Moves it on (its tick; the tests call it). */
	void Advance(float DeltaSeconds);

	/** Where an arc from From at LaunchVelocity under Gravity first meets the world (Query), stepped in Pieces up to MaxSeconds. */
	static bool PredictLanding(const UWorld* World, const FVector& From, const FVector& LaunchVelocity, float Gravity, float MaxSeconds,
		const FCollisionQueryParams& Query, FVector& OutWhere, FVector& OutNormal, float& OutSeconds, int32 Pieces = 12);

	bool IsHeld() const { return Phase == EPhase::Held; }
	bool IsFlying() const { return Phase == EPhase::Flying || Phase == EPhase::Dropping; }
	bool HasLanded() const { return Phase == EPhase::Landed; }
	bool WasCountered() const { return bCountered; }
	bool WasDropped() const { return bDropped; }
	/** Where it was found to come down as it was thrown, how long that would take, and where it did. */
	FVector GetPlannedLanding() const { return PlannedLanding; }
	float GetPlannedSeconds() const { return PlannedSeconds; }
	FVector GetLandedAt() const { return LandedAt; }
	float GetFlightTime() const { return FlightTime; }
	int32 GetPlayersHit() const { return PlayersHit; }
	AGravemotherRubble* GetRubble() const { return Rubble.Get(); }
	USphereComponent* GetHitZone() const { return HitZone; }
	UHealthComponent* GetHealthComponent() const { return Health; }
	/** Its shadow on the ground while it flies (null once landed, or before its decal is imported). */
	UDecalComponent* GetShadow() const { return Shadow; }

	/** Its size at full grip (cm across: SM_GraveClod_L's 121). */
	static constexpr float FullSize = 121.f;

private:
	enum class EPhase : uint8 { Held, Flying, Dropping, Landed };

	UFUNCTION()
	void HandleDeath(AController* Killer);

	void Fly(float DeltaSeconds);
	/** Its ribbon of dust and crumbs behind it (in flight). */
	void Trail(float DeltaSeconds);
	/** The shadow growing under where it'll come down. */
	void UpdateShadow();

	// --- Landing (GravemotherClodLanding.cpp) ---
	/** It comes down at Where (the world's Normal there): burst, hurt, rubble; or harmless when it was dropped. */
	void Land(const FVector& Where, const FVector& Normal);
	/** Shot in her grip: it bursts on her (dirt on her head and back), then it's gone. */
	void BurstOnMother();
	/** Every player near Where hurt and shoved by the burst; how many. */
	int32 HurtNear(const FVector& Where);
	void ClearShadow();

	UPROPERTY(VisibleAnywhere, Category = "Clod")
	TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere, Category = "Clod")
	TObjectPtr<USphereComponent> HitZone;
	UPROPERTY(VisibleAnywhere, Category = "Clod")
	TObjectPtr<UHealthComponent> Health;
	UPROPERTY(Transient)
	TObjectPtr<UDecalComponent> Shadow;

	TWeakObjectPtr<AGravemotherCreature> Mother;
	TWeakObjectPtr<AController> ThrowInstigator;
	TWeakObjectPtr<AActor> ThrowCauser;
	TWeakObjectPtr<AGravemotherRubble> Rubble;
	/** The shadow's stand-in: a ring of dark cracks bursting out over the flight. */
	TWeakObjectPtr<AGroundCrack> ShadowCrack;
	FGravemotherClodSettings Settings;
	FCollisionQueryParams WorldQuery;

	EPhase Phase = EPhase::Held;
	bool bCountered = false;
	bool bDropped = false;
	/** Its size against its mesh's (the stand-in sphere's is set to match FullSize). */
	float MeshScale = 1.f;
	float Grown = 0.3f;
	FVector Position = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	FVector SpinAxis = FVector::UpVector;
	float SpinSpeed = 3.f;
	float Damage = 0.f;
	float FlightTime = 0.f;
	FVector PlannedLanding = FVector::ZeroVector;
	FVector PlannedNormal = FVector::UpVector;
	float PlannedSeconds = 1.f;
	FVector LandedAt = FVector::ZeroVector;
	int32 PlayersHit = 0;
	float TrailIn = 0.f;
	float CrumbIn = 0.f;
};

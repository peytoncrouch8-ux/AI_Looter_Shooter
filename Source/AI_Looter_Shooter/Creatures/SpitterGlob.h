#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
#include "Creatures/SpitterPuddle.h"
#include "Creatures/SpitterRules.h"
#include "GameFramework/Actor.h"
#include "SpitterGlob.generated.h"

class ACharacter;
class AController;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMeshComponent;

/** One glob as it leaves a Spitter's fangs: its flight, what it does where it lands, and whose it is (copied: the Spitter may die). */
struct FSpitterGlobLaunch
{
	FVector Start = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	float Gravity = SpitterRules::GlobGravity;
	/** Its radius (cm): how near it must pass a player's capsule to hit them squarely, and how big it's drawn. */
	float Radius = 14.f;
	/** A square hit's damage, and the splash's on anyone else within SplashRadius of where it lands (before the per-hit roll). */
	float Damage = 10.f;
	float SplashDamage = 5.f;
	float SplashRadius = 160.f;
	/** The Spitter's size: its splat's. */
	float Size = 1.f;
	FLinearColor Color = FLinearColor(0.45f, 1.f, 0.12f);
	/** The puddle it leaves (its Spitter and instigator are this glob's). */
	FSpitterPuddleSpec Puddle;
	TWeakObjectPtr<AActor> Spitter;
	TWeakObjectPtr<AController> Instigator;
};

/**
 * A Spitter's glob of venom (ASpitterSpider): lobbed on a high arc (SpitterRules::GlobGravity), a glowing green blob that
 * wobbles as it flies and trails a streak and a green vapour. It hurts the player it comes down on, splashes anyone else
 * near where it lands, and leaves a venom puddle there (ASpitterPuddle) on ground it can lie on (on a wall, on the ground
 * under it; on a player, at their feet). It meets the world as the enemy pellets do (UEnemyProjectileSubsystem): one line
 * trace a frame against world-static geometry (volumes and the playable area's walls let it by, rocks and trees don't),
 * and players by plain capsule geometry, so it never hits a creature. One actor a glob: a Spitter throws one every few
 * seconds, a Gravebound one three.
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API ASpitterGlob : public AActor
{
	GENERATED_BODY()

public:
	ASpitterGlob();

	/** Throws a glob (begun even in a level that isn't playing, a test's). Null when it couldn't spawn. */
	static ASpitterGlob* Launch(UWorld* World, const FSpitterGlobLaunch& Launch);

	virtual void Tick(float DeltaSeconds) override;

	/**
	 * What a glob (and its puddle, and a Spitter judging its lob) meets of the world, as the pellets meet it: world-static
	 * geometry less every volume's box and the playable area's walls, and Ignored.
	 */
	static FCollisionQueryParams MakeWorldQuery(const UWorld* World, const AActor* Ignored);

	/** It has come down (it's gone a moment later). */
	bool HasLanded() const { return bLanded; }

	/** Where it came down, the player it hit squarely (if any), how many others its splash caught, and the puddle it left. */
	const FVector& GetLandedAt() const { return LandedAt; }
	AActor* GetDirectVictim() const { return DirectVictim.Get(); }
	int32 GetSplashed() const { return Splashed; }
	ASpitterPuddle* GetPuddle() const { return Puddle.Get(); }

	/** Where it is now, and how long it has flown. */
	const FVector& GetPosition() const { return Position; }
	float GetFlightTime() const { return Age; }

	/** A glob that has flown this long without coming down (off a cliff's edge) is gone. */
	static constexpr float MaxFlight = 4.f;

protected:
	virtual void BeginPlay() override;

private:
	/** Moves it on by DeltaSeconds, landing it where its path meets a player or the world. */
	void Fly(float DeltaSeconds);

	/** It comes down at Where (the world's Normal there; Victim: the player it hit squarely). */
	void Land(const FVector& Where, const FVector& Normal, ACharacter* Victim);

	/** The ground for its puddle: Where itself on floor-like ground, else straight below it. */
	bool FindPuddleGround(const FVector& Where, const FVector& Normal, bool bOnBody, FVector& OutGround, FVector& OutNormal) const;

	/** Its body where it is, wobbling, and its trail. */
	void Draw(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> GlowMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Look;

	FSpitterGlobLaunch Spec;
	FVector Position = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	FCollisionQueryParams WorldQuery;
	float Age = 0.f;
	/** Seconds to its next puff of vapour. */
	float PuffIn = 0.f;
	bool bLanded = false;
	/** It has hissed past the player's head already (NearMiss: once a glob). */
	bool bHeardFlyBy = false;
	FVector LandedAt = FVector::ZeroVector;
	TWeakObjectPtr<AActor> DirectVictim;
	TWeakObjectPtr<ASpitterPuddle> Puddle;
	int32 Splashed = 0;
};

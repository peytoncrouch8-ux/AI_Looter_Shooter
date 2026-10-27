#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GraveSaltCinderBed.generated.h"

class APawn;
class UAudioComponent;
class UPointLightComponent;

/**
 * The cinder jar's bed (GrenadeKinds, round 4): the forge coals packed under its salt, thrown down where it burst and
 * burning there a few seconds (Looter.Grenade.Cinder.BedRadius 2.6 m, .BedSeconds 5). Every .BedTick (0.5 s) it burns what
 * stands in it: the thrower's level's share (the same early ramp as the jar), .BedShare of the salt jar's heart a second,
 * as Ember (UEmberElementDamageType: the weakness table's x1.75 for spiders, slimes and the Gravemother, x0.5 for the
 * Unpaid, and Ember's tinted numbers); a boss at most half the jar's cap from all of it (GrenadeKinds::BedBossCap).
 * Never the thrower or a player (fun first), never through a wall.
 *
 * Seen and heard: a ring of ember light round its edge (where it burns, read at a glance), flames licking up off it,
 * embers rising, the bodies in it catching, a dim flickering light and a crackling loop; all of it dying down over its
 * last second. Drawn in code through FWeaponFX (no particle assets). Nothing collides with it.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AGraveSaltCinderBed : public AActor
{
	GENERATED_BODY()

public:
	AGraveSaltCinderBed();

	/**
	 * A bed under a cinder jar's burst at Burst (on the ground under it, within a few metres; none when there's no
	 * ground there, or only a steep face), burning for Thrower at LevelScale. Null when none was made.
	 */
	static AGraveSaltCinderBed* Spawn(UWorld* World, const FVector& Burst, APawn* Thrower, float LevelScale);

	/** One step of its life: its burns on their beat, its look, its end (Tick runs it; tests call it in their levels). */
	void Step(float DeltaSeconds);

	/** Burns what stands in it now, for Seconds of burning. Returns how many bodies it burned. */
	int32 Burn(float Seconds);

	float GetRadius() const { return Radius; }
	float GetSeconds() const { return Seconds; }
	float GetAge() const { return Age; }
	/** The ground's up where it lies. */
	FVector GetUp() const { return Up; }
	/** What it has dealt a boss so far (its cap's tally); 0 for anything else. */
	float GetBossDealt(const AActor* Boss) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	/** Its look this frame (flames, embers, the edge ring, the light) at Strength (1 burning, falling to 0 at its end). */
	void DrawFire(float DeltaSeconds, float Strength);
	/** 1 while it burns, falling over its last second to 0. */
	float GetStrength() const;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** Its flickering glow on what's round it (no shadows). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPointLightComponent> Glow;

	TWeakObjectPtr<APawn> Thrower;
	TWeakObjectPtr<UAudioComponent> Crackle;
	/** What it has dealt each boss, held to its cap. */
	TMap<TWeakObjectPtr<AActor>, float> BossDealt;

	FVector Up = FVector::UpVector;
	float Radius = 260.f;
	float Seconds = 5.f;
	float TickEvery = 0.5f;
	float LevelScale = 1.f;
	float Age = 0.f;
	float SinceBurn = 0.f;
	/** Flames and ember puffs owed to the next frames (a share of one carried over). */
	float FlamesOwed = 0.f;
	float EmbersOwed = 0.f;

	FRandomStream Random{ 0xc1d3 };
};

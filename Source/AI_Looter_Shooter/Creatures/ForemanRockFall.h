#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ForemanRockFall.generated.h"

class AController;
class AForemanTimberProp;
class AGroundCrack;
class UStaticMeshComponent;

/**
 * One marked spot of the Foreman's roof strike (Docs/Areas/RansomsRest_Expansion.md, "The Providence": "dust trickles on
 * four marked spots for 2 s, then rock falls there"): first the mark (dust sifting down from the roof over the spot, and a
 * ring of cracks spreading on the floor to how far the fall reaches: the time to step out), then the rock (a chunk of the
 * roof dropping onto it), which hurts a player standing in its reach, knocks them off it, and splinters a timber prop it
 * lands on. The rubble lies a few seconds and sinks away. It plays out by itself (it ticks), whatever becomes of the Foreman.
 *
 * The rock is one of the level's own (SM_Rock_A to D), sized to about RockSize; the roof over the spot is found by a trace
 * up (the stope's rock), or RoofFallback above the floor where there's none (a test level).
 */
UCLASS(NotPlaceable)
class AI_LOOTER_SHOOTER_API AForemanRockFall : public AActor
{
	GENERATED_BODY()

public:
	AForemanRockFall();

	/**
	 * Marks Floor (the ground, world) for DustSeconds, then drops the rock there: Damage to a player within Radius (cm, flat,
	 * to their body's side), the props within Radius splintered. By is who brought it down (credited with the hurt).
	 */
	static AForemanRockFall* Spawn(UWorld* World, const FVector& Floor, float Radius, float Damage, AActor* By, float DustSeconds,
		const TArray<AForemanTimberProp*>& Props);

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Moves it on by DeltaSeconds (the tick does; the tests call it in a level that never ticks). */
	void Advance(float DeltaSeconds);

	/** The rock has come down (and hurt whoever was in reach). */
	bool HasFallen() const { return bFallen; }

	/** It hurt the player when it fell. */
	bool HitPlayer() const { return bHitPlayer; }

	/** Where it comes down (the floor), and how far it reaches. */
	FVector GetSpot() const { return Spot; }
	float GetRadius() const { return Radius; }

	/** Seconds until the rock lands (0 once it has). */
	float GetTimeToFall() const { return FMath::Max(DustSeconds + DropSeconds - Age, 0.f); }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Rock;

	/** The rock's size (cm across) and how long it takes to drop from the roof (s). */
	static constexpr float RockSize = 75.f;
	static constexpr float DropSeconds = 0.3f;
	/** The roof over a spot with none found (cm). */
	static constexpr float RoofFallback = 650.f;
	/** How long the rubble lies, and how long it takes to sink away (s). */
	static constexpr float RubbleSeconds = 5.f;
	static constexpr float SinkSeconds = 1.f;

private:
	void Begin();
	void SiftDust();
	void Land();
	/** Its damage, shove and view's jolt on every player in reach; whether it hurt one. */
	bool HurtPlayers();

	FVector Spot = FVector::ZeroVector;
	FVector Roof = FVector::ZeroVector;
	float Radius = 170.f;
	float Damage = 0.f;
	float DustSeconds = 2.f;
	float Age = 0.f;
	float DustClock = 0.f;
	bool bFallen = false;
	bool bHitPlayer = false;
	TWeakObjectPtr<AActor> BroughtBy;
	TArray<TWeakObjectPtr<AForemanTimberProp>> Props;
	TWeakObjectPtr<AGroundCrack> Mark;
	float RockScale = 1.f;
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GravemotherRubble.generated.h"

class UDecalComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * What a Grave Clod leaves where it bursts (the user's call: "a rubble pile, cover, 10 s, then it settles away"): a pile of
 * fresh grave earth (SM_GraveRubble, its hulls the cover) with a few loose clods round it and an impact scar under it. It
 * blocks bullets, sight, walking and lobs like any rock (world-static to every query, tagged Obstacle so nothing spawns on
 * it), but stops a player only once none stands where it came down. Its time up, it sinks back into the ground. Until the
 * pile is imported it's a half-sunk earth mound of the engine sphere.
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API AGravemotherRubble : public AActor
{
	GENERATED_BODY()

public:
	AGravemotherRubble();

	/** A pile on InGround (world), turned Yaw, standing InSeconds, then settling away over InSettleSeconds; begun. */
	static AGravemotherRubble* Spawn(UWorld* World, const FVector& InGround, float Yaw, float InSeconds, float InSettleSeconds);

	virtual void Tick(float DeltaSeconds) override;

	/** Moves it on (its tick; the tests call it). Returns false once it's gone. */
	bool Advance(float DeltaSeconds);

	/** It's sinking away (cover no more). */
	bool IsSettling() const { return Age >= Seconds; }
	/** It stops walking players (none stood where it came down, or they've moved off it). */
	bool BlocksPawns() const { return bBlocksPawns; }
	float GetAge() const { return Age; }
	UStaticMeshComponent* GetPile() const { return Pile; }

	/** How long it pops up out of the burst (s). */
	static constexpr float RiseSeconds = 0.15f;

private:
	/** Whether a player's body overlaps the pile now. */
	bool IsPlayerInside() const;
	/** Sets the pile's blocking: on pawns only once clear; nothing at all once settling. */
	void ApplyCollision();

	UPROPERTY(VisibleAnywhere, Category = "Rubble")
	TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category = "Rubble")
	TObjectPtr<UStaticMeshComponent> Pile;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Loose;
	UPROPERTY(Transient)
	TObjectPtr<UDecalComponent> Scar;

	FVector Ground = FVector::ZeroVector;
	FVector PileScale = FVector::OneVector;
	FVector PileOffset = FVector::ZeroVector;
	float PileHeight = 110.f;
	float Seconds = 10.f;
	float SettleSeconds = 1.f;
	float Age = 0.f;
	bool bBlocksPawns = false;
	bool bSettled = false;
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GravemotherMound.generated.h"

class AController;
class AGravemotherCreature;
class ASpiderCreature;
class UAudioComponent;
class UBoxComponent;
class UDecalComponent;
class UHealthComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * One of the Gravemother's Unearthed (the user's call, 2026-10-19: "at 66% and 33% she slams the floor and 4 spiderlings dig
 * out of mounds that bulge for 1 s first; a mound shot while it bulges kills the spiderling in it"): a dome of grave earth
 * (SM_GraveMound, its cracks MI_Decal_MoundCracks) swelling out of the floor, stirring, for BulgeSeconds, then bursting into
 * a crater (SM_GraveMoundBurst) as its spiderling claws out (her RaiseFromMound). Shot while it bulges (any hit: a generous
 * box on the Weapon channel), it caves in on the spiderling: no spiderling, the shooter credited with its kill (its
 * experience, the red confirm, the kill's sound). Her death or her fight starting over caves the rest in quietly. Crater or
 * caved hole lies a few seconds, then sinks away. Until its meshes are imported it's a flattened earth ball.
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API AGravemotherMound : public AActor
{
	GENERATED_BODY()

public:
	AGravemotherMound();

	/** A mound for InMother on InFeet (the ground), its spiderling to face InYaw, bulging InBulgeSeconds; begun. */
	static AGravemotherMound* Spawn(AGravemotherCreature& InMother, const FVector& InFeet, float InYaw, float InBulgeSeconds, int32 InSpiderlingLevel);

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Moves it on (its tick; the tests call it). */
	void Advance(float DeltaSeconds);

	/** Caved in by By's shot while it bulges (its health's end does this): no spiderling. False when it's past bulging. */
	bool Pop(AController* By);
	/** Caved in quietly (her death, her fight over): no spiderling, no credit. */
	void Collapse();

	bool IsBulging() const { return Phase == EPhase::Bulging; }
	bool HasBurst() const { return Phase == EPhase::Burst; }
	bool WasPopped() const { return Phase == EPhase::Popped; }
	bool IsCollapsed() const { return Phase == EPhase::Collapsed; }
	/** How far it has swelled (0-1). */
	float GetBulge() const;
	FVector GetFeet() const { return Feet; }
	float GetYaw() const { return Yaw; }
	int32 GetSpiderlingLevel() const { return SpiderlingLevel; }
	ASpiderCreature* GetSpiderling() const;
	UBoxComponent* GetHitBox() const { return HitBox; }
	UHealthComponent* GetHealthComponent() const { return Health; }

	/** A burst crater lies this long, a caved one this long, then each sinks away over SinkSeconds (s). */
	static constexpr float CraterSeconds = 6.f;
	static constexpr float CavedSeconds = 4.f;
	static constexpr float SinkSeconds = 1.f;

private:
	enum class EPhase : uint8 { Bulging, Burst, Popped, Collapsed };

	UFUNCTION()
	void HandleDeath(AController* Killer);

	/** It bursts: the crater, the dust, its spiderling clawing out. */
	void Burst();
	/** Ends its bulge as Next (popped or collapsed): its sound stops, it caves in, its box is off. */
	void CaveIn(EPhase Next);
	/** Its mesh and box at its swell now. */
	void Shape();
	/** The spiderling's kill credited to By (its experience and a defeat in the bestiary, the kill's sound). */
	void CreditKill(AController* By) const;

	UPROPERTY(VisibleAnywhere, Category = "Mound")
	TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category = "Mound")
	TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere, Category = "Mound")
	TObjectPtr<UBoxComponent> HitBox;
	UPROPERTY(VisibleAnywhere, Category = "Mound")
	TObjectPtr<UHealthComponent> Health;
	UPROPERTY(Transient)
	TObjectPtr<UDecalComponent> Cracks;
	UPROPERTY(Transient)
	TObjectPtr<UDecalComponent> Scar;

	TWeakObjectPtr<AGravemotherCreature> Mother;
	TWeakObjectPtr<ASpiderCreature> Spiderling;
	TWeakObjectPtr<UAudioComponent> StirSound;
	FVector Feet = FVector::ZeroVector;
	float Yaw = 0.f;
	float BulgeSeconds = 1.f;
	int32 SpiderlingLevel = 1;
	EPhase Phase = EPhase::Bulging;
	float Age = 0.f;
	/** Seconds since it burst or caved. */
	float EndAge = 0.f;
	float SiftIn = 0.f;
	bool bStandIn = false;
	/** The burst crater's mesh, or the stand-in's. */
	bool bCraterStandIn = false;
};

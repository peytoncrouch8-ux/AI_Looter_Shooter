#pragma once

#include "CoreMinimal.h"
#include "Creatures/GravemotherTypes.h"
#include "GameFramework/Actor.h"
#include "GravemotherStone.generated.h"

class AController;
class AGravemotherCreature;
class UHealthComponent;
class USceneComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;

/**
 * The Gravemother's headstone (the user's pick over plain armour, 2026-10-19): a toppled granite headstone bound to her
 * abdomen by roots, an actor of its own attached to her body's abdomen bone, so a shot on it hits the stone, never her
 * (her health, her boss and her stagger never hear of it). It has its own health (FGravemotherStoneSettings::HealthShare
 * of hers: Main's 20%) and shows it: intact, cracked at 66% of it, shattered at 33% (SM_GravemotherStone_0/_1/_2), each
 * crack with its sound and a shower of chips. Shots glance off it with sparks, chips and a hard crack, as their own look
 * (UBulletSubsystem::OwnLookTag: no world puff over it). Broken, it cracks off (GravemotherStoneBreak.cpp): the slab is
 * gone and its pieces slide off her back to the floor, lie a while and sink away; she's told (OnStoneBroken), and her
 * whole abdomen is a critical spot from then. It also cracks off by itself at her 33% (her Break(true)).
 *
 * Only the Weapon channel sees it: it never blocks her movement, her lane's sweeps or her sight. It's tagged Breakable,
 * so its break isn't a kill to the HUD (a white hit marker, never the red confirm). Until its meshes are imported it's a
 * granite slab of the engine's cube, and its pieces five smaller cubes.
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API AGravemotherStone : public AActor
{
	GENERATED_BODY()

public:
	AGravemotherStone();

	/** Makes Mother's stone, on her abdomen at Placement (in the abdomen bone's space at her model's size), begun. */
	static AGravemotherStone* Spawn(AGravemotherCreature& InMother, const FTransform& Placement, const FGravemotherStoneSettings& InSettings);

	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator,
		AActor* DamageCauser) override;

	/** Whole again on her back: intact, InMaxHealth of health, shootable, its pieces put away (her life starting over). */
	void ResetStone(float InMaxHealth);

	/** Cracks off now (its health gone, or her 33%: bByHerThird), if it's still on. */
	void Break(bool bByHerThird);

	/** Shootable or not (her death turns it off; it stays on her body). */
	void SetShootable(bool bShootable);

	EGravemotherStone GetState() const { return State; }
	bool IsOn() const { return State != EGravemotherStone::Broken; }
	/** Its health left as a share (0 once broken). */
	float GetHealthShare() const;
	UHealthComponent* GetHealthComponent() const { return Health; }
	/** The slab shown on her back (the part bullets hit). */
	UStaticMeshComponent* GetSlab() const { return Slab; }
	/** Its pieces shown now (falling or lying), and those still falling. */
	int32 NumShardsShown() const;
	int32 NumShardsFalling() const;
	/** Its look is the stand-in (its meshes aren't imported). */
	bool IsStandIn() const { return bStandIn; }

	/** Moves its pieces on (its tick; the tests call it). */
	void Advance(float DeltaSeconds);

	/** Its state's mesh index shown now (0 intact, 1 cracked, 2 shattered; -1 broken). */
	int32 GetShownMesh() const { return ShownMesh; }

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser);
	UFUNCTION()
	void HandleDeath(AController* Killer);

	/** Its state's mesh on the slab (or the stand-in, tinted darker as it cracks). */
	void ShowState();
	/** A shot's look on it: sparks off the bounce, a flash, stone dust, chips, the crack (not twice in a moment). */
	void PlayHitLook(const FVector& Where, const FVector& Normal, const FVector& ShotDirection);
	/** A shower of chips and dust off the slab (a crack, the break). */
	void Shower(int32 Chips, int32 Dust, float Speed);

	// --- The crack-off (GravemotherStoneBreak.cpp) ---
	void MakeShards();
	void StartShards();
	void HideShards();
	void TickShards(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, Category = "Stone")
	TObjectPtr<USceneComponent> Pivot;
	UPROPERTY(VisibleAnywhere, Category = "Stone")
	TObjectPtr<UStaticMeshComponent> Slab;
	UPROPERTY(VisibleAnywhere, Category = "Stone")
	TObjectPtr<UHealthComponent> Health;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Shards;

	/** One falling piece: where it sits in the slab's frame when whole, its flight, its floor. */
	struct FShard
	{
		FTransform InSlab;
		/** Its own middle in its mesh's space (it turns about it). */
		FVector Center = FVector::ZeroVector;
		FVector Location = FVector::ZeroVector;
		FQuat Rotation = FQuat::Identity;
		FVector Velocity = FVector::ZeroVector;
		FVector Spin = FVector::ZeroVector;
		float FloorZ = 0.f;
		bool bLanded = false;
		bool bBounced = false;
	};
	TArray<FShard> ShardStates;

	TWeakObjectPtr<AGravemotherCreature> Mother;
	FGravemotherStoneSettings Settings;
	EGravemotherStone State = EGravemotherStone::Intact;
	int32 ShownMesh = 0;
	bool bStandIn = false;
	/** The pieces' clock since the break (negative: none falling or lying). */
	float ShardTime = -1.f;
	/** Its hits' crack and flash: not twice within a moment (a shotgun's pellets make one). */
	double LastHitSound = -1.0;
	FVector LastHitSoundAt = FVector::ZeroVector;
	uint64 LastFlashFrame = 0;
};

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Creatures/CreatureReactRules.h"
#include "Creatures/CreatureTypes.h"
#include "CreatureReactComponent.generated.h"

class AController;
class UHealthComponent;
class UMaterialInstanceDynamic;
class UMeshComponent;

/**
 * A creature's reactions besides its brain's and its body's own (the 2026-10-14 polish; Docs/Pipeline.md, "Creatures"):
 *  - its notice beat (FCreatureNotice), which ACreatureBase's brain starts and holds it for (CreatureBaseReact.cpp);
 *  - the hit flash: a shot that lands flashes its body white (warmer on a crit) for under a tenth of a second, through
 *    M_FX_HitFlash worn as an overlay on its skinned meshes (as the training dummies and egg sacs flash), worn a moment
 *    longer at nothing so steady fire doesn't put it on and off every shot; never over an overlay that isn't its own;
 *  - the dust its feet kick up, the ash an Unpaid crumbles to and the dirt it rises through, thrown through FWeaponFX's
 *    pooled, instanced puffs under the level's budgets (UCreatureReactSubsystem), only near a player, tinted by the ground
 *    under it (FSlideDust's names: none on water, a haze on wood) and the light;
 *  - its own dice for idle life (seeded as play begins), so a crowd never moves in step.
 * No tick: the creature's frame moves it on (Advance), on the creature's own clock.
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API UCreatureReactComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCreatureReactComponent();

	/** Moves the notice beat and the hit flash on (the creature's frame). */
	void Advance(float DeltaSeconds);

	/** Its brain changed state: a hunt that's over ends its beat and lets the next hunt have one. */
	void HandleStateChanged(ECreatureState OldState, ECreatureState NewState);

	FCreatureNotice& GetNotice() { return Notice; }
	const FCreatureNotice& GetNotice() const { return Notice; }

	// --- The hit flash ---

	/** Flashes its body now (a shot landed). */
	void FlashHit(bool bCritical);
	const FCreatureHitFlash& GetFlash() const { return Flash; }
	/** Its flash's overlay is on its body now (for the tests). */
	bool IsFlashWorn() const { return bFlashWorn; }
	UMaterialInstanceDynamic* GetFlashLook() const { return FlashLook; }

	/** M_FX_HitFlash, the flash's material. */
	static const TCHAR* const FlashMaterialPath;

	// --- Dust, ash and grave dirt ---

	/**
	 * Puffs of dust from a foot landing at Foot (the ground) while it runs at Velocity, sized for a creature of Size (its
	 * size scale): how many were thrown (none far from every player, past the level's allowance, or on water).
	 */
	int32 KickFootfall(const FVector& Foot, const FVector& Velocity, float Size, int32 Puffs = 1);

	/** Ash motes from Where, drifting up and along Drift (cm/s): an Unpaid crumbling. How many were thrown. */
	int32 ShedAsh(const FVector& Where, const FVector& Drift, float Size, int32 Count);

	/** Dirt and dust thrown up round Feet (something rising out of the ground or climbing out of it), Strength 0 to 1. */
	void ThrowGraveDirt(const FVector& Feet, float Size, float Strength);

	/** Its dice for idle life (a tap, a glance, a bubble), and its seed (0 to 1) for lengths that differ by creature. */
	FRandomStream& GetRandom() { return Random; }
	float GetSeed() const { return Seed; }

	/** Footfall dust is thrown only for a player this near (cm); ash and grave dirt a little farther. */
	static constexpr float DustSeenWithin = 3500.f;
	static constexpr float AshSeenWithin = 5000.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser);

	/** Puts the flash's overlay on its skinned meshes, or takes it off (only its own). */
	void WearFlash(bool bWear);
	/** The ground under it now, as FSlideDust names it (looked at again every so often, from its movement's floor). */
	uint8 GroundNow();

	FCreatureNotice Notice;
	FCreatureHitFlash Flash;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FlashLook;
	TArray<TWeakObjectPtr<UMeshComponent>> FlashMeshes;
	bool bFlashWorn = false;

	TWeakObjectPtr<UHealthComponent> Health;
	FRandomStream Random;
	float Seed = 0.5f;

	/** The ground under it (an ESlideGround), and when it was looked at (world time). */
	uint8 Ground = 0;
	double GroundLookedAt = -1000.0;
};

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LootBeamCrowd.generated.h"

class AActor;
class UStaticMeshComponent;

/**
 * Where small drops (ammo, silver, grenades: the plain white 1.5 m beam, the user's rule for a lone drop) lie close
 * together, one beam stands for them all, and none stands within reach of a gun's beam: the Gravemother's loot shower
 * left fifteen white beams filling her den (2026-10-23, round 3). Guns keep theirs, always.
 */
namespace LootBeamCrowd
{
	/** Small drops this close (cm) share one beam: the first of them to land keeps it. */
	inline constexpr float MergeRadius = 400.f;
	/** No small drop's beam this close (cm) to a gun lying on the ground: the gun's beam shows the spot. */
	inline constexpr float NearGunRadius = 300.f;
	/** How often (s) the beams are sorted again while small drops are about. */
	inline constexpr float LookSeconds = 0.25f;

	/**
	 * Which small drops show their beam, in their order (the first to land first): one shows unless a gun lies within
	 * NearGun of it or one shown already lies within Merge of it.
	 */
	AI_LOOTER_SHOOTER_API TArray<bool> ChooseShown(TConstArrayView<FVector> Small, TConstArrayView<FVector> Guns, float Merge = MergeRadius,
		float NearGun = NearGunRadius);
}

/**
 * The level's small drops' beams (LootBeamCrowd): each joins as its play begins (Join) and leaves as it goes; four times a
 * second, while any are about, it hides the beams a nearer one or a gun stands for (hidden in game, so a drop's own
 * hiding, a silver pile's while it flies, is never undone). Nothing else changes: a beam's look, height and the drop.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ULootBeamCrowdSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Owner's beam joins the crowd (its BeginPlay calls it); it leaves by itself as Owner goes. */
	static void Join(AActor* Owner, UStaticMeshComponent* Beam);

	/** Sorts the beams again now (the tick does every LookSeconds; tests call it). */
	void Refresh();

	/** Played worlds, and the editor preview worlds the automated tests build. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

private:
	struct FEntry
	{
		TWeakObjectPtr<AActor> Owner;
		TWeakObjectPtr<UStaticMeshComponent> Beam;
		/** Hidden by the crowd (another beam stands for it), as last set. */
		bool bCrowdHidden = false;
	};
	TArray<FEntry> Entries;
	float Clock = 0.f;
};

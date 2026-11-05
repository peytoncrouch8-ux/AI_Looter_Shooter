#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LumenScatterSubsystem.generated.h"

class AActor;
class UInstancedStaticMeshComponent;
class ULevel;

/**
 * Keeps the small instanced scatter (grass, flowers, pebbles, clutter, cliff life: every instanced mesh under
 * SmallRadius) out of Lumen and the distance fields. Ransom's Rest has ~66,000 such instances; in Lumen's scene they
 * made its surface cache recapture thousands of tiny cards, so High and Epic stalled for over a second a frame on the
 * reference card (2026-10-14: Epic 1-2 fps -> 22-28 fps without them). Things that small add nothing to bounced light
 * or distance field shadows worth that. Applied as play begins and to any level streamed in later; Medium and Low use
 * neither Lumen nor the distance fields, so it changes nothing there.
 *
 * It also gives the shrubs their shadows only on High and Epic: the scatter tagged ShadowHighOnly (sagebrush,
 * rabbitbrush, bushes: Tools/Unreal/build_island_scatter.py) casts none while Looter.Scatter.HighShadows is 0, which the
 * graphics preset sets on Low and Medium (about 0.1 ms at the farm on the reference card, the user's pick 2026-11-04).
 * Applied with the rest, and again to every game world whenever the variable changes (a preset picked in the menu).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ULumenScatterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** An instanced mesh whose bounds' radius is under this (cm) is small scatter. Trees, rocks and buildings are bigger. */
	static constexpr float SmallRadius = 150.f;

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	/** Whether the part is small scatter by its mesh's size (not its instances' scale: a stretched pebble is still a pebble). */
	static bool IsSmallScatter(const UInstancedStaticMeshComponent& Part);

	/** Leaves the actor's small scatter out of Lumen and the distance fields; the number of parts changed. */
	static int32 LeaveOutOf(const AActor& Actor);

	/** The component tag of scatter that casts a shadow only on High and Epic. */
	static FName ShadowHighOnlyTag();

	/** Whether Looter.Scatter.HighShadows lets the ShadowHighOnly scatter cast shadows now (1, the default, outside a preset). */
	static bool HighShadowsAllowed();

	/** Gives the actor's ShadowHighOnly instanced parts their shadow or takes it away; the number of parts changed. */
	static int32 ApplyHighShadows(const AActor& Actor, bool bShadows);

private:
	void HandleLevelAdded(ULevel* Level, UWorld* World);

	FDelegateHandle LevelAddedHandle;
};

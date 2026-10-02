#pragma once

#include "CoreMinimal.h"
#include "Missions/MissionObjective.h"
#include "MissionPlayerObjectives.generated.h"

// The objectives about the player's own things: what they carry, and the inventory's pages.

/** What a Collect objective counts. */
UENUM(BlueprintType)
enum class EMissionCollect : uint8
{
	/** Guns the player carries in their slots; what they carry already counts (the tutorial's rifle). */
	Weapons,
	/** Things picked up that match Item, each once (a Collect event from the pickup). */
	Items,
};

/** Have or gather a number of things: guns carried, or items picked up. */
UCLASS(BlueprintType, meta = (DisplayName = "Collect"))
class AI_LOOTER_SHOOTER_API UMissionCollectObjective : public UMissionObjective
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Objective")
	EMissionCollect What = EMissionCollect::Items;

	/** The items that count, when collecting items. */
	UPROPERTY(EditAnywhere, Category = "Objective", meta = (EditCondition = "What == EMissionCollect::Items", EditConditionHides))
	FMissionActorFilter Item;

	UPROPERTY(EditAnywhere, Category = "Objective", meta = (ClampMin = "1"))
	int32 Count = 1;

	virtual int32 GetRequired() const override { return FMath::Max(Count, 1); }
	virtual void Update(const FMissionContext& Context, FMissionObjectiveState& State, float DeltaSeconds) const override;
	virtual bool HandleEvent(const FMissionContext& Context, FMissionObjectiveState& State, const FMissionEvent& Happened) const override;
	virtual FMissionActorFilter GetTargets() const override;
	/** While collecting guns, a gun rack's waypoint is the gun lying on it (the rack itself once it's taken). */
	virtual FVector GetWaypointOf(const AActor& Found) const override;
	virtual FString DescribeRule() const override;
};

/** The inventory's pages, as an objective names them. */
UENUM(BlueprintType)
enum class EMissionPage : uint8
{
	/** Any page: the inventory is open. */
	Any,
	Loadout,
	/** The bestiary (the Ledger, from Main 2). */
	Bestiary,
	Missions,
};

/** Open the inventory, on a page or any (the tutorial's last step; Main 2's Ledger). */
UCLASS(BlueprintType, meta = (DisplayName = "Open an inventory page"))
class AI_LOOTER_SHOOTER_API UMissionOpenPageObjective : public UMissionObjective
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Objective")
	EMissionPage Page = EMissionPage::Any;

	UMissionOpenPageObjective();

	virtual void Update(const FMissionContext& Context, FMissionObjectiveState& State, float DeltaSeconds) const override;
	virtual FString DescribeRule() const override;
};

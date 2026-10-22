#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Story/StoryCondition.h"
#include "BountyArena.generated.h"

class ULineBatchComponent;
class USceneComponent;

/**
 * A place in an area a bounty's fight can happen (Docs/Work/2026-10-22-Next/BOUNTY.md, "Arenas"): its ground, where each
 * push comes from, where the mark comes in and where its strongbox drops, when it's open (OpenWhen), the kinds of mark it
 * hosts. It does nothing itself: UBountyBoardSubsystem finds it and makes an ABountyFight from it (the fight copies its
 * ground and spots) when a bounty here is taken. Placed by Tools/Unreal/build_area_bounties.py in Gameplay/Bounties, its
 * properties named as an encounter spawner's (build_area_camps.set_ground fills them). Never ticks.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ABountyArena : public AActor
{
	GENERATED_BODY()

public:
	ABountyArena();

	/** Its id in postings ("Scatter"): never renamed (a saved posting names it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty")
	FName ArenaId;

	/** Its name on a card, after "Last seen:" ("the Scatter"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty")
	FText DisplayName;

	/** The kinds of mark it hosts (BountyRules::Kinds: "Unpaid", "Spider", "Spitter", "DeadMiner"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty")
	TArray<FName> Kinds;

	/** Bounties are drawn for it only while this holds (after the story's own fight here is done). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty")
	FStoryCondition OpenWhen;

	// --- Its ground (as an encounter spawner's) ---

	/** Its fight hunts only within this (cm) of it, unless it has GroundCorners. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty|Ground", meta = (ClampMin = "0", Units = "cm"))
	float GiveUpRadius = 1300.f;

	/** Its ground as a polygon (world X and Y, cm): a fence's line, a band of trees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty|Ground")
	TArray<FVector> GroundCorners;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty|Ground", meta = (ClampMin = "0", Units = "cm"))
	float GroundMargin = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty|Ground", meta = (ClampMin = "0", Units = "cm"))
	float GroundMaxRise = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty|Ground", meta = (ClampMin = "50", Units = "cm"))
	float MaxGroundStep = 300.f;

	// --- How it springs and where they come from ---

	/** It springs as the player walks onto its ground (its GroundCorners), not on coming near its middle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty|Spring")
	bool bSpringOnGround = false;

	/** Above 0: it springs on the player coming this near (cm); 0: the tuning's (Looter.Bounty.SpringRadius). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty|Spring", meta = (ClampMin = "0", Units = "cm"))
	float SpringRadius = 0.f;

	/** Its pushes come round where the player stood as it sprang (the Webwood's drop): the spot lists unused. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty|Spring")
	bool bAroundPlayer = false;

	/** Its spiders drop from above (the Webwood's trees) rather than coming their own way. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty|Spring")
	bool bDropFromAbove = false;

	/** It doesn't spring while any of these spawners (by id) is fighting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty|Spring")
	TArray<FName> WaitForSpawners;

	/** The first push's spots, relative to it (ahead of the way in). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty|Spots", meta = (MakeEditWidget = true))
	TArray<FVector> PushSpots;

	/** The middle pushes' spots (the flank, behind). Empty: the first push's. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty|Spots", meta = (MakeEditWidget = true))
	TArray<FVector> FlankSpots;

	/** The mark's push's spots: the mark's own first, its guards' round it. Empty: the first push's. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty|Spots", meta = (MakeEditWidget = true))
	TArray<FVector> MarkSpots;

	/** Where its strongbox drops, relative to it (on clear, level ground), and which way the box faces (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty|Spots", meta = (MakeEditWidget = true))
	FVector RewardSpot = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty|Spots")
	float RewardYaw = 0.f;

	/** Its id, or the actor's name. */
	FName GetArenaId() const { return ArenaId.IsNone() ? GetFName() : ArenaId; }

	/** Its name for a card: DisplayName, or its id. */
	FText GetDisplayName() const { return DisplayName.IsEmpty() ? FText::FromName(GetArenaId()) : DisplayName; }

	/** Where its strongbox drops, in the world. */
	FVector GetRewardLocation() const;

	/** Draws its ground, its spots (push ahead orange, flank cyan, mark gold-white) and the reward's spot, as lines in BatchID. */
	void DrawArena(ULineBatchComponent& Lines, uint32 BatchID) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Anchor;
};

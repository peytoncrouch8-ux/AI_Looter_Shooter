#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Story/StoryCondition.h"
#include "SoulFollowers.generated.h"

class UInstancedStaticMeshComponent;

/** What a freed soul is doing. */
UENUM()
enum class ESoulFollow : uint8
{
	/** Rising out of its cracked lump to the trail (a second or so). */
	Rising,
	/** Drifting after the lantern at Sully's hip. */
	Following,
	/** Waiting by the Keeper's Gate while Abel is fought (his lantern hangs on the keeper's post, steps 1-3). */
	Waiting,
	/** Carried off west on the Gravewind as Saint Ada wakes (SaintAdaWakes), then gone. */
	Released,
};

/**
 * The souls freed from the barge's fallen soul coal (Main 7, MAINS_2_7.md section 11: "Each lump cracks and a soul rises and
 * follows the lantern (a growing trail of wisps behind Sully; at the boards they wait by the Keeper's Gate)"; they follow
 * again to the chapel, and rise west with the six dead when Saint Ada wakes). Spawned by UCarriedLanternSubsystem the first
 * time a lump cracks (or a load finds lumps cracked), one per world, never saved: the campaign's SoulCoal.<Id> flags are.
 *
 * Each soul is a small pale cyan-gold glow (the game's additive glow on a camera-facing quad, as Saint Ada's ember and the
 * cold open's glows are drawn: one instanced mesh, one draw, no light, no shadow), springing toward its place: a loose
 * swaying line behind Sully (CarriedLanternRules::FollowerSlot), a slow ring at the Keeper's Gate, or the Gravewind's way
 * west (ReleasedAt). At most CarriedLanternRules::MaxFollowers of the coal's and the six dead's lights besides (Release's
 * extras); only drawn within DrawReach of the view, and it ticks only while it has souls.
 */
UCLASS(NotPlaceable)
class AI_LOOTER_SHOOTER_API ASoulFollowers : public AActor
{
	GENERATED_BODY()

public:
	ASoulFollowers();

	virtual void Tick(float DeltaSeconds) override;

	/** A soul freed at From rises and joins the trail (bAtOnce: it's there already, a load). Returns how many follow now. */
	int32 AddSoul(const FVector& From, bool bAtOnce = false);

	/**
	 * The Gravewind takes them (SaintAdaWakes): every follower, and Extra lights rising from those points too (the six dead
	 * off the boards), carried off west into the sunset and gone. Once.
	 */
	void Release(const TArray<FVector>& Extra);
	bool IsReleased() const { return bReleased; }

	/** Souls following or waiting (not released). */
	int32 NumFollowing() const;
	int32 NumSouls() const { return Souls.Num(); }
	ESoulFollow GetMode(int32 Index) const { return Souls.IsValidIndex(Index) ? Souls[Index].Mode : ESoulFollow::Released; }
	FVector GetSoulLocation(int32 Index) const { return Souls.IsValidIndex(Index) ? Souls[Index].At : FVector::ZeroVector; }

	/** Moves them on (the tick's work; a test level never ticks, so tests call it). Leader: who they follow (none: the player). */
	void Advance(float DeltaSeconds);

	/** They follow a leader during Main 7 except while Abel is fought, when they wait at the Keeper's Gate (WaitWhen). */
	FStoryCondition WaitWhen;

	/** Where they wait: the actor tagged this nearest the boards (Gravewind Point's place), found once. */
	FName WaitTag;

	/** Who they follow (none: the first player's pawn), for the tests and the scenes. */
	TWeakObjectPtr<AActor> Leader;

	/** Drawn only with the view this near their middle (cm). */
	static constexpr float DrawReach = 9000.f;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Glows;

protected:
	virtual void BeginPlay() override;

private:
	struct FSoul
	{
		FVector At = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		FVector From = FVector::ZeroVector;
		ESoulFollow Mode = ESoulFollow::Rising;
		float Clock = 0.f;
		float Glow = 0.f;
		int32 Slot = 0;
		/** Put straight in its place next move (a loaded trail). */
		bool bSnap = false;
	};

	bool ShouldWait() const;
	FVector WaitSpot();
	/** Draws every soul as a quad facing the view (or hides them all, out of reach). */
	void Draw();

	TArray<FSoul> Souls;
	bool bReleased = false;
	float Clock = 0.f;
	bool bWaitFound = false;
	FVector FoundWait = FVector::ZeroVector;
	/** The story's waiting word, read a few times a second. */
	float WaitCheckLeft = 0.f;
	bool bWaitCached = false;
};

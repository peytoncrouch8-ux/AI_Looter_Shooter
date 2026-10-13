#pragma once

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/HuntingGround.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "BellRingerRules.generated.h"

/** What a Bell-Ringer's toll does when it lands (BellRingerRules::ChooseToll). */
enum class EBellToll : uint8
{
	/** No toll has landed yet. */
	None,
	/** It raised one of the Unpaid fallen near it lately. */
	Raise,
	/** None fallen to raise: it steadied its pack (a short haste, UBellRingerSteady). */
	Steady,
};

/** What it does about its target's distance while it hunts (BellRingerRules::ChooseStance). */
enum class EBellRingerStance : uint8
{
	/** Farther than it holds: it drifts nearer (the creature brain's chase does it). */
	Approach,
	/** Where it wants to be, behind its pack: it stands, faces them and tolls. */
	Hold,
	/** Too near: it backs off, a little at a time. */
	BackOff,
};

/**
 * The Bell-Ringer's numbers (ABellRingerCreature; Docs/Polish/BorderlandsComparison.md, item 8): where it stands, its
 * toll, what the toll raises or steadies, and its bell as a target. Distances are the design's whatever its size, as the
 * Unpaid's phase-step's are.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FBellRingerTuning
{
	GENERATED_BODY()

	// --- Hanging back ---

	/** It comes no nearer its target than this (cm): it stands there behind its pack and tolls. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stance", meta = (ClampMin = "100", Units = "cm"))
	float HoldDistance = 1100.f;

	/** A target nearer than this sends it backing off... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stance", meta = (ClampMin = "0", Units = "cm"))
	float BackOffBelow = 750.f;

	/** ...until it's this far again, for at most BackOffSeconds; then it holds BackOffRest before backing off again. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stance", meta = (ClampMin = "0", Units = "cm"))
	float BackOffUntil = 1050.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stance", meta = (ClampMin = "0", Units = "s"))
	float BackOffSeconds = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stance", meta = (ClampMin = "0", Units = "s"))
	float BackOffRest = 3.f;

	/** Its pace backing off, against its chase speed: slower than a player, so it can be run down. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stance", meta = (ClampMin = "0.1", ClampMax = "1.5"))
	float BackOffSpeedShare = 0.8f;

	// --- Cornered: the swipe ---

	/** Run down to this near (cm), it swipes with the bell; the swipe lands within SwipeReach in front of it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swipe", meta = (ClampMin = "50", Units = "cm"))
	float SwipeRange = 260.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swipe", meta = (ClampMin = "50", Units = "cm"))
	float SwipeReach = 300.f;

	// --- The toll ---

	/** It tolls only with its target this near (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toll", meta = (ClampMin = "100", Units = "cm"))
	float TollRange = 2400.f;

	/** Seconds from turning on a player to its first toll, and from one toll to the next (a Restless one's are shorter). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toll", meta = (ClampMin = "0", Units = "s"))
	float FirstTollDelay = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toll", meta = (ClampMin = "0.5", Units = "s"))
	float TollInterval = 7.f;

	/** The tell: the bell raised high and shaken, a halo of light gathering round it, before the toll lands (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toll", meta = (ClampMin = "0.2", Units = "s"))
	float TellSeconds = 1.3f;

	/** After the toll it lowers the bell over this long (s), standing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toll", meta = (ClampMin = "0", Units = "s"))
	float LowerSeconds = 0.7f;

	/** Its bell shot in the tell: it reels this long (s), the toll lost, and its next waits a whole interval. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Toll", meta = (ClampMin = "0", Units = "s"))
	float ReelSeconds = 1.1f;

	// --- What a toll raises ---

	/** One of the Unpaid fallen within this of it (cm) in the last FallenMaxAge seconds can be raised... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raise", meta = (ClampMin = "0", Units = "cm"))
	float RaiseRadius = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raise", meta = (ClampMin = "0", Units = "s"))
	float FallenMaxAge = 20.f;

	/** ...but no more than this many by one ringer (a life: placed ones start again when they come back). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raise", meta = (ClampMin = "0"))
	int32 MaxRaises = 2;

	/** A raised one comes back with this share of its health, and is worth this share of its experience. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raise", meta = (ClampMin = "0.05", ClampMax = "1"))
	float RaisedHealthShare = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raise", meta = (ClampMin = "0", ClampMax = "1"))
	float RaisedXPShare = 0.5f;

	/** Never one at the player's feet: a fallen nearer them than this (cm) stays down. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raise", meta = (ClampMin = "0", Units = "cm"))
	float RaiseClearOfPlayer = 300.f;

	/** Nor one fallen on another level of ground: no more than this above or below the ringer (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Raise", meta = (ClampMin = "0", Units = "cm"))
	float RaiseMaxRise = 300.f;

	// --- What a toll steadies ---

	/** None to raise: every living Unpaid within this of it (cm, itself too) is quickened for SteadySeconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steady", meta = (ClampMin = "0", Units = "cm"))
	float SteadyRadius = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steady", meta = (ClampMin = "0", Units = "s"))
	float SteadySeconds = 6.f;

	/** The quickening: its chase this much faster, the wait between its attacks this share as long. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steady", meta = (ClampMin = "1", ClampMax = "3"))
	float SteadySpeedScale = 1.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steady", meta = (ClampMin = "0.1", ClampMax = "1"))
	float SteadyCooldownScale = 0.6f;

	// --- The bell as a target ---

	/** The bell's hit zone (cm at size 1): a ball round its body, generous for a weak spot shot at from 10 m. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bell", meta = (ClampMin = "2", Units = "cm"))
	float BellHitRadius = 14.f;

	/** A Gravebound ringer's cracked toll sends a ring this far (cm) that slows the player, as its kind's shriek does. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bell", meta = (ClampMin = "100", Units = "cm"))
	float CrackedRingRadius = 1200.f;
};

/** One of the Unpaid fallen lately, as UBellRingerGraves keeps it: enough to raise it again where it fell. */
struct FBellRingerFallen
{
	/** The fallen body's id (its object's unique id), so each death is noted once. */
	uint32 Id = 0;
	TWeakObjectPtr<UClass> Class;
	FVector Feet = FVector::ZeroVector;
	float Yaw = 0.f;
	ECreatureRank Rank = ECreatureRank::Basic;
	/** Its level before its rank's (what a spawn keeps), and its size against its model (BodyScale). */
	int32 OwnLevel = 1;
	float BodyScale = 1.f;
	/** Where it fought, its middle made fixed: a raised one's home is its grave, not the spot it first stood on. */
	FHuntingGround Ground;
	/** When it fell (world seconds), and whether a toll has raised it already. */
	double DiedAt = 0.0;
	bool bRaised = false;
};

/** The Bell-Ringer's rules as plain functions, so the tests check them without a level (ABellRingerCreature plays them). */
namespace BellRingerRules
{
	/**
	 * Its stance at Distance (cm, seen from above) from its target: backing off when they come within BackOffBelow (once
	 * BackOffRest has passed since it last did), until BackOffUntil or BackOffSeconds; holding at HoldDistance or nearer;
	 * approaching beyond it.
	 */
	AI_LOOTER_SHOOTER_API EBellRingerStance ChooseStance(const FBellRingerTuning& Tuning, float Distance, bool bBackingOff, float BackOffTime,
		float SinceBackOff);

	/** Run down to within its swipe's range (cm at size 1, times SizeScale): it swipes. */
	AI_LOOTER_SHOOTER_API bool CanSwipe(const FBellRingerTuning& Tuning, float Distance, float SizeScale);

	/** Seconds between its tolls at its rank: the Restless and up ring sooner. */
	AI_LOOTER_SHOOTER_API float TollInterval(const FBellRingerTuning& Tuning, ECreatureRank Rank);

	/** A Gravebound (or Soulfed) ringer's bell is cracked: a sour toll, and its ring slows the player. */
	AI_LOOTER_SHOOTER_API bool IsCrackedBell(ECreatureRank Rank);

	/** Raise when there's a fallen to raise, room under the caps and raises left; else steady the pack. */
	AI_LOOTER_SHOOTER_API EBellToll ChooseToll(bool bFallenToRaise, int32 RaisedSoFar, int32 MaxRaises, bool bRoomForMore);

	/**
	 * Whether a toll from Ringer can raise Fallen now: not raised yet, fallen no more than MaxAge ago, within Radius of the
	 * ringer (seen from above) and MaxRise above or below it, and at least ClearOfPlayer from the player.
	 */
	AI_LOOTER_SHOOTER_API bool IsRaisable(const FBellRingerFallen& Fallen, const FVector& Ringer, double Now, const FVector& Player,
		const FBellRingerTuning& Tuning);

	/** The one a toll raises: the nearest raisable to the ringer (the most recent of two as near); INDEX_NONE for none. */
	AI_LOOTER_SHOOTER_API int32 ChooseFallen(TConstArrayView<FBellRingerFallen> Fallen, const FVector& Ringer, double Now, const FVector& Player,
		const FBellRingerTuning& Tuning);

	/** Whether a hit at Point landed on the bell: on its ball, Radius round its middle, give or take a little. */
	AI_LOOTER_SHOOTER_API bool IsBellHit(const FVector& BellCenter, float Radius, const FVector& Point);

	/** Ways to back off from Target, best first: straight away, then 35 and 70 degrees to either side (flat, unit). */
	AI_LOOTER_SHOOTER_API TArray<FVector> BackOffDirections(const FVector& Here, const FVector& Target);

	/**
	 * Where a fist holds a prop's handle, from its wrist (the hand bone's head) and its middle knuckle: most of the way out
	 * along the hand, where the curled fingers close round it.
	 */
	AI_LOOTER_SHOOTER_API FVector GripPoint(const FVector& Wrist, const FVector& Knuckle);
}

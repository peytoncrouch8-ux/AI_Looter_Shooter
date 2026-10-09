#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"

class AActor;
class ACameraActor;
class ACreatureBase;
class APawn;
class APlayerController;
class UPrimitiveComponent;
class UWorld;

/**
 * The cast Looter.CastShots photographs (CastShotDevCommands.cpp runs the pictures and the numbers): every creature kind
 * spawned on flat ground and on a slope in each of its states, packs, the player's body, and the story's characters where
 * they stand. This file says who, where, how each state is made through the game's own paths, which bones the numbers
 * watch, and where the camera goes. Developer builds only (the .cpp is empty in shipping).
 */
namespace CastShotScene
{
	/** What a subject is doing in its pictures. */
	enum class EState : uint8
	{
		Idle,
		Walk,
		Chase,
		Windup,
		Hurt,
		Death,
		/** A character as the level has it. */
		InPlace,
		/** Amos leaning on his rail, and sitting on it. */
		Lean,
		Sit,
		/** A pack closing on the player together. */
		Pack,
	};
	const TCHAR* StateName(EState State);

	/** Where a subject's pictures are taken. */
	enum class EPlace : uint8
	{
		Flat,
		Slope,
		/** Where a character stands in the level. */
		InPlace,
	};
	const TCHAR* PlaceName(EPlace Place);

	/** One subject: a creature kind to spawn (alone or as a pack), the player's body, or a character found in the level. */
	struct FSubject
	{
		/** Its name in the pictures' files (spider, unpaid-gravebound, sexton). */
		FString Name;
		/** A creature kind, spawned for its pictures (null for the player and the characters). */
		UClass* Class = nullptr;
		ECreatureRank Rank = ECreatureRank::Basic;
		bool bSpiderling = false;
		/** More than 0: a pack of this many on flat ground. */
		int32 PackCount = 0;
		bool bPlayer = false;
		/** A character in the level, photographed where it stands. */
		TWeakObjectPtr<AActor> Found;
		TArray<EPlace> Places;
		TArray<EState> States;
	};

	/** Everyone to photograph, in order. Only: name prefixes to keep (empty keeps everyone). */
	TArray<FSubject> Subjects(UWorld& World, TConstArrayView<FString> Only);

	/** The two places creatures are photographed: open flat ground, and an open slope (with the way up it). */
	struct FSpots
	{
		bool bFlat = false;
		FVector Flat = FVector::ZeroVector;
		bool bSlope = false;
		FVector Slope = FVector::ZeroVector;
		float SlopeDegrees = 0.f;
		/** Up the slope, level. */
		FVector Uphill = FVector::ForwardVector;
	};

	/**
	 * Open ground near Around: flat (under 5 degrees) and a slope (16 to 32, nearest 24), each on terrain tagged Ground,
	 * even, with nothing solid within 4 m, nothing over it (water, a roof) and outside every safe zone. Given spots (the
	 * command's flat= and slope=) are used as they are.
	 */
	FSpots FindSpots(UWorld& World, const FVector& Around, const FVector* GivenFlat, const FVector* GivenSlope);

	/** The ground under a spot (world-static, from 20 m over it), or false. */
	bool GroundAt(UWorld& World, const FVector& Spot, FVector& OutGround);

	/**
	 * A level way from Spot that Walker's body (its capsule) can walk Distance along: walkable ground all the way and nothing
	 * in its path. Ahead if it's clear, else the nearest turn from it that is; Ahead if none is. (The player walked into a
	 * cliff and a fence for its pictures: its feet ran on while its body stood, and read as sliding.)
	 */
	FVector ClearWay(UWorld& World, const APawn& Walker, const FVector& Spot, const FVector& Ahead, float Distance);

	/** Stops a pawn put somewhere new: it kept the last step's walking speed and ran on, feet skating, as it stood. */
	void StopMoving(APawn& Pawn);

	/**
	 * Stands the player on Spot facing Yaw, still, its controller looking that way. With a WalkDistance (cm) it faces along a
	 * clear way for that walk instead (ClearWay from InOutAhead), and InOutAhead becomes that way.
	 */
	void StandPlayer(UWorld& World, APawn& Player, APlayerController* Controller, const FVector& Spot, float Yaw, float WalkDistance,
		FVector& InOutAhead);

	/** A creature of Subject standing at Feet facing Yaw, held back (it hunts nobody until a state sets it on), dropping no loot. */
	ACreatureBase* Spawn(UWorld& World, const FSubject& Subject, const FVector& Feet, float Yaw);

	/**
	 * Sets State up on Creature through the game's own paths (its brain put in the state, damage dealt as a gun deals it),
	 * with Player put where the state needs them (Ahead: the way it should go). Returns how long to let it run before the
	 * picture (seconds), or a negative number when the state can't be made here.
	 */
	float Start(UWorld& World, ACreatureBase& Creature, EState State, APawn& Player, const FVector& Ahead);

	/** The bones the numbers watch on a body: its feet (and how near the ground a planted one is), its limbs and body parts, what trails behind it. */
	struct FBodyParts
	{
		TArray<FName> Feet;
		float PlantedHeight = 4.f;
		TArray<FName> Limbs;
		TArray<FName> Body;
		TArray<FName> Trailing;
		/** A slime: its gel's foot is measured as a ring round its body bone. */
		bool bSlimeFoot = false;
	};
	FBodyParts PartsOf(const AActor& Subject);

	/** The body to measure and frame: its skinned mesh with a model, else its biggest static mesh. */
	UPrimitiveComponent* BodyOf(const AActor& Subject);

	/** The box the camera frames round the subjects' bodies. */
	FBox BoundsOf(TConstArrayView<const AActor*> Subjects);

	/** How many angles each picture is taken from, and their names. */
	constexpr int32 AngleCount = 3;
	const TCHAR* AngleName(int32 Angle);

	/**
	 * Puts Camera on Bounds from Angle (0: in front, a little to its right and above; 1: its right side; 2: high from behind
	 * on its left) round FacingYaw, as far as a 40 degree view needs, and nearer when a wall is in the way. Ignored: actors
	 * the line of sight passes through.
	 */
	void Frame(UWorld& World, ACameraActor& Camera, const FBox& Bounds, float FacingYaw, int32 Angle, TConstArrayView<const AActor*> Ignored);
}

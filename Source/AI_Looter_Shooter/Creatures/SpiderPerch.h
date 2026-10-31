#pragma once

#include "CoreMinimal.h"
#include "SpiderPerch.generated.h"

/** What a lurking spider holds on to (FSpiderPerchSpot::Kind). */
UENUM(BlueprintType)
enum class ESpiderPerchKind : uint8
{
	/** A cliff face, a rock wall, a quarry wall: it hangs head down on it. */
	Wall,
	/** The top of a rock or a ledge: it crouches low at its edge, looking over. */
	Ledge,
	/** A tree's trunk: it hangs head down on the trunk's side. */
	Tree,
};

/**
 * One lurking spider's place in a lurk ambush (AAmbushSpawner, Entrance Lurk; the spider takes it up in
 * ASpiderCreature::BeginLurk, SpiderCreatureLurk.cpp). The build (Tools/Unreal/build_area_lurks.py) writes it from the layout's
 * gameplay.encounters.lurks; the spider finds the real surface near Spot as it takes its perch, so a rebuilt rock or wall
 * still holds it, and one that finds nothing lurks crouched on the ground where it was spawned instead.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FSpiderPerchSpot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perch")
	ESpiderPerchKind Kind = ESpiderPerchKind::Wall;

	/**
	 * Where it is looked for (world, cm). A wall: a point at the perch's height on the wall's foot line, or just in front of
	 * the face (the face is found looking back along -FaceYaw from 1.5 m out of it); a tree: on the trunk's axis at the
	 * perch's height; a ledge: over its top (the top is found looking down).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perch")
	FVector Spot = FVector::ZeroVector;

	/** Which way the wall faces (degrees: 0 is +X, 90 is +Y); a tree: the side of the trunk it hangs on; a ledge: the way it looks over the edge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perch")
	float FaceYaw = 0.f;

	/** It springs when the player comes within this (cm, flat, from the ground under it). 0: the default (Looter.Spider.Lurk.SpringRadius). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perch", meta = (ClampMin = "0", Units = "cm"))
	float SpringRadius = 0.f;
};

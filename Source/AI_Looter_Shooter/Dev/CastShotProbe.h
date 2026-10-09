#pragma once

#include "CoreMinimal.h"

class AActor;
class UPrimitiveComponent;
class USkinnedMeshComponent;
class UWorld;

/**
 * The numbers Looter.CastShots takes beside its pictures (CastShotDevCommands.cpp), for what pictures miss: feet in or over
 * the ground, limbs inside the body, feet sliding, bodies in each other or in the level, pops. Each measure is plain and
 * reads only what the game shows (bone transforms, the models' hit hulls, world-static geometry), so the creature
 * animation tests use them too. Developer builds only.
 */
namespace CastShotProbe
{
	/**
	 * The ground (world-static: terrain and solid props, never volumes or Ignored) under Point, from Above over it to Below
	 * under it. A surface over the point that it hangs under in open air (a fence's top rail over a sitting ghost's coat
	 * tail, a roof) isn't its ground: the ground is what lies below it then. Only a point inside the solid counts as under.
	 */
	bool GroundUnder(const UWorld& World, const FVector& Point, float Above, float Below, const AActor* Ignored, FVector& OutGround);

	/** How points sit against the ground under each: the deepest under it and the highest over it (cm, both 0 or more), and which. */
	struct FGroundGap
	{
		float Under = 0.f;
		FString UnderPart;
		float Over = 0.f;
		FString OverPart;
		int32 Measured = 0;
	};

	/** Points (world) against the ground under each; Names say which is which. */
	FGroundGap PointsAgainstGround(const UWorld& World, TConstArrayView<FVector> Points, TConstArrayView<FString> Names, const AActor* Ignored);

	/** Bones of Mesh (world) against the ground under each; bones it lacks are skipped. */
	FGroundGap BonesAgainstGround(const UWorld& World, const USkinnedMeshComponent& Mesh, TConstArrayView<FName> Bones, const AActor* Ignored);

	/** One body part inside another: how deep (cm), which part reaches in, and into what. */
	struct FIntrusion
	{
		float Depth = 0.f;
		FString Part;
		FString Into;
	};

	/**
	 * How deep Limbs' hit hulls reach into BodyParts' hulls on Mesh past where they reach in the rest pose (the model's own
	 * overlaps at the joints don't count): an arm through a torso, a leg through an abdomen. Against the part a limb hangs
	 * from (an upper arm's chest, a femur's thorax) the third of the limb nearest its joint is left out: a turning joint's
	 * sleeve folds into what it hangs from by design, as the Unpaid's own arm test has it.
	 */
	FIntrusion LimbsIntoBody(const USkinnedMeshComponent& Mesh, TConstArrayView<FName> Limbs, TConstArrayView<FName> BodyParts);

	/** How deep any of Mesh's hit hulls reach into any of Other's (two creatures standing in each other). */
	FIntrusion BodyIntoBody(const USkinnedMeshComponent& Mesh, const USkinnedMeshComponent& Other);

	/** A body against the level where it stands: how many of its points are inside world-static geometry, the deepest, and in what. */
	struct FLevelHit
	{
		float Depth = 0.f;
		FString Part;
		FString Into;
		int32 Inside = 0;
		int32 Measured = 0;
	};

	/**
	 * Body's points (a skinned mesh's hit hulls as posed, or its bones when it has none; a static mesh's simple collision)
	 * against world-static geometry other than Ignored's: a point is inside when every way out of it within 30 cm meets a
	 * surface, and its depth is the shortest of those ways.
	 */
	FLevelHit AgainstLevel(const UWorld& World, const UPrimitiveComponent& Body, const AActor* Ignored);

	/** Feet sliding while planted: watched frame by frame through a walk. */
	class FFootSlide
	{
	public:
		/**
		 * A foot within PlantedHeight (cm) of the ground for MinPlantedSeconds running counts as planted, and its slide over
		 * the ground from then on is summed. A foot only brushing the ground as a step lands or lifts off (still moving, as
		 * it should) isn't planted yet.
		 */
		void Sample(const UWorld& World, const USkinnedMeshComponent& Mesh, TConstArrayView<FName> Feet, float PlantedHeight,
			const AActor* Ignored, float DeltaSeconds);
		/** Forget which feet were down (a body that stopped standing on them: dead, curling up). */
		void Lift() { WasPlanted.Reset(); PlantedFor.Reset(); }

		static constexpr float MinPlantedSeconds = 0.05f;

		/** Planted feet's average speed over the ground (cm/s), the worst one frame showed, and which foot. */
		float MeanSlide() const { return PlantedSeconds > 0.f ? SlideDistance / PlantedSeconds : 0.f; }
		float WorstSlide() const { return Worst; }
		const FString& WorstFoot() const { return WorstName; }
		float GetPlantedSeconds() const { return PlantedSeconds; }

	private:
		TMap<FName, FVector> Last;
		TMap<FName, bool> WasPlanted;
		/** How long each foot has been near the ground, running (seconds). */
		TMap<FName, float> PlantedFor;
		float SlideDistance = 0.f;
		float PlantedSeconds = 0.f;
		float Worst = 0.f;
		FString WorstName;
	};

	/**
	 * Pops: the biggest change any bone's motion in the world makes from one frame to the next (cm/s over 60: cm per frame
	 * as at 60 frames a second). At 60 frames a second a bone that jumps D cm in one frame and stops reads D, as it did when
	 * this watched jumps against the body; a foot swinging fast but smoothly, or a planted foot while the body turns over
	 * it, reads little. (Measured against the body it read every planted foot of a turning spider and every fast step as a
	 * pop.) A body teleported (a phase-step) starts the watch over.
	 */
	class FPopWatch
	{
	public:
		void Sample(const USkinnedMeshComponent& Mesh, float DeltaSeconds);
		/** Forget the last frames (a new body, or a cut). */
		void Restart() { Last.Reset(); LastVelocity.Reset(); }

		float Worst = 0.f;
		FString WorstBone;

	private:
		/** Each bone's place in the world last frame and its velocity then (cm/s), and the body's place. */
		TArray<FVector> Last;
		TArray<FVector> LastVelocity;
		FVector LastBody = FVector::ZeroVector;
	};
}

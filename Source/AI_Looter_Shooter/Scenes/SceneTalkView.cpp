// SceneTalkView: a scene's view of someone talking, seen whole and in plain sight (see the header).

#include "Scenes/SceneTalkView.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"

namespace SceneTalkView
{
	namespace
	{
		/** Room round them in the frame: their height times this fills it top to bottom. */
		constexpr float FrameRoom = 1.3f;
		/** The look point sits this share of their standing height over their feet (a little over the middle: room over the hat). */
		constexpr float LookShare = 0.55f;
		/** The turns tried round them from their front (degrees), and the farther ring's reach (times the framing distance). */
		constexpr float Turns[] = { 0.f, 30.f, -30.f, 60.f, -60.f, 90.f, -90.f, 120.f, -120.f, 150.f, -150.f, 180.f };
		constexpr float FartherRing = 1.3f;
		/** The lens's ball (its near plane kept off a board) and how far under the lens ground is looked for (cm). */
		constexpr float LensRadius = 15.f;
		constexpr float GroundBelow = 400.f;
		/** Their shins (over the boards they stand on, so the deck itself isn't in the way) and inside their top (cm). */
		constexpr float ShinHeight = 25.f;
		constexpr float TopInset = 10.f;

		struct FJudged
		{
			int32 Seen = 0;
			bool bPathClear = false;
			bool bOverGround = false;

			bool IsWhole(int32 Targets) const { return Seen == Targets && bPathClear && bOverGround; }
			int32 Score() const { return Seen * 2 + (bPathClear ? 2 : 0) + (bOverGround ? 1 : 0); }
		};
	}

	float FramingDistance(float Height)
	{
		// Up-down half angle of a Fov-wide 16:9 frame.
		const float HalfUp = FMath::Atan(FMath::Tan(FMath::DegreesToRadians(Fov * 0.5f)) * 9.f / 16.f);
		return FMath::Max(Height, 1.f) * FrameRoom * 0.5f / FMath::Tan(HalfUp);
	}

	FVector LookPoint(const FFigure& Figure)
	{
		return Figure.Feet + FVector(0.0, 0.0, Figure.StandTop * LookShare);
	}

	TArray<FVector> Spots(const FFigure& Figure, float MinDistance)
	{
		const float Distance = FMath::Max(MinDistance, FramingDistance(Figure.StandTop));
		const FVector Look = LookPoint(Figure);
		TArray<FVector> Out;
		for (const float Ring : { 1.f, FartherRing })
		{
			for (const float Turn : Turns)
			{
				const FVector Out2D = FRotator(0.f, Figure.FacingYaw + Turn, 0.f).Vector();
				Out.Add(Look + Out2D * (Distance * Ring));
			}
		}
		return Out;
	}

	FTransform Find(const UWorld* World, const FFigure& Figure, float MinDistance)
	{
		const TArray<FVector> Tried = Spots(Figure, MinDistance);
		const FVector Look = LookPoint(Figure);
		auto ViewFrom = [&Look](const FVector& Spot) { return FTransform((Look - Spot).Rotation(), Spot); };
		if (!World || Tried.IsEmpty())
		{
			return ViewFrom(Tried.IsEmpty() ? Look - FVector(FramingDistance(Figure.StandTop), 0.0, 0.0) : Tried[0]);
		}

		// The level's solid things, placed ones too (the deck's rails and boards), not volumes; them left out.
		FCollisionObjectQueryParams Solid;
		Solid.AddObjectTypesToQuery(ECC_WorldStatic);
		Solid.AddObjectTypesToQuery(ECC_WorldDynamic);
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("SceneTalkView"), Figure.Who, /*bTraceComplex*/ false);
		const FVector Up = FVector::UpVector;
		const FVector Targets[] = {
			Figure.Feet + Up * ShinHeight,
			Figure.Feet + Up * (Figure.StandTop * 0.5f),
			Figure.Feet + Up * FMath::Max(Figure.LowTop - TopInset, ShinHeight),
			Figure.Feet + Up * FMath::Max(Figure.StandTop - TopInset, ShinHeight),
		};
		constexpr int32 NumTargets = UE_ARRAY_COUNT(Targets);

		int32 Best = 0;
		int32 BestScore = -1;
		for (int32 Index = 0; Index < Tried.Num(); ++Index)
		{
			const FVector& Spot = Tried[Index];
			FJudged Judged;
			FHitResult Hit;
			for (const FVector& Target : Targets)
			{
				Judged.Seen += World->LineTraceSingleByObjectType(Hit, Spot, Target, Solid, Params) ? 0 : 1;
			}
			// The lens itself clear of a wall or a rock (swept out from them, as a boss intro's boom is).
			Judged.bPathClear = !World->SweepSingleByObjectType(Hit, Look, Spot, FQuat::Identity, Solid, FCollisionShape::MakeSphere(LensRadius), Params);
			Judged.bOverGround = World->LineTraceSingleByObjectType(Hit, Spot, Spot - Up * (Spot.Z - Figure.Feet.Z + GroundBelow), Solid, Params);
			if (Judged.IsWhole(NumTargets))
			{
				return ViewFrom(Spot);
			}
			if (Judged.Score() > BestScore)
			{
				BestScore = Judged.Score();
				Best = Index;
			}
		}
		return ViewFrom(Tried[Best]);
	}
}

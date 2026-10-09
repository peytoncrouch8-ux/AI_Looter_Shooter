// Where Looter.CastShots photographs creatures (CastShotScene.h): open flat ground and an open slope near the player,
// found by looking at the ground ring by ring outward; the player stood there, still, and a clear way for it to walk.
// Developer builds only.

#include "Dev/CastShotScene.h"
#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Creatures/EncounterSubsystem.h"
#include "World/WorldQueries.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The search: rings this far apart out to this far, a sample every RingStep along each. */
	constexpr float RingStep = 400.f;
	constexpr float SearchRadius = 12000.f;
	/** Open: nothing solid but the ground within this far of a spot (cm). */
	constexpr float OpenRadius = 400.f;
	/** The slope's span either side of a spot, and how uneven ground may be (cm) to count as flat, or as an even slope. */
	constexpr float SlopeSpan = 200.f;
	constexpr float FlatUneven = 8.f;
	constexpr float SlopeUneven = 15.f;
	/** The slope wanted (degrees), and the range taken. */
	constexpr float WantedSlope = 24.f;
	constexpr float SlopeRange[2] = { 16.f, 32.f };
	/** A clear way is looked along a step at a time (cm), the body swept this far over the ground (its step-up height). */
	constexpr float SweepStep = 100.f;
	constexpr float StepLift = 35.f;

	FCollisionQueryParams GroundParams(const UWorld& World)
	{
		return LooterWorld::StaticGeometryParams(&World, TEXT("CastShotSpot"));
	}

	bool IsGroundActor(const AActor* Actor)
	{
		return Actor && Actor->ActorHasTag(TEXT("Ground"));
	}

	/** A spot's ground, its slope (degrees), its way up, how uneven it is, and whether it's open and outside every safe zone. */
	struct FSpotLook
	{
		FVector Ground = FVector::ZeroVector;
		float Degrees = 0.f;
		FVector Uphill = FVector::ForwardVector;
		float Uneven = 0.f;
		bool bUsable = false;
	};

	FSpotLook LookAt(UWorld& World, const FVector& Spot)
	{
		FSpotLook Look;
		const FCollisionQueryParams Params = GroundParams(World);
		const FCollisionObjectQueryParams Static(ECC_WorldStatic);
		FHitResult Middle;
		if (!World.LineTraceSingleByObjectType(Middle, Spot + FVector(0.0, 0.0, 2000.0), Spot - FVector(0.0, 0.0, 4000.0), Static, Params)
			|| !IsGroundActor(Middle.GetActor()))
		{
			return Look;
		}
		Look.Ground = Middle.ImpactPoint;
		// Four points round it make its plane; the middle's height off that plane says how uneven it is.
		FVector Points[4];
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const FVector Offset(Index == 0 ? SlopeSpan : (Index == 1 ? -SlopeSpan : 0.f), Index == 2 ? SlopeSpan : (Index == 3 ? -SlopeSpan : 0.f), 0.f);
			FHitResult Hit;
			if (!World.LineTraceSingleByObjectType(Hit, Look.Ground + Offset + FVector(0.0, 0.0, 600.0), Look.Ground + Offset - FVector(0.0, 0.0, 600.0),
				Static, Params) || !IsGroundActor(Hit.GetActor()))
			{
				return Look;
			}
			Points[Index] = Hit.ImpactPoint;
		}
		FVector Normal = FVector::CrossProduct(Points[0] - Points[1], Points[2] - Points[3]).GetSafeNormal();
		Normal *= Normal.Z < 0.0 ? -1.0 : 1.0;
		Look.Degrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(static_cast<float>(Normal.Z), -1.f, 1.f)));
		Look.Uphill = (-FVector(Normal.X, Normal.Y, 0.0)).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
		const FVector Average = (Points[0] + Points[1] + Points[2] + Points[3]) * 0.25;
		Look.Uneven = static_cast<float>(FMath::Abs(FVector::DotProduct(Look.Ground - Average, Normal)));

		// Nothing over it (water, a roof, a tree) and nothing solid near it but the ground; no safe zone (nothing hunts there).
		FHitResult Over;
		const FCollisionQueryParams SightParams(SCENE_QUERY_STAT(CastShotOver), false);
		if (World.LineTraceSingleByChannel(Over, Look.Ground + FVector(0.0, 0.0, 1500.0), Look.Ground + FVector(0.0, 0.0, 5.0), ECC_Visibility, SightParams)
			&& !IsGroundActor(Over.GetActor()))
		{
			return Look;
		}
		TArray<FOverlapResult> Near;
		FCollisionObjectQueryParams Solid;
		Solid.AddObjectTypesToQuery(ECC_WorldStatic);
		Solid.AddObjectTypesToQuery(ECC_WorldDynamic);
		World.OverlapMultiByObjectType(Near, Look.Ground + FVector(0.0, 0.0, OpenRadius * 0.6), FQuat::Identity, Solid,
			FCollisionShape::MakeSphere(OpenRadius), Params);
		for (const FOverlapResult& Each : Near)
		{
			const UPrimitiveComponent* Component = Each.GetComponent();
			if (Component && Component->GetCollisionEnabled() != ECollisionEnabled::NoCollision && !IsGroundActor(Each.GetActor()))
			{
				return Look;
			}
		}
		Look.bUsable = !UEncounterSubsystem::IsSheltered(&World, Look.Ground);
		return Look;
	}
}

bool CastShotScene::GroundAt(UWorld& World, const FVector& Spot, FVector& OutGround)
{
	FHitResult Hit;
	if (World.LineTraceSingleByObjectType(Hit, Spot + FVector(0.0, 0.0, 2000.0), Spot - FVector(0.0, 0.0, 4000.0),
		FCollisionObjectQueryParams(ECC_WorldStatic), GroundParams(World)))
	{
		OutGround = Hit.ImpactPoint;
		return true;
	}
	return false;
}

FVector CastShotScene::ClearWay(UWorld& World, const APawn& Walker, const FVector& Spot, const FVector& Ahead, float Distance)
{
	// The walker and what it carries are no obstacle.
	FCollisionQueryParams Params = GroundParams(World);
	TArray<AActor*> Carried;
	Walker.GetAttachedActors(Carried, /*bResetArray*/ true, /*bRecursivelyIncludeAttachedActors*/ true);
	Params.AddIgnoredActors(Carried);
	Params.AddIgnoredActor(&Walker);
	FCollisionObjectQueryParams Solid;
	Solid.AddObjectTypesToQuery(ECC_WorldStatic);
	Solid.AddObjectTypesToQuery(ECC_WorldDynamic);
	const FVector Extent = Walker.GetRootComponent() ? Walker.GetRootComponent()->Bounds.BoxExtent : FVector(35.0, 35.0, 90.0);
	const float HalfHeight = static_cast<float>(Extent.Z);
	const FCollisionShape Body = FCollisionShape::MakeCapsule(static_cast<float>(FMath::Min(Extent.X, Extent.Y)), HalfHeight);
	// Ahead first, then turning away from it either way; along each, the ground a meter at a time, walkable, and the body's
	// capsule swept from each meter to the next just over it.
	for (const float Turn : { 0.f, 30.f, -30.f, 60.f, -60.f, 90.f, -90.f, 120.f, -120.f, 150.f, -150.f, 180.f })
	{
		const FVector Way = Ahead.RotateAngleAxis(Turn, FVector::UpVector).GetSafeNormal2D();
		FVector From = Spot;
		bool bClear = true;
		for (float Along = SweepStep; Along <= Distance && bClear; Along += SweepStep)
		{
			FVector To;
			bClear = GroundAt(World, Spot + Way * Along, To) && FMath::Abs(To.Z - From.Z) < 0.7f * SweepStep;
			FHitResult Hit;
			const FVector Lift(0.0, 0.0, HalfHeight + StepLift);
			bClear = bClear && !World.SweepSingleByObjectType(Hit, From + Lift, To + Lift, FQuat::Identity, Solid, Body, Params);
			From = To;
		}
		if (bClear)
		{
			return Way;
		}
	}
	return Ahead;
}

void CastShotScene::StopMoving(APawn& Pawn)
{
	if (const ACharacter* Character = Cast<ACharacter>(&Pawn))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
		}
	}
}

void CastShotScene::StandPlayer(UWorld& World, APawn& Player, APlayerController* Controller, const FVector& Spot, float Yaw, float WalkDistance,
	FVector& InOutAhead)
{
	if (WalkDistance > 0.f)
	{
		InOutAhead = ClearWay(World, Player, Spot, InOutAhead, WalkDistance);
		Yaw = static_cast<float>(InOutAhead.Rotation().Yaw);
	}
	Player.SetActorLocationAndRotation(Spot + FVector(0.0, 0.0, Player.GetRootComponent()->Bounds.BoxExtent.Z + 5.0), FRotator(0.f, Yaw, 0.f),
		false, nullptr, ETeleportType::TeleportPhysics);
	StopMoving(Player);
	if (Controller)
	{
		Controller->SetControlRotation(FRotator(0.f, Yaw, 0.f));
	}
}

CastShotScene::FSpots CastShotScene::FindSpots(UWorld& World, const FVector& Around, const FVector* GivenFlat, const FVector* GivenSlope)
{
	FSpots Spots;
	if (GivenFlat)
	{
		Spots.bFlat = GroundAt(World, *GivenFlat, Spots.Flat);
	}
	if (GivenSlope)
	{
		const FSpotLook Look = LookAt(World, *GivenSlope);
		Spots.bSlope = GroundAt(World, *GivenSlope, Spots.Slope);
		Spots.SlopeDegrees = Look.Degrees;
		Spots.Uphill = Look.Uphill;
	}
	float BestSlopeMiss = TNumericLimits<float>::Max();
	auto StillLooking = [&Spots, &BestSlopeMiss, GivenSlope]()
	{
		return !Spots.bFlat || (!GivenSlope && (!Spots.bSlope || BestSlopeMiss > 3.f));
	};
	// Ring by ring outward: the nearest flat spot, and the slope nearest WantedSlope in the nearest rings that have one.
	for (float Radius = 0.f; Radius <= SearchRadius && StillLooking(); Radius += RingStep)
	{
		const int32 Samples = FMath::Max(1, FMath::RoundToInt(2.f * UE_PI * Radius / RingStep));
		for (int32 Sample = 0; Sample < Samples; ++Sample)
		{
			const float Angle = 2.f * UE_PI * Sample / Samples;
			const FSpotLook Look = LookAt(World, Around + FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), 0.f));
			if (!Look.bUsable)
			{
				continue;
			}
			if (!Spots.bFlat && Look.Degrees < 5.f && Look.Uneven < FlatUneven)
			{
				Spots.bFlat = true;
				Spots.Flat = Look.Ground;
			}
			const float Miss = FMath::Abs(Look.Degrees - WantedSlope);
			if (!GivenSlope && Look.Degrees >= SlopeRange[0] && Look.Degrees <= SlopeRange[1] && Look.Uneven < SlopeUneven && Miss < BestSlopeMiss)
			{
				BestSlopeMiss = Miss;
				Spots.bSlope = true;
				Spots.Slope = Look.Ground;
				Spots.SlopeDegrees = Look.Degrees;
				Spots.Uphill = Look.Uphill;
			}
		}
		// Past 40 m with both found, a slope nearer the wanted one isn't worth the walk.
		if (Spots.bSlope && Spots.bFlat && Radius > 4000.f)
		{
			break;
		}
	}
	return Spots;
}

#endif

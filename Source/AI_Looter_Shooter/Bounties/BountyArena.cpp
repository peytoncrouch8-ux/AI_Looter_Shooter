// ABountyArena: a bounty fight's place. Data only (the fight copies it); its lines for the console.

#include "Bounties/BountyArena.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/LineBatchComponent.h"
#include "Components/SceneComponent.h"

namespace
{
	/** Lines in a batch never expire on their own: the console command clears the batch. */
	constexpr float DrawForever = -1.f;

	void DrawCross(ULineBatchComponent& Lines, const FVector& At, double Size, const FLinearColor& Tint, uint32 BatchID)
	{
		Lines.DrawLine(At - FVector(Size, 0.0, 0.0), At + FVector(Size, 0.0, 0.0), Tint, SDPG_World, 4.f, DrawForever, BatchID);
		Lines.DrawLine(At - FVector(0.0, Size, 0.0), At + FVector(0.0, Size, 0.0), Tint, SDPG_World, 4.f, DrawForever, BatchID);
	}
}

ABountyArena::ABountyArena()
{
	PrimaryActorTick.bCanEverTick = false;
	Anchor = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Anchor->SetMobility(EComponentMobility::Static);
	RootComponent = Anchor;
#if WITH_EDITORONLY_DATA
	// The board reads every arena wherever the player is: in a partitioned world it never streams out.
	bIsSpatiallyLoaded = false;
#endif
	Tags.Add(TEXT("BountyArena"));
}

FVector ABountyArena::GetRewardLocation() const
{
	return GetActorTransform().TransformPositionNoScale(RewardSpot);
}

void ABountyArena::DrawArena(ULineBatchComponent& Lines, uint32 BatchID) const
{
	const FVector Here = GetActorLocation();
	const FLinearColor GroundTint = LooterUI::Color::Accent();
	if (GroundCorners.Num() >= 3)
	{
		for (int32 Index = 0; Index < GroundCorners.Num(); ++Index)
		{
			const FVector From = GroundCorners[Index] + FVector(0.0, 0.0, 50.0);
			const FVector To = GroundCorners[(Index + 1) % GroundCorners.Num()] + FVector(0.0, 0.0, 50.0);
			Lines.DrawLine(From, To, GroundTint, SDPG_World, 6.f, DrawForever, BatchID);
		}
	}
	else
	{
		constexpr int32 Sides = 48;
		for (int32 Side = 0; Side < Sides; ++Side)
		{
			const double From = 2.0 * UE_DOUBLE_PI * Side / Sides;
			const double To = 2.0 * UE_DOUBLE_PI * (Side + 1) / Sides;
			Lines.DrawLine(Here + FVector(FMath::Cos(From), FMath::Sin(From), 0.5) * GiveUpRadius,
				Here + FVector(FMath::Cos(To), FMath::Sin(To), 0.5) * GiveUpRadius, GroundTint, SDPG_World, 6.f, DrawForever, BatchID);
		}
	}
	const FTransform Frame = GetActorTransform();
	for (const FVector& Local : PushSpots)
	{
		DrawCross(Lines, Frame.TransformPositionNoScale(Local) + FVector(0.0, 0.0, 50.0), 60.0, LooterUI::Color::Accent(), BatchID);
	}
	for (const FVector& Local : FlankSpots)
	{
		DrawCross(Lines, Frame.TransformPositionNoScale(Local) + FVector(0.0, 0.0, 50.0), 60.0, LooterUI::Color::SegmentOn(), BatchID);
	}
	for (const FVector& Local : MarkSpots)
	{
		DrawCross(Lines, Frame.TransformPositionNoScale(Local) + FVector(0.0, 0.0, 50.0), 90.0, LooterUI::Color::Title(), BatchID);
	}
	const FVector Reward = GetRewardLocation();
	Lines.DrawLine(Reward, Reward + FVector(0.0, 0.0, 250.0), LooterUI::Color::Better(), SDPG_World, 8.f, DrawForever, BatchID);
	Lines.DrawLine(Here, Here + FVector(0.0, 0.0, 400.0), GroundTint, SDPG_World, 8.f, DrawForever, BatchID);
}

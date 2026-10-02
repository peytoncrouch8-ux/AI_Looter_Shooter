#include "World/PlayableArea.h"
#include "AI_Looter_Shooter.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/BoxComponent.h"
#include "Components/LineBatchComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "EngineUtils.h"

const FName APlayableArea::WallProfile(TEXT("PlayableBounds"));

namespace
{
	/**
	 * Gives a wall the PlayableBounds profile, or the same responses by hand when the profile is missing (a
	 * DefaultEngine.ini without it): walking pawns stop, everything else passes.
	 */
	void SetWallCollision(UBoxComponent& Wall)
	{
		FCollisionResponseTemplate Profile;
		if (UCollisionProfile::Get()->GetProfileTemplate(APlayableArea::WallProfile, Profile))
		{
			Wall.SetCollisionProfileName(APlayableArea::WallProfile, false);
			return;
		}
		static bool bWarned = false;
		if (!bWarned)
		{
			bWarned = true;
			UE_LOG(LogLooter, Warning, TEXT("Playable area: DefaultEngine.ini has no %s collision profile; the walls block pawns by hand."),
				*APlayableArea::WallProfile.ToString());
		}
		Wall.SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Wall.SetCollisionObjectType(ECC_WorldStatic);
		Wall.SetCollisionResponseToAllChannels(ECR_Ignore);
		Wall.SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	}
}

APlayableArea::APlayableArea()
{
	PrimaryActorTick.bCanEverTick = false;

	// A plain anchor: the corners are world positions, and the walls hang off it only so they belong to the actor.
	USceneComponent* Anchor = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Anchor->SetMobility(EComponentMobility::Static);
	RootComponent = Anchor;

#if WITH_EDITORONLY_DATA
	// The whole boundary matters wherever the player is: in a partitioned world it never streams out.
	bIsSpatiallyLoaded = false;
#endif
}

APlayableArea* APlayableArea::Find(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<APlayableArea> It(World); It; ++It)
	{
		if (It->GetBoundary().IsValid())
		{
			return *It;
		}
	}
	return nullptr;
}

bool APlayableArea::Contains(const FVector& Point) const
{
	return Shape.IsValid() && Shape.Contains(FVector2D(Point.X, Point.Y));
}

void APlayableArea::PostLoad()
{
	Super::PostLoad();
	Shape = MakeShape();
}

void APlayableArea::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// In the editor the boundary follows its properties as they change (a script setting them runs this again).
	Shape = MakeShape();
}

void APlayableArea::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	// The game builds the walls here, before anyone moves; the editor never has them, so nothing there (PCG, the build
	// scripts' traces, placing actors) ever meets them.
	const UWorld* World = GetWorld();
	if (World && World->IsGameWorld())
	{
		Rebuild();
	}
	else
	{
		Shape = MakeShape();
	}
}

void APlayableArea::Rebuild()
{
	Shape = MakeShape();
	for (UBoxComponent* Wall : Walls)
	{
		if (Wall)
		{
			Wall->DestroyComponent();
		}
	}
	Walls.Reset();

	for (const FWallBox& Box : MakeWallBoxes(Shape))
	{
		// Made as the game starts and never saved: the level only ever holds the corners and flags.
		UBoxComponent* Wall = NewObject<UBoxComponent>(this,
			MakeUniqueObjectName(this, UBoxComponent::StaticClass(), *FString::Printf(TEXT("Wall_%d"), Box.Edge)), RF_Transient);
		Wall->SetMobility(EComponentMobility::Static);
		// In world space like the corners, whatever the actor's own transform.
		Wall->SetUsingAbsoluteLocation(true);
		Wall->SetUsingAbsoluteRotation(true);
		Wall->SetUsingAbsoluteScale(true);
		Wall->SetRelativeLocation_Direct(Box.Center);
		Wall->SetRelativeRotation_Direct(Box.Rotation);
		Wall->InitBoxExtent(Box.Extent);
		Wall->SetupAttachment(GetRootComponent());
		SetWallCollision(*Wall);
		Wall->SetHiddenInGame(true);
		Wall->SetCanEverAffectNavigation(false);
		Wall->SetGenerateOverlapEvents(false);
		// Nobody climbs them: a character that bumps into one never tries to step up onto it.
		Wall->CanCharacterStepUpOn = ECB_No;
		Wall->RegisterComponent();
		Walls.Add(Wall);
	}

	int32 OpenCount = 0;
	for (int32 Edge = 0; Edge < Shape.NumEdges(); ++Edge)
	{
		OpenCount += Shape.IsOpen(Edge) ? 1 : 0;
	}
	UE_LOG(LogLooter, Log, TEXT("Playable area %s: %d corners, %d open edges, %d walls, %.0f square meters%s"), *GetName(),
		Shape.NumEdges(), OpenCount, Walls.Num(), Shape.SurfaceArea() / 10000.0, Shape.IsValid() ? TEXT("") : TEXT(" (not a usable boundary)"));
}

void APlayableArea::DrawBounds(ULineBatchComponent& Lines, uint32 BatchID) const
{
	const FPlayableBoundary Outline = MakeShape();
	if (Outline.NumEdges() < 2)
	{
		return;
	}
	// Lines in a batch never expire on their own: Looter.World.Bounds clears the batch when it is turned off.
	constexpr float Forever = -1.f;
	const FLinearColor ClosedColor = LooterUI::Color::Accent();
	const FLinearColor OpenColor = LooterUI::Color::SegmentOn();
	for (int32 Edge = 0; Edge < Corners.Num(); ++Edge)
	{
		const FVector& From = Corners[Edge];
		const FVector& To = Corners[(Edge + 1) % Corners.Num()];
		const FLinearColor& Tint = Outline.IsOpen(Edge) ? OpenColor : ClosedColor;
		// A rail at knee height and one over the head, so the edge reads over uneven ground, and a post at each corner.
		for (const double Rise : { 50.0, 250.0 })
		{
			Lines.DrawLine(From + FVector(0.0, 0.0, Rise), To + FVector(0.0, 0.0, Rise), Tint, SDPG_World, 8.f, Forever, BatchID);
		}
		Lines.DrawLine(From, From + FVector(0.0, 0.0, 400.0), Tint, SDPG_World, 8.f, Forever, BatchID);
	}
	for (const FWallBox& Box : MakeWallBoxes(Outline))
	{
		Lines.DrawBox(Box.Center, Box.Extent, Box.Rotation.Quaternion(), ClosedColor, Forever, SDPG_World, 3.f, BatchID);
	}
}

FPlayableBoundary APlayableArea::MakeShape() const
{
	TArray<FVector2D> Flat;
	Flat.Reserve(Corners.Num());
	for (const FVector& Corner : Corners)
	{
		Flat.Emplace(Corner.X, Corner.Y);
	}
	return FPlayableBoundary(MoveTemp(Flat), OpenEdges);
}

TArray<APlayableArea::FWallBox> APlayableArea::MakeWallBoxes(const FPlayableBoundary& Outline) const
{
	TArray<FWallBox> Boxes;
	if (Outline.NumEdges() != Corners.Num())
	{
		return Boxes;
	}
	for (const FPlayableWall& Wall : Outline.MakeWalls(WallSetback, WallThickness))
	{
		// From below the lower corner's ground to well over the higher one's.
		const FVector& From = Corners[Wall.Edge];
		const FVector& To = Corners[(Wall.Edge + 1) % Corners.Num()];
		const double Bottom = FMath::Min(From.Z, To.Z) - WallDepth;
		const double Top = FMath::Max(From.Z, To.Z) + WallHeight;
		FWallBox& Box = Boxes.AddDefaulted_GetRef();
		Box.Edge = Wall.Edge;
		Box.Center = FVector(Wall.Center.X, Wall.Center.Y, (Bottom + Top) * 0.5);
		Box.Extent = FVector(Wall.HalfLength, Wall.HalfThickness, (Top - Bottom) * 0.5);
		Box.Rotation = FRotator(0.0, FMath::RadiansToDegrees(FMath::Atan2(Wall.Direction.Y, Wall.Direction.X)), 0.0);
	}
	return Boxes;
}

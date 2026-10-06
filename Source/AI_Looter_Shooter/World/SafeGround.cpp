#include "World/SafeGround.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/EncounterSubsystem.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/LineBatchComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

ASafeGround::ASafeGround()
{
	PrimaryActorTick.bCanEverTick = false;

	// A plain anchor: the corners are world positions, and with none the circle is round it. Nothing to collide with.
	USceneComponent* Anchor = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Anchor->SetMobility(EComponentMobility::Static);
	RootComponent = Anchor;
	SetActorEnableCollision(false);

#if WITH_EDITORONLY_DATA
	// Where nothing hunts matters wherever the player is: in a partitioned world it never streams out.
	bIsSpatiallyLoaded = false;
#endif
}

void ASafeGround::PostLoad()
{
	Super::PostLoad();
	Rebuild();
}

void ASafeGround::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// In the editor its outline follows its corners as they change (a script setting them runs this again).
	Rebuild();
}

void ASafeGround::BeginPlay()
{
	Super::BeginPlay();
	Rebuild();
	if (UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this))
	{
		Encounters->RegisterSafeZone(this);
	}
	RefreshStory();
}

void ASafeGround::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this))
	{
		Encounters->UnregisterSafeZone(this);
	}
	Super::EndPlay(EndPlayReason);
}

void ASafeGround::Rebuild()
{
	TArray<FVector2D> Flat;
	Flat.Reserve(Corners.Num());
	for (const FVector& Corner : Corners)
	{
		Flat.Emplace(Corner.X, Corner.Y);
	}
	Outline = FPlayableBoundary(MoveTemp(Flat), TArray<bool>());
}

bool ASafeGround::Contains(const FVector& Point) const
{
	if (Outline.IsValid())
	{
		return Outline.Contains(FVector2D(Point.X, Point.Y));
	}
	return Radius > 0.f && FVector::DistSquared2D(Point, GetActorLocation()) <= FMath::Square(static_cast<double>(Radius));
}

FName ASafeGround::GetZoneId() const
{
	return ZoneId.IsNone() ? GetFName() : ZoneId;
}

double ASafeGround::GetSurfaceArea() const
{
	return Outline.IsValid() ? Outline.SurfaceArea() : UE_DOUBLE_PI * FMath::Square(static_cast<double>(Radius));
}

void ASafeGround::RefreshStory()
{
	// The session's record (a test's in tests); an empty condition holds with or without one.
	const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this);
	const bool bWanted = ActiveWhen.IsEmpty() || (Encounters && Encounters->IsStoryMet(ActiveWhen));
	const bool bFirst = !bStoryApplied;
	if (!bFirst && bWanted == bActive)
	{
		return;
	}
	bStoryApplied = true;
	bActive = bWanted;
	if (bFirst)
	{
		UE_LOG(LogLooter, Verbose, TEXT("Safe zone %s: %s as play begins (%s)."), *GetZoneId().ToString(), bActive ? TEXT("on") : TEXT("off"),
			*ActiveWhen.Describe());
	}
	else
	{
		UE_LOG(LogLooter, Log, TEXT("Safe zone %s: %s by the story (%s)."), *GetZoneId().ToString(), bActive ? TEXT("on") : TEXT("off"),
			*ActiveWhen.Describe());
	}
}

void ASafeGround::DrawZone(ULineBatchComponent& Lines, uint32 BatchID) const
{
	// Lines in a batch never expire on their own: Looter.Encounter.Zones clears the batch when it's turned off.
	constexpr float Forever = -1.f;
	const FLinearColor Tint = bActive ? LooterUI::Color::Better() : LooterUI::Color::TextDim();
	if (Corners.Num() >= 3)
	{
		for (int32 Index = 0; Index < Corners.Num(); ++Index)
		{
			const FVector& Corner = Corners[Index];
			const FVector& NextCorner = Corners[(Index + 1) % Corners.Num()];
			// A rail at knee height and one over the head, so the line reads over uneven ground, and a post at each corner.
			for (const double Rise : { 50.0, 250.0 })
			{
				Lines.DrawLine(Corner + FVector(0.0, 0.0, Rise), NextCorner + FVector(0.0, 0.0, Rise), Tint, SDPG_World, 8.f, Forever, BatchID);
			}
			Lines.DrawLine(Corner, Corner + FVector(0.0, 0.0, 400.0), Tint, SDPG_World, 8.f, Forever, BatchID);
		}
		return;
	}
	if (Radius > 0.f)
	{
		constexpr int32 Sides = 48;
		const FVector Middle = GetActorLocation() + FVector(0.0, 0.0, 50.0);
		for (int32 Side = 0; Side < Sides; ++Side)
		{
			const double AngleFrom = 2.0 * UE_DOUBLE_PI * Side / Sides;
			const double AngleTo = 2.0 * UE_DOUBLE_PI * (Side + 1) / Sides;
			Lines.DrawLine(Middle + FVector(FMath::Cos(AngleFrom), FMath::Sin(AngleFrom), 0.0) * Radius,
				Middle + FVector(FMath::Cos(AngleTo), FMath::Sin(AngleTo), 0.0) * Radius, Tint, SDPG_World, 8.f, Forever, BatchID);
		}
	}
}

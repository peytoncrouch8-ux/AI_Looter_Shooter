// AChickenYard: the yard, its birds made, its ground traced, the alarm and the calm after it, the voices.

#include "World/ChickenYard.h"
#include "World/AnimalCues.h"
#include "World/FaunaRules.h"
#include "World/WorldQueries.h"
#include "Audio/LooterSound.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The ground's grid step (cm), how high over the yard a trace starts and how far under it ends. */
	constexpr float GroundStep = 25.f;
	constexpr float GroundAbove = 150.f;
	constexpr float GroundBelow = 300.f;
	/** A point traced this far off the yard's middle height (cm) met something standing there (the coop's roof): it's ignored. */
	constexpr float GroundOddOut = 60.f;

	/** A pen's spots to walk and run to (cm apart). */
	constexpr float SpotStep = 15.f;

	/** The threat's share of the flee radius within which a scare stays fresh (no calming while it's that near). */
	constexpr float CalmScale = 1.3f;

	/** A cluck or a squawk comes from about the head: this high over the feet (cm, a hen's). */
	constexpr float HeadHeight = 34.f;

	template <typename T>
	T* FindIfMade(const TCHAR* ObjectPath)
	{
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath))))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<T> Finder(ObjectPath);
		return Finder.Object;
	}
}

AChickenYard::AChickenYard()
{
	CullDistance = 4500.f;
	CluckCue = FName(AnimalCue::ChickenCluck);
	SquawkCue = FName(AnimalCue::ChickenSquawk);
	CrowCue = FName(AnimalCue::ChickenCrow);
	// The models where Chicken.py's import puts them (the build script may set others).
	static USkeletalMesh* const Brown = FindIfMade<USkeletalMesh>(TEXT("/Game/Art/Creatures/SK_HenBrown.SK_HenBrown"));
	static USkeletalMesh* const White = FindIfMade<USkeletalMesh>(TEXT("/Game/Art/Creatures/SK_HenWhite.SK_HenWhite"));
	static USkeletalMesh* const Rooster = FindIfMade<USkeletalMesh>(TEXT("/Game/Art/Creatures/SK_Rooster.SK_Rooster"));
	HenBrownMesh = Brown;
	HenWhiteMesh = White;
	RoosterMesh = Rooster;
}

void AChickenYard::BeginPlay()
{
	Super::BeginPlay();
	SetupBirds();
}

void AChickenYard::SetupBirds()
{
	if (bBirdsReady)
	{
		return;
	}
	bBirdsReady = true;
	YardRandom.Initialize(Seed);

	// The pens' spots and corners, the yard's middle and reach.
	FBox2D Box(ForceInit);
	PenSpots.SetNum(Pens.Num());
	PenCorners.SetNum(Pens.Num());
	for (int32 Index = 0; Index < Pens.Num(); ++Index)
	{
		PenSpots[Index] = ChickenPens::Spots(Pens[Index], SpotStep, BodyRoom);
		PenCorners[Index] = ChickenPens::Corners(Pens[Index], BodyRoom);
		const FBox2D PenBox = ChickenPens::Bounds(Pens[Index]);
		if (PenBox.bIsValid)
		{
			Box += PenBox;
		}
	}
	const FVector Here = GetActorLocation();
	const FVector2D Middle = Box.bIsValid ? Box.GetCenter() : FVector2D(Here);
	Center = FVector(Middle.X, Middle.Y, Here.Z + 20.0);
	Reach = Box.bIsValid ? static_cast<float>(Box.GetExtent().Size()) : 100.f;
	TraceGround();

	// The birds: each pen's hens, brown and white turn about, and the rooster where he lives.
	for (int32 Index = 0; Index < Pens.Num(); ++Index)
	{
		if (PenSpots[Index].IsEmpty())
		{
			continue;
		}
		for (int32 Hen = 0; Hen < Pens[Index].Hens; ++Hen)
		{
			AddBird(Index, false);
		}
		if (Pens[Index].bRooster)
		{
			AddBird(Index, true);
		}
	}
	const bool bWhiteFirst = YardRandom.FRand() < 0.5f;
	int32 HenNumber = 0;
	for (FChicken& Bird : Birds)
	{
		if (!Bird.bRooster)
		{
			// Alternating, so a yard of two or more has both colourings.
			Bird.bWhite = ((HenNumber++ % 2) == 0) == bWhiteFirst;
		}
	}
	MakeFigures();
	CluckTimer = YardRandom.FRandRange(CluckSecondsMin * 0.3f, FMath::Max(CluckSecondsMin, CluckSecondsMax));
	CrowTimer = YardRandom.FRandRange(CrowSecondsMin * 0.25f, FMath::Max(CrowSecondsMin, CrowSecondsMax));
}

void AChickenYard::AddBird(int32 PenIndex, bool bRooster)
{
	const TArray<FVector2D>& Spots = PenSpots[PenIndex];
	FChicken& Bird = Birds.AddDefaulted_GetRef();
	const int32 Number = Birds.Num() - 1;
	Bird.Random.Initialize(Seed * 977 + Number * 7919 + 13);
	Bird.bRooster = bRooster;
	Bird.Pen = PenIndex;
	// A spot of its own: the farthest of a few draws from the birds already there.
	FVector2D Best = Spots[Bird.Random.RandRange(0, Spots.Num() - 1)];
	double BestRoom = -1.0;
	for (int32 Draw = 0; Draw < 8; ++Draw)
	{
		const FVector2D Spot = Spots[Bird.Random.RandRange(0, Spots.Num() - 1)];
		double Room = TNumericLimits<double>::Max();
		for (int32 Other = 0; Other < Number; ++Other)
		{
			if (Birds[Other].Pen == PenIndex)
			{
				Room = FMath::Min(Room, FVector2D::Distance(Spot, Birds[Other].Position));
			}
		}
		if (Room > BestRoom)
		{
			BestRoom = Room;
			Best = Spot;
		}
	}
	Bird.Position = Best;
	Bird.Height = GroundAt(Best);
	Bird.Yaw = Bird.Random.FRandRange(-180.f, 180.f);
	Bird.Scale = Bird.Random.FRandRange(0.94f, 1.06f);
	Bird.Target = Best;
	StartStand(Bird, 0.2f, 2.5f);
	Bird.Clock = Bird.Random.FRandRange(0.f, Bird.Length);
}

void AChickenYard::TraceGround()
{
	GroundHeights.Reset();
	GroundColumns = GroundRows = 0;
	const FBox2D Box(FVector2D(Center) - FVector2D(Reach + GroundStep), FVector2D(Center) + FVector2D(Reach + GroundStep));
	GroundBox = Box;
	UWorld* World = GetWorld();
	const double Middle = GetActorLocation().Z;
	if (!World || Pens.IsEmpty())
	{
		return;
	}
	GroundColumns = FMath::Max(2, FMath::CeilToInt(Box.GetSize().X / GroundStep) + 1);
	GroundRows = FMath::Max(2, FMath::CeilToInt(Box.GetSize().Y / GroundStep) + 1);
	GroundHeights.Init(static_cast<float>(Middle), GroundColumns * GroundRows);
	// The world's solid shapes only (the terrain, a dirt patch, the coop's floor), never the volumes.
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("ChickenYardGround"), this);
	for (int32 Row = 0; Row < GroundRows; ++Row)
	{
		for (int32 Column = 0; Column < GroundColumns; ++Column)
		{
			const FVector2D Point = Box.Min + FVector2D(Column * GroundStep, Row * GroundStep);
			FHitResult Hit;
			const FVector Start(Point.X, Point.Y, Middle + GroundAbove);
			const FVector End(Point.X, Point.Y, Middle - GroundBelow);
			if (World->LineTraceSingleByObjectType(Hit, Start, End, FCollisionObjectQueryParams(ECC_WorldStatic), Params)
				&& Hit.ImpactNormal.Z > 0.6 && FMath::Abs(Hit.ImpactPoint.Z - Middle) < GroundOddOut)
			{
				GroundHeights[Row * GroundColumns + Column] = static_cast<float>(Hit.ImpactPoint.Z);
			}
		}
	}
}

float AChickenYard::GroundAt(const FVector2D& Point) const
{
	if (GroundHeights.Num() != GroundColumns * GroundRows || GroundColumns < 2 || GroundRows < 2)
	{
		return static_cast<float>(GetActorLocation().Z);
	}
	const double X = FMath::Clamp((Point.X - GroundBox.Min.X) / GroundStep, 0.0, GroundColumns - 1.001);
	const double Y = FMath::Clamp((Point.Y - GroundBox.Min.Y) / GroundStep, 0.0, GroundRows - 1.001);
	const int32 Column = FMath::FloorToInt(X);
	const int32 Row = FMath::FloorToInt(Y);
	const float U = static_cast<float>(X - Column);
	const float V = static_cast<float>(Y - Row);
	auto At = [this](int32 C, int32 R) { return GroundHeights[R * GroundColumns + C]; };
	const float Low = FMath::Lerp(At(Column, Row), At(Column + 1, Row), U);
	const float High = FMath::Lerp(At(Column, Row + 1), At(Column + 1, Row + 1), U);
	return FMath::Lerp(Low, High, V);
}

FVector AChickenYard::GetFeet(int32 Index) const
{
	return Birds.IsValidIndex(Index) ? FVector(Birds[Index].Position, Birds[Index].Height) : FVector::ZeroVector;
}

FVector AChickenYard::HeadOf(const FChicken& Bird) const
{
	const FVector Forward = FRotator(0.f, Bird.Yaw, 0.f).Vector();
	const float Size = Bird.Scale * (Bird.bRooster ? 1.25f : 1.f);
	return FVector(Bird.Position, Bird.Height) + Forward * (12.0 * Size) + FVector(0.0, 0.0, HeadHeight * Size);
}

FVector AChickenYard::GetFaunaCenter() const
{
	return Center;
}

float AChickenYard::GetFaunaRadius() const
{
	return Reach + 40.f;
}

bool AChickenYard::IsBusy() const
{
	for (const FChicken& Bird : Birds)
	{
		if (Bird.Doing == EChickenDoing::Flee || Bird.Doing == EChickenDoing::Walk)
		{
			return true;
		}
	}
	return false;
}

void AChickenYard::UpdateFauna(const FFaunaTick& Tick, const FFaunaContext& Context)
{
	if (!bBirdsReady)
	{
		SetupBirds();
	}
	const float DeltaSeconds = Tick.DeltaSeconds;
	CheckAlarm(Tick, Context);
	if (bScared)
	{
		UpdateCalm(DeltaSeconds, Context);
	}
	for (int32 Index = 0; Index < Birds.Num(); ++Index)
	{
		UpdateBird(Index, DeltaSeconds);
	}
	UpdateVoices(DeltaSeconds, Context);
	// Posing is the cost: only while someone can see them (coming into view updates at once).
	if (Tick.bOnScreen && IsFaunaShown())
	{
		PoseBirds(DeltaSeconds);
	}
}

void AChickenYard::CheckAlarm(const FFaunaTick& Tick, const FFaunaContext& Context)
{
	// A gunshot or a strike they hear (strikes judged across the yard's spread).
	const int32 Heard = FaunaRules::FirstHeard(Center, Context.Noises, Tick.Since, GunfireRadius, ImpactRadius + Reach);
	if (Heard != INDEX_NONE)
	{
		Scare(Context.Noises[Heard].Location);
	}
	// Who watches whom: the nearest threat within the alert radius. A threat within the flee radius scatters them all;
	// once they're scattered, only a bird it's still too near runs again (the rest stand watching from the far side).
	for (int32 Index = 0; Index < Birds.Num(); ++Index)
	{
		FChicken& Bird = Birds[Index];
		const FVector Feet(Bird.Position, Bird.Height + 20.f);
		Bird.bWatching = false;
		float Nearest = TNumericLimits<float>::Max();
		for (const FFaunaThreat& Threat : Context.Threats)
		{
			if (FaunaRules::IsThreatened(Feet, Threat, FleeRadius) && Bird.Doing != EChickenDoing::Flee)
			{
				if (!bScared)
				{
					Scare(Threat.Location);
				}
				else
				{
					CalmTime = 0.f;
					StartFlee(Index, FVector2D(Threat.Location));
				}
			}
			const float Distance = static_cast<float>(FVector::Dist(Feet, Threat.Location));
			if (FaunaRules::IsThreatened(Feet, Threat, AlertRadius) && Distance < Nearest)
			{
				Nearest = Distance;
				Bird.bWatching = true;
				Bird.WatchAt = Threat.Location;
			}
		}
	}
}

void AChickenYard::Scare(const FVector& Source)
{
	CalmTime = 0.f;
	ScaredBy = Source;
	if (!bScared)
	{
		bScared = true;
		CalmNeeded = YardRandom.FRandRange(CalmSecondsMin, FMath::Max(CalmSecondsMin, CalmSecondsMax));
		// A squawk or two as they go, from whoever is nearest.
		if (Birds.Num() > 0)
		{
			const int32 First = YardRandom.RandRange(0, Birds.Num() - 1);
			Squawks.Add({ YardRandom.FRandRange(0.f, 0.12f), First });
			if (Birds.Num() > 2 && YardRandom.FRand() < 0.6f)
			{
				Squawks.Add({ YardRandom.FRandRange(0.25f, 0.6f), (First + 1 + YardRandom.RandRange(0, Birds.Num() - 2)) % Birds.Num() });
			}
		}
	}
	for (int32 Index = 0; Index < Birds.Num(); ++Index)
	{
		if (Birds[Index].Doing != EChickenDoing::Flee)
		{
			StartFlee(Index, FVector2D(Source));
		}
	}
}

void AChickenYard::UpdateCalm(float DeltaSeconds, const FFaunaContext& Context)
{
	bool bNear = false;
	for (const FChicken& Bird : Birds)
	{
		const FVector Feet(Bird.Position, Bird.Height + 20.f);
		for (const FFaunaThreat& Threat : Context.Threats)
		{
			bNear |= FaunaRules::IsThreatened(Feet, Threat, FleeRadius * CalmScale);
		}
	}
	CalmTime = bNear ? 0.f : CalmTime + DeltaSeconds;
	if (CalmTime >= CalmNeeded)
	{
		bScared = false;
	}
}

void AChickenYard::UpdateVoices(float DeltaSeconds, const FFaunaContext& Context)
{
	for (int32 Index = Squawks.Num() - 1; Index >= 0; --Index)
	{
		Squawks[Index].Key -= DeltaSeconds;
		if (Squawks[Index].Key <= 0.f)
		{
			if (Birds.IsValidIndex(Squawks[Index].Value))
			{
				PlayCue(SquawkCue, HeadOf(Birds[Squawks[Index].Value]));
			}
			Squawks.RemoveAt(Index);
		}
	}
	if (Birds.IsEmpty())
	{
		return;
	}
	auto PlayerWithin = [&](float Range)
	{
		for (const FFaunaThreat& Threat : Context.Threats)
		{
			if (Threat.bPlayer && FVector::Dist(Threat.Location, Center) < Range + Reach)
			{
				return true;
			}
		}
		return false;
	};
	// A cluck now and then from one pottering about.
	CluckTimer -= DeltaSeconds;
	if (CluckTimer <= 0.f)
	{
		CluckTimer = YardRandom.FRandRange(CluckSecondsMin, FMath::Max(CluckSecondsMin, CluckSecondsMax));
		const int32 Start = YardRandom.RandRange(0, Birds.Num() - 1);
		for (int32 Step = 0; Step < Birds.Num() && !bScared && PlayerWithin(CluckRange); ++Step)
		{
			FChicken& Bird = Birds[(Start + Step) % Birds.Num()];
			if (Bird.Doing != EChickenDoing::Flee && Bird.Doing != EChickenDoing::Crow && Bird.Doing != EChickenDoing::Sit)
			{
				PlayCue(CluckCue, HeadOf(Bird));
				break;
			}
		}
	}
	// The rooster's crow, when he's at ease.
	CrowTimer -= DeltaSeconds;
	if (CrowTimer <= 0.f)
	{
		CrowTimer = YardRandom.FRandRange(CrowSecondsMin, FMath::Max(CrowSecondsMin, CrowSecondsMax));
		for (int32 Index = 0; Index < Birds.Num(); ++Index)
		{
			if (Birds[Index].bRooster && !bScared && Birds[Index].Doing != EChickenDoing::Flee && PlayerWithin(CrowRange))
			{
				StartCrow(Index);
				PlayCue(CrowCue, HeadOf(Birds[Index]));
			}
		}
	}
}

void AChickenYard::PlayCue(FName Cue, const FVector& Where) const
{
	if (!Cue.IsNone() && IsFaunaShown())
	{
		LooterSound::PlayAt(this, Cue, Where);
	}
}

void AChickenYard::OnFaunaShownChanged(bool bShown)
{
	// Coming back into view the springs start from rest (no jolt from where the birds went meanwhile).
	for (FChicken& Bird : Birds)
	{
		Bird.SpringFrames = 0;
	}
}

UPoseableMeshComponent* AChickenYard::GetFigure(int32 Index) const
{
	return Figures.IsValidIndex(Index) ? Figures[Index].Get() : nullptr;
}

// AStoneSentinelShockwave (StoneSentinelShockwave.h): the ring of rock spikes a Stone Sentinel's fists send out along the
// ground: its rows laid on the land as its front reaches them, its dust, its spikes on screen. Whom it catches is
// StoneSentinelShockwaveHits.cpp's.

#include "Creatures/StoneSentinelShockwave.h"
#include "AI_Looter_Shooter.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/GravemotherAssets.h"
#include "Creatures/GravemotherFX.h"
#include "Creatures/StoneSentinelFX.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const TCHAR* const AStoneSentinelShockwave::SlabPath = TEXT("/Game/Art/Rocks/SM_Rock_C.SM_Rock_C");
const TCHAR* const AStoneSentinelShockwave::ChunkPath = TEXT("/Game/Art/Rocks/SM_Rock_D.SM_Rock_D");

namespace
{
	/** The share of spikes that are slabs (the rest chunks). */
	constexpr float SlabShare = 0.65f;
	/** A spike's lean out from the middle and to its side (degrees), its width against its height. */
	constexpr float LeanMin = 10.f;
	constexpr float LeanMax = 26.f;
	constexpr float SideLean = 8.f;
	constexpr float WidthMin = 0.32f;
	constexpr float WidthMax = 0.5f;
	/** Its height varies by this much either way, and a spike always keeps this share of itself in the ground. */
	constexpr float HeightJitterMin = 0.75f;
	constexpr float HeightJitterMax = 1.15f;
	constexpr float Buried = 0.12f;
	/** Rising, it swells from this share of its width. */
	constexpr float RiseWidthShare = 0.7f;
	/** The ground under a row is looked for this far over and under the last row's, past MaxStep (cm, at size 1). */
	constexpr float LookAbove = 50.f;
	constexpr float LookBelow = 200.f;
	/** A wall across the way is looked for at this height over the ground (cm, at size 1); a face steeper than this stops it. */
	constexpr float WallLookHeight = 40.f;
	constexpr float FloorNormalZ = 0.5f;
	/** Dust along the front: a wave piece every this many sectors, a puff every PuffEvery, a piece of debris every DebrisEvery (few, and
	 *  never at the lens: StoneSentinelFX; the wave's smoke grows to WaveSmokeSize and carries about WaveSmokeTravel, cm). */
	constexpr int32 WaveEvery = 6;
	constexpr float WaveSmokeSize = 130.f;
	constexpr float WaveSmokeTravel = 160.f;
	constexpr int32 PuffEvery = 8;
	constexpr int32 DebrisEvery = 8;
	/** The light on its dust (as the Unpaid's shriek wave's without a light source: near white). */
	const FLinearColor DustLight(0.9f, 0.88f, 0.85f);
	/** How long it may live at most (s): its last spike has sunk long before. */
	constexpr float MaxLife = 5.f;

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

	UInstancedStaticMeshComponent* MakeSet(AActor& Owner, const TCHAR* Name, USceneComponent* Parent)
	{
		UInstancedStaticMeshComponent* Set = Owner.CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Set->SetupAttachment(Parent);
		Set->SetMobility(EComponentMobility::Movable);
		Set->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Set->SetCastShadow(false);
		Set->SetCanEverAffectNavigation(false);
		Set->SetGenerateOverlapEvents(false);
		return Set;
	}

	/** How far Where is from the nearest of Lenses (cm). */
	float LensFrom(const TArray<FVector, TInlineAllocator<2>>& Lenses, const FVector& Where)
	{
		float Nearest = TNumericLimits<float>::Max();
		for (const FVector& Lens : Lenses)
		{
			Nearest = FMath::Min(Nearest, static_cast<float>(FVector::Dist(Lens, Where)));
		}
		return Nearest;
	}

	/** Eased out: fast up, settling at the top (a spike punching through). */
	float PunchOut(float Alpha)
	{
		const float T = FMath::Clamp(Alpha, 0.f, 1.f);
		return 1.f - FMath::Cube(1.f - T);
	}
}

AStoneSentinelShockwave::AStoneSentinelShockwave()
{
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
	SlabSet = MakeSet(*this, TEXT("Slabs"), Root);
	ChunkSet = MakeSet(*this, TEXT("Chunks"), Root);
	static UStaticMesh* const FoundSlab = FindIfMade<UStaticMesh>(SlabPath);
	static UStaticMesh* const FoundChunk = FindIfMade<UStaticMesh>(ChunkPath);
	SlabMesh = FoundSlab;
	ChunkMesh = FoundChunk;
}

AStoneSentinelShockwave* AStoneSentinelShockwave::Spawn(UWorld* World, const FSentinelShockLaunch& InLaunch, bool bInKeep)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	AStoneSentinelShockwave* Wave = World->SpawnActor<AStoneSentinelShockwave>(AStoneSentinelShockwave::StaticClass(),
		FTransform(InLaunch.Center), Params);
	if (!Wave)
	{
		return nullptr;
	}
	// Its rocks, or the engine's cube in granite where they aren't imported; each set's foot and size, so a spike stands on
	// its own foot and is sized by what it is. Made once: a kept wave is sent again with the same sets.
	UStaticMesh* Cube = GravemotherAssets::StandInMesh(GravemotherAssets::EStandIn::Cube);
	UInstancedStaticMeshComponent* Sets[2] = { Wave->SlabSet.Get(), Wave->ChunkSet.Get() };
	UStaticMesh* Meshes[2] = { Wave->SlabMesh.Get(), Wave->ChunkMesh.Get() };
	for (int32 Index = 0; Index < 2; ++Index)
	{
		UStaticMesh* Mesh = Meshes[Index] ? Meshes[Index] : Cube;
		if (!Sets[Index] || !Mesh)
		{
			continue;
		}
		Sets[Index]->SetStaticMesh(Mesh);
		if (!Meshes[Index])
		{
			Sets[Index]->SetMaterial(0, GravemotherAssets::StandInLook(Wave, GravemotherAssets::GraniteColor() * 0.9f));
		}
		const FBoxSphereBounds Bounds = Mesh->GetBounds();
		Wave->SetBottom[Index] = static_cast<float>(Bounds.Origin.Z - Bounds.BoxExtent.Z);
		Wave->SetAcross[Index] = FMath::Max(static_cast<float>(FMath::Max(Bounds.BoxExtent.X, Bounds.BoxExtent.Y)) * 2.f, 1.f);
		Wave->SetTall[Index] = FMath::Max(static_cast<float>(Bounds.BoxExtent.Z) * 2.f, 1.f);
	}
	Wave->bKeep = bInKeep;
	if (!bInKeep)
	{
		Wave->SetLifeSpan(MaxLife);
	}
	if (!Wave->HasActorBegunPlay())
	{
		Wave->DispatchBeginPlay();
	}
	Wave->Relaunch(InLaunch);
	return Wave;
}

void AStoneSentinelShockwave::Relaunch(const FSentinelShockLaunch& InLaunch)
{
	UWorld* World = GetWorld();
	Launch = InLaunch;
	Random.Initialize(InLaunch.Seed);
	WorldQuery = LooterWorld::StaticGeometryParams(World, TEXT("SentinelShock"), InLaunch.Causer.Get());
	if (const AActor* Over = InLaunch.PassesOver.Get())
	{
		WorldQuery.AddIgnoredActor(Over);
	}
	Sectors.Reset();
	Sectors.SetNum(FMath::Clamp(InLaunch.Tuning.Sectors, 8, 64));
	for (FSector& Sector : Sectors)
	{
		Sector.Ground = InLaunch.Center;
		Sector.bHasGround = true;
	}
	Spikes.Reset();
	Struck.Reset();
	Heard.Reset();
	Age = 0.f;
	NextRow = 0;
	PlayersHit = 0;
	bRunOut = false;
	bIdle = false;
	Radius = StoneSentinelMoves::FrontRadius(0.f, InLaunch.Tuning, InLaunch.Size);
	SetActorLocation(InLaunch.Center);
	SetActorTickEnabled(true);
}

void AStoneSentinelShockwave::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

int32 AStoneSentinelShockwave::NumStoppedSectors() const
{
	int32 Count = 0;
	for (const FSector& Sector : Sectors)
	{
		Count += Sector.bStopped ? 1 : 0;
	}
	return Count;
}

void AStoneSentinelShockwave::Advance(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.f)
	{
		return;
	}
	const FSentinelShockTuning& Shock = Launch.Tuning;
	const float Scale = FMath::Max(Launch.Size, 0.05f);
	const float Farthest = Shock.MaxRadius * Scale;
	Age += DeltaSeconds;
	if (!bRunOut)
	{
		const float From = Radius;
		Radius = StoneSentinelMoves::FrontRadius(Age, Shock, Scale);
		// Each row comes up as the front reaches it.
		for (;;)
		{
			const float RowRadius = (Shock.StartRadius + static_cast<float>(NextRow) * Shock.RowSpacing) * Scale;
			if (RowRadius > Radius || RowRadius > Farthest)
			{
				break;
			}
			LayRow(NextRow);
			++NextRow;
		}
		SweepPlayers(From, Radius);
		bRunOut = Radius >= Farthest - 0.5f;
	}
	const float Life = Shock.RiseSeconds + Shock.HoldSeconds + Shock.SinkSeconds;
	for (int32 Index = Spikes.Num() - 1; Index >= 0; --Index)
	{
		Spikes[Index].Age += DeltaSeconds;
		if (Spikes[Index].Age >= Life)
		{
			Spikes.RemoveAtSwap(Index, 1, EAllowShrinking::No);
		}
	}
	DrawSpikes();
	if (bRunOut && Spikes.IsEmpty() && !bIdle)
	{
		// Kept by its sentinel (and that still stands): idle, its spikes hidden, until it's sent again; else gone.
		if (bKeep && Launch.Causer.IsValid())
		{
			bIdle = true;
			SetActorTickEnabled(false);
		}
		else
		{
			Destroy();
		}
	}
}

// ---------------------------------------------------------------------------
// Its rows
// ---------------------------------------------------------------------------

float AStoneSentinelShockwave::BearingOf(const FVector& Where) const
{
	const FVector To = Where - Launch.Center;
	return FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(To.Y), static_cast<float>(To.X)));
}

int32 AStoneSentinelShockwave::SectorOf(float Bearing) const
{
	const int32 Count = FMath::Max(Sectors.Num(), 1);
	const float Step = 360.f / static_cast<float>(Count);
	const float Round = FMath::Fmod(Bearing + 360.f, 360.f);
	return FMath::Clamp(FMath::FloorToInt32(Round / Step), 0, Count - 1);
}

bool AStoneSentinelShockwave::FindRowGround(FSector& Sector, const FVector& Point, float RowRadius, FVector& OutGround) const
{
	if (Sector.bStopped)
	{
		return false;
	}
	const UWorld* World = GetWorld();
	const float Scale = FMath::Max(Launch.Size, 0.05f);
	const float Step = Launch.Tuning.MaxStep * Scale;
	const FVector Last = Sector.Ground;
	const FVector Above(Point.X, Point.Y, Last.Z + Step + LookAbove * Scale);
	const FVector Below(Point.X, Point.Y, Last.Z - Step - LookBelow * Scale);
	FHitResult Hit;
	if (!World || !World->LineTraceSingleByObjectType(Hit, Above, Below, FCollisionObjectQueryParams(ECC_WorldStatic), WorldQuery))
	{
		// Over an edge into nothing (a test level has no ground: it runs on at its middle's height).
		if (!World || World->IsGameWorld())
		{
			return false;
		}
		OutGround = FVector(Point.X, Point.Y, Last.Z);
		return true;
	}
	const FVector Ground = Hit.ImpactPoint;
	if (Hit.ImpactNormal.Z < FloorNormalZ || FMath::Abs(Ground.Z - Last.Z) > Step)
	{
		// A face too steep to run along, or a step up or down too high: it stops there.
		return false;
	}
	// A wall across its way (a crypt's side, a fence's stone foot) stops it as well.
	const FVector Lift(0.0, 0.0, WallLookHeight * Scale);
	if (World->LineTraceTestByObjectType(Last + Lift, Ground + Lift, FCollisionObjectQueryParams(ECC_WorldStatic), WorldQuery))
	{
		return false;
	}
	OutGround = Ground;
	return true;
}

void AStoneSentinelShockwave::LayRow(int32 Row)
{
	const FSentinelShockTuning& Shock = Launch.Tuning;
	const float Scale = FMath::Max(Launch.Size, 0.05f);
	const float RowRadius = (Shock.StartRadius + static_cast<float>(Row) * Shock.RowSpacing) * Scale;
	const int32 Count = Sectors.Num();
	if (Count <= 0)
	{
		return;
	}
	const float Step = 360.f / static_cast<float>(Count);
	// The ground along every way out first (so a spike between two ways can stand between their grounds).
	TArray<bool, TInlineAllocator<64>> Laid;
	Laid.SetNumZeroed(Count);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FSector& Sector = Sectors[Index];
		const float Middle = FMath::DegreesToRadians((static_cast<float>(Index) + 0.5f) * Step);
		const FVector Point = Launch.Center + FVector(FMath::Cos(Middle), FMath::Sin(Middle), 0.f) * RowRadius;
		FVector Ground;
		if (FindRowGround(Sector, Point, RowRadius, Ground))
		{
			Sector.Ground = Ground;
			Laid[Index] = true;
		}
		else if (!Sector.bStopped)
		{
			Sector.bStopped = true;
			Sector.StopRadius = RowRadius - Shock.RowSpacing * Scale * 0.5f;
		}
	}

	UWorld* World = GetWorld();
	UGravemotherFX* Effects = UGravemotherFX::Get(this);
	FWeaponFX* Weapons = nullptr;
	if (World && World->IsGameWorld())
	{
		if (UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>())
		{
			Weapons = &Bullets->GetEffects();
			Weapons->Initialize(World);
		}
	}
	const float Farthest = FMath::Max(Shock.MaxRadius * Scale, 1.f);
	TArray<FVector, TInlineAllocator<2>> Lenses;
	StoneSentinelFX::GatherLenses(this, Lenses);
	const float Tall = FMath::Lerp(Shock.SpikeTall, Shock.SpikeShort, FMath::Clamp(RowRadius / Farthest, 0.f, 1.f)) * Scale;
	const float ArcPerSector = FMath::DegreesToRadians(Step) * RowRadius;
	const int32 PerSector = FMath::Max(1, FMath::RoundToInt32(ArcPerSector / FMath::Max(Shock.SpikeSpacing * Scale, 1.f)));
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (!Laid[Index])
		{
			continue;
		}
		const FSector& Sector = Sectors[Index];
		for (int32 Piece = 0; Piece < PerSector; ++Piece)
		{
			const float Bearing = (static_cast<float>(Index) + (static_cast<float>(Piece) + Random.FRandRange(0.2f, 0.8f)) / static_cast<float>(PerSector)) * Step;
			if (StoneSentinelMoves::IsInGap(Bearing, Launch.Gaps, Shock.GapDegrees))
			{
				continue;
			}
			// Its foot between this way's ground and the next way's on its side, a little in or out of the row.
			const float Within = Bearing / Step - static_cast<float>(Index) - 0.5f;
			const int32 Other = (Index + (Within >= 0.f ? 1 : Count - 1)) % Count;
			const float FootZ = Laid[Other]
				? FMath::Lerp(static_cast<float>(Sector.Ground.Z), static_cast<float>(Sectors[Other].Ground.Z), FMath::Abs(Within))
				: static_cast<float>(Sector.Ground.Z);
			const float Radians = FMath::DegreesToRadians(Bearing);
			const FVector Out(FMath::Cos(Radians), FMath::Sin(Radians), 0.f);
			const FVector Foot = Launch.Center + Out * (RowRadius * Random.FRandRange(0.94f, 1.06f));
			FSpike& Spike = Spikes.AddDefaulted_GetRef();
			Spike.Set = Random.FRand() < SlabShare ? 0 : 1;
			Spike.Base = FVector(Foot.X, Foot.Y, FootZ);
			Spike.Height = Tall * Random.FRandRange(HeightJitterMin, HeightJitterMax);
			Spike.Width = Spike.Height * Random.FRandRange(WidthMin, WidthMax);
			// Leaning out from the middle (a turn round the way across it brings its top outward), a little to its side, turned
			// on itself.
			const FVector Across = FVector::CrossProduct(FVector::UpVector, Out).GetSafeNormal();
			const FQuat LeanOut(Across, FMath::DegreesToRadians(Random.FRandRange(LeanMin, LeanMax)));
			const FQuat LeanSide(Out, FMath::DegreesToRadians(Random.FRandRange(-SideLean, SideLean)));
			const FQuat Turn(FVector::UpVector, Random.FRandRange(0.f, 2.f * PI));
			Spike.Rotation = (LeanSide * LeanOut * Turn).GetNormalized();
		}
		// Dust shoved along the front, a puff kicked up, debris thrown (a few ways a row, staggered row to row).
		const FVector Ground = Sector.Ground;
		const float Middle = FMath::DegreesToRadians((static_cast<float>(Index) + 0.5f) * Step);
		const FVector Way(FMath::Cos(Middle), FMath::Sin(Middle), 0.f);
		if (Weapons && (Index + Row) % WaveEvery == 0
			&& !StoneSentinelFX::CrowdsLens(LensFrom(Lenses, Ground), WaveSmokeSize, WaveSmokeTravel))
		{
			Weapons->SpawnWaveLift(Ground + FVector(0.0, 0.0, 5.0), Way, DustLight, 0.9f);
		}
		if (Effects && (Index + Row) % PuffEvery == 0)
		{
			FGraveDustBurst Puff;
			Puff.Kind = EGraveDust::Floor;
			Puff.Count = 1;
			Puff.Spread = 30.f * Scale;
			Puff.SpreadUp = 10.f;
			Puff.StartSize = 50.f * Scale;
			Puff.EndSize = 120.f * Scale;
			Puff.Life = 0.8f;
			Puff.Opacity = 0.26f;
			Puff.Drift = FVector(Way * 60.f) + FVector(0.0, 0.0, 20.0);
			Puff.Outward = 80.f;
			StoneSentinelFX::Dust(this, Ground, Puff);
		}
		if (Effects && (Index + Row) % DebrisEvery == 0)
		{
			Effects->Chunks(EGraveChunk::Debris, Ground + FVector(0.0, 0.0, 10.0), (Way * 0.4f + FVector::UpVector).GetSafeNormal(), 1, 380.f, 0.7f);
		}
	}
}

// ---------------------------------------------------------------------------
// Its spikes on screen
// ---------------------------------------------------------------------------

void AStoneSentinelShockwave::DrawSpikes()
{
	const FSentinelShockTuning& Shock = Launch.Tuning;
	TArray<FTransform> Sets[2];
	for (const FSpike& Spike : Spikes)
	{
		const int32 Set = FMath::Clamp(Spike.Set, 0, 1);
		const float Risen = PunchOut(Spike.Age / FMath::Max(Shock.RiseSeconds, 0.01f));
		const float Sunk = FMath::Clamp((Spike.Age - Shock.RiseSeconds - Shock.HoldSeconds) / FMath::Max(Shock.SinkSeconds, 0.01f), 0.f, 1.f);
		// Up out of the ground, standing, then back down into it (slowly at first, the earth taking it back).
		const float Shown = Risen * (1.f - FMath::Square(Sunk));
		const float Width = Spike.Width * FMath::Lerp(RiseWidthShare, 1.f, Risen);
		const FVector Scale3(Width / SetAcross[Set], Width / SetAcross[Set], Spike.Height / SetTall[Set]);
		const float Offset = -Spike.Height * (1.f - Shown) - Spike.Height * Buried - SetBottom[Set] * static_cast<float>(Scale3.Z);
		const FVector Up = Spike.Rotation.GetUpVector();
		Sets[Set].Add(FTransform(Spike.Rotation, Spike.Base + Up * Offset, Scale3));
	}
	UInstancedStaticMeshComponent* Components[2] = { SlabSet.Get(), ChunkSet.Get() };
	bool bAnything = false;
	for (int32 Index = 0; Index < 2; ++Index)
	{
		UInstancedStaticMeshComponent* Set = Components[Index];
		if (!Set || !Set->GetStaticMesh())
		{
			continue;
		}
		TArray<FTransform>& Transforms = Sets[Index];
		bAnything |= !Transforms.IsEmpty();
		// A pool that only grows: new instances added once, spare ones hidden (shrunk to nothing under the ground), so a
		// frame is one transform update, never the instances torn down and built again.
		const int32 Have = Set->GetInstanceCount();
		const int32 Want = Transforms.Num();
		if (Want > Have)
		{
			TArray<FTransform> Extra;
			Extra.Append(Transforms.GetData() + Have, Want - Have);
			Set->AddInstances(Extra, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);
			Transforms.SetNum(Have);
		}
		else
		{
			const FTransform Hidden(FQuat::Identity, Launch.Center - FVector(0.0, 0.0, 500.0), FVector(0.001f));
			while (Transforms.Num() < Have)
			{
				Transforms.Add(Hidden);
			}
		}
		if (Transforms.Num() > 0)
		{
			Set->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ true, /*bTeleport*/ true);
		}
	}
	bDrawn = bAnything;
}

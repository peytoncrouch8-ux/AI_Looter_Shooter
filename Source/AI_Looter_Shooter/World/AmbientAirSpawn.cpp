// UAmbientAirSubsystem's placing: what's near the view (trees, lit lamps), what the place round it is like (a survey
// point now and then: how much is open grass, how much in the sun), how many of each kind are wanted, and new ones put
// where their kind belongs (motes in the sun, pollen and fluff over open grass, leaves in a near tree's crown, moths at a
// lit lamp), a few tries a frame within its traces. Those over their wanted count leave, fading.

#include "World/AmbientAirSubsystem.h"
#include "World/AmbientAirRules.h"
#include "Weapons/ImpactSurface.h"
#include "Components/LightComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	/** How often (s) the trees and lamps near the view are found again, or as soon as the view has moved this far (cm). */
	constexpr float NearSeconds = 0.5f;
	constexpr float NearMove = 300.f;
	/** The trees' grid cell (cm), as AmbientAirSubsystem.cpp files them; cells this many either side are looked in. */
	constexpr float TreeCell = 2500.f;
	constexpr int32 TreeSpan = 2;
	/** The most trees counted near: a forest's edge needs no more for its leaves. */
	constexpr int32 MaxNearTrees = 64;
	/** A survey point this often (s); each moves the place's shares this far toward what it found. */
	constexpr float SurveySeconds = 0.3f;
	constexpr float SurveyWeight = 0.1f;
	/** A point's ground is looked for from this far above it, down to this far below (cm). */
	constexpr float ProbeAbove = 1200.f;
	constexpr float ProbeBelow = 1500.f;
	/** The share placed in front of the view (its field of view and a little more), where they're seen. */
	constexpr float InFront = 0.75f;
	constexpr float FrontMargin = 15.f;
	/** A kind's wait after a miss (s), doubling to the longest. */
	constexpr float MissWait = 0.1f;
	constexpr float LongestWait = 1.f;
	/** How long leaving takes (s): the usual way, and when the view goes into the mine. */
	constexpr float LeavingSeconds = 1.f;
	constexpr float ShelterLeaving = 0.3f;
	/** A falling leaf's speed (cm/s) for working out where it comes down. */
	constexpr float LeafFall = 55.f;

	const FName GroundTag(TEXT("Ground"));

	FIntPoint CellOf(const FVector& Where)
	{
		return FIntPoint(FMath::FloorToInt(static_cast<float>(Where.X) / TreeCell), FMath::FloorToInt(static_cast<float>(Where.Y) / TreeCell));
	}
}

// ---------------------------------------------------------------------------
// What's round the view
// ---------------------------------------------------------------------------

void UAmbientAirSubsystem::RefreshSurroundings(const FAirView& View, float DeltaSeconds)
{
	for (float& Wait : KindWait)
	{
		Wait = FMath::Max(Wait - DeltaSeconds, 0.f);
	}
	NearIn -= DeltaSeconds;
	if (NearIn <= 0.f || FVector::DistSquared(View.Location, NearFrom) > FMath::Square(static_cast<double>(NearMove)))
	{
		NearIn = NearSeconds;
		NearFrom = View.Location;
		// The broadleaf trees whose leaves can fall within the leaves' reach.
		NearTrees.Reset();
		const float LeafReach = AmbientAirRules::Reach(EAirKind::Leaf, false);
		const FIntPoint Middle = CellOf(View.Location);
		for (int32 X = -TreeSpan; X <= TreeSpan && NearTrees.Num() < MaxNearTrees; ++X)
		{
			for (int32 Y = -TreeSpan; Y <= TreeSpan && NearTrees.Num() < MaxNearTrees; ++Y)
			{
				const TArray<int32>* Cell = TreeCells.Find(FIntPoint(Middle.X + X, Middle.Y + Y));
				if (!Cell)
				{
					continue;
				}
				for (int32 Index : *Cell)
				{
					if (FVector::DistSquared2D(Trees[Index].Base, View.Location) < FMath::Square(static_cast<double>(LeafReach)))
					{
						NearTrees.Add(Index);
						if (NearTrees.Num() >= MaxNearTrees)
						{
							break;
						}
					}
				}
			}
		}
		// The lit lamps the moths can come to.
		NearLamps.Reset();
		const float MothReach = AmbientAirRules::Reach(EAirKind::Moth, false);
		for (int32 Index = 0; Index < Lamps.Num(); ++Index)
		{
			if (FVector::DistSquared(Lamps[Index].Location, View.Location) < FMath::Square(static_cast<double>(MothReach)) && IsLampLit(Index))
			{
				NearLamps.Add(Index);
			}
		}
		Around.NearTrees = NearTrees.Num();
		Around.NearLamps = NearLamps.Num();
		Thin(View);
	}

	// A survey point now and then: how much of the place round the view is open grass, and how much is in the sun.
	SurveyIn -= DeltaSeconds;
	if (SurveyIn <= 0.f && !View.bSheltered && (PlaceProbe || TracesLeft >= 2))
	{
		SurveyIn = SurveySeconds;
		const FVector Point = PointAround(View, 150.f, AmbientAirRules::Reach(EAirKind::Fluff, false), static_cast<float>(View.Location.Z));
		FAirPlace Found;
		if (Probe(Point, true, true, Found))
		{
			const bool bMeadow = Found.bGround && Found.bOpenSky && AmbientAirRules::IsMeadow(Found.Surface);
			Around.GrassShare = FMath::Lerp(Around.GrassShare, bMeadow ? 1.f : 0.f, SurveyWeight);
			Around.SunShare = FMath::Lerp(Around.SunShare, Found.bInSun ? 1.f : 0.f, SurveyWeight);
		}
	}
}

bool UAmbientAirSubsystem::IsLampLit(int32 Lamp) const
{
	if (!Lamps.IsValidIndex(Lamp))
	{
		return false;
	}
	const FAirLamp& Each = Lamps[Lamp];
	const ULightComponent* Light = Each.Light.Get();
	if (!Light)
	{
		// A test's lamp has no light and says; a lamp whose light has gone is out.
		return Each.Light.IsExplicitlyNull() && Each.bLit;
	}
	const AActor* LightOwner = Light->GetOwner();
	return Light->IsVisible() && Light->Intensity > 0.01f && !(LightOwner && LightOwner->IsHidden());
}

// ---------------------------------------------------------------------------
// New ones
// ---------------------------------------------------------------------------

void UAmbientAirSubsystem::Place(const FAirView& View)
{
	if (View.bSheltered)
	{
		return;
	}
	const float Density = GetDensity();
	for (int32 Try = 0; Try < MaxPlacementsPerFrame; ++Try)
	{
		if (NumLive() >= AmbientAirRules::Budget)
		{
			return;
		}
		// The kind furthest short of its wanted count (as a share of it), unless it's waiting after a miss or can't be drawn.
		EAirKind Chosen = EAirKind::Count;
		float Shortest = 0.f;
		for (int32 Index = 0; Index < static_cast<int32>(EAirKind::Count); ++Index)
		{
			const EAirKind Kind = static_cast<EAirKind>(Index);
			if (KindWait[Index] > 0.f || (AmbientAirRules::IsPlacedByProbe(Kind) && !PlaceProbe && TracesLeft <= 0))
			{
				continue;
			}
			if (bComponentsMade && (Kind == EAirKind::Pollen || Kind == EAirKind::Fluff) && !Puffs.IsValid())
			{
				continue;
			}
			const int32 Wanted = AmbientAirRules::Target(Kind, View, Around, Density);
			const int32 Short = Wanted - Counts[Index];
			const float Share = Short >= 1 ? static_cast<float>(Short) / FMath::Max(Wanted, 1) : 0.f;
			if (Share > Shortest)
			{
				Shortest = Share;
				Chosen = Kind;
			}
		}
		if (Chosen == EAirKind::Count)
		{
			return;
		}
		const int32 Index = static_cast<int32>(Chosen);
		if (TryPlace(Chosen, View))
		{
			KindMisses[Index] = 0;
		}
		else
		{
			// No place for it this time (a road, the shade, a dark room): it waits, longer each miss, so it costs little.
			KindWait[Index] = FMath::Min(MissWait * static_cast<float>(1 << FMath::Min(KindMisses[Index], 4)), LongestWait);
			++KindMisses[Index];
		}
	}
}

bool UAmbientAirSubsystem::TryPlace(EAirKind Kind, const FAirView& View)
{
	const int32 Slot = FreeSlot();
	if (Slot == INDEX_NONE)
	{
		return false;
	}
	FAirParticle Particle;
	Begin(Particle, Kind);
	bool bPlaced = false;
	switch (Kind)
	{
	case EAirKind::Leaf: bPlaced = PlaceLeaf(Particle, View); break;
	case EAirKind::Moth: bPlaced = PlaceMoth(Particle, View); break;
	default: bPlaced = PlaceFromProbe(Particle, Kind, View); break;
	}
	if (!bPlaced)
	{
		return false;
	}
	Particle.bLive = true;
	Particles[Slot] = Particle;
	++Counts[static_cast<int32>(Kind)];
	return true;
}

void UAmbientAirSubsystem::Begin(FAirParticle& Particle, EAirKind Kind)
{
	const FAirKindLook& Look = AmbientAirRules::Look(Kind);
	Particle = FAirParticle();
	Particle.Kind = Kind;
	Particle.Size = Random.FRandRange(Look.SizeMin, Look.SizeMax);
	Particle.Life = Random.FRandRange(Look.LifeMin, Look.LifeMax);
	Particle.Phase = FVector(Random.FRandRange(0.f, UE_TWO_PI), Random.FRandRange(0.f, UE_TWO_PI), Random.FRandRange(0.f, UE_TWO_PI));
	Particle.Wander = Random.GetUnitVector() * Look.Wander;
	Particle.Timer = Random.FRandRange(0.f, 2.f);
	Particle.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, UE_TWO_PI));
	Particle.Spin = Random.GetUnitVector() * Random.FRandRange(1.5f, 4.f);
	Particle.Sway = FRotator(0.f, Random.FRandRange(0.f, 360.f), 0.f).Vector();
}

int32 UAmbientAirSubsystem::FreeSlot()
{
	for (int32 Offset = 0; Offset < Particles.Num(); ++Offset)
	{
		const int32 Index = (SlotCursor + Offset) % Particles.Num();
		if (!Particles[Index].bLive)
		{
			SlotCursor = (Index + 1) % Particles.Num();
			return Index;
		}
	}
	return INDEX_NONE;
}

FVector UAmbientAirSubsystem::PointAround(const FAirView& View, float MinReach, float MaxReach, float Z)
{
	// Mostly in front, where they'll be seen; some all round, so turning finds some there already.
	float Yaw = Random.FRandRange(0.f, 360.f);
	if (Random.FRand() < InFront)
	{
		const float Facing = FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(View.Direction.Y), static_cast<float>(View.Direction.X)));
		const float Half = FMath::Min(View.FieldOfView * 0.5f + FrontMargin, 180.f);
		Yaw = Facing + Random.FRandRange(-Half, Half);
	}
	// Even over the ground between the two reaches.
	const float Low = FMath::Max(MinReach, 0.f);
	const float High = FMath::Max(MaxReach, Low + 1.f);
	const float Distance = FMath::Sqrt(Random.FRandRange(Low * Low, High * High));
	const float Radians = FMath::DegreesToRadians(Yaw);
	return FVector(View.Location.X + FMath::Cos(Radians) * Distance, View.Location.Y + FMath::Sin(Radians) * Distance, Z);
}

bool UAmbientAirSubsystem::Probe(const FVector& Point, bool bGround, bool bSun, FAirPlace& OutPlace)
{
	OutPlace = FAirPlace();
	if (PlaceProbe)
	{
		return PlaceProbe(Point, OutPlace);
	}
	const UWorld* World = GetWorld();
	if (!World || TracesLeft < (bGround ? 1 : 0) + (bSun ? 1 : 0))
	{
		return false;
	}
	if (bGround)
	{
		// From above it: walkable terrain met first is ground open to the sky; a roof, a porch or a rock met first is not.
		--TracesLeft;
		++TracesThisFrame;
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, Point + FVector(0.0, 0.0, ProbeAbove), Point - FVector(0.0, 0.0, ProbeBelow),
			FCollisionObjectQueryParams(ECC_WorldStatic), GroundParams))
		{
			OutPlace.bGround = true;
			OutPlace.GroundZ = static_cast<float>(Hit.ImpactPoint.Z);
			const AActor* Actor = Hit.GetActor();
			OutPlace.bOpenSky = Actor && Actor->ActorHasTag(GroundTag);
			OutPlace.Surface = OutPlace.bOpenSky ? ImpactSurface::Of(Hit) : EImpactMaterial::Unknown;
		}
	}
	if (bSun)
	{
		--TracesLeft;
		++TracesThisFrame;
		OutPlace.bInSun = IsInSun(Point);
	}
	return true;
}

bool UAmbientAirSubsystem::PlaceFromProbe(FAirParticle& Particle, EAirKind Kind, const FAirView& View)
{
	const FAirKindLook& Look = AmbientAirRules::Look(Kind);
	const float Reach = AmbientAirRules::Reach(Kind, View.bUnderCover);
	if (Kind == EAirKind::Mote)
	{
		// At eye height, anywhere the sun reaches: under a roof that's its shafts through the door and the windows.
		const FVector Point = PointAround(View, 80.f, Reach, static_cast<float>(View.Location.Z) + Random.FRandRange(Look.HeightMin, Look.HeightMax));
		FAirPlace Found;
		if (!Probe(Point, false, true, Found) || !AmbientAirRules::Belongs(Kind, Found, View))
		{
			return false;
		}
		Particle.Location = Point;
		Particle.bInSun = true;
		return true;
	}
	// Pollen and fluff: over open grass, low over it.
	FVector Point = PointAround(View, 120.f, Reach, static_cast<float>(View.Location.Z));
	FAirPlace Found;
	if (!Probe(Point, true, false, Found) || !Found.bGround)
	{
		return false;
	}
	Found.bInSun = true; // the sun matters only to motes here; it's looked at below for the light
	if (!AmbientAirRules::Belongs(Kind, Found, View))
	{
		return false;
	}
	Point.Z = Found.GroundZ + Random.FRandRange(Look.HeightMin, Look.HeightMax);
	// Its light: a look toward the sun if the frame has one to spare, else the place's share of sun decides.
	FAirPlace Lit;
	if (Probe(Point, false, true, Lit))
	{
		Particle.bInSun = Lit.bInSun;
	}
	else
	{
		Particle.bInSun = Random.FRand() < Around.SunShare;
	}
	Particle.Location = Point;
	Particle.Floor = Found.GroundZ;
	return true;
}

bool UAmbientAirSubsystem::PlaceLeaf(FAirParticle& Particle, const FAirView& View)
{
	if (NearTrees.IsEmpty())
	{
		return false;
	}
	const int32 TreeIndex = NearTrees[Random.RandRange(0, NearTrees.Num() - 1)];
	const FAirTree& Tree = Trees[TreeIndex];
	// Off somewhere in its crown.
	const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
	const float Out = FMath::Sqrt(Random.FRand()) * Tree.CrownRadius * 0.85f;
	const float Z = Random.FRandRange(Tree.CrownBottom, FMath::Max(Tree.CrownTop, Tree.CrownBottom + 1.f));
	Particle.Location = FVector(Tree.Base.X + FMath::Cos(Angle) * Out, Tree.Base.Y + FMath::Sin(Angle) * Out, Z);
	Particle.Anchor = TreeIndex;
	Particle.Floor = static_cast<float>(Tree.Base.Z);
	Particle.Velocity = FVector(0.0, 0.0, -20.0);
	// Where it will come down (blown on by the breeze as it falls): the ground there, so it lies on a slope too, never in
	// the air. One look, if the frame has one to spare; else the tree's foot.
	const float Fall = FMath::Max(Z - Particle.Floor, 0.f) / LeafFall;
	const FVector Down = Particle.Location + View.BreezeDirection * (AmbientAirRules::Look(EAirKind::Leaf).Breeze * Fall);
	if (PlaceProbe)
	{
		FAirPlace Found;
		if (PlaceProbe(Down, Found) && Found.bGround)
		{
			Particle.Floor = Found.GroundZ;
		}
	}
	else if (TracesLeft > 0)
	{
		--TracesLeft;
		++TracesThisFrame;
		FHitResult Hit;
		if (const UWorld* World = GetWorld())
		{
			if (World->LineTraceSingleByObjectType(Hit, FVector(Down.X, Down.Y, Tree.CrownBottom), FVector(Down.X, Down.Y, Tree.Base.Z - 600.0),
				FCollisionObjectQueryParams(ECC_WorldStatic), GroundParams))
			{
				Particle.Floor = static_cast<float>(Hit.ImpactPoint.Z);
			}
		}
	}
	return true;
}

bool UAmbientAirSubsystem::PlaceMoth(FAirParticle& Particle, const FAirView& View)
{
	if (NearLamps.IsEmpty())
	{
		return false;
	}
	// A lamp with room for one more (a few tries).
	for (int32 Try = 0; Try < 3; ++Try)
	{
		const int32 Lamp = NearLamps[Random.RandRange(0, NearLamps.Num() - 1)];
		int32 There = 0;
		for (const FAirParticle& Each : Particles)
		{
			There += Each.bLive && Each.Kind == EAirKind::Moth && Each.Anchor == Lamp ? 1 : 0;
		}
		if (There >= AmbientAirRules::MothsPerLamp)
		{
			continue;
		}
		const FAirKindLook& Look = AmbientAirRules::Look(EAirKind::Moth);
		Particle.Anchor = Lamp;
		Particle.Orbit = Random.FRandRange(25.f, 80.f);
		Particle.Height = Random.FRandRange(Look.HeightMin, Look.HeightMax);
		Particle.OrbitAngle = Random.FRandRange(0.f, UE_TWO_PI);
		Particle.OrbitSpeed = Random.FRandRange(1.8f, 4.5f) * (Random.FRand() < 0.5f ? -1.f : 1.f);
		// Its next course (Wander: how far out, how high, how fast).
		Particle.Wander = FVector(Particle.Orbit, Particle.Height, Particle.OrbitSpeed);
		Particle.Location = Lamps[Lamp].Location + FVector(FMath::Cos(Particle.OrbitAngle) * Particle.Orbit,
			FMath::Sin(Particle.OrbitAngle) * Particle.Orbit, Particle.Height);
		return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
// Too many
// ---------------------------------------------------------------------------

void UAmbientAirSubsystem::Thin(const FAirView& View)
{
	const float Density = GetDensity();
	const float Fade = View.bSheltered ? ShelterLeaving : LeavingSeconds;
	int32 Staying[static_cast<int32>(EAirKind::Count)] = {};
	for (const FAirParticle& Particle : Particles)
	{
		Staying[static_cast<int32>(Particle.Kind)] += Particle.bLive && Particle.Leaving <= 0.f ? 1 : 0;
	}
	for (int32 Index = 0; Index < static_cast<int32>(EAirKind::Count); ++Index)
	{
		const EAirKind Kind = static_cast<EAirKind>(Index);
		const int32 Wanted = AmbientAirRules::Target(Kind, View, Around, Density);
		// A few over is fine (the place's shares wander); none wanted, all go. Those already leaving aren't counted again.
		int32 Excess = Staying[Index] - Wanted;
		if (Wanted > 0 && Excess <= FMath::Max(2, Wanted / 10))
		{
			Excess = 0;
		}
		for (FAirParticle& Particle : Particles)
		{
			if (!Particle.bLive || Particle.Kind != Kind)
			{
				continue;
			}
			// A moth whose lamp went out or out of reach goes whatever the count.
			const bool bLampGone = Kind == EAirKind::Moth && !NearLamps.Contains(Particle.Anchor);
			if (Particle.Leaving <= 0.f && (Excess > 0 || bLampGone))
			{
				Particle.Leaving = Fade;
				Excess -= 1;
			}
			else if (View.bSheltered && Particle.Leaving > Fade)
			{
				Particle.Leaving = Fade;
			}
		}
	}
}

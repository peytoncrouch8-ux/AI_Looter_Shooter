#include "Weapons/WeaponBrass.h"
#include "Audio/LooterSound.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	const TCHAR* const BrassCylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	/** The ammo pickups' brass and paper hull (Art/Models/Loot/Ammo.py), so a spent case is the round the player picked up. */
	const TCHAR* const BrassMaterialPath = TEXT("/Game/Art/Materials/MI_AmmoCaseBrass.MI_AmmoCaseBrass");
	const TCHAR* const BrassHullMaterialPath = TEXT("/Game/Art/Materials/MI_AmmoShellHull.MI_AmmoShellHull");
	/** Used until those exist: the flat stylized surface in their colors. */
	const TCHAR* const BrassFallbackPath = TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface");
	const FLinearColor BrassFallbackColor(0.62f, 0.42f, 0.14f);
	const FLinearColor BrassHullFallbackColor(0.32f, 0.07f, 0.05f);

	// Until Audio/LooterSoundCues.h lists them (a cue in the header without its sounds fails a test).
	const TCHAR* const CasingDirtCue = TEXT("Weapon.Casing.Dirt");
	const TCHAR* const CasingStoneCue = TEXT("Weapon.Casing.Stone");
	const TCHAR* const CasingWoodCue = TEXT("Weapon.Casing.Wood");
	const TCHAR* const CasingMetalCue = TEXT("Weapon.Casing.Metal");
	const TCHAR* const CasingWaterCue = TEXT("Weapon.Casing.Water");
	const TCHAR* const CasingHullCue = TEXT("Weapon.Casing.Hull");

	constexpr float BrassGravity = 980.f;
	/** The longest a leg of the arc may last before the case is given up. */
	constexpr float BrassMaxLegSeconds = 2.5f;
	/** A shotgun hull's brass head (cm long, and its diameter: a hair wider than the paper). */
	constexpr float BrassHullHead = 1.3f;
	constexpr float BrassHullHeadWidth = 2.2f;
	/** Sounds: never two closer than this (a burst's cases land together), never from farther than this. */
	constexpr double BrassSoundGap = 0.035;
	constexpr float BrassSoundRange = 2500.f;
	/** Past this the cases aren't drawn (cm): they're a few pixels at most. */
	constexpr int32 BrassCullStart = 2500;
	constexpr int32 BrassCullEnd = 3500;

	FVector BrassArc(const FVector& Start, const FVector& Velocity, float Time)
	{
		return Start + Velocity * Time + FVector(0.0, 0.0, -0.5 * BrassGravity * Time * Time);
	}

	UMaterialInterface* BrassMaterial(UObject* Outer, const TCHAR* Path, const FLinearColor& Fallback)
	{
		if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, Path))
		{
			return Material;
		}
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, BrassFallbackPath);
		UMaterialInstanceDynamic* Instance = Base ? UMaterialInstanceDynamic::Create(Base, Outer) : nullptr;
		if (!Instance)
		{
			return Base;
		}
		Instance->SetVectorParameterValue(TEXT("Color"), Fallback);
		Instance->SetScalarParameterValue(TEXT("Variation"), 0.05f);
		return Instance;
	}

	UInstancedStaticMeshComponent* BrassInstancesOn(AActor* Owner, const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Material)
	{
		UInstancedStaticMeshComponent* Instances = NewObject<UInstancedStaticMeshComponent>(Owner, Name, RF_Transient);
		Instances->SetupAttachment(Owner->GetRootComponent());
		Instances->SetMobility(EComponentMobility::Movable);
		Instances->SetStaticMesh(Mesh);
		Instances->SetMaterial(0, Material);
		Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		// A case is a centimetre across: its shadow is never seen, and costs a shadow draw.
		Instances->SetCastShadow(false);
		Instances->SetCanEverAffectNavigation(false);
		Instances->bReceivesDecals = false;
		Instances->SetCullDistances(BrassCullStart, BrassCullEnd);
		Instances->RegisterComponent();
		return Instances;
	}

	/** Replaces a component's instances with these (moved in place when the count is unchanged), as FWeaponFX draws. */
	void BrassDraw(UInstancedStaticMeshComponent* Instances, const TArray<FTransform>& Transforms)
	{
		if (!Instances)
		{
			return;
		}
		if (Instances->GetInstanceCount() != Transforms.Num())
		{
			Instances->ClearInstances();
			if (!Transforms.IsEmpty())
			{
				Instances->AddInstances(Transforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);
			}
			Instances->MarkRenderStateDirty();
		}
		else if (!Transforms.IsEmpty())
		{
			Instances->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
		}
	}

	FVector BrassPerpendicular(const FVector& Direction)
	{
		const FVector Other = FMath::Abs(Direction.Z) < 0.9 ? FVector::UpVector : FVector::ForwardVector;
		return FVector::CrossProduct(Direction, Other).GetSafeNormal();
	}
}

// ---------------------------------------------------------------------------
// Rules
// ---------------------------------------------------------------------------

ECasingKind FWeaponBrass::KindFor(EWeaponKind Kind)
{
	switch (Kind)
	{
	case EWeaponKind::Shotgun:
		return ECasingKind::Hull;
	case EWeaponKind::Revolver:
		return ECasingKind::Pistol;
	// The frontier-tech guns throw a bottlenecked case out of their port (the sniper's as its bolt is drawn back; the
	// launcher throws none: AWeaponBase::EjectAfterShot).
	case EWeaponKind::SMG:
	case EWeaponKind::Sniper:
	case EWeaponKind::LMG:
	case EWeaponKind::Launcher:
	case EWeaponKind::Rifle:
	case EWeaponKind::None:
	default:
		return ECasingKind::Rifle;
	}
}

FVector2D FWeaponBrass::SizeOf(ECasingKind Kind)
{
	// Length and diameter (cm): a rifle case, a 12-gauge hull, a heavy six-gun's case.
	switch (Kind)
	{
	case ECasingKind::Hull:
		return FVector2D(6.4, 2.05);
	case ECasingKind::Pistol:
		return FVector2D(3.1, 1.15);
	case ECasingKind::Rifle:
	default:
		return FVector2D(4.5, 0.95);
	}
}

bool FWeaponBrass::IsSoft(EImpactMaterial Surface)
{
	switch (Surface)
	{
	case EImpactMaterial::Dirt:
	case EImpactMaterial::Grass:
	case EImpactMaterial::Sand:
	case EImpactMaterial::Hay:
	case EImpactMaterial::Cloth:
		return true;
	default:
		return false;
	}
}

FName FWeaponBrass::CueFor(ECasingKind Kind, EImpactMaterial Surface)
{
	if (Surface == EImpactMaterial::Water)
	{
		return CasingWaterCue;
	}
	if (IsSoft(Surface))
	{
		return CasingDirtCue;
	}
	// A paper hull knocks hollow on anything hard; brass rings by what it struck.
	if (Kind == ECasingKind::Hull)
	{
		return CasingHullCue;
	}
	switch (Surface)
	{
	case EImpactMaterial::Wood:
		return CasingWoodCue;
	case EImpactMaterial::Metal:
	case EImpactMaterial::SheetMetal:
		return CasingMetalCue;
	default:
		// Stone, gravel, glass, bone, and what nothing names: a bright clink.
		return CasingStoneCue;
	}
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

void FWeaponBrass::Initialize(UWorld* World)
{
	if (Owner.IsValid() || !World)
	{
		return;
	}
	WorldRef = World;
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
	if (!Actor)
	{
		return;
	}
#if WITH_EDITOR
	Actor->SetActorLabel(TEXT("WeaponBrass"));
#endif
	USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"), RF_Transient);
	Root->SetMobility(EComponentMobility::Movable);
	Actor->SetRootComponent(Root);
	Root->RegisterComponent();
	Owner = Actor;

	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, BrassCylinderPath);
	BrassInstances = BrassInstancesOn(Actor, TEXT("Brass"), Cylinder, BrassMaterial(Actor, BrassMaterialPath, BrassFallbackColor));
	HullInstances = BrassInstancesOn(Actor, TEXT("Hulls"), Cylinder, BrassMaterial(Actor, BrassHullMaterialPath, BrassHullFallbackColor));
}

void FWeaponBrass::Shutdown()
{
	if (AActor* Actor = Owner.Get())
	{
		Actor->Destroy();
	}
	Owner.Reset();
	Casings.Reset();
	bDrawnAnything = false;
	bDirty = false;
}

// ---------------------------------------------------------------------------
// Flight
// ---------------------------------------------------------------------------

void FWeaponBrass::Eject(const FCasingLaunch& Launch)
{
	// The last MaxCasings are kept: the oldest goes (the one lying longest, usually far behind by now).
	while (Casings.Num() >= MaxCasings)
	{
		Casings.RemoveAt(0, 1, EAllowShrinking::No);
	}
	FCasing& Casing = Casings.AddDefaulted_GetRef();
	Casing.Kind = Launch.Kind;
	Casing.Location = Launch.Location;
	Casing.Rotation = Launch.Rotation;
	Casing.Spin = Launch.Spin;
	Casing.Channel = Launch.Channel;
	Casing.Holder = Launch.Holder;
	Casing.Gun = Launch.Gun;
	StartLeg(Casing, Launch.Location, Launch.Velocity);
	LastLaunch = Launch;
	++Launches;
	bDirty = true;
}

void FWeaponBrass::StartLeg(FCasing& Casing, const FVector& From, const FVector& Velocity)
{
	Casing.LegStart = From;
	Casing.LegVelocity = Velocity;
	Casing.LegTime = 0.f;
	Casing.LegLanding = -1.f;
	// How long until it's fallen MaxFall below where it starts: the arc is traced that far ahead and no farther.
	const float Rise = static_cast<float>(Velocity.Z);
	Casing.LegHorizon = FMath::Clamp((Rise + FMath::Sqrt(Rise * Rise + 2.f * BrassGravity * MaxFall)) / BrassGravity, 0.05f, BrassMaxLegSeconds);

	UWorld* World = WorldRef.Get();
	if (!World)
	{
		return;
	}
	// The arc ahead in a few straight chords (three for the high first flight, two for a low bounce): the first one it
	// meets is where it lands. The face it lands on names the surface (complex), which picks its sound.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CasingTrace), /*bTraceComplex*/ true);
	Params.bReturnFaceIndex = true;
	Params.bReturnPhysicalMaterial = true;
	if (const AActor* Holder = Casing.Holder.Get())
	{
		Params.AddIgnoredActor(Holder);
	}
	if (const AActor* Gun = Casing.Gun.Get())
	{
		Params.AddIgnoredActor(Gun);
	}
	const int32 Chords = Casing.Contacts == 0 ? 3 : 2;
	FVector ChordStart = From;
	float ChordStartTime = 0.f;
	for (int32 Chord = 1; Chord <= Chords; ++Chord)
	{
		const float ChordEndTime = Casing.LegHorizon * static_cast<float>(Chord) / static_cast<float>(Chords);
		const FVector ChordEnd = BrassArc(From, Velocity, ChordEndTime);
		FHitResult Hit;
		// Started inside something (a gun pressed to a wall): that chord is skipped, not landed on.
		if (World->LineTraceSingleByChannel(Hit, ChordStart, ChordEnd, Casing.Channel, Params) && !Hit.bStartPenetrating)
		{
			Casing.LegLanding = FMath::Lerp(ChordStartTime, ChordEndTime, Hit.Time);
			Casing.LandPoint = Hit.ImpactPoint;
			Casing.LandNormal = Hit.ImpactNormal.IsNearlyZero() ? FVector::UpVector : FVector(Hit.ImpactNormal);
			Casing.LandSurface = ImpactSurface::Of(Hit);
			return;
		}
		ChordStart = ChordEnd;
		ChordStartTime = ChordEndTime;
	}
}

bool FWeaponBrass::Land(FCasing& Casing)
{
	const FVector Normal = Casing.LandNormal.GetSafeNormal();
	const float Radius = static_cast<float>(SizeOf(Casing.Kind).Y) * 0.5f;
	const FVector Velocity = Casing.LegVelocity + FVector(0.0, 0.0, -BrassGravity * Casing.LegLanding);
	Casing.Location = Casing.LandPoint + Normal * Radius;
	++Casing.Contacts;
	PlayLanding(Casing, Casing.LandPoint);
	// Into the water it goes, with a plip.
	if (Casing.LandSurface == EImpactMaterial::Water)
	{
		return false;
	}

	// The bounce: it keeps some of its speed along the surface, and gives back a little of what went into it: a ring off
	// stone and steel, hardly any off soft dirt or grass, where a case just settles in.
	const bool bSoft = IsSoft(Casing.LandSurface);
	const bool bHard = Casing.LandSurface == EImpactMaterial::Metal || Casing.LandSurface == EImpactMaterial::SheetMetal;
	const float Restitution = bSoft ? 0.12f : (bHard ? 0.45f : 0.36f);
	const float Keep = bSoft ? 0.3f : 0.6f;
	const double Into = FVector::DotProduct(Velocity, Normal);
	const FVector Across = Velocity - Normal * Into;
	FVector Bounce = Across * Keep - Normal * (FMath::Min(Into, 0.0) * Restitution);
	// Its corner catches: a kick to one side.
	Bounce += Random.GetUnitVector() * (bSoft ? 6.f : 18.f);

	// It lies on floor-like ground once it has nothing left to bounce with; a wall only ever turns it back down (a few
	// contacts more at most, so a case in a crack can't bounce forever).
	const bool bFloor = Normal.Z >= 0.5;
	const bool bSpent = Casing.Contacts > MaxBounces || Bounce.SizeSquared() < FMath::Square(60.0)
		|| FVector::DotProduct(Bounce, Normal) < 25.0;
	if ((bFloor && bSpent) || Casing.Contacts > MaxBounces + 3)
	{
		Casing.bResting = true;
		Casing.RestAge = 0.f;
		Casing.Spin = FVector::ZeroVector;
		// Lying on its side, its length along the ground.
		FVector Axis = Casing.Rotation.GetAxisZ();
		Axis -= Normal * FVector::DotProduct(Axis, Normal);
		if (!Axis.Normalize())
		{
			Axis = BrassPerpendicular(Normal);
		}
		Casing.Rotation = FRotationMatrix::MakeFromZX(Axis, Normal).ToQuat();
		return true;
	}
	if (!bFloor)
	{
		// Off a wall: pushed clear of it so the next leg doesn't start in it.
		Bounce += Normal * 20.f;
	}
	Casing.Spin = Casing.Spin * 0.55f + Random.GetUnitVector() * Random.FRandRange(4.f, 12.f);
	StartLeg(Casing, Casing.Location, Bounce);
	return true;
}

void FWeaponBrass::PlayLanding(const FCasing& Casing, const FVector& Where)
{
	UWorld* World = WorldRef.Get();
	if (!World)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	if (Now - LastSoundTime < BrassSoundGap || (!Camera.IsZero() && FVector::DistSquared(Camera, Where) > FMath::Square(BrassSoundRange)))
	{
		return;
	}
	LastSoundTime = Now;
	// Each bounce quieter than the last; a six-gun's fat case a little lower than a rifle's.
	const float Volume = Casing.Contacts <= 1 ? 1.f : (Casing.Contacts == 2 ? 0.55f : 0.3f);
	const float Pitch = Casing.Kind == ECasingKind::Pistol ? 0.92f : (Casing.Kind == ECasingKind::Hull && IsSoft(Casing.LandSurface) ? 0.8f : 1.f);
	LooterSound::PlayAt(World, CueFor(Casing.Kind, Casing.LandSurface), Where, Volume, Pitch);
}

void FWeaponBrass::Tick(float DeltaSeconds, const FVector& CameraLocation)
{
	Camera = CameraLocation;
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	for (int32 Index = Casings.Num() - 1; Index >= 0; --Index)
	{
		FCasing& Casing = Casings[Index];
		if (Casing.bResting)
		{
			// Lying still: nothing to redraw until it starts to sink away.
			Casing.RestAge += Dt;
			if (Casing.RestAge >= RestSeconds + FadeSeconds)
			{
				Casings.RemoveAt(Index, 1, EAllowShrinking::No);
				bDirty = true;
			}
			else if (Casing.RestAge >= RestSeconds)
			{
				bDirty = true;
			}
			continue;
		}
		Casing.LegTime += Dt;
		bDirty = true;
		if (Casing.LegLanding >= 0.f && Casing.LegTime >= Casing.LegLanding)
		{
			if (!Land(Casing))
			{
				Casings.RemoveAt(Index, 1, EAllowShrinking::No);
			}
			continue;
		}
		if (Casing.LegLanding < 0.f && Casing.LegTime >= Casing.LegHorizon)
		{
			// Fell away over an edge, out of sight.
			Casings.RemoveAt(Index, 1, EAllowShrinking::No);
			continue;
		}
		Casing.Location = BrassArc(Casing.LegStart, Casing.LegVelocity, Casing.LegTime);
		const float Turn = static_cast<float>(Casing.Spin.Size()) * Dt;
		if (Turn > 0.f)
		{
			Casing.Rotation = (FQuat(Casing.Spin.GetSafeNormal(), Turn) * Casing.Rotation).GetNormalized();
		}
	}
	if (bDirty || (bDrawnAnything && Casings.IsEmpty()))
	{
		Redraw();
	}
}

void FWeaponBrass::Redraw()
{
	bDirty = false;
	TArray<FTransform> BrassTransforms;
	TArray<FTransform> HullTransforms;
	BrassTransforms.Reserve(Casings.Num() + 8);
	for (const FCasing& Casing : Casings)
	{
		// Sinking away at the end: shrinking into the ground it lies on (the rest pose's X is that ground's normal).
		const float Fade = Casing.bResting ? FMath::Clamp((Casing.RestAge - RestSeconds) / FadeSeconds, 0.f, 1.f) : 0.f;
		const float Shrink = 1.f - 0.8f * Fade;
		const FVector2D Size = SizeOf(Casing.Kind);
		const FVector Axis = Casing.Rotation.GetAxisZ();
		const FVector Center = Casing.Location - Casing.Rotation.GetAxisX() * (Size.Y * 0.5 * Fade);
		// The engine cylinder is 100 across and 100 long along its Z, about its middle.
		if (Casing.Kind == ECasingKind::Hull)
		{
			const double Body = Size.X - BrassHullHead;
			BrassTransforms.Add(FTransform(Casing.Rotation, Center + Axis * (-Size.X * 0.5 + BrassHullHead * 0.5),
				FVector(BrassHullHeadWidth, BrassHullHeadWidth, BrassHullHead) * (Shrink / 100.f)));
			HullTransforms.Add(FTransform(Casing.Rotation, Center + Axis * (BrassHullHead * 0.5),
				FVector(Size.Y, Size.Y, Body) * (Shrink / 100.f)));
		}
		else
		{
			BrassTransforms.Add(FTransform(Casing.Rotation, Center, FVector(Size.Y, Size.Y, Size.X) * (Shrink / 100.f)));
		}
	}
	BrassDraw(BrassInstances.Get(), BrassTransforms);
	BrassDraw(HullInstances.Get(), HullTransforms);
	bDrawnAnything = !Casings.IsEmpty();
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

int32 FWeaponBrass::NumFlying() const
{
	int32 Count = 0;
	for (const FCasing& Casing : Casings)
	{
		Count += Casing.bResting ? 0 : 1;
	}
	return Count;
}

int32 FWeaponBrass::NumOfKind(ECasingKind Kind) const
{
	int32 Count = 0;
	for (const FCasing& Casing : Casings)
	{
		Count += Casing.Kind == Kind ? 1 : 0;
	}
	return Count;
}

TArray<FVector> FWeaponBrass::GetLocations() const
{
	TArray<FVector> Locations;
	Locations.Reserve(Casings.Num());
	for (const FCasing& Casing : Casings)
	{
		Locations.Add(Casing.Location);
	}
	return Locations;
}

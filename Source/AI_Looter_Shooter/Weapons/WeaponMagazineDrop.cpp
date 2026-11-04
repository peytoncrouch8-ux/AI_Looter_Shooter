#include "Weapons/WeaponMagazineDrop.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Weapons/ReloadTuning.h"
#include "Weapons/WeaponBrass.h"

// The thrown magazines (WeaponMagazineDrop.h): the pool, its drawers, the flight, landing, rest and sink. The rules and the
// launch a gun gives one are WeaponMagazineDropRules.cpp.

namespace
{
	/** Past this a lying magazine isn't drawn (cm): it's a few pixels. */
	constexpr int32 DropCullStart = 3500;
	constexpr int32 DropCullEnd = 5000;
	/** A bounce slower than this (cm/s), or leaving the face slower than this, lies down instead. */
	constexpr float DropSpentSpeed = 70.f;
	constexpr float DropSpentAway = 30.f;
	/** Landing sounds: the first a thump (Loot.Land, lighter than a gun's) under the rounds' rattle; the bounce the rattle only. */
	constexpr float DropThumpVolume = 0.8f;
	constexpr float DropThumpPitch = 1.15f;
	constexpr float DropRattlePitch = 0.62f;
	constexpr float DropRattleVolume = 0.6f;
	constexpr float DropBounceVolume = 0.35f;
	/** Shrinks to this much of itself as it sinks away. */
	constexpr float DropSunkScale = 0.2f;

	/** Replaces a drawer's instances with these (moved in place when the count is unchanged), as the brass draws. */
	void DropDraw(UInstancedStaticMeshComponent* Instances, const TArray<FTransform>& Transforms)
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
			Instances->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ true, /*bTeleport*/ true);
		}
	}

	/** A gun's painted material as it is now, its own copy (a gun's live gauge, the SMG helix's rounds, mustn't light the thrown one). */
	UMaterialInterface* DropPaint(UMaterialInterface* Source, UObject* Outer)
	{
		UMaterialInstanceDynamic* Live = Cast<UMaterialInstanceDynamic>(Source);
		if (!Live || !Live->Parent)
		{
			return Source;
		}
		UMaterialInstanceDynamic* Copy = UMaterialInstanceDynamic::Create(Live->Parent, Outer);
		if (!Copy)
		{
			return Source;
		}
		Copy->CopyParameterOverrides(Live);
		return Copy;
	}
}

// ---------------------------------------------------------------------------
// The pool
// ---------------------------------------------------------------------------

UWeaponMagazineDrop* UWeaponMagazineDrop::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UWeaponMagazineDrop>() : nullptr;
}

bool UWeaponMagazineDrop::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Played worlds, and the editor preview worlds the automated tests build.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UWeaponMagazineDrop::Deinitialize()
{
	Pool.Reset();
	Drawers.Reset();
	DrawerKeys.Reset();
	Holder = nullptr;
	bDrawnAnything = false;
	Super::Deinitialize();
}

void UWeaponMagazineDrop::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Advance(DeltaTime);
}

bool UWeaponMagazineDrop::IsTickable() const
{
	// Asleep with nothing out (and nothing left drawn to clear).
	return bDrawnAnything || Num() > 0;
}

TStatId UWeaponMagazineDrop::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UWeaponMagazineDrop, STATGROUP_Tickables);
}

int32 UWeaponMagazineDrop::Num() const
{
	int32 Count = 0;
	for (const FThrown& Thrown : Pool)
	{
		Count += Thrown.bOut ? 1 : 0;
	}
	return Count;
}

int32 UWeaponMagazineDrop::NumFlying() const
{
	int32 Count = 0;
	for (const FThrown& Thrown : Pool)
	{
		Count += Thrown.bOut && !Thrown.bResting ? 1 : 0;
	}
	return Count;
}

TArray<FVector> UWeaponMagazineDrop::GetLocations() const
{
	TArray<FVector> Locations;
	for (const FThrown& Thrown : Pool)
	{
		if (Thrown.bOut)
		{
			Locations.Add(Thrown.Location);
		}
	}
	return Locations;
}

AActor* UWeaponMagazineDrop::GetHolder()
{
	// Drawers whose actor went with the level are made again (and whatever they drew went with them).
	if (IsValid(Holder))
	{
		return Holder;
	}
	Holder = nullptr;
	Drawers.Reset();
	DrawerKeys.Reset();
	for (FThrown& Thrown : Pool)
	{
		Thrown.bOut = false;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Holder = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
	if (!Holder)
	{
		return nullptr;
	}
#if WITH_EDITOR
	Holder->SetActorLabel(TEXT("MagazineDrops"));
#endif
	USceneComponent* Root = NewObject<USceneComponent>(Holder, TEXT("Root"), RF_Transient);
	Root->SetMobility(EComponentMobility::Movable);
	Holder->SetRootComponent(Root);
	Root->RegisterComponent();
	return Holder;
}

int32 UWeaponMagazineDrop::DrawerFor(const FMagazineDropLaunch& Launch)
{
	AActor* Owner = GetHolder();
	if (!Owner)
	{
		return INDEX_NONE;
	}
	auto Matches = [&Launch](const FDrawerKey& Key)
	{
		if (Key.Mesh.Get() != Launch.Mesh || Key.Materials.Num() != Launch.Materials.Num() || Key.CustomData != Launch.CustomData)
		{
			return false;
		}
		for (int32 Index = 0; Index < Key.Materials.Num(); ++Index)
		{
			if (Key.Materials[Index].Get() != Launch.Materials[Index])
			{
				return false;
			}
		}
		return true;
	};
	for (int32 Drawer = 0; Drawer < DrawerKeys.Num(); ++Drawer)
	{
		if (Matches(DrawerKeys[Drawer]) && IsValid(Drawers[Drawer]))
		{
			return Drawer;
		}
	}
	// An empty drawer is taken over before another is made (at most one a thrown magazine, so never past the pool).
	int32 Taken = INDEX_NONE;
	for (int32 Drawer = 0; Drawer < Drawers.Num() && Taken == INDEX_NONE; ++Drawer)
	{
		const bool bInUse = Pool.ContainsByPredicate([Drawer](const FThrown& Thrown) { return Thrown.bOut && Thrown.Drawer == Drawer; });
		Taken = !bInUse && IsValid(Drawers[Drawer]) ? Drawer : INDEX_NONE;
	}
	if (Taken == INDEX_NONE)
	{
		if (Drawers.Num() >= MagazineDrop::PoolSize)
		{
			return INDEX_NONE;
		}
		UInstancedStaticMeshComponent* Instances = NewObject<UInstancedStaticMeshComponent>(Owner, NAME_None, RF_Transient);
		Instances->SetupAttachment(Owner->GetRootComponent());
		Instances->SetMobility(EComponentMobility::Movable);
		Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Instances->SetGenerateOverlapEvents(false);
		Instances->SetCanEverAffectNavigation(false);
		// A hand-sized piece lying in the grass for a few seconds: its shadow isn't worth a shadow draw (the brass's call too).
		Instances->SetCastShadow(false);
		Instances->bReceivesDecals = false;
		Instances->SetCullDistances(DropCullStart, DropCullEnd);
		Instances->RegisterComponent();
		Taken = Drawers.Add(Instances);
		DrawerKeys.AddDefaulted();
	}
	// This mesh in this gun's paint (copied, so the gun's own gauges and later repaints don't reach it).
	UInstancedStaticMeshComponent* Instances = Drawers[Taken];
	Instances->ClearInstances();
	Instances->SetStaticMesh(Launch.Mesh);
	FDrawerKey& Key = DrawerKeys[Taken];
	Key.Mesh = Launch.Mesh;
	Key.Materials.Reset();
	Key.CustomData = Launch.CustomData;
	for (int32 Index = 0; Index < Launch.Materials.Num(); ++Index)
	{
		Key.Materials.Add(Launch.Materials[Index]);
		Instances->SetMaterial(Index, DropPaint(Launch.Materials[Index], Owner));
	}
	for (int32 Index = 0; Index < Launch.CustomData.Num(); ++Index)
	{
		Instances->SetCustomPrimitiveDataFloat(Index, Launch.CustomData[Index]);
	}
	return Taken;
}

void UWeaponMagazineDrop::Drop(const FMagazineDropLaunch& Launch)
{
	if (!Launch.Mesh)
	{
		return;
	}
	if (Pool.Num() < MagazineDrop::PoolSize)
	{
		Pool.SetNum(MagazineDrop::PoolSize);
	}
	// A free slot, else the earliest throw goes (usually lying far behind by now): measured in throws ago, not seconds, so two
	// thrown in one frame still go in order (a test's ten in a row kept the second, not the ninth, by age).
	TArray<float, TInlineAllocator<MagazineDrop::PoolSize>> Ages;
	for (const FThrown& Each : Pool)
	{
		Ages.Add(Each.bOut ? static_cast<float>(Throws - Each.Serial) : -1.f);
	}
	const int32 Slot = MagazineDrop::PickSlot(Ages);
	if (!Pool.IsValidIndex(Slot))
	{
		return;
	}
	Pool[Slot] = FThrown();
	Pool[Slot].Serial = ++Throws;
	const int32 Drawer = DrawerFor(Launch);
	if (Drawer == INDEX_NONE)
	{
		bDirty = true;
		return;
	}
	FThrown& Thrown = Pool[Slot];
	Thrown.bOut = true;
	Thrown.Drawer = Drawer;
	// Flown and landed by its middle (a part's origin is its socket, at one end).
	const FBox Bounds = Launch.Mesh->GetBoundingBox();
	Thrown.MeshMiddle = Bounds.GetCenter();
	Thrown.DrawnScale = Launch.Transform.GetScale3D();
	Thrown.WorldScale = Launch.WorldScale;
	Thrown.Extent = (Bounds.GetExtent() * Launch.WorldScale).GetAbs();
	Thrown.Rotation = Launch.Transform.GetRotation();
	Thrown.Location = Launch.Transform.TransformPosition(Thrown.MeshMiddle);
	Thrown.Spin = Launch.Spin;
	Thrown.Channel = Launch.Channel;
	Thrown.Holder = Launch.Holder;
	Thrown.Gun = Launch.Gun;
	StartLeg(Thrown, Thrown.Location, Launch.Velocity);
	bDirty = true;
}

void UWeaponMagazineDrop::StartLeg(FThrown& Thrown, const FVector& From, const FVector& Velocity)
{
	Thrown.LegStart = From;
	Thrown.LegVelocity = Velocity;
	Thrown.LegTime = 0.f;
	Thrown.LegLanding = -1.f;
	// Until it has fallen MaxFall below where it starts: the arc is traced that far ahead and no farther.
	const float Rise = static_cast<float>(Velocity.Z);
	Thrown.LegHorizon = FMath::Clamp((Rise + FMath::Sqrt(Rise * Rise + 2.f * MagazineDrop::Gravity * MagazineDrop::MaxFall)) / MagazineDrop::Gravity,
		0.05f, MagazineDrop::MaxLegSeconds);
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// The arc ahead in a few chords (three for the throw, two for a bounce): the first face met is where it lands, and
	// names the surface its sound is picked by (complex, as the brass).
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MagazineDropTrace), /*bTraceComplex*/ true);
	Params.bReturnFaceIndex = true;
	Params.bReturnPhysicalMaterial = true;
	if (const AActor* Thrower = Thrown.Holder.Get())
	{
		Params.AddIgnoredActor(Thrower);
	}
	if (const AActor* Gun = Thrown.Gun.Get())
	{
		Params.AddIgnoredActor(Gun);
	}
	const int32 Chords = Thrown.Contacts == 0 ? 3 : 2;
	FVector ChordStart = From;
	float ChordStartTime = 0.f;
	for (int32 Chord = 1; Chord <= Chords; ++Chord)
	{
		const float ChordEndTime = Thrown.LegHorizon * static_cast<float>(Chord) / static_cast<float>(Chords);
		const FVector ChordEnd = MagazineDrop::Arc(From, Velocity, ChordEndTime);
		FHitResult Hit;
		// Started inside something (a gun pressed to a wall): that chord is skipped, not landed on.
		if (World->LineTraceSingleByChannel(Hit, ChordStart, ChordEnd, Thrown.Channel, Params) && !Hit.bStartPenetrating)
		{
			Thrown.LegLanding = FMath::Lerp(ChordStartTime, ChordEndTime, Hit.Time);
			Thrown.LandPoint = Hit.ImpactPoint;
			Thrown.LandNormal = Hit.ImpactNormal.IsNearlyZero() ? FVector::UpVector : FVector(Hit.ImpactNormal);
			Thrown.LandSurface = ImpactSurface::Of(Hit);
			return;
		}
		ChordStart = ChordEnd;
		ChordStartTime = ChordEndTime;
	}
}

bool UWeaponMagazineDrop::Land(FThrown& Thrown)
{
	const FVector Normal = Thrown.LandNormal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	const FVector Velocity = Thrown.LegVelocity + FVector(0.0, 0.0, -MagazineDrop::Gravity * Thrown.LegLanding);
	++Thrown.Contacts;
	const bool bWater = Thrown.LandSurface == EImpactMaterial::Water;
	const bool bSoft = FWeaponBrass::IsSoft(Thrown.LandSurface);
	// The first landing thumps under the rounds' rattle; a bounce only rattles. Into water: a plip, and it's gone.
	const FName Rattle = FWeaponBrass::CueFor(ECasingKind::Rifle, Thrown.LandSurface);
	if (Thrown.Contacts == 1 && !bWater)
	{
		LooterSound::PlayAt(GetWorld(), LooterSoundCue::LootLand, Thrown.LandPoint, DropThumpVolume, DropThumpPitch);
	}
	LooterSound::PlayAt(GetWorld(), Rattle, Thrown.LandPoint, Thrown.Contacts == 1 ? DropRattleVolume : DropBounceVolume, DropRattlePitch);
	if (bWater)
	{
		return false;
	}
	// It lies on floor-like ground once it has nothing left to bounce with; a wall only turns it back down (a few contacts
	// more at most, so it can't rattle in a crack forever).
	FVector Bounce = MagazineDrop::Bounce(Velocity, Normal, bSoft) + Random.GetUnitVector() * (bSoft ? 5.f : 15.f);
	const bool bFloor = Normal.Z >= 0.5;
	const bool bSpent = Thrown.Contacts > MagazineDrop::MaxBounces || Bounce.Size() < DropSpentSpeed
		|| FVector::DotProduct(Bounce, Normal) < DropSpentAway;
	if ((bFloor && bSpent) || Thrown.Contacts > MagazineDrop::MaxBounces + 3)
	{
		Thrown.bResting = true;
		Thrown.RestAge = 0.f;
		Thrown.Spin = FVector::ZeroVector;
		Thrown.Rotation = MagazineDrop::RestRotation(Thrown.Rotation, Normal, Thrown.Extent);
		Thrown.Location = Thrown.LandPoint + Normal * MagazineDrop::RestLift(Thrown.Extent);
		return true;
	}
	if (!bFloor)
	{
		// Off a wall: pushed clear so the next leg doesn't start in it.
		Bounce += Normal * 20.f;
	}
	Thrown.Spin = Thrown.Spin * 0.5f + Random.GetUnitVector() * Random.FRandRange(2.f, 6.f);
	// From where its middle is when it touches (its half size off the face), not from inside the ground.
	StartLeg(Thrown, Thrown.LandPoint + Normal * MagazineDrop::RestLift(Thrown.Extent), Bounce);
	return true;
}

void UWeaponMagazineDrop::Advance(float DeltaSeconds)
{
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	const float Rest = ReloadTuning::DropMagRest();
	for (FThrown& Thrown : Pool)
	{
		if (!Thrown.bOut)
		{
			continue;
		}
		Thrown.Age += Dt;
		if (Thrown.bResting)
		{
			// Lying still: nothing to redraw until it starts to sink away.
			Thrown.RestAge += Dt;
			if (Thrown.RestAge >= Rest + MagazineDrop::SinkSeconds)
			{
				Thrown.bOut = false;
				bDirty = true;
			}
			else if (Thrown.RestAge >= Rest)
			{
				bDirty = true;
			}
			continue;
		}
		Thrown.LegTime += Dt;
		bDirty = true;
		if (Thrown.LegLanding >= 0.f && Thrown.LegTime >= Thrown.LegLanding)
		{
			Thrown.bOut = Land(Thrown);
			continue;
		}
		if (Thrown.LegLanding < 0.f && Thrown.LegTime >= Thrown.LegHorizon)
		{
			// Fell away over an edge, out of sight.
			Thrown.bOut = false;
			continue;
		}
		Thrown.Location = MagazineDrop::Arc(Thrown.LegStart, Thrown.LegVelocity, Thrown.LegTime);
		const float Turn = static_cast<float>(Thrown.Spin.Size()) * Dt;
		if (Turn > 0.f)
		{
			Thrown.Rotation = (FQuat(Thrown.Spin.GetSafeNormal(), Turn) * Thrown.Rotation).GetNormalized();
		}
	}
	if (bDirty || (bDrawnAnything && Num() == 0))
	{
		Redraw();
	}
}

void UWeaponMagazineDrop::Redraw()
{
	bDirty = false;
	const float Rest = ReloadTuning::DropMagRest();
	TArray<TArray<FTransform>> PerDrawer;
	PerDrawer.SetNum(Drawers.Num());
	for (const FThrown& Thrown : Pool)
	{
		if (!Thrown.bOut || !PerDrawer.IsValidIndex(Thrown.Drawer))
		{
			continue;
		}
		// Its first-person size grows (or shrinks) into its own as it leaves the view; lying, it sinks into the ground at
		// the end, shrinking.
		const float Grown = FMath::SmoothStep(0.f, 1.f, Thrown.Age / MagazineDrop::GrowSeconds);
		const float Sunk = Thrown.bResting ? MagazineDrop::SinkShare(Thrown.RestAge, Rest) : 0.f;
		const FVector Scale = FMath::Lerp(Thrown.DrawnScale, Thrown.WorldScale, Grown) * (1.f - (1.f - DropSunkScale) * Sunk);
		const FVector Middle = Thrown.Location - Thrown.LandNormal * (Thrown.bResting ? MagazineDrop::RestLift(Thrown.Extent) * Sunk : 0.f);
		PerDrawer[Thrown.Drawer].Add(FTransform(Thrown.Rotation, Middle - Thrown.Rotation.RotateVector(Thrown.MeshMiddle * Scale), Scale));
	}
	for (int32 Drawer = 0; Drawer < Drawers.Num(); ++Drawer)
	{
		DropDraw(Drawers[Drawer], PerDrawer[Drawer]);
	}
	bDrawnAnything = Num() > 0;
}

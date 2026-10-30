// UCreatureTierFXSubsystem: the shells' shared looks, starting a top tier's death show (its sound, its burst, the ground
// its ring runs over, its motes and glints), and the pillars' pool. CreatureTierFXSubsystemDraw.cpp plays the show frame
// by frame.

#include "Creatures/CreatureTierFXSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/CreatureBase.h"
#include "Loot/LootDropComponent.h"
#include "Weapons/WeaponFX.h"
#include "World/LightBeam.h"
#include "World/WorldQueries.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"

const TCHAR* const UCreatureTierFXSubsystem::ShellMaterialPath = TEXT("/Game/Art/Materials/Masters/M_CreatureTier.M_CreatureTier");

namespace
{
	// M_CreatureTier's parameters (build_tier_material.py), set once per shared look.
	const FName TierShellColorParam(TEXT("Color"));
	const FName TierShellGlowParam(TEXT("Glow"));
	const FName TierShellEyeOnParam(TEXT("EyeOn"));
	const FName TierShellEyeMinParam(TEXT("EyeMin"));
	const FName TierShellEyeMaxParam(TEXT("EyeMax"));
	const FName TierShellCoreOnParam(TEXT("CoreOn"));
	const FName TierShellCoreCenterParam(TEXT("CoreCenter"));
	const FName TierShellCoreRadiusParam(TEXT("CoreRadius"));
	const FName TierShellInsideOnlyParam(TEXT("InsideOnly"));
	const FName TierShellRimOnParam(TEXT("RimOn"));
	const FName TierShellShroudFadeParam(TEXT("ShroudFade"));

	/** The ground is looked for this far over and under the feet (cm), and taken within these of them. */
	constexpr double TierGroundUp = 150.0;
	constexpr double TierGroundDown = 300.0;
	constexpr float TierMostRise = 140.f;
	constexpr float TierMostDrop = 140.f;

	/** The motes are born over the show's first part, round the pillar's foot. */
	constexpr float TierMoteBornWithin = 0.55f;

	FVector2D TierSpokeWay(int32 Spoke)
	{
		const float Angle = UE_TWO_PI * static_cast<float>(Spoke) / static_cast<float>(UCreatureTierFXSubsystem::RingSegments);
		return FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
	}
}

UCreatureTierFXSubsystem* UCreatureTierFXSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UCreatureTierFXSubsystem>() : nullptr;
}

bool UCreatureTierFXSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Played worlds, and the editor preview worlds the automated tests build.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UCreatureTierFXSubsystem::Deinitialize()
{
	Shows.Reset();
	Pillars.Reset();
	PillarHolder = nullptr;
	ShellLooks.Reset();
	Super::Deinitialize();
}

void UCreatureTierFXSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Advance(DeltaTime);
}

bool UCreatureTierFXSubsystem::IsTickable() const
{
	return !Shows.IsEmpty();
}

TStatId UCreatureTierFXSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCreatureTierFXSubsystem, STATGROUP_Tickables);
}

// ---------------------------------------------------------------------------
// The shells' looks
// ---------------------------------------------------------------------------

UMaterialInstanceDynamic* UCreatureTierFXSubsystem::GetShellLook(const FCreatureTierShell& Shell, ECreatureRank Rank)
{
	if (!Shell.bWorn || Shell.Key.IsNone())
	{
		return nullptr;
	}
	if (const TObjectPtr<UMaterialInstanceDynamic>* Found = ShellLooks.Find(Shell.Key))
	{
		if (*Found)
		{
			return *Found;
		}
	}
	if (!bShellMaterialLooked)
	{
		// Looked for once a level: a checkout without it (before build_tier_material.py has run) shows no shells.
		bShellMaterialLooked = true;
		if (FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ShellMaterialPath))))
		{
			ShellMaterial = LoadObject<UMaterialInterface>(nullptr, ShellMaterialPath);
		}
		if (!ShellMaterial)
		{
			UE_LOG(LogLooter, Warning, TEXT("Creature tiers: %s is missing (Tools/Unreal/build_tier_material.py makes it): no tier glow ")
				TEXT("on eyes or cores, no ghost-rim."), ShellMaterialPath);
		}
	}
	if (!ShellMaterial)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Look = UMaterialInstanceDynamic::Create(ShellMaterial, this);
	// A Touched creature's shell burns in its element's colour (its key names the element, so the look is shared only by
	// creatures Touched alike).
	Look->SetVectorParameterValue(TierShellColorParam, CreatureTierLook::ShellColor(Shell, Rank));
	Look->SetScalarParameterValue(TierShellGlowParam, Shell.Glow);
	Look->SetScalarParameterValue(TierShellEyeOnParam, Shell.bEyes ? 1.f : 0.f);
	Look->SetVectorParameterValue(TierShellEyeMinParam, FLinearColor(Shell.EyeMin.X, Shell.EyeMin.Y, 0.f, 0.f));
	Look->SetVectorParameterValue(TierShellEyeMaxParam, FLinearColor(Shell.EyeMax.X, Shell.EyeMax.Y, 0.f, 0.f));
	Look->SetScalarParameterValue(TierShellCoreOnParam, Shell.bCore ? 1.f : 0.f);
	Look->SetVectorParameterValue(TierShellCoreCenterParam, FLinearColor(Shell.CoreCenter.X, Shell.CoreCenter.Y, Shell.CoreCenter.Z, 0.f));
	Look->SetScalarParameterValue(TierShellCoreRadiusParam, Shell.CoreRadius);
	Look->SetScalarParameterValue(TierShellInsideOnlyParam, Shell.bInsideOnly ? 1.f : 0.f);
	Look->SetScalarParameterValue(TierShellRimOnParam, Shell.bRim ? 1.f : 0.f);
	Look->SetScalarParameterValue(TierShellShroudFadeParam, Shell.bShroudFade ? 1.f : 0.f);
	ShellLooks.Add(Shell.Key, Look);
	return Look;
}

// ---------------------------------------------------------------------------
// Starting a death's show
// ---------------------------------------------------------------------------

bool UCreatureTierFXSubsystem::PlayDeath(const ACreatureBase& Creature)
{
	// From the middle of the body as drawn (as its own burst is), the pillar from the ground under it, the pop where its
	// drops fly out (ULootLibrary::SpawnKillLoot throws them from 60 cm over the actor).
	const USkeletalMeshComponent* Body = Creature.GetMesh();
	const FVector Center = Body ? FVector(Body->Bounds.Origin) : Creature.GetActorLocation();
	float Radius = 0.f;
	float HalfHeight = 0.f;
	Creature.GetSimpleCollisionCylinder(Radius, HalfHeight);
	const FVector Feet = Creature.GetActorLocation() - FVector(0.0, 0.0, HalfHeight);
	const ULootDropComponent* Loot = Creature.FindComponentByClass<ULootDropComponent>();
	const bool bLootPop = Loot && Loot->bDropOnDeath;
	return PlayDeathAt(Creature.GetRank(), Center, Feet, 100.f * Creature.GetSizeScale(), Creature.GetActorLocation() + FVector(0.0, 0.0, 60.0),
		bLootPop);
}

bool UCreatureTierFXSubsystem::PlayDeathAt(ECreatureRank Rank, const FVector& Center, const FVector& Feet, float Size, const FVector& LootSpot,
	bool bLootPop)
{
	UWorld* World = GetWorld();
	const FCreatureTierDeathShow Look = CreatureTierLook::DeathShowFor(Rank);
	if (!World || !Look.bValid)
	{
		return false;
	}
	// A crowd of them dying together: the oldest show gives way (it's nearly over anyway).
	if (Shows.Num() >= MaxShows)
	{
		HidePillars(Shows[0].Slot);
		Shows.RemoveAt(0);
	}
	const int32 Slot = FreeSlot();
	FTierShow& Show = Shows.AddDefaulted_GetRef();
	Show.Rank = Rank;
	Show.Look = Look;
	Show.Color = CreatureTierLook::GlowColor(Rank);
	Show.Center = Center;
	Show.Feet = Feet;
	Show.LootSpot = LootSpot;
	Show.Size = FMath::Max(Size, 20.f);
	Show.bLootPop = bLootPop;
	Show.Slot = Slot;
	FindGround(Show);
	SeedSparks(Show);

	// At once: the death's sound at the body, and soul-light bursting out of it in its colour, bigger than the body's own.
	LooterSound::PlayAt(this, CreatureTierLook::DeathCue(Rank), Center);
	if (UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>())
	{
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		Effects.SpawnDeathBurst(EDeathBurst::SoulLight, Center, FVector::ZeroVector, Show.Size * Look.BurstScale, Show.Color);
	}
	DrawPillars(Show);
	UE_LOG(LogLooter, Verbose, TEXT("Creature tiers: %s death show at %s."), *UEnum::GetValueAsString(Rank), *Center.ToCompactString());
	return true;
}

void UCreatureTierFXSubsystem::FindGround(FTierShow& Show) const
{
	Show.EdgeHeight.Init(0.f, RingSegments);
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// The terrain and what's built on it, never the level's volumes (a corpse's own collision is off already).
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, FName(TEXT("TierDeathGround")), nullptr, /*bTraceComplex*/ false);
	const FCollisionObjectQueryParams Objects(ECC_WorldStatic);
	auto HeightAt = [&](const FVector& Over, float Fallback)
	{
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, Over + FVector(0.0, 0.0, TierGroundUp), Over - FVector(0.0, 0.0, TierGroundDown), Objects, Params))
		{
			return FMath::Clamp(static_cast<float>(Hit.ImpactPoint.Z - Show.Feet.Z), -TierMostDrop, TierMostRise);
		}
		return Fallback;
	};
	// The ground under the body first (a floating Unpaid's feet are over it), then at the ring's end along each spoke.
	Show.Feet.Z += HeightAt(Show.Feet, 0.f);
	for (int32 Spoke = 0; Spoke < RingSegments; ++Spoke)
	{
		const FVector2D Way = TierSpokeWay(Spoke);
		Show.EdgeHeight[Spoke] = HeightAt(Show.Feet + FVector(Way.X, Way.Y, 0.0) * Show.Look.RingRadius, 0.f);
	}
}

void UCreatureTierFXSubsystem::SeedSparks(FTierShow& Show)
{
	// Motes: born at the pillar's foot over the show's first half second, each launched as it's born (DrawSparks).
	Show.Motes.SetNum(Show.Look.Motes);
	for (int32 Index = 0; Index < Show.Motes.Num(); ++Index)
	{
		FTierSpark& Mote = Show.Motes[Index];
		Mote = FTierSpark();
		Mote.Born = TierMoteBornWithin * static_cast<float>(Index) / FMath::Max(Show.Motes.Num() - 1, 1) + Random.FRandRange(0.f, 0.05f);
		Mote.Life = Random.FRandRange(0.9f, 1.5f);
		Mote.Phase = Random.FRandRange(0.f, UE_TWO_PI);
		Mote.Width = Random.FRandRange(1.8f, 3.f);
		Mote.Glow = Random.FRandRange(0.7f, 1.f);
		Mote.bHot = Index % 4 == 0;
		Mote.Location = Show.Feet;
	}
	// Glints: the loot's pop throws them all at once from where the drops fly out.
	Show.Glints.SetNum(Show.bLootPop ? Show.Look.Glints : 0);
	for (int32 Index = 0; Index < Show.Glints.Num(); ++Index)
	{
		FTierSpark& Glint = Show.Glints[Index];
		Glint = FTierSpark();
		Glint.Born = Show.Look.PopDelay + Random.FRandRange(0.f, 0.05f);
		Glint.Life = Random.FRandRange(0.5f, 0.9f);
		Glint.Phase = Random.FRandRange(0.f, UE_TWO_PI);
		Glint.Width = Random.FRandRange(1.6f, 2.4f);
		Glint.Glow = Random.FRandRange(0.8f, 1.f);
		Glint.bHot = Index % 3 == 0;
		Glint.Location = Show.LootSpot;
	}
}

// ---------------------------------------------------------------------------
// The pillars' pool
// ---------------------------------------------------------------------------

UStaticMeshComponent* UCreatureTierFXSubsystem::PillarFor(int32 Slot, bool bCore)
{
	UWorld* World = GetWorld();
	if (!World || Slot < 0 || Slot >= MaxShows)
	{
		return nullptr;
	}
	if (!PillarHolder)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		PillarHolder = World->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
		if (!PillarHolder)
		{
			return nullptr;
		}
		USceneComponent* Root = NewObject<USceneComponent>(PillarHolder, TEXT("Root"));
		PillarHolder->SetRootComponent(Root);
		Root->RegisterComponent();
		Pillars.SetNum(MaxShows * 2);
	}
	const int32 Index = Slot * 2 + (bCore ? 1 : 0);
	if (!Pillars.IsValidIndex(Index))
	{
		return nullptr;
	}
	if (!Pillars[Index])
	{
		// The loot beams' glow on the engine's cylinder (no collision, no shadow), its own instance for its own colour.
		UStaticMeshComponent* Beam = NewObject<UStaticMeshComponent>(PillarHolder);
		Beam->SetMobility(EComponentMobility::Movable);
		Beam->SetupAttachment(PillarHolder->GetRootComponent());
		Beam->RegisterComponent();
		LightBeams::Setup(Beam, LightBeams::CreateMaterial(Beam, FLinearColor::White, 1.f, 100.f), 100.f, 10.f);
		Beam->SetVisibility(false);
		Pillars[Index] = Beam;
	}
	return Pillars[Index];
}

void UCreatureTierFXSubsystem::HidePillars(int32 Slot)
{
	for (const bool bCore : { false, true })
	{
		const int32 Index = Slot * 2 + (bCore ? 1 : 0);
		if (Pillars.IsValidIndex(Index) && Pillars[Index])
		{
			Pillars[Index]->SetVisibility(false);
		}
	}
}

int32 UCreatureTierFXSubsystem::FreeSlot() const
{
	for (int32 Slot = 0; Slot < MaxShows; ++Slot)
	{
		const bool bTaken = Shows.ContainsByPredicate([Slot](const FTierShow& Show) { return Show.Slot == Slot; });
		if (!bTaken)
		{
			return Slot;
		}
	}
	return 0;
}

int32 UCreatureTierFXSubsystem::NumPillarsLit() const
{
	int32 Lit = 0;
	for (const TObjectPtr<UStaticMeshComponent>& Beam : Pillars)
	{
		Lit += Beam && Beam->IsVisible() ? 1 : 0;
	}
	return Lit;
}

FVector UCreatureTierFXSubsystem::CameraLocation() const
{
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	if (Controller && Controller->PlayerCameraManager)
	{
		return Controller->PlayerCameraManager->GetCameraLocation();
	}
	// No camera (a test level): far overhead, so nothing is dimmed as near the eye.
	return Shows.IsEmpty() ? FVector(0.0, 0.0, 1.0e5) : Shows[0].Center + FVector(0.0, 0.0, 1.0e5);
}

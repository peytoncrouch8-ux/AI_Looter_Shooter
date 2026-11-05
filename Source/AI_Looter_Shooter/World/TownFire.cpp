// ATownFire: a fire in a town (the Company's barrel fire at the gate, a forge, a boiler): a cheap watch-fire recipe the story
// and the light of the moment turn on, no interaction.

#include "World/TownFire.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "World/HouseLights.h"
#include "World/LightingStateSubsystem.h"
#include "World/StoryProp.h"
#include "World/WatchFire.h"
#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The gunfire's additive glow on the engine's plane (R, G, B, strength, shape: 0 a streak, 1 round), as the watch fire's. */
	const TCHAR* GlowPath = TEXT("/Game/Weapons/FX/M_FX_Glow.M_FX_Glow");
	const TCHAR* PlanePath = TEXT("/Engine/BasicShapes/Plane.Plane");
	/** Smoke.py's plume (8 m tall), the chimneys' and the train's. */
	const TCHAR* SmokePath = TEXT("/Game/Art/Props/SM_SmokePlume.SM_SmokePlume");
	constexpr int32 GlowFloats = 5;
	constexpr float StreakShape = 0.f;
	constexpr float RoundShape = 1.f;

	/** The tongues, crossed about the upright this many times; the halo's size and glow, a share of the flames'. */
	constexpr int32 Tongues = 4;
	constexpr float HaloSize = 2.4f;
	constexpr float HaloGlow = 0.1f;

	/** Far from the player, it looks again this often (s) whether they've come near. */
	constexpr float FarTickSeconds = 0.5f;

	/** The light sits this far up the flames (a share of their height). */
	constexpr float LightUp = 0.4f;

	/** An asset found only once it's in this checkout, so the class works, and its tests run, without it. In a constructor. */
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

ATownFire::ATownFire()
{
	// Ticks only while it burns (RefreshTick).
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	static UStaticMesh* const Plane = FindIfMade<UStaticMesh>(PlanePath);
	static UMaterialInterface* const Glow = FindIfMade<UMaterialInterface>(GlowPath);
	static UStaticMesh* const Plume = FindIfMade<UStaticMesh>(SmokePath);
	CardMesh = Plane;
	GlowMaterial = Glow;
	SmokeMesh = Plume;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	Flame = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Flame"));
	Flame->SetupAttachment(Root);
	Flame->SetMobility(EComponentMobility::Movable);
	Flame->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Flame->SetGenerateOverlapEvents(false);
	Flame->SetCanEverAffectNavigation(false);
	Flame->SetCastShadow(false);
	Flame->bReceivesDecals = false;
	Flame->SetNumCustomDataFloats(GlowFloats);
	Flame->SetVisibility(false);

	// The town's light rules (the towns plan 4.1): a 2400 K fire, never a shadow, no light function, no lighting of the
	// translucent things (the smoke plumes need no firelight), gone beyond 45 m, and hidden while the fire is out.
	FireLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireLight"));
	FireLight->SetupAttachment(Root);
	FireLight->SetMobility(EComponentMobility::Movable);
	FireLight->SetCastShadows(false);
	FireLight->SetIntensityUnits(ELightUnits::Candelas);
	FireLight->SetIntensity(LightCandela);
	FireLight->SetAttenuationRadius(LightRadius);
	FireLight->SourceRadius = 14.f;
	FireLight->bUseTemperature = true;
	FireLight->Temperature = LightTemperature;
	FireLight->bAffectTranslucentLighting = false;
	FireLight->MaxDrawDistance = AHouseLights::LampDrawDistance;
	FireLight->MaxDistanceFadeRange = AHouseLights::LampFadeRange;
	FireLight->SetVisibility(false);

	// A faint wisp leaning downwind, seen, never touched, never shadowed.
	Smoke = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Smoke"));
	Smoke->SetupAttachment(Root);
	Smoke->SetMobility(EComponentMobility::Movable);
	Smoke->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Smoke->SetGenerateOverlapEvents(false);
	Smoke->SetCanEverAffectNavigation(false);
	Smoke->SetCastShadow(false);
	Smoke->bReceivesDecals = false;
	Smoke->SetVisibility(false);
}

void ATownFire::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildFlame();
	ApplyFire();
}

void ATownFire::BeginPlay()
{
	Super::BeginPlay();
	BuildFlame();
	if (ShownWhen.Num() > 0 || HiddenWhen.Num() > 0)
	{
		if (UMissionRunner* Runner = UMissionRunner::Get(this))
		{
			BoundRunner = Runner;
			MissionsChangedHandle = Runner->OnChanged.AddUObject(this, &ATownFire::HandleMissionsChanged);
		}
	}
	// As the level begins, lit as the state it starts in and shown as the story stands: no catch for a fire already burning.
	ULightingStateSubsystem* Lighting = ULightingStateSubsystem::Get(this);
	if (Lighting)
	{
		LightingHandle = Lighting->OnChanged.AddUObject(this, &ATownFire::HandleLightingChanged);
	}
	KindNow = LightingStates::Kind(Lighting ? Lighting->GetState() : NAME_None);
	RefreshStory();
	bBegun = true;
}

void ATownFire::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(MissionsChangedHandle);
	}
	BoundRunner.Reset();
	MissionsChangedHandle.Reset();
	if (ULightingStateSubsystem* Lighting = ULightingStateSubsystem::Get(this))
	{
		Lighting->OnChanged.Remove(LightingHandle);
	}
	LightingHandle.Reset();
	LooterSound::Stop(BurnLoop.Get());
	BurnLoop.Reset();
	Super::EndPlay(EndPlayReason);
}

void ATownFire::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// The rules and the state
// ---------------------------------------------------------------------------

bool ATownFire::BurnsIn(ELightKind Kind) const
{
	switch (Kind)
	{
	case ELightKind::Gloom: return bBurnsInGloom;
	case ELightKind::Dusk: return bBurnsAtDusk;
	default: return bBurnsByDay;
	}
}

bool ATownFire::ShouldShow(const FCampaignRecord* Campaign, const UMissionRunner* Runner) const
{
	if (ShownWhen.Num() == 0 && HiddenWhen.Num() == 0)
	{
		return true;
	}
	return AStoryProp::Applies(ShownWhen, HiddenWhen, Campaign, Runner);
}

void ATownFire::RefreshStory()
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	SetShown(ShouldShow(Runner ? &Runner->GetCampaign() : nullptr, Runner));
}

void ATownFire::HandleMissionsChanged()
{
	RefreshStory();
}

void ATownFire::HandleLightingChanged(const FLightingStateChange& Change)
{
	ApplyState(Change.To);
}

void ATownFire::SetShown(bool bInShown)
{
	bShown = bInShown;
	ApplyFire();
}

void ATownFire::ApplyState(FName State)
{
	KindNow = LightingStates::Kind(State);
	ApplyFire();
}

void ATownFire::Advance(float DeltaSeconds)
{
	if (!bBurning)
	{
		RefreshTick();
		return;
	}
	BurnTime += DeltaSeconds;
	if (CatchAge >= 0.f)
	{
		CatchAge += DeltaSeconds;
		// Caught: Catch() is 1 from 0.6 s on.
		if (CatchAge >= 0.6f)
		{
			CatchAge = -1.f;
		}
	}
	ApplyFire();
}

float ATownFire::GetLightCandela() const
{
	return bBurning && FireLight ? FireLight->Intensity : 0.f;
}

float ATownFire::GetFlameScale() const
{
	return bBurning && Flame ? static_cast<float>(Flame->GetRelativeScale3D().Z) : 0.f;
}

// ---------------------------------------------------------------------------
// The look and the sound
// ---------------------------------------------------------------------------

void ATownFire::BuildFlame()
{
	Flame->SetStaticMesh(CardMesh);
	Flame->SetMaterial(0, GlowMaterial);
	Flame->SetNumCustomDataFloats(GlowFloats);
	Flame->ClearInstances();
	// The engine's plane is 100 x 100 in its XY, facing +Z; a streak's bright head is at its +X end. Pitched down a quarter
	// turn, its head is at the fire bed and its tail up the flame, its face out to the side (AWatchFire::BuildFlame's cards).
	const float Tongue[] = { FlameColor.R, FlameColor.G, FlameColor.B, FlameStrength, StreakShape };
	for (int32 Card = 0; Card < Tongues; ++Card)
	{
		const FRotator Around(-90.0, 180.0 / Tongues * Card, 0.0);
		Flame->SetCustomData(Flame->AddInstance(FTransform(Around, FVector(0.0, 0.0, FlameHeight * 0.5),
			FVector(FlameHeight / 100.f, FlameWidth / 100.f, 1.f))), Tongue);
	}
	// Two shorter tongues off the middle, so the top is ragged, not one point.
	const float Short[] = { FlameColor.R, FlameColor.G * 0.95f, FlameColor.B * 0.8f, FlameStrength * 0.8f, StreakShape };
	for (int32 Card = 0; Card < 2; ++Card)
	{
		const FVector Off(Card == 0 ? FlameWidth * 0.25 : -FlameWidth * 0.2, Card == 0 ? -FlameWidth * 0.1 : FlameWidth * 0.2, FlameHeight * 0.32);
		Flame->SetCustomData(Flame->AddInstance(FTransform(FRotator(-90.0, 60.0 + 70.0 * Card, 0.0), Off,
			FVector(FlameHeight * 0.65f / 100.f, FlameWidth * 0.7f / 100.f, 1.f))), Short);
	}
	// A bright round heart low in the flames, seen from every side (flat, and standing along X and Y).
	const float Heart[] = { FlameColor.R, FlameColor.G * 0.9f, FlameColor.B * 0.6f, FlameStrength * 1.2f, RoundShape };
	const FRotator HeartCards[] = { FRotator(0.0, 0.0, 0.0), FRotator(0.0, 0.0, 90.0), FRotator(90.0, 0.0, 0.0) };
	for (const FRotator& Card : HeartCards)
	{
		Flame->SetCustomData(Flame->AddInstance(FTransform(Card, FVector(0.0, 0.0, FlameHeight * 0.2), FVector(FlameWidth * 1.4f / 100.f))), Heart);
	}
	// A soft halo: the fire's glow across the road from a distance, past where its light has faded.
	const float Halo[] = { FlameColor.R, FlameColor.G * 0.85f, FlameColor.B * 0.7f, FlameStrength * HaloGlow, RoundShape };
	for (const FRotator& Card : { FRotator(0.0, 0.0, 90.0), FRotator(90.0, 0.0, 0.0) })
	{
		Flame->SetCustomData(Flame->AddInstance(FTransform(Card, FVector(0.0, 0.0, FlameHeight * 0.35), FVector(FlameHeight * HaloSize / 100.f))), Halo);
	}
	FireLight->SetRelativeLocation(FVector(0.0, 0.0, FlameHeight * LightUp));
	FireLight->SetAttenuationRadius(LightRadius);
	FireLight->bUseTemperature = true;
	FireLight->Temperature = LightTemperature;
	// The wisp rises from the fire bed, turned to the wind whichever way the fire stands.
	Smoke->SetStaticMesh(SmokeMesh);
	Smoke->SetRelativeScale3D(FVector(SmokeScale));
	Smoke->SetRelativeLocation(FVector(0.0, 0.0, FlameHeight * 0.5));
	Smoke->SetUsingAbsoluteRotation(true);
	Smoke->SetWorldRotation(FRotator(0.0, SmokeYaw, 0.0));
}

void ATownFire::ApplyFire()
{
	const bool bWasBurning = bBurning;
	bBurning = bShown && BurnsIn(KindNow);
	if (bBurning && !bWasBurning)
	{
		// Lit while the player watches (a story step, the light switched): the flames catch from nothing. As a level begins it
		// is burning from the first frame.
		BurnTime = 0.f;
		CatchAge = bBegun ? 0.f : -1.f;
	}
	Flame->SetVisibility(bBurning && CardMesh != nullptr);
	FireLight->SetVisibility(bBurning);
	Smoke->SetVisibility(bBurning && SmokeMesh != nullptr);
	ApplySound();
	RefreshTick();
	if (!bBurning)
	{
		return;
	}
	// The flames catch from nothing, and flicker taller and shorter all the while (the watch fire's three unsteady waves).
	const float Caught = CatchAge >= 0.f ? AWatchFire::Catch(CatchAge) : 1.f;
	const float Flick = AWatchFire::Flicker(BurnTime);
	const float Height = FMath::Max(Caught * Flick, 0.02f);
	const float Width = FMath::Max(Caught * (1.f + 0.5f * (Flick - 1.f)), 0.02f);
	Flame->SetRelativeScale3D(FVector(Width, Width, Height));
	FireLight->SetIntensity(LightCandela * Caught * (0.85f + 0.15f * Flick));
}

void ATownFire::ApplySound()
{
	// Only in a level being played: not the editor's, where a fire placed by a forge would crackle at its builder.
	const UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	if (bBurning && !BurnLoop.IsValid())
	{
		BurnLoop = LooterSound::Start(this, LooterSoundCue::WatchFire::Burn, Root, NAME_None, SoundVolume, SoundPitch);
	}
	else if (!bBurning && BurnLoop.IsValid())
	{
		LooterSound::Stop(BurnLoop.Get(), 0.6f);
		BurnLoop.Reset();
	}
}

bool ATownFire::IsPlayerNear() const
{
	const UWorld* World = GetWorld();
	const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
	if (!Player)
	{
		return false;
	}
	FVector Eyes;
	FRotator Looking;
	Player->GetPlayerViewPoint(Eyes, Looking);
	return FVector::DistSquared(Eyes, GetActorLocation()) <= FMath::Square(static_cast<double>(FlickerRange));
}

void ATownFire::RefreshTick()
{
	SetActorTickEnabled(bBurning);
	if (bBurning)
	{
		SetActorTickInterval(CatchAge >= 0.f || IsPlayerNear() ? 0.f : FarTickSeconds);
	}
}

// ---------------------------------------------------------------------------
// How many lights are lit in a view
// ---------------------------------------------------------------------------

int32 ATownFire::CountLitInView(UWorld* World, const FVector& Eyes)
{
	if (!World)
	{
		return 0;
	}
	const double ReachSquared = FMath::Square(static_cast<double>(AHouseLights::LampDrawDistance));
	int32 Count = 0;
	for (TActorIterator<AHouseLights> It(World); It; ++It)
	{
		// Each lamp is drawn as far as it is now set to be (45 m, or 25 m for the smog's dim lamps).
		const double Reach = FMath::Min(ReachSquared, FMath::Square(static_cast<double>(It->Lamp->MaxDrawDistance)));
		Count += It->IsLampLit() && FVector::DistSquared(It->Lamp->GetComponentLocation(), Eyes) <= Reach ? 1 : 0;
	}
	for (TActorIterator<ATownFire> It(World); It; ++It)
	{
		Count += It->IsBurning() && FVector::DistSquared(It->FireLight->GetComponentLocation(), Eyes) <= ReachSquared ? 1 : 0;
	}
	return Count;
}

#if !UE_BUILD_SHIPPING
namespace
{
	/** Looter.Lights.Count: the town lights lit within 45 m of the camera, against the plan's 26 for any view. */
	void CountCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (!World || !World->IsGameWorld())
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.World() && Context.World()->IsGameWorld())
				{
					World = Context.World();
					break;
				}
			}
		}
		const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
		if (!Player)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Lights.Count: play a level first."));
			return;
		}
		FVector Eyes;
		FRotator Looking;
		Player->GetPlayerViewPoint(Eyes, Looking);
		const ULightingStateSubsystem* Lighting = ULightingStateSubsystem::Get(World);
		const FName State = Lighting ? Lighting->GetState() : NAME_None;
		int32 Fires = 0;
		for (TActorIterator<ATownFire> It(World); It; ++It)
		{
			Fires += It->IsBurning() ? 1 : 0;
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Lights.Count: %d town lights lit within their draw distance of the camera (%.0f m; the smog's dim lamps %.0f m; limit 26; %d fires burning in the level), state %s (%s)."),
			ATownFire::CountLitInView(World, Eyes), AHouseLights::LampDrawDistance / 100.f, AHouseLights::GloomDrawDistance / 100.f, Fires,
			State.IsNone() ? TEXT("none") : *State.ToString(), LightingStates::KindName(LightingStates::Kind(State)));
	}

	FAutoConsoleCommandWithWorldAndArgs GCountCommand(TEXT("Looter.Lights.Count"),
		TEXT("Logs how many town lights (house, street and porch lamps, fires) are lit within their draw distance of the camera."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CountCommand));
}
#endif

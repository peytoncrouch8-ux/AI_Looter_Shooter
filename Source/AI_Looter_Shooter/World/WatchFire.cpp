// AWatchFire: the watch fire on Calder's Post's tower: lit by a hold, kept by the story, a cheap fire that flickers near.

#include "World/WatchFire.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionFocus.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSubsystem.h"
#include "Story/CaptionSubsystem.h"
#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "WatchFire"

namespace
{
	/** The gunfire's additive glow on the engine's plane (R, G, B, strength, shape: 0 a streak, 1 round). */
	const TCHAR* GlowPath = TEXT("/Game/Weapons/FX/M_FX_Glow.M_FX_Glow");
	const TCHAR* PlanePath = TEXT("/Engine/BasicShapes/Plane.Plane");
	constexpr int32 GlowFloats = 5;
	constexpr float StreakShape = 0.f;
	constexpr float RoundShape = 1.f;

	/** The Last Watch: its fire lit in its third step (from 0: 2), after the oil. */
	const FName LastWatch(TEXT("LastWatch"));
	constexpr int32 LightStep = 2;

	/** The tongues, crossed about the upright this many times; the halo's size and glow, a share of the flames'. */
	constexpr int32 Tongues = 4;
	constexpr float HaloSize = 2.4f;
	constexpr float HaloGlow = 0.1f;

	/** A flare's whole length (s); the light's swell at its peak; the flames' swell. */
	constexpr float FlareSeconds = 1.75f;
	constexpr float FlareLight = 2.f;
	constexpr float FlareHeight = 0.6f;
	constexpr float FlareWidth = 0.3f;

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

	FStoryCondition During(FName Mission, int32 FromStep = 0)
	{
		FStoryCondition Made;
		Made.DuringMission = Mission;
		Made.FromStep = FromStep;
		return Made;
	}
}

AWatchFire::AWatchFire()
{
	// Ticks only while lit (RefreshTick).
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	static UStaticMesh* const Plane = FindIfMade<UStaticMesh>(PlanePath);
	static UMaterialInterface* const Glow = FindIfMade<UMaterialInterface>(GlowPath);
	CardMesh = Plane;
	GlowMaterial = Glow;

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

	// A fire's light: warm, never a shadow (the area's rule for every lantern), and gone past a long walk.
	FireLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireLight"));
	FireLight->SetupAttachment(Root);
	FireLight->SetMobility(EComponentMobility::Movable);
	FireLight->SetCastShadows(false);
	FireLight->SetIntensityUnits(ELightUnits::Candelas);
	FireLight->SetIntensity(LightCandela);
	FireLight->SetAttenuationRadius(LightRadius);
	FireLight->SetLightColor(FLinearColor(1.f, 0.62f, 0.32f));
	FireLight->SetMaxDrawDistance(LightDrawDistance);
	FireLight->SetMaxDistanceFadeRange(1500.f);
	FireLight->SetVisibility(false);

	Prompt = LOCTEXT("Prompt", "Light the watch fire");
	DryLine = LOCTEXT("Dry", "The brazier's dry. It wants oil.");
	LightableWhen = { During(LastWatch, LightStep) };
	FStoryCondition After;
	After.AfterMissions = { LastWatch };
	LitWhen = { After };
	TriesWhen = { During(LastWatch) };
}

void AWatchFire::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildFlame();
	ApplyFire();
}

void AWatchFire::BeginPlay()
{
	Super::BeginPlay();
	BuildFlame();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		MissionsChangedHandle = Runner->OnChanged.AddUObject(this, &AWatchFire::HandleMissionsChanged);
	}
	// Lit from the start when the story has it burning (a later visit): no flare.
	RefreshStory();
	ApplyFire();
	ApplySound();
	RefreshTick();
}

void AWatchFire::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(MissionsChangedHandle);
	}
	BoundRunner.Reset();
	MissionsChangedHandle.Reset();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	LooterSound::Stop(BurnLoop.Get());
	BurnLoop.Reset();
	Super::EndPlay(EndPlayReason);
}

void AWatchFire::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// The rules
// ---------------------------------------------------------------------------

bool AWatchFire::AnyMet(const TArray<FStoryCondition>& Conditions, const FCampaignRecord& Campaign, const UMissionRunner* Runner)
{
	return Conditions.ContainsByPredicate([&Campaign, Runner](const FStoryCondition& Each) { return Each.IsMet(Campaign, Runner); });
}

float AWatchFire::Flicker(float Seconds)
{
	return 1.f + 0.08f * FMath::Sin(UE_TWO_PI * 7.3f * Seconds) + 0.05f * FMath::Sin(UE_TWO_PI * 11.9f * Seconds + 1.3f)
		+ 0.04f * FMath::Sin(UE_TWO_PI * 3.1f * Seconds + 0.4f);
}

float AWatchFire::Flare(float Since)
{
	if (Since < 0.f)
	{
		return 0.f;
	}
	return FMath::SmoothStep(0.f, 0.25f, Since) * (1.f - FMath::SmoothStep(0.25f, FlareSeconds, Since));
}

float AWatchFire::Catch(float Since)
{
	return Since < 0.f ? 1.f : FMath::SmoothStep(0.f, 0.6f, Since);
}

bool AWatchFire::ShouldBeLit(const FCampaignRecord& Campaign, const UMissionRunner* Runner) const
{
	return Campaign.HasFlag(LitFlag) || AnyMet(LitWhen, Campaign, Runner);
}

bool AWatchFire::CanLight() const
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return !bLit && Runner && Runner->IsActive() && AnyMet(LightableWhen, Runner->GetCampaign(), Runner);
}

bool AWatchFire::IsDryNow() const
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return !bLit && !CanLight() && Runner && Runner->IsActive() && AnyMet(TriesWhen, Runner->GetCampaign(), Runner);
}

void AWatchFire::RefreshStory()
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	if (!Runner || !Runner->IsActive())
	{
		return;
	}
	const bool bWanted = ShouldBeLit(Runner->GetCampaign(), Runner);
	if (bWanted != bLit)
	{
		SetLit(bWanted);
	}
}

void AWatchFire::HandleMissionsChanged()
{
	RefreshStory();
}

// ---------------------------------------------------------------------------
// Using it
// ---------------------------------------------------------------------------

FInteractionOptions AWatchFire::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options;
	Options.Reach = Reach;
	if (CanLight())
	{
		Options.bTap = false;
		Options.bHold = true;
		Options.HoldSeconds = HoldSeconds;
		Options.HoldPrompt = Prompt;
		return Options;
	}
	if (IsDryNow())
	{
		// A tap, so the player learns at once what it wants.
		Options.bTap = true;
		Options.bHold = false;
		Options.TapPrompt = Prompt;
		return Options;
	}
	return FInteractionOptions::None();
}

bool AWatchFire::Interact(UInteractionComponent& User, bool bHeld)
{
	if (bHeld)
	{
		return Light(User.GetOwner());
	}
	if (!IsDryNow())
	{
		return false;
	}
	LooterSound::PlayAt(this, LooterSoundCue::WatchFire::Dry, GetInteractionLocation().Get(GetActorLocation()));
	if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this))
	{
		Captions->Play({ FStoryLine::Make(FText::GetEmpty(), DryLine) }, ECaptionPlay::Interrupt);
	}
	return true;
}

TOptional<FVector> AWatchFire::GetInteractionLocation() const
{
	return TOptional<FVector>(GetActorLocation() + FVector(0.0, 0.0, UseHeight));
}

TOptional<FInteractionBox> AWatchFire::GetInteractionBox() const
{
	// The basket from under its fire bed up into the first of the flames, its middle a little over the bed.
	return InteractionFocus::BoxAround(GetActorLocation() + FVector(0.0, 0.0, LookHalfSize.Z * 0.2), LookHalfSize.GetAbs(), GetActorQuat());
}

bool AWatchFire::Light(AActor* User)
{
	if (!CanLight())
	{
		return false;
	}
	bLit = true;
	BurnTime = 0.f;
	FlareAge = 0.f;
	// Kept by the story from now on: every later visit finds it burning.
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		Runner->GetCampaign().SetFlag(LitFlag);
	}
	if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
	{
		Sessions->SaveSoon();
	}
	LooterSound::PlayAt(this, LooterSoundCue::WatchFire::Light, GetActorLocation());
	ApplyFire();
	ApplySound();
	RefreshTick();
	UE_LOG(LogLooter, Log, TEXT("%s: the watch fire is lit (%s)."), *GetActorNameOrLabel(), User ? *User->GetActorNameOrLabel() : TEXT("nobody"));
	return true;
}

void AWatchFire::SetLit(bool bInLit)
{
	bLit = bInLit;
	FlareAge = -1.f;
	ApplyFire();
	ApplySound();
	RefreshTick();
}

void AWatchFire::Advance(float DeltaSeconds)
{
	if (!bLit)
	{
		RefreshTick();
		return;
	}
	BurnTime += DeltaSeconds;
	if (FlareAge >= 0.f)
	{
		FlareAge += DeltaSeconds;
		if (FlareAge >= FlareSeconds)
		{
			FlareAge = -1.f;
		}
	}
	ApplyFire();
	RefreshTick();
}

float AWatchFire::GetLightCandela() const
{
	return bLit && FireLight ? FireLight->Intensity : 0.f;
}

float AWatchFire::GetFlameScale() const
{
	return bLit && Flame ? static_cast<float>(Flame->GetRelativeScale3D().Z) : 0.f;
}

// ---------------------------------------------------------------------------
// The look and the sound
// ---------------------------------------------------------------------------

void AWatchFire::BuildFlame()
{
	Flame->SetStaticMesh(CardMesh);
	Flame->SetMaterial(0, GlowMaterial);
	Flame->SetNumCustomDataFloats(GlowFloats);
	Flame->ClearInstances();
	// The engine's plane is 100 x 100 in its XY, facing +Z; a streak's bright head is at its +X end. Pitched down a quarter
	// turn, its head is at the fire bed and its tail up the flame, its face out to the side.
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
	// A faint wide halo: what the posts down the Rim see of it, a point of light on the tower from far off.
	const float Halo[] = { FlameColor.R, FlameColor.G * 0.85f, FlameColor.B * 0.7f, FlameStrength * HaloGlow, RoundShape };
	for (const FRotator& Card : { FRotator(0.0, 0.0, 90.0), FRotator(90.0, 0.0, 0.0) })
	{
		Flame->SetCustomData(Flame->AddInstance(FTransform(Card, FVector(0.0, 0.0, FlameHeight * 0.35), FVector(FlameHeight * HaloSize / 100.f))), Halo);
	}
	FireLight->SetRelativeLocation(FVector(0.0, 0.0, FlameHeight * LightUp));
	FireLight->SetAttenuationRadius(LightRadius);
	FireLight->SetMaxDrawDistance(LightDrawDistance);
}

void AWatchFire::ApplyFire()
{
	Flame->SetVisibility(bLit && CardMesh != nullptr);
	FireLight->SetVisibility(bLit);
	if (!bLit)
	{
		return;
	}
	// The flames catch from nothing as it's lit, swell with the flare, and flicker taller and shorter all the while.
	const float Caught = FlareAge >= 0.f ? Catch(FlareAge) : 1.f;
	const float Swell = Flare(FlareAge);
	const float Flick = Flicker(BurnTime);
	const float Height = FMath::Max(Caught * (1.f + FlareHeight * Swell) * Flick, 0.02f);
	const float Width = FMath::Max(Caught * (1.f + FlareWidth * Swell) * (1.f + 0.5f * (Flick - 1.f)), 0.02f);
	Flame->SetRelativeScale3D(FVector(Width, Width, Height));
	FireLight->SetIntensity(LightCandela * Caught * (1.f + FlareLight * Swell) * (0.85f + 0.15f * Flick));
}

void AWatchFire::ApplySound()
{
	if (bLit && !BurnLoop.IsValid())
	{
		BurnLoop = LooterSound::Start(this, LooterSoundCue::WatchFire::Burn, Root);
	}
	else if (!bLit && BurnLoop.IsValid())
	{
		LooterSound::Stop(BurnLoop.Get(), 0.6f);
		BurnLoop.Reset();
	}
}

bool AWatchFire::IsPlayerNear() const
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

void AWatchFire::RefreshTick()
{
	SetActorTickEnabled(bLit);
	if (bLit)
	{
		SetActorTickInterval(FlareAge >= 0.f || IsPlayerNear() ? 0.f : FarTickSeconds);
	}
}

#undef LOCTEXT_NAMESPACE

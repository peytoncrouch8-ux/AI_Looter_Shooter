// AChapelWindowHose: the Company's hose over Saint Ada's window sill, drinking her ember; its pull and its whip out.

#include "World/ChapelWindowHose.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionFocus.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "World/ChapelReliquary.h"
#include "World/CompanyHoseCart.h"
#include "World/LamplighterEffects.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "ChapelWindowHose"

const FName AChapelWindowHose::HoseTag(TEXT("Hose_ChapelWindow"));
const FName AChapelWindowHose::PulledEvent(TEXT("ChapelHose.Pulled"));

namespace
{
	const TCHAR* HoseCylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* HoseQuadPath = TEXT("/Engine/BasicShapes/Plane.Plane");
	const TCHAR* HoseRubberPath = TEXT("/Game/Art/Landfall/Materials/MI_Landfall_HoseRubber.MI_Landfall_HoseRubber");
	const TCHAR* HoseBlackPath = TEXT("/Game/Art/Materials/MI_PaintBlack.MI_PaintBlack");
	const TCHAR* HoseBrassPath = TEXT("/Game/Art/Materials/MI_BrassWorn.MI_BrassWorn");
	const TCHAR* HoseGlowPath = TEXT("/Game/Weapons/FX/M_FX_Glow.M_FX_Glow");
	constexpr int32 HoseGlowFloats = 5;

	/** The nozzle's size; her light's pulses along it (their look and pace: ChapelWindowHoseLine.cpp). */
	const FVector NozzleScale(0.12, 0.12, 0.3);
	constexpr int32 PulseCount = 5;
	/** Its drink is heard within this (cm). */
	constexpr double DrinkReach = 1800.0;
	/** The smog it coughs out of the window as it comes free. */
	constexpr int32 CoughPuffs = 7;

	template <typename T>
	T* HoseAssetIfMade(const TCHAR* ObjectPath)
	{
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath))))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<T> Finder(ObjectPath);
		return Finder.Object;
	}
}

AChapelWindowHose::AChapelWindowHose()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	Tags.Add(HoseTag);

	static UStaticMesh* const Cylinder = HoseAssetIfMade<UStaticMesh>(HoseCylinderPath);
	static UStaticMesh* const Quad = HoseAssetIfMade<UStaticMesh>(HoseQuadPath);
	static UMaterialInterface* const Rubber = HoseAssetIfMade<UMaterialInterface>(HoseRubberPath);
	static UMaterialInterface* const Black = HoseAssetIfMade<UMaterialInterface>(HoseBlackPath);
	static UMaterialInterface* const Brass = HoseAssetIfMade<UMaterialInterface>(HoseBrassPath);
	static UMaterialInterface* const Glow = HoseAssetIfMade<UMaterialInterface>(HoseGlowPath);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// The hose: seen only (the player walks over it; the Interact key finds it by its box).
	Sections = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Sections"));
	Sections->SetupAttachment(Root);
	Sections->SetMobility(EComponentMobility::Movable);
	Sections->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Sections->SetCanEverAffectNavigation(false);
	Sections->SetStaticMesh(Cylinder);
	if (UMaterialInterface* Skin = Rubber ? Rubber : Black)
	{
		Sections->SetMaterial(0, Skin);
	}

	Nozzle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Nozzle"));
	Nozzle->SetupAttachment(Root);
	Nozzle->SetMobility(EComponentMobility::Movable);
	Nozzle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Nozzle->SetCanEverAffectNavigation(false);
	Nozzle->SetStaticMesh(Cylinder);
	Nozzle->SetRelativeScale3D(NozzleScale);
	if (Brass)
	{
		Nozzle->SetMaterial(0, Brass);
	}

	Pulses = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Pulses"));
	Pulses->SetupAttachment(Root);
	Pulses->SetMobility(EComponentMobility::Movable);
	Pulses->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Pulses->SetCastShadow(false);
	Pulses->bReceivesDecals = false;
	Pulses->SetStaticMesh(Quad);
	if (Glow)
	{
		Pulses->SetMaterial(0, Glow);
	}
	Pulses->SetVisibility(false);

	Prompt = LOCTEXT("Pull", "Pull out the hose");
}

void AChapelWindowHose::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// In the editor it shows in the window, drinking.
	bOut = false;
	PlaceSections();
}

void AChapelWindowHose::BeginPlay()
{
	Super::BeginPlay();
	if (Pulses)
	{
		Pulses->ClearInstances();
		Pulses->SetNumCustomDataFloats(HoseGlowFloats);
		for (int32 Index = 0; Index < PulseCount; ++Index)
		{
			Pulses->AddInstance(FTransform(FQuat::Identity, GetActorLocation(), FVector(0.01)), /*bWorldSpace*/ true);
		}
	}
	Reliquary = FindReliquary();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		ChangedHandle = Runner->OnChanged.AddUObject(this, &AChapelWindowHose::HandleRunnerChanged);
	}
	bShown = false;
	bOut = false;
	WhipClock = WhipSeconds;
	RefreshStory();
}

void AChapelWindowHose::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(ChangedHandle);
	}
	BoundRunner.Reset();
	ChangedHandle.Reset();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	LooterSound::Stop(DrinkSound.Get(), 0.f);
	DrinkSound.Reset();
	Super::EndPlay(EndPlayReason);
}

void AChapelWindowHose::HandleRunnerChanged()
{
	RefreshStory();
}

// ---------------------------------------------------------------------------
// Its story
// ---------------------------------------------------------------------------

void AChapelWindowHose::RefreshStory()
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	bool bWantShown = true;
	bool bPast = false;
	if (Runner)
	{
		const FCampaignRecord& Campaign = Runner->GetCampaign();
		bWantShown = ShownWhen.IsEmpty() || ShownWhen.IsMet(Campaign, Runner);
		bPast = PulledWhen.ContainsByPredicate([&Campaign, Runner](const FStoryCondition& When) { return When.IsMet(Campaign, Runner); });
	}
	bShown = bWantShown;
	if (bPast && !bOut)
	{
		// Its story is past it: out on the yard's ground already, no whip.
		bOut = true;
		WhipClock = WhipSeconds;
	}
	SetActorHiddenInGame(!bShown);
	SetReliquaryDrinking(bShown && !bOut);
	PlaceSections();
	PlacePulses();
	SetActorTickEnabled(bShown && (!bOut || IsWhipping()));
	if (!bShown || bOut)
	{
		LooterSound::Stop(DrinkSound.Get(), 0.3f);
		DrinkSound.Reset();
	}
}

bool AChapelWindowHose::CanPull() const
{
	if (!bShown || bOut)
	{
		return false;
	}
	if (PullableWhen.IsEmpty())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return !Runner || PullableWhen.IsMet(Runner->GetCampaign(), Runner);
}

bool AChapelWindowHose::Pull(AController* By, bool bForce)
{
	if (bOut || (!bForce && !CanPull()))
	{
		return false;
	}
	bOut = true;
	WhipClock = 0.f;
	SetReliquaryDrinking(false);
	LooterSound::Stop(DrinkSound.Get(), 0.05f);
	DrinkSound.Reset();
	CoughAndSnap();
	// Its cart stops drinking (the spill, Vane's complaint), if the pull came first.
	if (ACompanyHoseCart* Cart = FindCart(); Cart && Cart->GetCartState() == EHoseCartState::Pumping)
	{
		Cart->StopPumping(By, /*bBurst*/ false);
	}
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		Runner->NotifyEvent(FMissionEvent::Named(PulledEvent, this));
	}
	SetActorTickEnabled(true);
	UE_LOG(LogLooter, Log, TEXT("%s: pulled out of the window%s."), *GetActorNameOrLabel(), By ? TEXT("") : TEXT(" (its cart stopped)"));
	return true;
}

// ---------------------------------------------------------------------------
// Being pulled
// ---------------------------------------------------------------------------

FInteractionOptions AChapelWindowHose::GetInteractionOptions(const UInteractionComponent& User) const
{
	if (!CanPull())
	{
		return FInteractionOptions::None();
	}
	FInteractionOptions Options;
	Options.bTap = false;
	Options.bHold = true;
	Options.HoldSeconds = HoldSeconds;
	Options.HoldPrompt = Prompt;
	return Options;
}

bool AChapelWindowHose::Interact(UInteractionComponent& User, bool bHeld)
{
	if (!bHeld)
	{
		return false;
	}
	const APawn* Pawn = Cast<APawn>(User.GetOwner());
	return Pull(Pawn ? Pawn->GetController() : nullptr);
}

TOptional<FVector> AChapelWindowHose::GetInteractionLocation() const
{
	// Just over the sill, where it crosses into the window.
	return TOptional<FVector>(GetActorTransform().TransformPosition(FVector(12.0, 0.0, 10.0)));
}

TOptional<FInteractionBox> AChapelWindowHose::GetInteractionBox() const
{
	// The sill and the hose hanging over it outside, down toward the ground: looked at anywhere there.
	return InteractionFocus::BoxAround(GetActorTransform().TransformPosition(FVector(25.0, 0.0, -35.0)), FVector(35.0, 45.0, 60.0),
		GetActorQuat());
}

// (Its line, its sections, its pulses and its whip: ChapelWindowHoseLine.cpp.)

void AChapelWindowHose::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void AChapelWindowHose::Advance(float DeltaSeconds)
{
	if (!bShown)
	{
		return;
	}
	if (bOut)
	{
		if (WhipClock < WhipSeconds)
		{
			WhipClock = FMath::Min(WhipClock + DeltaSeconds, WhipSeconds);
			PlaceSections();
			if (WhipClock >= WhipSeconds)
			{
				// Down on the yard: a slap on the ground, and it lies still.
				LooterSound::PlayAt(this, LooterSoundCue::ImpactWood, GetActorTransform().TransformPosition(PulledRest), 0.7f, 0.8f);
				SetActorTickEnabled(false);
			}
		}
		return;
	}
	// Its cart stopped by the valve or the tank: the hose bucks and whips out on its own, and the missions hear it out.
	if (const ACompanyHoseCart* Cart = FindCart(); Cart && Cart->GetCartState() == EHoseCartState::Stopped)
	{
		if (Pull(nullptr, /*bForce*/ true))
		{
			if (UMissionRunner* Runner = UMissionRunner::Get(this))
			{
				Runner->NotifyEvent(FMissionEvent::Interaction(this, /*bHeld*/ true));
			}
		}
		return;
	}
	PulseClock += DeltaSeconds;
	PlacePulses();
	if (IsPlayerNear(DrinkReach))
	{
		if (!DrinkSound.IsValid())
		{
			DrinkSound = LooterSound::Start(this, LooterSoundCue::DredgerHoseDrain, Root, NAME_None, 0.45f, 0.75f);
		}
	}
	else if (DrinkSound.IsValid())
	{
		LooterSound::Stop(DrinkSound.Get(), 0.3f);
		DrinkSound.Reset();
	}
}

FVector AChapelWindowHose::GetNozzle() const
{
	TArray<FVector> Points;
	BuildLine(Points);
	return GetActorTransform().TransformPosition(Points.Num() > 0 ? Points.Last() : FVector::ZeroVector);
}

void AChapelWindowHose::CoughAndSnap()
{
	const FVector Sill = GetActorTransform().TransformPosition(FVector(20.0, 0.0, 15.0));
	const FVector Out = GetActorForwardVector();
	LooterSound::PlayAt(this, LooterSoundCue::DredgerHoseSnap, Sill, 1.15f, 0.9f);
	LooterSound::PlayAt(this, LooterSoundCue::ImpactWood, Sill, 0.8f, 1.1f);
	// What it drank coughs out of the window as it comes free: black smog and a spit of sparks off the latch.
	for (int32 Puff = 0; Puff < CoughPuffs; ++Puff)
	{
		const FVector Drift = Out * FMath::FRandRange(60.f, 140.f) + FVector(FMath::FRandRange(-50.f, 50.f), FMath::FRandRange(-50.f, 50.f),
			FMath::FRandRange(50.f, 120.f));
		LamplighterEffects::SmokePuff(this, Sill + FVector(0.0, 0.0, FMath::FRandRange(-20.f, 40.f)), Drift);
	}
	LamplighterEffects::Sparks(this, Sill, Out, 6, 0.6f);
	if (IsPlayerNear(800.0))
	{
		LamplighterEffects::Shake(this, 0.35f, 0.35f);
	}
}

bool AChapelWindowHose::IsPlayerNear(double Within) const
{
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Player = Controller ? Controller->GetPawn() : nullptr;
	return Player && FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) <= Within * Within;
}

ACompanyHoseCart* AChapelWindowHose::FindCart() const
{
	UWorld* World = GetWorld();
	if (!World || CartTag.IsNone())
	{
		return nullptr;
	}
	for (TActorIterator<ACompanyHoseCart> It(World); It; ++It)
	{
		if (It->ActorHasTag(CartTag))
		{
			return *It;
		}
	}
	return nullptr;
}

AChapelReliquary* AChapelWindowHose::FindReliquary() const
{
	UWorld* World = GetWorld();
	if (!World || ReliquaryTag.IsNone())
	{
		return nullptr;
	}
	for (TActorIterator<AChapelReliquary> It(World); It; ++It)
	{
		if (It->ActorHasTag(ReliquaryTag))
		{
			return *It;
		}
	}
	return nullptr;
}

void AChapelWindowHose::SetReliquaryDrinking(bool bDrinking) const
{
	if (AChapelReliquary* Found = Reliquary.Get())
	{
		Found->SetDrinking(bDrinking);
	}
}

#undef LOCTEXT_NAMESPACE

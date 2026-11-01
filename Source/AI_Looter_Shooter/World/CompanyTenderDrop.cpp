// ACompanyTenderDrop: a Company tender flown off the barge to hang over a drop, its ropes down, and back.

#include "World/CompanyTenderDrop.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/RappelSpawner.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "World/LamplighterEffects.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The user's Tripo tender (TRIPO_MODELS.md no. 14), else the gang's skiff scaled to ~12 m in iron. */
	const TCHAR* TenderHullPath = TEXT("/Game/Art/Tripo/SM_CompanyTender.SM_CompanyTender");
	const TCHAR* TenderSkiffPath = TEXT("/Game/Art/Vehicles/SM_Skiff_A_Gang.SM_Skiff_A_Gang");
	const TCHAR* TenderIronPath = TEXT("/Game/Art/Materials/MI_IronBlack.MI_IronBlack");
	constexpr float SkiffScale = 1.35f;

	/** Hanging: it bobs this much (cm, s) and rolls a little (degrees). */
	constexpr float BobHeight = 28.f;
	constexpr float BobSeconds = 3.4f;
	constexpr float BobRoll = 1.6f;
	/** Its run in turns from its heading to HoverYaw over the run's last share. */
	constexpr float TurnShare = 0.35f;
	/** Its engine is heard within this (cm) as it comes. */
	constexpr double EngineReach = 12000.0;

	template <typename T>
	T* TenderAssetIfMade(const TCHAR* ObjectPath)
	{
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath))))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<T> Finder(ObjectPath);
		return Finder.Object;
	}

	/** Eased both ends: a ship slowing into a hover. */
	float EaseRun(float Alpha)
	{
		const float A = FMath::Clamp(Alpha, 0.f, 1.f);
		return A * A * (3.f - 2.f * A);
	}
}

ACompanyTenderDrop::ACompanyTenderDrop()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	static UStaticMesh* const Tripo = TenderAssetIfMade<UStaticMesh>(TenderHullPath);
	static UStaticMesh* const Skiff = TenderAssetIfMade<UStaticMesh>(TenderSkiffPath);
	static UMaterialInterface* const Iron = TenderAssetIfMade<UMaterialInterface>(TenderIronPath);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	Hull = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hull"));
	Hull->SetupAttachment(Root);
	Hull->SetMobility(EComponentMobility::Movable);
	Hull->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Hull->SetCanEverAffectNavigation(false);
	if (Tripo)
	{
		Hull->SetStaticMesh(Tripo);
	}
	else if (Skiff)
	{
		Hull->SetStaticMesh(Skiff);
		Hull->SetRelativeScale3D(FVector(SkiffScale));
		for (int32 Slot = 0; Iron && Slot < Skiff->GetStaticMaterials().Num(); ++Slot)
		{
			Hull->SetMaterial(Slot, Iron);
		}
	}
	// Its ropes' tops by default: four, two a side (the build sets them to the hull it has).
	RopeAnchors = { FVector(300.0, 230.0, -60.0), FVector(-300.0, 230.0, -60.0), FVector(300.0, -230.0, -60.0),
		FVector(-300.0, -230.0, -60.0) };
}

void ACompanyTenderDrop::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void ACompanyTenderDrop::BeginPlay()
{
	Super::BeginPlay();
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		ChangedHandle = Runner->OnChanged.AddUObject(this, &ACompanyTenderDrop::HandleRunnerChanged);
	}
	// Placed hanging where it hovers (the editor's look); play starts it away until its story calls it.
	if (HoverAt.IsNearlyZero())
	{
		HoverAt = GetActorLocation();
		HoverYaw = GetActorRotation().Yaw;
	}
	SetState(ETenderDropState::Away);
	bStoryHeld = false;
	RefreshStory();
}

void ACompanyTenderDrop::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(ChangedHandle);
	}
	BoundRunner.Reset();
	ChangedHandle.Reset();
	LooterSound::Stop(EngineSound.Get(), 0.f);
	EngineSound.Reset();
	Super::EndPlay(EndPlayReason);
}

void ACompanyTenderDrop::HandleRunnerChanged()
{
	RefreshStory();
}

void ACompanyTenderDrop::RefreshStory()
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	const bool bHolds = Runner && !ComeWhen.IsEmpty() && ComeWhen.IsMet(Runner->GetCampaign(), Runner);
	if (bHolds && !bStoryHeld && State == ETenderDropState::Away)
	{
		Come();
	}
	else if (!bHolds && bStoryHeld && (State == ETenderDropState::Coming || State == ETenderDropState::Hovering))
	{
		// Its step is over (the turn-in, a load past it): it goes home.
		Leave();
	}
	bStoryHeld = bHolds;
}

void ACompanyTenderDrop::SetState(ETenderDropState NewState)
{
	State = NewState;
	Clock = 0.f;
	LingerClock = 0.f;
	const bool bOut = State != ETenderDropState::Away;
	SetActorHiddenInGame(!bOut);
	SetActorTickEnabled(bOut);
	if (!bOut)
	{
		LooterSound::Stop(EngineSound.Get(), 0.5f);
		EngineSound.Reset();
	}
	PlaceNow();
}

void ACompanyTenderDrop::Come(bool bForce)
{
	if (State != ETenderDropState::Away && !bForce)
	{
		return;
	}
	if (ComeFrom.IsNearlyZero())
	{
		// No barge to come from: in from high to the north-east.
		ComeFrom = HoverAt + FVector(9000.0, 9000.0, 2500.0);
	}
	SetState(ETenderDropState::Coming);
	EngineSound = LooterSound::Start(this, LooterSoundCue::ShipEngine, Root, NAME_None, 0.7f, 1.35f);
	UE_LOG(LogLooter, Log, TEXT("%s: off the barge, flying in to hang over the drop."), *GetActorNameOrLabel());
}

void ACompanyTenderDrop::HoverNow()
{
	if (State == ETenderDropState::Away)
	{
		Come(true);
	}
	SetState(ETenderDropState::Hovering);
}

void ACompanyTenderDrop::Leave()
{
	if (State == ETenderDropState::Away || State == ETenderDropState::Leaving)
	{
		return;
	}
	SetState(ETenderDropState::Leaving);
	UE_LOG(LogLooter, Log, TEXT("%s: its men are down; back to the barge."), *GetActorNameOrLabel());
}

FVector ACompanyTenderDrop::RunAt(const FVector& From, const FVector& To, float Alpha, float Rise)
{
	const float A = FMath::Clamp(Alpha, 0.f, 1.f);
	return FMath::Lerp(From, To, EaseRun(A)) + FVector(0.0, 0.0, Rise * 4.f * A * (1.f - A));
}

void ACompanyTenderDrop::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void ACompanyTenderDrop::Advance(float DeltaSeconds)
{
	Clock += DeltaSeconds;
	BobClock += DeltaSeconds;
	switch (State)
	{
	case ETenderDropState::Coming:
		if (Clock >= FlySeconds)
		{
			SetState(ETenderDropState::Hovering);
			// It settles into its hover: a creak of its frame, a shove of air under it.
			LamplighterEffects::DustSkirt(this, HoverAt - FVector(0.0, 0.0, 1400.0), 600.f, 6);
			LooterSound::PlayAt(this, LooterSoundCue::ShipCreak, HoverAt, 0.8f, 1.2f);
			return;
		}
		break;
	case ETenderDropState::Hovering:
		if (IsDropDone())
		{
			LingerClock += DeltaSeconds;
			if (LingerClock >= LeaveAfter)
			{
				Leave();
				return;
			}
		}
		break;
	case ETenderDropState::Leaving:
		if (Clock >= FlySeconds)
		{
			SetState(ETenderDropState::Away);
			return;
		}
		break;
	default:
		return;
	}
	PlaceNow();
}

void ACompanyTenderDrop::PlaceNow()
{
	const FVector From = ComeFrom.IsNearlyZero() ? HoverAt + FVector(9000.0, 9000.0, 2500.0) : ComeFrom;
	FVector At = HoverAt;
	float Yaw = HoverYaw;
	float Roll = 0.f;
	const FVector Heading = (HoverAt - From).GetSafeNormal2D();
	const float RunYaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Heading.Y, Heading.X)));
	switch (State)
	{
	case ETenderDropState::Coming:
	{
		const float Alpha = FMath::Clamp(Clock / FMath::Max(FlySeconds, 0.1f), 0.f, 1.f);
		At = RunAt(From, HoverAt, Alpha, RunRise);
		// Nose along its run, turning broadside to the drop as it slows.
		const float Turn = FMath::Clamp((Alpha - (1.f - TurnShare)) / TurnShare, 0.f, 1.f);
		Yaw = RunYaw + FMath::FindDeltaAngleDegrees(RunYaw, HoverYaw) * EaseRun(Turn);
		Roll = -6.f * FMath::Sin(Alpha * static_cast<float>(UE_PI));
		break;
	}
	case ETenderDropState::Leaving:
	{
		const float Alpha = FMath::Clamp(Clock / FMath::Max(FlySeconds, 0.1f), 0.f, 1.f);
		At = RunAt(HoverAt, From, Alpha, RunRise);
		const float Turn = FMath::Clamp(Alpha / TurnShare, 0.f, 1.f);
		const float Back = RunYaw + 180.f;
		Yaw = HoverYaw + FMath::FindDeltaAngleDegrees(HoverYaw, Back) * EaseRun(Turn);
		break;
	}
	default:
		break;
	}
	// It never hangs dead still: a slow bob and roll.
	const float TwoPi = static_cast<float>(UE_TWO_PI);
	const float Bob = FMath::Sin(BobClock * TwoPi / BobSeconds);
	At.Z += BobHeight * Bob;
	Roll += BobRoll * FMath::Sin(BobClock * TwoPi / (BobSeconds * 1.3f));
	SetActorLocationAndRotation(At, FRotator(0.0, Yaw, Roll), false, nullptr, ETeleportType::TeleportPhysics);
}

FVector ACompanyTenderDrop::GetRopeAnchor(int32 Index) const
{
	if (!RopeAnchors.IsValidIndex(Index))
	{
		return GetActorLocation();
	}
	return GetActorTransform().TransformPosition(RopeAnchors[Index]);
}

bool ACompanyTenderDrop::IsDropDone() const
{
	if (DropSpawnerId.IsNone())
	{
		return false;
	}
	const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this);
	const AEncounterSpawner* Spawner = Encounters ? Encounters->FindSpawner(DropSpawnerId) : nullptr;
	if (!Spawner)
	{
		return true;
	}
	const ARappelSpawner* Ropes = Cast<ARappelSpawner>(Spawner);
	return Spawner->GetWavesStarted() >= Spawner->GetPlannedWaves() && Spawner->NumOwed() == 0 && !(Ropes && Ropes->IsSliding());
}

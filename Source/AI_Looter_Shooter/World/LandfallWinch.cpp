// ALandfallWinch: the anchor winch on the Mooring Ledge: cranked with a held Interact, it runs the chain out to the
// Lamplighter and holds her.

#include "World/LandfallWinch.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionFocus.h"
#include "Interaction/InteractionSubsystem.h"
#include "World/LamplighterEffects.h"
#include "World/LamplighterRules.h"
#include "World/LamplighterShip.h"
#include "World/PilotSortie.h"
#include "Missions/MissionRunner.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "LandfallWinch"

namespace
{
	// The stand-in's look, one path each (the Tripo anchor winch replaces them: TRIPO_MODELS.md 16).
	const TCHAR* CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* CylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* IronPath = TEXT("/Game/Art/Materials/MI_IronBlack.MI_IronBlack");
	const TCHAR* WoodPath = TEXT("/Game/Art/Materials/MI_LanternWood.MI_LanternWood");

	/** Its size (cm): a 70 cm timber block, an iron drum 55 across and 80 long on it, a 40 cm crank arm at the drum's end. */
	constexpr float BlockHeight = 70.f;
	constexpr float DrumHeight = 100.f;
	constexpr float DrumWidth = 55.f;
	constexpr float DrumLength = 80.f;
	constexpr float CrankLength = 40.f;
	constexpr float ChainWidth = 7.f;
	/** The crank turns once a second while it's wound. */
	constexpr float CrankDegreesPerSecond = 360.f;
}

ALandfallWinch::ALandfallWinch()
{
	PrimaryActorTick.bCanEverTick = true;
	Tags.Add(LandfallRules::WinchTag());
	SetCanBeDamaged(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(CubePath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(CylinderPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Iron(IronPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Wood(WoodPath);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Base = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
	Base->SetupAttachment(Root);
	Base->SetStaticMesh(Cube.Object);
	Base->SetMaterial(0, Wood.Object);
	Base->SetRelativeLocation(FVector(0.0, 0.0, BlockHeight * 0.5));
	Base->SetRelativeScale3D(FVector(0.7, 1.0, BlockHeight / 100.0));
	Base->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Base->SetCanEverAffectNavigation(false);

	// The drum lies across the block (along its Y), the chain's end wound on it.
	Drum = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Drum"));
	Drum->SetupAttachment(Root);
	Drum->SetStaticMesh(Cylinder.Object);
	Drum->SetMaterial(0, Iron.Object);
	Drum->SetRelativeLocationAndRotation(FVector(0.0, 0.0, DrumHeight), FRotator(0.0, 0.0, 90.0));
	Drum->SetRelativeScale3D(FVector(DrumWidth, DrumWidth, DrumLength) / 100.0);
	Drum->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Drum->SetCanEverAffectNavigation(false);

	Crank = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Crank"));
	Crank->SetupAttachment(Root);
	Crank->SetStaticMesh(Cube.Object);
	Crank->SetMaterial(0, Iron.Object);
	Crank->SetRelativeScale3D(FVector(0.06, 0.06, CrankLength / 100.0));
	Crank->SetMobility(EComponentMobility::Movable);
	Crank->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Crank->SetCollisionResponseToAllChannels(ECR_Ignore);
	Crank->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Crank->SetCanEverAffectNavigation(false);

	Chain = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Chain"));
	Chain->SetupAttachment(Root);
	Chain->SetStaticMesh(Cylinder.Object);
	Chain->SetMaterial(0, Iron.Object);
	Chain->SetMobility(EComponentMobility::Movable);
	Chain->SetUsingAbsoluteLocation(true);
	Chain->SetUsingAbsoluteRotation(true);
	Chain->SetUsingAbsoluteScale(true);
	Chain->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Chain->SetCastShadow(false);
	Chain->SetVisibility(false);

	WindPrompt = LOCTEXT("Wind", "Drop the anchor");
	CastOffPrompt = LOCTEXT("CastOff", "Cast off");
}

void ALandfallWinch::BeginPlay()
{
	Super::BeginPlay();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
	if (!Ship.IsValid())
	{
		SetShip(ALamplighterShip::Find(GetWorld()));
	}
	Tick(0.f);
}

void ALandfallWinch::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	LooterSound::Stop(CrankSound.Get());
	CrankSound.Reset();
	Super::EndPlay(EndPlayReason);
}

void ALandfallWinch::SetShip(ALamplighterShip* InShip)
{
	Ship = InShip;
	PlaceChain(InShip && InShip->GetFlight() == EShipFlight::Moored && InShip->IsAnchored());
}

bool ALandfallWinch::CanWind() const
{
	const ALamplighterShip* Boat = Ship.Get();
	return Boat && Boat->GetFlight() == EShipFlight::Moored && !Boat->IsAnchored();
}

bool ALandfallWinch::IsBeingWound() const
{
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	const UInteractionComponent* Interaction = Pawn ? Pawn->FindComponentByClass<UInteractionComponent>() : nullptr;
	return Interaction && Interaction->GetFocusedActor() == this && Interaction->IsHolding() && Interaction->GetHoldProgress() > 0.f;
}

bool ALandfallWinch::CanCastOff() const
{
	const ALamplighterShip* Boat = Ship.Get();
	if (!Boat || Boat->GetFlight() != EShipFlight::Moored || !Boat->IsAnchored() || CastOffWhen.IsEmpty())
	{
		return false;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	const APilotSortie* Sortie = APilotSortie::Find(GetWorld());
	return Runner && CastOffWhen.IsMet(Runner->GetCampaign(), Runner) && Sortie && Sortie->GetPhase() == EPilotPhase::WaitCastOff;
}

bool ALandfallWinch::CastOff(bool bForce)
{
	APilotSortie* Sortie = APilotSortie::Find(GetWorld());
	if ((!bForce && !CanCastOff()) || !Sortie)
	{
		return false;
	}
	LooterSound::Stop(CrankSound.Get(), 0.05f);
	CrankSound.Reset();
	// The chain comes in off her hull with a clatter and the pawl drops: she's free of the ledge.
	LooterSound::PlayAt(this, LooterSoundCue::AnchorSet, GetActorLocation(), 1.f, 1.15f);
	LamplighterEffects::Sparks(this, Drum->GetComponentLocation(), FVector::UpVector, 5, 1.f);
	PlaceChain(false);
	if (!Sortie->CastOff())
	{
		UE_LOG(LogLooter, Warning, TEXT("%s: cast off, but the sortie wasn't waiting to fly her."), *GetActorNameOrLabel());
	}
	return true;
}

void ALandfallWinch::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Wound to drop the anchor (Main 1), or the other way to bring it up (Main 6's cast off).
	const bool bCastingOff = IsBeingWound() && CanCastOff();
	const bool bWinding = (CanWind() && IsBeingWound()) || bCastingOff;
	if (bWinding)
	{
		const float Turn = bCastingOff ? -CrankDegreesPerSecond : CrankDegreesPerSecond;
		CrankAngle = FMath::Fmod(CrankAngle + Turn * DeltaSeconds + 360.f, 360.f);
		if (!CrankSound.IsValid())
		{
			CrankSound = LooterSound::Start(this, LooterSoundCue::AnchorCrank, Drum, NAME_None, 1.f, bCastingOff ? 1.12f : 1.f);
		}
	}
	else if (CrankSound.IsValid())
	{
		LooterSound::Stop(CrankSound.Get(), 0.15f);
		CrankSound.Reset();
	}
	// The arm turns about the drum's end: its middle half its length out from the axle.
	const FVector Axle(0.0, DrumLength * 0.5 + 6.0, DrumHeight);
	const double Angle = FMath::DegreesToRadians(static_cast<double>(CrankAngle));
	const FVector Out(FMath::Sin(Angle), 0.0, FMath::Cos(Angle));
	Crank->SetRelativeLocationAndRotation(Axle + Out * (CrankLength * 0.5), FRotationMatrix::MakeFromZ(Out).Rotator());
	if (bChainShown)
	{
		PlaceChain(true);
	}
}

bool ALandfallWinch::Wind()
{
	ALamplighterShip* Boat = Ship.Get();
	if (!CanWind() || !Boat)
	{
		return false;
	}
	LooterSound::Stop(CrankSound.Get(), 0.05f);
	CrankSound.Reset();
	// The chain runs out with a clatter and snaps taut to her hull.
	LooterSound::PlayAt(this, LooterSoundCue::AnchorSet, GetActorLocation());
	LamplighterEffects::Sparks(this, Drum->GetComponentLocation(), FVector::UpVector, 5, 1.f);
	Boat->SetAnchored(true);
	PlaceChain(true);
	return true;
}

FInteractionOptions ALandfallWinch::GetInteractionOptions(const UInteractionComponent& User) const
{
	const bool bWind = CanWind();
	if (!bWind && !CanCastOff())
	{
		return FInteractionOptions::None();
	}
	FInteractionOptions Options;
	Options.bTap = false;
	Options.bHold = true;
	Options.HoldSeconds = HoldSeconds;
	Options.HoldPrompt = bWind ? WindPrompt : CastOffPrompt;
	return Options;
}

bool ALandfallWinch::Interact(UInteractionComponent& User, bool bHeld)
{
	return bHeld && (Wind() || CastOff());
}

TOptional<FVector> ALandfallWinch::GetInteractionLocation() const
{
	return TOptional<FVector>(GetActorTransform().TransformPosition(FVector(0.0, 0.0, DrumHeight)));
}

TOptional<FInteractionBox> ALandfallWinch::GetInteractionBox() const
{
	// The whole winch, block, drum and crank.
	return InteractionFocus::BoxAround(GetActorTransform().TransformPosition(FVector(0.0, 10.0, DrumHeight * 0.6)),
		FVector(45.0, 70.0, DrumHeight * 0.65), GetActorQuat());
}

void ALandfallWinch::PlaceChain(bool bShown)
{
	bChainShown = bShown;
	const ALamplighterShip* Boat = Ship.Get();
	if (!Chain)
	{
		return;
	}
	if (!bShown || !Boat)
	{
		// Not wound yet: the crank turns while the player holds on it.
		Chain->SetVisibility(false);
		SetActorTickEnabled(true);
		return;
	}
	const FVector From = Drum->GetComponentLocation();
	const FVector To = Boat->GetAnchorPoint();
	const FVector Span = To - From;
	const double Length = Span.Size();
	if (Length < 1.0)
	{
		Chain->SetVisibility(false);
		return;
	}
	Chain->SetWorldLocationAndRotation((From + To) * 0.5, FRotationMatrix::MakeFromZ(Span).Rotator());
	Chain->SetWorldScale3D(FVector(ChainWidth / 100.0, ChainWidth / 100.0, Length / 100.0));
	Chain->SetVisibility(true);
	// Taut now: nothing more to move each frame (unless it may cast her off again: its crank turns under the hold then).
	SetActorTickEnabled(!CastOffWhen.IsEmpty());
}

#undef LOCTEXT_NAMESPACE

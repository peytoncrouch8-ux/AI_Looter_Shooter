// ABargeHoseDrum: the gantry over the barge's hose drum, and the lever that jams it short of Saint Ada's roof.

#include "World/BargeHoseDrum.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionFocus.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionRunner.h"
#include "World/BargeBoarding.h"
#include "World/BargeLooks.h"
#include "World/BargeRules.h"
#include "World/LamplighterEffects.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"

#define LOCTEXT_NAMESPACE "BargeHoseDrum"

const FName ABargeHoseDrum::DrumTag(TEXT("Barge_HoseDrum"));

namespace
{
	/** The drum turns this fast paying hose out (degrees a second), ratcheting this often; heard within this (cm). */
	constexpr float TurnRate = 40.f;
	constexpr float RatchetEvery = 0.45f;
	constexpr float Earshot = 4500.f;
	/** The jam lever on the housing's -Y side (its frame): its pin (the gantry model's LeverPivot socket: the lever model
	 *  turns about it), and its arm's middle (where it's grabbed, and the stand-in rod's middle); thrown in this long. */
	const FVector LeverPivot(140.0, -172.0, 96.0);
	const FVector LeverAt(140.0, -172.0, 150.0);
	constexpr float LeverSeconds = 0.25f;
}

ABargeHoseDrum::ABargeHoseDrum()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.f;
	SetCanBeDamaged(false);
	Tags.Add(DrumTag);

	static UStaticMesh* const CubeMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cube);
	static UStaticMesh* const CylinderMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cylinder);
	static UStaticMesh* const GantryMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::DrumGantry);
	static UStaticMesh* const DrumMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::HoseDrum);
	static UStaticMesh* const LeverMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::DrumLever);
	static UMaterialInterface* const Iron = BargeLooks::Find<UMaterialInterface>(BargeLooks::Iron);
	static UMaterialInterface* const IronBlack = BargeLooks::Find<UMaterialInterface>(BargeLooks::IronBlack);
	static UMaterialInterface* const Rubber = BargeLooks::Find<UMaterialInterface>(BargeLooks::Rubber);
	static UMaterialInterface* const Brass = BargeLooks::Find<UMaterialInterface>(BargeLooks::Brass);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// The gantry's housing over the drum's well, cover to crouch behind: the kit's riveted box and red gantry (its pivot on
	// the deck, scaled flat to the rules' footprint; its five hulls block), else an iron box 4 m long.
	Housing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Housing"));
	Housing->SetupAttachment(Root);
	Housing->SetMobility(EComponentMobility::Movable);
	Housing->SetStaticMesh(GantryMesh ? GantryMesh : CubeMesh);
	if (GantryMesh)
	{
		Housing->SetRelativeScale3D(FVector(BargeRules::DrumGantryHalf.X / BargeLooks::GantryModelHalf.X,
			BargeRules::DrumGantryHalf.Y / BargeLooks::GantryModelHalf.Y, 1.0));
	}
	else
	{
		Housing->SetMaterial(0, Iron);
		Housing->SetRelativeLocation(FVector(0.0, 0.0, 70.0));
		Housing->SetRelativeScale3D(FVector(4.0, 2.6, 1.4));
	}
	Housing->SetCollisionProfileName(TEXT("BlockAll"));
	Housing->SetCollisionObjectType(ECC_WorldStatic);
	Housing->SetCanEverAffectNavigation(false);

	// The drum turning in its well, wound with black hose: the kit's drum on the well's floor (its axle in the gantry's top
	// bearing), else a rubber-black cylinder showing over the box.
	Drum = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Drum"));
	Drum->SetupAttachment(Root);
	Drum->SetMobility(EComponentMobility::Movable);
	Drum->SetStaticMesh(DrumMesh ? DrumMesh : CylinderMesh);
	if (DrumMesh)
	{
		Drum->SetRelativeLocation(FVector(0.0, 0.0, BargeLooks::DrumOnGantry));
		if (Rubber)
		{
			// Its hose the same rubber as her hanging hose.
			BargeLooks::SetSlot(*Drum, BargeLooks::HoseSlot, Rubber);
		}
	}
	else
	{
		Drum->SetMaterial(0, Rubber ? Rubber : IronBlack);
		Drum->SetRelativeLocation(FVector(0.0, 0.0, 170.0));
		Drum->SetRelativeScale3D(FVector(2.2, 2.2, 2.4));
	}
	Drum->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Drum->SetCanEverAffectNavigation(false);

	// The jam lever: the kit's iron arm with its brass grip turning about its pin, else a brass rod about its middle.
	Lever = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lever"));
	Lever->SetupAttachment(Root);
	Lever->SetMobility(EComponentMobility::Movable);
	Lever->SetStaticMesh(LeverMesh ? LeverMesh : CylinderMesh);
	if (LeverMesh)
	{
		Lever->SetRelativeLocation(LeverPivot);
	}
	else
	{
		Lever->SetMaterial(0, Brass);
		Lever->SetRelativeLocation(LeverAt);
		Lever->SetRelativeScale3D(FVector(0.09, 0.09, 1.2));
	}
	Lever->SetRelativeRotation(FRotator(0.f, 0.f, -35.f));
	Lever->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Lever->SetCanEverAffectNavigation(false);

	Prompt = LOCTEXT("Jam", "Jam the hose drum");
}

void ABargeHoseDrum::BeginPlay()
{
	Super::BeginPlay();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
}

void ABargeHoseDrum::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

bool ABargeHoseDrum::StoryHolds(const FStoryCondition& Condition) const
{
	if (Condition.IsEmpty())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return !Runner || Condition.IsMet(Runner->GetCampaign(), Runner);
}

bool ABargeHoseDrum::CanJam() const
{
	return !bJammed && StoryHolds(JamWhen);
}

bool ABargeHoseDrum::Jam(AActor* By, bool bForce)
{
	if (bJammed || (!bForce && !CanJam()))
	{
		return false;
	}
	bJammed = true;
	bTurning = false;
	LeverAlpha = 0.f;
	// The lever thrown into the gears: iron screaming on iron, the drum stopped dead, the deck shuddering under it.
	const FVector At = GetActorTransform().TransformPosition(FVector(0.0, 0.0, 170.0));
	LooterSound::PlayAt(this, LooterSoundCue::GrappleStrain, At, 1.3f, 0.7f);
	LooterSound::PlayAt(this, LooterSoundCue::ImpactMetal, At, 1.2f, 0.6f);
	LooterSound::PlayAt(this, LooterSoundCue::Foreman::PropSplinter, At, 0.6f, 1.3f);
	LamplighterEffects::Sparks(this, At, FVector::UpVector + GetActorRightVector(), 18, 1.2f);
	LamplighterEffects::Sparks(this, At, FVector::UpVector - GetActorRightVector(), 12, 1.f);
	LamplighterEffects::Shake(this, 0.45f, 0.8f);
	UE_LOG(LogLooter, Log, TEXT("%s: the hose drum is jammed; her hose stops short of Saint Ada's roof."), *GetActorNameOrLabel());
	if (ABargeBoarding* Boarding = Barge.Get())
	{
		Boarding->NoteDrumJammed();
	}
	return true;
}

void ABargeHoseDrum::SetJammedNow(bool bInJammed)
{
	bJammed = bInJammed;
	bTurning = !bInJammed && bTurning;
	LeverAlpha = bInJammed ? 1.f : 0.f;
	Lever->SetRelativeRotation(FRotator(0.f, 0.f, bInJammed ? 35.f : -35.f));
}

void ABargeHoseDrum::SetTurning(bool bInTurning)
{
	bTurning = bInTurning && !bJammed;
}

void ABargeHoseDrum::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bJammed && LeverAlpha < 1.f)
	{
		LeverAlpha = FMath::Min(LeverAlpha + DeltaSeconds / LeverSeconds, 1.f);
		Lever->SetRelativeRotation(FRotator(0.f, 0.f, FMath::Lerp(-35.f, 35.f, LeverAlpha)));
	}
	if (!bTurning)
	{
		return;
	}
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Player = Controller ? Controller->GetPawn() : nullptr;
	if (!Player || FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) > FMath::Square(Earshot))
	{
		return;
	}
	DrumAngle = FMath::Fmod(DrumAngle + TurnRate * DeltaSeconds, 360.f);
	Drum->SetRelativeRotation(FRotator(0.f, DrumAngle, 0.f));
	RatchetWait -= DeltaSeconds;
	if (RatchetWait <= 0.f)
	{
		// The pawl clicking over the drum's teeth: the sound of her hose going down to Saint Ada.
		RatchetWait = RatchetEvery;
		LooterSound::PlayAt(this, LooterSoundCue::AnchorCrank, Drum->GetComponentLocation(), 0.55f, 0.75f);
	}
}

FInteractionOptions ABargeHoseDrum::GetInteractionOptions(const UInteractionComponent& User) const
{
	if (!CanJam())
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

bool ABargeHoseDrum::Interact(UInteractionComponent& User, bool bHeld)
{
	return bHeld && Jam(User.GetOwner());
}

TOptional<FVector> ABargeHoseDrum::GetInteractionLocation() const
{
	return TOptional<FVector>(GetActorTransform().TransformPosition(LeverAt));
}

TOptional<FInteractionBox> ABargeHoseDrum::GetInteractionBox() const
{
	// The lever and the housing's side round it.
	return InteractionFocus::BoxAround(GetActorTransform().TransformPosition(LeverAt), FVector(80.0, 60.0, 90.0), GetActorQuat());
}

#undef LOCTEXT_NAMESPACE

// Out of line: the header only forward-declares the barge, and the weak pointer needs its full type to convert.
void ABargeHoseDrum::SetBarge(ABargeBoarding* InBarge)
{
	Barge = InBarge;
}

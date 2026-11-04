// AFurnaceDraftValve: a wheel valve on the soul furnace's pipe run; shut, the furnace starves a notch.

#include "World/FurnaceDraftValve.h"
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
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

#define LOCTEXT_NAMESPACE "FurnaceDraftValve"

const FName AFurnaceDraftValve::ValveTag(TEXT("Furnace_DraftValve"));

namespace
{
	/** The wheel spins down in this long once the hold is done, three and a half turns, slowing as it seats. */
	constexpr float SpinSeconds = 1.2f;
	constexpr float SpinTurns = 3.5f;
	/** The wheel's middle in front of the wall (its frame: +X into the hall), at a hand's height (the model's Wheel socket);
	 *  the valve model's top flange, where its riser to the ceiling starts (its PipeTop socket). */
	const FVector WheelAt(45.0, 0.0, 130.0);
	const FVector RiserFoot(-10.0, 0.0, 300.0);
	/** Steam spits from the pipe's joint while the wheel turns, this often. */
	constexpr float SteamEvery = 0.08f;
}

AFurnaceDraftValve::AFurnaceDraftValve()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetCanBeDamaged(false);
	Tags.Add(ValveTag);

	static UStaticMesh* const CylinderMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cylinder);
	static UStaticMesh* const ValveMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::DraftValve);
	static UStaticMesh* const WheelMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::ValveWheel);
	static UStaticMesh* const PipeMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Pipe);
	static UMaterialInterface* const Iron = BargeLooks::Find<UMaterialInterface>(BargeLooks::Iron);
	static UMaterialInterface* const Brass = BargeLooks::Find<UMaterialInterface>(BargeLooks::Brass);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// The valve: the kit's model at the floor (its pipe stub, body, bonnet and straps; its two hulls block), else an iron
	// pipe up the wall from the floor to the ceiling.
	Pipe = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pipe"));
	Pipe->SetupAttachment(Root);
	Pipe->SetMobility(EComponentMobility::Movable);
	Pipe->SetStaticMesh(ValveMesh ? ValveMesh : CylinderMesh);
	if (!ValveMesh)
	{
		Pipe->SetMaterial(0, Iron);
		// Up the wall from the floor to the ceiling, where the draught runs to the furnace's crown.
		Pipe->SetRelativeLocation(FVector(-10.0, 0.0, 450.0));
		Pipe->SetRelativeScale3D(FVector(0.45, 0.45, 9.0));
	}
	Pipe->SetCollisionProfileName(TEXT("BlockAll"));
	Pipe->SetCollisionObjectType(ECC_WorldStatic);
	Pipe->SetCanEverAffectNavigation(false);
	Pipe->SetCastShadow(false);

	// The hand wheel on the bonnet's stem (the model's Wheel socket): the kit's brass wheel, else a brass disc.
	bWheelModel = WheelMesh != nullptr;
	Wheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wheel"));
	Wheel->SetupAttachment(Root);
	Wheel->SetMobility(EComponentMobility::Movable);
	Wheel->SetStaticMesh(bWheelModel ? WheelMesh : CylinderMesh);
	if (!bWheelModel)
	{
		Wheel->SetMaterial(0, Brass);
		Wheel->SetRelativeScale3D(FVector(0.9, 0.9, 0.08));
	}
	Wheel->SetRelativeLocation(WheelAt);
	Wheel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Wheel->SetCanEverAffectNavigation(false);
	Wheel->SetCastShadow(false);

	// With the valve model, its riser on up to the hall's ceiling in pipe sections (they block, as the stand-in's pipe did).
	Riser = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Riser"));
	Riser->SetupAttachment(Root);
	Riser->SetMobility(EComponentMobility::Movable);
	Riser->SetStaticMesh(ValveMesh ? PipeMesh : nullptr);
	Riser->SetCollisionProfileName(TEXT("BlockAll"));
	Riser->SetCollisionObjectType(ECC_WorldStatic);
	Riser->SetCanEverAffectNavigation(false);
	Riser->SetCastShadow(false);

	Prompt = LOCTEXT("Shut", "Shut the draft valve");
	PoseWheel();
}

void AFurnaceDraftValve::BeginPlay()
{
	Super::BeginPlay();
	if (Riser->GetStaticMesh())
	{
		TArray<FTransform> Runs;
		BargeLooks::AddPipeRun(Runs, RiserFoot, BargeRules::HallHeight - RiserFoot.Z, BargeRules::ValveRiserWidth);
		Riser->ClearInstances();
		Riser->AddInstances(Runs, /*bShouldReturnIndices*/ false);
	}
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
}

void AFurnaceDraftValve::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

bool AFurnaceDraftValve::StoryHolds(const FStoryCondition& Condition) const
{
	if (Condition.IsEmpty())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return !Runner || Condition.IsMet(Runner->GetCampaign(), Runner);
}

bool AFurnaceDraftValve::CanShut() const
{
	return !bShut && StoryHolds(ShutWhen);
}

bool AFurnaceDraftValve::Shut(AActor* By, bool bForce)
{
	if (bShut || (!bForce && !CanShut()))
	{
		return false;
	}
	bShut = true;
	Turn = 0.f;
	SteamWait = 0.f;
	SetActorTickEnabled(true);
	// The wheel's iron squeal, the pipe's last breath venting, and the hall's light faltering with it.
	const FVector At = GetActorTransform().TransformPosition(WheelAt);
	LooterSound::PlayAt(this, LooterSoundCue::Fittings::Crank, At, 1.f, 0.85f);
	LooterSound::PlayAt(this, LooterSoundCue::Hero::GaugeVent, At + FVector(0.0, 0.0, 120.0), 1.2f, 0.7f);
	LamplighterEffects::Shake(this, 0.18f, 0.4f);
	UE_LOG(LogLooter, Log, TEXT("%s: a draft valve is shut."), *GetActorNameOrLabel());
	if (ABargeBoarding* Boarding = Barge.Get())
	{
		Boarding->NoteValveShut(*this);
	}
	return true;
}

void AFurnaceDraftValve::SetShutNow(bool bInShut)
{
	bShut = bInShut;
	Turn = bInShut ? 1.f : 0.f;
	SetActorTickEnabled(false);
	PoseWheel();
}

void AFurnaceDraftValve::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Turn = FMath::Min(Turn + DeltaSeconds / SpinSeconds, 1.f);
	PoseWheel();
	SteamWait -= DeltaSeconds;
	if (SteamWait <= 0.f)
	{
		SteamWait = SteamEvery;
		LamplighterEffects::GasPuff(this, GetActorTransform().TransformPosition(FVector(20.0, 0.0, 260.0)),
			GetActorForwardVector() + FVector(0.0, 0.0, 0.6));
	}
	if (Turn >= 1.f)
	{
		// Seated: a clank, and the pipe goes quiet.
		LooterSound::PlayAt(this, LooterSoundCue::ImpactMetal, GetActorTransform().TransformPosition(WheelAt), 0.9f, 0.7f);
		SetActorTickEnabled(false);
	}
}

void AFurnaceDraftValve::PoseWheel()
{
	if (!Wheel)
	{
		return;
	}
	// Slowing as it seats (an ease out over its turns), spun about its axle into the hall (+X): the model's axle is its own X;
	// the stand-in disc's is its Z, turned onto X.
	const float Eased = 1.f - FMath::Square(1.f - Turn);
	const FQuat Spin(FVector::XAxisVector, UE_TWO_PI * SpinTurns * Eased);
	Wheel->SetRelativeRotation(bWheelModel ? Spin : Spin * FQuat(FVector::YAxisVector, UE_HALF_PI));
}

FInteractionOptions AFurnaceDraftValve::GetInteractionOptions(const UInteractionComponent& User) const
{
	if (!CanShut())
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

bool AFurnaceDraftValve::Interact(UInteractionComponent& User, bool bHeld)
{
	return bHeld && Shut(User.GetOwner());
}

TOptional<FVector> AFurnaceDraftValve::GetInteractionLocation() const
{
	return TOptional<FVector>(GetActorTransform().TransformPosition(WheelAt));
}

TOptional<FInteractionBox> AFurnaceDraftValve::GetInteractionBox() const
{
	return InteractionFocus::BoxAround(GetActorTransform().TransformPosition(WheelAt), FVector(40.0, 70.0, 70.0), GetActorQuat());
}

#undef LOCTEXT_NAMESPACE

// Out of line: the header only forward-declares the barge, and the weak pointer needs its full type to convert.
void AFurnaceDraftValve::SetBarge(ABargeBoarding* InBarge)
{
	Barge = InBarge;
}

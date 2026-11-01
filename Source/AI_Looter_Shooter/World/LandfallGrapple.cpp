// ALandfallGrapple: one of the tender's grapples: its flight, its bite in the Lamplighter's rail, the cut and its fall.

#include "World/LandfallGrapple.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Interaction/InteractionFocus.h"
#include "Interaction/InteractionSubsystem.h"
#include "Player/ViewKick.h"
#include "World/CompanyTender.h"
#include "World/LamplighterEffects.h"
#include "World/LamplighterRules.h"
#include "World/LamplighterShip.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "LandfallGrapple"

namespace
{
	// The stand-in's look, one path each (the Tripo grapple hook replaces the shaft and prongs).
	const TCHAR* CylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* IronPath = TEXT("/Game/Art/Materials/MI_IronBlack.MI_IronBlack");

	/** The hook's size (cm): a 60 cm shaft, three 45 cm prongs splayed 35 degrees; the line 5 cm thick. */
	constexpr float ShaftLength = 60.f;
	constexpr float ShaftWidth = 9.f;
	constexpr float ProngLength = 45.f;
	constexpr float ProngWidth = 6.f;
	constexpr float ProngSplay = 35.f;
	constexpr float LineWidth = 5.f;
	/** The engine cylinder is 100 cm tall and across. */
	constexpr float CylinderSize = 100.f;
	/** How high the grapple's flight arcs over its straight line (cm). */
	constexpr float FlightArc = 80.f;
	/** A cut hook tears off outward and up, then falls. */
	constexpr float TearOut = 350.f;
	constexpr float TearUp = 250.f;

	/** Query-only for the crosshair's line: it stops no one and nothing else. */
	void MakeLookable(UStaticMeshComponent& Part)
	{
		Part.SetMobility(EComponentMobility::Movable);
		Part.SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Part.SetCollisionObjectType(ECC_WorldDynamic);
		Part.SetCollisionResponseToAllChannels(ECR_Ignore);
		Part.SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Part.SetGenerateOverlapEvents(false);
		Part.SetCanEverAffectNavigation(false);
	}
}

ALandfallGrapple::ALandfallGrapple()
{
	// The Lamplighter moves it on right after herself (ALamplighterShip::AdvancePieces), so its line never lags her.
	PrimaryActorTick.bCanEverTick = false;
	Tags.Add(LandfallRules::GrappleTag());
	SetCanBeDamaged(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(CylinderPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Iron(IronPath);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// The shaft lies back along the line (-X) from the crown at the root; the engine cylinder stands on Z, pitched onto X.
	Shaft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shaft"));
	Shaft->SetupAttachment(Root);
	Shaft->SetRelativeLocationAndRotation(FVector(-ShaftLength * 0.5f, 0.0, 0.0), FRotator(90.0, 0.0, 0.0));
	Shaft->SetRelativeScale3D(FVector(ShaftWidth, ShaftWidth, ShaftLength) / CylinderSize);
	MakeLookable(*Shaft);

	for (int32 Index = 0; Index < 3; ++Index)
	{
		// Three prongs round the crown, each splayed forward and out, a third of a turn apart.
		const double Around = FMath::DegreesToRadians(120.0 * Index + 90.0);
		const double Splay = FMath::DegreesToRadians(static_cast<double>(ProngSplay));
		const FVector Along = FVector(FMath::Cos(Splay), FMath::Sin(Splay) * FMath::Cos(Around), FMath::Sin(Splay) * FMath::Sin(Around));
		UStaticMeshComponent* Prong = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Prong%d"), Index));
		Prong->SetupAttachment(Root);
		Prong->SetRelativeLocationAndRotation(Along * (ProngLength * 0.5f), FRotationMatrix::MakeFromZ(Along).Rotator());
		Prong->SetRelativeScale3D(FVector(ProngWidth, ProngWidth, ProngLength) / CylinderSize);
		MakeLookable(*Prong);
		Prongs.Add(Prong);
	}

	// The line is placed in the world every frame, between the hook's eye and the tender, whatever the hook's turn.
	Line = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Line"));
	Line->SetupAttachment(Root);
	Line->SetUsingAbsoluteLocation(true);
	Line->SetUsingAbsoluteRotation(true);
	Line->SetUsingAbsoluteScale(true);
	Line->SetMobility(EComponentMobility::Movable);
	Line->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Line->SetCastShadow(false);
	Line->SetCanEverAffectNavigation(false);

	TInlineComponentArray<UStaticMeshComponent*> Parts;
	Parts.Add(Shaft);
	Parts.Append(Prongs);
	Parts.Add(Line);
	for (UStaticMeshComponent* Part : Parts)
	{
		Part->SetStaticMesh(Cylinder.Object);
		Part->SetMaterial(0, Iron.Object);
	}

	CutPrompt = LOCTEXT("Cut", "Cut the grapple line");
}

ALandfallGrapple* ALandfallGrapple::Fire(ALamplighterShip& ForShip, ACompanyTender& FromTender, int32 Launcher, const FVector& InBiteLocal)
{
	UWorld* World = ForShip.GetWorld();
	if (!World)
	{
		return nullptr;
	}
	const FVector From = FromTender.GetLauncher(Launcher);
	const FVector Target = ForShip.GetActorTransform().TransformPosition(InBiteLocal);
	ALandfallGrapple* Grapple = World->SpawnActorDeferred<ALandfallGrapple>(ALandfallGrapple::StaticClass(),
		FTransform(FRotationMatrix::MakeFromX(Target - From).Rotator(), From), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Grapple)
	{
		return nullptr;
	}
	Grapple->Ship = &ForShip;
	Grapple->Tender = &FromTender;
	Grapple->LauncherIndex = Launcher;
	Grapple->BiteLocal = InBiteLocal;
	Grapple->FlightFrom = From;
	Grapple->FinishSpawning(FTransform(FRotationMatrix::MakeFromX(Target - From).Rotator(), From));
	LooterSound::PlayAt(&ForShip, LooterSoundCue::GrappleFire, From);
	LamplighterEffects::Sparks(&ForShip, From, Target - From, 4, 1.2f);
	return Grapple;
}

void ALandfallGrapple::BeginPlay()
{
	Super::BeginPlay();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
	PlaceLine(GetEye(), GetLineTail());
}

void ALandfallGrapple::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	LooterSound::Stop(StrainSound.Get());
	StrainSound.Reset();
	Super::EndPlay(EndPlayReason);
}

void ALandfallGrapple::Advance(float DeltaSeconds)
{
	Clock += DeltaSeconds;
	const ALamplighterShip* Boat = Ship.Get();
	switch (State)
	{
	case ELandfallGrappleState::Flying:
	{
		if (!Boat)
		{
			Destroy();
			return;
		}
		const FVector Target = Boat->GetActorTransform().TransformPosition(BiteLocal);
		const float Alpha = FMath::Clamp(Clock / FlightSeconds, 0.f, 1.f);
		const FVector Here = FMath::Lerp(FlightFrom, Target, static_cast<double>(Alpha))
			+ FVector(0.0, 0.0, FlightArc * FMath::Sin(UE_PI * Alpha));
		SetActorLocationAndRotation(Here, FRotationMatrix::MakeFromX(Target - FlightFrom).Rotator());
		if (Alpha >= 1.f)
		{
			Bite();
		}
		break;
	}
	case ELandfallGrappleState::Biting:
		break;
	case ELandfallGrappleState::Cut:
		// Torn off the rail: it falls, turning over, dragged on its line after the tender.
		FallVelocity.Z -= 980.f * DeltaSeconds;
		AddActorWorldOffset(FallVelocity * DeltaSeconds);
		AddActorWorldRotation(FRotator(FallSpin.Y, FallSpin.Z, FallSpin.X) * DeltaSeconds);
		if (Clock >= FallSeconds)
		{
			Destroy();
			return;
		}
		break;
	}
	PlaceLine(GetEye(), GetLineTail());
}

void ALandfallGrapple::Bite()
{
	ALamplighterShip* Boat = Ship.Get();
	if (State != ELandfallGrappleState::Flying || !Boat)
	{
		return;
	}
	const FVector Target = Boat->GetActorTransform().TransformPosition(BiteLocal);
	SetActorLocationAndRotation(Target, FRotationMatrix::MakeFromX(Target - FlightFrom).Rotator());
	if (USceneComponent* Deck = Boat->GetDeck())
	{
		AttachToComponent(Deck, FAttachmentTransformRules::KeepWorldTransform);
	}
	State = ELandfallGrappleState::Biting;
	Clock = 0.f;
	// The bite: iron into the rail, a clang and a spray of sparks back along the line, a jolt the player feels.
	LooterSound::PlayAt(this, LooterSoundCue::GrappleHit, Target);
	LamplighterEffects::Sparks(this, Target, FlightFrom - Target, 10, 1.4f);
	LamplighterEffects::Shake(this, 0.35f, 0.4f);
	StrainSound = LooterSound::Start(this, LooterSoundCue::GrappleStrain, Root);
	Boat->NotifyGrappleBit(*this);
	PlaceLine(GetEye(), GetLineTail());
}

bool ALandfallGrapple::Cut()
{
	ALamplighterShip* Boat = Ship.Get();
	if (State != ELandfallGrappleState::Biting || !Boat || !Boat->CanCutGrapples())
	{
		return false;
	}
	State = ELandfallGrappleState::Cut;
	Clock = 0.f;
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	const FVector Here = GetActorLocation();
	FVector Out = GetLineTail() - Here;
	Out.Z = 0.0;
	Out = Out.GetSafeNormal(UE_SMALL_NUMBER, -GetActorForwardVector());
	FallVelocity = Out * TearOut + FVector(0.0, 0.0, TearUp);
	FallSpin = FVector(FMath::FRandRange(-300.f, 300.f), FMath::FRandRange(-200.f, 200.f), FMath::FRandRange(-250.f, 250.f));
	// The line parts: a crack, a burst of sparks off the rail, the view jolted.
	LooterSound::PlayAt(this, LooterSoundCue::GrappleCut, Here);
	LooterSound::Stop(StrainSound.Get(), 0.05f);
	StrainSound.Reset();
	LamplighterEffects::Sparks(this, Here, Out + FVector(0.0, 0.0, 0.6), 14, 1.6f);
	FViewKick Jolt;
	Jolt.Pitch = 1.2f;
	Jolt.Roll = FMath::FRandRange(-0.8f, 0.8f);
	Jolt.Frequency = 9.f;
	LamplighterEffects::Kick(this, Jolt);
	UE_LOG(LogLooter, Log, TEXT("Landfall: a grapple line is cut."));
	Boat->NotifyGrappleCut(*this);
	return true;
}

FInteractionOptions ALandfallGrapple::GetInteractionOptions(const UInteractionComponent& User) const
{
	const ALamplighterShip* Boat = Ship.Get();
	if (State != ELandfallGrappleState::Biting || !Boat || !Boat->CanCutGrapples())
	{
		return FInteractionOptions::None();
	}
	FInteractionOptions Options;
	Options.bTap = false;
	Options.bHold = true;
	Options.HoldSeconds = HoldSeconds;
	Options.HoldPrompt = CutPrompt;
	return Options;
}

bool ALandfallGrapple::Interact(UInteractionComponent& User, bool bHeld)
{
	return bHeld && Cut();
}

TOptional<FVector> ALandfallGrapple::GetInteractionLocation() const
{
	return TOptional<FVector>(GetActorLocation());
}

TOptional<FInteractionBox> ALandfallGrapple::GetInteractionBox() const
{
	// The hook from the eye of its shaft to the tips of its prongs, with a hand's room round it.
	return InteractionFocus::BoxAround(GetActorTransform().TransformPosition(FVector(-10.0, 0.0, 0.0)), FVector(48.0, 32.0, 32.0), GetActorQuat());
}

FVector ALandfallGrapple::GetEye() const
{
	return GetActorTransform().TransformPositionNoScale(FVector(-ShaftLength - 2.f, 0.0, 0.0));
}

FVector ALandfallGrapple::GetLineTail() const
{
	if (const ACompanyTender* Launching = Tender.Get())
	{
		return Launching->GetLauncher(LauncherIndex);
	}
	// The tender's gone: the line hangs from the hook.
	return GetEye() - FVector(0.0, 0.0, 400.0);
}

void ALandfallGrapple::PlaceLine(const FVector& Head, const FVector& Tail)
{
	const FVector Span = Tail - Head;
	const double Length = Span.Size();
	if (!Line || Length < 1.0)
	{
		return;
	}
	Line->SetWorldLocationAndRotation((Head + Tail) * 0.5, FRotationMatrix::MakeFromZ(Span).Rotator());
	Line->SetWorldScale3D(FVector(LineWidth / CylinderSize, LineWidth / CylinderSize, Length / CylinderSize));
}

#undef LOCTEXT_NAMESPACE

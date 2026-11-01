// ALandfallSpyglass: the spyglass at the lookout: Interact plays the smog reveal through it.

#include "World/LandfallSpyglass.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaTravelSubsystem.h"
#include "Interaction/InteractionFocus.h"
#include "Interaction/InteractionSubsystem.h"
#include "Scenes/SceneSubsystem.h"
#include "Scenes/SmogReveal.h"
#include "Session/CampaignRecord.h"
#include "World/LamplighterRules.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "LandfallSpyglass"

namespace
{
	// The stand-in's look, one path each.
	const TCHAR* CylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* BrassPath = TEXT("/Game/Art/Materials/MI_BrassWorn.MI_BrassWorn");
	const TCHAR* WoodPath = TEXT("/Game/Art/Materials/MI_LanternWood.MI_LanternWood");

	/** Its size (cm): the mount at eye height on three splayed legs, a 110 cm tube 9 across, its eyepiece behind. */
	constexpr float MountHeight = 140.f;
	constexpr float TubeLength = 110.f;
	constexpr float TubeWidth = 9.f;
	constexpr float LegSplay = 18.f;
	constexpr float LegWidth = 4.f;
	/** The tube sits forward of the mount by this much (the eyepiece behind it at the player's eye). */
	constexpr float TubeAhead = 30.f;
}

ALandfallSpyglass::ALandfallSpyglass()
{
	PrimaryActorTick.bCanEverTick = false;
	Tags.Add(LandfallRules::SpyglassTag());
	SetCanBeDamaged(false);
	BargeTag = LandfallRules::BargeTag();
	LookPrompt = LOCTEXT("Look", "Look through the spyglass");

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(CylinderPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Brass(BrassPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Wood(WoodPath);

	// Movable: the tube turns toward the barge as play begins.
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	Mount = CreateDefaultSubobject<USceneComponent>(TEXT("Mount"));
	Mount->SetupAttachment(Root);
	Mount->SetMobility(EComponentMobility::Movable);
	Mount->SetRelativeLocation(FVector(0.0, 0.0, MountHeight));

	// The tube along the mount's +X (the engine cylinder stands on Z: pitched onto X).
	Tube = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tube"));
	Tube->SetupAttachment(Mount);
	Tube->SetStaticMesh(Cylinder.Object);
	Tube->SetMaterial(0, Brass.Object);
	Tube->SetRelativeLocationAndRotation(FVector(TubeAhead, 0.0, 0.0), FRotator(90.0, 0.0, 0.0));
	Tube->SetRelativeScale3D(FVector(TubeWidth, TubeWidth, TubeLength) / 100.0);
	Tube->SetMobility(EComponentMobility::Movable);
	Tube->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Tube->SetCanEverAffectNavigation(false);

	Eyepiece = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Eyepiece"));
	Eyepiece->SetupAttachment(Mount);
	Eyepiece->SetStaticMesh(Cylinder.Object);
	Eyepiece->SetMaterial(0, Brass.Object);
	Eyepiece->SetRelativeLocationAndRotation(FVector(TubeAhead - TubeLength * 0.5 - 6.0, 0.0, 0.0), FRotator(90.0, 0.0, 0.0));
	Eyepiece->SetRelativeScale3D(FVector(0.05, 0.05, 0.14));
	Eyepiece->SetMobility(EComponentMobility::Movable);
	Eyepiece->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Eyepiece->SetCollisionResponseToAllChannels(ECR_Ignore);
	Eyepiece->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Eyepiece->SetCanEverAffectNavigation(false);

	for (int32 Index = 0; Index < 3; ++Index)
	{
		// Three legs splayed a third of a turn apart, from the mount down to the ground.
		const double Around = FMath::DegreesToRadians(120.0 * Index);
		const double Splay = FMath::DegreesToRadians(static_cast<double>(LegSplay));
		const FVector Down = FVector(FMath::Sin(Splay) * FMath::Cos(Around), FMath::Sin(Splay) * FMath::Sin(Around), -FMath::Cos(Splay));
		const float Length = MountHeight / FMath::Cos(Splay);
		UStaticMeshComponent* Leg = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Leg%d"), Index));
		Leg->SetupAttachment(Root);
		Leg->SetStaticMesh(Cylinder.Object);
		Leg->SetMaterial(0, Wood.Object);
		Leg->SetRelativeLocationAndRotation(FVector(0.0, 0.0, MountHeight) + Down * (Length * 0.5), FRotationMatrix::MakeFromZ(Down).Rotator());
		Leg->SetRelativeScale3D(FVector(LegWidth, LegWidth, Length) / 100.0);
		Leg->SetMobility(EComponentMobility::Movable);
		Leg->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Leg->SetCanEverAffectNavigation(false);
		Legs.Add(Leg);
	}
}

void ALandfallSpyglass::BeginPlay()
{
	Super::BeginPlay();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
	if (const AActor* Barge = FindBarge())
	{
		AimAt(Barge->GetActorLocation());
	}
}

void ALandfallSpyglass::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

AActor* ALandfallSpyglass::FindBarge() const
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(BargeTag))
		{
			return *It;
		}
	}
	return nullptr;
}

void ALandfallSpyglass::AimAt(const FVector& Target)
{
	const FVector Toward = Target - Mount->GetComponentLocation();
	if (!Toward.IsNearlyZero())
	{
		Mount->SetWorldRotation(Toward.Rotation());
	}
}

FVector ALandfallSpyglass::GetLens() const
{
	return Mount->GetComponentTransform().TransformPositionNoScale(FVector(TubeAhead + TubeLength * 0.5 + 12.0, 0.0, 0.0));
}

bool ALandfallSpyglass::Look()
{
	USceneSubsystem* Scenes = USceneSubsystem::Get(this);
	if (!Scenes || Scenes->IsPlaying())
	{
		return false;
	}
	FCampaignRecord* Campaign = UAreaTravelSubsystem::FindCampaign(GetWorld());
	const bool bFull = !Campaign || !Campaign->HasFlag(LandfallRules::RevealedFlag());
	AActor* Barge = FindBarge();
	if (Barge)
	{
		AimAt(Barge->GetActorLocation());
	}
	if (!Scenes->Play(SmogReveal::Make(*Scenes, *this, Barge, bFull)))
	{
		// Scenes off: nothing to see, but the look counts (the missions heard it played).
		return !USceneSubsystem::AreScenesOn(GetWorld());
	}
	if (Campaign && bFull)
	{
		Campaign->SetFlag(LandfallRules::RevealedFlag());
	}
	return true;
}

FInteractionOptions ALandfallSpyglass::GetInteractionOptions(const UInteractionComponent& User) const
{
	const USceneSubsystem* Scenes = USceneSubsystem::Get(this);
	if (Scenes && Scenes->IsPlaying())
	{
		return FInteractionOptions::None();
	}
	FInteractionOptions Options;
	Options.bTap = true;
	Options.bHold = false;
	Options.TapPrompt = LookPrompt;
	return Options;
}

bool ALandfallSpyglass::Interact(UInteractionComponent& User, bool bHeld)
{
	return Look();
}

TOptional<FVector> ALandfallSpyglass::GetInteractionLocation() const
{
	return TOptional<FVector>(Mount->GetComponentLocation());
}

TOptional<FInteractionBox> ALandfallSpyglass::GetInteractionBox() const
{
	// The tube and its eyepiece, with room round them.
	return InteractionFocus::BoxAround(Mount->GetComponentTransform().TransformPositionNoScale(FVector(TubeAhead * 0.5, 0.0, 0.0)),
		FVector(TubeLength * 0.6, 22.0, 22.0), Mount->GetComponentQuat());
}

#undef LOCTEXT_NAMESPACE

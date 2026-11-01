// ALamplighterLauncher: one of Junie's two grapple launchers on the Lamplighter's starboard rail (Main 6 step 2): aimed at
// the barge's rail while she's alongside, fired with a held Interact, its hook biting the barge and its chain hauling the two
// ships together; cut loose in the escape.

#include "World/LamplighterLauncher.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Interaction/InteractionFocus.h"
#include "Interaction/InteractionSubsystem.h"
#include "Player/ViewKick.h"
#include "World/LamplighterEffects.h"
#include "World/LamplighterShip.h"
#include "World/PilotSortieRules.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "LamplighterLauncher"

namespace
{
	// The stand-in's look, one path each (the Tripo grapple launcher replaces them: TRIPO_MODELS.md 37).
	const TCHAR* CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* CylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* ConePath = TEXT("/Engine/BasicShapes/Cone.Cone");
	const TCHAR* IronPath = TEXT("/Game/Art/Materials/MI_IronBlack.MI_IronBlack");
	const TCHAR* WoodPath = TEXT("/Game/Art/Materials/MI_LanternWood.MI_LanternWood");

	/** Its size (cm): a 55 cm timber block, the barrel 1.2 m long and 26 cm across turning on its top. */
	constexpr float BlockHeight = 55.f;
	constexpr float BarrelLength = 120.f;
	constexpr float BarrelWidth = 26.f;
	constexpr float ChainWidth = 6.f;
	/** At rest it points out over her starboard rail, this far up (degrees). */
	constexpr float RestElevation = 12.f;
	/** It turns to its aim this fast (a share a second, eased), and aims only at a rail this near (cm). */
	constexpr float AimRate = 3.f;
	constexpr float AimReach = 8000.f;
	/** The barrel's kick (cm) and its spring home. */
	constexpr float Kick = 28.f;
	constexpr float Spring = 180.f;
	constexpr float Damping = 18.f;
	/** A cut hook falls this long before it's put away. */
	constexpr float CutFallSeconds = 2.5f;
	constexpr float Gravity = 980.f;
	/** Its three prongs: this far round the shaft's tip, pointing back and out. */
	constexpr float ProngOut = 12.f;

	void MakeDressing(UStaticMeshComponent& Mesh)
	{
		Mesh.SetMobility(EComponentMobility::Movable);
		Mesh.SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh.SetCanEverAffectNavigation(false);
		Mesh.SetCastShadow(false);
	}
}

ALamplighterLauncher::ALamplighterLauncher()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);
	Tags.Add(PilotLight::LauncherTag());

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(CubePath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(CylinderPath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(ConePath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Iron(IronPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Wood(WoodPath);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	Mount = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mount"));
	Mount->SetupAttachment(Root);
	Mount->SetStaticMesh(Cube.Object);
	Mount->SetMaterial(0, Wood.Object);
	Mount->SetRelativeLocation(FVector(0.0, 0.0, BlockHeight * 0.5));
	Mount->SetRelativeScale3D(FVector(0.55, 0.5, BlockHeight / 100.0));
	MakeDressing(*Mount);
	// Solid to walk round (world dynamic: the creatures' ground traces never take it for a floor).
	Mount->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Mount->SetCollisionObjectType(ECC_WorldDynamic);

	// The barrel is placed in the world's frame each frame (it turns to aim and kicks back).
	Barrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Barrel"));
	Barrel->SetupAttachment(Root);
	Barrel->SetStaticMesh(Cylinder.Object);
	Barrel->SetMaterial(0, Iron.Object);
	Barrel->SetUsingAbsoluteLocation(true);
	Barrel->SetUsingAbsoluteRotation(true);
	Barrel->SetUsingAbsoluteScale(true);
	Barrel->SetWorldScale3D(FVector(BarrelWidth / 100.0, BarrelWidth / 100.0, BarrelLength / 100.0));
	MakeDressing(*Barrel);
	// The crosshair finds it (the Interact key's line), nothing else does.
	Barrel->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Barrel->SetCollisionResponseToAllChannels(ECR_Ignore);
	Barrel->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	Hook = CreateDefaultSubobject<USceneComponent>(TEXT("Hook"));
	Hook->SetupAttachment(Root);
	Hook->SetUsingAbsoluteLocation(true);
	Hook->SetUsingAbsoluteRotation(true);
	Hook->SetMobility(EComponentMobility::Movable);
	UStaticMeshComponent* Shaft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HookShaft"));
	Shaft->SetupAttachment(Hook);
	Shaft->SetStaticMesh(Cylinder.Object);
	Shaft->SetMaterial(0, Iron.Object);
	// Along the hook's +X, behind its tip.
	Shaft->SetRelativeLocationAndRotation(FVector(-25.0, 0.0, 0.0), FRotator(-90.0, 0.0, 0.0));
	Shaft->SetRelativeScale3D(FVector(0.08, 0.08, 0.5));
	MakeDressing(*Shaft);
	HookParts.Add(Shaft);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UStaticMeshComponent* Prong = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("HookProng%d"), Index));
		Prong->SetupAttachment(Hook);
		Prong->SetStaticMesh(Cone.Object);
		Prong->SetMaterial(0, Iron.Object);
		const double Angle = UE_TWO_PI * Index / 3.0;
		const FVector Round(0.0, FMath::Cos(Angle), FMath::Sin(Angle));
		// Back and out from the tip, as a grapple's flukes.
		Prong->SetRelativeLocationAndRotation(FVector(-6.0, 0.0, 0.0) + Round * ProngOut,
			FRotationMatrix::MakeFromZ(FVector(-0.55, 0.0, 0.0) + Round * 0.85).Rotator());
		Prong->SetRelativeScale3D(FVector(0.1, 0.1, 0.28));
		MakeDressing(*Prong);
		HookParts.Add(Prong);
	}
	Hook->SetVisibility(false, /*bPropagateToChildren*/ true);

	Chain = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Chain"));
	Chain->SetupAttachment(Root);
	Chain->SetStaticMesh(Cylinder.Object);
	Chain->SetMaterial(0, Iron.Object);
	Chain->SetUsingAbsoluteLocation(true);
	Chain->SetUsingAbsoluteRotation(true);
	Chain->SetUsingAbsoluteScale(true);
	MakeDressing(*Chain);
	Chain->SetVisibility(false);

	FirePrompt = LOCTEXT("Fire", "Fire the grapple");
}

ALamplighterLauncher* ALamplighterLauncher::SpawnOn(ALamplighterShip& Ship, const FVector& DeckLocal)
{
	UWorld* World = Ship.GetWorld();
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = &Ship;
	const FVector At = Ship.DeckPoint(DeckLocal);
	ALamplighterLauncher* Launcher = World->SpawnActor<ALamplighterLauncher>(At, Ship.GetActorRotation(), Params);
	if (Launcher)
	{
		// Fixed to her deck: it rides her wherever she flies.
		if (UPrimitiveComponent* Deck = Ship.GetDeck())
		{
			Launcher->AttachToComponent(Deck, FAttachmentTransformRules::KeepWorldTransform);
		}
		Launcher->Advance(0.f, FVector::ZeroVector);
	}
	return Launcher;
}

void ALamplighterLauncher::BeginPlay()
{
	Super::BeginPlay();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
}

void ALamplighterLauncher::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	LooterSound::Stop(StrainSound.Get(), 0.2f);
	StrainSound.Reset();
	Super::EndPlay(EndPlayReason);
}

FVector ALamplighterLauncher::GetPivot() const
{
	return GetActorTransform().TransformPosition(FVector(0.0, 0.0, BlockHeight + BarrelWidth * 0.5));
}

FVector ALamplighterLauncher::GetMuzzle() const
{
	return GetPivot() + Aim.GetUpVector() * (BarrelLength - Recoil);
}

FVector ALamplighterLauncher::GetHookLocation() const
{
	return Hook->GetComponentLocation();
}

void ALamplighterLauncher::AimAt(const FVector& Target, float DeltaSeconds)
{
	// At rest out over her starboard rail, a little up; turned to the barge's rail once it's within reach.
	const FVector Rest = GetActorRightVector() * FMath::Cos(FMath::DegreesToRadians(RestElevation))
		+ GetActorUpVector() * FMath::Sin(FMath::DegreesToRadians(RestElevation));
	FVector Want = Rest;
	if (!Target.IsNearlyZero() && FVector::Dist(Target, GetPivot()) <= AimReach)
	{
		Want = (Target - GetPivot()).GetSafeNormal();
	}
	const FQuat WantAim = FRotationMatrix::MakeFromZ(Want).ToQuat();
	Aim = bAimSet && DeltaSeconds > 0.f ? FQuat::Slerp(Aim, WantAim, FMath::Clamp(AimRate * DeltaSeconds, 0.f, 1.f)) : WantAim;
	bAimSet = true;
}

void ALamplighterLauncher::PlaceHook(const FVector& At, const FVector& Facing)
{
	const FVector Ahead = Facing.IsNearlyZero() ? GetActorRightVector() : Facing.GetSafeNormal();
	Hook->SetWorldLocationAndRotation(At, FRotationMatrix::MakeFromX(Ahead).Rotator());
}

void ALamplighterLauncher::PlaceChain(const FVector& From, const FVector& To)
{
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
}

void ALamplighterLauncher::Advance(float DeltaSeconds, const FVector& Anchor)
{
	if (!Anchor.IsNearlyZero())
	{
		LastAnchor = Anchor;
		bHasAnchor = true;
	}
	Clock += DeltaSeconds;
	// The barrel's kick springs home.
	if (DeltaSeconds > 0.f)
	{
		RecoilSpeed += (-Spring * Recoil - Damping * RecoilSpeed) * DeltaSeconds;
		Recoil = FMath::Max(Recoil + RecoilSpeed * DeltaSeconds, -4.f);
	}
	switch (State)
	{
	case ELauncherState::Ready:
		AimAt(bHasAnchor ? LastAnchor : FVector::ZeroVector, DeltaSeconds);
		break;
	case ELauncherState::Flying:
	{
		// The hook arcs across the gap to the rail (which moves with the barge: aimed at where it is now).
		BiteAt = bHasAnchor ? LastAnchor : BiteAt;
		const float A = FMath::Clamp(Clock / FlightSeconds, 0.f, 1.f);
		const FVector Line = FMath::Lerp(FlightFrom, BiteAt, static_cast<double>(A));
		const FVector At = Line + FVector(0.0, 0.0, ArcLift * FMath::Sin(UE_PI * A));
		PlaceHook(At, BiteAt - FlightFrom);
		AimAt(At, DeltaSeconds);
		if (A >= 1.f)
		{
			BiteNow(BiteAt);
			LooterSound::PlayAt(this, LooterSoundCue::GrappleHit, BiteAt);
			LamplighterEffects::Sparks(this, BiteAt, (FlightFrom - BiteAt).GetSafeNormal(), 10, 1.f);
			LamplighterEffects::Shake(this, 0.15f, 0.25f);
			UE_LOG(LogLooter, Log, TEXT("The Pilot Light: a grapple bites the barge's rail."));
		}
		break;
	}
	case ELauncherState::Biting:
		BiteAt = bHasAnchor ? LastAnchor : BiteAt;
		PlaceHook(BiteAt, BiteAt - GetPivot());
		AimAt(BiteAt, DeltaSeconds);
		break;
	case ELauncherState::Cut:
	{
		FallVelocity.Z -= Gravity * DeltaSeconds;
		const FVector At = Hook->GetComponentLocation() + FallVelocity * DeltaSeconds;
		PlaceHook(At, FallVelocity);
		if (Clock >= CutFallSeconds)
		{
			Hook->SetVisibility(false, true);
			Chain->SetVisibility(false);
		}
		break;
	}
	}
	const FVector Pivot = GetPivot();
	Barrel->SetWorldLocationAndRotation(Pivot + Aim.GetUpVector() * (BarrelLength * 0.5f - Recoil), Aim);
	if (State != ELauncherState::Ready && Hook->IsVisible())
	{
		PlaceChain(GetMuzzle(), Hook->GetComponentLocation());
	}
}

bool ALamplighterLauncher::Fire(const FVector& Anchor)
{
	if (!CanFire())
	{
		return false;
	}
	if (!Anchor.IsNearlyZero())
	{
		LastAnchor = Anchor;
		bHasAnchor = true;
	}
	if (!bHasAnchor)
	{
		return false;
	}
	State = ELauncherState::Flying;
	Clock = 0.f;
	BiteAt = LastAnchor;
	AimAt(BiteAt, 0.f);
	FlightFrom = GetMuzzle();
	Hook->SetVisibility(true, true);
	PlaceHook(FlightFrom, BiteAt - FlightFrom);
	// The report: a heavy thump, the barrel kicking back on its block, a puff of smoke and sparks at its mouth, felt on deck.
	Recoil = Kick;
	RecoilSpeed = 0.f;
	LooterSound::PlayAt(this, LooterSoundCue::GrappleFire, FlightFrom);
	LamplighterEffects::SmokePuff(this, FlightFrom, Aim.GetUpVector() * 160.0);
	LamplighterEffects::Sparks(this, FlightFrom, Aim.GetUpVector(), 6, 0.8f);
	FViewKick Jolt;
	Jolt.Pitch = -1.6f;
	Jolt.Frequency = 7.f;
	Jolt.Damping = 0.45f;
	LamplighterEffects::Kick(this, Jolt);
	LamplighterEffects::Shake(this, 0.2f, 0.3f);
	UE_LOG(LogLooter, Log, TEXT("The Pilot Light: a launcher fires its grapple at the barge."));
	return true;
}

void ALamplighterLauncher::BiteNow(const FVector& Anchor)
{
	if (!Anchor.IsNearlyZero())
	{
		LastAnchor = Anchor;
		bHasAnchor = true;
	}
	State = ELauncherState::Biting;
	Clock = 0.f;
	BiteAt = LastAnchor;
	Hook->SetVisibility(true, true);
	AimAt(BiteAt, 0.f);
	PlaceHook(BiteAt, BiteAt - GetPivot());
	PlaceChain(GetMuzzle(), BiteAt);
}

void ALamplighterLauncher::SetStraining(bool bInStraining)
{
	if (bInStraining && !StrainSound.IsValid() && State == ELauncherState::Biting)
	{
		StrainSound = LooterSound::Start(this, LooterSoundCue::GrappleStrain, Barrel);
	}
	else if (!bInStraining && StrainSound.IsValid())
	{
		LooterSound::Stop(StrainSound.Get(), 0.3f);
		StrainSound.Reset();
	}
}

bool ALamplighterLauncher::Cut()
{
	if (State != ELauncherState::Flying && State != ELauncherState::Biting)
	{
		return false;
	}
	SetStraining(false);
	State = ELauncherState::Cut;
	Clock = 0.f;
	FallVelocity = FVector(0.0, 0.0, -100.0);
	// Holloway's axe on the chain: it parts with a crack and sparks, and the hook tears free of the dying barge.
	LooterSound::PlayAt(this, LooterSoundCue::GrappleCut, GetMuzzle());
	LamplighterEffects::Sparks(this, GetMuzzle(), FVector::UpVector, 8, 1.f);
	return true;
}

void ALamplighterLauncher::Reload()
{
	SetStraining(false);
	State = ELauncherState::Ready;
	Clock = 0.f;
	Hook->SetVisibility(false, true);
	Chain->SetVisibility(false);
}

FInteractionOptions ALamplighterLauncher::GetInteractionOptions(const UInteractionComponent& User) const
{
	if (!CanFire())
	{
		return FInteractionOptions::None();
	}
	FInteractionOptions Options;
	Options.bTap = false;
	Options.bHold = true;
	Options.HoldSeconds = HoldSeconds;
	Options.HoldPrompt = FirePrompt;
	Options.Reach = 320.f;
	return Options;
}

bool ALamplighterLauncher::Interact(UInteractionComponent& User, bool bHeld)
{
	// The missions hear the held use of the tagged launcher (Main 6 step 2: two of them) from the player's component.
	return bHeld && Fire(FVector::ZeroVector);
}

TOptional<FVector> ALamplighterLauncher::GetInteractionLocation() const
{
	return TOptional<FVector>(GetPivot());
}

TOptional<FInteractionBox> ALamplighterLauncher::GetInteractionBox() const
{
	// The block and the barrel on it.
	return InteractionFocus::BoxAround(GetActorTransform().TransformPosition(FVector(0.0, 35.0, BlockHeight * 0.8)),
		FVector(45.0, 85.0, BlockHeight * 0.85), GetActorQuat());
}

#undef LOCTEXT_NAMESPACE

// ABargeFurnace: the barge's soul furnace, Abel's light beating in its cage, and the grate ring that heats round it.

#include "World/BargeFurnace.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/HealthComponent.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionFocus.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionRunner.h"
#include "Scenes/CageBurst.h"
#include "World/BargeBoarding.h"
#include "World/BargeLooks.h"
#include "World/BargeRules.h"
#include "World/KeepersLantern.h"
#include "World/LamplighterEffects.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"

#define LOCTEXT_NAMESPACE "BargeFurnace"

const FName ABargeFurnace::CageTag(TEXT("Furnace_SoulCage"));
const TCHAR* const ABargeFurnace::BodyPath = BargeLooks::TripoFurnace;
const TCHAR* const ABargeFurnace::GratePath = BargeLooks::TripoGrate;

namespace
{
	/** The heart's beat: a double beat a little over once a second, as a heart under strain. */
	constexpr float BeatHz = 1.1f;
	/** The heart's size (cm across) at rest and at its beat's peak; the smog's round it, out of step, so the black flares. */
	constexpr float HeartRest = 110.f;
	constexpr float HeartPeak = 150.f;
	constexpr float SmogRest = 120.f;
	constexpr float SmogPeak = 165.f;
	/** Where the heart hangs in front of the furnace's face (its frame). */
	const FVector HeartAt(330.0, 0.0, BargeRules::CageUp);
	const FLinearColor HeartColor(1.f, 0.86f, 0.6f);
	/** A player's feet within this of the floor stand on the ring. */
	constexpr float OnFloor = 80.f;

	float Beat(float Clock)
	{
		const float Phase = FMath::Frac(Clock * BeatHz);
		const auto Pulse = [](float X, float At) { return FMath::Exp(-FMath::Square(X - At) / 0.004f); };
		return FMath::Clamp(Pulse(Phase, 0.f) + Pulse(Phase, 1.f) + 0.6f * Pulse(Phase, 0.2f), 0.f, 1.f);
	}

	UStaticMeshComponent* MakeFurnacePart(AActor& Owner, USceneComponent* Parent, const TCHAR* Name, UStaticMesh* Mesh,
		UMaterialInterface* Look, const FVector& At, const FVector& Scale)
	{
		UStaticMeshComponent* Part = Owner.CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetStaticMesh(Mesh);
		if (Look)
		{
			Part->SetMaterial(0, Look);
		}
		Part->SetRelativeLocation(At);
		Part->SetRelativeScale3D(Scale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCanEverAffectNavigation(false);
		Part->SetCastShadow(false);
		return Part;
	}

	UInstancedStaticMeshComponent* MakeFurnaceInstances(AActor& Owner, USceneComponent* Parent, const TCHAR* Name, UStaticMesh* Mesh,
		UMaterialInterface* Look)
	{
		UInstancedStaticMeshComponent* Part = Owner.CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetStaticMesh(Mesh);
		if (Look)
		{
			Part->SetMaterial(0, Look);
		}
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCanEverAffectNavigation(false);
		Part->SetCastShadow(false);
		return Part;
	}
}

ABargeFurnace::ABargeFurnace()
{
	// The barge moves it on with herself (her tick): it never ticks on its own.
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);
	Tags.Add(CageTag);

	static UStaticMesh* const CubeMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cube);
	static UStaticMesh* const CylinderMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cylinder);
	static UStaticMesh* const SphereMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Sphere);
	static UStaticMesh* const ConeMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cone);
	static UStaticMesh* const TripoBody = BargeLooks::Find<UStaticMesh>(BodyPath);
	static UStaticMesh* const TripoWedge = BargeLooks::Find<UStaticMesh>(GratePath);
	static UMaterialInterface* const Iron = BargeLooks::Find<UMaterialInterface>(BargeLooks::Iron);
	static UMaterialInterface* const IronBlack = BargeLooks::Find<UMaterialInterface>(BargeLooks::IronBlack);
	static UMaterialInterface* const Glow = BargeLooks::Find<UMaterialInterface>(BargeLooks::Glow);
	static UMaterialInterface* const Soul = BargeLooks::Find<UMaterialInterface>(BargeLooks::Soul);
	static UMaterialInterface* const Smog = BargeLooks::Find<UMaterialInterface>(BargeLooks::Smog);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	const float D = BargeRules::FurnaceRadius * 2.f / 100.f;
	// The body: the Tripo furnace at its size (its pivot on the floor), else a riveted iron drum 6 m across and 6 m tall.
	Body = MakeFurnacePart(*this, Root, TEXT("Body"), TripoBody ? TripoBody : CylinderMesh, TripoBody ? nullptr : Iron,
		TripoBody ? FVector::ZeroVector : FVector(0.0, 0.0, BargeRules::FurnaceHeight * 0.5), TripoBody ? FVector::OneVector : FVector(D, D, BargeRules::FurnaceHeight / 100.f));
	// Its iron is what bullets and boots meet (a 6 m drum the fight goes round).
	Body->SetCollisionProfileName(TEXT("BlockAll"));
	Body->SetCollisionObjectType(ECC_WorldStatic);
	Crown = MakeFurnacePart(*this, Root, TEXT("Crown"), SphereMesh, IronBlack, FVector(0.0, 0.0, BargeRules::FurnaceHeight), FVector(D * 0.92f, D * 0.92f, 2.2f));
	Crown->SetVisibility(TripoBody == nullptr);
	Ironwork = MakeFurnaceInstances(*this, Root, TEXT("Ironwork"), CylinderMesh, IronBlack);
	CageFrame = MakeFurnacePart(*this, Root, TEXT("CageFrame"), CubeMesh, IronBlack, FVector(BargeRules::FurnaceRadius - 15.0, 0.0, BargeRules::CageUp),
		FVector(0.3, 2.1, 2.1));
	Heart = MakeFurnacePart(*this, Root, TEXT("Heart"), SphereMesh, Soul, HeartAt, FVector(HeartRest / 100.f));
	HeartSmog = MakeFurnacePart(*this, Root, TEXT("HeartSmog"), SphereMesh, Smog, HeartAt, FVector(SmogRest / 100.f));
	Shards = MakeFurnaceInstances(*this, Root, TEXT("Shards"), CubeMesh, IronBlack);

	CageLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CageLight"));
	CageLight->SetupAttachment(Root);
	CageLight->SetMobility(EComponentMobility::Movable);
	CageLight->SetRelativeLocation(HeartAt + FVector(60.0, 0.0, 0.0));
	CageLight->SetIntensityUnits(ELightUnits::Candelas);
	CageLight->SetIntensity(LitCandelas);
	CageLight->SetAttenuationRadius(LightRadius);
	CageLight->SetLightColor(HeartColor);
	// The hall's one light, never a shadow (the area keeps to three shadowless lights in view).
	CageLight->SetCastShadows(false);

	GrateCold = MakeFurnaceInstances(*this, Root, TEXT("GrateCold"), TripoWedge ? TripoWedge : CubeMesh, TripoWedge ? nullptr : IronBlack);
	GrateHot = MakeFurnaceInstances(*this, Root, TEXT("GrateHot"), TripoWedge ? TripoWedge : CubeMesh, Glow);
	GrateFire = MakeFurnaceInstances(*this, Root, TEXT("GrateFire"), ConeMesh, Glow);

	Prompt = LOCTEXT("Open", "Open the soul cage");
	NeedsLanternPrompt = LOCTEXT("NeedsLantern", "Abel's lantern is needed to carry him");
}

void ABargeFurnace::BeginPlay()
{
	Super::BeginPlay();
	// The pipes up into the hall's ceiling (from under its crown: never through the deck over it), the bands round the drum,
	// the cage's bars across its window.
	TArray<FTransform> Iron;
	const double PipeFoot = BargeRules::FurnaceHeight - BargeRules::PipeUnderCrown;
	const double PipeLength = BargeRules::HallHeight - PipeFoot;
	for (const FVector& Pipe : { FVector(-120.0, 150.0, 0.0), FVector(-120.0, -150.0, 0.0), FVector(-220.0, 0.0, 0.0) })
	{
		Iron.Add(FTransform(FQuat::Identity, Pipe + FVector(0.0, 0.0, PipeFoot + PipeLength * 0.5), FVector(0.55, 0.55, PipeLength / 100.0)));
	}
	const float D = BargeRules::FurnaceRadius * 2.f / 100.f + 0.12f;
	for (const float Up : { 60.f, 300.f, 540.f })
	{
		Iron.Add(FTransform(FQuat::Identity, FVector(0.0, 0.0, Up), FVector(D, D, 0.14)));
	}
	for (int32 Bar = -2; Bar <= 2; ++Bar)
	{
		Iron.Add(FTransform(FQuat::Identity, FVector(HeartAt.X + 70.0, Bar * 38.0, BargeRules::CageUp), FVector(0.07, 0.07, 1.9)));
	}
	Ironwork->ClearInstances();
	Ironwork->AddInstances(Iron, /*bShouldReturnIndices*/ false);
	LaidMask = 0xFFFFFFFFu;
	LayGrates(0u, false);
	SetCageBurst(bCageBurst);
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
}

void ABargeFurnace::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

bool ABargeFurnace::StoryHolds(const FStoryCondition& Condition) const
{
	if (Condition.IsEmpty())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return !Runner || Condition.IsMet(Runner->GetCampaign(), Runner);
}

// ---------------------------------------------------------------------------
// The heart, the light, the ring
// ---------------------------------------------------------------------------

void ABargeFurnace::Advance(float DeltaSeconds)
{
	HeartClock += DeltaSeconds;
	if (!bCageBurst && !bOut)
	{
		// The heart beats; the furnace's black flares round its edges out of step. Each valve shut weakens both.
		const float Strength = 1.f - 0.22f * ValvesShut;
		const float Pulse = Beat(HeartClock);
		const float Black = Beat(HeartClock + 0.45f / BeatHz);
		Heart->SetRelativeScale3D(FVector(FMath::Lerp(HeartRest, HeartPeak, Pulse) * Strength / 100.f));
		HeartSmog->SetRelativeScale3D(FVector(FMath::Lerp(SmogRest, SmogPeak, Black) * FMath::Max(Strength, 0.6f) / 100.f));
		CageLight->SetIntensity(LitCandelas * Strength * (0.7f + 0.3f * Pulse));
	}
	if (bRingRunning)
	{
		const bool bWasBurning = IsRingBurning();
		RingClock += DeltaSeconds;
		const bool bBurning = IsRingBurning();
		LayGrates(GetHotMask(), bBurning);
		if (bBurning && !bWasBurning && GetHotMask() != 0u)
		{
			// Each turn's wedges catch with a sear across the floor.
			LooterSound::PlayAt(this, LooterSoundCue::Spitter::Sizzle, GetActorLocation(), 0.7f, 0.8f);
		}
		HurtOnGrates(DeltaSeconds);
	}
}

void ABargeFurnace::SetValvesShut(int32 Shut)
{
	const int32 Was = ValvesShut;
	ValvesShut = FMath::Clamp(Shut, 0, BargeRules::NumValves);
	if (ValvesShut > Was && !bOut)
	{
		// A notch colder: a lurch of the flame through the window and a gout of sparks off the crown.
		LamplighterEffects::Sparks(this, GetActorTransform().TransformPosition(FVector(0.0, 0.0, BargeRules::FurnaceHeight)), FVector::UpVector, 8, 0.8f);
		LamplighterEffects::Shake(this, 0.25f, 0.5f);
	}
	if (ValvesShut >= BargeRules::NumValves)
	{
		SetRingRunning(false);
	}
	LayGrates(GetHotMask(), IsRingBurning());
}

void ABargeFurnace::SetRingRunning(bool bRunning)
{
	bRingRunning = bRunning && ValvesShut < BargeRules::NumValves && !bOut;
	if (!bRingRunning)
	{
		LayGrates(0u, false);
	}
}

uint32 ABargeFurnace::GetHotMask() const
{
	return bRingRunning ? BargeRules::HotMask(RingClock, ValvesShut) : 0u;
}

bool ABargeFurnace::IsRingBurning() const
{
	return bRingRunning && BargeRules::IsBurning(RingClock);
}

FTransform ABargeFurnace::WedgeTransform(int32 Wedge, float Up) const
{
	const float Angle = (Wedge + 0.5f) * 360.f / BargeRules::Grates;
	const float Middle = (BargeRules::GrateInner + BargeRules::GrateOuter) * 0.5f;
	const float Turn = FMath::DegreesToRadians(Angle);
	const FVector At(FMath::Cos(Turn) * Middle, FMath::Sin(Turn) * Middle, Up);
	// A plate the ring's depth long and its share of the middle circle wide, a little narrow so the wedges read apart.
	const float Width = UE_TWO_PI * Middle / BargeRules::Grates * 0.92f;
	return FTransform(FRotator(0.f, Angle, 0.f), At, FVector((BargeRules::GrateOuter - BargeRules::GrateInner) / 100.f, Width / 100.f, 0.12f));
}

void ABargeFurnace::LayGrates(uint32 Mask, bool bBurning)
{
	if (Mask == LaidMask && bBurning == bLaidBurning)
	{
		return;
	}
	LaidMask = Mask;
	bLaidBurning = bBurning;
	TArray<FTransform> Cold, Hot, Fire;
	for (int32 Wedge = 0; Wedge < BargeRules::Grates; ++Wedge)
	{
		const bool bHot = (Mask & (1u << Wedge)) != 0u;
		(bHot ? Hot : Cold).Add(WedgeTransform(Wedge, 6.f));
		if (bHot && bBurning)
		{
			// Tongues of furnace fire up through the burning wedge's slots.
			FTransform Tongue = WedgeTransform(Wedge, 70.f);
			Tongue.SetRotation(FQuat::Identity);
			Tongue.SetScale3D(FVector(1.6, 1.6, 1.3));
			Fire.Add(Tongue);
		}
	}
	const auto Lay = [](UInstancedStaticMeshComponent* Look, const TArray<FTransform>& Where)
	{
		Look->ClearInstances();
		if (!Where.IsEmpty())
		{
			Look->AddInstances(Where, /*bShouldReturnIndices*/ false);
		}
	};
	Lay(GrateCold, Cold);
	Lay(GrateHot, Hot);
	Lay(GrateFire, Fire);
}

void ABargeFurnace::HurtOnGrates(float DeltaSeconds)
{
	if (!IsRingBurning())
	{
		HurtClock = 0.f;
		return;
	}
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Player = Controller ? Controller->GetPawn() : nullptr;
	if (!Player)
	{
		return;
	}
	const float Half = Player->GetRootComponent() ? static_cast<float>(Player->GetRootComponent()->Bounds.BoxExtent.Z) : 90.f;
	const FVector Local = GetActorTransform().InverseTransformPosition(Player->GetActorLocation() - FVector(0.0, 0.0, Half));
	const int32 Wedge = BargeRules::GrateAt(FVector2D(Local.X, Local.Y));
	if (Local.Z > OnFloor || Local.Z < -OnFloor || Wedge == INDEX_NONE || (GetHotMask() & (1u << Wedge)) == 0u)
	{
		HurtClock = 0.f;
		return;
	}
	HurtClock += DeltaSeconds;
	if (HurtClock < BargeRules::HurtEvery)
	{
		return;
	}
	const UHealthComponent* Life = Player->FindComponentByClass<UHealthComponent>();
	const float Amount = (Life ? Life->GetMaxHealth() : 100.f) * BargeRules::GrateBurnShare * HurtClock;
	HurtClock = 0.f;
	UGameplayStatics::ApplyDamage(Player, Amount, nullptr, this, UDamageType::StaticClass());
	LooterSound::PlayAt(this, LooterSoundCue::Spitter::Burn, Player->GetActorLocation(), 0.8f);
}

// ---------------------------------------------------------------------------
// The cage
// ---------------------------------------------------------------------------

bool ABargeFurnace::CanOpenCage(bool bForce) const
{
	if (bCageBurst || bOut)
	{
		return false;
	}
	if (bForce)
	{
		return true;
	}
	return ValvesShut >= BargeRules::NumValves && StoryHolds(OpenWhen) && AKeepersLantern::IsTakenIn(this);
}

bool ABargeFurnace::BurstCage(AActor* By, bool bForce)
{
	if (!CanOpenCage(bForce))
	{
		return false;
	}
	UE_LOG(LogLooter, Log, TEXT("%s: the soul cage is opened. What comes out isn't Abel."), *GetActorNameOrLabel());
	ABargeBoarding* Boarding = Barge.Get();
	// The scene bursts it and sets the barge dying as it ends; with scenes off it leaves that end state at once.
	if (!Boarding || !CageBurst::Play(*Boarding, *this))
	{
		SetCageBurst(true);
		if (Boarding)
		{
			Boarding->NoteCageBurst();
		}
	}
	return true;
}

void ABargeFurnace::SetCageBurst(bool bBurst)
{
	bCageBurst = bBurst;
	Heart->SetVisibility(!bBurst && !bOut);
	HeartSmog->SetVisibility(!bBurst && !bOut);
	CageFrame->SetVisibility(true);
	CageLight->SetVisibility(!bBurst && !bOut);
	Shards->ClearInstances();
	if (bBurst)
	{
		// The window's glass and two of its bars flung across the floor, fanned out from the cage.
		FRandomStream Spread(6006);
		TArray<FTransform> Flung;
		for (int32 Index = 0; Index < 12; ++Index)
		{
			const float Out = Spread.FRandRange(420.f, 1000.f);
			const float Across = Spread.FRandRange(-420.f, 420.f);
			Flung.Add(FTransform(FRotator(Spread.FRandRange(-20.f, 20.f), Spread.FRandRange(0.f, 360.f), Spread.FRandRange(-20.f, 20.f)),
				FVector(Out, Across, 8.0), FVector(Spread.FRandRange(0.2f, 0.5f), Spread.FRandRange(0.06f, 0.3f), 0.04)));
		}
		Shards->AddInstances(Flung, /*bShouldReturnIndices*/ false);
	}
}

void ABargeFurnace::PutOut()
{
	bOut = true;
	SetRingRunning(false);
	Heart->SetVisibility(false);
	HeartSmog->SetVisibility(false);
	CageLight->SetVisibility(false);
}

FVector ABargeFurnace::GetCageLocation() const
{
	return GetActorTransform().TransformPosition(HeartAt);
}

FVector ABargeFurnace::GetCageFacing() const
{
	return GetActorForwardVector().GetSafeNormal2D();
}

FVector ABargeFurnace::GetBackWall() const
{
	return GetActorTransform().TransformPosition(FVector(-BargeRules::HallRadius, 0.0, BargeRules::CageUp + 200.0));
}

FInteractionOptions ABargeFurnace::GetInteractionOptions(const UInteractionComponent& User) const
{
	if (bCageBurst || bOut || ValvesShut < BargeRules::NumValves || !StoryHolds(OpenWhen))
	{
		return FInteractionOptions::None();
	}
	FInteractionOptions Options;
	if (!AKeepersLantern::IsTakenIn(this))
	{
		// Shown, not usable: the reason the cage won't be opened yet (he can only be carried in his own lantern).
		Options.bUsable = false;
		Options.bTap = true;
		Options.TapPrompt = NeedsLanternPrompt;
		return Options;
	}
	Options.bTap = false;
	Options.bHold = true;
	Options.HoldSeconds = HoldSeconds;
	Options.HoldPrompt = Prompt;
	return Options;
}

bool ABargeFurnace::Interact(UInteractionComponent& User, bool bHeld)
{
	return bHeld && BurstCage(User.GetOwner());
}

TOptional<FVector> ABargeFurnace::GetInteractionLocation() const
{
	return TOptional<FVector>(GetActorTransform().TransformPosition(HeartAt + FVector(80.0, 0.0, 0.0)));
}

TOptional<FInteractionBox> ABargeFurnace::GetInteractionBox() const
{
	// The cage's window and an arm's reach in front of it.
	return InteractionFocus::BoxAround(GetActorTransform().TransformPosition(HeartAt + FVector(40.0, 0.0, 0.0)), FVector(110.0, 140.0, 140.0), GetActorQuat());
}

#undef LOCTEXT_NAMESPACE

// Out of line: the header only forward-declares the barge, and the weak pointer needs its full type to convert.
void ABargeFurnace::SetBarge(ABargeBoarding* InBarge)
{
	Barge = InBarge;
}

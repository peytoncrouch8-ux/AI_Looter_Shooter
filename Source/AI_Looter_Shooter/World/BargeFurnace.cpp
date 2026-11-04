// ABargeFurnace: the barge's soul furnace, Abel's light beating in its cage, and the grate ring that heats round it. The
// cage (its opening, its burst, the furnace going out, its interaction) is BargeFurnaceCage.cpp.

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
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"

#define LOCTEXT_NAMESPACE "BargeFurnace"

const FName ABargeFurnace::CageTag(TEXT("Furnace_SoulCage"));
const TCHAR* const ABargeFurnace::BodyPath = BargeLooks::Furnace;
const TCHAR* const ABargeFurnace::GratePath = BargeLooks::FurnaceGrate;

namespace
{
	/** The heart's beat: a double beat a little over once a second, as a heart under strain. */
	constexpr float BeatHz = 1.1f;
	/** The heart's size (cm across) at rest and at its beat's peak; the smog's round it, out of step, so the black flares. */
	constexpr float HeartRest = 110.f;
	constexpr float HeartPeak = 150.f;
	constexpr float SmogRest = 120.f;
	constexpr float SmogPeak = 165.f;
	/** Where the heart hangs (its frame), past the drum's face: in the boiler model, this far inside it, in the cavity behind
	 *  its barred porthole (its Heart socket, BARGE_PROPS.md); before the plain stand-in drum, this far out in front of it.
	 *  The cage's light this far in front of the heart (out through the porthole); the cage's mouth (where the burst comes
	 *  out) this far in front of the porthole (clear of the boiler's plinth). */
	constexpr float HeartIn = -100.f;
	constexpr float HeartOut = 30.f;
	constexpr float LightOut = 60.f;
	constexpr float MouthOut = 60.f;
	const FName HeartSocket(TEXT("Heart"));
	const FName WindowSocket(TEXT("Window"));
	const FName CrownSocket(TEXT("Crown"));
	/** The crown's pipe collars stand this far over the model's crown (its Pipe_N sockets). */
	constexpr float PipeCollarUp = 40.f;
	/** The stand-in grate plate's middle over the floor (half its 12 cm); the model's fire socket on its rails' top; a burning
	 *  wedge's fire this big (fire-tongue units: 1.6 m across, 1.3 m tall). */
	constexpr float GratePlateUp = 6.f;
	constexpr float GrateFireUp = 9.f;
	const FVector GrateFireSize(1.6, 1.6, 1.3);
	const FLinearColor HeartColor(1.f, 0.86f, 0.6f);

	/** A socket's place on Mesh (its own frame), else Fallback. */
	FVector SocketOr(const UStaticMesh* Mesh, FName Name, const FVector& Fallback)
	{
		const UStaticMeshSocket* Socket = Mesh ? Mesh->FindSocket(Name) : nullptr;
		return Socket ? Socket->RelativeLocation : Fallback;
	}
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
	static UStaticMesh* const FurnaceMesh = BargeLooks::Find<UStaticMesh>(BodyPath);
	static UStaticMesh* const BurstMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::FurnaceBurst);
	static UStaticMesh* const GrateMesh = BargeLooks::Find<UStaticMesh>(GratePath);
	static UStaticMesh* const FireMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::FireTongue);
	static UStaticMesh* const ShardMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::CageShard);
	static UStaticMesh* const PipeMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Pipe);
	static UMaterialInterface* const Iron = BargeLooks::Find<UMaterialInterface>(BargeLooks::Iron);
	static UMaterialInterface* const IronBlack = BargeLooks::Find<UMaterialInterface>(BargeLooks::IronBlack);
	static UMaterialInterface* const Glow = BargeLooks::Find<UMaterialInterface>(BargeLooks::Glow);
	static UMaterialInterface* const Soul = BargeLooks::Find<UMaterialInterface>(BargeLooks::Soul);
	static UMaterialInterface* const Smog = BargeLooks::Find<UMaterialInterface>(BargeLooks::Smog);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	WholeMesh = FurnaceMesh;
	BrokenMesh = BurstMesh;
	bGrateModel = GrateMesh != nullptr;
	bFireModel = FireMesh != nullptr;
	bShardModel = ShardMesh != nullptr;
	const float D = BargeRules::FurnaceRadius * 2.f / 100.f;
	// The body: the kit's riveted boiler (its pivot on the floor, scaled to the rules' radius and height), else an iron drum
	// FurnaceRadius round and 6 m tall.
	BodyScale = FVector(BargeRules::FurnaceRadius / BargeLooks::FurnaceModelRadius, BargeRules::FurnaceRadius / BargeLooks::FurnaceModelRadius,
		BargeRules::FurnaceHeight / BargeLooks::FurnaceModelHeight);
	Body = MakeFurnacePart(*this, Root, TEXT("Body"), FurnaceMesh ? FurnaceMesh : CylinderMesh, FurnaceMesh ? nullptr : Iron,
		FurnaceMesh ? FVector::ZeroVector : FVector(0.0, 0.0, BargeRules::FurnaceHeight * 0.5), FurnaceMesh ? BodyScale : FVector(D, D, BargeRules::FurnaceHeight / 100.f));
	// Its iron is what bullets and boots meet (a 5 m drum the fight goes round).
	Body->SetCollisionProfileName(TEXT("BlockAll"));
	Body->SetCollisionObjectType(ECC_WorldStatic);
	Crown = MakeFurnacePart(*this, Root, TEXT("Crown"), SphereMesh, IronBlack, FVector(0.0, 0.0, BargeRules::FurnaceHeight), FVector(D * 0.92f, D * 0.92f, 2.2f));
	Crown->SetVisibility(FurnaceMesh == nullptr);
	Ironwork = MakeFurnaceInstances(*this, Root, TEXT("Ironwork"), CylinderMesh, IronBlack);
	Pipes = MakeFurnaceInstances(*this, Root, TEXT("Pipes"), PipeMesh, nullptr);
	CageFrame = MakeFurnacePart(*this, Root, TEXT("CageFrame"), CubeMesh, IronBlack, FVector(BargeRules::FurnaceRadius - 15.0, 0.0, BargeRules::CageUp),
		FVector(0.3, 2.1, 2.1));
	CageFrame->SetVisibility(FurnaceMesh == nullptr);
	// The heart: inside the boiler's cavity behind its barred porthole (its Heart socket), else hanging out before the plain
	// drum's face.
	HeartAt = FurnaceMesh ? SocketOr(FurnaceMesh, HeartSocket, FVector(BargeLooks::FurnaceModelRadius + HeartIn, 0.0, BargeRules::CageUp / BodyScale.Z)) * BodyScale
		: FVector(BargeRules::FurnaceRadius + HeartOut, 0.0, BargeRules::CageUp);
	MouthAt = FurnaceMesh ? SocketOr(FurnaceMesh, WindowSocket, HeartAt / BodyScale) * BodyScale + FVector(MouthOut, 0.0, 0.0) : HeartAt;
	Heart = MakeFurnacePart(*this, Root, TEXT("Heart"), SphereMesh, Soul, HeartAt, FVector(HeartRest / 100.f));
	HeartSmog = MakeFurnacePart(*this, Root, TEXT("HeartSmog"), SphereMesh, Smog, HeartAt, FVector(SmogRest / 100.f));
	Shards = MakeFurnaceInstances(*this, Root, TEXT("Shards"), ShardMesh ? ShardMesh : CubeMesh, ShardMesh ? nullptr : IronBlack);

	CageLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CageLight"));
	CageLight->SetupAttachment(Root);
	CageLight->SetMobility(EComponentMobility::Movable);
	CageLight->SetRelativeLocation(HeartAt + FVector(LightOut, 0.0, 0.0));
	CageLight->SetIntensityUnits(ELightUnits::Candelas);
	CageLight->SetIntensity(LitCandelas);
	CageLight->SetAttenuationRadius(LightRadius);
	CageLight->SetLightColor(HeartColor);
	// The hall's one light, never a shadow (the area keeps to three shadowless lights in view).
	CageLight->SetCastShadows(false);

	// The ring: the kit's wedge (its ember bed on the CinderGlow slot: iron while cold, its own glow when hot), else plates;
	// its fire the tongue model, else the engine cone.
	GrateCold = MakeFurnaceInstances(*this, Root, TEXT("GrateCold"), GrateMesh ? GrateMesh : CubeMesh, GrateMesh ? nullptr : IronBlack);
	GrateHot = MakeFurnaceInstances(*this, Root, TEXT("GrateHot"), GrateMesh ? GrateMesh : CubeMesh, GrateMesh ? nullptr : Glow);
	GrateFire = MakeFurnaceInstances(*this, Root, TEXT("GrateFire"), FireMesh ? FireMesh : ConeMesh, FireMesh ? nullptr : Glow);
	if (GrateMesh)
	{
		BargeLooks::SetSlot(*GrateCold, BargeLooks::GlowSlot, IronBlack);
	}
	DarkGlow = IronBlack;

	Prompt = LOCTEXT("Open", "Open the soul cage");
	NeedsLanternPrompt = LOCTEXT("NeedsLantern", "Abel's lantern is needed to carry him");
}

void ABargeFurnace::BeginPlay()
{
	Super::BeginPlay();
	// The pipes up into the hall's ceiling (from the crown's collars: never through the deck over it). The boiler model has
	// its own bands and its porthole's bars; the stand-in drum gets them as iron cylinders, its pipes from under its crown.
	TArray<FTransform> Iron;
	TArray<FTransform> Runs;
	const FVector PipeFeet[] = { FVector(-120.0, 150.0, 0.0), FVector(-120.0, -150.0, 0.0), FVector(-220.0, 0.0, 0.0) };
	if (WholeMesh)
	{
		for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(PipeFeet)); ++Index)
		{
			const FVector Foot = SocketOr(WholeMesh, FName(*FString::Printf(TEXT("Pipe_%d"), Index + 1)),
				PipeFeet[Index] + FVector(0.0, 0.0, BargeLooks::FurnaceModelHeight + PipeCollarUp)) * BodyScale;
			if (Pipes->GetStaticMesh())
			{
				BargeLooks::AddPipeRun(Runs, Foot, BargeRules::HallHeight - Foot.Z, BargeRules::FurnacePipeWidth);
			}
			else
			{
				const double Length = BargeRules::HallHeight - Foot.Z;
				Iron.Add(FTransform(FQuat::Identity, Foot + FVector(0.0, 0.0, Length * 0.5),
					FVector(BargeRules::FurnacePipeWidth / 100.0, BargeRules::FurnacePipeWidth / 100.0, Length / 100.0)));
			}
		}
	}
	else
	{
		const double PipeFoot = BargeRules::FurnaceHeight - BargeRules::PipeUnderCrown;
		const double PipeLength = BargeRules::HallHeight - PipeFoot;
		for (const FVector& Pipe : PipeFeet)
		{
			Iron.Add(FTransform(FQuat::Identity, Pipe + FVector(0.0, 0.0, PipeFoot + PipeLength * 0.5),
				FVector(BargeRules::FurnacePipeWidth / 100.0, BargeRules::FurnacePipeWidth / 100.0, PipeLength / 100.0)));
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
	}
	Ironwork->ClearInstances();
	Ironwork->AddInstances(Iron, /*bShouldReturnIndices*/ false);
	if (Pipes->GetStaticMesh())
	{
		Pipes->ClearInstances();
		Pipes->AddInstances(Runs, false);
	}
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
		LamplighterEffects::Sparks(this, GetActorTransform().TransformPosition(SocketOr(WholeMesh, CrownSocket, FVector(0.0, 0.0, BargeRules::FurnaceHeight / BodyScale.Z)) * BodyScale),
			FVector::UpVector, 8, 0.8f);
		LamplighterEffects::Shake(this, 0.25f, 0.5f);
	}
	if (ValvesShut >= BargeRules::NumValves)
	{
		SetRingRunning(false);
	}
	UpdateGlow();
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
	if (bGrateModel)
	{
		// The kit's wedge on the floor at the ring's middle radius, +X out along it, scaled from the ring it was built for.
		return FTransform(FRotator(0.f, Angle, 0.f), At, FVector((BargeRules::GrateOuter - BargeRules::GrateInner) / BargeLooks::GrateModelDepth,
			Middle * BargeLooks::GrateModelCount / (BargeLooks::GrateModelMiddle * BargeRules::Grates), 1.0));
	}
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
		// The model stands on the floor; the stand-in plate is centred, half its 12 cm over it.
		(bHot ? Hot : Cold).Add(WedgeTransform(Wedge, bGrateModel ? 0.f : GratePlateUp));
		if (bHot && bBurning)
		{
			// Tongues of furnace fire up through the burning wedge's bars, standing on its rails (the model's Fire socket).
			const FVector Foot = WedgeTransform(Wedge, bGrateModel ? GrateFireUp : 0.f).GetLocation();
			Fire.Add(BargeLooks::FireAt(Foot, GrateFireSize, bFireModel));
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

#undef LOCTEXT_NAMESPACE

// Out of line: the header only forward-declares the barge, and the weak pointer needs its full type to convert.
void ABargeFurnace::SetBarge(ABargeBoarding* InBarge)
{
	Barge = InBarge;
}

// ASextonsFerry: the Ferryman's landing and his ferry hanging in the fog under Gravewind Point: its parts, Sexton in the
// stern, the sway and the chains.

#include "World/SextonsFerry.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const TCHAR* const ASextonsFerry::FerryModelPath = TEXT("/Game/Art/Tripo/SM_SextonsFerry.SM_SextonsFerry");

namespace
{
	// The stand-in's look, one path each.
	const TCHAR* FerryCubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* FerryCylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* FerryLanternPath = TEXT("/Game/Art/Props/SM_PorchLantern.SM_PorchLantern");
	// Bank B ("Drift"): strata only. Bank A's curtain stood up through the landing once the bank was raised to the hull.
	const TCHAR* FerryFogPath = TEXT("/Game/Art/Props/SM_CanyonFog_B.SM_CanyonFog_B");
	const TCHAR* FerryHullPaintPath = TEXT("/Game/Art/Materials/MI_Charcoal.MI_Charcoal");
	const TCHAR* FerryTimberPath = TEXT("/Game/Art/Materials/MI_LanternWood.MI_LanternWood");
	const TCHAR* FerryIronPath = TEXT("/Game/Art/Materials/MI_IronBlack.MI_IronBlack");

	/** The rails: their height and girth; the blockers over them reach this high (cm). */
	constexpr float RailHeight = 95.f;
	constexpr float RailGirth = 8.f;
	constexpr float BlockerHeight = 190.f;
	/** The chains' thickness (cm), and how far under the deck's boards their tops are (its beams). */
	constexpr float ChainWidth = 4.f;
	constexpr float DeckUnderside = 45.f;
	/** The ferry hangs this far under the landing's planks (its gunwale about at the planks). */
	constexpr float FerryHang = -20.f;
	/** The stern thwart where Sexton sits, and the chain eyes at bow and stern (ferry frame, cm): the Tripo ferry's sockets
	    SeatAt / BowEye / SternEye (lanes/TRIPO_M23.md; the stand-in's were (0,-320,8) and (0,+-440,10), close enough). */
	const FVector SeatAt(0.0, -333.0, 6.0);
	const FVector BowEye(0.0, 456.0, 27.0);
	const FVector SternEye(0.0, -403.0, 30.0);
	/** Where the Tripo ferry's own lantern hangs, on its pole at the stern (its Lantern socket). */
	const FVector TripoLanternAt(-1.0, -430.0, 112.0);
	/** The prow's lantern, hanging from its pole's arm. */
	const FVector LanternAt(0.0, 530.0, 135.0);
	/** The sway: a slow roll, a slower turn, a bob (degrees, cm), each at its own pace (s a swing) so they never fall in step. */
	constexpr float RollDegrees = 1.2f;
	constexpr float RollPeriod = 7.f;
	constexpr float TurnDegrees = 0.6f;
	constexpr float TurnPeriod = 11.f;
	constexpr float BobCm = 3.f;
	constexpr float BobPeriod = 5.f;
	/**
	 * The fog bank round the ferry's waterline (landing frame, cm), not under it: at 4 m down (the play check of 2026-11-01)
	 * the hull hung clear over it. Bank B (7 m tall, its three strata at about 1, 4 and 6 m) flattened to a 1.15 m band: its
	 * strata at about -134, -86 and -51 against the hull's -100 to -7, all under the landing's planks (-14); a little past
	 * the ferry so less of it runs into the cliff, and wide enough (17 x 13 m) that the whole hull sits in it.
	 */
	const FVector FogAt(ASextonsFerry::FerryOut + 100.f, ASextonsFerry::FerryAlong, -150.f);
	const FVector FogScale(0.45, 0.45, 0.16);
	/**
	 * M_CanyonFog's defaults are for banks seen across the canyon (it fades out within 4-12 m of the camera and 3 m from
	 * what's behind it): this one is seen from 2-6 m and wraps the hull, so it shows from close and fades only near the hull.
	 */
	constexpr float FogNearStart = 50.f;
	constexpr float FogNearEnd = 250.f;
	constexpr float FogDepthFade = 60.f;

	template <typename T>
	T* FerryAssetIfMade(const TCHAR* ObjectPath)
	{
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath))))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<T> Finder(ObjectPath);
		return Finder.Object;
	}

	/** A cylinder part stretched from A to B (its parent's frame), Width across. */
	void StretchChain(UStaticMeshComponent* Part, const FVector& A, const FVector& B, float Width)
	{
		if (!Part)
		{
			return;
		}
		const FVector Span = B - A;
		Part->SetRelativeLocationAndRotation((A + B) * 0.5, FRotationMatrix::MakeFromZ(Span.GetSafeNormal()).Rotator());
		Part->SetRelativeScale3D(FVector(Width / 100.0, Width / 100.0, FMath::Max(Span.Size(), 1.0) / 100.0));
	}
}

ASextonsFerry::ASextonsFerry()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.f;
	Tags.Add(FName(TEXT("Obstacle")));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(FerryCubePath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(FerryCylinderPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> HullPaint(FerryHullPaintPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Timber(FerryTimberPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Iron(FerryIronPath);
	static UStaticMesh* const LanternMesh = FerryAssetIfMade<UStaticMesh>(FerryLanternPath);
	static UStaticMesh* const FogMesh = FerryAssetIfMade<UStaticMesh>(FerryFogPath);
	static UStaticMesh* const TripoFerry = FerryAssetIfMade<UStaticMesh>(FerryModelPath);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	auto MakePart = [&](USceneComponent* Parent, const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Look, const FVector& At,
		const FRotator& Turn, const FVector& Scale, bool bSolid)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetStaticMesh(Mesh);
		if (Look)
		{
			Part->SetMaterial(0, Look);
		}
		Part->SetRelativeLocationAndRotation(At, Turn);
		Part->SetRelativeScale3D(Scale);
		Part->SetCanEverAffectNavigation(false);
		Part->SetCollisionProfileName(bSolid ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
		return Part;
	};

	// The landing's planks, walked on.
	Landing = MakePart(Root, TEXT("Landing"), Cube.Object, Timber.Object, FVector(0.0, 0.0, -7.0), FRotator::ZeroRotator,
		FVector(LandingDepth / 100.0, LandingWidth / 100.0, 0.14), true);

	// The rails, all four sides, and a post at each corner.
	const float HalfDepth = LandingDepth * 0.5f - 5.f;
	const float HalfWidth = LandingWidth * 0.5f - 5.f;
	Rails.Add(MakePart(Root, TEXT("RailOut"), Cube.Object, Timber.Object, FVector(HalfDepth, 0.0, RailHeight), FRotator::ZeroRotator,
		FVector(RailGirth / 100.0, LandingWidth / 100.0, RailGirth / 100.0), false));
	Rails.Add(MakePart(Root, TEXT("RailIn"), Cube.Object, Timber.Object, FVector(-HalfDepth, 0.0, RailHeight), FRotator::ZeroRotator,
		FVector(RailGirth / 100.0, LandingWidth / 100.0, RailGirth / 100.0), false));
	Rails.Add(MakePart(Root, TEXT("RailLeft"), Cube.Object, Timber.Object, FVector(0.0, -HalfWidth, RailHeight), FRotator::ZeroRotator,
		FVector(LandingDepth / 100.0, RailGirth / 100.0, RailGirth / 100.0), false));
	Rails.Add(MakePart(Root, TEXT("RailRight"), Cube.Object, Timber.Object, FVector(0.0, HalfWidth, RailHeight), FRotator::ZeroRotator,
		FVector(LandingDepth / 100.0, RailGirth / 100.0, RailGirth / 100.0), false));
	int32 Corner = 0;
	for (const float SideX : { -1.f, 1.f })
	{
		for (const float SideY : { -1.f, 1.f })
		{
			const FName Name(*FString::Printf(TEXT("RailPost%d"), Corner++));
			Rails.Add(MakePart(Root, *Name.ToString(), Cylinder.Object, Timber.Object, FVector(SideX * HalfDepth, SideY * HalfWidth, RailHeight * 0.5f),
				FRotator::ZeroRotator, FVector(0.1, 0.1, RailHeight / 100.0), true));
		}
	}

	// Unseen walls over the rails: pawns only (shots, sight and the Interact key's line pass them).
	auto MakeBlocker = [&](const TCHAR* Name, const FVector& At, const FVector& Extent)
	{
		UBoxComponent* Blocker = CreateDefaultSubobject<UBoxComponent>(Name);
		Blocker->SetupAttachment(Root);
		Blocker->SetRelativeLocation(At);
		Blocker->InitBoxExtent(Extent);
		Blocker->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Blocker->SetCollisionObjectType(ECC_WorldStatic);
		Blocker->SetCollisionResponseToAllChannels(ECR_Ignore);
		Blocker->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Blocker->SetCanEverAffectNavigation(false);
		Blocker->SetHiddenInGame(true);
		return Blocker;
	};
	const float Half = BlockerHeight * 0.5f;
	Blockers.Add(MakeBlocker(TEXT("BlockOut"), FVector(LandingDepth * 0.5f, 0.0, Half), FVector(6.0, LandingWidth * 0.5f, Half)));
	Blockers.Add(MakeBlocker(TEXT("BlockIn"), FVector(-LandingDepth * 0.5f, 0.0, Half), FVector(6.0, LandingWidth * 0.5f, Half)));
	Blockers.Add(MakeBlocker(TEXT("BlockLeft"), FVector(0.0, -LandingWidth * 0.5f, Half), FVector(LandingDepth * 0.5f, 6.0, Half)));
	Blockers.Add(MakeBlocker(TEXT("BlockRight"), FVector(0.0, LandingWidth * 0.5f, Half), FVector(LandingDepth * 0.5f, 6.0, Half)));

	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FName Name(*FString::Printf(TEXT("LandingChain%d"), Index));
		UStaticMeshComponent* Chain = MakePart(Root, *Name.ToString(), Cylinder.Object, Iron.Object, FVector::ZeroVector, FRotator::ZeroRotator,
			FVector::OneVector, false);
		Chain->SetCastShadow(false);
		LandingChains.Add(Chain);
	}

	// The ferry: its frame sways; the hull in charcoal-black timbers, a high prow pole with its lantern, a stern thwart.
	FerryFrame = CreateDefaultSubobject<USceneComponent>(TEXT("FerryFrame"));
	FerryFrame->SetupAttachment(Root);
	FerryFrame->SetMobility(EComponentMobility::Movable);
	FerryFrame->SetRelativeLocation(FVector(FerryOut, FerryAlong, FerryHang));
	UMaterialInterface* Black = HullPaint.Object ? HullPaint.Object.Get() : Iron.Object.Get();
	const double Beam = FerryBeam / 100.0;
	const double Long = FerryLength / 100.0;
	Hull.Add(MakePart(FerryFrame, TEXT("HullFloor"), Cube.Object, Black, FVector(0.0, 0.0, -80.0), FRotator::ZeroRotator, FVector(Beam, Long, 0.08), true));
	Hull.Add(MakePart(FerryFrame, TEXT("HullSideNear"), Cube.Object, Black, FVector(-FerryBeam * 0.5f, 0.0, -35.0), FRotator(0.0, 0.0, -6.0),
		FVector(0.06, Long, 0.95), true));
	Hull.Add(MakePart(FerryFrame, TEXT("HullSideFar"), Cube.Object, Black, FVector(FerryBeam * 0.5f, 0.0, -35.0), FRotator(0.0, 0.0, 6.0),
		FVector(0.06, Long, 0.95), true));
	Hull.Add(MakePart(FerryFrame, TEXT("HullBow"), Cube.Object, Black, FVector(0.0, FerryLength * 0.5f, -30.0), FRotator::ZeroRotator,
		FVector(Beam, 0.06, 1.05), true));
	Hull.Add(MakePart(FerryFrame, TEXT("HullStern"), Cube.Object, Black, FVector(0.0, -FerryLength * 0.5f, -35.0), FRotator::ZeroRotator,
		FVector(Beam, 0.06, 0.95), true));
	Hull.Add(MakePart(FerryFrame, TEXT("ProwPole"), Cylinder.Object, Black, FVector(0.0, FerryLength * 0.5f + 20.0, 60.0), FRotator::ZeroRotator,
		FVector(0.14, 0.14, 2.4), true));
	Hull.Add(MakePart(FerryFrame, TEXT("ProwArm"), Cube.Object, Iron.Object, FVector(0.0, FerryLength * 0.5f + 50.0, 170.0), FRotator::ZeroRotator,
		FVector(0.05, 0.6, 0.05), false));
	Hull.Add(MakePart(FerryFrame, TEXT("Thwart"), Cube.Object, Timber.Object, SeatAt - FVector(0.0, 0.0, 4.0), FRotator::ZeroRotator,
		FVector(Beam - 0.1, 0.4, 0.06), false));
	// The long sweep oar off the stern, its blade down in the fog.
	Hull.Add(MakePart(FerryFrame, TEXT("SweepOar"), Cube.Object, Timber.Object, FVector(0.0, -FerryLength * 0.5f - 140.0, -40.0), FRotator(0.0, 0.0, 18.0),
		FVector(0.06, 3.2, 0.06), false));

	Model = MakePart(FerryFrame, TEXT("Model"), TripoFerry, nullptr, FVector::ZeroVector, FRotator::ZeroRotator, FVector::OneVector,
		TripoFerry != nullptr);
	if (TripoFerry)
	{
		// The Tripo ferry is hull, pole and thwart in one: the stand-in's cubes go (the seat and chain eyes stay where they were).
		for (UStaticMeshComponent* Part : Hull)
		{
			Part->SetVisibility(false);
			Part->SetHiddenInGame(true);
			Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	else
	{
		Model->SetVisibility(false);
		Model->SetHiddenInGame(true);
	}

	Seat = CreateDefaultSubobject<USceneComponent>(TEXT("Seat"));
	Seat->SetupAttachment(FerryFrame);
	Seat->SetMobility(EComponentMobility::Movable);
	// Facing the landing (the ferry frame's -X).
	Seat->SetRelativeLocationAndRotation(SeatAt, FRotator(0.0, 180.0, 0.0));

	Lantern = MakePart(FerryFrame, TEXT("Lantern"), LanternMesh, nullptr, LanternAt, FRotator::ZeroRotator, FVector::OneVector, false);
	LanternLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LanternLight"));
	LanternLight->SetupAttachment(FerryFrame);
	LanternLight->SetRelativeLocation(LanternAt);
	LanternLight->SetIntensityUnits(ELightUnits::Candelas);
	LanternLight->SetIntensity(10.f);
	LanternLight->SetAttenuationRadius(900.f);
	LanternLight->SetLightColor(FLinearColor(1.f, 0.72f, 0.42f));
	LanternLight->SetCastShadows(false);
	LanternLight->SetMobility(EComponentMobility::Movable);
	if (TripoFerry)
	{
		// The Tripo ferry carries its own lantern on a pole at the stern: the stand-in's prow lantern goes, the light moves onto it.
		Lantern->SetVisibility(false);
		Lantern->SetHiddenInGame(true);
		LanternLight->SetRelativeLocation(TripoLanternAt);
	}

	BowChain = MakePart(Root, TEXT("BowChain"), Cylinder.Object, Iron.Object, FVector::ZeroVector, FRotator::ZeroRotator, FVector::OneVector, false);
	SternChain = MakePart(Root, TEXT("SternChain"), Cylinder.Object, Iron.Object, FVector::ZeroVector, FRotator::ZeroRotator, FVector::OneVector, false);
	BowChain->SetMobility(EComponentMobility::Movable);
	SternChain->SetMobility(EComponentMobility::Movable);
	BowChain->SetCastShadow(false);
	SternChain->SetCastShadow(false);

	// The fog bank round its waterline (its own fog material, unlit by the scene's light: the Ferryman's fog shows in any weather).
	Fog = MakePart(Root, TEXT("Fog"), FogMesh, nullptr, FogAt, FRotator::ZeroRotator, FogScale, false);
	Fog->SetCastShadow(false);
	// Placed again as play begins (PlaceFog), so it must be movable (unlit and shadowless: it costs nothing more).
	Fog->SetMobility(EComponentMobility::Movable);
}

void ASextonsFerry::PlaceFog()
{
	// Always from the constants: a placed ferry saved its fog at the old 0.3 scale (build_area_main2 set it per instance).
	if (Fog)
	{
		Fog->SetRelativeLocationAndRotation(FogAt, FRotator::ZeroRotator);
		Fog->SetRelativeScale3D(FogScale);
	}
}

void ASextonsFerry::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	PlaceFog();
	Sway(0.f);
}

void ASextonsFerry::BeginPlay()
{
	Super::BeginPlay();
	PlaceFog();
	// One dynamic instance: the fog seen from close (FogNearStart). Parameters M_CanyonFog lacks are ignored.
	if (Fog && Fog->GetStaticMesh())
	{
		if (UMaterialInstanceDynamic* Close = Fog->CreateDynamicMaterialInstance(0))
		{
			Close->SetScalarParameterValue(TEXT("NearStart"), FogNearStart);
			Close->SetScalarParameterValue(TEXT("NearEnd"), FogNearEnd);
			Close->SetScalarParameterValue(TEXT("DepthFade"), FogDepthFade);
		}
	}
	SwayClock = FMath::FRandRange(0.f, 10.f);
	Sway(SwayClock);
	// Sexton in the stern, swaying with it.
	if (AActor* Ferryman = Passenger.Get())
	{
		// He moves with it, so he must be movable (a placed story character may have been left static).
		if (USceneComponent* Body = Ferryman->GetRootComponent())
		{
			Body->SetMobility(EComponentMobility::Movable);
		}
		Ferryman->AttachToComponent(Seat, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
}

void ASextonsFerry::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	LooterSound::Stop(Creak.Get(), 0.f);
	Creak.Reset();
	Super::EndPlay(EndPlayReason);
}

bool ASextonsFerry::IsPlayerWithin(double Reach) const
{
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Player = Controller ? Controller->GetPawn() : nullptr;
	return Player && FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) <= Reach * Reach;
}

void ASextonsFerry::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsPlayerWithin(SwayReach))
	{
		LooterSound::Stop(Creak.Get(), 0.5f);
		Creak.Reset();
		return;
	}
	SwayClock += DeltaSeconds;
	Sway(SwayClock);
	if (IsPlayerWithin(CreakReach))
	{
		if (!Creak.IsValid())
		{
			Creak = LooterSound::Start(this, LooterSoundCue::ShipCreak, FerryFrame, NAME_None, 0.5f, 0.8f);
		}
	}
	else if (Creak.IsValid())
	{
		LooterSound::Stop(Creak.Get(), 0.5f);
		Creak.Reset();
	}
}

void ASextonsFerry::Sway(float Seconds)
{
	if (!FerryFrame)
	{
		return;
	}
	const float Swing = Seconds * static_cast<float>(UE_TWO_PI);
	const float Roll = RollDegrees * FMath::Sin(Swing / RollPeriod);
	const float Turn = TurnDegrees * FMath::Sin(Swing / TurnPeriod + 1.f);
	const float Bob = BobCm * FMath::Sin(Swing / BobPeriod + 2.f);
	FerryFrame->SetRelativeLocationAndRotation(FVector(FerryOut, FerryAlong, FerryHang + Bob), FRotator(0.0, Turn, Roll));
	StringChains();
}

void ASextonsFerry::StringChains()
{
	// The landing's: its inner corners straight up under the deck, its outer ones back up to the deck's open edge.
	const float X = LandingDepth * 0.5f - 10.f;
	const float Y = LandingWidth * 0.5f - 10.f;
	const float Up = ClimbDrop - DeckUnderside;
	const FVector Ends[4][2] = {
		{ FVector(-X, -Y, 0.0), FVector(-X, -Y, Up) },
		{ FVector(-X, Y, 0.0), FVector(-X, Y, Up) },
		{ FVector(X, -Y, 0.0), FVector(DeckFrontX - 20.0, -Y, Up) },
		{ FVector(X, Y, 0.0), FVector(DeckFrontX - 20.0, Y, Up) },
	};
	for (int32 Index = 0; Index < LandingChains.Num() && Index < 4; ++Index)
	{
		StretchChain(LandingChains[Index], Ends[Index][0], Ends[Index][1], ChainWidth);
	}
	// The ferry's: bow and stern eyes up to the deck's edge over them.
	if (FerryFrame)
	{
		const FTransform Frame = FerryFrame->GetRelativeTransform();
		StretchChain(BowChain, Frame.TransformPosition(BowEye), FVector(DeckFrontX - 20.0, FerryAlong + BowEye.Y - 60.0, ClimbDrop - 10.0), ChainWidth * 1.2f);
		StretchChain(SternChain, Frame.TransformPosition(SternEye), FVector(DeckFrontX - 20.0, FerryAlong + SternEye.Y + 60.0, ClimbDrop - 10.0), ChainWidth * 1.2f);
	}
}

FVector ASextonsFerry::GetLandingSpot(float Along, float InFromRail) const
{
	return GetActorTransform().TransformPosition(FVector(LandingDepth * 0.5f - InFromRail, Along, 0.0));
}

FTransform ASextonsFerry::GetSeatTransform() const
{
	return Seat ? Seat->GetComponentTransform() : GetActorTransform();
}

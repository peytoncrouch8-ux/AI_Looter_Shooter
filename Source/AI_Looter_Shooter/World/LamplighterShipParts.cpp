// ALamplighterShip: her pieces: the stand-in hull, her walkable deck and rail walls, the Great Lantern and its flame, the
// wheel and the engine, the gangway, the prow's marker and the crew at their stations.

#include "World/LamplighterShip.h"
#include "AI_Looter_Shooter.h"
#include "Story/StoryCondition.h"
#include "Story/WardenCrewMember.h"
#include "World/LandfallMarker.h"
#include "World/LanternFlame.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// The user's Tripo models (TRIPO_MODELS.md 10, 11; lanes/IMPORT.md). Stand-ins were SM_Skiff_A_Packet and SM_KeepersLantern.
	const TCHAR* HullMeshPath = TEXT("/Game/Art/Tripo/SM_Lamplighter.SM_Lamplighter");
	const TCHAR* LanternMeshPath = TEXT("/Game/Art/Tripo/SM_GreatLantern.SM_GreatLantern");
	const TCHAR* GangwayMeshPath = TEXT("/Game/Art/Vehicles/SM_Skiff_Gangplank.SM_Skiff_Gangplank");
	const TCHAR* EngineMeshPath = TEXT("/Game/Art/Props/SM_Barrel_A.SM_Barrel_A");
	const TCHAR* CylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* IronPath = TEXT("/Game/Art/Materials/MI_LanternIron.MI_LanternIron");
	const TCHAR* WoodPath = TEXT("/Game/Art/Materials/MI_LanternWood.MI_LanternWood");

	// --- Her deck, in her frame (cm; the keel's middle at 0, the bow along +X; the Tripo hull measured: lanes/IMPORT.md) ---

	/** The main deck from the stern deck's step to the forecastle, the stern deck (it rises aft, 32 cm up), and the
	 *  forecastle under the Lantern (1.18 m up: a solid block from the deck, not climbed; Finch stands on it). Z of a centre
	 *  is its top over DeckTop; the boxes reach down to 10 cm under the main deck. */
	const FVector MainDeckCenter(132.0, 0.0, 0.0);
	const FVector MainDeckExtent(923.0, 360.0, 10.0);
	const FVector SternDeckCenter(-976.0, 0.0, 32.0);
	const FVector SternDeckExtent(187.0, 340.0, 21.0);
	const FVector ProwDeckCenter(1159.0, 0.0, 118.0);
	const FVector ProwDeckExtent(104.0, 200.0, 64.0);
	/** The rail walls: this tall above the deck (more than a jump), this thick. */
	constexpr float WallHeight = 320.f;
	constexpr float WallHalfThick = 15.f;
	/** The side rails in three lengths each (aft, the gangway's middle, fore), from X to X. */
	const FVector2D SideSpans[] = { FVector2D(-1163.0, -250.0), FVector2D(-250.0, 150.0), FVector2D(150.0, 1055.0) };

	/** Where the pieces stand (her frame, Z from the deck's top). */
	/** On the hull's own lantern pedestal at the prow (SM_Lamplighter's socket Lantern: 2.22 m over the main deck). */
	const FVector LanternSpot(1329.0, 0.0, 0.0);
	constexpr float LanternPostHeight = 222.f;
	/** SM_GreatLantern is imported at its 1.5 m; the flame keeps the size it had on the stand-in (a 35 cm lantern x 4.2). */
	constexpr float LanternScale = 1.f;
	constexpr float LanternFlameScale = 4.2f;
	constexpr float LanternCandela = 40.f;
	constexpr float LanternRadius = 1400.f;
	const FLinearColor LanternWarm(1.f, 0.66f, 0.3f);
	/** The captain's wheel on the stern deck (the hull's own big wheel stands forward of it, its hub 2.8 m up: out of reach). */
	const FVector WheelSpot(-1050.0, 0.0, 142.0);
	/** The engine works amidships, forward of the hull's console and clear of the bag's struts. */
	const FVector EngineSpots[] = { FVector(450.0, -55.0, 0.0), FVector(450.0, 55.0, 0.0) };
	const FVector LandingLocal(300.0, 0.0, 0.0);
	const FVector ProwMarkerLocal(990.0, 0.0, 90.0);
	/** On the bag's top (15 m over the deck). */
	const FVector HobPerchLocal(250.0, 0.0, 1512.0);
	/** The gangway plank's own length (SM_Skiff_Gangplank, 3 m) and its scale. */
	constexpr float PlankLength = 300.f;
	constexpr float GangwayScale = 1.3f;

	/** The crew's stations: Holloway at the wheel, Finch by the Great Lantern, Junie at the engine (her frame, yaw). */
	struct FStation
	{
		EWardenCrew Crew;
		FVector Spot;
		float Yaw;
	};
	const FStation Stations[] = {
		{ EWardenCrew::Captain, FVector(-1120.0, 0.0, 32.0), 0.f },
		{ EWardenCrew::Keeper, FVector(1210.0, 105.0, 118.0), -25.f },
		{ EWardenCrew::Engineer, FVector(450.0, -165.0, 0.0), 90.f },
	};

	/** A floor the player walks and the boarders stand on: blocks everything, world-static for the creatures' ground traces. */
	void MakeFloor(UBoxComponent& Box)
	{
		Box.SetMobility(EComponentMobility::Movable);
		Box.SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Box.SetCollisionObjectType(ECC_WorldStatic);
		Box.SetCanEverAffectNavigation(false);
		Box.SetGenerateOverlapEvents(false);
		Box.SetHiddenInGame(true);
	}

	/** A rail wall: stops pawns only, so shots, the camera and the creatures' ground traces pass it. */
	void MakeWall(UBoxComponent& Box)
	{
		Box.SetMobility(EComponentMobility::Movable);
		Box.SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box.SetCollisionObjectType(ECC_WorldDynamic);
		Box.SetCollisionResponseToAllChannels(ECR_Ignore);
		Box.SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Box.SetCanEverAffectNavigation(false);
		Box.SetGenerateOverlapEvents(false);
		Box.SetHiddenInGame(true);
	}

	/** A deck piece's part: no collision of its own (the deck and the walls are hers). */
	void MakeDressing(UStaticMeshComponent& Mesh)
	{
		Mesh.SetMobility(EComponentMobility::Movable);
		Mesh.SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh.SetCanEverAffectNavigation(false);
	}
}

void ALamplighterShip::BuildRails()
{
	Deck = CreateDefaultSubobject<UBoxComponent>(TEXT("Deck"));
	Deck->SetupAttachment(Root);
	Deck->SetBoxExtent(MainDeckExtent, false);
	Deck->SetRelativeLocation(MainDeckCenter + FVector(0.0, 0.0, DeckTop - MainDeckExtent.Z));
	MakeFloor(*Deck);

	SternDeck = CreateDefaultSubobject<UBoxComponent>(TEXT("SternDeck"));
	SternDeck->SetupAttachment(Root);
	SternDeck->SetBoxExtent(SternDeckExtent, false);
	SternDeck->SetRelativeLocation(SternDeckCenter + FVector(0.0, 0.0, DeckTop - SternDeckExtent.Z));
	MakeFloor(*SternDeck);

	ProwDeck = CreateDefaultSubobject<UBoxComponent>(TEXT("ProwDeck"));
	ProwDeck->SetupAttachment(Root);
	ProwDeck->SetBoxExtent(ProwDeckExtent, false);
	ProwDeck->SetRelativeLocation(ProwDeckCenter + FVector(0.0, 0.0, DeckTop - ProwDeckExtent.Z));
	MakeFloor(*ProwDeck);

	const double WallZ = DeckTop + WallHeight * 0.5;
	auto AddWall = [this, WallZ](const TCHAR* Name, const FVector2D& Center, const FVector2D& Half)
	{
		UBoxComponent* Wall = CreateDefaultSubobject<UBoxComponent>(Name);
		Wall->SetupAttachment(Root);
		Wall->SetBoxExtent(FVector(Half.X, Half.Y, WallHeight * 0.5f), false);
		Wall->SetRelativeLocation(FVector(Center.X, Center.Y, WallZ));
		MakeWall(*Wall);
		Rails.Add(Wall);
	};
	// Order matters: each side's three spans first (port 0-2, starboard 3-5), so the gangway's is found by its side.
	for (const float SideSign : { -1.f, 1.f })
	{
		for (int32 Span = 0; Span < UE_ARRAY_COUNT(SideSpans); ++Span)
		{
			const FVector2D& Along = SideSpans[Span];
			AddWall(*FString::Printf(TEXT("Rail_%s_%d"), SideSign < 0.f ? TEXT("Port") : TEXT("Starboard"), Span),
				FVector2D((Along.X + Along.Y) * 0.5, SideSign * RailOut), FVector2D((Along.Y - Along.X) * 0.5, WallHalfThick));
		}
	}
	AddWall(TEXT("Rail_Stern"), FVector2D(SideSpans[0].X - WallHalfThick, 0.0), FVector2D(WallHalfThick, RailOut + WallHalfThick));
	// The prow's shoulders close the main deck's sides in to the narrower prow; its own sides and front close it.
	const double Shoulder = (RailOut + ProwDeckExtent.Y) * 0.5;
	AddWall(TEXT("Rail_ShoulderPort"), FVector2D(SideSpans[2].Y, -Shoulder), FVector2D(WallHalfThick, (RailOut - ProwDeckExtent.Y) * 0.5));
	AddWall(TEXT("Rail_ShoulderStarboard"), FVector2D(SideSpans[2].Y, Shoulder), FVector2D(WallHalfThick, (RailOut - ProwDeckExtent.Y) * 0.5));
	AddWall(TEXT("Rail_ProwPort"), FVector2D(ProwDeckCenter.X, -(ProwDeckExtent.Y + WallHalfThick)), FVector2D(ProwDeckExtent.X, WallHalfThick));
	AddWall(TEXT("Rail_ProwStarboard"), FVector2D(ProwDeckCenter.X, ProwDeckExtent.Y + WallHalfThick), FVector2D(ProwDeckExtent.X, WallHalfThick));
	AddWall(TEXT("Rail_ProwFront"), FVector2D(ProwDeckCenter.X + ProwDeckExtent.X + WallHalfThick, 0.0),
		FVector2D(WallHalfThick, ProwDeckExtent.Y + 2.0 * WallHalfThick));

	LandingSpot = CreateDefaultSubobject<USceneComponent>(TEXT("LandingSpot"));
	LandingSpot->SetupAttachment(Root);
	LandingSpot->SetRelativeLocation(LandingLocal + FVector(0.0, 0.0, DeckTop));
	LandingSpot->ComponentTags.Add(LandfallRules::LandingTag());

	HobPerch = CreateDefaultSubobject<USceneComponent>(TEXT("HobPerch"));
	HobPerch->SetupAttachment(Root);
	HobPerch->SetRelativeLocation(HobPerchLocal + FVector(0.0, 0.0, DeckTop));
	HobPerch->ComponentTags.Add(LandfallRules::HobPerchTag());
}

void ALamplighterShip::ApplyLook()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> HullMesh(HullMeshPath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> LanternMesh(LanternMeshPath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> GangwayMesh(GangwayMeshPath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> EngineMesh(EngineMeshPath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(CylinderPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Iron(IronPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Wood(WoodPath);

	// The hull is only looked at: her own deck boxes carry the player and the rail walls keep them aboard. The stand-in's
	// scaled side slabs reach well above its painted gunwale, so they'd stop shots across to the tender, and the crosshair's
	// sight of the grapples in the rail, in thin air.
	Hull = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hull"));
	Hull->SetupAttachment(Root);
	Hull->SetStaticMesh(HullMesh.Object);
	Hull->SetRelativeScale3D(HullScale);
	Hull->SetMobility(EComponentMobility::Movable);
	Hull->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Hull->SetCanEverAffectNavigation(false);

	LanternPost = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LanternPost"));
	LanternPost->SetupAttachment(Root);
	LanternPost->SetStaticMesh(Cylinder.Object);
	LanternPost->SetMaterial(0, Iron.Object);
	LanternPost->SetRelativeLocation(LanternSpot + FVector(0.0, 0.0, DeckTop + LanternPostHeight * 0.5));
	LanternPost->SetRelativeScale3D(FVector(0.16, 0.16, LanternPostHeight / 100.0));
	MakeDressing(*LanternPost);

	GreatLantern = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GreatLantern"));
	GreatLantern->SetupAttachment(Root);
	GreatLantern->SetStaticMesh(LanternMesh.Object);
	GreatLantern->SetRelativeLocation(LanternSpot + FVector(0.0, 0.0, DeckTop + LanternPostHeight));
	GreatLantern->SetRelativeScale3D(FVector(LanternScale));
	MakeDressing(*GreatLantern);

	// The Lantern's warm light on the deck and the crew: shadowless, as every lantern in the game (Pipeline: one point light).
	LanternLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LanternLight"));
	LanternLight->SetupAttachment(GreatLantern, TEXT("Light"));
	LanternLight->SetMobility(EComponentMobility::Movable);
	LanternLight->SetCastShadows(false);
	LanternLight->SetIntensityUnits(ELightUnits::Candelas);
	LanternLight->SetIntensity(LanternCandela);
	LanternLight->SetAttenuationRadius(LanternRadius);
	LanternLight->SetLightColor(LanternWarm);

	Wheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wheel"));
	Wheel->SetupAttachment(Root);
	Wheel->SetStaticMesh(Cylinder.Object);
	Wheel->SetMaterial(0, Wood.Object);
	// A wheel 1 m across standing upright across her length, facing the bow.
	Wheel->SetRelativeLocationAndRotation(WheelSpot + FVector(0.0, 0.0, DeckTop), FRotator(90.0, 0.0, 0.0));
	Wheel->SetRelativeScale3D(FVector(1.0, 1.0, 0.08));
	MakeDressing(*Wheel);

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(EngineSpots); ++Index)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Engine%d"), Index));
		Part->SetupAttachment(Root);
		Part->SetStaticMesh(EngineMesh.Object);
		Part->SetRelativeLocation(EngineSpots[Index] + FVector(0.0, 0.0, DeckTop));
		Part->SetRelativeScale3D(FVector(1.3));
		// Solid to walk round, but not a floor the creatures' ground traces take.
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Part->SetCollisionObjectType(ECC_WorldDynamic);
		Part->SetCanEverAffectNavigation(false);
		EngineHousing.Add(Part);
	}

	Gangway = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Gangway"));
	Gangway->SetupAttachment(Root);
	Gangway->SetStaticMesh(GangwayMesh.Object);
	Gangway->SetMobility(EComponentMobility::Movable);
	Gangway->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Gangway->SetCanEverAffectNavigation(false);
	Gangway->SetVisibility(false);
}

void ALamplighterShip::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (Hull)
	{
		Hull->SetRelativeScale3D(HullScale);
	}
}

void ALamplighterShip::SpawnPieces()
{
	Hull->SetRelativeScale3D(HullScale);
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (!ProwMarker)
	{
		ProwMarker = ALandfallMarker::SpawnTagged(World, LandfallRules::ProwTag(), DeckPoint(ProwMarkerLocal), Deck.Get());
	}
	if (bSpawnCrew && Crew.IsEmpty())
	{
		for (const FStation& Station : Stations)
		{
			const FTransform At(GetActorRotation() + FRotator(0.0, Station.Yaw, 0.0), DeckPoint(Station.Spot));
			AWardenCrewMember* Member = World->SpawnActorDeferred<AWardenCrewMember>(AWardenCrewMember::StaticClass(), At, this, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!Member)
			{
				continue;
			}
			Member->Crew = Station.Crew;
			// Brother Finch goes ashore at Main 4's turn-in to keep Saint Ada lit by hand (A Coal Under Ash; build_area_main4.py
			// stands him at the Reliquary from then): the ship's Finch is aboard until it.
			if (Station.Crew == EWardenCrew::Keeper)
			{
				Member->ShownWhen.BeforeMissions = { FName(TEXT("Main4")) };
			}
			Member->FinishSpawning(At);
			// Standing on the spot whatever its origin: a capsule's is its middle.
			if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Member->GetRootComponent()))
			{
				Member->AddActorWorldOffset(FVector(0.0, 0.0, Capsule->GetScaledCapsuleHalfHeight()));
			}
			// Carried by her, not walking: a character's own movement would fight the deck it's fixed to.
			if (ACharacter* Body = Cast<ACharacter>(Member))
			{
				if (UCharacterMovementComponent* Movement = Body->GetCharacterMovement())
				{
					Movement->DisableMovement();
				}
			}
			Member->AttachToComponent(Deck.Get(), FAttachmentTransformRules::KeepWorldTransform);
			Crew.Add(Member);
		}
	}
	if (!LanternFlame && GreatLantern)
	{
		const FTransform Wick = GreatLantern->DoesSocketExist(TEXT("Light")) ? GreatLantern->GetSocketTransform(TEXT("Light"))
			: GreatLantern->GetComponentTransform();
		ALanternFlame* Flame = World->SpawnActorDeferred<ALanternFlame>(ALanternFlame::StaticClass(), FTransform(Wick.GetLocation()), this);
		if (Flame)
		{
			// Lit from the start, whatever the story (the Great Lantern never goes out), and the Great Lantern's size.
			Flame->ShownWhen = FStoryCondition();
			Flame->bLit = true;
			Flame->Length = 10.f * LanternFlameScale;
			Flame->Width = 4.f * LanternFlameScale;
			Flame->LeanDegrees = 48.f;
			Flame->Strength = 30.f;
			Flame->FinishSpawning(FTransform(Wick.GetLocation()));
			Flame->AttachToComponent(GreatLantern, FAttachmentTransformRules::KeepWorldTransform, TEXT("Light"));
			LanternFlame = Flame;
		}
	}
	LeanLantern();
}

void ALamplighterShip::LeanLantern()
{
	if (!LanternFlame)
	{
		return;
	}
	AActor* Target = LanternTargetActor.Get();
	if (!Target && !LanternTarget.IsNone())
	{
		LanternSearchWait -= 0.5f;
		if (LanternSearchWait <= 0.f)
		{
			// Looked for now and then only: a level walk is a few thousand actors.
			LanternSearchWait = 5.f;
			for (TActorIterator<AActor> It(GetWorld()); It && !Target; ++It)
			{
				if (It->ActorHasTag(LanternTarget))
				{
					Target = *It;
				}
			}
			LanternTargetActor = Target;
		}
	}
	if (Target)
	{
		const FVector Toward = Target->GetActorLocation() - LanternFlame->GetActorLocation();
		LanternFlame->Bearing = FMath::RadiansToDegrees(FMath::Atan2(Toward.Y, Toward.X));
	}
	LanternFlame->Lean();
}

void ALamplighterShip::ShowGangway(bool bShown)
{
	const float SideSign = GangwaySide >= 0.f ? 1.f : -1.f;
	// The rail's length the gangway stands in opens; every other one is shut (a jump moved it, or she's flying again).
	const int32 Spans = static_cast<int32>(UE_ARRAY_COUNT(SideSpans));
	const float At = FMath::Clamp(GangwayAt, static_cast<float>(SideSpans[0].X) + 50.f, static_cast<float>(SideSpans[Spans - 1].Y) - 50.f);
	int32 GangwaySpan = 0;
	for (int32 Span = 0; Span < Spans; ++Span)
	{
		if (At >= SideSpans[Span].X && At <= SideSpans[Span].Y)
		{
			GangwaySpan = Span;
		}
	}
	for (int32 Index = 0; Index < 2 * Spans && Index < Rails.Num(); ++Index)
	{
		const bool bOpening = bShown && Index == (SideSign < 0.f ? 0 : Spans) + GangwaySpan;
		if (Rails[Index])
		{
			Rails[Index]->SetCollisionEnabled(bOpening ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
		}
	}
	if (!Gangway)
	{
		return;
	}
	// The plank reaches along its own -Y from its hinge (Skiff.py), so on the starboard side it's turned about; tipped down
	// to the ground from her deck.
	const float Reach = PlankLength * GangwayScale;
	const float Down = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(DeckTop / Reach, 0.f, 1.f)));
	const FQuat Facing(FVector::UpVector, SideSign < 0.f ? 0.0 : UE_PI);
	Gangway->SetRelativeLocationAndRotation(FVector(At, SideSign * RailOut, DeckTop), Facing * FQuat(FVector::XAxisVector, FMath::DegreesToRadians(Down)));
	Gangway->SetRelativeScale3D(FVector(GangwayScale));
	Gangway->SetVisibility(bShown);
	if (bShown)
	{
		Gangway->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Gangway->SetCollisionObjectType(ECC_WorldStatic);
	}
	else
	{
		Gangway->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

UPrimitiveComponent* ALamplighterShip::GetDeck() const
{
	return Deck.Get();
}

FVector ALamplighterShip::DeckPoint(const FVector& Local) const
{
	return GetActorTransform().TransformPositionNoScale(FVector(Local.X, Local.Y, DeckTop + Local.Z));
}

void ALamplighterShip::BraceCrew(float Seconds)
{
	for (AWardenCrewMember* Member : Crew)
	{
		if (IsValid(Member))
		{
			Member->Brace(Seconds);
		}
	}
}

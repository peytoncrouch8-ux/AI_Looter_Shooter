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

	// --- Her deck, in her frame (cm; the keel's middle at 0, the bow along +X) ---
	// Measured on the Tripo hull (Docs/Work/2026-11-04-Art/COLLISION.md: rays down every 10 cm over her deck): the main deck
	// is crowned, 10 cm over DeckTop on her centreline falling 4.6 degrees to -18 at the bulwarks; aft of x -250 it rises
	// to +50 at the wheel, the crown flattening; the forecastle is 1 m up, rising 6 degrees to the Lantern, its aft edge an
	// arc (x 1050 on the centreline, 960 at the sides) with a notch on the centreline down to the main deck.

	/** A floor piece: its rectangle in plan (her frame) and its top, the plane z = DeckTop + Rise + SlopeX x + SlopeY y; the
	 *  box reaches Depth under its top. Each was fitted to the hull's deck (within 3 cm on 86-100% of the walk area). */
	struct FDeckPiece
	{
		const TCHAR* Name;
		float X0, X1, Y0, Y1;
		float Rise, SlopeX, SlopeY;
		float Depth;
	};
	/** The pieces overlap their neighbours by this much in plan, so no seam opens between two. */
	constexpr float PieceOverlap = 5.f;
	/** Deck (the crown strip, the floor the player rides), SternDeck (at the wheel) and ProwDeck (the forecastle, 1 m up: a
	 *  solid block from the main deck, not climbed; Finch stands on it) are the named ones; the forecastle's arc is stepped
	 *  round in four more. */
	const FDeckPiece DeckPieces[] = {
		{ TEXT("Deck"), -250.f, 820.f, -40.f, 40.f, 10.51f, -0.00270f, 0.f, 30.f },
		{ TEXT("SternDeck"), -1163.f, -1000.f, -250.f, 250.f, -40.56f, -0.08093f, 0.01020f, 40.f },
		{ TEXT("ProwDeck"), 1050.f, 1300.f, -240.f, 240.f, -11.37f, 0.10849f, 0.f, 125.f },
		{ TEXT("DeckMainPort"), -250.f, 820.f, -360.f, -40.f, 15.52f, -0.00672f, 0.08032f, 30.f },
		{ TEXT("DeckMainStarboard"), -250.f, 820.f, 40.f, 360.f, 16.19f, -0.00752f, -0.08269f, 30.f },
		{ TEXT("DeckFore"), 820.f, 1060.f, -40.f, 40.f, 2.05f, 0.00792f, 0.f, 30.f },
		{ TEXT("DeckForePort"), 820.f, 1060.f, -340.f, -40.f, 5.34f, 0.00671f, 0.05729f, 30.f },
		{ TEXT("DeckForeStarboard"), 820.f, 1060.f, 40.f, 340.f, 4.40f, 0.00837f, -0.06084f, 30.f },
		{ TEXT("ProwArcPort"), 1000.f, 1050.f, -260.f, -140.f, -11.37f, 0.10849f, 0.f, 115.f },
		{ TEXT("ProwArcStarboard"), 1000.f, 1050.f, 140.f, 260.f, -11.37f, 0.10849f, 0.f, 115.f },
		{ TEXT("ProwArcPortAft"), 960.f, 1000.f, -260.f, -200.f, -11.37f, 0.10849f, 0.f, 110.f },
		{ TEXT("ProwArcStarboardAft"), 960.f, 1000.f, 200.f, 260.f, -11.37f, 0.10849f, 0.f, 110.f },
		{ TEXT("DeckAftPort"), -500.f, -250.f, -360.f, 0.f, 8.61f, -0.02372f, 0.05934f, 30.f },
		{ TEXT("DeckAftStarboard"), -500.f, -250.f, 0.f, 360.f, 9.59f, -0.02327f, -0.06401f, 30.f },
		{ TEXT("DeckWaistPort"), -750.f, -500.f, -340.f, 0.f, -0.09f, -0.03737f, 0.04705f, 30.f },
		{ TEXT("DeckWaistStarboard"), -750.f, -500.f, 0.f, 340.f, -1.06f, -0.04021f, -0.04982f, 30.f },
		{ TEXT("DeckQuarterPort"), -1000.f, -750.f, -300.f, 0.f, -29.92f, -0.07150f, 0.01835f, 30.f },
		{ TEXT("DeckQuarterStarboard"), -1000.f, -750.f, 0.f, 300.f, -32.39f, -0.07451f, -0.02051f, 30.f },
	};

	/** What stands on her deck, solid to walk round and to shots (the hull itself has no collision): the console and its
	 *  engine, the starboard locker, the port chest, the wheel house aft. Plan rectangle and top over DeckTop (cm). */
	struct FDeckBlock
	{
		const TCHAR* Name;
		float X0, X1, Y0, Y1, Top;
	};
	const FDeckBlock DeckBlocks[] = {
		{ TEXT("Console"), -240.f, 205.f, -132.f, 137.f, 135.f },
		{ TEXT("ConsoleEngine"), -185.f, 25.f, -115.f, 45.f, 300.f },
		{ TEXT("Locker"), -175.f, 215.f, 288.f, 380.f, 112.f },
		{ TEXT("Chest"), -525.f, -215.f, -281.f, -227.f, 65.f },
		{ TEXT("WheelHouse"), -1055.f, -870.f, 5.f, 180.f, 203.f },
	};

	/** The rail walls: this tall above the deck (more than a jump), this thick. */
	constexpr float WallHeight = 320.f;
	constexpr float WallHalfThick = 15.f;
	/** The side rails in three lengths each (aft, the gangway's middle, fore), from X to X; the gangway's opens. */
	const FVector2D SideSpans[] = { FVector2D(-1163.0, -250.0), FVector2D(-250.0, 150.0), FVector2D(150.0, 1055.0) };
	/** Her bulwark's inner face, out from her centreline (both sides alike), measured 50 cm over the deck: each side's rail
	 *  is a wall along this line, so the player walks to the bulwark she sees (no further: the bow and stern curve in). */
	const FVector2D BulwarkLine[] = {
		FVector2D(-1163.0, 215.0), FVector2D(-1100.0, 227.0), FVector2D(-1000.0, 255.0), FVector2D(-900.0, 280.0),
		FVector2D(-800.0, 301.0), FVector2D(-700.0, 321.0), FVector2D(-600.0, 337.0), FVector2D(-500.0, 354.0),
		FVector2D(-400.0, 366.0), FVector2D(-250.0, 379.0), FVector2D(-150.0, 386.0), FVector2D(0.0, 392.0),
		FVector2D(150.0, 388.0), FVector2D(300.0, 381.0), FVector2D(450.0, 367.0), FVector2D(550.0, 352.0),
		FVector2D(650.0, 335.0), FVector2D(750.0, 312.0), FVector2D(850.0, 286.0), FVector2D(950.0, 233.0),
		FVector2D(1000.0, 184.0) };
	/** A side rail's tag: which side and which of SideSpans it stands in (ShowGangway opens the gangway's). */
	FName SideRailTag(float SideSign, int32 Span)
	{
		return FName(*FString::Printf(TEXT("SideRail_%s_%d"), SideSign < 0.f ? TEXT("Port") : TEXT("Starboard"), Span));
	}

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
	/** The engine works amidships, forward of the hull's console and clear of the bag's struts (on the crowned deck, 8 cm up). */
	const FVector EngineSpots[] = { FVector(450.0, -55.0, 8.0), FVector(450.0, 55.0, 8.0) };
	/** Her crown under the arrival spot is 10 cm over DeckTop. */
	const FVector LandingLocal(300.0, 0.0, 10.0);
	const FVector ProwMarkerLocal(990.0, 0.0, 90.0);
	/** On the bag's top (15 m over the deck). */
	const FVector HobPerchLocal(250.0, 0.0, 1512.0);
	/** The gangway plank's own length (SM_Skiff_Gangplank, 3 m) and its scale. */
	constexpr float PlankLength = 300.f;
	constexpr float GangwayScale = 1.3f;

	/** The crew's stations: Holloway at the wheel, Finch by the Great Lantern, Junie at the engine (her frame, yaw; Z the deck
	 *  measured under each: the stern deck 50 cm up at the wheel, the forecastle 120). */
	struct FStation
	{
		EWardenCrew Crew;
		FVector Spot;
		float Yaw;
	};
	const FStation Stations[] = {
		{ EWardenCrew::Captain, FVector(-1120.0, 0.0, 50.0), 0.f },
		{ EWardenCrew::Keeper, FVector(1210.0, 105.0, 120.0), -25.f },
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

	/** Something standing on her deck: solid to walk round and to shots, but not a floor the creatures' ground traces take. */
	void MakeBlock(UBoxComponent& Box)
	{
		Box.SetMobility(EComponentMobility::Movable);
		Box.SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Box.SetCollisionObjectType(ECC_WorldDynamic);
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
	// Her floor: each piece tilted onto its plane, its top on the hull's deck. Deck, SternDeck and ProwDeck are three of them.
	for (const FDeckPiece& Piece : DeckPieces)
	{
		UBoxComponent* Box = CreateDefaultSubobject<UBoxComponent>(Piece.Name);
		Box->SetupAttachment(Root);
		const double CX = (Piece.X0 + Piece.X1) * 0.5;
		const double CY = (Piece.Y0 + Piece.Y1) * 0.5;
		const FVector Up = FVector(-Piece.SlopeX, -Piece.SlopeY, 1.0).GetSafeNormal();
		const FVector Top(CX, CY, DeckTop + Piece.Rise + Piece.SlopeX * CX + Piece.SlopeY * CY);
		const FVector Extent((Piece.X1 - Piece.X0) * 0.5 + PieceOverlap, (Piece.Y1 - Piece.Y0) * 0.5 + PieceOverlap, Piece.Depth * 0.5);
		Box->SetBoxExtent(Extent, false);
		Box->SetRelativeLocationAndRotation(Top - Up * Extent.Z, FRotationMatrix::MakeFromZX(Up, FVector::XAxisVector).Rotator());
		MakeFloor(*Box);
		const FName Name(Piece.Name);
		if (Name == FName(TEXT("Deck")))
		{
			Deck = Box;
		}
		else if (Name == FName(TEXT("SternDeck")))
		{
			SternDeck = Box;
		}
		else if (Name == FName(TEXT("ProwDeck")))
		{
			ProwDeck = Box;
		}
	}
	for (const FDeckBlock& Block : DeckBlocks)
	{
		UBoxComponent* Box = CreateDefaultSubobject<UBoxComponent>(Block.Name);
		Box->SetupAttachment(Root);
		// From 30 cm under DeckTop (under the crowned deck everywhere) to its top.
		const double Bottom = DeckTop - 30.0;
		const double TopZ = DeckTop + Block.Top;
		Box->SetBoxExtent(FVector((Block.X1 - Block.X0) * 0.5, (Block.Y1 - Block.Y0) * 0.5, (TopZ - Bottom) * 0.5), false);
		Box->SetRelativeLocation(FVector((Block.X0 + Block.X1) * 0.5, (Block.Y0 + Block.Y1) * 0.5, (TopZ + Bottom) * 0.5));
		MakeBlock(*Box);
	}

	const double WallZ = DeckTop + WallHeight * 0.5;
	auto AddWall = [this, WallZ](const TCHAR* Name, const FVector2D& Center, const FVector2D& Half, float Yaw = 0.f) -> UBoxComponent*
	{
		UBoxComponent* Wall = CreateDefaultSubobject<UBoxComponent>(Name);
		Wall->SetupAttachment(Root);
		Wall->SetBoxExtent(FVector(Half.X, Half.Y, WallHeight * 0.5f), false);
		Wall->SetRelativeLocationAndRotation(FVector(Center.X, Center.Y, WallZ), FRotator(0.0, Yaw, 0.0));
		MakeWall(*Wall);
		Rails.Add(Wall);
		return Wall;
	};
	// Each side's rail: a wall along each stretch of her bulwark line (just outside it), tagged with its side and span.
	for (const float SideSign : { -1.f, 1.f })
	{
		for (int32 Index = 0; Index + 1 < UE_ARRAY_COUNT(BulwarkLine); ++Index)
		{
			const FVector2D A(BulwarkLine[Index].X, SideSign * BulwarkLine[Index].Y);
			const FVector2D B(BulwarkLine[Index + 1].X, SideSign * BulwarkLine[Index + 1].Y);
			const FVector2D Along = (B - A).GetSafeNormal();
			FVector2D Out(-Along.Y, Along.X);
			if (Out.Y * SideSign < 0.0)
			{
				Out = -Out;
			}
			const FVector2D Middle = (A + B) * 0.5 + Out * WallHalfThick;
			int32 Span = 0;
			for (int32 S = 0; S < UE_ARRAY_COUNT(SideSpans); ++S)
			{
				Span = Middle.X >= SideSpans[S].X ? S : Span;
			}
			// Each stretch a little longer than its line, so the corners between two close.
			UBoxComponent* Wall = AddWall(*FString::Printf(TEXT("Rail_%s_%d"), SideSign < 0.f ? TEXT("Port") : TEXT("Starboard"), Index),
				Middle, FVector2D((B - A).Size() * 0.5 + WallHalfThick, WallHalfThick),
				static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Along.Y, Along.X))));
			Wall->ComponentTags.Add(SideRailTag(SideSign, Span));
		}
	}
	AddWall(TEXT("Rail_Stern"), FVector2D(SideSpans[0].X - WallHalfThick, 0.0), FVector2D(WallHalfThick, RailOut + WallHalfThick));
	// Finch's walk on the forecastle (x 1055..1263, |y| <= 200) closed by its sides and front; its shoulders close the main
	// deck's rails in to it (the forecastle's own pieces block the main deck's side of them).
	const FVector2D Forecastle(1159.0, 104.0);
	const double ForecastleOut = 200.0;
	const double Shoulder = (RailOut + ForecastleOut) * 0.5;
	AddWall(TEXT("Rail_ShoulderPort"), FVector2D(SideSpans[2].Y, -Shoulder), FVector2D(WallHalfThick, (RailOut - ForecastleOut) * 0.5));
	AddWall(TEXT("Rail_ShoulderStarboard"), FVector2D(SideSpans[2].Y, Shoulder), FVector2D(WallHalfThick, (RailOut - ForecastleOut) * 0.5));
	AddWall(TEXT("Rail_ProwPort"), FVector2D(Forecastle.X, -(ForecastleOut + WallHalfThick)), FVector2D(Forecastle.Y, WallHalfThick));
	AddWall(TEXT("Rail_ProwStarboard"), FVector2D(Forecastle.X, ForecastleOut + WallHalfThick), FVector2D(Forecastle.Y, WallHalfThick));
	AddWall(TEXT("Rail_ProwFront"), FVector2D(Forecastle.X + Forecastle.Y + WallHalfThick, 0.0),
		FVector2D(WallHalfThick, ForecastleOut + 2.0 * WallHalfThick));

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
	// The side rails are stretches along her bulwark, each tagged with its side and span (BuildRails).
	const FName Opening = SideRailTag(SideSign, GangwaySpan);
	for (UBoxComponent* Wall : Rails)
	{
		bool bSideRail = false;
		for (int32 Span = 0; Span < Spans && Wall && !bSideRail; ++Span)
		{
			bSideRail = Wall->ComponentHasTag(SideRailTag(-1.f, Span)) || Wall->ComponentHasTag(SideRailTag(1.f, Span));
		}
		if (bSideRail)
		{
			const bool bOpening = bShown && Wall->ComponentHasTag(Opening);
			Wall->SetCollisionEnabled(bOpening ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
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

// ALamplighterShip: her pieces: the stand-in hull, her walkable deck and rail walls, the Great Lantern and its flame, the
// wheel and the engine, the prow's marker and the crew at their stations (their numbers: LamplighterShipKit.h; the gangway,
// the lantern's lean and the crew's bracing: LamplighterShipGangway.cpp).

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

#include "World/LamplighterShipKit.h"

using namespace LamplighterShipKit;

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
			if (SideSign > 0.f && FMath::Max(A.X, B.X) > PlankHingeX - PlankOpenHalf && FMath::Min(A.X, B.X) < PlankHingeX + PlankOpenHalf)
			{
				Wall->ComponentTags.Add(PlankRailTag());
			}
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


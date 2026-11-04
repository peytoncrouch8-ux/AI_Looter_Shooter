// ABargeBoarding: her deck kit, built in code in her own frame (BargeRules). The reworked Tripo hull carries its own deck,
// bulwark, house, stacks and tower with their collision, so the kit adds only what it hasn't got: the unseen rail walls on
// the bulwark's line (nobody climbs it and falls), the vestibule's floor, cover, the vents, the falling third stack, the
// work lamp; and the stand-in's deck plate and rails when the hull isn't there. The vestibule's walls, the office
// and the furnace hall in the house are BargeBoardingHall.cpp. Collision is plain boxes (hidden); the looks are a few
// instanced meshes and the stand-in shapes.

#include "World/BargeBoarding.h"
#include "World/BargeLooks.h"
#include "World/BargeRules.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

UStaticMeshComponent* ABargeBoarding::MakePiece(const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Look, const FVector& Center,
	const FVector& Size, const FRotator& Turn, bool bWalkable)
{
	UStaticMeshComponent* Piece = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Piece->SetupAttachment(Kit);
	Piece->SetMobility(EComponentMobility::Movable);
	Piece->SetStaticMesh(Mesh);
	if (Look)
	{
		Piece->SetMaterial(0, Look);
	}
	Piece->SetRelativeLocationAndRotation(Center, Turn);
	Piece->SetRelativeScale3D(Size / 100.0);
	Piece->SetCanEverAffectNavigation(false);
	if (bWalkable)
	{
		// World-static, as the ground is: creatures' ground traces and the spawners' probes find it.
		Piece->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Piece->SetCollisionObjectType(ECC_WorldStatic);
	}
	else
	{
		Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	return Piece;
}

UBoxComponent* ABargeBoarding::MakeWall(const TCHAR* Name, const FVector& Center, const FVector& Half, const FRotator& Turn)
{
	// A solid wall or floor (the hall's iron): it stops pawns and shots alike; its look is instanced apart.
	UBoxComponent* Box = CreateDefaultSubobject<UBoxComponent>(Name);
	Box->SetupAttachment(Kit);
	Box->SetMobility(EComponentMobility::Movable);
	Box->SetBoxExtent(Half, false);
	Box->SetRelativeLocationAndRotation(Center, Turn);
	Box->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Box->SetCollisionObjectType(ECC_WorldStatic);
	Box->SetCanEverAffectNavigation(false);
	Box->SetGenerateOverlapEvents(false);
	Box->SetHiddenInGame(true);
	return Box;
}

UBoxComponent* ABargeBoarding::MakeBlock(const TCHAR* Name, const FBox2D& Footprint, float Up)
{
	// Superstructure as the kit has it: nothing drawn (the hull is the look), everything stopped.
	const FVector2D Middle = Footprint.GetCenter();
	const FVector2D Half = Footprint.GetExtent();
	return MakeWall(Name, FVector(Middle.X, Middle.Y, BargeRules::DeckTop + Up * 0.5), FVector(Half.X, Half.Y, Up * 0.5));
}

UInstancedStaticMeshComponent* ABargeBoarding::MakeInstanced(const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Look)
{
	UInstancedStaticMeshComponent* Looks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
	Looks->SetupAttachment(Kit);
	Looks->SetMobility(EComponentMobility::Movable);
	Looks->SetStaticMesh(Mesh);
	if (Look)
	{
		Looks->SetMaterial(0, Look);
	}
	Looks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Looks->SetCanEverAffectNavigation(false);
	Looks->bReceivesDecals = false;
	return Looks;
}

void ABargeBoarding::BuildKit()
{
	static UStaticMesh* const CubeMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cube);
	static UStaticMesh* const CylinderMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cylinder);
	static UStaticMesh* const ConeMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cone);
	static UStaticMesh* const SphereMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Sphere);
	static UStaticMesh* const TripoCoal = BargeLooks::Find<UStaticMesh>(BargeLooks::TripoSoulCoal);
	static UStaticMesh* const TripoHose = BargeLooks::Find<UStaticMesh>(BargeLooks::TripoHose);
	static UStaticMesh* const TripoVent = BargeLooks::Find<UStaticMesh>(BargeLooks::TripoVent);
	static UMaterialInterface* const Iron = BargeLooks::Find<UMaterialInterface>(BargeLooks::Iron);
	static UMaterialInterface* const IronBlack = BargeLooks::Find<UMaterialInterface>(BargeLooks::IronBlack);
	static UMaterialInterface* const Glow = BargeLooks::Find<UMaterialInterface>(BargeLooks::Glow);
	static UMaterialInterface* const Rubber = BargeLooks::Find<UMaterialInterface>(BargeLooks::Rubber);

	RailLook = MakeInstanced(TEXT("RailLook"), CubeMesh, IronBlack);
	CoverLook = MakeInstanced(TEXT("CoverLook"), CubeMesh, Iron);
	VentCold = MakeInstanced(TEXT("VentCold"), TripoVent ? TripoVent : CubeMesh, TripoVent ? nullptr : IronBlack);
	VentGlow = MakeInstanced(TEXT("VentGlow"), TripoVent ? TripoVent : CubeMesh, Glow);
	VentFire = MakeInstanced(TEXT("VentFire"), ConeMesh, Glow);
	HallWallLook = MakeInstanced(TEXT("HallWallLook"), CubeMesh, Iron);
	CatwalkLook = MakeInstanced(TEXT("CatwalkLook"), CubeMesh, IronBlack);
	HoseSections = MakeInstanced(TEXT("HoseSections"), TripoHose ? TripoHose : CylinderMesh, TripoHose ? nullptr : Rubber);
	SternFire = MakeInstanced(TEXT("SternFire"), ConeMesh, Glow);
	CoalRain = MakeInstanced(TEXT("CoalRain"), TripoCoal ? TripoCoal : SphereMesh, TripoCoal ? nullptr : Glow);
	// Her hose and the coal hang in the world's frame (laid world-space each frame), the rest in hers.
	for (UInstancedStaticMeshComponent* InWorld : { HoseSections.Get(), CoalRain.Get() })
	{
		InWorld->SetUsingAbsoluteLocation(true);
		InWorld->SetUsingAbsoluteRotation(true);
		InWorld->SetUsingAbsoluteScale(true);
		InWorld->SetCastShadow(false);
	}
	for (UInstancedStaticMeshComponent* Fire : { VentGlow.Get(), VentFire.Get(), SternFire.Get() })
	{
		Fire->SetCastShadow(false);
	}

	BuildDeck();
	BuildVestibule();
	BuildHall();

	// The office's radio set on the desk's corner: Ned's call comes out of it.
	const FVector Desk = BargeRules::DeskTop();
	Radio = MakePiece(TEXT("Radio"), CubeMesh, IronBlack, Desk + BargeRules::RadioOnDesk, FVector(40.0, 30.0, 30.0), FRotator::ZeroRotator, false);

	// The work lamp over the house's door: shadowless, warm; out as she dies.
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(BargeRules::DeckLampSpots)); ++Index)
	{
		UPointLightComponent* Lamp = CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("DeckLamp%d"), Index));
		Lamp->SetupAttachment(Kit);
		Lamp->SetMobility(EComponentMobility::Movable);
		Lamp->SetRelativeLocation(BargeRules::DeckLampSpots[Index]);
		Lamp->SetIntensityUnits(ELightUnits::Candelas);
		Lamp->SetIntensity(BargeRules::DeckLampCandelas);
		Lamp->SetAttenuationRadius(BargeRules::DeckLampReach);
		Lamp->SetLightColor(BargeRules::DeckLampColor);
		Lamp->SetCastShadows(false);
		DeckLamps.Add(Lamp);
	}
}

void ABargeBoarding::BuildDeck()
{
	static UStaticMesh* const CubeMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cube);
	static UStaticMesh* const CylinderMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cylinder);
	static UStaticMesh* const TripoStack = BargeLooks::Find<UStaticMesh>(BargeLooks::TripoStack);
	static UStaticMesh* const HullMesh = BargeLooks::Find<UStaticMesh>(HullPath);
	static UMaterialInterface* const Iron = BargeLooks::Find<UMaterialInterface>(BargeLooks::Iron);
	static UMaterialInterface* const IronBlack = BargeLooks::Find<UMaterialInterface>(BargeLooks::IronBlack);

	const bool bHull = HullMesh != nullptr;
	const float Top = BargeRules::DeckTop;
	const float T = BargeRules::PlateThickness;
	// The kit's floor plates: the vestibule's always (a hair over the deck, inside the house); the open deck's only over the
	// stand-in box (the hull's own deck is walked and shot on as it is).
	int32 PlateIndex = 0;
	for (const BargeRules::FDeckPlate& Plate : BargeRules::DeckPlates)
	{
		if (Plate.bOnHullDeck && bHull)
		{
			continue;
		}
		const FBox2D Piece(FVector2D(Plate.XMin, Plate.YMin), FVector2D(Plate.XMax, Plate.YMax));
		const float PlateTop = Top + (Plate.bOnHullDeck ? 0.f : BargeRules::InteriorFloorLift);
		PlateParts.Add(MakePiece(*FString::Printf(TEXT("DeckPlate%d"), PlateIndex++), CubeMesh, Iron,
			FVector(Piece.GetCenter().X, Piece.GetCenter().Y, PlateTop - T * 0.5), FVector(Piece.GetSize().X, Piece.GetSize().Y, T)));
	}

	// The rail walls (BargeRules::RailWallLayout): unseen, pawns only (shots and the camera pass over), standing on the
	// bulwark's line over its 110 cm so nobody climbs it and falls, open at its gap on the port side where the plank lands and
	// the jump is made; and across the bow.
	const float WallZ = Top + BargeRules::RailWallHeight * 0.5f;
	for (const BargeRules::FRailWall& Layout : BargeRules::RailWallLayout())
	{
		const FVector2D Middle = Layout.Footprint.GetCenter();
		const FVector2D Half = Layout.Footprint.GetExtent();
		UBoxComponent* Wall = CreateDefaultSubobject<UBoxComponent>(Layout.Name);
		Wall->SetupAttachment(Kit);
		Wall->SetMobility(EComponentMobility::Movable);
		Wall->SetBoxExtent(FVector(Half.X, Half.Y, BargeRules::RailWallHeight * 0.5), false);
		Wall->SetRelativeLocation(FVector(Middle.X, Middle.Y, WallZ));
		Wall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Wall->SetCollisionObjectType(ECC_WorldDynamic);
		Wall->SetCollisionResponseToAllChannels(ECR_Ignore);
		Wall->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Wall->SetCanEverAffectNavigation(false);
		Wall->SetGenerateOverlapEvents(false);
		Wall->SetHiddenInGame(true);
		RailWalls.Add(Wall);
	}

	// Superstructure a hull without its own collision would need walled off (none for ours).
	int32 BlockIndex = 0;
	for (const BargeRules::FDeckBlock& Block : BargeRules::DeckBlocks())
	{
		BlockWalls.Add(MakeBlock(*FString::Printf(TEXT("DeckBlock%d"), BlockIndex++),
			FBox2D(FVector2D(Block.XMin, Block.YMin), FVector2D(Block.XMax, Block.YMax)), Block.Up));
	}

	// Cover: crates and plate stacks the riflemen crouch behind (instanced look, a box each to stop shots and boots).
	int32 CoverIndex = 0;
	for (const FVector& Spot : BargeRules::CoverSpots)
	{
		MakeWall(*FString::Printf(TEXT("Cover%d"), CoverIndex++), Spot + FVector(0.0, 0.0, BargeRules::CoverHalf.Z), BargeRules::CoverHalf);
	}

	// The kit's third stack against the house's fore face, the one that falls in step 7, turned about its foot (the hull's
	// own two stand on the house's roof).
	FallingStackPivot = CreateDefaultSubobject<USceneComponent>(TEXT("FallingStackPivot"));
	FallingStackPivot->SetupAttachment(Kit);
	FallingStackPivot->SetMobility(EComponentMobility::Movable);
	FallingStackPivot->SetRelativeLocation(BargeRules::FallingStackFoot);
	FallingStack = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FallingStack"));
	FallingStack->SetupAttachment(FallingStackPivot);
	FallingStack->SetMobility(EComponentMobility::Movable);
	FallingStack->SetStaticMesh(TripoStack ? TripoStack : CylinderMesh);
	if (!TripoStack)
	{
		FallingStack->SetMaterial(0, IronBlack);
	}
	FallingStack->SetRelativeLocation(FVector(0.0, 0.0, BargeRules::StackHeight * 0.5));
	FallingStack->SetRelativeScale3D(FVector(BargeRules::StackRadius * 2.0 / 100.0, BargeRules::StackRadius * 2.0 / 100.0, BargeRules::StackHeight / 100.0));
	FallingStack->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	FallingStack->SetCollisionObjectType(ECC_WorldStatic);
	FallingStack->SetCanEverAffectNavigation(false);
}

void ABargeBoarding::LayStaticLooks()
{
	if (!RailLook || !CoverLook || !HallWallLook || !CatwalkLook || !VentCold)
	{
		return;
	}
	const float Top = BargeRules::DeckTop;
	// The visible rail: a low iron rail in 5 m spans along the bulwark's line, open at the berth's gap, over the stand-in box
	// only (the hull has its bulwark: BargeRules::bDrawRails draws it anyway).
	TArray<FTransform> Rails;
	const auto RailRun = [&Rails, Top](const FVector& From, const FVector& To)
	{
		const FVector Run = To - From;
		const int32 Spans = FMath::Max(1, FMath::CeilToInt32(Run.Size() / 500.0));
		for (int32 Span = 0; Span < Spans; ++Span)
		{
			const FVector A = From + Run * (static_cast<double>(Span) / Spans);
			const FVector B = From + Run * (static_cast<double>(Span + 1) / Spans);
			Rails.Add(FTransform(FRotator(0.f, static_cast<float>(Run.Rotation().Yaw), 0.f), (A + B) * 0.5 + FVector(0.0, 0.0, Top + BargeRules::RailHeight - 6.0),
				FVector((B - A).Size() / 100.0, 0.1, 0.12)));
			Rails.Add(FTransform(FQuat::Identity, A + FVector(0.0, 0.0, Top + BargeRules::RailHeight * 0.5), FVector(0.08, 0.08, BargeRules::RailHeight / 100.0)));
		}
	};
	if (!HasHull() || BargeRules::bDrawRails)
	{
		const float Side = BargeRules::BerthSide;
		const float Line = BargeRules::BulwarkHalfBeam - BargeRules::BulwarkThick * 0.5f;
		const float Aft = BargeRules::HouseFore;
		RailRun(FVector(Aft, -Side * Line, 0.0), FVector(BargeRules::DeckFore, -Side * Line, 0.0));
		RailRun(FVector(Aft, Side * Line, 0.0), FVector(BargeRules::BerthX - BargeRules::GapHalf, Side * Line, 0.0));
		RailRun(FVector(BargeRules::BerthX + BargeRules::GapHalf, Side * Line, 0.0), FVector(BargeRules::DeckFore, Side * Line, 0.0));
		RailRun(FVector(BargeRules::DeckFore, -Line, 0.0), FVector(BargeRules::DeckFore, Line, 0.0));
	}
	RailLook->ClearInstances();
	RailLook->AddInstances(Rails, /*bShouldReturnIndices*/ false);

	TArray<FTransform> Cover;
	for (const FVector& Spot : BargeRules::CoverSpots)
	{
		Cover.Add(FTransform(FQuat::Identity, Spot + FVector(0.0, 0.0, BargeRules::CoverHalf.Z), BargeRules::CoverHalf / 50.0));
	}
	CoverLook->ClearInstances();
	CoverLook->AddInstances(Cover, false);

	LayHallLooks();

	// The vents, all cold (the deck's tick lays their glow and fire).
	TArray<FTransform> Vents;
	for (const FVector& Spot : BargeRules::VentSpots)
	{
		Vents.Add(FTransform(FQuat::Identity, Spot + FVector(0.0, 0.0, BargeRules::VentGrilleUp),
			FVector(BargeRules::VentHalf / 50.0, BargeRules::VentHalf / 50.0, BargeRules::VentGrilleThick / 100.0)));
	}
	VentCold->ClearInstances();
	VentCold->AddInstances(Vents, false);
	VentGlow->ClearInstances();
	VentFire->ClearInstances();
	VentGlowMask = 0u;
	VentFireMask = 0u;
}

// ABargeBoarding: her walkable kit, built in code in her own frame (BargeRules): the top deck and its rails, the hatch and
// its stair, cover, stacks, the crane; the furnace hall below with its walls, catwalk ring, stairs, stokeholds and the
// stoker's office. Collision is plain boxes (hidden); the looks are a few instanced meshes and the stand-in shapes.

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

namespace
{
	/** The hatch's hole in the deck (the barge's frame): long enough along its stair's run for a man's head under the deck. */
	constexpr float HoleXMin = -2880.f;
	constexpr float HoleXMax = -2540.f;
	constexpr float HoleYMin = -60.f;
	constexpr float HoleYMax = 380.f;
	/** The stairs: tangent to the catwalk ring's middle (degrees round the hall), the hatch's from the deck to the catwalk,
	 *  two from the catwalk to the floor. */
	constexpr float RingMiddle = (BargeRules::CatwalkInner + BargeRules::HallRadius) * 0.5f;
	constexpr float StairWidth = 180.f;
	constexpr float StairThick = 24.f;
	struct FStair
	{
		float FromAngle;
		float ToAngle;
		float FromUp;
		float ToUp;
	};
	const FStair Stairs[] = {
		{ 180.f, 126.f, BargeRules::DeckTop - BargeRules::PlateThickness - BargeRules::HallFloor, BargeRules::CatwalkUp },
		{ 225.f, 260.f, BargeRules::CatwalkUp, 0.f },
		{ 315.f, 350.f, BargeRules::CatwalkUp, 0.f },
	};
	/** The hall's wall in panels this many degrees wide; the gaps (the office over the catwalk, the stokeholds' doors). */
	constexpr int32 Panels = 36;
	bool IsOfficeGap(float Angle) { return Angle < 10.f || Angle > 350.f; }
	bool IsStokeholdGap(float Angle) { return FMath::Abs(Angle - 90.f) < 10.f || FMath::Abs(Angle - 270.f) < 10.f; }
	/** The catwalk ring in this many straight spans. */
	constexpr int32 CatwalkSpans = 18;
	/** A stokehold's alcove: this deep past the wall (the hull's beam allows no more), this wide. */
	constexpr float AlcoveDepth = 120.f;
	constexpr float AlcoveHalfWidth = 190.f;
	/** The rail walls: tall and unseen, a hand's thickness. */
	constexpr float RailWallHalfThick = 15.f;

	FVector HallXY(float AngleDegrees, float Radius)
	{
		const float Turn = FMath::DegreesToRadians(AngleDegrees);
		return BargeRules::HallCenter + FVector(FMath::Cos(Turn) * Radius, FMath::Sin(Turn) * Radius, 0.0);
	}
}

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
		Walkables.Add(Piece);
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
	BuildHall();

	// The office's radio set on the desk's corner: Ned's call comes out of it.
	const FVector Desk = BargeRules::DeskTop();
	Radio = MakePiece(TEXT("Radio"), CubeMesh, IronBlack, Desk + BargeRules::RadioOnDesk, FVector(40.0, 30.0, 30.0), FRotator::ZeroRotator, false);

	// Two work lamps over the deck, on the first stack and the crane: shadowless, warm; out as she dies.
	for (int32 Index = 0; Index < 2; ++Index)
	{
		UPointLightComponent* Lamp = CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("DeckLamp%d"), Index));
		Lamp->SetupAttachment(Kit);
		Lamp->SetMobility(EComponentMobility::Movable);
		Lamp->SetRelativeLocation(Index == 0 ? BargeRules::StackSpots[0] + FVector(250.0, 0.0, 700.0)
			: BargeRules::CraneSpot + FVector(0.0, 0.0, BargeRules::CraneHeight - 80.0));
		Lamp->SetIntensityUnits(ELightUnits::Candelas);
		Lamp->SetIntensity(35.f);
		Lamp->SetAttenuationRadius(2600.f);
		Lamp->SetLightColor(FLinearColor(1.f, 0.72f, 0.42f));
		Lamp->SetCastShadows(false);
		DeckLamps.Add(Lamp);
	}
}

void ABargeBoarding::BuildDeck()
{
	static UStaticMesh* const CubeMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cube);
	static UStaticMesh* const CylinderMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cylinder);
	static UStaticMesh* const TripoStack = BargeLooks::Find<UStaticMesh>(BargeLooks::TripoStack);
	static UStaticMesh* const TripoCrane = BargeLooks::Find<UStaticMesh>(BargeLooks::TripoCrane);
	static UStaticMesh* const TripoHull = BargeLooks::Find<UStaticMesh>(HullPath);
	static UMaterialInterface* const Iron = BargeLooks::Find<UMaterialInterface>(BargeLooks::Iron);
	static UMaterialInterface* const IronBlack = BargeLooks::Find<UMaterialInterface>(BargeLooks::IronBlack);

	const float Top = BargeRules::DeckTop;
	const float T = BargeRules::PlateThickness;
	const float Beam = BargeRules::DeckHalfBeam;
	// The deck in four plates round the hatch's hole (riveted iron: the Tripo deck plate kit replaces the look later).
	const auto Plate = [&](const TCHAR* Name, float XMin, float XMax, float YMin, float YMax)
	{
		MakePiece(Name, CubeMesh, Iron, FVector((XMin + XMax) * 0.5, (YMin + YMax) * 0.5, Top - T * 0.5), FVector(XMax - XMin, YMax - YMin, T));
	};
	Plate(TEXT("DeckFore"), HoleXMax, BargeRules::DeckFore, -Beam, Beam);
	Plate(TEXT("DeckAft"), BargeRules::DeckAft, HoleXMin, -Beam, Beam);
	Plate(TEXT("DeckHatchStarboard"), HoleXMin, HoleXMax, HoleYMax, Beam);
	Plate(TEXT("DeckHatchPort"), HoleXMin, HoleXMax, -Beam, HoleYMin);

	// The rail walls: unseen, pawns only (shots and the camera pass over the low visible rail), with the berth's gap on the
	// port side where the plank lands and the jump is made.
	const float WallZ = Top + BargeRules::RailWallHeight * 0.5f;
	const float Side = BargeRules::BerthSide;
	const float GapFore = BargeRules::BerthX + BargeRules::GapHalf;
	const float GapAft = BargeRules::BerthX - BargeRules::GapHalf;
	const auto RailWall = [&](const TCHAR* Name, float XMin, float XMax, float YMin, float YMax)
	{
		UBoxComponent* Wall = CreateDefaultSubobject<UBoxComponent>(Name);
		Wall->SetupAttachment(Kit);
		Wall->SetMobility(EComponentMobility::Movable);
		Wall->SetBoxExtent(FVector((XMax - XMin) * 0.5, (YMax - YMin) * 0.5, BargeRules::RailWallHeight * 0.5), false);
		Wall->SetRelativeLocation(FVector((XMin + XMax) * 0.5, (YMin + YMax) * 0.5, WallZ));
		Wall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Wall->SetCollisionObjectType(ECC_WorldDynamic);
		Wall->SetCollisionResponseToAllChannels(ECR_Ignore);
		Wall->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Wall->SetCanEverAffectNavigation(false);
		Wall->SetGenerateOverlapEvents(false);
		Wall->SetHiddenInGame(true);
		RailWalls.Add(Wall);
	};
	const float Edge = Beam + RailWallHalfThick;
	RailWall(TEXT("RailWallFar"), BargeRules::DeckAft, BargeRules::DeckFore, -Side * Edge - RailWallHalfThick, -Side * Edge + RailWallHalfThick);
	RailWall(TEXT("RailWallBerthFore"), GapFore, BargeRules::DeckFore, Side * Edge - RailWallHalfThick, Side * Edge + RailWallHalfThick);
	RailWall(TEXT("RailWallBerthAft"), BargeRules::DeckAft, GapAft, Side * Edge - RailWallHalfThick, Side * Edge + RailWallHalfThick);
	RailWall(TEXT("RailWallBow"), BargeRules::DeckFore, BargeRules::DeckFore + 2.f * RailWallHalfThick, -Edge, Edge);
	RailWall(TEXT("RailWallStern"), BargeRules::DeckAft - 2.f * RailWallHalfThick, BargeRules::DeckAft, -Edge, Edge);
	// Round the hatch's hole on three sides: its stair is entered from the port edge (where the stair's top is).
	RailWall(TEXT("HatchWallFore"), HoleXMax, HoleXMax + 2.f * RailWallHalfThick, HoleYMin, HoleYMax);
	RailWall(TEXT("HatchWallAft"), HoleXMin - 2.f * RailWallHalfThick, HoleXMin, HoleYMin, HoleYMax);
	RailWall(TEXT("HatchWallStarboard"), HoleXMin, HoleXMax, HoleYMax, HoleYMax + 2.f * RailWallHalfThick);

	// Cover: crates and plate stacks the riflemen crouch behind (instanced look, a box each to stop shots and boots).
	int32 CoverIndex = 0;
	for (const FVector& Spot : BargeRules::CoverSpots)
	{
		MakeWall(*FString::Printf(TEXT("Cover%d"), CoverIndex++), Spot + FVector(0.0, 0.0, BargeRules::CoverHalf.Z), BargeRules::CoverHalf);
	}

	// The stacks: the Tripo hull carries two of its own (only their collision here); the third is the kit's, the one that
	// falls in step 7, turned about its foot.
	for (int32 Index = 0; Index < 2; ++Index)
	{
		const FVector Foot = BargeRules::StackSpots[Index];
		MakeWall(*FString::Printf(TEXT("StackWall%d"), Index), Foot + FVector(0.0, 0.0, BargeRules::StackHeight * 0.5),
			FVector(BargeRules::StackRadius, BargeRules::StackRadius, BargeRules::StackHeight * 0.5));
		UStaticMeshComponent* Look = MakePiece(*FString::Printf(TEXT("StackLook%d"), Index), TripoStack ? TripoStack : CylinderMesh,
			TripoStack ? nullptr : IronBlack, Foot + FVector(0.0, 0.0, BargeRules::StackHeight * 0.5),
			FVector(BargeRules::StackRadius * 2.0, BargeRules::StackRadius * 2.0, BargeRules::StackHeight), FRotator::ZeroRotator, false);
		Look->SetVisibility(TripoHull == nullptr);
	}
	FallingStackPivot = CreateDefaultSubobject<USceneComponent>(TEXT("FallingStackPivot"));
	FallingStackPivot->SetupAttachment(Kit);
	FallingStackPivot->SetMobility(EComponentMobility::Movable);
	FallingStackPivot->SetRelativeLocation(BargeRules::StackSpots[2]);
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

	// The jib crane: a mast and its jib out over the deck.
	MakePiece(TEXT("CraneMast"), TripoCrane ? TripoCrane : CylinderMesh, TripoCrane ? nullptr : IronBlack,
		BargeRules::CraneSpot + FVector(0.0, 0.0, BargeRules::CraneHeight * 0.5), FVector(70.0, 70.0, BargeRules::CraneHeight));
	UStaticMeshComponent* Jib = MakePiece(TEXT("CraneJib"), CubeMesh, IronBlack,
		BargeRules::CraneSpot + FVector(-BargeRules::CraneReach * 0.45, -BargeRules::CraneReach * 0.25, BargeRules::CraneHeight - 60.0),
		FVector(BargeRules::CraneReach, 45.0, 55.0), FRotator(8.0, 25.0, 0.0), false);
	Jib->SetVisibility(TripoCrane == nullptr);
}

void ABargeBoarding::BuildHall()
{
	static UStaticMesh* const CubeMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cube);
	static UStaticMesh* const CylinderMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cylinder);
	static UMaterialInterface* const Iron = BargeLooks::Find<UMaterialInterface>(BargeLooks::Iron);
	static UMaterialInterface* const IronBlack = BargeLooks::Find<UMaterialInterface>(BargeLooks::IronBlack);
	static UMaterialInterface* const Wood = BargeLooks::Find<UMaterialInterface>(BargeLooks::Wood);

	const float Floor = BargeRules::HallFloor;
	const float R = BargeRules::HallRadius;
	const float Catwalk = Floor + BargeRules::CatwalkUp;
	// The floor: a riveted iron disc to see, a box to stand on (the walls bound it).
	MakePiece(TEXT("HallFloorLook"), CylinderMesh, IronBlack, BargeRules::HallCenter + FVector(0.0, 0.0, Floor - 5.0),
		FVector(R * 2.0, R * 2.0, 10.0), FRotator::ZeroRotator, false);
	Walkables.Add(MakeWall(TEXT("HallFloor"), BargeRules::HallCenter + FVector(0.0, 0.0, Floor - 15.0), FVector(R + 140.0, R + 140.0, 15.0)));

	// The round wall, panel by panel, open at the office (over the catwalk only) and at the two stokeholds.
	const float PanelWidth = UE_TWO_PI * (R + BargeRules::WallThickness * 0.5f) / Panels * 1.04f;
	for (int32 Index = 0; Index < Panels; ++Index)
	{
		const float Angle = (Index + 0.5f) * 360.f / Panels;
		if (IsStokeholdGap(Angle))
		{
			continue;
		}
		const bool bUnderOffice = IsOfficeGap(Angle);
		const float Height = bUnderOffice ? BargeRules::CatwalkUp : BargeRules::HallHeight;
		const FVector At = HallXY(Angle, R + BargeRules::WallThickness * 0.5f) + FVector(0.0, 0.0, Floor + Height * 0.5f);
		HallWalls.Add(MakeWall(*FString::Printf(TEXT("HallWall%d"), Index), At,
			FVector(BargeRules::WallThickness * 0.5, PanelWidth * 0.5, Height * 0.5), FRotator(0.f, Angle, 0.f)));
	}

	// The catwalk ring at 4 m, in straight spans (each a box to walk, the same transforms for its grated look).
	const float SpanLength = UE_TWO_PI * RingMiddle / CatwalkSpans * 1.06f;
	for (int32 Index = 0; Index < CatwalkSpans; ++Index)
	{
		const float Angle = (Index + 0.5f) * 360.f / CatwalkSpans;
		Walkables.Add(MakeWall(*FString::Printf(TEXT("Catwalk%d"), Index), HallXY(Angle, RingMiddle) + FVector(0.0, 0.0, Catwalk - 10.0),
			FVector((BargeRules::HallRadius - BargeRules::CatwalkInner) * 0.5, SpanLength * 0.5, 10.0), FRotator(0.f, Angle, 0.f)));
	}

	// The stairs: tangent chords of the ring, sloped between their two heights.
	int32 StairIndex = 0;
	for (const FStair& Stair : Stairs)
	{
		const FVector From = HallXY(Stair.FromAngle, RingMiddle) + FVector(0.0, 0.0, Floor + Stair.FromUp);
		const FVector To = HallXY(Stair.ToAngle, RingMiddle) + FVector(0.0, 0.0, Floor + Stair.ToUp);
		const FVector Run = To - From;
		const FRotator Turn = Run.Rotation();
		MakePiece(*FString::Printf(TEXT("Stair%d"), StairIndex++), CubeMesh, IronBlack, (From + To) * 0.5 - FVector(0.0, 0.0, StairThick * 0.5),
			FVector(Run.Size(), StairWidth, StairThick), Turn);
	}

	// The stokeholds' alcoves: a floor, a back wall (their iron doors swung open), two sides.
	int32 AlcoveIndex = 0;
	for (const float Angle : BargeRules::StokeholdAngles)
	{
		const FVector Out = HallXY(Angle, 1.f) - BargeRules::HallCenter;
		const FVector Across(-Out.Y, Out.X, 0.0);
		const FVector Middle = BargeRules::HallCenter + Out * (R + AlcoveDepth * 0.5);
		const FRotator Facing(0.f, Angle, 0.f);
		const float Up = BargeRules::CatwalkUp - 20.f;
		Walkables.Add(MakeWall(*FString::Printf(TEXT("AlcoveFloor%d"), AlcoveIndex), Middle + FVector(0.0, 0.0, Floor - 15.0),
			FVector(AlcoveDepth * 0.5 + 20.0, AlcoveHalfWidth, 15.0), Facing));
		MakeWall(*FString::Printf(TEXT("AlcoveBack%d"), AlcoveIndex), BargeRules::HallCenter + Out * (R + AlcoveDepth + 10.0) + FVector(0.0, 0.0, Floor + Up * 0.5),
			FVector(10.0, AlcoveHalfWidth + 20.0, Up * 0.5), Facing);
		for (const float SideSign : { -1.f, 1.f })
		{
			MakeWall(*FString::Printf(TEXT("AlcoveSide%d_%d"), AlcoveIndex, SideSign > 0.f ? 1 : 0),
				Middle + Across * SideSign * (AlcoveHalfWidth + 10.0) + FVector(0.0, 0.0, Floor + Up * 0.5), FVector(AlcoveDepth * 0.5 + 20.0, 10.0, Up * 0.5), Facing);
		}
		// Its door: an iron slab swung back against the alcove's side.
		MakePiece(*FString::Printf(TEXT("AlcoveDoor%d"), AlcoveIndex), CubeMesh, IronBlack,
			Middle + Across * (AlcoveHalfWidth - 20.0) + FVector(0.0, 0.0, Floor + 160.0), FVector(AlcoveDepth, 12.0, 320.0), Facing, false);
		++AlcoveIndex;
	}

	// The stoker's office past the wall at the catwalk's height: its floor, three walls, a desk and an iron safe.
	const FVector Out = HallXY(BargeRules::OfficeAngle, 1.f) - BargeRules::HallCenter;
	const FVector Across(-Out.Y, Out.X, 0.0);
	const FRotator Facing(0.f, BargeRules::OfficeAngle, 0.f);
	const FVector Middle = BargeRules::HallCenter + Out * (R + BargeRules::OfficeDepth * 0.5);
	const float WallUp = 300.f;
	MakePiece(TEXT("OfficeFloor"), CubeMesh, Wood, Middle + FVector(0.0, 0.0, Catwalk - 10.0),
		FVector(BargeRules::OfficeDepth + 40.0, BargeRules::OfficeHalfWidth * 2.0, 20.0), Facing);
	MakePiece(TEXT("OfficeBack"), CubeMesh, Iron, BargeRules::HallCenter + Out * (R + BargeRules::OfficeDepth + 10.0) + FVector(0.0, 0.0, Catwalk + WallUp * 0.5),
		FVector(20.0, BargeRules::OfficeHalfWidth * 2.0 + 40.0, WallUp), Facing);
	for (const float SideSign : { -1.f, 1.f })
	{
		MakePiece(*FString::Printf(TEXT("OfficeSide%d"), SideSign > 0.f ? 1 : 0), CubeMesh, Iron,
			Middle + Across * SideSign * (BargeRules::OfficeHalfWidth + 10.0) + FVector(0.0, 0.0, Catwalk + WallUp * 0.5),
			FVector(BargeRules::OfficeDepth + 40.0, 20.0, WallUp), Facing);
	}
	const FVector Desk = BargeRules::DeskTop();
	MakePiece(TEXT("OfficeDesk"), CubeMesh, Wood, Desk - FVector(0.0, 0.0, BargeRules::DeskTopUp * 0.5), FVector(90.0, 180.0, BargeRules::DeskTopUp), Facing);
	MakePiece(TEXT("OfficeSafe"), CubeMesh, IronBlack, Middle + Across * (BargeRules::OfficeHalfWidth - 60.0) + Out * 120.0 + FVector(0.0, 0.0, Catwalk + 65.0),
		FVector(80.0, 80.0, 130.0), Facing);
}

void ABargeBoarding::LayStaticLooks()
{
	if (!RailLook || !CoverLook || !HallWallLook || !CatwalkLook || !VentCold)
	{
		return;
	}
	const float Top = BargeRules::DeckTop;
	const float Beam = BargeRules::DeckHalfBeam;
	// The visible rail: a low iron rail in 5 m spans round the deck, open at the berth's gap.
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
	const float Side = BargeRules::BerthSide;
	RailRun(FVector(BargeRules::DeckAft, -Side * Beam, 0.0), FVector(BargeRules::DeckFore, -Side * Beam, 0.0));
	RailRun(FVector(BargeRules::DeckAft, Side * Beam, 0.0), FVector(BargeRules::BerthX - BargeRules::GapHalf, Side * Beam, 0.0));
	RailRun(FVector(BargeRules::BerthX + BargeRules::GapHalf, Side * Beam, 0.0), FVector(BargeRules::DeckFore, Side * Beam, 0.0));
	RailRun(FVector(BargeRules::DeckFore, -Beam, 0.0), FVector(BargeRules::DeckFore, Beam, 0.0));
	RailRun(FVector(BargeRules::DeckAft, -Beam, 0.0), FVector(BargeRules::DeckAft, Beam, 0.0));
	// The hatch's coaming round three sides of its hole.
	RailRun(FVector(HoleXMin, HoleYMax, 0.0), FVector(HoleXMax, HoleYMax, 0.0));
	RailRun(FVector(HoleXMin, HoleYMin, 0.0), FVector(HoleXMin, HoleYMax, 0.0));
	RailRun(FVector(HoleXMax, HoleYMin, 0.0), FVector(HoleXMax, HoleYMax, 0.0));
	RailLook->ClearInstances();
	RailLook->AddInstances(Rails, /*bShouldReturnIndices*/ false);

	TArray<FTransform> Cover;
	for (const FVector& Spot : BargeRules::CoverSpots)
	{
		Cover.Add(FTransform(FQuat::Identity, Spot + FVector(0.0, 0.0, BargeRules::CoverHalf.Z), BargeRules::CoverHalf / 50.0));
	}
	CoverLook->ClearInstances();
	CoverLook->AddInstances(Cover, false);

	// The hall's wall panels and the catwalk's grating, as their boxes stand.
	TArray<FTransform> Walls;
	for (const UBoxComponent* Wall : HallWalls)
	{
		if (Wall)
		{
			Walls.Add(FTransform(Wall->GetRelativeRotation(), Wall->GetRelativeLocation(), Wall->GetUnscaledBoxExtent() / 50.0));
		}
	}
	HallWallLook->ClearInstances();
	HallWallLook->AddInstances(Walls, false);
	TArray<FTransform> Grating;
	const float SpanLength = UE_TWO_PI * RingMiddle / CatwalkSpans * 1.06f;
	for (int32 Index = 0; Index < CatwalkSpans; ++Index)
	{
		const float Angle = (Index + 0.5f) * 360.f / CatwalkSpans;
		Grating.Add(FTransform(FRotator(0.f, Angle, 0.f), HallXY(Angle, RingMiddle) + FVector(0.0, 0.0, BargeRules::HallFloor + BargeRules::CatwalkUp - 10.0),
			FVector((BargeRules::HallRadius - BargeRules::CatwalkInner) / 100.0, SpanLength / 100.0, 0.2)));
		// Its inner hand rail.
		Grating.Add(FTransform(FRotator(0.f, Angle, 0.f), HallXY(Angle, BargeRules::CatwalkInner + 6.f) + FVector(0.0, 0.0, BargeRules::HallFloor + BargeRules::CatwalkUp + 100.0),
			FVector(0.06, SpanLength / 100.0, 0.08)));
	}
	CatwalkLook->ClearInstances();
	CatwalkLook->AddInstances(Grating, false);

	// The vents, all cold (the deck's tick lays their glow and fire).
	TArray<FTransform> Vents;
	for (const FVector& Spot : BargeRules::VentSpots)
	{
		Vents.Add(FTransform(FQuat::Identity, Spot + FVector(0.0, 0.0, 4.0), FVector(BargeRules::VentHalf / 50.0, BargeRules::VentHalf / 50.0, 0.1)));
	}
	VentCold->ClearInstances();
	VentCold->AddInstances(Vents, false);
	VentGlow->ClearInstances();
	VentFire->ClearInstances();
	VentGlowMask = 0u;
	VentFireMask = 0u;
}

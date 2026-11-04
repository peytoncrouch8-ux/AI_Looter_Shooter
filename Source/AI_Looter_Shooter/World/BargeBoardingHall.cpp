// ABargeBoarding: inside her house, at deck level, built in code in her own frame (BargeRules): the vestibule inside the
// door (its walls, the lining round the door, its roof; its floor is a deck plate), the stoker's office aft of it (floor,
// walls, roof, the doorway onto the hall, the desk and the safe), and the furnace hall: its floor, its round wall (a lintel
// over the office's doorway and over each stokehold's door), its ceiling under the house's roof, the catwalk ring (open
// over the two stairs that climb up through it, its ends past their feet walled), the stokeholds' alcoves. The house is empty inside, so all of this closes itself in: nobody sees
// into the shell. One level: door, vestibule, office, hall.

#include "World/BargeBoarding.h"
#include "World/BargeLooks.h"
#include "World/BargeRules.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace
{
	/** The hall's wall in panels this many degrees wide; the gaps (the office's doorway, the stokeholds' doors). */
	constexpr int32 Panels = 36;
	bool IsOfficeGap(float Angle) { return Angle < 10.f || Angle > 350.f; }
	bool IsStokeholdGap(float Angle) { return FMath::Abs(Angle - 90.f) < 10.f || FMath::Abs(Angle - 270.f) < 10.f; }

	FVector HallXY(float AngleDegrees, float Radius)
	{
		const float Turn = FMath::DegreesToRadians(AngleDegrees);
		return BargeRules::HallCenter + FVector(FMath::Cos(Turn) * Radius, FMath::Sin(Turn) * Radius, 0.0);
	}

	float CatwalkSpanLength()
	{
		return UE_TWO_PI * BargeRules::CatwalkMiddle / BargeRules::CatwalkSpans * BargeRules::CatwalkSpanStretch;
	}
}

void ABargeBoarding::BuildVestibule()
{
	static UStaticMesh* const CubeMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cube);
	static UMaterialInterface* const Iron = BargeLooks::Find<UMaterialInterface>(BargeLooks::Iron);

	// Iron to see and to stop pawns and shots (the house's shell has no rooms of its own).
	const auto Solid = [this](const TCHAR* Name, const FVector& Min, const FVector& Max)
	{
		UStaticMeshComponent* Piece = MakePiece(Name, CubeMesh, Iron, (Min + Max) * 0.5, Max - Min, FRotator::ZeroRotator, false);
		Piece->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Piece->SetCollisionObjectType(ECC_WorldStatic);
		Piece->SetCastShadow(false);
	};
	const float Top = BargeRules::DeckTop;
	const float Up = Top + BargeRules::VestibuleHeight;
	const float W = BargeRules::VestibuleWall;
	const float Half = BargeRules::VestibuleHalfWidth;
	const float Aft = BargeRules::VestibuleAft;
	const float Door = BargeRules::DoorHalfWidth;
	const float DoorTop = Top + BargeRules::DoorHeight;
	// The lining stands just inside the house's fore wall; the walls and the roof reach to it.
	const float Lining = BargeRules::HouseFore - BargeRules::HouseWallThick;
	Solid(TEXT("VestibulePort"), FVector(Aft - W, -(Half + W), Top), FVector(Lining, -Half, Up));
	Solid(TEXT("VestibuleStarboard"), FVector(Aft - W, Half, Top), FVector(Lining, Half + W, Up));
	Solid(TEXT("VestibuleRoof"), FVector(Aft - W, -(Half + W), Up), FVector(Lining, Half + W, Up + W));
	// The lining round the house's door: either side of it, and the lintel over it.
	Solid(TEXT("VestibuleLiningPort"), FVector(Lining - W, -Half, Top), FVector(Lining, -Door, Up));
	Solid(TEXT("VestibuleLiningStarboard"), FVector(Lining - W, Door, Top), FVector(Lining, Half, Up));
	Solid(TEXT("VestibuleLintel"), FVector(Lining - W, -Door, DoorTop), FVector(Lining, Door, Up));
	// Its aft wall is the office's fore wall, the office's width, round a doorway as big as the house's.
	const float OfficeOut = BargeRules::OfficeHalfWidth + BargeRules::OfficeWall;
	Solid(TEXT("VestibuleAftPort"), FVector(Aft - W, -OfficeOut, Top), FVector(Aft, -Door, Up));
	Solid(TEXT("VestibuleAftStarboard"), FVector(Aft - W, Door, Top), FVector(Aft, OfficeOut, Up));
	Solid(TEXT("VestibuleAftLintel"), FVector(Aft - W, -Door, DoorTop), FVector(Aft, Door, Up));
}

void ABargeBoarding::BuildHall()
{
	static UStaticMesh* const CubeMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cube);
	static UStaticMesh* const CylinderMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cylinder);
	static UMaterialInterface* const Iron = BargeLooks::Find<UMaterialInterface>(BargeLooks::Iron);
	static UMaterialInterface* const IronBlack = BargeLooks::Find<UMaterialInterface>(BargeLooks::IronBlack);
	static UMaterialInterface* const Wood = BargeLooks::Find<UMaterialInterface>(BargeLooks::Wood);

	const float Floor = BargeRules::HallFloor;
	const float FloorTop = Floor + BargeRules::InteriorFloorLift;
	const float R = BargeRules::HallRadius;
	const float Catwalk = Floor + BargeRules::CatwalkUp;
	// The floor: a riveted iron disc to see, a box to stand on (the walls bound it), a hair over the deck's height.
	MakePiece(TEXT("HallFloorLook"), CylinderMesh, IronBlack, BargeRules::HallCenter + FVector(0.0, 0.0, FloorTop - 5.0),
		FVector(R * 2.0, R * 2.0, 10.0), FRotator::ZeroRotator, false);
	MakeWall(TEXT("HallFloor"), BargeRules::HallCenter + FVector(0.0, 0.0, FloorTop - BargeRules::HallFloorThick * 0.5),
		FVector(R + BargeRules::HallFloorOut, BargeRules::HallSkinHalfBeam - BargeRules::SkinGap, BargeRules::HallFloorThick * 0.5));

	// The round wall, panel by panel: over the office's doorway and each stokehold's door only a lintel from the door's top
	// to the ceiling.
	const float PanelWidth = UE_TWO_PI * (R + BargeRules::WallThickness * 0.5f) / Panels * 1.04f;
	for (int32 Index = 0; Index < Panels; ++Index)
	{
		const float Angle = (Index + 0.5f) * 360.f / Panels;
		const float From = IsStokeholdGap(Angle) ? BargeRules::AlcoveUp : IsOfficeGap(Angle) ? BargeRules::OfficeDoorUp : 0.f;
		const float To = BargeRules::HallHeight;
		const FVector At = HallXY(Angle, R + BargeRules::WallThickness * 0.5f) + FVector(0.0, 0.0, Floor + (From + To) * 0.5f);
		HallWalls.Add(MakeWall(*FString::Printf(TEXT("HallWall%d"), Index), At,
			FVector(BargeRules::WallThickness * 0.5, PanelWidth * 0.5, (To - From) * 0.5), FRotator(0.f, Angle, 0.f)));
	}

	// The ceiling on the walls' top, under the house's roof. Shots stop on it.
	const FBox2D Over = BargeRules::HallCeilingArea();
	UStaticMeshComponent* Ceiling = MakePiece(TEXT("HallCeiling"), CubeMesh, Iron,
		FVector(Over.GetCenter().X, Over.GetCenter().Y, Floor + BargeRules::HallHeight + BargeRules::CeilingThickness * 0.5),
		FVector(Over.GetSize().X, Over.GetSize().Y, BargeRules::CeilingThickness), FRotator::ZeroRotator, false);
	Ceiling->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Ceiling->SetCollisionObjectType(ECC_WorldStatic);
	Ceiling->SetCastShadow(false);

	// The catwalk ring at 4 m, in straight spans (each a box to walk, the same transforms for its grated look), open over the
	// stairs (a span there would roof the climb).
	const float RingDepth = BargeRules::HallRadius - BargeRules::CatwalkInner;
	for (int32 Index = 0; Index < BargeRules::CatwalkSpans; ++Index)
	{
		if (!BargeRules::IsCatwalkSpanLaid(Index))
		{
			continue;
		}
		const float Angle = BargeRules::CatwalkSpanAngle(Index);
		MakeWall(*FString::Printf(TEXT("Catwalk%d"), Index), HallXY(Angle, BargeRules::CatwalkMiddle) + FVector(0.0, 0.0, Catwalk - 10.0),
			FVector(RingDepth * 0.5, CatwalkSpanLength() * 0.5, 10.0), FRotator(0.f, Angle, 0.f));
	}

	// The two stairs from the floor up through the ring's openings to the catwalk: chords of the ring, sloped between their two
	// heights. Past each one's foot an unseen wall (pawns only) closes the ring's end, so nobody walks off it into the drop.
	for (int32 Index = 0; Index < BargeRules::NumFloorStairs; ++Index)
	{
		const BargeRules::FStairRun Stair = BargeRules::FloorStair(Index);
		const FVector Run = Stair.To - Stair.From;
		MakePiece(*FString::Printf(TEXT("Stair%d"), Index), CubeMesh, IronBlack, (Stair.From + Stair.To) * 0.5 - FVector(0.0, 0.0, BargeRules::StairThick * 0.5),
			FVector(Run.Size(), BargeRules::StairWidth, BargeRules::StairThick), Run.Rotation());
		const float End = BargeRules::CatwalkEndAngle(Index);
		UBoxComponent* EndWall = CreateDefaultSubobject<UBoxComponent>(*FString::Printf(TEXT("CatwalkEnd%d"), Index));
		EndWall->SetupAttachment(Kit);
		EndWall->SetMobility(EComponentMobility::Movable);
		EndWall->SetBoxExtent(FVector(RingDepth * 0.5, BargeRules::RailWallThick * 0.5, BargeRules::RailWallHeight * 0.5), false);
		EndWall->SetRelativeLocationAndRotation(HallXY(End, BargeRules::CatwalkMiddle) + FVector(0.0, 0.0, Catwalk + BargeRules::RailWallHeight * 0.5),
			FRotator(0.f, End, 0.f));
		EndWall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		EndWall->SetCollisionObjectType(ECC_WorldDynamic);
		EndWall->SetCollisionResponseToAllChannels(ECR_Ignore);
		EndWall->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		EndWall->SetCanEverAffectNavigation(false);
		EndWall->SetGenerateOverlapEvents(false);
		EndWall->SetHiddenInGame(true);
	}

	// The stokeholds' alcoves, kept inside the house's walls (BargeRules::AlcoveDepth): a floor, a back wall (their iron doors
	// swung open), two sides and a roof under the catwalk, all iron to see.
	const float Wall = BargeRules::AlcoveWall;
	const float Depth = BargeRules::AlcoveDepth;
	const float HalfWidth = BargeRules::AlcoveHalfWidth;
	const float Up = BargeRules::AlcoveUp;
	int32 AlcoveIndex = 0;
	for (const float Angle : BargeRules::StokeholdAngles)
	{
		const FVector Out = HallXY(Angle, 1.f) - BargeRules::HallCenter;
		const FVector Across(-Out.Y, Out.X, 0.0);
		const FVector Middle = BargeRules::HallCenter + Out * (R + Depth * 0.5);
		const FRotator Facing(0.f, Angle, 0.f);
		UBoxComponent* AlcoveFloor = MakeWall(*FString::Printf(TEXT("AlcoveFloor%d"), AlcoveIndex), Middle + FVector(0.0, 0.0, FloorTop - 15.0),
			FVector(Depth * 0.5 + Wall, HalfWidth + Wall, 15.0), Facing);
		HallWalls.Add(AlcoveFloor);
		HallWalls.Add(MakeWall(*FString::Printf(TEXT("AlcoveBack%d"), AlcoveIndex),
			BargeRules::HallCenter + Out * (R + Depth + Wall * 0.5) + FVector(0.0, 0.0, Floor + Up * 0.5), FVector(Wall * 0.5, HalfWidth + Wall, Up * 0.5), Facing));
		for (const float SideSign : { -1.f, 1.f })
		{
			HallWalls.Add(MakeWall(*FString::Printf(TEXT("AlcoveSide%d_%d"), AlcoveIndex, SideSign > 0.f ? 1 : 0),
				Middle + Across * SideSign * (HalfWidth + Wall * 0.5) + FVector(0.0, 0.0, Floor + Up * 0.5), FVector(Depth * 0.5 + Wall, Wall * 0.5, Up * 0.5), Facing));
		}
		HallWalls.Add(MakeWall(*FString::Printf(TEXT("AlcoveRoof%d"), AlcoveIndex),
			BargeRules::HallCenter + Out * (R + (Depth + Wall) * 0.5) + FVector(0.0, 0.0, Floor + Up + Wall * 0.5),
			FVector((Depth + Wall) * 0.5, HalfWidth + Wall, Wall * 0.5), Facing));
		// Its door: an iron slab swung back against the alcove's side.
		MakePiece(*FString::Printf(TEXT("AlcoveDoor%d"), AlcoveIndex), CubeMesh, IronBlack,
			Middle + Across * (HalfWidth - 20.0) + FVector(0.0, 0.0, Floor + Up * 0.5), FVector(Depth, 12.0, Up - 20.0), Facing, false);
		++AlcoveIndex;
	}

	// The stoker's office between the hall's rim and the vestibule, on the hall's floor: its floor, side walls and roof, its
	// aft wall either side of the doorway onto the hall (the vestibule's aft wall is its fore wall), the desk toward its port
	// end, the safe at its starboard end.
	const FVector Out = HallXY(BargeRules::OfficeAngle, 1.f) - BargeRules::HallCenter;
	const FVector Across(-Out.Y, Out.X, 0.0);
	const FRotator Facing(0.f, BargeRules::OfficeAngle, 0.f);
	const FVector Middle = BargeRules::HallCenter + Out * (R + BargeRules::OfficeDepth * 0.5);
	const float Height = BargeRules::OfficeHeight;
	const float OfficeHalf = BargeRules::OfficeHalfWidth;
	const float OfficeWall = BargeRules::OfficeWall;
	const float AftWall = BargeRules::OfficeAftWall;
	const float Deep = BargeRules::OfficeDepth;
	MakePiece(TEXT("OfficeFloor"), CubeMesh, Wood, BargeRules::HallCenter + Out * (R + (Deep - OfficeWall) * 0.5) + FVector(0.0, 0.0, FloorTop - 10.0),
		FVector(Deep + OfficeWall, (OfficeHalf + OfficeWall) * 2.0, 20.0), Facing);
	for (const float SideSign : { -1.f, 1.f })
	{
		MakePiece(*FString::Printf(TEXT("OfficeSide%d"), SideSign > 0.f ? 1 : 0), CubeMesh, Iron,
			BargeRules::HallCenter + Out * (R + (Deep - AftWall) * 0.5) + Across * SideSign * (OfficeHalf + OfficeWall * 0.5) + FVector(0.0, 0.0, Floor + Height * 0.5),
			FVector(Deep + AftWall, OfficeWall, Height), Facing);
		// The aft wall from the doorway out to the side wall (its back to the hall's curved wall: closed off behind it).
		const float Span = OfficeHalf + OfficeWall - BargeRules::OfficeDoorHalf;
		MakePiece(*FString::Printf(TEXT("OfficeAft%d"), SideSign > 0.f ? 1 : 0), CubeMesh, Iron,
			BargeRules::HallCenter + Out * (R - AftWall * 0.5) + Across * SideSign * (BargeRules::OfficeDoorHalf + Span * 0.5)
				+ FVector(0.0, 0.0, Floor + Height * 0.5),
			FVector(AftWall, Span, Height), Facing);
	}
	MakePiece(TEXT("OfficeRoof"), CubeMesh, Iron, BargeRules::HallCenter + Out * (R + (Deep - AftWall) * 0.5) + FVector(0.0, 0.0, Floor + Height + OfficeWall * 0.5),
		FVector(Deep + AftWall, (OfficeHalf + OfficeWall) * 2.0, OfficeWall), Facing);
	const FVector Desk = BargeRules::DeskTop();
	MakePiece(TEXT("OfficeDesk"), CubeMesh, Wood, Desk - FVector(0.0, 0.0, BargeRules::DeskTopUp * 0.5), FVector(90.0, 180.0, BargeRules::DeskTopUp), Facing);
	MakePiece(TEXT("OfficeSafe"), CubeMesh, IronBlack, Middle + Across * BargeRules::SafeAcross + Out * BargeRules::SafeOut + FVector(0.0, 0.0, FloorTop + 65.0),
		FVector(80.0, 80.0, 130.0), Facing);
}

void ABargeBoarding::LayHallLooks()
{
	if (!HallWallLook || !CatwalkLook)
	{
		return;
	}
	// The hall's iron (wall panels, lintels, the alcoves) as their boxes stand; the catwalk's grating and its inner hand rail.
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
	const float SpanLength = CatwalkSpanLength();
	for (int32 Index = 0; Index < BargeRules::CatwalkSpans; ++Index)
	{
		if (!BargeRules::IsCatwalkSpanLaid(Index))
		{
			continue;
		}
		const float Angle = BargeRules::CatwalkSpanAngle(Index);
		Grating.Add(FTransform(FRotator(0.f, Angle, 0.f), HallXY(Angle, BargeRules::CatwalkMiddle) + FVector(0.0, 0.0, BargeRules::HallFloor + BargeRules::CatwalkUp - 10.0),
			FVector((BargeRules::HallRadius - BargeRules::CatwalkInner) / 100.0, SpanLength / 100.0, 0.2)));
		Grating.Add(FTransform(FRotator(0.f, Angle, 0.f), HallXY(Angle, BargeRules::CatwalkInner + 6.f) + FVector(0.0, 0.0, BargeRules::HallFloor + BargeRules::CatwalkUp + 100.0),
			FVector(0.06, SpanLength / 100.0, 0.08)));
	}
	CatwalkLook->ClearInstances();
	CatwalkLook->AddInstances(Grating, false);
}

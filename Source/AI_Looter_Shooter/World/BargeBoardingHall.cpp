// ABargeBoarding: below the deck, built in code in her own frame (BargeRules), in the hull's hollow under the casing: the
// furnace hall (its floor, its round wall with lintels over the stokeholds' doors, its ceiling closing the hollow (Tripo made
// no deck inside the casing) cut round the hatch's hole, the catwalk ring open under the hatch's stair and over the two
// floor stairs, its ends walled, the stokeholds' alcoves) and the hatch's stair from the cross walk down onto the catwalk.
// The hollow is empty, so all of this closes itself in: nobody sees into the shell. The way in: the hatch on the cross walk,
// down its stair onto the catwalk (the stoker's corner is there: BargeBoardingProps.cpp), down a floor stair to the floor.

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
	constexpr int32 Panels = BargeRules::HallWallPanels;
	constexpr float GapHalfDegrees = BargeRules::DoorGapDegrees;

	FVector HallXY(float AngleDegrees, float Radius)
	{
		return BargeRules::HallPoint(AngleDegrees, Radius) - FVector(0.0, 0.0, BargeRules::HallFloor);
	}

	/** A hall angle as a yaw in the barge's frame. */
	float HallYaw(float AngleDegrees)
	{
		return BargeRules::OfficeAngle + AngleDegrees;
	}

	float CatwalkSpanLength()
	{
		return UE_TWO_PI * BargeRules::CatwalkMiddle / BargeRules::CatwalkSpans * BargeRules::CatwalkSpanStretch;
	}
}

void ABargeBoarding::BuildHall()
{
	static UStaticMesh* const CubeMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cube);
	static UStaticMesh* const CylinderMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cylinder);
	static UMaterialInterface* const KitIron = BargeLooks::FindOr<UMaterialInterface>(BargeLooks::KitIron, BargeLooks::Iron);
	static UMaterialInterface* const KitIronDark = BargeLooks::FindOr<UMaterialInterface>(BargeLooks::KitIronDark, BargeLooks::IronBlack);

	const float Floor = BargeRules::HallFloor;
	const float R = BargeRules::HallRadius;
	const float Catwalk = Floor + BargeRules::CatwalkUp;
	// The floor: a riveted iron disc to see, a box to stand on (the walls bound it).
	const float Disc = BargeRules::HallFloorLookThick;
	MakePiece(TEXT("HallFloorLook"), CylinderMesh, KitIronDark, BargeRules::HallCenter + FVector(0.0, 0.0, Floor - Disc * 0.5),
		FVector(R * 2.0, R * 2.0, Disc), FRotator::ZeroRotator, false);
	MakeWall(TEXT("HallFloor"), BargeRules::HallCenter + FVector(0.0, 0.0, Floor - BargeRules::HallFloorThick * 0.5),
		FVector(R + BargeRules::HallFloorOut, BargeRules::HallSkinHalfBeam - BargeRules::SkinGap, BargeRules::HallFloorThick * 0.5));

	// The round wall, panel by panel: over each stokehold's door only a lintel from the door's top.
	const float PanelWidth = UE_TWO_PI * (R + BargeRules::WallThickness * 0.5f) / Panels * BargeRules::HallWallPanelStretch;
	const float Top = BargeRules::HallHeight;
	for (int32 Index = 0; Index < Panels; ++Index)
	{
		const float Angle = (Index + 0.5f) * 360.f / Panels;
		TArray<FVector2D, TInlineAllocator<2>> Runs;
		if (BargeRules::IsNearAngle(Angle, BargeRules::StokeholdAngles[0], GapHalfDegrees) || BargeRules::IsNearAngle(Angle, BargeRules::StokeholdAngles[1], GapHalfDegrees))
		{
			Runs.Add(FVector2D(BargeRules::AlcoveUp, Top));
		}
		else
		{
			Runs.Add(FVector2D(0.0, Top));
		}
		for (int32 Run = 0; Run < Runs.Num(); ++Run)
		{
			const FVector2D Span = Runs[Run];
			const FVector At = HallXY(Angle, R + BargeRules::WallThickness * 0.5f) + FVector(0.0, 0.0, Floor + (Span.X + Span.Y) * 0.5);
			HallWalls.Add(MakeWall(*FString::Printf(TEXT("HallWall%d_%d"), Index, Run), At,
				FVector(BargeRules::WallThickness * 0.5, PanelWidth * 0.5, (Span.Y - Span.X) * 0.5), FRotator(0.f, HallYaw(Angle), 0.f)));
		}
	}

	// The ceiling on the walls' top, closing the hollow (inside the casing the hull has no deck over it), cut round the
	// hatch's hole. Shots stop on it.
	const FBox2D Over = BargeRules::HallCeilingArea();
	int32 CeilingIndex = 0;
	for (const FBox2D& Piece : BargeRules::CutAround(Over, BargeRules::HatchHole()))
	{
		MakePiece(*FString::Printf(TEXT("HallCeiling%d"), CeilingIndex++), CubeMesh, KitIron,
			FVector(Piece.GetCenter(), BargeRules::HallCeiling - BargeRules::CeilingThickness * 0.5),
			FVector(Piece.GetSize(), BargeRules::CeilingThickness))->SetCastShadow(false);
	}

	// The catwalk ring at 4 m, in straight spans (each a box to walk, the same transforms for its grated look), open over the
	// stairs (a span there would roof the climb).
	const float RingDepth = BargeRules::CatwalkDepth;
	for (int32 Index = 0; Index < BargeRules::CatwalkSpans; ++Index)
	{
		if (!BargeRules::IsCatwalkSpanLaid(Index))
		{
			continue;
		}
		const float Angle = BargeRules::CatwalkSpanAngle(Index);
		MakeWall(*FString::Printf(TEXT("Catwalk%d"), Index), HallXY(Angle, BargeRules::CatwalkMiddle) + FVector(0.0, 0.0, Catwalk - BargeRules::CatwalkThick * 0.5),
			FVector(RingDepth * 0.5, CatwalkSpanLength() * 0.5, BargeRules::CatwalkThick * 0.5), FRotator(0.f, HallYaw(Angle), 0.f));
	}

	// The two stairs from the ring's openings down to the floor: chords of the ring, unseen ramps (their treads are looks).
	// Past each one's foot, and past the hatch stair's top, an unseen wall (pawns only) closes the ring's end, so nobody walks
	// off it into the drop. The walls stand under the ceiling (the hatch stair passes over the one by its top).
	const float EndHeight = FMath::Min(BargeRules::RailWallHeight, BargeRules::HallHeight - BargeRules::CatwalkUp);
	const auto EndWallAt = [this, RingDepth, Catwalk, EndHeight](const TCHAR* Name, float End)
	{
		UBoxComponent* EndWall = CreateDefaultSubobject<UBoxComponent>(Name);
		EndWall->SetupAttachment(Kit);
		EndWall->SetMobility(EComponentMobility::Movable);
		EndWall->SetBoxExtent(FVector(RingDepth * 0.5, BargeRules::RailWallThick * 0.5, EndHeight * 0.5), false);
		EndWall->SetRelativeLocationAndRotation(HallXY(End, BargeRules::CatwalkMiddle) + FVector(0.0, 0.0, Catwalk + EndHeight * 0.5),
			FRotator(0.f, HallYaw(End), 0.f));
		EndWall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		EndWall->SetCollisionObjectType(ECC_WorldDynamic);
		EndWall->SetCollisionResponseToAllChannels(ECR_Ignore);
		EndWall->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		EndWall->SetCanEverAffectNavigation(false);
		EndWall->SetGenerateOverlapEvents(false);
		EndWall->SetHiddenInGame(true);
	};
	for (int32 Index = 0; Index < BargeRules::NumFloorStairs; ++Index)
	{
		MakeRamp(*FString::Printf(TEXT("Stair%d"), Index), BargeRules::FloorStair(Index));
		EndWallAt(*FString::Printf(TEXT("CatwalkEnd%d"), Index), BargeRules::CatwalkEndAngle(Index));
	}
	EndWallAt(TEXT("CatwalkEndHatch"), BargeRules::HatchCatwalkEnd());

	// The stokeholds' alcoves, kept inside her skin (BargeRules::AlcoveDepth): a floor, a back wall (their iron doors swung
	// open), two sides and a roof under the catwalk.
	const float Wall = BargeRules::AlcoveWall;
	const float Depth = BargeRules::AlcoveDepth;
	const float HalfWidth = BargeRules::AlcoveHalfWidth();
	const float Up = BargeRules::AlcoveUp;
	const float FloorHalf = BargeRules::AlcoveFloorThick * 0.5f;
	const float DoorGap = BargeRules::AlcoveDoorGap;
	int32 AlcoveIndex = 0;
	for (const float Angle : BargeRules::StokeholdAngles)
	{
		const FVector Out = HallXY(Angle, 1.f) - BargeRules::HallCenter;
		const FVector Across(-Out.Y, Out.X, 0.0);
		const FVector Middle = BargeRules::HallCenter + Out * (R + Depth * 0.5);
		const FRotator Facing(0.f, HallYaw(Angle), 0.f);
		HallWalls.Add(MakeWall(*FString::Printf(TEXT("AlcoveFloor%d"), AlcoveIndex), Middle + FVector(0.0, 0.0, Floor - FloorHalf),
			FVector(Depth * 0.5 + Wall, HalfWidth + Wall, FloorHalf), Facing));
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
		MakePiece(*FString::Printf(TEXT("AlcoveDoor%d"), AlcoveIndex), CubeMesh, KitIronDark,
			Middle + Across * (HalfWidth - DoorGap) + FVector(0.0, 0.0, Floor + Up * 0.5), FVector(Depth, BargeRules::AlcoveDoorThick, Up - DoorGap), Facing, false);
		++AlcoveIndex;
	}
}

void ABargeBoarding::BuildHatchStair()
{
	// The hatch's stair: from the cross walk at the hole's edge down through the hall's ceiling onto the catwalk (its treads
	// are looks).
	MakeRamp(TEXT("HatchStair"), BargeRules::HatchStair(), BargeRules::HatchStairWidth);
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
	const float Catwalk = BargeRules::HallFloor + BargeRules::CatwalkUp;
	for (int32 Index = 0; Index < BargeRules::CatwalkSpans; ++Index)
	{
		if (!BargeRules::IsCatwalkSpanLaid(Index))
		{
			continue;
		}
		const float Angle = BargeRules::CatwalkSpanAngle(Index);
		const FRotator Facing(0.f, HallYaw(Angle), 0.f);
		// The grating a hair under the walk's top (the office's floor reaches over it in the doorway: no flicker).
		Grating.Add(FTransform(Facing, HallXY(Angle, BargeRules::CatwalkMiddle) + FVector(0.0, 0.0, Catwalk - BargeRules::CatwalkThick * 0.5 - BargeRules::LookDrop),
			FVector(BargeRules::CatwalkDepth / 100.0, SpanLength / 100.0, BargeRules::CatwalkThick / 100.0)));
		Grating.Add(FTransform(Facing, HallXY(Angle, BargeRules::CatwalkInner + BargeRules::CatwalkRailIn) + FVector(0.0, 0.0, Catwalk + BargeRules::CatwalkRailUp),
			FVector(BargeRules::CatwalkRailWidth / 100.0, SpanLength / 100.0, BargeRules::CatwalkRailDepth / 100.0)));
	}
	CatwalkLook->ClearInstances();
	CatwalkLook->AddInstances(Grating, false);
}

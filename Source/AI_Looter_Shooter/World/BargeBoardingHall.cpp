// ABargeBoarding: below the deck, built in code in her own frame (BargeRules), in the hull's hollow under the casing: the
// furnace hall (its floor, its round wall with lintels over the stokeholds' doors, its ceiling closing the hollow (Tripo made
// no deck inside the casing) cut round the hatch's hole, the catwalk ring open under the hatch's stair and over the two
// floor stairs, its ends walled, a hand rail along its inner edge, the stokeholds' alcoves) and the hatch's stair from the
// cross walk down onto the catwalk, railed both sides.
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

	/** A span's length at the hand rail's radius: its straight boxes at the middle's length stuck their ends out into the
	 *  walkway (Main's run, 2026-11-04: the ring's walk stopped in Handrail15 by its span's end). */
	float RailSpanLength()
	{
		return UE_TWO_PI * (BargeRules::CatwalkInner + BargeRules::CatwalkRailIn) / BargeRules::CatwalkSpans * BargeRules::CatwalkSpanStretch;
	}

	/** The hatch stair's run, its slope's turn and up, and its sides' way (flat, square to the run). */
	struct FHatchRails
	{
		FVector Middle;
		FVector Run;
		FRotator Slope;
		FVector Up;
		FVector Across;
	};
	FHatchRails HatchRails()
	{
		const BargeRules::FStairRun Stair = BargeRules::HatchStair();
		FHatchRails Rails;
		Rails.Middle = (Stair.From + Stair.To) * 0.5;
		Rails.Run = Stair.To - Stair.From;
		Rails.Slope = Rails.Run.Rotation();
		Rails.Up = Rails.Slope.Quaternion().GetUpVector();
		Rails.Across = FVector(-Rails.Run.Y, Rails.Run.X, 0.0).GetSafeNormal();
		return Rails;
	}

	/** Where a hatch stair's rail stands: its side (-1, 1), a hand in from the stair's edge. */
	FVector HatchRailSide(const FHatchRails& Rails, double Side)
	{
		return Rails.Across * Side * (BargeRules::HatchStairWidth * 0.5 - BargeRules::HatchRailIn);
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
	// The hand rails' iron (the kit's lighter plate, MI_BargeKitIron), laid with the hall's looks.
	HandrailLook = MakeInstanced(TEXT("HandrailLook"), CubeMesh, KitIron);
	HandrailLook->SetCastShadow(false);
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
	// An unseen wall for pawns only (shots pass): the ring's ends, its hand rail, the hatch stair's.
	const auto PawnWall = [this](const TCHAR* Name, const FVector& Center, const FVector& Half, const FRotator& Turn)
	{
		UBoxComponent* Wall = CreateDefaultSubobject<UBoxComponent>(Name);
		Wall->SetupAttachment(Kit);
		Wall->SetMobility(EComponentMobility::Movable);
		Wall->SetBoxExtent(Half, false);
		Wall->SetRelativeLocationAndRotation(Center, Turn);
		Wall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Wall->SetCollisionObjectType(ECC_WorldDynamic);
		Wall->SetCollisionResponseToAllChannels(ECR_Ignore);
		Wall->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Wall->SetCanEverAffectNavigation(false);
		Wall->SetGenerateOverlapEvents(false);
		Wall->SetHiddenInGame(true);
	};
	const auto EndWallAt = [&PawnWall, RingDepth, Catwalk, EndHeight](const TCHAR* Name, float End)
	{
		PawnWall(Name, HallXY(End, BargeRules::CatwalkMiddle) + FVector(0.0, 0.0, Catwalk + EndHeight * 0.5),
			FVector(RingDepth * 0.5, BargeRules::RailWallThick * 0.5, EndHeight * 0.5), FRotator(0.f, HallYaw(End), 0.f));
	};
	// The hand rail along each laid span's inner edge (the PIE walk, 2026-11-04: off it was a 5.8 m fall); the floor stairs'
	// openings have no span, so no rail. Its wall stands to the ceiling (a 1 m one is jumped); its look is 1 m.
	for (int32 Index = 0; Index < BargeRules::CatwalkSpans; ++Index)
	{
		if (BargeRules::IsCatwalkSpanLaid(Index))
		{
			const float Angle = BargeRules::CatwalkSpanAngle(Index);
			PawnWall(*FString::Printf(TEXT("Handrail%d"), Index), HallXY(Angle, BargeRules::CatwalkInner + BargeRules::CatwalkRailIn) + FVector(0.0, 0.0, Catwalk + EndHeight * 0.5),
				FVector(BargeRules::CatwalkRailWidth * 0.5, RailSpanLength() * 0.5, EndHeight * 0.5), FRotator(0.f, HallYaw(Angle), 0.f));
		}
	}
	// Both sides of the hatch's stair (over the hall's drop, over the hole's gap beside it), in upright pieces along it: each
	// from the treads under its lower end to HatchRailWallUp over its upper end, so nothing overhangs past the stair's foot.
	const FHatchRails Hatch = HatchRails();
	const FRotator Along(0.f, static_cast<float>(Hatch.Run.Rotation().Yaw), 0.f);
	const FVector Piece = Hatch.Run / BargeRules::HatchRailPieces;
	const FVector Overlap = Hatch.Run * (BargeRules::HatchRailOverlap / FMath::Max(Hatch.Run.Size2D(), 1.0));
	for (const double Side : { -1.0, 1.0 })
	{
		const FVector StairTop = Hatch.Middle - Hatch.Run * 0.5 + HatchRailSide(Hatch, Side);
		for (int32 Index = 0; Index < BargeRules::HatchRailPieces; ++Index)
		{
			const FVector Upper = StairTop + Piece * Index - (Index > 0 ? Overlap : FVector::ZeroVector);
			const FVector Lower = StairTop + Piece * (Index + 1) + (Index + 1 < BargeRules::HatchRailPieces ? Overlap : FVector::ZeroVector);
			const double Bottom = FMath::Min(Upper.Z, Lower.Z);
			const double Height = FMath::Max(Upper.Z, Lower.Z) + BargeRules::HatchRailWallUp - Bottom;
			const FVector Middle((Upper.X + Lower.X) * 0.5, (Upper.Y + Lower.Y) * 0.5, Bottom + Height * 0.5);
			PawnWall(*FString::Printf(TEXT("HatchRail%d_%d"), Side < 0.0 ? 0 : 1, Index), Middle,
				FVector((Lower - Upper).Size2D() * 0.5, BargeRules::CatwalkRailWidth * 0.5, Height * 0.5), Along);
		}
	}
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
	// The hall's iron (wall panels, lintels, the alcoves) as their boxes stand; the catwalk's grating; the hand rails.
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
	TArray<FTransform> Rails;
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
		// The rail: its top and a mid rail along the span, a stanchion at its start (and at its end where the ring ends).
		const FVector RailAt = HallXY(Angle, BargeRules::CatwalkInner + BargeRules::CatwalkRailIn);
		for (const float Up : { BargeRules::CatwalkRailUp, BargeRules::CatwalkRailMidUp })
		{
			Rails.Add(FTransform(Facing, RailAt + FVector(0.0, 0.0, Catwalk + Up),
				FVector(BargeRules::CatwalkRailWidth / 100.0, RailSpanLength() / 100.0, BargeRules::CatwalkRailDepth / 100.0)));
		}
		// The stanchions on the rails' line, where two spans' straight rails cross.
		const float Half = 180.f / BargeRules::CatwalkSpans;
		const float RailJoint = (BargeRules::CatwalkInner + BargeRules::CatwalkRailIn) / FMath::Cos(FMath::DegreesToRadians(Half));
		for (const float Edge : { Angle - Half, Angle + Half })
		{
			if (Edge > Angle && BargeRules::IsCatwalkSpanLaid((Index + 1) % BargeRules::CatwalkSpans))
			{
				continue;
			}
			Rails.Add(FTransform(Facing, HallXY(Edge, RailJoint) + FVector(0.0, 0.0, Catwalk + BargeRules::CatwalkRailUp * 0.5),
				FVector(BargeRules::CatwalkStanchion / 100.0, BargeRules::CatwalkStanchion / 100.0, BargeRules::CatwalkRailUp / 100.0)));
		}
	}
	CatwalkLook->ClearInstances();
	CatwalkLook->AddInstances(Grating, false);
	// The hatch stair's: along each side, its rails straight over the treads (their height upright, so their ends stand over
	// the stair's: none overhangs its foot), stanchions at its ends and its middle.
	const FHatchRails Hatch = HatchRails();
	const FQuat Pitched = Hatch.Slope.Quaternion();
	for (const double Side : { -1.0, 1.0 })
	{
		const FVector Line = Hatch.Middle + HatchRailSide(Hatch, Side);
		for (const float Up : { BargeRules::CatwalkRailUp, BargeRules::CatwalkRailMidUp })
		{
			Rails.Add(FTransform(Pitched, Line + FVector(0.0, 0.0, Up),
				FVector(Hatch.Run.Size() / 100.0, BargeRules::CatwalkRailWidth / 100.0, BargeRules::CatwalkRailDepth / 100.0)));
		}
		for (const double Along : { -0.5, 0.0, 0.5 })
		{
			Rails.Add(FTransform(FQuat::Identity, Line + Hatch.Run * Along + FVector(0.0, 0.0, BargeRules::CatwalkRailUp * 0.5),
				FVector(BargeRules::CatwalkStanchion / 100.0, BargeRules::CatwalkStanchion / 100.0, BargeRules::CatwalkRailUp / 100.0)));
		}
	}
	if (HandrailLook)
	{
		HandrailLook->ClearInstances();
		HandrailLook->AddInstances(Rails, false);
	}
}

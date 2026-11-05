// BargeRules: the boarding stage's numbers as plain math (Main 6, "The Pilot Light"): her decks as zones, the rail walls, the
// berth's clearances, the deck's pieces. The hall's half is BargeHallRules.cpp.

#include "World/BargeRules.h"

namespace BargeRules
{
	FName Mission()
	{
		static const FName Name(TEXT("Main6"));
		return Name;
	}

	FName DrumJammedFlag()
	{
		static const FName Name(TEXT("Pilot.DrumJammed"));
		return Name;
	}

	FName BargeFellFlag()
	{
		static const FName Name(TEXT("Pilot.BargeFell"));
		return Name;
	}

	const FDeckZone* DeckZoneAt(const FVector2D& At)
	{
		for (const FDeckZone& Zone : DeckZones)
		{
			if (At.X >= Zone.XMin && At.X <= Zone.XMax && At.Y >= Zone.YMin && At.Y <= Zone.YMax)
			{
				return &Zone;
			}
		}
		return nullptr;
	}

	FVector OnDeck(const FVector2D& At, float Up)
	{
		const FDeckZone* Zone = DeckZoneAt(At);
		return FVector(At.X, At.Y, (Zone ? Zone->Z : WellDeckZ) + Up);
	}

	FStairRun ForecastleStair(int32 Index)
	{
		// Up its step: the top on the forecastle's aft edge, the foot on the well deck.
		const double Side = FMath::Clamp(Index, 0, NumForecastleStairs - 1) == 0 ? -1.0 : 1.0;
		return { FVector(ForecastleAft, Side * ForecastleStairAcross, ForecastleZ),
			FVector(ForecastleAft - ForecastleStairRun, Side * ForecastleStairAcross, WellDeckZ) };
	}

	bool IsOpenDeck(const FVector2D& At)
	{
		if (!DeckZoneAt(At) || HatchCoaming().IsInside(At))
		{
			return false;
		}
		for (const FDeckFootprint& Fitting : DeckFittings())
		{
			if (At.X >= Fitting.XMin && At.X <= Fitting.XMax && At.Y >= Fitting.YMin && At.Y <= Fitting.YMax)
			{
				return false;
			}
		}
		for (int32 Index = 0; Index < NumForecastleStairs; ++Index)
		{
			const FStairRun Stair = ForecastleStair(Index);
			const FBox2D Footprint(FVector2D(Stair.To.X, Stair.To.Y - ForecastleStairHalfWidth), FVector2D(Stair.From.X, Stair.From.Y + ForecastleStairHalfWidth));
			if (Footprint.IsInside(At))
			{
				return false;
			}
		}
		return true;
	}

	TArray<FRailWall> RailWallLayout()
	{
		// One on each run of her rail line, just outside its inner face (outboard), at its deck's height; the run on the berth's
		// side that holds the berth is cut at the gap, where the plank lands and the jump is made. Then across the stern
		// platform's aft edge.
		TArray<FRailWall> Walls;
		const double T = RailWallThick;
		int32 Index = 0;
		for (const FRailRun& Run : RailLine)
		{
			const double Out = Run.YInner < 0.f ? -1.0 : 1.0;
			const double YMin = FMath::Min(static_cast<double>(Run.YInner), Run.YInner + Out * T);
			const double YMax = FMath::Max(static_cast<double>(Run.YInner), Run.YInner + Out * T);
			const TCHAR* Side = Out < 0.0 ? TEXT("P") : TEXT("S");
			const auto Add = [&Walls, &Run](const FString& Name, double XMin, double XMax, double Lo, double Hi)
			{
				Walls.Add({ FName(*Name), FBox2D(FVector2D(XMin, Lo), FVector2D(XMax, Hi)), Run.Z });
			};
			const FString Name = FString::Printf(TEXT("RailWall_%s%02d"), Side, Index++);
			if (Out == static_cast<double>(BerthSide) && BerthX > Run.XMin && BerthX < Run.XMax)
			{
				Add(Name + TEXT("_Aft"), Run.XMin, BerthX - GapHalf, YMin, YMax);
				Add(Name + TEXT("_Fore"), BerthX + GapHalf, Run.XMax, YMin, YMax);
			}
			else
			{
				Add(Name, Run.XMin, Run.XMax, YMin, YMax);
			}
		}
		Walls.Add({ FName(TEXT("RailWall_SternAft")), FBox2D(FVector2D(SternAft - T, -SternFloorHalfBeam), FVector2D(SternAft, SternFloorHalfBeam)), SternZ });
		return Walls;
	}

	TArray<FBox2D> CutAround(const FBox2D& Piece, const FBox2D& Hole)
	{
		if (!Piece.Intersect(Hole))
		{
			return { Piece };
		}
		// The strips aft and fore of the hole the piece's full width, then the two beside it between them.
		TArray<FBox2D> Pieces;
		const FBox2D Cut(FVector2D(FMath::Max(Hole.Min.X, Piece.Min.X), FMath::Max(Hole.Min.Y, Piece.Min.Y)),
			FVector2D(FMath::Min(Hole.Max.X, Piece.Max.X), FMath::Min(Hole.Max.Y, Piece.Max.Y)));
		const auto Keep = [&Pieces](const FVector2D& Min, const FVector2D& Max)
		{
			if (Max.X - Min.X > 1.0 && Max.Y - Min.Y > 1.0)
			{
				Pieces.Add(FBox2D(Min, Max));
			}
		};
		Keep(Piece.Min, FVector2D(Cut.Min.X, Piece.Max.Y));
		Keep(FVector2D(Cut.Max.X, Piece.Min.Y), Piece.Max);
		Keep(FVector2D(Cut.Min.X, Piece.Min.Y), FVector2D(Cut.Max.X, Cut.Min.Y));
		Keep(FVector2D(Cut.Min.X, Cut.Max.Y), FVector2D(Cut.Max.X, Piece.Max.Y));
		return Pieces;
	}

	float PlankSillClearance()
	{
		// Her hinge (her deck's edge at her rail) to the landing (BoardingInset in from the deck's edge), measured outward from
		// the barge's middle; the plank's floor where it crosses the rail's inner face, over the gap's sill.
		const float HingeOut = BerthOut - ShipRailOut;
		const float HingeUp = WellDeckZ - BerthUnderDeck;
		const float LandOut = WellDeckHalfBeam - BoardingInset;
		const float CrossOut = BulwarkHalfBeam - BulwarkThick;
		const float Share = (CrossOut - LandOut) / FMath::Max(HingeOut - LandOut, 1.f);
		const float Line = FMath::Lerp(WellDeckZ, HingeUp, Share);
		return Line - PlankUnderside - (WellDeckZ + SillUp);
	}

	float BerthBagClearance()
	{
		return BerthOut - ShipBagHalfBeam - BulwarkHalfBeam;
	}

	float JumpBagClearance()
	{
		// The rail's top on her side, from her keel at the jump berth (both in the barge's frame), turned by the full list (the
		// barge's turn about her +X; her berth side goes down) into her level frame: how far inboard of her keel line it ends
		// up, less her bag's half beam.
		const FVector2D Rail(BerthSide * BulwarkHalfBeam, WellDeckZ + RailHeight);
		const FVector2D Keel(BerthSide * JumpOut, WellDeckZ - JumpUnderDeck - ShipDeckOverKeel);
		const FVector2D Rel = Rail - Keel;
		const float Turn = FMath::DegreesToRadians(-BerthSide * ListDegrees);
		const float Across = Rel.X * FMath::Cos(Turn) - Rel.Y * FMath::Sin(Turn);
		return -BerthSide * Across - ShipBagHalfBeam;
	}

	FVector CoverFallbackHalf(ECover Kind)
	{
		return CoverFallbackHalves[FMath::Clamp(static_cast<int32>(Kind), 0, static_cast<int32>(UE_ARRAY_COUNT(CoverFallbackHalves)) - 1)];
	}

	FVector StackFallDir()
	{
		const float Aft = FMath::DegreesToRadians(StackFallAft);
		return FVector(-FMath::Sin(Aft), BerthSide * FMath::Cos(Aft), 0.0);
	}

	FVector StackFallPivot()
	{
		return FallingStackFoot + StackFallDir() * StackFootRadius;
	}

	FVector StackTip()
	{
		// Turned a quarter about the flange's edge, the tube lies from that edge out its length.
		return StackFallPivot() + StackFallDir() * StackHeight;
	}

	FBox2D CasingRoof()
	{
		return FBox2D(FVector2D(CasingRoofAft, -CasingRoofHalfBeam), FVector2D(CasingRoofFore, CasingRoofHalfBeam));
	}

	FVector DeckMiddle()
	{
		return FVector((WellDeckAft + WellDeckFore) * 0.5, 0.0, WellDeckZ);
	}

	FVector WorkLampSeat(const FWorkLamp& Lamp)
	{
		// On the rim's plate; under a post's arm's end (the arm runs along the post's top); at a roof's rod's foot.
		switch (Lamp.Mount)
		{
		case ELampMount::Post:
			return Lamp.Fixed + FVector(Lamp.Out, 0.0) * WorkLampArm + FVector(0.0, 0.0, Lamp.Up - WorkLampIron);
		case ELampMount::Drop:
			return Lamp.Fixed - FVector(0.0, 0.0, Lamp.Up);
		default:
			return Lamp.Fixed + FVector(0.0, 0.0, WorkLampPlate);
		}
	}
}

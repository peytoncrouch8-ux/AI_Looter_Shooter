#include "World/PlayableBoundary.h"

namespace
{
	/** Below this (cm) an edge has no length: the same corner given twice. */
	constexpr double ShortestWallEdge = 1.0;
	/**
	 * How far past an outward corner a wall may run to meet its neighbour, in wall depths (setback plus thickness).
	 * Corners sharper than about 28 degrees would need more, and keep a narrow notch at their tip.
	 */
	constexpr double LongestCornerReach = 4.0;

	/**
	 * Whether the edge from A to B crosses the line of constant X. Each end counts on one side only, so a line through
	 * a corner crosses one of its two edges, never both or neither.
	 */
	bool EdgeCrossesX(const FVector2D& A, const FVector2D& B, double X)
	{
		return (A.X > X) != (B.X > X);
	}

	/**
	 * Where the edge from A to B crosses the line of constant X (only asked when it does). Contains and the minimap's
	 * rows share it, so they always agree.
	 */
	double EdgeCrossingY(const FVector2D& A, const FVector2D& B, double X)
	{
		return A.Y + (X - A.X) / (B.X - A.X) * (B.Y - A.Y);
	}
}

FPlayableBoundary::FPlayableBoundary(TArray<FVector2D> InCorners, TArray<bool> InOpenEdges)
	: Corners(MoveTemp(InCorners))
	, Open(MoveTemp(InOpenEdges))
{
	// The shoelace formula: its sign says which way round the corners run, so the walls know which side is out.
	for (int32 Index = 0; Index < Corners.Num(); ++Index)
	{
		SignedArea += FVector2D::CrossProduct(Corners[Index], Corners[(Index + 1) % Corners.Num()]);
	}
	SignedArea *= 0.5;
}

bool FPlayableBoundary::IsValid() const
{
	return Corners.Num() >= 3 && FMath::Abs(SignedArea) > 1.0;
}

bool FPlayableBoundary::Contains(const FVector2D& Point) const
{
	// Count the edges a ray from the point toward +Y crosses: an odd count is inside.
	bool bInside = false;
	for (int32 Index = 0; Index < Corners.Num(); ++Index)
	{
		const FVector2D& A = Corners[Index];
		const FVector2D& B = Corners[(Index + 1) % Corners.Num()];
		if (EdgeCrossesX(A, B, Point.X) && EdgeCrossingY(A, B, Point.X) > Point.Y)
		{
			bInside = !bInside;
		}
	}
	return bInside;
}

void FPlayableBoundary::CrossingsAtX(double X, TArray<double>& OutY) const
{
	OutY.Reset();
	for (int32 Index = 0; Index < Corners.Num(); ++Index)
	{
		const FVector2D& A = Corners[Index];
		const FVector2D& B = Corners[(Index + 1) % Corners.Num()];
		if (EdgeCrossesX(A, B, X))
		{
			OutY.Add(EdgeCrossingY(A, B, X));
		}
	}
	OutY.Sort();
}

int32 FPlayableBoundary::FindCrossedEdge(const FVector2D& From, const FVector2D& To) const
{
	// Solve From + AlongPath * Path = A + AlongEdge * Side for each edge; the crossing nearest From is the way out.
	const FVector2D Path = To - From;
	int32 First = INDEX_NONE;
	double FirstAlongPath = TNumericLimits<double>::Max();
	for (int32 Index = 0; Index < Corners.Num(); ++Index)
	{
		const FVector2D A = EdgeStart(Index);
		const FVector2D Side = EdgeEnd(Index) - A;
		const double Denominator = FVector2D::CrossProduct(Path, Side);
		if (FMath::IsNearlyZero(Denominator))
		{
			continue;
		}
		const FVector2D Offset = A - From;
		const double AlongPath = FVector2D::CrossProduct(Offset, Side) / Denominator;
		const double AlongEdge = FVector2D::CrossProduct(Offset, Path) / Denominator;
		if (AlongPath >= 0.0 && AlongPath <= 1.0 && AlongEdge >= 0.0 && AlongEdge <= 1.0 && AlongPath < FirstAlongPath)
		{
			First = Index;
			FirstAlongPath = AlongPath;
		}
	}
	return First;
}

int32 FPlayableBoundary::FindNearestEdge(const FVector2D& Point, double* OutDistance) const
{
	int32 Nearest = INDEX_NONE;
	double Best = TNumericLimits<double>::Max();
	for (int32 Index = 0; Index < Corners.Num(); ++Index)
	{
		const double Distance = DistanceToSegment(Point, EdgeStart(Index), EdgeEnd(Index));
		if (Distance < Best)
		{
			Best = Distance;
			Nearest = Index;
		}
	}
	if (OutDistance)
	{
		*OutDistance = Best;
	}
	return Nearest;
}

FVector2D FPlayableBoundary::Outward(int32 Edge) const
{
	const FVector2D Along = (EdgeEnd(Edge) - EdgeStart(Edge)).GetSafeNormal();
	// Counterclockwise corners have the inside on the left of each edge, so out is on the right; clockwise, the reverse.
	return SignedArea >= 0.0 ? FVector2D(Along.Y, -Along.X) : FVector2D(-Along.Y, Along.X);
}

TArray<FPlayableWall> FPlayableBoundary::MakeWalls(double Setback, double Thickness) const
{
	TArray<FPlayableWall> Result;
	if (!IsValid())
	{
		return Result;
	}
	const int32 Count = Corners.Num();
	const double Winding = SignedArea >= 0.0 ? 1.0 : -1.0;
	auto EdgeLength = [this](int32 Edge) { return FVector2D::Distance(EdgeStart(Edge), EdgeEnd(Edge)); };
	auto EdgeDirection = [this](int32 Edge) { return (EdgeEnd(Edge) - EdgeStart(Edge)).GetSafeNormal(); };
	// The next edge with some length before or after this one (Step -1 or 1), skipping corners given twice.
	auto Neighbour = [&EdgeLength, Count](int32 Edge, int32 Step)
	{
		for (int32 Tries = 1; Tries < Count; ++Tries)
		{
			const int32 Other = ((Edge + Step * Tries) % Count + Count) % Count;
			if (EdgeLength(Other) >= ShortestWallEdge)
			{
				return Other;
			}
		}
		return Edge;
	};
	// How far a wall runs past the corner from Incoming to Outgoing to meet the other's wall: nothing at an inward
	// corner (the walls already cross there), or beside an open edge (no wall to meet).
	auto Reach = [&](int32 Incoming, int32 Outgoing)
	{
		if (Incoming == Outgoing || IsOpen(Incoming) || IsOpen(Outgoing))
		{
			return 0.0;
		}
		const FVector2D In = EdgeDirection(Incoming);
		const FVector2D Out = EdgeDirection(Outgoing);
		const double Turn = FVector2D::CrossProduct(In, Out) * Winding;
		if (Turn <= 1e-6)
		{
			return 0.0;
		}
		// Offset outward, the two walls' faces meet tan(turn / 2) wall depths past the corner (sin / (1 + cos)).
		const double TanHalfTurn = Turn / FMath::Max(1.0 + FVector2D::DotProduct(In, Out), 1e-6);
		return (Setback + Thickness) * FMath::Min(TanHalfTurn, LongestCornerReach);
	};

	for (int32 Edge = 0; Edge < Count; ++Edge)
	{
		const double Length = EdgeLength(Edge);
		if (IsOpen(Edge) || Length < ShortestWallEdge)
		{
			continue;
		}
		const FVector2D Along = EdgeDirection(Edge);
		const double Before = Reach(Neighbour(Edge, -1), Edge);
		const double After = Reach(Edge, Neighbour(Edge, 1));
		FPlayableWall& Wall = Result.AddDefaulted_GetRef();
		Wall.Edge = Edge;
		Wall.Direction = Along;
		Wall.HalfLength = (Length + Before + After) * 0.5;
		Wall.HalfThickness = Thickness * 0.5;
		Wall.Center = EdgeStart(Edge) + Along * ((Length + After - Before) * 0.5) + Outward(Edge) * (Setback + Thickness * 0.5);
	}
	return Result;
}

double FPlayableBoundary::DistanceToSegment(const FVector2D& Point, const FVector2D& A, const FVector2D& B)
{
	const FVector2D Along = B - A;
	const double LengthSquared = Along.SizeSquared();
	const double Share = LengthSquared > 0.0 ? FMath::Clamp(FVector2D::DotProduct(Point - A, Along) / LengthSquared, 0.0, 1.0) : 0.0;
	return FVector2D::Distance(Point, A + Along * Share);
}

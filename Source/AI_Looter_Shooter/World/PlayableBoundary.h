#pragma once

#include "CoreMinimal.h"

/** Where an invisible wall stands behind one closed edge, seen from above (FPlayableBoundary::MakeWalls). */
struct FPlayableWall
{
	/** The edge it stands behind. */
	int32 Edge = INDEX_NONE;
	/** Its middle (world XY, cm). */
	FVector2D Center = FVector2D::ZeroVector;
	/** Along the edge, from its first corner toward its second (unit length). */
	FVector2D Direction = FVector2D(1.0, 0.0);
	/** Half its length along the edge, and half its thickness (cm). */
	double HalfLength = 0.0;
	double HalfThickness = 0.0;
};

/**
 * A playable area's boundary seen from above: a polygon of corners in world XY (cm), in order around it either way,
 * with each edge closed (a wall stands behind it) or open (a drop that is part of play). Edge i runs from corner i to
 * the next one, and the last edge back to corner 0. Plain geometry with no world, shared by APlayableArea, fall
 * recovery, the minimap's bake and the tests.
 */
struct AI_LOOTER_SHOOTER_API FPlayableBoundary
{
	FPlayableBoundary() = default;
	/** Corners in order; InOpenEdges holds one flag per edge (true = open), and missing entries count as closed. */
	FPlayableBoundary(TArray<FVector2D> InCorners, TArray<bool> InOpenEdges);

	/** At least three corners enclosing some ground. */
	bool IsValid() const;

	int32 NumEdges() const { return Corners.Num(); }
	const TArray<FVector2D>& GetCorners() const { return Corners; }
	FVector2D EdgeStart(int32 Edge) const { return Corners[Edge]; }
	FVector2D EdgeEnd(int32 Edge) const { return Corners[(Edge + 1) % Corners.Num()]; }
	bool IsOpen(int32 Edge) const { return Open.IsValidIndex(Edge) && Open[Edge]; }

	/** The ground it encloses (square cm). */
	double SurfaceArea() const { return FMath::Abs(SignedArea); }

	/** Whether a point lies inside. A point exactly on an edge may go either way. */
	bool Contains(const FVector2D& Point) const;

	/**
	 * Where the boundary crosses the line of constant world X through X, as sorted Y values. The crossings pair up:
	 * points from the first up to (not including) the second are inside, then from the third to the fourth, and so on,
	 * which is exactly what Contains answers for them. The minimap fills its rows of texels with it.
	 */
	void CrossingsAtX(double X, TArray<double>& OutY) const;

	/** The edge a straight path from From to To crosses first, or INDEX_NONE if it crosses none. */
	int32 FindCrossedEdge(const FVector2D& From, const FVector2D& To) const;

	/** The edge nearest a point (INDEX_NONE without edges), and optionally how far it is (cm). */
	int32 FindNearestEdge(const FVector2D& Point, double* OutDistance = nullptr) const;

	/** An edge's unit normal pointing out of the area (zero for an edge of no length). */
	FVector2D Outward(int32 Edge) const;

	/**
	 * Where the walls stand: one box behind each closed edge, Setback past it and Thickness deep (cm). At an outward
	 * corner between two closed edges both walls run on past the corner until their outer faces meet, so there is no
	 * gap to squeeze through; at an inward corner they already cross. Beside an open edge a wall ends at its corner.
	 */
	TArray<FPlayableWall> MakeWalls(double Setback, double Thickness) const;

	/** How far a point lies from the segment from A to B (in whatever units they are given in). */
	static double DistanceToSegment(const FVector2D& Point, const FVector2D& A, const FVector2D& B);

private:
	TArray<FVector2D> Corners;
	TArray<bool> Open;
	/** Positive when the corners run counterclockwise (with X to the right and Y up), negative when clockwise. */
	double SignedArea = 0.0;
};

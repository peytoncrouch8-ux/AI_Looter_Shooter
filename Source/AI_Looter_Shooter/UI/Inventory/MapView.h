#pragma once

#include "CoreMinimal.h"

/**
 * How the map page shows the baked map, as plain math so the tests drive it: the frame it's drawn in (page units), how
 * far it's zoomed (1: the whole map fits the frame) and which point of the map sits in the frame's middle. Map
 * coordinates are UMinimapSubsystem's: U runs east and V south, 0-1 over the baked square, so north is up. The map never
 * slides off where it could fill the frame; smaller than the frame along a side, it stays centred there.
 */
struct AI_LOOTER_SHOOTER_API FMapView
{
	/** Zoom 1 fits the whole map; the most it zooms in. */
	static constexpr double MinZoom = 1.0;
	static constexpr double MaxZoom = 6.0;

	/** The frame's size (page units). */
	FVector2D Frame = FVector2D(1000.0, 700.0);

	double Zoom = 1.0;

	/** The map point in the frame's middle (0-1). */
	FVector2D Center = FVector2D(0.5, 0.5);

	/** The map square's side at zoom 1: it fits the frame's shorter side. */
	double FitSide() const { return FMath::Max(FMath::Min(Frame.X, Frame.Y), 1.0); }

	/** The map square's side now (page units). */
	double Side() const { return FitSide() * Zoom; }

	/** Where a map point is drawn in the frame (from its top-left). */
	FVector2D UVToFrame(const FVector2D& UV) const { return Frame * 0.5 + (UV - Center) * Side(); }

	/** The map point under a point of the frame. */
	FVector2D FrameToUV(const FVector2D& Point) const { return Center + (Point - Frame * 0.5) / Side(); }

	/** The map square's top-left corner in the frame (it may lie outside it, zoomed in). */
	FVector2D MapTopLeft() const { return UVToFrame(FVector2D::ZeroVector); }

	/** Zoom into its range, and the centre where the map fills the frame (or centred along a side it can't fill). */
	void Clamp();

	/** Zooms by Factor about a point of the frame: the map point under it stays under it (as far as Clamp allows). */
	void ZoomAt(const FVector2D& FramePoint, double Factor);

	/** Drags the map by Delta (page units): the map follows the pointer. */
	void Pan(const FVector2D& Delta);

	/** Zooms and centres so a box of the map (0-1) fills the frame, with Margin (a share of the box) round it. */
	void Fit(const FBox2D& Box, double Margin = 0.06);

	/** A grid step (world cm, from Steps) that draws lines at least MinSpacing page units apart over a map MapSize cm wide. */
	double GridStep(double MapSize, double MinSpacing) const;

	/**
	 * Which of Points lies best in Direction from From (the D-pad picking the next grave that way): only points ahead,
	 * within 60 degrees of it, the nearer and the straighter ahead winning; Exclude (the one chosen now) is skipped.
	 * INDEX_NONE when nothing lies that way.
	 */
	static int32 PickInDirection(TConstArrayView<FVector2D> Points, const FVector2D& From, const FVector2D& Direction,
		int32 Exclude = INDEX_NONE);
};

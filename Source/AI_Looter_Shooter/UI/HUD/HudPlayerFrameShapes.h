#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"
#include "UI/Style/LooterUIStyle.h"

/**
 * The player frame's bar measurements and drawing helpers, shared by UHudPlayerFrameWidget's pictures
 * (HudPlayerFrameWidgetPictures.cpp) and its bars' slanted stretches (HudPlayerFrameWidgetStretches.cpp); nothing else
 * uses them. The bars are measured in pixels at 1080p from their box's top-left before the lean (the frame's HealthBarX/Y
 * and XPBarX/Y), from Docs/Handoffs/CloudIslandConcepts_2026-10-05.md, "The player frame".
 */
namespace HudFrameShapes
{
	using FPoints = TArray<FVector2D>;

	/** Every HUD bar leans 16 degrees: a point this many pixels further up sits this many times as far right (tan 16°). */
	constexpr float LeanPerPixel = 0.28674539f;

	// --- The health bar ---
	constexpr float HealthLength = 459.f;
	constexpr float HealthThickness = 27.f;
	constexpr float HealthMid = HealthThickness * 0.5f;
	constexpr float HealthRim = 1.7f;
	/** The track: inset 3.4 px in the bezel, and starting 34 px in, so an empty bar still begins at the medallion's edge. */
	constexpr float TrackLeft = HealthRim + 34.f;
	constexpr float TrackRight = HealthLength - HealthRim - 3.4f;
	constexpr float TrackTop = HealthRim + 3.4f;
	constexpr float TrackBottom = HealthThickness - TrackTop;
	constexpr float TrackHeight = TrackBottom - TrackTop;
	/** Half the lean across the track's height: a stretch's end is cut this far left of its level, its top this far right. */
	constexpr float TrackHalfLean = TrackHeight * 0.5f * LeanPerPixel;
	/** The bar's pictures' box, round the bar with room for the lean, the clamp and the shadow; the track's narrower one. */
	constexpr float HealthBoxLeft = -8.f;
	constexpr float HealthBoxTop = -8.f;
	constexpr float HealthBoxWidth = 484.f;
	constexpr float HealthBoxHeight = 44.f;
	constexpr float TrackBoxTop = 4.f;
	constexpr float TrackBoxHeight = 19.f;
	/** An end piece's box round its cut (at 0), and how far its colors reach back over the body so no seam shows. */
	constexpr float HealthCapLeft = -4.f;
	constexpr float HealthCapWidth = 12.f;
	constexpr float HealthCapOverlap = 1.f;
	/** The heal's shine: about a third of a full bar long, as tall as the track. */
	constexpr float ShineLength = 110.f;

	// --- The experience bar ---
	constexpr float XPLength = 326.4f;
	constexpr float XPThickness = 13.6f;
	constexpr float XPMid = XPThickness * 0.5f;
	constexpr float XPEdgeWidth = 1.275f;
	/** Ten sections, a tenth of the level each, inside a 1.7 px border and 2.55 px apart. */
	constexpr float SectionInset = 1.7f;
	constexpr float SectionTop = SectionInset;
	constexpr float SectionBottom = XPThickness - SectionInset;
	constexpr int32 SectionCount = 10;
	constexpr float XPSectionGap = 2.55f;
	constexpr float SectionLength = (XPLength - SectionInset * 2.f - XPSectionGap * (SectionCount - 1)) / SectionCount;
	constexpr float SectionHalfLean = (SectionBottom - SectionTop) * 0.5f * LeanPerPixel;
	constexpr float XPBoxLeft = -8.f;
	constexpr float XPBoxTop = -5.f;
	constexpr float XPBoxWidth = 344.f;
	constexpr float XPBoxHeight = 24.f;
	constexpr float SectionBoxTop = 1.f;
	constexpr float SectionBoxHeight = 12.f;
	constexpr float XPCapLeft = -3.f;
	constexpr float XPCapWidth = 8.f;
	/** Half a pixel: the frame often rests faded, and a wider overlap would show there as a denser line. */
	constexpr float XPCapOverlap = 0.5f;

	/** The bars' pictures are drawn at twice their size (a unit is a pixel), so they stay crisp. */
	constexpr float BarPixelsPerUnit = 2.f;

	/** A diamond round Centre, Radius to its tips, clockwise from the top. */
	FPoints FrameDiamond(const FVector2D& Centre, float Radius);

	/** Points as a closed line (strokes are open polylines). */
	FPoints ClosedLine(FPoints Points);

	/** Points of a bar that leans about the height Mid (higher points move right), moved by Shift into a picture's box. */
	FPoints Leaned(const FPoints& Points, float Mid, const FVector2D& Shift);

	/** The box [Left, Right] x [Top, Bottom] of a bar leaning about Mid. */
	FPoints LeanBox(float Left, float Top, float Right, float Bottom, float Mid, const FVector2D& Shift);

	/**
	 * The part of a stretch's end piece left of its leaning cut, between heights Top and Bottom: the cut runs from (0,
	 * CutBottom) up to the right with the lean, and the piece reaches Overlap back over the body.
	 */
	FPoints EndWedge(float Top, float Bottom, float CutBottom, float Overlap, const FVector2D& Shift);

	/** Subject cut to the inside of Clip, a convex polygon listed clockwise on screen (Sutherland-Hodgman). */
	FPoints ClipToConvex(const FPoints& Subject, const FPoints& Clip);

	/** Painted layers: filled shapes, stroked lines, and a layer's color made a straight gradient. */
	LooterUI::FPaintLayer PaintFill(TArray<FPoints> Shapes, const FLinearColor& Color, float Opacity = 1.f);
	LooterUI::FPaintLayer PaintStroke(TArray<FPoints> Lines, float Width, const FLinearColor& Color, float Opacity = 1.f);
	LooterUI::FPaintLayer PaintGraded(LooterUI::FPaintLayer Layer, const FLinearColor& To, const FVector2D& Start, const FVector2D& End);

	/**
	 * A soft drop shadow (ink) under a shape, faked with three widening layers since a picture can't blur: Outline(Grow)
	 * is the shape moved down and grown by Grow, up to Spread. Holes are cut out of each, where a background (a track, the
	 * portrait's glass) would otherwise show it through when the UI transparency setting fades that background.
	 */
	void AddSoftShadow(LooterUI::FPaintedIcon& Picture, TFunctionRef<FPoints(float)> Outline, const TArray<FPoints>& Holes,
		float Spread);

	/**
	 * Cuts the portrait's window out of a health picture (Shift moves the bar's pixels into it): the bar's left end is
	 * tucked behind the medallion, partly behind the window, and nothing may show through the portrait's glass when the
	 * UI transparency setting fades it.
	 */
	void KeepOutOfWindow(LooterUI::FPaintedIcon& Picture, const FVector2D& Shift);

	/** Where experience section Section starts along the bar (at its middle height, before the lean). */
	float SectionStart(int32 Section);

	/** The ten sections between heights Top and Bottom. */
	TArray<FPoints> XPSections(float Top, float Bottom, const FVector2D& Shift);
}

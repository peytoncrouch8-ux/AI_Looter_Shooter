// PauseMenuArt's dial and backdrop (E2 "Big Dial", the user's pick 2026-10-30, with the sun taken out: E2b). The dial is the
// mockup's emblemSVG with its needleCentre (Docs/Archive/Work/2026-10-30-PauseMenu/mockups/e.js): the main menu's salt ring and
// ticks (LooterReticle::Halo's proportions), a faint inner scale, a fine inner ring with inward ticks, and the slim gold
// needle on its hollow pivot. The tags, cards and small pieces are in PauseMenuArtShapes.cpp.

#include "UI/Menus/PauseMenuArt.h"
#include "UI/Menus/PauseMenuArtInk.h"
#include "UI/Style/LooterReticleArt.h"

using namespace LooterUI;
using namespace PauseMenuArtInk;

namespace
{
	// --- The ring's proportions: the main menu's halo (LooterReticle::Halo), which the mockup copies ---

	float TickGap(float Radius) { return FMath::Max(4.f, Radius * 0.05f); }
	float TickLength(float Radius) { return FMath::Max(6.f, Radius * 0.07f); }
	float LongExtra(float Radius) { return FMath::Max(7.f, Radius * 0.09f); }
	bool IsBig(float Radius) { return Radius >= 80.f; }

	/** The gold arc inside the ring beside the aim: its radius (inside the faint scale), half its sweep, its width. */
	float GoldArcRadius(float Radius) { return Radius * 0.875f * 0.9f; }
	constexpr float GoldArcHalf = 14.f;
	float GoldArcWidth(float Radius) { return FMath::Max(3.f, Radius * 0.03f); }

	/** The needle: half its width at the pivot, its gold length to just short of the ring, its salt tail. */
	float NeedleHalfWidth(float Radius) { return FMath::Max(2.8f, Radius * 0.038f); }
	float NeedleTip(float Radius) { return Radius * 0.9f; }
	float NeedleTail(float Radius) { return Radius * 0.26f; }
	float PivotRadius(float Radius) { return FMath::Max(6.f, Radius * 0.075f); }

	/** The range arc's lit run: 32 px of the arc each way of the aim, 5 px gold; the lead stops 28 short of the arc. */
	constexpr float RunHalfPx = 32.f;
	constexpr float RunWidth = 5.f;
	constexpr float LeadShort = 28.f;

	/** The face: ring, ticks, faint scale, inner ring and its ticks, round Centre. */
	void AddFace(FInk& Ink, const FVector2D& Centre, float Radius)
	{
		const FLinearColor Salt = Color::SaltLine();
		Ink.Line({ CirclePoints(Centre, Radius, true) }, IsBig(Radius) ? 3.f : 2.5f, Salt, 1.f, true);

		// Ticks outside the ring every 10 degrees, the four at the cross long.
		TArray<TArray<FVector2D>> Shorts;
		TArray<TArray<FVector2D>> Longs;
		const float Inner = Radius + TickGap(Radius);
		const float Outer = Inner + TickLength(Radius);
		for (int32 Index = 0; Index < 36; ++Index)
		{
			const bool bLong = Index % 9 == 0;
			(bLong ? Longs : Shorts).Add(Spoke(Centre, Index * 10.f, Inner, Outer + (bLong ? LongExtra(Radius) : 0.f)));
		}
		Ink.Line(MoveTemp(Shorts), IsBig(Radius) ? 2.f : 1.6f, Salt, 0.85f, true);
		Ink.Line(MoveTemp(Longs), IsBig(Radius) ? 2.6f : 2.f, Salt, 1.f, true);

		// A faint scale between the needle's tip and the ring, so a big dial doesn't read empty.
		TArray<TArray<FVector2D>> Faint;
		for (int32 Index = 0; Index < 72; ++Index)
		{
			Faint.Add(Spoke(Centre, Index * 5.f, Radius * 0.86f, Radius * (Index % 2 ? 0.9f : 0.93f)));
		}
		Ink.Line(MoveTemp(Faint), 1.2f, Salt, 0.32f, false);
		Ink.Line({ CirclePoints(Centre, Radius * 0.84f, true) }, 1.2f, Salt, 0.22f, false);

		// The open centre (the sun's place): a fine inner ring with ticks pointing in, long at the cross.
		const float InnerRing = Radius * 0.64f;
		Ink.Line({ CirclePoints(Centre, InnerRing, true) }, 1.6f, Salt, 0.7f, true);
		TArray<TArray<FVector2D>> Small;
		TArray<TArray<FVector2D>> Cross;
		for (int32 Index = 0; Index < 72; ++Index)
		{
			const bool bCross = Index % 18 == 0;
			const float Length = bCross ? Radius * 0.13f : (Index % 2 == 0 ? Radius * 0.06f : Radius * 0.035f);
			(bCross ? Cross : Small).Add(Spoke(Centre, Index * 5.f, InnerRing, InnerRing - Length));
		}
		Ink.Line(MoveTemp(Small), IsBig(Radius) ? 1.4f : 1.1f, Salt, 0.45f, false);
		Ink.Line(MoveTemp(Cross), IsBig(Radius) ? 2.f : 1.6f, Salt, 0.9f, true);
	}

	/**
	 * What turns with the aim, round Centre, aimed at Aim: the gold arc inside the ring, the needle and its tail, the pivot;
	 * with ArcRadius > 0 the gold lead out to the range arc and the gold run on it.
	 */
	void AddNeedle(FInk& Ink, const FVector2D& Centre, float Radius, float Aim, float ArcRadius)
	{
		const FLinearColor Gold = Color::SoulGold();
		Ink.Line({ ArcPoints(Centre, GoldArcRadius(Radius), Aim - GoldArcHalf, Aim + GoldArcHalf) }, GoldArcWidth(Radius), Gold, 1.f, true);
		if (ArcRadius > 0.f)
		{
			const float LeadFrom = Radius + TickGap(Radius) + TickLength(Radius) + 6.f;
			Ink.Line({ Spoke(Centre, Aim, LeadFrom, ArcRadius - LeadShort) }, 2.f, Gold, 1.f, true);
			const float RunHalf = FMath::RadiansToDegrees(RunHalfPx / ArcRadius);
			Ink.Line({ ArcPoints(Centre, ArcRadius, Aim - RunHalf, Aim + RunHalf) }, RunWidth, Gold, 1.f, true);
		}

		// The needle: a long thin diamond, gold to just short of the ring, a short salt tail behind the pivot.
		const float Angle = FMath::DegreesToRadians(Aim);
		const FVector2D Along(FMath::Cos(Angle), FMath::Sin(Angle));
		const FVector2D Side(-Along.Y, Along.X);
		const float Half = NeedleHalfWidth(Radius);
		const TArray<FVector2D> Front = { Centre - Side * Half, Centre + Along * NeedleTip(Radius), Centre + Side * Half };
		const TArray<FVector2D> Tail = { Centre - Side * Half, Centre - Along * NeedleTail(Radius), Centre + Side * Half };
		Ink.Shape(Front, Gold, 0.75f);
		Ink.Shape(Tail, Color::SaltLine(), 0.75f);

		// The pivot: a hollow salt ring on dark glass, a gold dot in it.
		const float Pivot = PivotRadius(Radius);
		Ink.Add(FillLayer({ CirclePoints(Centre, Pivot, false) }, Color::SlateGlassFoot()));
		Ink.Line({ CirclePoints(Centre, Pivot, true) }, IsBig(Radius) ? 2.5f : 2.f, Color::SaltLine(), 1.f, false);
		Ink.Add(FillLayer({ CirclePoints(Centre, FMath::Max(2.2f, Pivot * 0.38f), false) }, Gold));
	}

	/** DialNeedle's layers round the origin, and the box they reach. */
	TArray<FPaintLayer> NeedleLayers(float Radius, float ArcRadius, FBox2D& OutBounds)
	{
		FInk Ink;
		AddNeedle(Ink, FVector2D::ZeroVector, Radius, 0.f, ArcRadius);
		TArray<FPaintLayer> Layers = Ink.Layers();
		OutBounds = LayersBounds(Layers);
		return Layers;
	}

	// --- The backdrop's field (e.js: the page shade, common.css: the world dimmed and its vignette) ---

	constexpr int32 FieldWidth = 96;
	constexpr int32 FieldHeight = 54;
	/** The game under the menu dimmed as a whole (common.css: brightness .76). */
	constexpr float WorldDim = 0.24f;
	/** The shade across the screen: its darkness at these shares of the width (e.js hub: .86, .74, .22, .34). */
	constexpr float ShadeStops[][2] = { { 0.f, 0.86f }, { 0.34f, 0.74f }, { 0.62f, 0.22f }, { 1.f, 0.34f } };
	/** The vignette: an ellipse 75% x 70% round (58%, 45%); clear in the middle, .25 at 55% out, .85 at the rim. */
	constexpr float VignetteX = 0.58f;
	constexpr float VignetteY = 0.45f;
	constexpr float VignetteRadiusX = 0.75f;
	constexpr float VignetteRadiusY = 0.7f;

	float ShadeAt(float U)
	{
		for (int32 Index = 1; Index < static_cast<int32>(UE_ARRAY_COUNT(ShadeStops)); ++Index)
		{
			if (U <= ShadeStops[Index][0])
			{
				const float Alpha = (U - ShadeStops[Index - 1][0]) / (ShadeStops[Index][0] - ShadeStops[Index - 1][0]);
				return FMath::Lerp(ShadeStops[Index - 1][1], ShadeStops[Index][1], Alpha);
			}
		}
		return ShadeStops[UE_ARRAY_COUNT(ShadeStops) - 1][1];
	}

	float VignetteAt(float U, float V)
	{
		const float D = FMath::Sqrt(FMath::Square((U - VignetteX) / VignetteRadiusX) + FMath::Square((V - VignetteY) / VignetteRadiusY));
		if (D <= 0.55f)
		{
			return 0.25f * D / 0.55f;
		}
		return D >= 1.f ? 0.85f : FMath::Lerp(0.25f, 0.85f, (D - 0.55f) / 0.45f);
	}
}

float PauseMenuArt::DialExtent(float Radius)
{
	// The long ticks' tips, their half width and dark edge, then the pad.
	return Radius + TickGap(Radius) + TickLength(Radius) + LongExtra(Radius) + (2.6f + EdgeExtra) * 0.5f + LooterReticle::Pad;
}

FPaintedIcon PauseMenuArt::DialFace(float Radius)
{
	const float Extent = DialExtent(Radius);
	FInk Ink;
	AddFace(Ink, FVector2D(Extent, Extent), Radius);
	return Ink.Finish(FVector2D(Extent * 2.f, Extent * 2.f));
}

FPaintedIcon PauseMenuArt::Dial(float Radius, float NeedleDeg)
{
	const float Extent = DialExtent(Radius);
	const FVector2D Centre(Extent, Extent);
	FInk Face;
	AddFace(Face, Centre, Radius);
	// The needle over the whole face, its own edges over the face's lines.
	FInk Needle;
	AddNeedle(Needle, Centre, Radius, NeedleDeg, 0.f);
	FPaintedIcon Icon = Face.Finish(FVector2D(Extent * 2.f, Extent * 2.f));
	Icon.Layers.Append(Needle.Layers());
	return Icon;
}

FVector2D PauseMenuArt::DialNeedleCentre(float Radius, float ArcRadius)
{
	FBox2D Bounds(ForceInit);
	NeedleLayers(Radius, ArcRadius, Bounds);
	return FVector2D(LooterReticle::Pad, LooterReticle::Pad) - Bounds.Min;
}

FPaintedIcon PauseMenuArt::DialNeedle(float Radius, float ArcRadius)
{
	FBox2D Bounds(ForceInit);
	FPaintedIcon Icon;
	Icon.Layers = NeedleLayers(Radius, ArcRadius, Bounds);
	MoveLayers(Icon.Layers, FVector2D(LooterReticle::Pad, LooterReticle::Pad) - Bounds.Min);
	Icon.ViewBox = Bounds.GetSize() + FVector2D(LooterReticle::Pad * 2.f);
	return Icon;
}

FPaintedIcon PauseMenuArt::Backdrop()
{
	// The field's darkness texel by texel, the dimmed world, the shade and the vignette laid over each other, then painted
	// in runs of equal darkness (one layer per step of 1/160: the texture's filter blends the steps away).
	constexpr int32 Steps = 160;
	TMap<int32, TArray<TArray<FVector2D>>> Runs;
	for (int32 Y = 0; Y < FieldHeight; ++Y)
	{
		const float V = (Y + 0.5f) / FieldHeight;
		int32 RunStart = 0;
		int32 RunStep = INDEX_NONE;
		for (int32 X = 0; X <= FieldWidth; ++X)
		{
			int32 Step = INDEX_NONE;
			if (X < FieldWidth)
			{
				const float U = (X + 0.5f) / FieldWidth;
				const float Clear = (1.f - WorldDim) * (1.f - ShadeAt(U)) * (1.f - VignetteAt(U, V));
				Step = FMath::Clamp(FMath::RoundToInt32((1.f - Clear) * Steps), 0, Steps);
			}
			if (Step != RunStep)
			{
				if (RunStep > 0 && X > RunStart)
				{
					Runs.FindOrAdd(RunStep).Add({ FVector2D(RunStart, Y), FVector2D(X, Y), FVector2D(X, Y + 1), FVector2D(RunStart, Y + 1) });
				}
				RunStart = X;
				RunStep = Step;
			}
		}
	}
	FPaintedIcon Icon;
	Icon.ViewBox = FVector2D(FieldWidth, FieldHeight);
	for (TPair<int32, TArray<TArray<FVector2D>>>& Run : Runs)
	{
		Icon.Layers.Add(FillLayer(MoveTemp(Run.Value), Color::Backdrop().CopyWithNewOpacity(float(Run.Key) / Steps)));
	}
	return Icon;
}

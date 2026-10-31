// PauseMenuArt's shapes, from the mockup's C "Tag Rack" pieces (Docs/Archive/Work/2026-10-30-PauseMenu/mockups/e.js hTagPts,
// vTagPts, the callout, gemIcon, ornament; common.js pipsSVG, barSVG, portraitHTML): the options' tags on their side, the
// cards standing up like the gun card's tag, the settings' pennants, the gems, and the hero card's small pieces. The
// glass is a background (EPart::Fill, faded by the UI transparency), the lines stay solid (EPart::Lines).

#include "UI/Menus/PauseMenuArt.h"
#include "UI/Menus/PauseMenuArtInk.h"
#include "UI/Style/LooterReticleArt.h"

using namespace LooterUI;
using namespace PauseMenuArtInk;

namespace
{
	using PauseMenuArt::ELook;
	using PauseMenuArt::EPart;

	constexpr float Pad = LooterReticle::Pad;

	/** The standing cards' shoulders and raised step (vTagPts: cut 26, the step 12 high and 12 wide each side). */
	constexpr float CardCut = 26.f;
	constexpr float StepSlope = 12.f;
	/** The callout's point back at its tag: 28 deep, 30 each way of its tip (e.js). */
	constexpr float CalloutDepth = 28.f;
	constexpr float CalloutHalf = 30.f;
	/** The help card's and callout's step for a gem (e.js: notch 50), the pennant's (30) and its point (14). */
	constexpr float CardNotchHalf = 50.f;
	constexpr float PennantNotchHalf = 30.f;
	constexpr float PennantPoint = 14.f;
	/** The pennant's hook: a 32 px diamond, its top 14 px over the body. */
	constexpr float HookSize = 32.f;
	constexpr float HookAbove = 14.f;

	/** The lit gem's inside: the gem's dark with the gold washed over it (the mockup's warm brown, from the palette). */
	constexpr float GemLitWash = 0.22f;

	/**
	 * The gun card's tag standing up (vTagPts), W x H from the origin: a raised step NotchHalf each side of the middle along
	 * the top (none at 0), cut shoulders, and a point Point deep at the foot (0: cut corners).
	 */
	TArray<FVector2D> StandingOutline(float W, float H, float NotchHalf, float Point)
	{
		const float Top = NotchHalf > 0.f ? PauseMenuArt::CardTopStep : 0.f;
		const float Mid = W * 0.5f;
		TArray<FVector2D> Points = { FVector2D(0.f, Top + CardCut), FVector2D(CardCut, Top) };
		if (NotchHalf > 0.f)
		{
			Points.Append({ FVector2D(Mid - NotchHalf - StepSlope, Top), FVector2D(Mid - NotchHalf, 0.f), FVector2D(Mid + NotchHalf, 0.f),
				FVector2D(Mid + NotchHalf + StepSlope, Top) });
		}
		Points.Append({ FVector2D(W - CardCut, Top), FVector2D(W, Top + CardCut) });
		if (Point > 0.f)
		{
			Points.Append({ FVector2D(W, H - Point - CardCut), FVector2D(W - CardCut, H - Point), FVector2D(Mid, H), FVector2D(CardCut, H - Point),
				FVector2D(0.f, H - Point - CardCut) });
		}
		else
		{
			Points.Append({ FVector2D(W, H - CardCut), FVector2D(W - CardCut, H), FVector2D(CardCut, H), FVector2D(0.f, H - CardCut) });
		}
		return Points;
	}

	/** The callout (e.js): square on its left, where a point reaches back PointY down; cut on its right; a gem step on top
	 *  if asked. */
	TArray<FVector2D> CalloutOutline(float W, float H, float PointY, bool bStep)
	{
		const float Top = bStep ? PauseMenuArt::CardTopStep : 0.f;
		const float Mid = (CalloutDepth + W) * 0.5f;
		const float Tip = FMath::Clamp(PointY, Top + CalloutHalf, H - CalloutHalf);
		TArray<FVector2D> Points = { FVector2D(CalloutDepth, Top) };
		if (bStep)
		{
			Points.Append({ FVector2D(Mid - CardNotchHalf - StepSlope, Top), FVector2D(Mid - CardNotchHalf, 0.f), FVector2D(Mid + CardNotchHalf, 0.f),
				FVector2D(Mid + CardNotchHalf + StepSlope, Top) });
		}
		Points.Append({ FVector2D(W - CardCut, Top), FVector2D(W, Top + CardCut), FVector2D(W, H - CardCut), FVector2D(W - CardCut, H),
			FVector2D(CalloutDepth, H), FVector2D(CalloutDepth, Tip + CalloutHalf), FVector2D(0.f, Tip), FVector2D(CalloutDepth, Tip - CalloutHalf) });
		return Points;
	}

	/** The option's tag on its side (hTagPts), W x H from the origin: the gem's notch at the left, the point at the right. */
	TArray<FVector2D> SideTagOutline(float W, float H)
	{
		const float M = H * 0.5f;
		return { FVector2D(34.f, 0.f), FVector2D(W - 42.f, 0.f), FVector2D(W - 18.f, 18.f), FVector2D(W, M), FVector2D(W - 18.f, H - 18.f),
			FVector2D(W - 42.f, H), FVector2D(34.f, H), FVector2D(12.f, H - 22.f), FVector2D(12.f, M + 28.f), FVector2D(0.f, M + 18.f),
			FVector2D(0.f, M - 18.f), FVector2D(12.f, M - 28.f), FVector2D(12.f, 22.f) };
	}

	TArray<FVector2D> Diamond(const FVector2D& Origin, float Size, float Inset)
	{
		const float H = Size * 0.5f;
		return { Origin + FVector2D(H, Inset), Origin + FVector2D(Size - Inset, H), Origin + FVector2D(H, Size - Inset), Origin + FVector2D(Inset, H) };
	}

	/** How a shape is painted: its glass from one colour to another along a line, its line. */
	struct FShapeLook
	{
		FLinearColor FillFrom;
		FLinearColor FillTo;
		FVector2D GradientStart;
		FVector2D GradientEnd;
		float LineWidth = 3.f;
		FLinearColor Line;
		float LineOpacity = 1.f;
	};

	/** The outline (in view-box units) painted as Look says: the glass for Fill, the line for Lines, both for Whole. */
	void AddShape(FPaintedIcon& Icon, const TArray<FVector2D>& Outline, const FShapeLook& Look, EPart Part)
	{
		if (Part != EPart::Lines)
		{
			FPaintLayer Glass = FillLayer({ Outline }, Look.FillFrom);
			Glass.GradientTo = Look.FillTo;
			Glass.GradientStart = Look.GradientStart;
			Glass.GradientEnd = Look.GradientEnd;
			Icon.Layers.Add(MoveTemp(Glass));
		}
		if (Part != EPart::Fill)
		{
			Icon.Layers.Add(StrokeLayer({ ClosedLine(Outline) }, Look.LineWidth, Look.Line, Look.LineOpacity));
		}
	}

	/** The slate glass from top to foot over a box from Top to Bottom (the cards'). */
	FShapeLook GlassLook(float Top, float Bottom, float LineWidth, float LineOpacity)
	{
		FShapeLook Look;
		Look.FillFrom = Color::SlateGlassTop();
		Look.FillTo = Color::SlateGlassFoot();
		Look.GradientStart = FVector2D(0.f, Top);
		Look.GradientEnd = FVector2D(0.f, Bottom);
		Look.LineWidth = LineWidth;
		Look.Line = Color::SaltLine();
		Look.LineOpacity = LineOpacity;
		return Look;
	}

	/** A tag's or pennant's look: lit gold over a gold wash, danger red over a red wash, plain salt over glass. */
	FShapeLook TabLook(ELook Look, const FVector2D& From, const FVector2D& To, float LitFoot)
	{
		FShapeLook Result;
		Result.GradientStart = From;
		Result.GradientEnd = To;
		switch (Look)
		{
		case ELook::Lit:
			Result.FillFrom = Color::SoulGold().CopyWithNewOpacity(0.3f);
			Result.FillTo = Color::SlateGlassTop().CopyWithNewOpacity(LitFoot);
			Result.LineWidth = 4.f;
			Result.Line = Color::SoulGold();
			Result.LineOpacity = 1.f;
			break;
		case ELook::Danger:
			Result.FillFrom = Color::Danger().CopyWithNewOpacity(0.6f);
			Result.FillTo = Color::SlateGlassFoot();
			Result.LineWidth = 3.f;
			Result.Line = Color::Worse();
			Result.LineOpacity = 0.75f;
			break;
		default:
			Result.FillFrom = Color::SlateGlassTop();
			Result.FillTo = Color::SlateGlassFoot();
			Result.LineWidth = 3.f;
			Result.Line = Color::SaltLine();
			Result.LineOpacity = 0.55f;
			break;
		}
		return Result;
	}

	FLinearColor LookLine(ELook Look)
	{
		return Look == ELook::Lit ? Color::SoulGold() : Look == ELook::Danger ? Color::Worse() : Color::SaltLine();
	}

	/** A gem's inside: dark, or with the gold washed over it when lit. */
	void AddGemInside(FPaintedIcon& Icon, const TArray<FVector2D>& Outline, bool bLit)
	{
		Icon.Layers.Add(FillLayer({ Outline }, bLit ? Color::SlateGem() : Color::SlateGlassFoot()));
		if (bLit)
		{
			Icon.Layers.Add(FillLayer({ Outline }, Color::SoulGold(), GemLitWash));
		}
	}
}

// ---------------------------------------------------------------------------
// Tags, cards and pennants
// ---------------------------------------------------------------------------

FPaintedIcon PauseMenuArt::Tag(float Width, float Height, ELook Look, EPart Part)
{
	// hTagPts(w - 4, h - 4) moved in 2, so the 4 px lit line reaches the box's edge; the wash runs left to right.
	FPaintedIcon Icon;
	Icon.ViewBox = FVector2D(Width + Pad * 2.f, Height + Pad * 2.f);
	const FVector2D Origin(Pad + 2.f, Pad + 2.f);
	const TArray<FVector2D> Outline = Moved(SideTagOutline(Width - 4.f, Height - 4.f), Origin);
	AddShape(Icon, Outline, TabLook(Look, FVector2D(Pad, 0.f), FVector2D(Pad + Width, 0.f), 0.9f), Part);
	if (Look == ELook::Lit && Part != EPart::Fill)
	{
		// Two gold rivets by the point.
		for (const float Y : { 14.f, Height - 14.f })
		{
			Icon.Layers.Add(FillLayer({ CirclePoints(FVector2D(Pad + Width - 46.f, Pad + Y), 3.5f, false) }, Color::SoulGold()));
		}
	}
	return Icon;
}

FPaintedIcon PauseMenuArt::Card(float Width, float Height, bool bTopGem, bool bPointLeft, float PointY, EPart Part)
{
	FPaintedIcon Icon;
	Icon.ViewBox = FVector2D(Width + Pad * 2.f, Height + Pad * 2.f);
	if (bPointLeft)
	{
		// The callout: its points as the mockup has them, the 3 px line centred on its edge (the pad holds the outer half).
		const TArray<FVector2D> Outline = Moved(CalloutOutline(Width, Height, PointY, bTopGem), FVector2D(Pad, Pad));
		AddShape(Icon, Outline, GlassLook(Pad, Pad + Height, 3.f, 0.8f), Part);
	}
	else
	{
		const TArray<FVector2D> Outline = Moved(StandingOutline(Width - 4.f, Height - 4.f, bTopGem ? CardNotchHalf : 0.f, 0.f), FVector2D(Pad + 2.f, Pad + 2.f));
		AddShape(Icon, Outline, GlassLook(Pad, Pad + Height, 3.f, 0.75f), Part);
	}
	return Icon;
}

FPaintedIcon PauseMenuArt::TagCard(float Width, float Height, float NotchHalf, float PointDepth, float LineWidth, float LineOpacity, EPart Part)
{
	FPaintedIcon Icon;
	Icon.ViewBox = FVector2D(Width + Pad * 2.f, Height + Pad * 2.f);
	const TArray<FVector2D> Outline = Moved(StandingOutline(Width - 4.f, Height - 4.f, NotchHalf, PointDepth), FVector2D(Pad + 2.f, Pad + 2.f));
	AddShape(Icon, Outline, GlassLook(Pad, Pad + Height, LineWidth, LineOpacity), Part);
	return Icon;
}

FPaintedIcon PauseMenuArt::Pennant(float Width, float Height, ELook Look, EPart Part)
{
	// The gun card's pennant (vTagPts: notch 30, point 14), hanging from its hook; the wash runs top to foot.
	FPaintedIcon Icon;
	Icon.ViewBox = FVector2D(Width + Pad * 2.f, Height + PennantHookRise + Pad * 2.f);
	const FVector2D Body(Pad, Pad + PennantHookRise);
	const TArray<FVector2D> Outline = Moved(StandingOutline(Width - 4.f, Height - 4.f, PennantNotchHalf, PennantPoint), Body + FVector2D(2.f, 2.f));
	AddShape(Icon, Outline, TabLook(Look, FVector2D(0.f, Body.Y), FVector2D(0.f, Body.Y + Height), 0.92f), Part);
	if (Part != EPart::Fill)
	{
		const TArray<FVector2D> Hook = Diamond(Body + FVector2D(Width * 0.5f - HookSize * 0.5f, -HookAbove), HookSize, 2.f);
		AddGemInside(Icon, Hook, Look == ELook::Lit);
		Icon.Layers.Add(StrokeLayer({ ClosedLine(Hook) }, 3.f, LookLine(Look)));
	}
	return Icon;
}

// ---------------------------------------------------------------------------
// Gems
// ---------------------------------------------------------------------------

FPaintedIcon PauseMenuArt::GemLook(float Size, ELook Look, EPart Part)
{
	// The mockup's gem: a diamond with a 3.5 px line, kept inside its own box.
	FPaintedIcon Icon;
	Icon.ViewBox = FVector2D(Size, Size);
	const TArray<FVector2D> Outline = Diamond(FVector2D::ZeroVector, Size, 2.5f);
	if (Part != EPart::Lines)
	{
		AddGemInside(Icon, Outline, Look == ELook::Lit);
	}
	if (Part != EPart::Fill)
	{
		Icon.Layers.Add(StrokeLayer({ ClosedLine(Outline) }, 3.5f, LookLine(Look)));
	}
	return Icon;
}

FPaintedIcon PauseMenuArt::Gem(float Size, bool bLit, EPart Part)
{
	return GemLook(Size, bLit ? ELook::Lit : ELook::Plain, Part);
}

FPaintedIcon PauseMenuArt::GoldGem(float Size, float LineWidth)
{
	FPaintedIcon Icon;
	Icon.ViewBox = FVector2D(Size, Size);
	const TArray<FVector2D> Outline = Diamond(FVector2D::ZeroVector, Size, LineWidth * 0.5f + 0.75f);
	Icon.Layers.Add(FillLayer({ Outline }, Color::SlateGlassFoot()));
	Icon.Layers.Add(StrokeLayer({ ClosedLine(Outline) }, LineWidth, Color::SoulGold()));
	return Icon;
}

// ---------------------------------------------------------------------------
// The small pieces
// ---------------------------------------------------------------------------

FPaintedIcon PauseMenuArt::Ornament(float Width)
{
	FPaintedIcon Icon;
	Icon.ViewBox = FVector2D(Width, 14.f);
	const float Mid = Width * 0.5f;
	Icon.Layers.Add(StrokeLayer({ { FVector2D(1.f, 7.f), FVector2D(Mid - 16.f, 7.f) }, { FVector2D(Mid + 16.f, 7.f), FVector2D(Width - 1.f, 7.f) } },
		1.5f, Color::SaltLine(), 0.5f));
	Icon.Layers.Add(StrokeLayer({ ClosedLine({ FVector2D(Mid, 1.5f), FVector2D(Mid + 6.f, 7.f), FVector2D(Mid, 12.5f), FVector2D(Mid - 6.f, 7.f) }) },
		2.f, Color::SoulGold()));
	return Icon;
}

FPaintedIcon PauseMenuArt::Pip(const FLinearColor& Tint, bool bLit)
{
	// pipsSVG: a pip leaning right, lit in its colour or dark with a faint grey edge.
	FPaintedIcon Icon;
	Icon.ViewBox = FVector2D(PipWidth + PipLean + 2.f, PipHeight);
	const TArray<FVector2D> Outline = { FVector2D(1.f + PipLean, 1.f), FVector2D(1.f + PipWidth + PipLean, 1.f), FVector2D(1.f + PipWidth, PipHeight - 1.f),
		FVector2D(1.f, PipHeight - 1.f) };
	Icon.Layers.Add(FillLayer({ Outline }, bLit ? Tint : Color::Outline().CopyWithNewOpacity(0.6f)));
	Icon.Layers.Add(StrokeLayer({ ClosedLine(Outline) }, 1.5f, bLit ? Tint : Color::SlateIcon(), bLit ? 1.f : 0.6f));
	return Icon;
}

FPaintedIcon PauseMenuArt::StepPip(int32 State)
{
	// The mission card's steps: done ones filled salt, the current one gold, the rest hollow.
	FPaintedIcon Icon;
	Icon.ViewBox = FVector2D(StepPipSize, StepPipSize);
	const TArray<FVector2D> Outline = Diamond(FVector2D::ZeroVector, StepPipSize, 1.f);
	if (State == 0 || State == 1)
	{
		Icon.Layers.Add(FillLayer({ Outline }, State == 1 ? Color::SoulGold() : Color::SaltLine()));
	}
	Icon.Layers.Add(StrokeLayer({ ClosedLine(Outline) }, 1.8f, State == 1 ? Color::SoulGold() : Color::SlateIcon()));
	return Icon;
}

FPaintedIcon PauseMenuArt::SlantBar(float Width, float Height, bool bFill, const FLinearColor& Tint)
{
	// barSVG: leaning like the HUD's bars; the fill sits 2 px inside the track at each end.
	FPaintedIcon Icon;
	Icon.ViewBox = FVector2D(Width, Height);
	const float Lean = Height * 0.7f;
	if (bFill)
	{
		Icon.Layers.Add(FillLayer({ { FVector2D(2.f + Lean, 0.f), FVector2D(Width - 2.f, 0.f), FVector2D(Width - 2.f - Lean, Height), FVector2D(2.f, Height) } }, Tint));
	}
	else
	{
		const TArray<FVector2D> Track = { FVector2D(Lean, 0.75f), FVector2D(Width - 0.75f, 0.75f), FVector2D(Width - Lean, Height - 0.75f),
			FVector2D(0.75f, Height - 0.75f) };
		Icon.Layers.Add(FillLayer({ Track }, Color::Outline().CopyWithNewOpacity(0.7f)));
		Icon.Layers.Add(StrokeLayer({ ClosedLine(Track) }, 1.5f, Color::SaltLine(), 0.3f));
	}
	return Icon;
}

FPaintedIcon PauseMenuArt::Medallion(float Size)
{
	// portraitHTML's bezel: gunmetal under a salt line, a faint salt diamond inside; hollow, the portrait shows through.
	FPaintedIcon Icon;
	Icon.ViewBox = FVector2D(Size + Pad * 2.f, Size + Pad * 2.f);
	const FVector2D Origin(Pad, Pad);
	Icon.Layers.Add(StrokeLayer({ ClosedLine(Diamond(Origin, Size, 3.f)) }, 7.f, Color::MetalMid()));
	Icon.Layers.Add(StrokeLayer({ ClosedLine(Diamond(Origin, Size, 2.f)) }, 3.f, Color::SaltLine()));
	Icon.Layers.Add(StrokeLayer({ ClosedLine(Diamond(Origin, Size, 11.f)) }, 1.2f, Color::SaltLine(), 0.35f));
	return Icon;
}

// UHudPlayerFrameWidget's pictures, each drawn once into a shared texture, and where each sits in the frame's box. The
// bars' measurements and the drawing helpers are in HudPlayerFrameShapes.h; the medallion and the gem are drawn here in
// the mockup's units (Docs/HudMockup/NewHud.dc.html draws the frame at 85%, so a unit is 0.85 px).

#include "UI/HUD/HudPlayerFrameWidget.h"
#include "UI/HUD/HudPlayerFrameShapes.h"
#include "UI/HUD/HudPortraitWidget.h"
#include "UI/Style/LooterUIStyle.h"

using namespace LooterUI;
using namespace HudFrameShapes;

namespace
{
	constexpr float MedalUnit = 0.85f;
	/** The medallion's pictures: a square this many units across round its centre (the horn's tip reaches 112 out). */
	constexpr float MedallionBox = 232.f;
	constexpr float MedalPixelsPerUnit = 1.75f;
	/**
	 * To the tips: the bezel; the window, the portrait's own (it fills it, clipped without anti-aliasing, so the window's
	 * ink edge is drawn wide and centred on its rim to hide the steps); the hairline inside that edge; the bezel's lines.
	 */
	constexpr float BezelRadius = 86.f;
	constexpr float WindowRadius = UHudPortraitWidget::WindowSize * 0.5f / MedalUnit;
	constexpr float WindowEdgeWidth = 3.f;
	constexpr float WindowHairlineRadius = 73.2f;
	constexpr float BezelLineRadius = 81.f;
	constexpr float GemBox = 52.f;
	constexpr float GemRadius = 22.f;
	constexpr float GemPixelsPerUnit = 2.f;
	/** The ring spreads to 2.6 times the gem's size, so its texture is drawn big enough to stay crisp there. */
	constexpr float RingPixelsPerUnit = 4.5f;

	/** The health fill's bands (the top 36% light, the bottom 28% dark), its lit top line and its light leading edge. */
	constexpr float HighBandBottom = TrackTop + TrackHeight * 0.36f;
	constexpr float LowBandTop = TrackBottom - TrackHeight * 0.28f;
	constexpr float FillLitLine = 0.85f;
	constexpr float LeadEdgeWidth = 1.7f;
	/** The hatch: lines falling to the right at 45 degrees, 1.2 px across and 5.1 px apart square to them, black at 13%. */
	constexpr float FillHatchGap = 5.1f;
	constexpr float FillHatchWidth = 1.2f;
	constexpr float FillHatchOpacity = 0.13f;
	/** The quarter cuts, and the orange "]" over the far end: 11.9 x 32.3 from just past the end, its arms 4.25 thick. */
	constexpr float QuarterCutWidth = 2.55f;
	constexpr float ClampRight = HealthLength + 5.95f;
	constexpr float ClampLeft = ClampRight - 11.9f;
	constexpr float ClampTop = -4.25f;
	constexpr float ClampBottom = ClampTop + 32.3f;
	constexpr float ClampArm = 4.25f;
	constexpr float ClampReach = 7.65f;

	FVector2D Medal(float X, float Y)
	{
		return FVector2D(MedallionBox * 0.5f + X, MedallionBox * 0.5f + Y);
	}

	FVector2D GemAt(float X, float Y)
	{
		return FVector2D(GemBox * 0.5f + X, GemBox * 0.5f + Y);
	}

	FLinearColor ShineColor()
	{
		// The heal's green, paled toward white, nearly opaque at its heart.
		return FMath::Lerp(Color::Heal(), FLinearColor::White, 0.6f).CopyWithNewOpacity(0.9f);
	}

	const FPaintedIcon& MedallionBackPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(MedallionBox);
			// The window is cut out of everything here: the portrait draws it, its glass a background that fades.
			const FPoints Window = FrameDiamond(Medal(0.f, 0.f), WindowRadius);
			AddSoftShadow(Result, [](float Grow) { return FrameDiamond(Medal(0.f, 3.f), BezelRadius + Grow * UE_SQRT_2); }, { Window }, 4.f);

			// The horn on the left tip, behind the bezel: gunmetal lit from the top-left, an orange chevron inlaid.
			const FPoints Horn = { Medal(-72.f, -17.f), Medal(-112.f, 0.f), Medal(-72.f, 17.f), Medal(-85.f, 0.f) };
			Result.Layers.Add(PaintGraded(PaintFill({ Horn }, Color::MetalHi()), Color::MetalMid(), Medal(-112.f, -17.f), Medal(-72.f, 17.f)));
			Result.Layers.Add(PaintStroke({ ClosedLine(Horn) }, 2.5f, Color::Ink()));
			const FPoints Inlay = { Medal(-84.f, -7.f), Medal(-101.f, 0.f), Medal(-84.f, 7.f), Medal(-89.f, 0.f) };
			Result.Layers.Add(PaintFill({ Inlay }, Color::Accent()));
			Result.Layers.Add(PaintStroke({ ClosedLine(Inlay) }, 1.4f, Color::Ink()));

			// The bezel, lit from the top-left: the lower half's darker gunmetal under the whole ring, then the upper half's
			// over it (one layer over another, so no seam opens between them).
			const FVector2D LitFrom = Medal(-43.f, -43.f);
			const FVector2D LitTo = Medal(43.f, 43.f);
			Result.Layers.Add(PaintGraded(PaintFill({ FrameDiamond(Medal(0.f, 0.f), BezelRadius), Window }, Color::MetalLow()),
				Color::MetalDeep(), LitFrom, LitTo));
			const FPoints UpperRing = { Medal(-BezelRadius, 0.f), Medal(0.f, -BezelRadius), Medal(BezelRadius, 0.f),
				Medal(WindowRadius, 0.f), Medal(0.f, -WindowRadius), Medal(-WindowRadius, 0.f) };
			Result.Layers.Add(PaintGraded(PaintFill({ UpperRing }, Color::MetalHi()), Color::MetalMid(), LitFrom, LitTo));
			Result.Layers.Add(PaintStroke({ ClosedLine(FrameDiamond(Medal(0.f, 0.f), BezelRadius)) }, 3.2f, Color::Ink()));
			// A light line inside the upper edges, a dark one inside the lower.
			Result.Layers.Add(PaintStroke({ { Medal(-BezelLineRadius, 0.f), Medal(0.f, -BezelLineRadius), Medal(BezelLineRadius, 0.f) } },
				1.3f, FLinearColor::White, 0.28f));
			Result.Layers.Add(PaintStroke({ { Medal(-BezelLineRadius, 0.f), Medal(0.f, BezelLineRadius), Medal(BezelLineRadius, 0.f) } },
				1.3f, Color::Ink(), 0.35f));
			return Result;
		}();
		return Picture;
	}

	/** Over the portrait: the window's ink edge (hiding the portrait's own), the cyan hairline, and the clamps on the tips. */
	const FPaintedIcon& MedallionFrontPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(MedallionBox);
			Result.Layers.Add(PaintStroke({ ClosedLine(FrameDiamond(Medal(0.f, 0.f), WindowRadius)) }, WindowEdgeWidth, Color::Ink()));
			Result.Layers.Add(PaintStroke({ ClosedLine(FrameDiamond(Medal(0.f, 0.f), WindowHairlineRadius)) }, 1.6f, Color::Hairline(), 0.9f));
			const FPoints TopClamp = { Medal(-16.f, -80.f), Medal(0.f, -98.f), Medal(16.f, -80.f), Medal(10.f, -75.f),
				Medal(0.f, -87.f), Medal(-10.f, -75.f) };
			Result.Layers.Add(PaintFill({ TopClamp }, Color::Accent()));
			Result.Layers.Add(PaintStroke({ ClosedLine(TopClamp) }, 2.2f, Color::Ink()));
			const FPoints LowClamp = { Medal(-10.f, 81.f), Medal(0.f, 93.f), Medal(10.f, 81.f), Medal(6.f, 78.f),
				Medal(0.f, 85.f), Medal(-6.f, 78.f) };
			Result.Layers.Add(PaintFill({ LowClamp }, Color::Accent()));
			Result.Layers.Add(PaintStroke({ ClosedLine(LowClamp) }, 1.8f, Color::Ink()));
			return Result;
		}();
		return Picture;
	}

	/** The level gem: two flat cyan halves (the dark one under the whole gem, the light one over its top), an ink edge. */
	const FPaintedIcon& GemPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(GemBox);
			AddSoftShadow(Result, [](float Grow) { return FrameDiamond(GemAt(0.f, 2.4f), GemRadius + Grow * UE_SQRT_2); }, {}, 3.f);
			Result.Layers.Add(PaintFill({ FrameDiamond(GemAt(0.f, 0.f), GemRadius) }, Color::GemDark()));
			Result.Layers.Add(PaintFill({ { GemAt(-GemRadius, 0.f), GemAt(0.f, -GemRadius), GemAt(GemRadius, 0.f) } }, Color::GemLight()));
			Result.Layers.Add(PaintStroke({ ClosedLine(FrameDiamond(GemAt(0.f, 0.f), GemRadius)) }, 2.8f, Color::Ink()));
			Result.Layers.Add(PaintStroke({ { GemAt(-14.5f, 0.f), GemAt(0.f, -14.5f), GemAt(14.5f, 0.f) } }, 1.3f, FLinearColor::White, 0.8f));
			return Result;
		}();
		return Picture;
	}

	/** The gem's face inside its edge, white: laid over the gem, fading, it makes the gem flash. */
	const FPaintedIcon& GemFlashPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(GemBox);
			Result.Layers.Add(PaintFill({ FrameDiamond(GemAt(0.f, 0.f), GemRadius - 1.4f) }, FLinearColor::White));
			return Result;
		}();
		return Picture;
	}

	const FPaintedIcon& GemRingPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(GemBox);
			Result.Layers.Add(PaintStroke({ ClosedLine(FrameDiamond(GemAt(0.f, 0.f), GemRadius)) }, 2.4f, Color::CyanText()));
			return Result;
		}();
		return Picture;
	}

	/** The health bar's metal: the ink rim and the gunmetal bezel, lit along its top and shaded along its bottom. */
	const FPaintedIcon& HealthBackPicture()
	{
		static const FPaintedIcon Picture = []
		{
			const FVector2D Shift(-HealthBoxLeft, -HealthBoxTop);
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(HealthBoxWidth, HealthBoxHeight);
			// The track's place is cut out of everything, so when the UI transparency setting fades the track the world
			// shows through the bar's inside.
			const FPoints Track = LeanBox(TrackLeft, TrackTop, TrackRight, TrackBottom, HealthMid, Shift);
			AddSoftShadow(Result, [&Shift](float Grow)
			{
				return LeanBox(-Grow, 1.7f - Grow, HealthLength + Grow, HealthThickness + 1.7f + Grow, HealthMid + 1.7f, Shift);
			}, { Track }, 3.f);
			Result.Layers.Add(PaintFill({ LeanBox(0.f, 0.f, HealthLength, HealthThickness, HealthMid, Shift), Track }, Color::Ink()));
			const float Right = HealthLength - HealthRim;
			const float Bottom = HealthThickness - HealthRim;
			Result.Layers.Add(PaintGraded(PaintFill({ LeanBox(HealthRim, HealthRim, Right, Bottom, HealthMid, Shift), Track },
				Color::BarMetalHi()), Color::BarMetalLow(), FVector2D(0.f, HealthRim) + Shift, FVector2D(0.f, Bottom) + Shift));
			Result.Layers.Add(PaintFill({ LeanBox(HealthRim, HealthRim, Right, HealthRim + 0.85f, HealthMid, Shift) }, FLinearColor::White, 0.35f));
			Result.Layers.Add(PaintFill({ LeanBox(HealthRim, Bottom - 0.85f, Right, Bottom, HealthMid, Shift) }, Color::Ink(), 0.5f));
			// A thin dark line inside the track's edge keeps its shape readable when the track has faded.
			Result.Layers.Add(PaintFill({ Track, LeanBox(TrackLeft + 0.9f, TrackTop + 0.85f, TrackRight - 0.9f, TrackBottom - 0.85f,
				HealthMid, Shift) }, Color::Ink(), 0.6f));
			KeepOutOfWindow(Result, Shift);
			return Result;
		}();
		return Picture;
	}

	const FPaintedIcon& HealthTrackPicture()
	{
		static const FPaintedIcon Picture = []
		{
			const FVector2D Shift(-HealthBoxLeft, -TrackBoxTop);
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(HealthBoxWidth, TrackBoxHeight);
			Result.Layers.Add(PaintFill({ LeanBox(TrackLeft, TrackTop, TrackRight, TrackBottom, HealthMid, Shift) }, Color::Track(), 0.88f));
			KeepOutOfWindow(Result, Shift);
			return Result;
		}();
		return Picture;
	}

	/** Over the fill: dark cuts at its quarters (as on creature bars), and the orange "]" clamp over the bar's far end. */
	const FPaintedIcon& HealthFrontPicture()
	{
		static const FPaintedIcon Picture = []
		{
			const FVector2D Shift(-HealthBoxLeft, -HealthBoxTop);
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(HealthBoxWidth, HealthBoxHeight);
			FPaintLayer Cuts = PaintFill({}, Color::Ink());
			for (const float Quarter : { 0.25f, 0.5f, 0.75f })
			{
				const float At = TrackLeft + Quarter * (TrackRight - TrackLeft);
				Cuts.Fills.Add(LeanBox(At - QuarterCutWidth * 0.5f, TrackTop, At + QuarterCutWidth * 0.5f, TrackBottom, HealthMid, Shift));
			}
			Result.Layers.Add(MoveTemp(Cuts));
			const float Inner = ClampLeft + ClampReach;
			const FPoints Clamp = Leaned({ FVector2D(ClampLeft, ClampTop), FVector2D(ClampRight, ClampTop),
				FVector2D(ClampRight, ClampBottom), FVector2D(ClampLeft, ClampBottom), FVector2D(ClampLeft, ClampBottom - ClampArm),
				FVector2D(Inner, ClampBottom - ClampArm), FVector2D(Inner, ClampTop + ClampArm), FVector2D(ClampLeft, ClampTop + ClampArm) },
				HealthMid, Shift);
			Result.Layers.Add(PaintFill({ Clamp }, Color::Accent()));
			Result.Layers.Add(PaintStroke({ ClosedLine(Clamp) }, 1.6f, Color::Ink()));
			return Result;
		}();
		return Picture;
	}

	/**
	 * The whole fill, cropped to length: three flat red bands (the middle one under all, the light top and dark bottom
	 * over it), its lit top line, the hatch, and the light edge along the far end (shown only when the bar is full).
	 */
	const FPaintedIcon& HealthFillBodyPicture()
	{
		static const FPaintedIcon Picture = []
		{
			const FVector2D Shift(-HealthBoxLeft, -TrackBoxTop);
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(HealthBoxWidth, TrackBoxHeight);
			const FPoints Track = LeanBox(TrackLeft, TrackTop, TrackRight, TrackBottom, HealthMid, Shift);
			Result.Layers.Add(PaintFill({ Track }, Color::Health()));
			Result.Layers.Add(PaintFill({ LeanBox(TrackLeft, TrackTop, TrackRight, HighBandBottom, HealthMid, Shift) }, Color::HealthHi()));
			Result.Layers.Add(PaintFill({ LeanBox(TrackLeft, LowBandTop, TrackRight, TrackBottom, HealthMid, Shift) }, Color::HealthLow()));
			Result.Layers.Add(PaintFill({ LeanBox(TrackLeft, TrackTop, TrackRight, TrackTop + FillLitLine, HealthMid, Shift) },
				Color::HealthEdge(), 0.45f));

			FPaintLayer FillHatch = PaintFill({}, Color::Ink(), FillHatchOpacity);
			const float Along = FillHatchGap * UE_SQRT_2;
			const float Across = FillHatchWidth * UE_SQRT_2;
			for (float Start = TrackLeft - TrackHeight - Along; Start < TrackRight + Along; Start += Along)
			{
				const FPoints Line = { FVector2D(Start, TrackTop) + Shift, FVector2D(Start + Across, TrackTop) + Shift,
					FVector2D(Start + Across + TrackHeight, TrackBottom) + Shift, FVector2D(Start + TrackHeight, TrackBottom) + Shift };
				FPoints Inside = ClipToConvex(Line, Track);
				if (Inside.Num() >= 3)
				{
					FillHatch.Fills.Add(MoveTemp(Inside));
				}
			}
			Result.Layers.Add(MoveTemp(FillHatch));
			Result.Layers.Add(PaintFill({ LeanBox(TrackRight - LeadEdgeWidth, TrackTop, TrackRight, TrackBottom, HealthMid, Shift) },
				Color::HealthEdge()));
			KeepOutOfWindow(Result, Shift);
			return Result;
		}();
		return Picture;
	}

	/** The fill's leaning end, cut at 0: the bands and top line as in the body, and the light leading edge. */
	const FPaintedIcon& HealthFillCapPicture()
	{
		static const FPaintedIcon Picture = []
		{
			const FVector2D Shift(-HealthCapLeft, -TrackBoxTop);
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(HealthCapWidth, TrackBoxHeight);
			auto Wedge = [&Shift](float Top, float Bottom) { return EndWedge(Top, Bottom, TrackBottom, HealthCapOverlap, Shift); };
			Result.Layers.Add(PaintFill({ Wedge(TrackTop, TrackBottom) }, Color::Health()));
			Result.Layers.Add(PaintFill({ Wedge(TrackTop, HighBandBottom) }, Color::HealthHi()));
			Result.Layers.Add(PaintFill({ Wedge(LowBandTop, TrackBottom) }, Color::HealthLow()));
			Result.Layers.Add(PaintFill({ Wedge(TrackTop, TrackTop + FillLitLine) }, Color::HealthEdge(), 0.45f));
			const float TopCut = TrackHeight * LeanPerPixel;
			Result.Layers.Add(PaintFill({ { FVector2D(TopCut - LeadEdgeWidth, TrackTop) + Shift, FVector2D(TopCut, TrackTop) + Shift,
				FVector2D(0.f, TrackBottom) + Shift, FVector2D(-LeadEdgeWidth, TrackBottom) + Shift } }, Color::HealthEdge()));
			return Result;
		}();
		return Picture;
	}

	/** The track's shape in white (tinted: the chip, the low-health beat), and its end piece. */
	const FPaintedIcon& HealthWhiteBodyPicture()
	{
		static const FPaintedIcon Picture = []
		{
			const FVector2D Shift(-HealthBoxLeft, -TrackBoxTop);
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(HealthBoxWidth, TrackBoxHeight);
			Result.Layers.Add(PaintFill({ LeanBox(TrackLeft, TrackTop, TrackRight, TrackBottom, HealthMid, Shift) }, FLinearColor::White));
			KeepOutOfWindow(Result, Shift);
			return Result;
		}();
		return Picture;
	}

	const FPaintedIcon& HealthWhiteCapPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(HealthCapWidth, TrackBoxHeight);
			Result.Layers.Add(PaintFill({ EndWedge(TrackTop, TrackBottom, TrackBottom, HealthCapOverlap, FVector2D(-HealthCapLeft, -TrackBoxTop)) },
				FLinearColor::White));
			return Result;
		}();
		return Picture;
	}

	/** The experience bar's ink edge round its track, with its soft shadow (the track cut out of both). */
	const FPaintedIcon& XPEdgePicture()
	{
		static const FPaintedIcon Picture = []
		{
			const FVector2D Shift(-XPBoxLeft, -XPBoxTop);
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(XPBoxWidth, XPBoxHeight);
			const FPoints Track = LeanBox(0.f, 0.f, XPLength, XPThickness, XPMid, Shift);
			const float Edge = XPEdgeWidth;
			AddSoftShadow(Result, [&Shift, Edge](float Grow)
			{
				return LeanBox(-Edge - Grow, 1.7f - Edge - Grow, XPLength + Edge + Grow, XPThickness + Edge + 1.7f + Grow, XPMid + 1.7f, Shift);
			}, { Track }, 2.4f);
			Result.Layers.Add(PaintFill({ LeanBox(-Edge, -Edge, XPLength + Edge, XPThickness + Edge, XPMid, Shift), Track }, Color::Ink()));
			return Result;
		}();
		return Picture;
	}

	/** The dark track and the faint sections still to earn: a background. */
	const FPaintedIcon& XPTrackPicture()
	{
		static const FPaintedIcon Picture = []
		{
			const FVector2D Shift(-XPBoxLeft, -XPBoxTop);
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(XPBoxWidth, XPBoxHeight);
			Result.Layers.Add(PaintFill({ LeanBox(0.f, 0.f, XPLength, XPThickness, XPMid, Shift) }, Color::Track(), 0.8f));
			Result.Layers.Add(PaintFill(XPSections(SectionTop, SectionBottom, Shift), Color::Hairline(), 0.17f));
			return Result;
		}();
		return Picture;
	}

	/** The ten sections in two flat cyan halves (the dark one under all, the light one over the top halves). */
	const FPaintedIcon& XPFillBodyPicture()
	{
		static const FPaintedIcon Picture = []
		{
			const FVector2D Shift(-XPBoxLeft, -SectionBoxTop);
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(XPBoxWidth, SectionBoxHeight);
			Result.Layers.Add(PaintFill(XPSections(SectionTop, SectionBottom, Shift), Color::XPDark()));
			Result.Layers.Add(PaintFill(XPSections(SectionTop, XPMid, Shift), Color::XPLight()));
			return Result;
		}();
		return Picture;
	}

	const FPaintedIcon& XPFillCapPicture()
	{
		static const FPaintedIcon Picture = []
		{
			const FVector2D Shift(-XPCapLeft, -SectionBoxTop);
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(XPCapWidth, SectionBoxHeight);
			Result.Layers.Add(PaintFill({ EndWedge(SectionTop, SectionBottom, SectionBottom, XPCapOverlap, Shift) }, Color::XPDark()));
			Result.Layers.Add(PaintFill({ EndWedge(SectionTop, XPMid, SectionBottom, XPCapOverlap, Shift) }, Color::XPLight()));
			return Result;
		}();
		return Picture;
	}

	/** The sections in white (tinted: the just-earned stretch), and their end piece. */
	const FPaintedIcon& XPWhiteBodyPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(XPBoxWidth, SectionBoxHeight);
			Result.Layers.Add(PaintFill(XPSections(SectionTop, SectionBottom, FVector2D(-XPBoxLeft, -SectionBoxTop)), FLinearColor::White));
			return Result;
		}();
		return Picture;
	}

	const FPaintedIcon& XPWhiteCapPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(XPCapWidth, SectionBoxHeight);
			Result.Layers.Add(PaintFill({ EndWedge(SectionTop, SectionBottom, SectionBottom, XPCapOverlap, FVector2D(-XPCapLeft, -SectionBoxTop)) },
				FLinearColor::White));
			return Result;
		}();
		return Picture;
	}

	FSlateBrush BarPictureBrush(FName Name, const FPaintedIcon& Picture)
	{
		return PaintedIconBrush(Name, Picture, BarPixelsPerUnit, Picture.ViewBox);
	}
}

FSlateBrush UHudPlayerFrameWidget::PictureBrush(EHudFramePicture Picture)
{
	const FVector2D MedalSize(MedallionBox * MedalUnit);
	const FVector2D GemSize(GemBox * MedalUnit);
	switch (Picture)
	{
	case EHudFramePicture::MedallionBack: return PaintedIconBrush(TEXT("HudFrameMedalBack"), MedallionBackPicture(), MedalPixelsPerUnit, MedalSize);
	case EHudFramePicture::MedallionFront: return PaintedIconBrush(TEXT("HudFrameMedalFront"), MedallionFrontPicture(), MedalPixelsPerUnit, MedalSize);
	case EHudFramePicture::Gem: return PaintedIconBrush(TEXT("HudFrameGem"), GemPicture(), GemPixelsPerUnit, GemSize);
	case EHudFramePicture::GemFlash: return PaintedIconBrush(TEXT("HudFrameGemFlash"), GemFlashPicture(), GemPixelsPerUnit, GemSize);
	case EHudFramePicture::GemRing: return PaintedIconBrush(TEXT("HudFrameGemRing"), GemRingPicture(), RingPixelsPerUnit, GemSize);
	case EHudFramePicture::HealthBack: return BarPictureBrush(TEXT("HudFrameHealthBack"), HealthBackPicture());
	case EHudFramePicture::HealthTrack: return BarPictureBrush(TEXT("HudFrameHealthTrack"), HealthTrackPicture());
	case EHudFramePicture::HealthFront: return BarPictureBrush(TEXT("HudFrameHealthFront"), HealthFrontPicture());
	case EHudFramePicture::HealthShine: return GlowBrush(FVector2D(ShineLength, TrackHeight), ShineColor());
	case EHudFramePicture::HealthFillBody: return BarPictureBrush(TEXT("HudFrameHealthFill"), HealthFillBodyPicture());
	case EHudFramePicture::HealthFillCap: return BarPictureBrush(TEXT("HudFrameHealthFillEnd"), HealthFillCapPicture());
	case EHudFramePicture::HealthWhiteBody: return BarPictureBrush(TEXT("HudFrameHealthWhite"), HealthWhiteBodyPicture());
	case EHudFramePicture::HealthWhiteCap: return BarPictureBrush(TEXT("HudFrameHealthWhiteEnd"), HealthWhiteCapPicture());
	case EHudFramePicture::XPEdge: return BarPictureBrush(TEXT("HudFrameXPEdge"), XPEdgePicture());
	case EHudFramePicture::XPTrack: return BarPictureBrush(TEXT("HudFrameXPTrack"), XPTrackPicture());
	case EHudFramePicture::XPFillBody: return BarPictureBrush(TEXT("HudFrameXPFill"), XPFillBodyPicture());
	case EHudFramePicture::XPFillCap: return BarPictureBrush(TEXT("HudFrameXPFillEnd"), XPFillCapPicture());
	case EHudFramePicture::XPWhiteBody: return BarPictureBrush(TEXT("HudFrameXPWhite"), XPWhiteBodyPicture());
	case EHudFramePicture::XPWhiteCap: return BarPictureBrush(TEXT("HudFrameXPWhiteEnd"), XPWhiteCapPicture());
	default: return FSlateBrush();
	}
}

FVector2D UHudPlayerFrameWidget::PicturePosition(EHudFramePicture Picture)
{
	const FVector2D Health = InFrame(HealthBarX, HealthBarY);
	const FVector2D XP = InFrame(XPBarX, XPBarY);
	switch (Picture)
	{
	case EHudFramePicture::MedallionBack:
	case EHudFramePicture::MedallionFront: return InFrame(MedallionX, MedallionY) - FVector2D(MedallionBox * MedalUnit * 0.5f);
	case EHudFramePicture::Gem:
	case EHudFramePicture::GemFlash:
	case EHudFramePicture::GemRing: return InFrame(GemX, GemY) - FVector2D(GemBox * MedalUnit * 0.5f);
	case EHudFramePicture::HealthBack:
	case EHudFramePicture::HealthFront: return Health + FVector2D(HealthBoxLeft, HealthBoxTop);
	case EHudFramePicture::HealthTrack:
	case EHudFramePicture::HealthFillBody:
	case EHudFramePicture::HealthWhiteBody: return Health + FVector2D(HealthBoxLeft, TrackBoxTop);
	// An end piece sits where it would with its cut at the bar's 0; PaintStretch moves it along.
	case EHudFramePicture::HealthFillCap:
	case EHudFramePicture::HealthWhiteCap: return Health + FVector2D(HealthCapLeft, TrackBoxTop);
	case EHudFramePicture::HealthShine: return Health + FVector2D(0.f, TrackTop);
	case EHudFramePicture::XPEdge:
	case EHudFramePicture::XPTrack: return XP + FVector2D(XPBoxLeft, XPBoxTop);
	case EHudFramePicture::XPFillBody:
	case EHudFramePicture::XPWhiteBody: return XP + FVector2D(XPBoxLeft, SectionBoxTop);
	case EHudFramePicture::XPFillCap:
	case EHudFramePicture::XPWhiteCap: return XP + FVector2D(XPCapLeft, SectionBoxTop);
	default: return FVector2D::ZeroVector;
	}
}

// UHudShieldBarWidget's pictures, each drawn once into a shared texture, and where each sits in the widget. Drawn as the
// frame's bars are (HudPlayerFrameShapes' helpers, the same 16 degree lean), smaller and in the kit's cyan; nothing of the
// bar may show through the portrait's glass, so every picture is cut along the medallion's window.

#include "UI/HUD/HudShieldBarWidget.h"
#include "UI/HUD/HudPlayerFrameShapes.h"
#include "UI/HUD/HudPlayerFrameWidget.h"
#include "UI/HUD/HudPortraitWidget.h"
#include "UI/Style/LooterUIStyle.h"

using namespace LooterUI;
using namespace HudFrameShapes;

namespace
{
	using W = UHudShieldBarWidget;

	/** The fill's light top band (its top 45%), its lit top line and its light leading edge. */
	constexpr float HighBandBottom = W::TrackTop + W::TrackHeight * 0.45f;
	constexpr float LitLine = 0.8f;
	constexpr float LeadEdge = 1.4f;
	/** A sliver: a small leaning chip of the ward. */
	constexpr float ShardLength = 7.f;
	constexpr float ShardHeight = 3.f;
	constexpr float ShardBox = 12.f;

	/** The pictures' box's top-left in bar pixels moved to the picture's own origin. */
	const FVector2D BoxShift(-W::BoxLeft, -W::BoxTop);
	const FVector2D CapShift(-W::CapLeft, -W::BoxTop);

	/**
	 * Cuts the portrait's window out of a picture whose top-left sits at ScreenTopLeft on the 1080p screen: the window's
	 * edge there is the line x - y = const, so keeping the side past it (half a pixel out) is enough; the bezel covers the
	 * cut (HudFrameShapes::KeepOutOfWindow's rule, for this bar's place).
	 */
	void KeepOffWindow(FPaintedIcon& Picture, const FVector2D& ScreenTopLeft)
	{
		const float ScreenLine = UHudPlayerFrameWidget::MedallionX - UHudPlayerFrameWidget::MedallionY + UHudPortraitWidget::WindowSize * 0.5f + 0.5f;
		const float Line = ScreenLine - static_cast<float>(ScreenTopLeft.X - ScreenTopLeft.Y);
		const FPoints PastWindow = { FVector2D(Line - 1000.f, -1000.f), FVector2D(5000.f, -1000.f), FVector2D(5000.f, 1000.f),
			FVector2D(Line + 1000.f, 1000.f) };
		for (FPaintLayer& Layer : Picture.Layers)
		{
			TArray<FPoints> Kept;
			for (const FPoints& Shape : Layer.Fills)
			{
				FPoints Cut = ClipToConvex(Shape, PastWindow);
				if (Cut.Num() >= 3)
				{
					Kept.Add(MoveTemp(Cut));
				}
			}
			Layer.Fills = MoveTemp(Kept);
		}
	}

	FVector2D BoxScreen()
	{
		return FVector2D(W::BarX + W::BoxLeft, W::BarY + W::BoxTop);
	}

	FPoints TrackShape(const FVector2D& Shift)
	{
		return LeanBox(W::TrackLeft, W::TrackTop, W::TrackRight, W::TrackBottom, W::Mid, Shift);
	}

	const FPaintedIcon& BackPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(W::BoxWidth, W::BoxHeight);
			const FPoints Track = TrackShape(BoxShift);
			// A thin ink rim, the gunmetal bezel lit from the top inside it, the track cut out (even-odd with the outline).
			Result.Layers.Add(PaintFill({ LeanBox(0.f, 0.f, W::Length, W::Thickness, W::Mid, BoxShift), Track }, Color::Ink()));
			const float Right = W::Length - W::Rim;
			const float Bottom = W::Thickness - W::Rim;
			Result.Layers.Add(PaintGraded(PaintFill({ LeanBox(W::Rim, W::Rim, Right, Bottom, W::Mid, BoxShift), Track }, Color::BarMetalHi()),
				Color::BarMetalLow(), FVector2D(0.f, W::Rim) + BoxShift, FVector2D(0.f, Bottom) + BoxShift));
			Result.Layers.Add(PaintFill({ LeanBox(W::Rim, W::Rim, Right, W::Rim + 0.7f, W::Mid, BoxShift) }, FLinearColor::White, 0.3f));
			// The cyan hairline the kit puts inside a bezel, along the track's top edge.
			Result.Layers.Add(PaintFill({ LeanBox(W::TrackLeft, W::TrackTop - 0.6f, W::TrackRight, W::TrackTop, W::Mid, BoxShift) }, Color::Hairline(), 0.55f));
			KeepOffWindow(Result, BoxScreen());
			return Result;
		}();
		return Picture;
	}

	const FPaintedIcon& TrackPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(W::BoxWidth, W::BoxHeight);
			Result.Layers.Add(PaintFill({ TrackShape(BoxShift) }, Color::Track(), 0.88f));
			KeepOffWindow(Result, BoxScreen());
			return Result;
		}();
		return Picture;
	}

	const FPaintedIcon& FillBodyPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(W::BoxWidth, W::BoxHeight);
			// The kit's cyan in two halves (the experience sections' own), the top band light, a lit line along the top and,
			// at the far end, the light leading edge a full ward shows.
			Result.Layers.Add(PaintFill({ TrackShape(BoxShift) }, Color::XPDark()));
			Result.Layers.Add(PaintFill({ LeanBox(W::TrackLeft, W::TrackTop, W::TrackRight, HighBandBottom, W::Mid, BoxShift) }, Color::XPLight()));
			Result.Layers.Add(PaintFill({ LeanBox(W::TrackLeft, W::TrackTop, W::TrackRight, W::TrackTop + LitLine, W::Mid, BoxShift) }, Color::GemLight(), 0.6f));
			Result.Layers.Add(PaintFill({ LeanBox(W::TrackRight - LeadEdge, W::TrackTop, W::TrackRight, W::TrackBottom, W::Mid, BoxShift) }, Color::GemLight()));
			KeepOffWindow(Result, BoxScreen());
			return Result;
		}();
		return Picture;
	}

	const FPaintedIcon& FillCapPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(W::CapWidth, W::BoxHeight);
			auto Wedge = [](float Top, float Bottom) { return EndWedge(Top, Bottom, W::TrackBottom, W::CapOverlap, CapShift); };
			Result.Layers.Add(PaintFill({ Wedge(W::TrackTop, W::TrackBottom) }, Color::XPDark()));
			Result.Layers.Add(PaintFill({ Wedge(W::TrackTop, HighBandBottom) }, Color::XPLight()));
			Result.Layers.Add(PaintFill({ Wedge(W::TrackTop, W::TrackTop + LitLine) }, Color::GemLight(), 0.6f));
			// Its light leading edge along the cut.
			auto CutAt = [](float Y) { return (W::TrackBottom - Y) * LeanPerPixel; };
			Result.Layers.Add(PaintFill({ { FVector2D(CutAt(W::TrackTop) - LeadEdge, W::TrackTop) + CapShift, FVector2D(CutAt(W::TrackTop), W::TrackTop) + CapShift,
				FVector2D(0.f, W::TrackBottom) + CapShift, FVector2D(-LeadEdge, W::TrackBottom) + CapShift } }, Color::GemLight()));
			return Result;
		}();
		return Picture;
	}

	const FPaintedIcon& WhiteBodyPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(W::BoxWidth, W::BoxHeight);
			Result.Layers.Add(PaintFill({ TrackShape(BoxShift) }, FLinearColor::White));
			KeepOffWindow(Result, BoxScreen());
			return Result;
		}();
		return Picture;
	}

	const FPaintedIcon& WhiteCapPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(W::CapWidth, W::BoxHeight);
			Result.Layers.Add(PaintFill({ EndWedge(W::TrackTop, W::TrackBottom, W::TrackBottom, W::CapOverlap, CapShift) }, FLinearColor::White));
			return Result;
		}();
		return Picture;
	}

	const FPaintedIcon& ShardPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(ShardBox, ShardBox);
			const FVector2D Centre((ShardBox - ShardLength) * 0.5f, (ShardBox - ShardHeight) * 0.5f);
			// Lit white with a thin dark edge, so it reads over a bright sky: tinted cyan as it flies.
			Result.Layers.Add(PaintFill({ LeanBox(-0.6f, -0.6f, ShardLength + 0.6f, ShardHeight + 0.6f, ShardHeight * 0.5f, Centre) }, Color::Ink(), 0.7f));
			Result.Layers.Add(PaintFill({ LeanBox(0.f, 0.f, ShardLength, ShardHeight, ShardHeight * 0.5f, Centre) }, FLinearColor::White));
			return Result;
		}();
		return Picture;
	}

	FSlateBrush BarBrush(FName Name, const FPaintedIcon& Picture)
	{
		return PaintedIconBrush(Name, Picture, BarPixelsPerUnit, Picture.ViewBox);
	}
}

FSlateBrush UHudShieldBarWidget::PictureBrush(EHudShieldPicture Picture)
{
	switch (Picture)
	{
	case EHudShieldPicture::Back: return BarBrush(TEXT("HudShieldBack"), BackPicture());
	case EHudShieldPicture::Track: return BarBrush(TEXT("HudShieldTrack"), TrackPicture());
	case EHudShieldPicture::FillBody: return BarBrush(TEXT("HudShieldFill"), FillBodyPicture());
	case EHudShieldPicture::FillCap: return BarBrush(TEXT("HudShieldFillEnd"), FillCapPicture());
	case EHudShieldPicture::WhiteBody: return BarBrush(TEXT("HudShieldWhite"), WhiteBodyPicture());
	case EHudShieldPicture::WhiteCap: return BarBrush(TEXT("HudShieldWhiteEnd"), WhiteCapPicture());
	case EHudShieldPicture::Shard: return PaintedIconBrush(TEXT("HudShieldShard"), ShardPicture(), 3.f, FVector2D(ShardBox, ShardBox));
	default: return FSlateBrush();
	}
}

FVector2D UHudShieldBarWidget::PicturePosition(EHudShieldPicture Picture)
{
	switch (Picture)
	{
	// An end piece sits where it would with its cut at the bar's 0; PaintStretch moves it along.
	case EHudShieldPicture::FillCap:
	case EHudShieldPicture::WhiteCap: return FVector2D(CapLeft - BoxLeft, 0.f);
	case EHudShieldPicture::Shard: return FVector2D(-ShardBox * 0.5f - BoxLeft, Mid - ShardBox * 0.5f - BoxTop);
	default: return FVector2D::ZeroVector;
	}
}

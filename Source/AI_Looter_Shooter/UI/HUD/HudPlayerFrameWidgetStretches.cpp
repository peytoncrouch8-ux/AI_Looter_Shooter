// UHudPlayerFrameWidget's bars as slanted stretches: each shows its bar from the start up to a level as its picture
// cropped just short of the level plus an end piece moved along to finish it with the HUD's lean, so the leading edge
// leans like the bar's ends and stays smooth (only edges inside a texture are anti-aliased), and the heal's shine.

#include "UI/HUD/HudPlayerFrameWidget.h"
#include "UI/HUD/HudPlayerFrameShapes.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"

using namespace LooterUI;
using namespace HudFrameShapes;

namespace
{
	/**
	 * A stretch shorter than this into its track or section shows as a little more (health: anything left must show) or as
	 * none of that section (experience): its end piece would otherwise poke out past the slanted start.
	 */
	constexpr float HealthMinReach = TrackHalfLean * 2.f + 1.5f;
	constexpr float SectionMinReach = SectionHalfLean * 2.f + 1.5f;
	/** A health stretch this full shows the whole bar, its own far end and no end piece. */
	constexpr float HealthFull = 0.9995f;

	/** A stretch's drawn end: nothing, the whole bar, or not drawn yet. */
	constexpr float CutHidden = -1000.f;
	constexpr float CutFull = 100000.f;
	constexpr float CutUnpainted = -2000.f;

	bool IsHealthStretch(EHudFrameStretch Stretch)
	{
		return Stretch == EHudFrameStretch::HealthChip || Stretch == EHudFrameStretch::HealthFill || Stretch == EHudFrameStretch::HealthBeat;
	}

	/** A stretch's body and end piece. */
	TPair<EHudFramePicture, EHudFramePicture> StretchPictures(EHudFrameStretch Stretch)
	{
		switch (Stretch)
		{
		case EHudFrameStretch::HealthFill: return { EHudFramePicture::HealthFillBody, EHudFramePicture::HealthFillCap };
		case EHudFrameStretch::HealthChip:
		case EHudFrameStretch::HealthBeat: return { EHudFramePicture::HealthWhiteBody, EHudFramePicture::HealthWhiteCap };
		case EHudFrameStretch::XPFill:
		case EHudFrameStretch::XPBefore: return { EHudFramePicture::XPFillBody, EHudFramePicture::XPFillCap };
		default: return { EHudFramePicture::XPWhiteBody, EHudFramePicture::XPWhiteCap };
		}
	}

	/** The middle of a health stretch's leaning end, in the bar's pixels; anything left shows at least a sliver. */
	float HealthLevel(float Amount)
	{
		return FMath::Max(TrackLeft + Amount * (TrackRight - TrackLeft), TrackLeft + HealthMinReach);
	}

	/**
	 * The middle of an experience stretch's leaning end, or below 0 for none. Each section is a tenth of the level; one the
	 * stretch has barely entered shows only up to the end of the one before.
	 */
	float XPLevel(float Amount)
	{
		const float Tenths = Amount * SectionCount;
		int32 Section = FMath::Clamp(FMath::FloorToInt32(Tenths), 0, SectionCount - 1);
		float Into = FMath::Min((Tenths - Section) * SectionLength, SectionLength);
		if (Into < SectionMinReach)
		{
			if (Section == 0)
			{
				return -1.f;
			}
			--Section;
			Into = SectionLength;
		}
		return SectionStart(Section) + Into;
	}
}

void UHudPlayerFrameWidget::AddStretch(UCanvasPanel* Canvas, EHudFrameStretch Stretch, const FLinearColor& Tint)
{
	const int32 Count = static_cast<int32>(EHudFrameStretch::Count);
	if (StretchBodies.Num() != Count)
	{
		StretchBodies.SetNum(Count);
		StretchCaps.SetNum(Count);
		StretchBrushes.SetNum(Count);
		StretchShown.Init(CutUnpainted, Count);
	}
	const int32 Index = static_cast<int32>(Stretch);
	const TPair<EHudFramePicture, EHudFramePicture> Pictures = StretchPictures(Stretch);
	auto AddLayer = [this, Canvas, &Tint](EHudFramePicture Picture, const FSlateBrush& Brush)
	{
		UImage* Image = MakeImage(WidgetTree, Brush);
		Image->SetColorAndOpacity(Tint);
		Image->SetVisibility(ESlateVisibility::Hidden);
		UCanvasPanelSlot* ImageSlot = Canvas->AddChildToCanvas(Image);
		// Sized by its brush, so a cropped body stays anchored at its start.
		ImageSlot->SetAutoSize(true);
		ImageSlot->SetPosition(PicturePosition(Picture));
		return Image;
	};
	StretchBrushes[Index] = PictureBrush(Pictures.Key);
	StretchBodies[Index] = AddLayer(Pictures.Key, StretchBrushes[Index]);
	StretchCaps[Index] = AddLayer(Pictures.Value, PictureBrush(Pictures.Value));
	StretchShown[Index] = CutUnpainted;
}

void UHudPlayerFrameWidget::TintStretch(EHudFrameStretch Stretch, const FLinearColor& Tint)
{
	const int32 Index = static_cast<int32>(Stretch);
	if (StretchBodies.IsValidIndex(Index) && StretchCaps.IsValidIndex(Index) && StretchBodies[Index] && StretchCaps[Index])
	{
		StretchBodies[Index]->SetColorAndOpacity(Tint);
		StretchCaps[Index]->SetColorAndOpacity(Tint);
	}
}

void UHudPlayerFrameWidget::PaintStretch(EHudFrameStretch Stretch, float Amount)
{
	const int32 Index = static_cast<int32>(Stretch);
	if (!StretchBodies.IsValidIndex(Index) || !StretchCaps.IsValidIndex(Index) || !StretchShown.IsValidIndex(Index)
		|| !StretchBrushes.IsValidIndex(Index))
	{
		return;
	}
	UImage* Body = StretchBodies[Index];
	UImage* Cap = StretchCaps[Index];
	if (!Body || !Cap)
	{
		return;
	}

	// The cut: just short of the level, by half the lean, so the end piece's slant passes through the level at the
	// stretch's middle height. Whole pixels, so the body's edge and the end piece's meet on a pixel's edge.
	const bool bHealth = IsHealthStretch(Stretch);
	float Cut = CutHidden;
	if (Amount >= (bHealth ? HealthFull : 1.f))
	{
		Cut = CutFull;
	}
	else if (Amount > 0.f)
	{
		const float Level = bHealth ? HealthLevel(Amount) : XPLevel(Amount);
		if (Level >= 0.f)
		{
			Cut = FMath::RoundToFloat(Level - (bHealth ? TrackHalfLean : SectionHalfLean));
		}
	}
	if (Cut == StretchShown[Index])
	{
		return;
	}
	StretchShown[Index] = Cut;

	if (Cut == CutHidden)
	{
		Body->SetVisibility(ESlateVisibility::Hidden);
		Cap->SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	const FSlateBrush& Whole = StretchBrushes[Index];
	if (Cut == CutFull)
	{
		// The whole picture: the bar's own far end, smooth in the texture, with no end piece over it.
		Body->SetBrush(Whole);
		Body->SetVisibility(ESlateVisibility::HitTestInvisible);
		Cap->SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	// Cropped, not squeezed: the body shows its picture from its start up to the cut, so nothing in it stretches.
	const float WholeLength = static_cast<float>(Whole.ImageSize.X);
	const float Length = FMath::Clamp(Cut - (bHealth ? HealthBoxLeft : XPBoxLeft), 1.f, WholeLength);
	FSlateBrush Cropped = Whole;
	Cropped.ImageSize = FVector2D(Length, Whole.ImageSize.Y);
	Cropped.SetUVRegion(FBox2f(FVector2f(0.f, 0.f), FVector2f(Length / WholeLength, 1.f)));
	Body->SetBrush(Cropped);
	Body->SetVisibility(ESlateVisibility::HitTestInvisible);
	Cap->SetRenderTranslation(FVector2D(Cut, 0.f));
	Cap->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UHudPlayerFrameWidget::PaintShine(float Progress, float Amount)
{
	if (!Shine)
	{
		return;
	}
	// The fill's reach along the track: from where its start is fully inside the track to its cut (or the far end).
	const float From = TrackLeft + TrackHalfLean;
	const float To = Amount >= HealthFull ? TrackRight + TrackHalfLean : HealthLevel(Amount) - TrackHalfLean;
	float Left = 0.f;
	float VisibleLeft = 0.f;
	float VisibleRight = 0.f;
	if (Progress < 1.f && Amount > 0.f && To > From)
	{
		// It sweeps in from before the fill's start and out past its end, quick at first.
		Left = FMath::Lerp(From - ShineLength, To, FMath::InterpEaseOut(0.f, 1.f, FMath::Clamp(Progress, 0.f, 1.f), 2.f));
		VisibleLeft = FMath::Max(Left, From);
		VisibleRight = FMath::Min(Left + ShineLength, To);
	}
	if (VisibleRight - VisibleLeft < 0.5f)
	{
		if (bShineShown)
		{
			Shine->SetVisibility(ESlateVisibility::Hidden);
			bShineShown = false;
		}
		return;
	}
	// Cut off at the fill's ends rather than clipped: it's soft at both ends, so a straight cut doesn't show.
	FSlateBrush Cropped = ShineBrush;
	Cropped.ImageSize = FVector2D(VisibleRight - VisibleLeft, ShineBrush.ImageSize.Y);
	Cropped.SetUVRegion(FBox2f(FVector2f((VisibleLeft - Left) / ShineLength, 0.f), FVector2f((VisibleRight - Left) / ShineLength, 1.f)));
	Shine->SetBrush(Cropped);
	Shine->SetRenderTranslation(FVector2D(VisibleLeft, 0.f));
	if (!bShineShown)
	{
		Shine->SetVisibility(ESlateVisibility::HitTestInvisible);
		bShineShown = true;
	}
}

#include "UI/HUD/HudNotchTallyWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Weapons/WeaponNotches.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

using namespace LooterUI;

namespace
{
	/** The mark's box (the cuts and slash inside it, with room for their ink), and the count's type. */
	constexpr float TallyWidth = 18.f;
	constexpr float TallyHeight = 16.f;
	constexpr float TallyPixelsPerUnit = 2.f;
	constexpr int32 TallyCountSize = 15;
	constexpr int32 TallyCountSpacing = 60;
	/** The count stands this far after the mark. */
	constexpr float TallyCountGap = 4.f;
	/** The pop: the count from this big, the mark a little less. */
	constexpr float TallyPopFrom = 1.55f;
	constexpr float TallyMarkPopShare = 0.5f;

	/**
	 * The notches' tally (the kit's TallyBrush, four cuts and a slash) drawn inked for the HUD: each stroke white over a
	 * wider ink one, so tinting it colours the white and leaves the ink dark, and it reads over sky and snow alike.
	 */
	const FPaintedIcon& InkedTallyPicture()
	{
		static const FPaintedIcon Picture = []
		{
			TArray<TArray<FVector2D>> Strokes;
			for (const float X : { 3.5f, 7.f, 10.5f, 14.f })
			{
				Strokes.Add({ FVector2D(X, 3.f), FVector2D(X, 13.f) });
			}
			Strokes.Add({ FVector2D(1.5f, 11.5f), FVector2D(16.5f, 4.5f) });
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(TallyWidth, TallyHeight);
			FPaintLayer Ink;
			Ink.Strokes = Strokes;
			Ink.StrokeWidth = 3.4f;
			Ink.Color = Color::Ink();
			Result.Layers.Add(Ink);
			FPaintLayer Cuts;
			Cuts.Strokes = Strokes;
			Cuts.StrokeWidth = 1.6f;
			Cuts.Color = FLinearColor::White;
			Result.Layers.Add(Cuts);
			return Result;
		}();
		return Picture;
	}

	/** Out fast, overshooting a touch, settling (a pop's ease). */
	float TallyEaseOutBack(float Share)
	{
		const float T = FMath::Clamp(Share, 0.f, 1.f) - 1.f;
		constexpr float Overshoot = 1.7f;
		return 1.f + T * T * ((Overshoot + 1.f) * T + Overshoot);
	}
}

TSharedRef<SWidget> UHudNotchTallyWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Row->SetVisibility(ESlateVisibility::HitTestInvisible);
		TallyMark = MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudNotchTally"), InkedTallyPicture(), TallyPixelsPerUnit,
			FVector2D(TallyWidth, TallyHeight)));
		TallyMark->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Row->AddChildToHorizontalBox(TallyMark)->SetVerticalAlignment(VAlign_Center);
		CountWords = MakeFloatingText(WidgetTree, TallyCountSize, Color::Text(), TallyCountSpacing);
		// It pops from its left end, so it grows away from the gem after it rather than over it.
		CountWords->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
		UHorizontalBoxSlot* CountSlot = Row->AddChildToHorizontalBox(CountWords);
		CountSlot->SetVerticalAlignment(VAlign_Center);
		CountSlot->SetPadding(FMargin(TallyCountGap, 0.f, 0.f, 0.f));
		WidgetTree->RootWidget = Row;
		Apply();
	}
	return Super::RebuildWidget();
}

FString UHudNotchTallyWidget::CountText(int32 Kills)
{
	return Kills > 0 ? FText::AsNumber(Kills).ToString() : FString();
}

FLinearColor UHudNotchTallyWidget::TallyColor(int32 Kills)
{
	switch (WeaponNotches::TierFor(Kills))
	{
	case ENotchTier::Blooded: return Color::Accent();
	case ENotchTier::Named: return Color::AccentLight();
	case ENotchTier::SoulForged: return Color::CyanText();
	default: return Color::TextDim();
	}
}

float UHudNotchTallyWidget::PopScaleAt(float Age)
{
	if (Age < 0.f || Age >= PopSeconds)
	{
		return 1.f;
	}
	return FMath::Lerp(TallyPopFrom, 1.f, TallyEaseOutBack(Age / PopSeconds));
}

bool UHudNotchTallyWidget::Update(int32 Kills, bool bNewGun, float DeltaSeconds)
{
	const int32 Shown = FMath::Max(Kills, 0);
	bool bRose = false;
	bool bChanged = false;
	if (Shown != ShownKills)
	{
		// A kill with the gun in hand pops; the first sight of a gun (or one just drawn) shows its count as it is.
		bRose = !bNewGun && ShownKills != INDEX_NONE && Shown > ShownKills;
		ShownKills = Shown;
		PopAge = bRose ? 0.f : PopSeconds;
		bChanged = true;
	}
	else if (PopAge < PopSeconds)
	{
		PopAge = FMath::Min(PopAge + DeltaSeconds, PopSeconds);
		bChanged = true;
	}
	if (bChanged)
	{
		Apply();
	}
	return bRose;
}

void UHudNotchTallyWidget::Apply()
{
	const bool bShow = ShownKills > 0;
	const ESlateVisibility Wanted = bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (GetVisibility() != Wanted)
	{
		SetVisibility(Wanted);
	}
	if (!TallyMark || !CountWords || !bShow)
	{
		return;
	}
	CountWords->SetText(FText::FromString(CountText(ShownKills)));
	// The pop: the count swells and settles, flashing white into its own colour; the mark swells a little less and
	// flashes into its tier's colour.
	const float Share = FMath::Clamp(PopAge / PopSeconds, 0.f, 1.f);
	const float Scale = PopScaleAt(PopAge);
	CountWords->SetRenderScale(FVector2D(Scale));
	CountWords->SetColorAndOpacity(FSlateColor(FMath::Lerp(FLinearColor::White, Color::Text(), Share)));
	TallyMark->SetRenderScale(FVector2D(1.f + (Scale - 1.f) * TallyMarkPopShare));
	TallyMark->SetColorAndOpacity(FMath::Lerp(FLinearColor::White, TallyColor(ShownKills), Share));
}

bool UHudNotchTallyWidget::IsShowing() const
{
	return GetVisibility() != ESlateVisibility::Collapsed && ShownKills > 0;
}

FString UHudNotchTallyWidget::GetShownText() const
{
	return CountWords ? CountWords->GetText().ToString() : FString();
}

float UHudNotchTallyWidget::GetCountScale() const
{
	return CountWords ? static_cast<float>(CountWords->GetRenderTransform().Scale.X) : 1.f;
}

#include "UI/HUD/HudGrenadeWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/InkedIconData.inl"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

using namespace LooterUI;

namespace
{
	/** The row: the key tab, then the icon, then the count over its pips, each this far apart. */
	constexpr float TabGap = 8.f;
	constexpr float IconGap = 5.f;
	/** A pip: a slim slanted bar like the HUD's others, this big, this far apart, sheared this many degrees. */
	constexpr float PipWidth = 7.f;
	constexpr float PipHeight = 5.f;
	constexpr float PipGap = 3.f;
	constexpr float PipSlant = 16.f;
	/** The count's line sits closer to its pips than its font's line height. */
	constexpr float PipTuck = -4.f;
	/** The count's dark outline, as the cartridge's counts have it. */
	constexpr int32 CountOutline = 2;

	/** How long each flash runs, how far it pops, and how hard a refusal shakes (px). */
	constexpr float GainedSeconds = 0.5f;
	constexpr float ThrownSeconds = 0.3f;
	constexpr float DeniedSeconds = 0.4f;
	constexpr float GainedPop = 0.35f;
	constexpr float ThrownPop = 0.15f;
	constexpr float DeniedShake = 4.f;

	/** None left: the icon dimmed (grey dims an Inked icon). */
	const FLinearColor EmptyIcon(0.5f, 0.5f, 0.5f, 0.8f);

	FLinearColor WithAlpha(FLinearColor Tone, float Alpha)
	{
		Tone.A = Alpha;
		return Tone;
	}

	/** The key tab: a small plate with its top-left and bottom-right corners cut, as the slots' tabs are. */
	TArray<FVector2D> TabOutline()
	{
		const float W = UHudGrenadeWidget::TabWidth;
		const float H = UHudGrenadeWidget::TabHeight;
		return { FVector2D(3.f, 0.f), FVector2D(W, 0.f), FVector2D(W, H - 3.f), FVector2D(W - 3.f, H), FVector2D(0.f, H), FVector2D(0.f, 3.f) };
	}

	const FVectorIcon& TabPlateIcon()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Plate;
			Plate.ViewBox = FVector2D(UHudGrenadeWidget::TabWidth, UHudGrenadeWidget::TabHeight);
			Plate.Fills.Add(TabOutline());
			return Plate;
		}();
		return Icon;
	}

	const FVectorIcon& TabEdgeIcon()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Edge;
			Edge.ViewBox = FVector2D(UHudGrenadeWidget::TabWidth, UHudGrenadeWidget::TabHeight);
			Edge.StrokeWidth = 1.f;
			TArray<FVector2D> Line = TabOutline();
			// Pulled in half the line's width, so the stroke stays inside the picture.
			for (FVector2D& Point : Line)
			{
				Point = FVector2D(FMath::Clamp(Point.X, 0.5f, UHudGrenadeWidget::TabWidth - 0.5f), FMath::Clamp(Point.Y, 0.5f, UHudGrenadeWidget::TabHeight - 0.5f));
			}
			const FVector2D First = Line[0];
			Line.Add(First);
			Edge.Strokes.Add(Line);
			return Edge;
		}();
		return Icon;
	}

	UImage* AddLayer(UWidgetTree* Tree, UOverlay* Overlay, const FSlateBrush& Brush)
	{
		UImage* Image = MakeImage(Tree, Brush);
		UOverlaySlot* LayerSlot = Overlay->AddChildToOverlay(Image);
		LayerSlot->SetHorizontalAlignment(HAlign_Center);
		LayerSlot->SetVerticalAlignment(VAlign_Center);
		return Image;
	}
}

const FInkedIcon& UHudGrenadeWidget::Icon()
{
	// The salt grenade as an Inked icon, in the ammo icons' box and ink line: a squat stoppered tin with a grave cross on
	// its label and the waxed fuse curling up out of the cork to a spark (Art/Icons/InkedIcons.py makes it).
	static const FInkedIcon Tin = InkedIconData::GraveSaltGrenade();
	return Tin;
}

TSharedRef<SWidget> UHudGrenadeWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Line->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Line;
		Row = Line;

		// The key tab: dark glass (a background), its cyan edge, and the bound key.
		UOverlay* Tab = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		const FVector2D TabSize(TabWidth, TabHeight);
		UImage* Glass = AddLayer(WidgetTree, Tab, IconBrush(TEXT("HudGrenadeTabPlate"), TabPlateIcon(), 2.f, TabSize, WithAlpha(Color::ScreenBg(), 0.85f)));
		MarkBackground(Glass);
		AddLayer(WidgetTree, Tab, IconBrush(TEXT("HudGrenadeTabEdge"), TabEdgeIcon(), 2.f, TabSize, WithAlpha(Color::Hairline(), 0.7f)));
		KeyText = MakeFloatingText(WidgetTree, 12, Color::Text(), 0, ETextJustify::Center);
		UOverlaySlot* KeySlot = Tab->AddChildToOverlay(KeyText);
		KeySlot->SetHorizontalAlignment(HAlign_Center);
		KeySlot->SetVerticalAlignment(VAlign_Center);
		// Chakra Petch's digits and capitals sit low in its line: lifted to the plate's middle, as the slots' numbers are.
		KeyText->SetRenderTranslation(FVector2D(0.f, -3.f));
		UHorizontalBoxSlot* TabSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Tab, TabWidth, TabHeight));
		TabSlot->SetVerticalAlignment(VAlign_Center);
		TabSlot->SetPadding(FMargin(0.f, 0.f, TabGap, 0.f));

		// The tin's icon, its own ink line carrying it over any background.
		IconImage = MakeImage(WidgetTree, InkedIconBrush(TEXT("GraveSaltGrenade"), Icon(), FVector2D(IconSize, IconSize)));
		UHorizontalBoxSlot* IconSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, IconImage, IconSize, IconSize));
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(0.f, 0.f, IconGap, 0.f));

		// The count over its pips, right-aligned at the column's edge.
		UVerticalBox* Counts = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		CountText = MakeFloatingText(WidgetTree, CountSize, Color::Text(), 0, ETextJustify::Right);
		FSlateFontInfo CountFont = FloatingFont(CountSize);
		CountFont.OutlineSettings.OutlineSize = CountOutline;
		CountText->SetFont(CountFont);
		// It pops about its own middle.
		CountText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Counts->AddChildToVerticalBox(CountText)->SetHorizontalAlignment(HAlign_Right);
		UHorizontalBox* PipRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Pips.Reset();
		for (int32 Index = 0; Index < MaxPips; ++Index)
		{
			UImage* Pip = MakeImage(WidgetTree, RectBrush(FLinearColor::White, Color::Ink(), 1.f));
			USizeBox* PipBox = MakeSized(WidgetTree, Pip, PipWidth, PipHeight);
			PipBox->SetRenderShear(FVector2D(-PipSlant, 0.f));
			UHorizontalBoxSlot* PipSlot = PipRow->AddChildToHorizontalBox(PipBox);
			PipSlot->SetPadding(FMargin(Index > 0 ? PipGap : 0.f, 0.f, 0.f, 0.f));
			Pip->SetVisibility(ESlateVisibility::Collapsed);
			PipBox->SetVisibility(ESlateVisibility::Collapsed);
			Pips.Add(Pip);
		}
		UVerticalBoxSlot* PipsSlot = Counts->AddChildToVerticalBox(PipRow);
		PipsSlot->SetHorizontalAlignment(HAlign_Right);
		PipsSlot->SetPadding(FMargin(0.f, PipTuck, 1.f, 0.f));
		Line->AddChildToHorizontalBox(Counts)->SetVerticalAlignment(VAlign_Center);

		Line->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Line->SetVisibility(ESlateVisibility::Collapsed);
		ShownCount = INDEX_NONE;
		ShownMax = INDEX_NONE;
		ShownKey.Reset();
		bShown = false;
		FlashLeft = 0.f;
	}
	return Super::RebuildWidget();
}

void UHudGrenadeWidget::Update(int32 Count, int32 MaxCount, bool bUnlocked, const FString& KeyName, float DeltaTime)
{
	if (!Row || !CountText || !KeyText || !IconImage)
	{
		return;
	}
	if (bUnlocked != bShown)
	{
		bShown = bUnlocked;
		Row->SetVisibility(bShown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (!bShown)
	{
		return;
	}
	if (KeyName != ShownKey)
	{
		ShownKey = KeyName;
		KeyText->SetText(FText::FromString(KeyName));
	}
	const int32 Max = FMath::Clamp(MaxCount, 0, MaxPips);
	if (Count != ShownCount || Max != ShownMax)
	{
		ShownCount = Count;
		ShownMax = Max;
		CountText->SetText(FText::AsNumber(FMath::Max(Count, 0)));
		for (int32 Index = 0; Index < Pips.Num(); ++Index)
		{
			UWidget* Box = Pips[Index]->GetParent();
			const bool bUsed = Index < Max;
			Pips[Index]->SetVisibility(bUsed ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			if (Box)
			{
				Box->SetVisibility(bUsed ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			}
			// Lit from the right, as the count reads: the last one thrown goes dark at the left.
			const bool bLit = Index >= Max - Count;
			Pips[Index]->SetColorAndOpacity(bLit ? Color::XPLight() : WithAlpha(Color::Track(), 0.8f));
		}
		PaintState();
	}

	if (FlashLeft <= 0.f)
	{
		return;
	}
	FlashLeft = FMath::Max(FlashLeft - DeltaTime, 0.f);
	const float Length = FlashKind == EHudGrenadeFlash::Gained ? GainedSeconds : (FlashKind == EHudGrenadeFlash::Thrown ? ThrownSeconds : DeniedSeconds);
	const float Alpha = FMath::Clamp(FlashLeft / Length, 0.f, 1.f);
	FWidgetTransform Transform;
	switch (FlashKind)
	{
	case EHudGrenadeFlash::Gained:
		Transform.Scale = FVector2D(1.f + GainedPop * Alpha * Alpha);
		CountText->SetColorAndOpacity(FSlateColor(FMath::Lerp(ShownCount > 0 ? Color::Text() : Color::Worse(), FLinearColor::White, Alpha)));
		break;
	case EHudGrenadeFlash::Thrown:
		Transform.Scale = FVector2D(1.f + ThrownPop * Alpha);
		CountText->SetColorAndOpacity(FSlateColor(FMath::Lerp(ShownCount > 0 ? Color::Text() : Color::Worse(), Color::Accent(), Alpha)));
		break;
	case EHudGrenadeFlash::Denied:
		// A quick shake side to side, dying away, the count red.
		Transform.Translation = FVector2D(FMath::Sin((Length - FlashLeft) * 55.f) * DeniedShake * Alpha, 0.f);
		CountText->SetColorAndOpacity(FSlateColor(FMath::Lerp(Color::Worse(), Color::Hurt(), Alpha)));
		break;
	}
	Row->SetRenderTransform(Transform);
	if (FlashLeft <= 0.f)
	{
		Row->SetRenderTransform(FWidgetTransform());
		PaintState();
	}
}

void UHudGrenadeWidget::Flash(EHudGrenadeFlash Kind)
{
	FlashKind = Kind;
	FlashLeft = Kind == EHudGrenadeFlash::Gained ? GainedSeconds : (Kind == EHudGrenadeFlash::Thrown ? ThrownSeconds : DeniedSeconds);
}

void UHudGrenadeWidget::PaintState()
{
	const bool bEmpty = ShownCount <= 0;
	CountText->SetColorAndOpacity(FSlateColor(bEmpty ? Color::Worse() : Color::Text()));
	IconImage->SetColorAndOpacity(bEmpty ? EmptyIcon : FLinearColor::White);
}

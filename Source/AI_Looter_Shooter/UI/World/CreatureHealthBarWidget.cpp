#include "UI/World/CreatureHealthBarWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
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
	constexpr float BarHeight = 6.f;
	/** The same lean as the HUD's bars. */
	constexpr float BarSlant = 16.f;
	/** How long the chip of lost health stays before it drains, and how fast it drains (bar lengths per second). */
	constexpr float ChipHold = 0.4f;
	constexpr float ChipDrainRate = 0.8f;

	/** The dividers: solid near-black cuts, clear against the red and the empty track alike. */
	constexpr float DividerWidth = 2.f;
	const FLinearColor DividerColor(0.01f, 0.01f, 0.015f, 1.f);

	/** A dark rim round the whole bar so it holds its shape over bright sky and grass, and a lit strip along the red's top. */
	const FLinearColor RimColor(0.f, 0.f, 0.f, 0.8f);
	constexpr float HighlightHeight = 1.5f;

	/** The trait line's letter spacing (thousandths of an em): a spaced label, as the level's "LV". */
	constexpr int32 TraitLetterSpacing = 90;

	/** A top tier's sting: the word and the frame go this far toward white at its peak (a white-hot of their own colour). */
	constexpr float TopTierFlashWhite = 0.7f;

	USizeBox* AddFill(UWidgetTree* Tree, UOverlay* Bar, UWidget* Content, float Height = BarHeight)
	{
		USizeBox* Fill = MakeSized(Tree, Content, 0.f, Height);
		UOverlaySlot* FillSlot = Bar->AddChildToOverlay(Fill);
		FillSlot->SetHorizontalAlignment(HAlign_Left);
		FillSlot->SetVerticalAlignment(VAlign_Fill);
		return Fill;
	}

	/** A clamp biting one end of the framed bar: a small block in the tier's colour (the image's tint) with a dark edge. */
	UImage* AddClamp(UWidgetTree* Tree, UOverlay* Frame, EHorizontalAlignment Side)
	{
		UImage* Clamp = MakeImage(Tree, RectBrush(FLinearColor::White, Color::Outline(), 1.f));
		UOverlaySlot* ClampSlot = Frame->AddChildToOverlay(MakeSized(Tree, Clamp, UCreatureHealthBarWidget::ClampWidth, 0.f));
		ClampSlot->SetHorizontalAlignment(Side);
		ClampSlot->SetVerticalAlignment(VAlign_Fill);
		return Clamp;
	}
}

TSharedRef<SWidget> UCreatureHealthBarWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

		// "LV 1  Brown Spider": a small dim level, then the name as it's written, centered over the bar. A ranked creature's
		// word goes between them in its rank's color ("LV 2  Restless Brown Spider").
		UHorizontalBox* Label = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		LevelText = MakeFloatingText(WidgetTree, 10, Color::TextDim(), 80);
		UHorizontalBoxSlot* LevelSlot = Label->AddChildToHorizontalBox(LevelText);
		LevelSlot->SetVerticalAlignment(VAlign_Bottom);
		LevelSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 1.f));
		RankText = MakeFloatingText(WidgetTree, 13, Color::Text(), 20);
		UHorizontalBoxSlot* RankSlot = Label->AddChildToHorizontalBox(RankText);
		RankSlot->SetVerticalAlignment(VAlign_Bottom);
		RankSlot->SetPadding(FMargin(0.f, 0.f, 5.f, 0.f));
		NameText = MakeFloatingText(WidgetTree, 13, Color::Text(), 20);
		Label->AddChildToHorizontalBox(NameText)->SetVerticalAlignment(VAlign_Bottom);
		// The rank sting's pop scales the word (or the name) from its middle.
		RankText->SetRenderTransformPivot(FVector2D(0.5, 0.5));
		NameText->SetRenderTransformPivot(FVector2D(0.5, 0.5));
		Box->AddChildToVerticalBox(Label)->SetHorizontalAlignment(HAlign_Center);
		ApplyLabel();

		// Gravebound and up: its traits in a small dim line under the name ("FLEET · WARDED"), collapsed without any.
		TraitText = MakeFloatingText(WidgetTree, TraitTextSize, Color::TextDim(), TraitLetterSpacing, ETextJustify::Center);
		UVerticalBoxSlot* TraitSlot = Box->AddChildToVerticalBox(TraitText);
		TraitSlot->SetHorizontalAlignment(HAlign_Center);
		TraitSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 0.f));

		// A Warded creature's ward: a slim bar over the health's, back to front its dark track (faded by the UI transparency
		// setting), the chip, the pale-cyan fill, the break's white flash and the rim; leaning as the health bar does, and
		// collapsed (the tag as it always was) with no ward standing.
		UOverlay* Ward = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		FLinearColor WardTrackColor = Color::Track();
		WardTrackColor.A = 0.65f;
		UImage* WardTrack = MakeImage(WidgetTree, RectBrush(WardTrackColor));
		MarkBackground(WardTrack);
		FillOverlaySlot(Ward->AddChildToOverlay(WardTrack));
		WardChipBox = AddFill(WidgetTree, Ward, MakeImage(WidgetTree, RectBrush(WardChipColor())), WardHeight);
		WardFillBox = AddFill(WidgetTree, Ward, MakeImage(WidgetTree, RectBrush(WardFillColor())), WardHeight);
		WardFlashImage = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
		FillOverlaySlot(Ward->AddChildToOverlay(WardFlashImage));
		FillOverlaySlot(Ward->AddChildToOverlay(MakeImage(WidgetTree, RectBrush(FLinearColor::Transparent, RimColor, 1.f))));
		WardBox = MakeSized(WidgetTree, Ward, BarWidth, WardHeight);
		WardBox->SetRenderTransformPivot(FVector2D(0.5, 0.5));
		UVerticalBoxSlot* WardSlot = Box->AddChildToVerticalBox(WardBox);
		WardSlot->SetHorizontalAlignment(HAlign_Center);
		WardSlot->SetPadding(FMargin(0.f, WardGap, 0.f, 0.f));

		// The bar, back to front: a dark track (faded by the UI transparency setting), the chip, the health with its lit
		// top edge, the quarter lines, then the rim.
		UOverlay* Bar = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UImage* Track = MakeImage(WidgetTree, RectBrush(FLinearColor(0.01f, 0.015f, 0.02f, 0.65f)));
		MarkBackground(Track);
		FillOverlaySlot(Bar->AddChildToOverlay(Track));
		ChipFill = AddFill(WidgetTree, Bar, MakeImage(WidgetTree, RectBrush(FLinearColor(1.f, 0.82f, 0.72f, 0.85f))));

		UOverlay* Health = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		FillOverlaySlot(Health->AddChildToOverlay(MakeImage(WidgetTree, RectBrush(Color::Health()))));
		UOverlaySlot* HighlightSlot = Health->AddChildToOverlay(MakeSized(WidgetTree,
			MakeImage(WidgetTree, RectBrush(FMath::Lerp(Color::Health(), FLinearColor::White, 0.45f))), 0.f, HighlightHeight));
		HighlightSlot->SetHorizontalAlignment(HAlign_Fill);
		HighlightSlot->SetVerticalAlignment(VAlign_Top);
		HealthFill = AddFill(WidgetTree, Bar, Health);

		UCanvasPanel* Dividers = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		for (const float Position : DividerPositions())
		{
			UCanvasPanelSlot* LineSlot = Dividers->AddChildToCanvas(MakeImage(WidgetTree, RectBrush(DividerColor)));
			LineSlot->SetPosition(FVector2D(BarWidth * Position - DividerWidth * 0.5f, 0.f));
			LineSlot->SetSize(FVector2D(DividerWidth, BarHeight));
		}
		FillOverlaySlot(Bar->AddChildToOverlay(Dividers));
		FillOverlaySlot(Bar->AddChildToOverlay(MakeImage(WidgetTree, RectBrush(FLinearColor::Transparent, RimColor, 1.f))));

		// The bar in its frame, FramePad round it: for Hallowed and Ethereal a dark edge, a line in the tier's colour (the
		// image's tint, so it flares without a new brush) and a clamp biting each end, like the HUD's metalwork. Every other
		// tag leaves the frame out and looks as it always did (the pad stands for the bar's old 3 px under the name).
		UOverlay* Frame = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		FrameEdge = MakeImage(WidgetTree, RectBrush(FLinearColor::Transparent, Color::Outline(), 1.f));
		FillOverlaySlot(Frame->AddChildToOverlay(FrameEdge));
		FrameTierLine = MakeImage(WidgetTree, RectBrush(FLinearColor::Transparent, FLinearColor::White, FrameLine));
		FillOverlaySlot(Frame->AddChildToOverlay(FrameTierLine), FMargin(1.f));
		UOverlaySlot* BarSlot = Frame->AddChildToOverlay(MakeSized(WidgetTree, Bar, BarWidth, BarHeight));
		BarSlot->SetHorizontalAlignment(HAlign_Center);
		BarSlot->SetVerticalAlignment(VAlign_Center);
		ClampLeft = AddClamp(WidgetTree, Frame, HAlign_Left);
		ClampRight = AddClamp(WidgetTree, Frame, HAlign_Right);

		FrameBox = MakeSized(WidgetTree, Frame, BarWidth + 2.f * FramePad, BarHeight + 2.f * FramePad);
		FrameBox->SetRenderTransformPivot(FVector2D(0.5, 0.5));
		UVerticalBoxSlot* FrameSlot = Box->AddChildToVerticalBox(FrameBox);
		FrameSlot->SetHorizontalAlignment(HAlign_Center);
		FrameSlot->SetPadding(FMargin(0.f, FMath::Max(3.f - FramePad, 0.f), 0.f, 0.f));

		Box->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Box;
		ApplyBar();
		ApplyTier();
		ApplyWard();
	}
	return Super::RebuildWidget();
}

void UCreatureHealthBarWidget::SetCreature(const FText& InName, int32 InLevel, const FText& InRankWord, const FLinearColor& InRankColor)
{
	if (!CreatureName.EqualTo(InName) || CreatureLevel != InLevel || !RankWord.EqualTo(InRankWord) || !RankColor.Equals(InRankColor))
	{
		CreatureName = InName;
		CreatureLevel = InLevel;
		RankWord = InRankWord;
		RankColor = InRankColor;
		ApplyLabel();
	}
}

void UCreatureHealthBarWidget::ApplyLabel()
{
	if (LevelText && RankText && NameText)
	{
		LevelText->SetText(FText::FromString(FString::Printf(TEXT("LV %d"), CreatureLevel)));
		// The tier shows in its color on its word and the name both ("Hallowed Meadow Wolf" all in saint-gold; the tiers'
		// design, 2026-10-20), and a boss's name with no word. Basic's color is the plain text color, so a Regular creature's
		// tag looks as it always did.
		const bool bHasWord = !RankWord.IsEmpty();
		RankText->SetText(RankWord);
		RankText->SetColorAndOpacity(FSlateColor(RankColor));
		RankText->SetVisibility(bHasWord ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		NameText->SetText(CreatureName);
		NameText->SetColorAndOpacity(FSlateColor(RankColor));
		// A pop under way carries on over the fresh colors.
		if (bRankFlashing)
		{
			ApplyRankFlash();
		}
	}
}

// ---------------------------------------------------------------------------
// Its tier: the frame, the trait line
// ---------------------------------------------------------------------------

void UCreatureHealthBarWidget::SetTier(bool bInThickFrame, const FLinearColor& InFrameColor, bool bInTopTierFlash)
{
	if (bThickFrame != bInThickFrame || bTopTierFlash != bInTopTierFlash || !FrameColor.Equals(InFrameColor))
	{
		bThickFrame = bInThickFrame;
		FrameColor = InFrameColor;
		bTopTierFlash = bInTopTierFlash;
		ApplyTier();
	}
}

void UCreatureHealthBarWidget::SetTraitLine(const FText& InLine)
{
	if (!TraitLine.EqualTo(InLine))
	{
		TraitLine = InLine;
		ApplyTier();
	}
}

void UCreatureHealthBarWidget::ApplyTier()
{
	if (TraitText)
	{
		TraitText->SetText(TraitLine);
		TraitText->SetVisibility(TraitLine.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	const ESlateVisibility FrameShown = bThickFrame ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	for (UImage* Part : { FrameEdge.Get(), FrameTierLine.Get(), ClampLeft.Get(), ClampRight.Get() })
	{
		if (Part)
		{
			Part->SetVisibility(FrameShown);
		}
	}
	// The frame's colour and lean, flared if a top tier's sting is under way.
	ApplyFrameFlare(bRankFlashing && bTopTierFlash ? FrameFlareAt(RankFlashClock) : 0.f);
}

void UCreatureHealthBarWidget::ApplyFrameFlare(float Flare)
{
	if (!FrameBox)
	{
		return;
	}
	// The bar and its frame lean as one, as the HUD's bars do; a top tier's sting swells them a little from the middle.
	const float Scale = 1.f + (FrameFlareScale - 1.f) * Flare;
	FWidgetTransform Transform;
	Transform.Scale = FVector2D(Scale, Scale);
	Transform.Shear = FVector2D(BarSlant, 0.f);
	FrameBox->SetRenderTransform(Transform);
	// The tier's line and clamps go white-hot with the flare and cool back to the tier's colour.
	const FLinearColor Line = FMath::Lerp(FrameColor, FLinearColor::White, TopTierFlashWhite * Flare);
	for (UImage* Part : { FrameTierLine.Get(), ClampLeft.Get(), ClampRight.Get() })
	{
		if (Part)
		{
			Part->SetColorAndOpacity(Line);
		}
	}
}

// ---------------------------------------------------------------------------
// The rank sting's pop (a top tier's: bigger, longer, its frame flaring)
// ---------------------------------------------------------------------------

float UCreatureHealthBarWidget::RankFlashScaleAt(float Seconds)
{
	if (Seconds < 0.f || Seconds >= RankFlashSeconds)
	{
		return 1.f;
	}
	// Ease out, so the word is big for the first instant and settles quickly: a pop, not a swell.
	const float Left = 1.f - Seconds / RankFlashSeconds;
	return FMath::Lerp(1.f, RankFlashPeakScale, Left * Left * Left);
}

FLinearColor UCreatureHealthBarWidget::RankFlashColorAt(float Seconds, const FLinearColor& Normal)
{
	if (Seconds < 0.f || Seconds >= RankFlashSeconds)
	{
		return Normal;
	}
	// The lit color holds a little longer than the size does, then cools to the word's own.
	const float Left = 1.f - Seconds / RankFlashSeconds;
	return FMath::Lerp(Normal, Color::Accent(), Left * Left);
}

float UCreatureHealthBarWidget::TierFlashScaleAt(float Seconds, bool bTopTier)
{
	if (!bTopTier)
	{
		return RankFlashScaleAt(Seconds);
	}
	if (Seconds < 0.f || Seconds >= TopTierFlashSeconds)
	{
		return 1.f;
	}
	const float Left = 1.f - Seconds / TopTierFlashSeconds;
	return FMath::Lerp(1.f, TopTierFlashPeakScale, Left * Left * Left);
}

FLinearColor UCreatureHealthBarWidget::TierFlashColorAt(float Seconds, const FLinearColor& Normal, bool bTopTier)
{
	if (!bTopTier)
	{
		return RankFlashColorAt(Seconds, Normal);
	}
	if (Seconds < 0.f || Seconds >= TopTierFlashSeconds)
	{
		return Normal;
	}
	// A white-hot of its own colour rather than the accent's orange (orange over saint-gold or red reads as neither).
	const float Left = 1.f - Seconds / TopTierFlashSeconds;
	return FMath::Lerp(Normal, FMath::Lerp(Normal, FLinearColor::White, TopTierFlashWhite), Left * Left);
}

float UCreatureHealthBarWidget::FrameFlareAt(float Seconds)
{
	if (Seconds < 0.f || Seconds >= TopTierFlashSeconds)
	{
		return 0.f;
	}
	const float Left = 1.f - Seconds / TopTierFlashSeconds;
	return Left * Left;
}

void UCreatureHealthBarWidget::SetRankStingAge(float Seconds)
{
	const float Length = FlashSeconds();
	const bool bPlaying = Seconds >= 0.f && Seconds < Length;
	if (!bPlaying)
	{
		if (bRankFlashing)
		{
			// Over: the word settles back to its own size and color.
			bRankFlashing = false;
			RankFlashClock = Length;
			ApplyRankFlash();
		}
		return;
	}
	// Joined at the creature's own age, so the pop agrees with the sting's sound; the tick below paints it from here on.
	const bool bStarting = !bRankFlashing;
	bRankFlashing = true;
	RankFlashClock = Seconds;
	if (bStarting)
	{
		ApplyRankFlash();
	}
}

void UCreatureHealthBarWidget::ApplyRankFlash()
{
	if (!RankText || !NameText)
	{
		return;
	}
	// The word pops; a rank with no word (a boss) has its name in the rank's color instead, and that pops. Either way the
	// popping text's own color is the rank's.
	const bool bHasWord = !RankWord.IsEmpty();
	UTextBlock* Popping = bHasWord ? RankText : NameText;
	UTextBlock* Resting = bHasWord ? NameText : RankText;
	const float Clock = bRankFlashing ? RankFlashClock : FlashSeconds();
	const float Scale = TierFlashScaleAt(Clock, bTopTierFlash);
	Popping->SetRenderScale(FVector2D(Scale, Scale));
	Popping->SetColorAndOpacity(FSlateColor(TierFlashColorAt(Clock, RankColor, bTopTierFlash)));
	Resting->SetRenderScale(FVector2D(1.0, 1.0));
	// A top tier's frame flares with its word.
	if (bThickFrame)
	{
		ApplyFrameFlare(bTopTierFlash ? FrameFlareAt(Clock) : 0.f);
	}
}

TArray<float> UCreatureHealthBarWidget::DividerPositions()
{
	TArray<float> Positions;
	for (int32 Part = 1; Part < Parts; ++Part)
	{
		Positions.Add(static_cast<float>(Part) / Parts);
	}
	return Positions;
}

void UCreatureHealthBarWidget::SetHealth(float Health, float MaxHealth)
{
	const float NewFraction = MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f;
	if (FMath::IsNearlyEqual(NewFraction, Fraction))
	{
		return;
	}
	if (NewFraction < Fraction)
	{
		// A hit: what it took lingers as the chip for a moment.
		GhostHold = ChipHold;
	}
	Fraction = NewFraction;
	GhostFraction = FMath::Max(GhostFraction, Fraction);
	ApplyBar();
}

void UCreatureHealthBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (bRankFlashing)
	{
		// The pop runs on its own clock between the creature's calls, so it stays smooth at any update rate.
		RankFlashClock += InDeltaTime;
		bRankFlashing = RankFlashClock < FlashSeconds();
		ApplyRankFlash();
	}
	AdvanceWard(InDeltaTime);
	if (GhostFraction <= Fraction)
	{
		return;
	}
	if (GhostHold > 0.f)
	{
		GhostHold -= InDeltaTime;
		return;
	}
	GhostFraction = FMath::Max(Fraction, GhostFraction - ChipDrainRate * InDeltaTime);
	ApplyBar();
}

void UCreatureHealthBarWidget::ApplyBar()
{
	if (HealthFill && ChipFill)
	{
		HealthFill->SetWidthOverride(BarWidth * Fraction);
		ChipFill->SetWidthOverride(BarWidth * GhostFraction);
	}
}

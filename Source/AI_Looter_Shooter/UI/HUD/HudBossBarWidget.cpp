#include "UI/HUD/HudBossBarWidget.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
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
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

using namespace LooterUI;

namespace
{
	/** How far down from the top of the screen the name sits: clear of the frame rate and the minimap in the corners. */
	constexpr float BossBarTop = 30.f;

	/** The bar: a core BossCoreHeight tall inside a thin dark rim, leaning like the creatures' tags and the HUD's bars. */
	constexpr float BossCoreHeight = 8.f;
	constexpr float BossRim = 2.f;
	constexpr float BossBarHeight = BossCoreHeight + BossRim * 2.f;
	constexpr float BossCoreWidth = UHudBossBarWidget::BarWidth - BossRim * 2.f;
	constexpr float BossBarSlant = 16.f;
	/** The lit strip along the fill's top, and the ticks across the bar. */
	constexpr float BossHighlightHeight = 1.5f;
	constexpr float BossTickWidth = 2.f;

	/** After a hit the lost part lingers this long, then drains at this share of the bar per second (as the vitals bar). */
	constexpr float BossChipHoldSeconds = 0.45f;
	constexpr float BossChipDrainSpeed = 0.6f;
	/** Seconds to fade fully in or out, to grey or back, and how long a new phase's name flashes. */
	constexpr float BossFadeSeconds = 0.4f;
	constexpr float BossGreySeconds = 0.25f;
	constexpr float BossPhaseFlashSeconds = 1.6f;

	/** The empty track: the HUD's dark glass (as the vitals ring's disc and the level badge's inside). */
	FLinearColor TrackColor() { return Hex(7, 26, 40, 170); }
	FLinearColor RimColor() { return Color::Outline() * FLinearColor(1.f, 1.f, 1.f, 0.55f); }
	FLinearColor ChipColor() { return Hex(255, 233, 221, 230); }
	/** The fill while the boss can't be hurt: a cold grey that still reads against the dark track. */
	FLinearColor GreyFill() { return Hex(138, 146, 154); }
	/** A tick is a dark cut through the lit bar and a light line on the empty track: one color would vanish on one of them. */
	FLinearColor LitTickColor() { return Color::Outline() * FLinearColor(1.f, 1.f, 1.f, 0.8f); }
	FLinearColor TrackTickColor() { return Color::TextDim() * FLinearColor(1.f, 1.f, 1.f, 0.55f); }

	/** A child of the core, left-aligned, sized by width later (ApplyFill). */
	USizeBox* AddFill(UWidgetTree* Tree, UOverlay* Core, UWidget* Content)
	{
		USizeBox* Fill = MakeSized(Tree, Content, 0.f, BossCoreHeight);
		Fill->SetWidthOverride(0.f);
		UOverlaySlot* FillSlot = Core->AddChildToOverlay(Fill);
		FillSlot->SetHorizontalAlignment(HAlign_Left);
		FillSlot->SetVerticalAlignment(VAlign_Fill);
		return Fill;
	}
}

TSharedRef<SWidget> UHudBossBarWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Root;

		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

		// "LV 9  ABEL RANSOM, THE KEEPER": a small dim level, then the name in its rank's color, centered over the bar.
		UHorizontalBox* Title = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		LevelLabel = MakeFloatingText(WidgetTree, 12, Color::TextDim(), 80);
		UHorizontalBoxSlot* LevelSlot = Title->AddChildToHorizontalBox(LevelLabel);
		LevelSlot->SetVerticalAlignment(VAlign_Bottom);
		LevelSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 2.f));
		NameLabel = MakeFloatingText(WidgetTree, 19, NameColor, 120);
		Title->AddChildToHorizontalBox(NameLabel)->SetVerticalAlignment(VAlign_Bottom);
		Box->AddChildToVerticalBox(Title)->SetHorizontalAlignment(HAlign_Center);

		// The bar's core, back to front: the dark track (a background), the chip, then the health with its lit top edge.
		UOverlay* Core = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UImage* Track = MakeImage(WidgetTree, RectBrush(TrackColor()));
		MarkBackground(Track);
		FillOverlaySlot(Core->AddChildToOverlay(Track));
		UImage* Chip = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
		Chip->SetColorAndOpacity(ChipColor());
		ChipBox = AddFill(WidgetTree, Core, Chip);
		UOverlay* Lit = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		FillImage = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
		FillOverlaySlot(Lit->AddChildToOverlay(FillImage));
		HighlightImage = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
		UOverlaySlot* HighlightSlot = Lit->AddChildToOverlay(MakeSized(WidgetTree, HighlightImage, 0.f, BossHighlightHeight));
		HighlightSlot->SetHorizontalAlignment(HAlign_Fill);
		HighlightSlot->SetVerticalAlignment(VAlign_Top);
		FillBox = AddFill(WidgetTree, Core, Lit);

		// The thin dark rim round the core keeps the bar readable on bright sky; it is the bar's only backing, so it fades
		// with the UI transparency setting. The ticks go over the rim too, so they notch its edge.
		UBorder* RimBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		RimBorder->SetBrush(RectBrush(RimColor()));
		MarkBackground(RimBorder);
		RimBorder->SetPadding(FMargin(BossRim));
		RimBorder->SetContent(Core);
		UOverlay* Stack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		FillOverlaySlot(Stack->AddChildToOverlay(RimBorder));
		TickLayer = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		FillOverlaySlot(Stack->AddChildToOverlay(TickLayer));
		USizeBox* BarSize = MakeSized(WidgetTree, Stack, BarWidth, BossBarHeight);
		BarSize->SetRenderShear(FVector2D(BossBarSlant, 0.f));
		UVerticalBoxSlot* BarSlot = Box->AddChildToVerticalBox(BarSize);
		BarSlot->SetHorizontalAlignment(HAlign_Center);
		BarSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));

		// Under the bar: the phase's name, or while the boss can't be hurt, what to do about it.
		PhaseLabel = MakeFloatingText(WidgetTree, 12, Color::TextDim(), 200, ETextJustify::Center);
		UVerticalBoxSlot* PhaseSlot = Box->AddChildToVerticalBox(PhaseLabel);
		PhaseSlot->SetHorizontalAlignment(HAlign_Center);
		PhaseSlot->SetPadding(FMargin(0.f, 5.f, 0.f, 0.f));

		UCanvasPanelSlot* BoxSlot = Root->AddChildToCanvas(Box);
		BoxSlot->SetAnchors(FAnchors(0.5f, 0.f));
		BoxSlot->SetAlignment(FVector2D(0.5f, 0.f));
		BoxSlot->SetPosition(FVector2D(0.f, BossBarTop));
		BoxSlot->SetAutoSize(true);
		Box->SetRenderOpacity(Opacity);
		Box->SetVisibility(Opacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		Cluster = Box;

		ShownFillWidth = -1.f;
		ShownChipWidth = -1.f;
		BuildTicks();
		ApplyTexts();
		ApplyFill();
		ApplyColors();
	}
	return Super::RebuildWidget();
}

// ---------------------------------------------------------------------------
// What it shows
// ---------------------------------------------------------------------------

TArray<float> UHudBossBarWidget::MakeTickShares(const TArray<float>& PhaseShares)
{
	// The first phase starts at full health (no tick at the bar's end); a share at the very ends would sit on the rim.
	TArray<float> Shares;
	for (int32 Index = 1; Index < PhaseShares.Num(); ++Index)
	{
		const float Share = PhaseShares[Index];
		if (Share > 0.001f && Share < 0.999f && !Shares.ContainsByPredicate([Share](float Other) { return FMath::IsNearlyEqual(Other, Share, 0.001f); }))
		{
			Shares.Add(Share);
		}
	}
	Shares.Sort([](float A, float B) { return A > B; });
	return Shares;
}

FText UHudBossBarWidget::MakeLevelText(int32 Level)
{
	return FText::FromString(FString::Printf(TEXT("LV %d"), FMath::Max(Level, 1)));
}

FText UHudBossBarWidget::MakePhaseLine(const FText& InPhaseName, bool bUntargetable, const FText& InHint)
{
	return (bUntargetable && !InHint.IsEmpty() ? InHint : InPhaseName).ToUpper();
}

FText UHudBossBarWidget::GetNameText() const
{
	return NameLabel ? NameLabel->GetText() : BossName.ToUpper();
}

FText UHudBossBarWidget::GetLevelText() const
{
	return LevelLabel ? LevelLabel->GetText() : MakeLevelText(BossLevel);
}

FText UHudBossBarWidget::GetPhaseText() const
{
	return PhaseLabel ? PhaseLabel->GetText() : MakePhaseLine(PhaseName, bGreyed, Hint);
}

void UHudBossBarWidget::SetBoss(const FText& InName, int32 InLevel, const FLinearColor& InNameColor, const TArray<float>& PhaseShares)
{
	BossName = InName;
	BossLevel = FMath::Max(InLevel, 1);
	NameColor = InNameColor;
	TickShares = MakeTickShares(PhaseShares);
	// A new fight: full, in its first phase, hurtable.
	Fraction = 1.f;
	ChipFraction = 1.f;
	ChipHold = 0.f;
	PhaseIndex = INDEX_NONE;
	PhaseName = FText::GetEmpty();
	Hint = FText::GetEmpty();
	bGreyed = false;
	Grey = 0.f;
	PhaseFlash = 0.f;
	ShownFillWidth = -1.f;
	ShownChipWidth = -1.f;
	BuildTicks();
	ApplyTexts();
	ApplyFill();
	ApplyColors();
}

void UHudBossBarWidget::SetHealth(float Health, float MaxHealth)
{
	const float NewFraction = MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f;
	if (FMath::IsNearlyEqual(NewFraction, Fraction))
	{
		return;
	}
	if (NewFraction < Fraction)
	{
		// A hit: what it took lingers as the chip for a moment.
		ChipHold = BossChipHoldSeconds;
	}
	else
	{
		// Healed (a reset): no chip.
		ChipFraction = NewFraction;
	}
	Fraction = NewFraction;
	ChipFraction = FMath::Max(ChipFraction, Fraction);
	ApplyFill();
}

void UHudBossBarWidget::SetPhase(int32 Index, const FText& InPhaseName)
{
	if (Index == PhaseIndex && PhaseName.EqualTo(InPhaseName))
	{
		return;
	}
	// A later phase flashes its name; the first is only the fight starting.
	if (PhaseIndex != INDEX_NONE && Index > PhaseIndex)
	{
		PhaseFlash = BossPhaseFlashSeconds;
	}
	PhaseIndex = Index;
	PhaseName = InPhaseName;
	ApplyTexts();
	ApplyColors();
}

void UHudBossBarWidget::SetUntargetable(bool bInUntargetable, const FText& InHint)
{
	if (bGreyed == bInUntargetable && Hint.EqualTo(InHint))
	{
		return;
	}
	bGreyed = bInUntargetable;
	Hint = InHint;
	ApplyTexts();
	ApplyColors();
}

void UHudBossBarWidget::SetShown(bool bInShown)
{
	bWanted = bInShown;
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void UHudBossBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!Cluster)
	{
		return;
	}

	// In while its fight is on and nothing covers the game; out otherwise.
	const float Target = bWanted && !IsMenuOpen() ? 1.f : 0.f;
	if (Opacity != Target)
	{
		Opacity = FMath::FInterpConstantTo(Opacity, Target, InDeltaTime, 1.f / BossFadeSeconds);
		Cluster->SetRenderOpacity(Opacity);
		Cluster->SetVisibility(Opacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	bool bFillMoved = false;
	if (ChipFraction > Fraction)
	{
		if (ChipHold > 0.f)
		{
			ChipHold -= InDeltaTime;
		}
		else
		{
			ChipFraction = FMath::Max(Fraction, ChipFraction - BossChipDrainSpeed * InDeltaTime);
			bFillMoved = true;
		}
	}
	bool bColorsMoved = false;
	const float GreyTarget = bGreyed ? 1.f : 0.f;
	if (Grey != GreyTarget)
	{
		Grey = FMath::FInterpConstantTo(Grey, GreyTarget, InDeltaTime, 1.f / BossGreySeconds);
		bColorsMoved = true;
	}
	if (PhaseFlash > 0.f)
	{
		PhaseFlash = FMath::Max(0.f, PhaseFlash - InDeltaTime);
		bColorsMoved = true;
	}
	if (bFillMoved)
	{
		ApplyFill();
	}
	if (bColorsMoved)
	{
		ApplyColors();
	}
}

bool UHudBossBarWidget::IsMenuOpen() const
{
	const APlayerController* Player = GetOwningPlayer();
	const ALooterHUD* LooterHUD = Player ? Cast<ALooterHUD>(Player->GetHUD()) : nullptr;
	if (LooterHUD && LooterHUD->IsMenuOpen())
	{
		return true;
	}
	const UWorld* World = GetWorld();
	return World && World->IsPaused();
}

void UHudBossBarWidget::ApplyTexts()
{
	if (LevelLabel)
	{
		LevelLabel->SetText(MakeLevelText(BossLevel));
	}
	if (NameLabel)
	{
		NameLabel->SetText(BossName.ToUpper());
	}
	if (PhaseLabel)
	{
		const FText Line = MakePhaseLine(PhaseName, bGreyed, Hint);
		PhaseLabel->SetText(Line);
		PhaseLabel->SetVisibility(Line.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UHudBossBarWidget::BuildTicks()
{
	if (!TickLayer || !WidgetTree)
	{
		return;
	}
	TickLayer->ClearChildren();
	TickImages.Reset();
	for (const float Share : TickShares)
	{
		UImage* Tick = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
		UCanvasPanelSlot* TickSlot = TickLayer->AddChildToCanvas(Tick);
		TickSlot->SetPosition(FVector2D(BossRim + BossCoreWidth * Share - BossTickWidth * 0.5f, 0.f));
		TickSlot->SetSize(FVector2D(BossTickWidth, BossBarHeight));
		TickImages.Add(Tick);
	}
	ShownLitTicks = INDEX_NONE;
}

void UHudBossBarWidget::ApplyFill()
{
	if (!FillBox || !ChipBox)
	{
		return;
	}
	// In half pixels, so a draining chip only repaints when its edge visibly moves.
	const float FillWidth = FMath::RoundToFloat(BossCoreWidth * Fraction * 2.f) * 0.5f;
	const float ChipWidth = FMath::RoundToFloat(BossCoreWidth * ChipFraction * 2.f) * 0.5f;
	if (FillWidth != ShownFillWidth)
	{
		ShownFillWidth = FillWidth;
		FillBox->SetWidthOverride(FillWidth);
	}
	if (ChipWidth != ShownChipWidth)
	{
		ShownChipWidth = ChipWidth;
		ChipBox->SetWidthOverride(ChipWidth);
	}

	// A tick is on the lit part while the health left is past its line; the ticks are highest first.
	int32 LitTicks = 0;
	for (const float Share : TickShares)
	{
		LitTicks += Share < Fraction ? 1 : 0;
	}
	if (LitTicks == ShownLitTicks)
	{
		return;
	}
	ShownLitTicks = LitTicks;
	for (int32 Index = 0; Index < TickImages.Num() && Index < TickShares.Num(); ++Index)
	{
		if (UImage* Tick = TickImages[Index])
		{
			Tick->SetColorAndOpacity(TickShares[Index] < Fraction ? LitTickColor() : TrackTickColor());
		}
	}
}

void UHudBossBarWidget::ApplyColors()
{
	if (!FillImage || !HighlightImage || !NameLabel || !LevelLabel || !PhaseLabel)
	{
		return;
	}
	const FLinearColor Fill = FMath::Lerp(Color::Health(), GreyFill(), Grey);
	FillImage->SetColorAndOpacity(Fill);
	HighlightImage->SetColorAndOpacity(FMath::Lerp(Fill, FLinearColor::White, 0.45f));
	NameLabel->SetColorAndOpacity(FSlateColor(FMath::Lerp(NameColor, Color::TextDim(), Grey * 0.6f)));
	LevelLabel->SetColorAndOpacity(FSlateColor(Color::TextDim()));
	// A hint reads in the text color (it's what to do now); a new phase's name flashes in the accent, then settles dim.
	const float Flash = FMath::Clamp(PhaseFlash / BossPhaseFlashSeconds, 0.f, 1.f);
	const FLinearColor Line = bGreyed && !Hint.IsEmpty() ? Color::Text() : FMath::Lerp(Color::TextDim(), Color::Accent(), Flash);
	PhaseLabel->SetColorAndOpacity(FSlateColor(Line));
}

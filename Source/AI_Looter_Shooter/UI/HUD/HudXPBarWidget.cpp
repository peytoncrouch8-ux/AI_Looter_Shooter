#include "UI/HUD/HudXPBarWidget.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"

using namespace LooterUI;

namespace
{
	constexpr int32 SegmentCount = 40;
	constexpr float BarHeight = 8.f;
	/** The same lean as the HUD's health and magazine bars. */
	constexpr float BarSlant = 16.f;

	/** Opacity when nothing is happening, and how long it stays fully visible after a gain (as the HUD's corners do). */
	constexpr float IdleOpacity = 0.6f;
	constexpr float ActivityHold = 3.f;

	/** How fast the bar eases to a new amount (per second, of the remaining gap), and its slowest speed in levels per second. */
	constexpr double EaseRate = 5.0;
	constexpr double MinEaseSpeed = 0.25;

	/** Seconds the "+10 XP" stays up, and the level number glows after a level-up. */
	constexpr float GainDuration = 1.8f;
	constexpr float FlashDuration = 1.5f;

	FString FormatXP(int64 Value)
	{
		return FText::AsNumber(Value).ToString();
	}
}

TSharedRef<SWidget> UHudXPBarWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

		// Top row: "LV 12" on the left, "+10 XP" and "XP 40 / 100" on the right, over the bar's ends.
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UTextBlock* LevelLabel = MakeFloatingText(WidgetTree, 12, Color::TextDim(), 100);
		LevelLabel->SetText(FText::FromString(TEXT("LV")));
		UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LevelLabel);
		LabelSlot->SetVerticalAlignment(VAlign_Bottom);
		LabelSlot->SetPadding(FMargin(0.f, 0.f, 5.f, 3.f));
		LevelValue = MakeFloatingText(WidgetTree, 20, Color::Text());
		Row->AddChildToHorizontalBox(LevelValue)->SetVerticalAlignment(VAlign_Bottom);
		Row->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass()))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		GainText = MakeFloatingText(WidgetTree, 13, Color::Accent(), 60, ETextJustify::Right);
		GainText->SetVisibility(ESlateVisibility::Hidden);
		UHorizontalBoxSlot* GainSlot = Row->AddChildToHorizontalBox(GainText);
		GainSlot->SetVerticalAlignment(VAlign_Bottom);
		GainSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 3.f));
		XPText = MakeFloatingText(WidgetTree, 12, Color::TextDim(), 60, ETextJustify::Right);
		UHorizontalBoxSlot* XPSlot = Row->AddChildToHorizontalBox(XPText);
		XPSlot->SetVerticalAlignment(VAlign_Bottom);
		XPSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
		Box->AddChildToVerticalBox(MakeSized(WidgetTree, Row, BarWidth));

		// The bar: slim segments sheared into a parallelogram, on a faint dark backing (faded by the UI transparency).
		UBorder* Backing = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Backing->SetBrush(RectBrush(FLinearColor(0.f, 0.02f, 0.04f, 0.45f)));
		MarkBackground(Backing);
		Backing->SetPadding(FMargin(2.f));
		UHorizontalBox* Bar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Segments.Reset();
		for (int32 Index = 0; Index < SegmentCount; ++Index)
		{
			UImage* Segment = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
			Segment->SetColorAndOpacity(Color::SegmentOff());
			UHorizontalBoxSlot* SegmentSlot = Bar->AddChildToHorizontalBox(Segment);
			SegmentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			SegmentSlot->SetPadding(FMargin(0.f, 0.f, Index + 1 < SegmentCount ? 2.f : 0.f, 0.f));
			Segments.Add(Segment);
		}
		SegmentFill.Init(0.f, SegmentCount);
		Backing->SetContent(Bar);
		USizeBox* BarSize = MakeSized(WidgetTree, Backing, BarWidth, BarHeight);
		BarSize->SetRenderShear(FVector2D(BarSlant, 0.f));
		Box->AddChildToVerticalBox(BarSize)->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));

		Box->SetVisibility(ESlateVisibility::HitTestInvisible);
		Box->SetRenderOpacity(IdleOpacity);
		Cluster = Box;
		WidgetTree->RootWidget = Box;
	}
	return Super::RebuildWidget();
}

void UHudXPBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	UPlayerProgressionSubsystem* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	if (Progression.Get() != Subsystem)
	{
		if (UPlayerProgressionSubsystem* Old = Progression.Get())
		{
			Old->OnXPChanged.RemoveAll(this);
			Old->OnLevelUp.RemoveAll(this);
		}
		if (Subsystem)
		{
			Subsystem->OnXPChanged.AddUObject(this, &UHudXPBarWidget::HandleXPChanged);
			Subsystem->OnLevelUp.AddUObject(this, &UHudXPBarWidget::HandleLevelUp);
		}
		Progression = Subsystem;
	}
	Cluster->SetVisibility(Subsystem ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	Retarget(true);
}

void UHudXPBarWidget::NativeDestruct()
{
	// The subsystem outlives the HUD (it carries across levels): leave no bindings behind.
	if (UPlayerProgressionSubsystem* Subsystem = Progression.Get())
	{
		Subsystem->OnXPChanged.RemoveAll(this);
		Subsystem->OnLevelUp.RemoveAll(this);
	}
	Progression.Reset();
	Super::NativeDestruct();
}

void UHudXPBarWidget::HandleLevelUp(int32 NewLevel)
{
	PendingAnnouncement = FMath::Max(PendingAnnouncement, NewLevel);
}

void UHudXPBarWidget::HandleXPChanged(int64 Gained, EXPSource Source)
{
	// Several kills in quick succession add up into one "+XP".
	if (Gained > 0)
	{
		ShownGain = GainTime > 0.f ? ShownGain + Gained : Gained;
		GainText->SetText(FText::FromString(FString::Printf(TEXT("+%s XP"), *FormatXP(ShownGain))));
		GainText->SetVisibility(ESlateVisibility::HitTestInvisible);
		GainTime = GainDuration;
	}
	Activity = ActivityHold;
	// Earned experience eases in; a level set directly (console, reset) jumps there.
	Retarget(Gained <= 0);
}

void UHudXPBarWidget::Retarget(bool bSnap)
{
	const UPlayerProgressionSubsystem* Subsystem = Progression.Get();
	if (!Subsystem)
	{
		return;
	}

	const int32 Level = Subsystem->GetLevel();
	bMaxLevel = Subsystem->IsMaxLevel();
	TargetProgress = Level + (bMaxLevel ? 0.0 : static_cast<double>(Subsystem->GetLevelProgress()));
	if (bSnap || ShownProgress < 0.0 || TargetProgress < ShownProgress)
	{
		ShownProgress = TargetProgress;
		PendingAnnouncement = 0;
	}
	else if (TargetProgress - ShownProgress > 2.0)
	{
		// A huge gain skips the levels in between: the bar fills the last one and wraps once.
		ShownProgress = FMath::FloorToDouble(TargetProgress) - 1.0;
	}

	// The numbers follow at once unless the bar still has to fill up to the new level.
	bXPTextPending = FMath::FloorToInt(ShownProgress) != Level;
	if (!bXPTextPending)
	{
		UpdateXPText();
	}
	ShowProgress();
}

void UHudXPBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const bool bEasing = ShownProgress < TargetProgress;
	const float WantedOpacity = Activity > 0.f ? 1.f : IdleOpacity;
	const bool bFading = !FMath::IsNearlyEqual(Cluster->GetRenderOpacity(), WantedOpacity, 0.005f);
	if (!bEasing && !bFading && GainTime <= 0.f && FlashTime <= 0.f && Activity <= 0.f)
	{
		return;
	}

	if (bEasing)
	{
		const double Gap = TargetProgress - ShownProgress;
		const double Step = FMath::Max(Gap * EaseRate, MinEaseSpeed) * InDeltaTime;
		ShownProgress = Gap <= Step ? TargetProgress : ShownProgress + Step;
		ShowProgress();
	}

	if (GainTime > 0.f)
	{
		GainTime -= InDeltaTime;
		// Rises a little as it fades, after a moment at full strength.
		const float Age = GainDuration - GainTime;
		GainText->SetRenderOpacity(FMath::Clamp(GainTime / 0.6f, 0.f, 1.f));
		GainText->SetRenderTranslation(FVector2D(0.f, -8.f * FMath::Clamp(Age / GainDuration, 0.f, 1.f)));
		if (GainTime <= 0.f)
		{
			GainText->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (FlashTime > 0.f)
	{
		FlashTime = FMath::Max(0.f, FlashTime - InDeltaTime);
		LevelValue->SetColorAndOpacity(FSlateColor(FMath::Lerp(Color::Text(), Color::Accent(), FlashTime / FlashDuration)));
	}

	Activity = FMath::Max(0.f, Activity - InDeltaTime);
	if (bFading)
	{
		const float Opacity = FMath::FInterpTo(Cluster->GetRenderOpacity(), WantedOpacity, InDeltaTime, 5.f);
		Cluster->SetRenderOpacity(FMath::IsNearlyEqual(Opacity, WantedOpacity, 0.005f) ? WantedOpacity : Opacity);
	}
}

void UHudXPBarWidget::ShowProgress()
{
	// At the top level the bar stays full once it gets there.
	const bool bFull = bMaxLevel && ShownProgress >= TargetProgress;
	const double Level = FMath::FloorToDouble(ShownProgress);
	SetShownLevel(static_cast<int32>(Level));
	PaintBar(bFull ? 1.f : static_cast<float>(ShownProgress - Level));
}

void UHudXPBarWidget::SetShownLevel(int32 Level)
{
	if (Level == ShownLevel)
	{
		return;
	}
	const bool bWrapped = ShownLevel != INDEX_NONE && Level > ShownLevel;
	ShownLevel = Level;
	LevelValue->SetText(FText::AsNumber(Level));

	if (bWrapped)
	{
		FlashTime = FlashDuration;
		if (PendingAnnouncement > 0 && Level >= PendingAnnouncement)
		{
			OnAnnouncement.ExecuteIfBound(FText::FromString(FString::Printf(TEXT("Level up! Level %d"), PendingAnnouncement)));
			PendingAnnouncement = 0;
		}
	}
	if (bXPTextPending && Progression.IsValid() && Level >= Progression->GetLevel())
	{
		bXPTextPending = false;
		UpdateXPText();
	}
}

void UHudXPBarWidget::PaintBar(float Fraction)
{
	// Whole segments light up in the accent color; the one being filled glows in proportion, so small gains still show.
	const float Lit = FMath::Clamp(Fraction, 0.f, 1.f) * SegmentCount;
	for (int32 Index = 0; Index < Segments.Num(); ++Index)
	{
		// In sixteenths: easing only repaints a segment when it visibly changes.
		const float Fill = FMath::RoundToFloat(FMath::Clamp(Lit - Index, 0.f, 1.f) * 16.f) / 16.f;
		if (Fill == SegmentFill[Index])
		{
			continue;
		}
		SegmentFill[Index] = Fill;
		const FLinearColor SegmentColor = Fill <= 0.f ? Color::SegmentOff()
			: Color::Accent().CopyWithNewOpacity(FMath::Lerp(0.2f, 1.f, Fill));
		Segments[Index]->SetColorAndOpacity(SegmentColor);
	}
}

void UHudXPBarWidget::UpdateXPText()
{
	const UPlayerProgressionSubsystem* Subsystem = Progression.Get();
	if (!Subsystem)
	{
		return;
	}
	XPText->SetText(FText::FromString(Subsystem->IsMaxLevel() ? FString(TEXT("MAX"))
		: FString::Printf(TEXT("XP %s / %s"), *FormatXP(Subsystem->GetXP()), *FormatXP(Subsystem->GetXPToNextLevel()))));
	XPText->SetColorAndOpacity(FSlateColor(Subsystem->IsMaxLevel() ? Color::Accent() : Color::TextDim()));
}

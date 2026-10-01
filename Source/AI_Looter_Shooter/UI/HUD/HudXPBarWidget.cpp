#include "UI/HUD/HudXPBarWidget.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"

using namespace LooterUI;

namespace
{
	/** Ten sections, each a tenth of the way to the next level, marked by faint ticks on a hairline. */
	constexpr int32 SectionCount = 10;
	constexpr float LineHeight = 3.f;
	constexpr float TickWidth = 1.5f;
	constexpr float TickHeight = 9.f;

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
		UTextBlock* LevelLabel = MakeFloatingText(WidgetTree, 15, Color::Accent(), 100);
		LevelLabel->SetText(FText::FromString(TEXT("LV")));
		UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LevelLabel);
		LabelSlot->SetVerticalAlignment(VAlign_Bottom);
		LabelSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
		LevelValue = MakeFloatingText(WidgetTree, 15, Color::Accent());
		Row->AddChildToHorizontalBox(LevelValue)->SetVerticalAlignment(VAlign_Bottom);
		Row->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass()))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		GainText = MakeFloatingText(WidgetTree, 13, Color::Accent(), 60, ETextJustify::Right);
		GainText->SetVisibility(ESlateVisibility::Hidden);
		UHorizontalBoxSlot* GainSlot = Row->AddChildToHorizontalBox(GainText);
		GainSlot->SetVerticalAlignment(VAlign_Bottom);
		GainSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 3.f));
		XPText = MakeFloatingText(WidgetTree, 14, Color::TextDim(), 60, ETextJustify::Right);
		UHorizontalBoxSlot* XPSlot = Row->AddChildToHorizontalBox(XPText);
		XPSlot->SetVerticalAlignment(VAlign_Bottom);
		XPSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
		Box->AddChildToVerticalBox(MakeSized(WidgetTree, Row, BarWidth));

		// The bar: a hairline along the bottom of the screen with a faint tick at every tenth of the level. The filled part
		// and the rest share its width (FilledSlot / RestSlot).
		UOverlay* Bar = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UOverlaySlot* TrackSlot = Bar->AddChildToOverlay(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Hex(90, 200, 255, 64))), 0.f, LineHeight));
		TrackSlot->SetHorizontalAlignment(HAlign_Fill);
		TrackSlot->SetVerticalAlignment(VAlign_Center);
		UHorizontalBox* Split = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		FilledSlot = Split->AddChildToHorizontalBox(MakeImage(WidgetTree, RectBrush(Color::Accent())));
		FSlateChildSize NoWidth(ESlateSizeRule::Fill);
		NoWidth.Value = 0.f;
		FilledSlot->SetSize(NoWidth);
		RestSlot = Split->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass()));
		RestSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		UOverlaySlot* SplitSlot = Bar->AddChildToOverlay(MakeSized(WidgetTree, Split, 0.f, LineHeight));
		SplitSlot->SetHorizontalAlignment(HAlign_Fill);
		SplitSlot->SetVerticalAlignment(VAlign_Center);
		// The ticks: ten equal cells, each with a tick at its right end but the last.
		UHorizontalBox* Ticks = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		for (int32 Index = 0; Index < SectionCount; ++Index)
		{
			UOverlay* Cell = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			if (Index + 1 < SectionCount)
			{
				UOverlaySlot* TickSlot = Cell->AddChildToOverlay(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::TextDim() * FLinearColor(1.f, 1.f, 1.f, 0.4f))), TickWidth, TickHeight));
				TickSlot->SetHorizontalAlignment(HAlign_Right);
				TickSlot->SetVerticalAlignment(VAlign_Center);
			}
			Ticks->AddChildToHorizontalBox(Cell)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		UOverlaySlot* TicksSlot = Bar->AddChildToOverlay(Ticks);
		TicksSlot->SetHorizontalAlignment(HAlign_Fill);
		TicksSlot->SetVerticalAlignment(VAlign_Fill);
		Box->AddChildToVerticalBox(MakeSized(WidgetTree, Bar, BarWidth, TickHeight))->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));

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
		LevelValue->SetColorAndOpacity(FSlateColor(FMath::Lerp(Color::Accent(), Color::Text(), FlashTime / FlashDuration)));
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
	// The filled part and the rest share the line's width; in 1/500ths, so easing only repaints when it visibly moves.
	const float Fill = FMath::RoundToFloat(FMath::Clamp(Fraction, 0.f, 1.f) * 500.f) / 500.f;
	if (Fill == ShownFill || !FilledSlot || !RestSlot)
	{
		return;
	}
	ShownFill = Fill;
	FSlateChildSize Filled(ESlateSizeRule::Fill);
	Filled.Value = Fill;
	FilledSlot->SetSize(Filled);
	FSlateChildSize Rest(ESlateSizeRule::Fill);
	Rest.Value = 1.f - Fill;
	RestSlot->SetSize(Rest);
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

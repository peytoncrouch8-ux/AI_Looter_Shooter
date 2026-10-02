#include "UI/HUD/HudXPBarWidget.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"

// The widget tree and its shapes are built in HudXPBarWidgetLayout.cpp; this file follows the player's progress.

using namespace LooterUI;

namespace
{
	/** Opacity when nothing is happening, and how long it stays fully visible after a gain (as the HUD's corners do). */
	constexpr float IdleOpacity = 0.6f;
	constexpr float ActivityHold = 3.f;

	/** How fast the bar eases to a new amount (per second, of the remaining gap), and its slowest speed in levels per second. */
	constexpr double EaseRate = 5.0;
	constexpr double MinEaseSpeed = 0.25;
	/** After a gain, the just-earned stretch shows on its own this long before the bar starts catching up to it. */
	constexpr float GainHold = 0.35f;
	/** Seconds the leading edge's glow takes to settle once the bar has caught up. */
	constexpr float EdgeGlowFade = 0.6f;

	/** Seconds the "+10 XP" stays up, and the ring and number flash after a level-up. */
	constexpr float GainDuration = 1.8f;
	constexpr float FlashDuration = 1.5f;
	/** Seconds the level-up pulse takes to spread, and how far it gets (times the circle's size). */
	constexpr float PulseDuration = 0.7f;
	constexpr float PulseScale = 1.8f;

	/** The bar is drawn in steps of about a pixel, so easing only repaints when it visibly moves. */
	constexpr float BarSteps = UHudXPBarWidget::BarWidth;

	FString FormatXP(int64 Value)
	{
		return FText::AsNumber(Value).ToString();
	}

	float ToBarStep(float Fraction)
	{
		return FMath::RoundToFloat(FMath::Clamp(Fraction, 0.f, 1.f) * BarSteps) / BarSteps;
	}

	FSlateChildSize ShareOfBar(float Share)
	{
		FSlateChildSize Size(ESlateSizeRule::Fill);
		Size.Value = Share;
		return Size;
	}
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
	Cluster->SetRenderOpacity(Activity > 0.f ? 1.f : IdleOpacity);
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
	const bool bWasResting = ShownProgress >= TargetProgress;
	Retarget(Gained <= 0);
	if (Gained > 0 && ShownProgress < TargetProgress)
	{
		EdgeGlow = 1.f;
		PaintEdge(EdgeGlow);
		// A bar at rest lets the new stretch show on its own for a moment; one already catching up keeps going, so a
		// burst of kills never stalls it.
		if (bWasResting)
		{
			GainHoldTime = GainHold;
		}
	}
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
		GainHoldTime = 0.f;
	}
	else if (TargetProgress - ShownProgress > 2.0)
	{
		// A huge gain skips the levels in between: the bar fills the last one and wraps once.
		ShownProgress = FMath::FloorToDouble(TargetProgress) - 1.0;
	}

	// The numbers follow at once unless the bar still has to fill up to the new level.
	bXPTextPending = FMath::FloorToInt32(ShownProgress) != Level;
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
	if (!bEasing && !bFading && GainTime <= 0.f && FlashTime <= 0.f && PulseTime <= 0.f && EdgeGlow <= 0.f && Activity <= 0.f)
	{
		return;
	}

	if (GainHoldTime > 0.f)
	{
		GainHoldTime = FMath::Max(0.f, GainHoldTime - InDeltaTime);
	}
	else if (bEasing)
	{
		const double Gap = TargetProgress - ShownProgress;
		const double Step = FMath::Max(Gap * EaseRate, MinEaseSpeed) * InDeltaTime;
		ShownProgress = Gap <= Step ? TargetProgress : ShownProgress + Step;
		ShowProgress();
	}
	else if (EdgeGlow > 0.f)
	{
		// Caught up: the leading edge's glow settles back to its resting light.
		EdgeGlow = FMath::Max(0.f, EdgeGlow - InDeltaTime / EdgeGlowFade);
		PaintEdge(EdgeGlow);
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
		PaintBadge(FlashTime / FlashDuration);
	}

	if (PulseTime > 0.f)
	{
		// The ring spreads quickly, slowing as it fades out.
		PulseTime = FMath::Max(0.f, PulseTime - InDeltaTime);
		const float Age = 1.f - PulseTime / PulseDuration;
		const float Spread = FMath::InterpEaseOut(0.f, 1.f, Age, 2.f);
		BadgePulse->SetRenderScale(FVector2D(FMath::Lerp(1.f, PulseScale, Spread)));
		BadgePulse->SetRenderOpacity(1.f - Age);
		if (PulseTime <= 0.f)
		{
			BadgePulse->SetVisibility(ESlateVisibility::Hidden);
		}
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
	const float Fraction = bFull ? 1.f : static_cast<float>(ShownProgress - Level);

	// The stretch still to catch up to runs ahead to the target, or to the bar's end while the target is in a later level
	// (after the wrap it starts again from the left).
	float GainEnd = Fraction;
	if (ShownProgress < TargetProgress)
	{
		const double TargetLevel = FMath::FloorToDouble(TargetProgress);
		GainEnd = TargetLevel > Level ? 1.f : static_cast<float>(TargetProgress - TargetLevel);
	}
	PaintBar(Fraction, GainEnd);
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
		// The ring sends out a pulse, and the ring and number flash.
		FlashTime = FlashDuration;
		PaintBadge(1.f);
		PulseTime = PulseDuration;
		BadgePulse->SetRenderScale(FVector2D(1.f));
		BadgePulse->SetRenderOpacity(1.f);
		BadgePulse->SetVisibility(ESlateVisibility::HitTestInvisible);
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

void UHudXPBarWidget::PaintBar(float Fraction, float GainEnd)
{
	const float Fill = ToBarStep(Fraction);
	const float Gained = FMath::Max(Fill, ToBarStep(GainEnd));
	const int32 Halves = FilledSlots.Num();
	if ((Fill == ShownFill && Gained == ShownGainEnd) || Halves == 0 || GainedSlots.Num() != Halves || RestSlots.Num() != Halves
		|| FillEdges.Num() != Halves)
	{
		return;
	}
	ShownFill = Fill;
	ShownGainEnd = Gained;
	for (int32 Half = 0; Half < Halves; ++Half)
	{
		// Each half shows its share of the bar (the left one 0 to 0.5, the right one 0.5 to 1) as its own 0-1, in which the
		// earned part, the just-earned stretch and the rest share its width.
		const float HalfFill = FMath::Clamp(Fill * Halves - Half, 0.f, 1.f);
		const float HalfGained = FMath::Clamp(Gained * Halves - Half, 0.f, 1.f);
		FilledSlots[Half]->SetSize(ShareOfBar(HalfFill));
		GainedSlots[Half]->SetSize(ShareOfBar(HalfGained - HalfFill));
		RestSlots[Half]->SetSize(ShareOfBar(1.f - HalfGained));
		// The leading edge shows where the earned part ends: in the last half it reaches into.
		const bool bLeads = HalfFill > 0.f && (Half + 1 == Halves || Fill * Halves <= Half + 1);
		FillEdges[Half]->SetVisibility(bLeads ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	// The just-earned stretch is light too, so the notches over it cut dark like those over the earned part.
	PaintTicks(Gained);
}

void UHudXPBarWidget::PaintBadge(float Flash)
{
	// At rest a light number in an accent ring; just after a level-up the two swap (a light ring round an orange number)
	// and ease back.
	const float Amount = FMath::Clamp(Flash, 0.f, 1.f);
	LevelValue->SetColorAndOpacity(FSlateColor(FMath::Lerp(Color::Text(), Color::Accent(), Amount)));
	BadgeRing->SetColorAndOpacity(FMath::Lerp(Color::Accent(), Color::Text(), Amount));
}

void UHudXPBarWidget::PaintEdge(float Glow)
{
	// A lighter accent at rest, white while the bar takes in a gain.
	const FLinearColor EdgeColor = FMath::Lerp(Hex(255, 214, 150), FLinearColor::White, FMath::Clamp(Glow, 0.f, 1.f));
	for (UImage* Edge : FillEdges)
	{
		Edge->SetColorAndOpacity(EdgeColor);
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
		: FString::Printf(TEXT("%s / %s XP"), *FormatXP(Subsystem->GetXP()), *FormatXP(Subsystem->GetXPToNextLevel()))));
	XPText->SetColorAndOpacity(FSlateColor(Subsystem->IsMaxLevel() ? Color::Accent() : Color::TextDim()));
}

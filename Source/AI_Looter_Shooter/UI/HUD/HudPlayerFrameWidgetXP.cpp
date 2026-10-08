// UHudPlayerFrameWidget's experience bar and level gem, following the player's progress (UPlayerProgressionSubsystem's
// events), and the frame's tick: the bar catching up, the just-earned stretch, the level-up's gem, ring and announcement,
// and the idle fade. Health is in HudPlayerFrameWidget.cpp.

#include "UI/HUD/HudPlayerFrameWidget.h"
#include "UI/HUD/HudPortraitWidget.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"

using namespace LooterUI;

namespace
{
	/** How fast the bar eases to a new amount (per second, of the remaining gap), and its slowest speed in levels per second. */
	constexpr double XPEaseRate = 5.0;
	constexpr double XPMinEaseSpeed = 0.25;
	/** After a gain the new stretch shows on its own this long before the bar starts catching up (a bar already moving keeps going). */
	constexpr float XPGainWait = 0.35f;
	/** The just-earned stretch stays white at least this long after the last gain and until the bar has caught up, then fades over this long. */
	constexpr float XPGainHoldSeconds = 0.825f;
	constexpr float XPGainFadeSeconds = 0.675f;
	/** Over the part the fill has already covered the white is a little thinner, so the fill catching up still reads. */
	constexpr float XPGlowStrength = 0.8f;
	/** The "+160 XP" floats this long. */
	constexpr float XPFloatSeconds = 1.4f;
	/** The gem flashes white and fades over this long; its ring spreads to this size over this long, fading. */
	constexpr float GemFlashSeconds = 0.9f;
	constexpr float GemFlashStrength = 0.75f;
	constexpr float GemRingSeconds = 1.1f;
	constexpr float RingScale = 2.6f;
	constexpr float RingStartOpacity = 0.95f;

	FString FormatXP(int64 Value)
	{
		return FText::AsNumber(Value).ToString();
	}

	float XPEaseOut(float Alpha)
	{
		return FMath::InterpEaseOut(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f), 2.f);
	}
}

bool UHudPlayerFrameWidget::BindProgression()
{
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
			Subsystem->OnXPChanged.AddUObject(this, &UHudPlayerFrameWidget::HandleXPChanged);
			Subsystem->OnLevelUp.AddUObject(this, &UHudPlayerFrameWidget::HandleLevelUp);
		}
		Progression = Subsystem;
	}
	return Subsystem != nullptr;
}

void UHudPlayerFrameWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindProgression();
	Retarget(true);
}

void UHudPlayerFrameWidget::NativeDestruct()
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

void UHudPlayerFrameWidget::HandleLevelUp(int32 NewLevel)
{
	PendingAnnouncement = FMath::Max(PendingAnnouncement, NewLevel);
}

void UHudPlayerFrameWidget::HandleXPChanged(int64 Gained, EXPSource Source)
{
	if (!XPFloat)
	{
		return;
	}
	// Several kills in quick succession add up into one "+XP".
	if (Gained > 0)
	{
		ShownGain = XPFloatTime > 0.f ? ShownGain + Gained : Gained;
		XPFloat->SetText(FText::FromString(FString::Printf(TEXT("+%s XP"), *FormatXP(ShownGain))));
		XPFloat->SetVisibility(ESlateVisibility::HitTestInvisible);
		XPFloatTime = XPFloatSeconds;
		PaintFloat(XPFloat, 0.f, XPFloatSeconds);
	}
	Activity = ActivityHoldSeconds;

	// Earned experience eases in; a level set directly (console, reset) jumps there.
	const bool bWasResting = ShownProgress >= TargetProgress;
	Retarget(Gained <= 0);
	if (Gained > 0 && ShownProgress < TargetProgress)
	{
		// The new stretch shows white from where the bar is now; one still showing keeps its start, so a burst of kills
		// reads as one stretch.
		if (GainAlpha <= 0.f || GainStart < 0.f)
		{
			GainStart = static_cast<float>(ShownProgress - FMath::FloorToDouble(ShownProgress));
		}
		GainAlpha = 1.f;
		GainAge = 0.f;
		GainFade = 0.f;
		TintStretch(EHudFrameStretch::XPGain, FLinearColor::White);
		TintStretch(EHudFrameStretch::XPGlow, FLinearColor::White.CopyWithNewOpacity(XPGlowStrength));
		if (bWasResting)
		{
			GainHoldTime = XPGainWait;
		}
		ShowProgress();
	}
}

void UHudPlayerFrameWidget::Retarget(bool bSnap)
{
	const UPlayerProgressionSubsystem* Subsystem = Progression.Get();
	if (!Subsystem || !XPValue)
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
		GainStart = -1.f;
		GainAlpha = 0.f;
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
	// Whatever level this jumped to (a snap, or the skip over a huge gain's levels) shows quietly: only a level the bar
	// reaches by easing across it is celebrated (NativeTick).
	ShowProgress(/*bCelebrate*/ false);
}

void UHudPlayerFrameWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!Cluster)
	{
		return;
	}
	// The progression subsystem wasn't there when the frame was made (a local player not set up yet): bind as soon as it is,
	// showing the progress as it stands (no level-up to celebrate).
	if (!Progression.IsValid() && BindProgression())
	{
		Retarget(true);
	}

	const bool bEasing = ShownProgress < TargetProgress;
	if (GainHoldTime > 0.f)
	{
		GainHoldTime = FMath::Max(0.f, GainHoldTime - InDeltaTime);
	}
	else if (bEasing)
	{
		const double Gap = TargetProgress - ShownProgress;
		const double Step = FMath::Max(Gap * XPEaseRate, XPMinEaseSpeed) * InDeltaTime;
		ShownProgress = Gap <= Step ? TargetProgress : ShownProgress + Step;
		ShowProgress();
	}

	if (GainAlpha > 0.f)
	{
		// White while the bar catches up and a moment after, then it fades, showing the cyan the bar now holds.
		GainAge += InDeltaTime;
		if (ShownProgress >= TargetProgress && GainAge >= XPGainHoldSeconds)
		{
			GainFade += InDeltaTime;
		}
		const float Alpha = 1.f - FMath::Clamp(GainFade / XPGainFadeSeconds, 0.f, 1.f);
		if (Alpha != GainAlpha)
		{
			GainAlpha = Alpha;
			TintStretch(EHudFrameStretch::XPGain, FLinearColor::White.CopyWithNewOpacity(GainAlpha));
			TintStretch(EHudFrameStretch::XPGlow, FLinearColor::White.CopyWithNewOpacity(GainAlpha * XPGlowStrength));
			if (GainAlpha <= 0.f)
			{
				GainStart = -1.f;
				ShowProgress();
			}
		}
	}

	TickXPEffects(InDeltaTime);

	// Full health and nothing happening: step back. Hurt, or something just happened: full strength.
	Activity = FMath::Max(0.f, Activity - InDeltaTime);
	const float Wanted = Activity > 0.f || (LastFraction >= 0.f && LastFraction < 0.999f) ? 1.f : IdleOpacity;
	const float Opacity = Cluster->GetRenderOpacity();
	if (!FMath::IsNearlyEqual(Opacity, Wanted, 0.002f))
	{
		const float Next = FMath::FInterpTo(Opacity, Wanted, InDeltaTime, 5.f);
		Cluster->SetRenderOpacity(FMath::IsNearlyEqual(Next, Wanted, 0.005f) ? Wanted : Next);
	}
}

bool UHudPlayerFrameWidget::TickXPEffects(float DeltaTime)
{
	if (!XPFloat || !GemFlash || !GemRing)
	{
		return false;
	}
	if (XPFloatTime > 0.f)
	{
		XPFloatTime -= DeltaTime;
		if (XPFloatTime <= 0.f)
		{
			XPFloat->SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			PaintFloat(XPFloat, XPFloatSeconds - XPFloatTime, XPFloatSeconds);
		}
	}
	if (GemFlashTime > 0.f)
	{
		// Bright at once, settling quickly.
		GemFlashTime = FMath::Max(0.f, GemFlashTime - DeltaTime);
		GemFlash->SetRenderOpacity(GemFlashStrength * (1.f - XPEaseOut(1.f - GemFlashTime / GemFlashSeconds)));
		if (GemFlashTime <= 0.f)
		{
			GemFlash->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	if (RingTime > 0.f)
	{
		// The ring spreads quickly, slowing as it fades out.
		RingTime = FMath::Max(0.f, RingTime - DeltaTime);
		const float Spread = XPEaseOut(1.f - RingTime / GemRingSeconds);
		GemRing->SetRenderScale(FVector2D(FMath::Lerp(1.f, RingScale, Spread)));
		GemRing->SetRenderOpacity(RingStartOpacity * (1.f - Spread));
		if (RingTime <= 0.f)
		{
			GemRing->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	return XPFloatTime > 0.f || GemFlashTime > 0.f || RingTime > 0.f;
}

void UHudPlayerFrameWidget::ShowProgress(bool bCelebrate)
{
	if (StretchBodies.Num() != static_cast<int32>(EHudFrameStretch::Count))
	{
		return;
	}
	// At the top level the bar stays full once it gets there.
	const bool bFull = bMaxLevel && ShownProgress >= TargetProgress;
	const double Level = FMath::FloorToDouble(ShownProgress);
	SetShownLevel(static_cast<int32>(Level), bCelebrate);
	const float Fill = bFull ? 1.f : static_cast<float>(ShownProgress - Level);

	// The stretch still to catch up to runs ahead to the target, or to the bar's end while the target is in a later level
	// (after the wrap it starts again from the left).
	float GainEnd = Fill;
	if (ShownProgress < TargetProgress)
	{
		const double TargetLevel = FMath::FloorToDouble(TargetProgress);
		GainEnd = TargetLevel > Level ? 1.f : static_cast<float>(TargetProgress - TargetLevel);
	}

	// White ahead of the fill (under it), the fill, the white over what the fill has covered since the gain began, and the
	// fill as it was before the gain laid back over that, so the glow starts where the gain did.
	const bool bGain = GainAlpha > 0.f;
	PaintStretch(EHudFrameStretch::XPGain, bGain && GainEnd > Fill ? GainEnd : 0.f);
	PaintStretch(EHudFrameStretch::XPFill, Fill);
	const bool bGlow = bGain && GainStart >= 0.f && Fill > GainStart;
	PaintStretch(EHudFrameStretch::XPGlow, bGlow ? Fill : 0.f);
	PaintStretch(EHudFrameStretch::XPBefore, bGlow ? GainStart : 0.f);
}

void UHudPlayerFrameWidget::SetShownLevel(int32 Level, bool bCelebrate)
{
	if (Level == ShownLevel || !LevelText || !GemFlash || !GemRing)
	{
		return;
	}
	// A level the bar eased across: celebrated. One it was set to without filling up to it shows its number and nothing more.
	const bool bWrapped = bCelebrate && ShownLevel != INDEX_NONE && Level > ShownLevel;
	ShownLevel = Level;
	LevelText->SetText(FText::AsNumber(Level));

	if (bWrapped)
	{
		// The bar got to the new level: the gem flashes and sends out a ring, the portrait's eyes flare, and the HUD hears
		// of it to show the banner. The just-earned stretch carries on from the new level's start.
		GemFlashTime = GemFlashSeconds;
		GemFlash->SetRenderOpacity(GemFlashStrength);
		GemFlash->SetVisibility(ESlateVisibility::HitTestInvisible);
		RingTime = GemRingSeconds;
		GemRing->SetRenderScale(FVector2D(1.f));
		GemRing->SetRenderOpacity(RingStartOpacity);
		GemRing->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (GainStart >= 0.f)
		{
			GainStart = 0.f;
		}
		if (Portrait)
		{
			Portrait->PlayLevelUp();
		}
		if (PendingAnnouncement > 0 && Level >= PendingAnnouncement)
		{
			const int32 Announced = PendingAnnouncement;
			PendingAnnouncement = 0;
			OnLevelUp.ExecuteIfBound(Announced);
		}
	}
	if (bXPTextPending && Progression.IsValid() && Level >= Progression->GetLevel())
	{
		bXPTextPending = false;
		UpdateXPText();
	}
}

void UHudPlayerFrameWidget::UpdateXPText()
{
	const UPlayerProgressionSubsystem* Subsystem = Progression.Get();
	if (!Subsystem || !XPValue || !XPMax)
	{
		return;
	}
	if (Subsystem->IsMaxLevel())
	{
		XPValue->SetText(FText::FromString(TEXT("MAX")));
		XPValue->SetColorAndOpacity(FSlateColor(Color::Accent()));
		XPMax->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	XPValue->SetText(FText::FromString(FormatXP(Subsystem->GetXP())));
	XPValue->SetColorAndOpacity(FSlateColor(Color::Text()));
	XPMax->SetText(FText::FromString(FString::Printf(TEXT("/ %s XP"), *FormatXP(Subsystem->GetXPToNextLevel()))));
	XPMax->SetVisibility(ESlateVisibility::HitTestInvisible);
}

// The boss bar's motion: the intro (the drop, the fill running up, the title spelled out), a hit's flash and a big hit's
// jolt, a new phase's flash, the callouts and the boss's death. HudBossBarWidget.cpp has what it shows and its colors.
#include "UI/HUD/HudBossBarWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "LooterBossBar"

namespace
{
	/** The intro: the bar drops this far (px) into place over DropSeconds; its fill runs up from SweepStart over SweepSeconds. */
	constexpr float BossIntroDrop = 26.f;
	constexpr float BossIntroDropSeconds = 0.35f;
	constexpr float BossIntroSweepStart = 0.15f;
	constexpr float BossIntroSweepSeconds = 0.9f;
	/** The title is spelled out from TypeStart, this many letters a second. */
	constexpr float BossTitleTypeStart = 0.35f;
	constexpr float BossTitleLettersPerSecond = 26.f;
	/** Once the title gives the line to the phase's name, the name flashes and pops this long (s; a new phase's is 1.6). */
	constexpr float BossTitleHandOverFlash = 0.96f;

	/** How quickly a hit's flash and a new phase's flash fade, and a jolt settles (a second's share). */
	constexpr float BossHitFlashFade = 3.5f;
	constexpr float BossBarFlashFade = 1.4f;
	constexpr float BossJoltFade = 5.f;
	/** A jolt shakes the bar sideways this far (px), this fast (radians a second). */
	constexpr float BossJoltPixels = 4.f;
	constexpr float BossJoltSpeed = 95.f;
	/** A callout's least time on the line (s). */
	constexpr float BossShortestCallout = 0.3f;
}

FText UHudBossBarWidget::MakeIntroTitle(const FText& InTitle, float Time)
{
	const FString Upper = InTitle.ToUpper().ToString();
	const int32 Letters = FMath::Clamp(FMath::FloorToInt32((Time - BossTitleTypeStart) * BossTitleLettersPerSecond), 0, Upper.Len());
	return FText::FromString(Upper.Left(Letters));
}

float UHudBossBarWidget::GetIntroSweep() const
{
	if (IntroTime < 0.f)
	{
		return 1.f;
	}
	return FMath::SmoothStep(0.f, 1.f, FMath::Clamp((IntroTime - BossIntroSweepStart) / BossIntroSweepSeconds, 0.f, 1.f));
}

bool UHudBossBarWidget::IsTitleShowing() const
{
	return IntroTime >= 0.f && !Title.IsEmpty();
}

FText UHudBossBarWidget::CurrentLine() const
{
	if (bDefeated || CalloutLeft > 0.f)
	{
		return Callout.ToUpper();
	}
	if (IsTitleShowing())
	{
		return MakeIntroTitle(Title, IntroTime);
	}
	return MakePhaseLine(PhaseName, bGreyed, Hint);
}

void UHudBossBarWidget::SetTitle(const FText& InTitle)
{
	Title = InTitle;
	ShownTitleChars = INDEX_NONE;
	ApplyTexts();
}

void UHudBossBarWidget::PlayIntro()
{
	IntroTime = 0.f;
	ShownTitleChars = INDEX_NONE;
	HitFlash = 0.f;
	Jolt = 0.f;
	BarFlash = 0.f;
	ShownFillWidth = -1.f;
	ShownChipWidth = -1.f;
	ApplyFill();
	ApplyTexts();
	ApplyColors();
}

void UHudBossBarWidget::ShowCallout(const FText& InCallout, float Seconds)
{
	if (bDefeated || InCallout.IsEmpty())
	{
		return;
	}
	Callout = InCallout;
	CalloutSeconds = FMath::Max(Seconds, BossShortestCallout);
	CalloutLeft = CalloutSeconds;
	ApplyTexts();
	ApplyColors();
}

void UHudBossBarWidget::PlayDefeated(const FText& InLine)
{
	bDefeated = true;
	Callout = InLine.IsEmpty() ? LOCTEXT("Defeated", "Defeated") : InLine;
	CalloutLeft = 0.f;
	IntroTime = -1.f;
	BarFlash = 1.f;
	HitFlash = 1.f;
	Jolt = 1.f;
	ShownFillWidth = -1.f;
	ApplyFill();
	ApplyTexts();
	ApplyColors();
}

void UHudBossBarWidget::TickMotion(float DeltaSeconds)
{
	MotionClock += DeltaSeconds;
	bool bFill = false;
	bool bColors = false;
	bool bTexts = false;

	if (IntroTime >= 0.f)
	{
		const float Before = IntroTime;
		IntroTime += DeltaSeconds;
		// The fill runs up until its sweep is done.
		bFill = Before < BossIntroSweepStart + BossIntroSweepSeconds;
		const int32 Letters = MakeIntroTitle(Title, IntroTime).ToString().Len();
		if (Letters != ShownTitleChars)
		{
			ShownTitleChars = Letters;
			bTexts = true;
		}
		if (IntroTime >= IntroSeconds)
		{
			// The title gives the line to the phase's name, which pops.
			IntroTime = -1.f;
			if (!Title.IsEmpty())
			{
				PhaseFlash = FMath::Max(PhaseFlash, BossTitleHandOverFlash);
			}
			bFill = bTexts = bColors = true;
		}
	}
	if (CalloutLeft > 0.f)
	{
		CalloutLeft = FMath::Max(0.f, CalloutLeft - DeltaSeconds);
		bColors = true;
		bTexts |= CalloutLeft <= 0.f;
	}
	if (HitFlash > 0.f)
	{
		HitFlash = FMath::Max(0.f, HitFlash - DeltaSeconds * BossHitFlashFade);
		bColors = true;
	}
	if (BarFlash > 0.f)
	{
		BarFlash = FMath::Max(0.f, BarFlash - DeltaSeconds * BossBarFlashFade);
		bColors = true;
	}
	Jolt = FMath::Max(0.f, Jolt - DeltaSeconds * BossJoltFade);

	// The intro's drop into place (eased out) and a jolt's shake, as one offset of the whole readout.
	const float Drop = IntroTime >= 0.f ? 1.f - FMath::Clamp(IntroTime / BossIntroDropSeconds, 0.f, 1.f) : 0.f;
	const FVector2D Offset(FMath::Sin(MotionClock * BossJoltSpeed) * BossJoltPixels * Jolt * Jolt, -BossIntroDrop * Drop * Drop);
	if (Cluster && !Offset.Equals(ShownOffset, 0.05))
	{
		ShownOffset = Offset;
		Cluster->SetRenderTranslation(Offset);
	}

	if (bTexts)
	{
		ApplyTexts();
	}
	if (bFill)
	{
		ApplyFill();
	}
	if (bColors)
	{
		ApplyColors();
	}
}

#undef LOCTEXT_NAMESPACE

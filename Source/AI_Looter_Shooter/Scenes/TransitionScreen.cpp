#include "Scenes/TransitionScreen.h"

namespace
{
	/** Quick at first, settling into place: how the title rises and its line draws out. */
	float EaseOut(float Alpha)
	{
		const float Clamped = FMath::Clamp(Alpha, 0.f, 1.f);
		return 1.f - (1.f - Clamped) * (1.f - Clamped) * (1.f - Clamped);
	}

	/** Gentle at both ends: how the white comes and goes. */
	float Smooth(float Alpha)
	{
		return FMath::SmoothStep(0.f, 1.f, Alpha);
	}
}

void FTransitionScreen::Drive(float InWhite)
{
	const float Clamped = FMath::Clamp(InWhite, 0.f, 1.f);
	if (Clamped <= 0.f)
	{
		Clear();
		return;
	}
	Phase = ETransitionPhase::Driven;
	White = Clamped;
	Clock = 0.f;
	Unattended = 0.f;
	bTitle = false;
}

void FTransitionScreen::FadeIn(float Seconds)
{
	if (Seconds <= 0.f)
	{
		Hold();
		return;
	}
	FadeFrom = GetWhite();
	FadeSeconds = Seconds;
	Phase = ETransitionPhase::Fading;
	Clock = 0.f;
	Unattended = 0.f;
	bTitle = false;
}

void FTransitionScreen::Hold()
{
	Phase = ETransitionPhase::Held;
	White = 1.f;
	Clock = 0.f;
	Unattended = 0.f;
	bTitle = false;
}

void FTransitionScreen::Reveal(bool bWithTitle)
{
	// From whatever white there is now: a reveal over a reveal carries on from where the first one had got to.
	const float From = GetWhite();
	if (From <= 0.f && !bWithTitle)
	{
		Clear();
		return;
	}
	Phase = ETransitionPhase::Revealing;
	White = From;
	Clock = 0.f;
	Unattended = 0.f;
	bTitle = bWithTitle;
}

void FTransitionScreen::Clear()
{
	Phase = ETransitionPhase::Clear;
	White = 0.f;
	Clock = 0.f;
	Unattended = 0.f;
	bTitle = false;
}

bool FTransitionScreen::Advance(float DeltaSeconds)
{
	const float Step = FMath::Clamp(DeltaSeconds, 0.f, LongestStep);
	switch (Phase)
	{
	case ETransitionPhase::Fading:
		Clock += Step;
		if (Clock >= FadeSeconds)
		{
			Hold();
		}
		return false;
	case ETransitionPhase::Driven:
	case ETransitionPhase::Held:
		// Never white for good: whatever was meant to reveal it (an arrival, the end of a scene) didn't.
		Unattended += Step;
		if (Unattended >= HoldTimeout)
		{
			Reveal(false);
			return true;
		}
		return false;
	case ETransitionPhase::Revealing:
		Clock += Step;
		if (Clock >= RevealSeconds(bTitle))
		{
			Clear();
		}
		return false;
	case ETransitionPhase::Clear:
		break;
	}
	return false;
}

bool FTransitionScreen::IsShowing() const
{
	return Phase != ETransitionPhase::Clear;
}

float FTransitionScreen::GetWhite() const
{
	switch (Phase)
	{
	case ETransitionPhase::Driven:
	case ETransitionPhase::Held:
		return White;
	case ETransitionPhase::Fading:
		return FMath::Lerp(FadeFrom, 1.f, Smooth(Clock / FadeSeconds));
	case ETransitionPhase::Revealing:
		return White * (1.f - Smooth((Clock - ThinStart()) / ThinDuration()));
	case ETransitionPhase::Clear:
		break;
	}
	return 0.f;
}

float FTransitionScreen::GetTitleOpacity() const
{
	if (Phase != ETransitionPhase::Revealing || !bTitle)
	{
		return 0.f;
	}
	const float Risen = EaseOut((Clock - TitleDelay) / TitleRiseSeconds);
	const float Faded = Smooth((Clock - ThinStart() - TitleFadeDelay) / TitleFadeSeconds);
	return Risen * (1.f - Faded);
}

float FTransitionScreen::GetTitleRise() const
{
	if (Phase != ETransitionPhase::Revealing || !bTitle)
	{
		return 1.f;
	}
	const float Risen = EaseOut((Clock - TitleDelay) / TitleRiseSeconds);
	const float Faded = Smooth((Clock - ThinStart() - TitleFadeDelay) / TitleFadeSeconds);
	// Up into its place, then on up a little as it fades with the white.
	return (1.f - Risen) - 0.35f * Faded;
}

float FTransitionScreen::GetTitleLine() const
{
	if (Phase != ETransitionPhase::Revealing || !bTitle)
	{
		return 0.f;
	}
	// It starts drawing out halfway up the title's rise.
	return EaseOut((Clock - TitleDelay - 0.5f * TitleRiseSeconds) / TitleRiseSeconds);
}

float FTransitionScreen::RevealSeconds(bool bWithTitle)
{
	if (!bWithTitle)
	{
		return PlainDelay + PlainThinSeconds;
	}
	const float Thinning = TitleDelay + TitleRiseSeconds + TitleHoldSeconds;
	return FMath::Max(Thinning + ThinSeconds, Thinning + TitleFadeDelay + TitleFadeSeconds);
}

float FTransitionScreen::ThinStart() const
{
	return bTitle ? TitleDelay + TitleRiseSeconds + TitleHoldSeconds : PlainDelay;
}

float FTransitionScreen::ThinDuration() const
{
	return bTitle ? ThinSeconds : PlainThinSeconds;
}

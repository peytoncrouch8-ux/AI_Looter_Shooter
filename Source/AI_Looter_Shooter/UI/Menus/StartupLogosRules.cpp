#include "UI/Menus/StartupLogosRules.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<float> CVarLeadIn(TEXT("Looter.Startup.LeadIn"), 0.5f,
		TEXT("Seconds of dark before the first startup card (the window and the menu's world settle)."));
	TAutoConsoleVariable<float> CVarFadeIn(TEXT("Looter.Startup.FadeIn"), 0.6f, TEXT("Seconds a startup card takes to fade in."));
	TAutoConsoleVariable<float> CVarHold(TEXT("Looter.Startup.Hold"), 2.2f, TEXT("Seconds a startup card holds at full."));
	TAutoConsoleVariable<float> CVarFadeOut(TEXT("Looter.Startup.FadeOut"), 0.5f, TEXT("Seconds a startup card takes to fade out."));
	TAutoConsoleVariable<float> CVarGap(TEXT("Looter.Startup.Gap"), 0.3f, TEXT("Seconds of dark between two startup cards."));
	TAutoConsoleVariable<float> CVarSkipFade(TEXT("Looter.Startup.SkipFade"), 0.2f,
		TEXT("Seconds a startup card cut short by a key takes to fade out."));
	TAutoConsoleVariable<float> CVarMinShown(TEXT("Looter.Startup.MinShown"), 0.35f,
		TEXT("Seconds a startup card shows before a key can skip it (a key held from launching never skips two)."));
	TAutoConsoleVariable<float> CVarReveal(TEXT("Looter.Startup.Reveal"), 0.9f,
		TEXT("Seconds the dark cover takes to fade off the main menu after the last startup card."));

	float Smooth(float Alpha)
	{
		return FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f));
	}

	float Read(const TAutoConsoleVariable<float>& Variable)
	{
		return FMath::Max(Variable.GetValueOnGameThread(), 0.f);
	}
}

FStartupLogoTiming FStartupLogoTiming::FromConsole()
{
	FStartupLogoTiming Result;
	Result.LeadIn = Read(CVarLeadIn);
	Result.FadeIn = Read(CVarFadeIn);
	Result.Hold = Read(CVarHold);
	Result.FadeOut = Read(CVarFadeOut);
	Result.Gap = Read(CVarGap);
	Result.SkipFadeOut = Read(CVarSkipFade);
	Result.MinShown = Read(CVarMinShown);
	Result.Reveal = Read(CVarReveal);
	return Result;
}

float FStartupLogos::FullLength(int32 InNumCards, const FStartupLogoTiming& InTiming)
{
	const int32 Cards = FMath::Max(InNumCards, 0);
	const float PerCard = InTiming.FadeIn + InTiming.Hold + InTiming.FadeOut;
	return (Cards > 0 ? InTiming.LeadIn : 0.f) + Cards * PerCard + FMath::Max(Cards - 1, 0) * InTiming.Gap + InTiming.Reveal;
}

void FStartupLogos::Start(int32 InNumCards, bool bInSkippable, const FStartupLogoTiming& InTiming)
{
	Timing = InTiming;
	NumCards = FMath::Max(InNumCards, 0);
	bSkippable = bInSkippable;
	Card = INDEX_NONE;
	StartedCard = INDEX_NONE;
	CardTime = 0.f;
	if (NumCards > 0)
	{
		Card = 0;
		Enter(EStartupLogoPhase::LeadIn);
	}
	else
	{
		Enter(EStartupLogoPhase::Reveal);
	}
	// A zero lead-in (or a zero-length cover) moves on at once.
	Tick(0.f);
}

void FStartupLogos::Enter(EStartupLogoPhase NewPhase)
{
	Phase = NewPhase;
	PhaseTime = 0.f;
	if (NewPhase == EStartupLogoPhase::FadeIn)
	{
		CardTime = 0.f;
		bCutShort = false;
		StartedCard = Card;
	}
}

float FStartupLogos::PhaseLength() const
{
	switch (Phase)
	{
	case EStartupLogoPhase::LeadIn: return Timing.LeadIn;
	case EStartupLogoPhase::FadeIn: return Timing.FadeIn;
	case EStartupLogoPhase::Hold: return Timing.Hold;
	case EStartupLogoPhase::FadeOut: return bCutShort ? Timing.SkipFadeOut : Timing.FadeOut;
	case EStartupLogoPhase::Gap: return Timing.Gap;
	case EStartupLogoPhase::Reveal: return Timing.Reveal;
	default: return 0.f;
	}
}

void FStartupLogos::AfterCard()
{
	if (Card + 1 < NumCards)
	{
		++Card;
		Enter(EStartupLogoPhase::Gap);
	}
	else
	{
		Card = INDEX_NONE;
		Enter(EStartupLogoPhase::Reveal);
	}
}

void FStartupLogos::Tick(float DeltaSeconds)
{
	PhaseTime += FMath::Max(DeltaSeconds, 0.f);
	if (Phase == EStartupLogoPhase::FadeIn || Phase == EStartupLogoPhase::Hold || Phase == EStartupLogoPhase::FadeOut)
	{
		CardTime += FMath::Max(DeltaSeconds, 0.f);
	}
	// A long frame may cross several phases (zero-length ones too); each takes its own share of the time.
	while (Phase != EStartupLogoPhase::Done && PhaseTime >= PhaseLength())
	{
		const float Over = PhaseTime - PhaseLength();
		switch (Phase)
		{
		case EStartupLogoPhase::LeadIn:
		case EStartupLogoPhase::Gap:
			Enter(EStartupLogoPhase::FadeIn);
			break;
		case EStartupLogoPhase::FadeIn:
			Enter(EStartupLogoPhase::Hold);
			break;
		case EStartupLogoPhase::Hold:
			FadeFrom = 1.f;
			Enter(EStartupLogoPhase::FadeOut);
			break;
		case EStartupLogoPhase::FadeOut:
			AfterCard();
			break;
		case EStartupLogoPhase::Reveal:
			Enter(EStartupLogoPhase::Done);
			break;
		default:
			break;
		}
		PhaseTime = Over;
	}
}

bool FStartupLogos::Skip()
{
	if (!bSkippable)
	{
		return false;
	}
	switch (Phase)
	{
	case EStartupLogoPhase::FadeIn:
	case EStartupLogoPhase::Hold:
		if (CardTime < Timing.MinShown)
		{
			return false;
		}
		// From wherever the fade-in had got to, quickly out.
		FadeFrom = GetCardOpacity();
		Enter(EStartupLogoPhase::FadeOut);
		bCutShort = true;
		Tick(0.f);
		return true;
	case EStartupLogoPhase::Gap:
		// The card coming is skipped before it shows: the gap carries on to the one after, or the cover goes.
		if (Card + 1 < NumCards)
		{
			++Card;
		}
		else
		{
			Card = INDEX_NONE;
			Enter(EStartupLogoPhase::Reveal);
			Tick(0.f);
		}
		return true;
	default:
		// The lead-in (a key held from launching), a card already going, and the cover's fade take no key.
		return false;
	}
}

int32 FStartupLogos::GetCard() const
{
	return Phase == EStartupLogoPhase::Reveal || Phase == EStartupLogoPhase::Done ? INDEX_NONE : Card;
}

float FStartupLogos::GetCardOpacity() const
{
	const float Length = PhaseLength();
	const float Alpha = Length > 0.f ? PhaseTime / Length : 1.f;
	switch (Phase)
	{
	case EStartupLogoPhase::FadeIn: return Smooth(Alpha);
	case EStartupLogoPhase::Hold: return 1.f;
	case EStartupLogoPhase::FadeOut: return FadeFrom * (1.f - Smooth(Alpha));
	default: return 0.f;
	}
}

float FStartupLogos::GetCardProgress() const
{
	const float Whole = Timing.FadeIn + Timing.Hold + Timing.FadeOut;
	return Whole > 0.f ? FMath::Clamp(CardTime / Whole, 0.f, 1.f) : 1.f;
}

float FStartupLogos::GetCover() const
{
	if (Phase == EStartupLogoPhase::Done)
	{
		return 0.f;
	}
	if (Phase != EStartupLogoPhase::Reveal)
	{
		return 1.f;
	}
	return Timing.Reveal > 0.f ? 1.f - Smooth(PhaseTime / Timing.Reveal) : 0.f;
}

int32 FStartupLogos::TakeStartedCard()
{
	const int32 Result = StartedCard;
	StartedCard = INDEX_NONE;
	return Result;
}

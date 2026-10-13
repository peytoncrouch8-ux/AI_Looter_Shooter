// BossShrineRules: a vigil stone's rules as plain functions (when it rises, when its boss can be called back, the words).

#include "World/BossShrineRules.h"
#include "Session/SessionSubsystem.h"

#define LOCTEXT_NAMESPACE "LooterBossShrine"

namespace BossShrineRules
{
	bool IsLoadedSince(int32 LoadsAtDefeat, int32 LoadsNow)
	{
		return LoadsNow > LoadsAtDefeat;
	}

	double SecondsLeft(double DefeatedAt, double PlayedNow)
	{
		// Not kept, or kept "after" now (a save whose time played went back): ready rather than waiting it out.
		if (DefeatedAt < 0.0 || PlayedNow < DefeatedAt)
		{
			return 0.0;
		}
		return FMath::Max(CallBackSeconds - (PlayedNow - DefeatedAt), 0.0);
	}

	FVerdict Evaluate(const FInputs& In)
	{
		FVerdict Verdict;
		if (!In.bEverBeaten)
		{
			return Verdict;
		}
		if (In.bBossPresent)
		{
			Verdict.State = EBossShrineState::Present;
			return Verdict;
		}
		if (!In.bStoryAllows)
		{
			Verdict.State = EBossShrineState::NotYet;
			return Verdict;
		}
		// The user's rule: five minutes, or at once after a Save & Quit.
		const double Left = IsLoadedSince(In.LoadsAtDefeat, In.LoadsNow) ? 0.0 : SecondsLeft(In.DefeatedAt, In.PlayedNow);
		Verdict.State = Left > 0.0 ? EBossShrineState::Waiting : EBossShrineState::Ready;
		Verdict.SecondsLeft = Left;
		return Verdict;
	}

	bool ComesBackOnLoad(bool bAway, double DefeatedAt, int32 LoadsAtDefeat, int32 LoadsNow)
	{
		return bAway && (DefeatedAt < 0.0 || IsLoadedSince(LoadsAtDefeat, LoadsNow));
	}

	bool ShouldRise(bool bEverBeaten, bool bRisen, bool bScenePlaying, bool bBossPresent, float SinceDefeat, float PlayerDistance,
		float Radius)
	{
		return bEverBeaten && !bRisen && !bScenePlaying && !bBossPresent && SinceDefeat >= RiseDelay && PlayerDistance <= Radius;
	}

	FText Countdown(double Seconds)
	{
		const int32 Whole = FMath::Max(FMath::CeilToInt(Seconds - KINDA_SMALL_NUMBER), 0);
		return FText::FromString(FString::Printf(TEXT("%d:%02d"), Whole / 60, Whole % 60));
	}

	FText StatusText(EBossShrineState State, const FText& BossName, double SecondsLeft)
	{
		switch (State)
		{
		case EBossShrineState::Ready:
			return FText::Format(LOCTEXT("StatusReady", "{0} rests. Ring the bell and it rises to fight again."), BossName);
		case EBossShrineState::Waiting:
			return FText::Format(LOCTEXT("StatusWaiting", "Laid to rest too lately. The bell answers in {0}."), Countdown(SecondsLeft));
		case EBossShrineState::Present:
			return FText::Format(LOCTEXT("StatusPresent", "{0} walks again. Lay it to rest first."), BossName);
		case EBossShrineState::NotYet:
			return LOCTEXT("StatusNotYet", "Not yet. The dead here aren't done with you.");
		case EBossShrineState::Dormant:
		default:
			return FText::GetEmpty();
		}
	}

	FText LastKillText(double DefeatedAt, double PlayedNow, const FDateTime& DefeatedOn, const FDateTime& Now, bool bThisLoad)
	{
		if (DefeatedAt < 0.0)
		{
			return LOCTEXT("LastKillUnknown", "Before the stones kept count");
		}
		const FText Clock = FText::FromString(USessionSubsystem::FormatSavedTime(DefeatedOn, Now));
		if (!bThisLoad || PlayedNow < DefeatedAt)
		{
			return Clock;
		}
		const FText Ago = FText::FromString(USessionSubsystem::FormatPlayTime(PlayedNow - DefeatedAt));
		return FText::Format(LOCTEXT("LastKillAgo", "{0}, {1} of play ago"), Clock, Ago);
	}
}

#undef LOCTEXT_NAMESPACE

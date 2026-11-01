#include "Bosses/AbelCorruptionRules.h"

namespace AbelCorruptionRules
{
	EBreak TendrilBreaks(const FTendrilFacts& Facts)
	{
		if (Facts.bOwnerDown && !Facts.bFinal)
		{
			return EBreak::OwnerDown;
		}
		if (Facts.bVictimGone)
		{
			return EBreak::VictimGone;
		}
		if (Facts.bFinal)
		{
			// The last of the smog fights back through the whole hold: nothing but the hold's end lets it go.
			return EBreak::None;
		}
		if (Facts.bLanternShot)
		{
			return EBreak::LanternShot;
		}
		if (Facts.bStaggered)
		{
			return EBreak::Staggered;
		}
		if (Facts.Distance > TendrilBreakRange)
		{
			return EBreak::Distance;
		}
		if (Facts.LatchedFor >= TendrilLongest)
		{
			return EBreak::Time;
		}
		return EBreak::None;
	}

	bool IsLanternFlaring(float LatchedFor)
	{
		// The first flare comes a second after the bite (time to see the line, then the window), then one every FlareEvery.
		const float Since = LatchedFor - 1.f;
		return Since >= 0.f && FMath::Fmod(Since, TendrilFlareEvery) < TendrilFlareSeconds;
	}

	float CorruptionFor(int32 Filled)
	{
		return 1.f - static_cast<float>(FMath::Clamp(Filled, 0, Pips)) / static_cast<float>(Pips);
	}

	int32 CallBackFor(int32 Phase)
	{
		return Phase >= 1 && Phase <= 2 ? Phase - 1 : INDEX_NONE;
	}

	int32 NextDrink(const TArray<bool>& Drunk, int32 Last)
	{
		const int32 Count = Drunk.Num();
		for (int32 Step = 1; Step <= Count; ++Step)
		{
			const int32 Index = ((Last == INDEX_NONE ? -1 : Last) + Step + Count) % Count;
			if (!Drunk[Index])
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	bool AreSixSaved(int32 Bodies, int32 Drunk)
	{
		return Bodies > 0 && Drunk == 0;
	}

	float StepSurge(float Share, bool bOn, float DeltaSeconds)
	{
		const float Rate = 1.f / FMath::Max(bOn ? SurgeIn : SurgeOut, 0.01f);
		return FMath::FInterpConstantTo(Share, bOn ? 1.f : 0.f, DeltaSeconds, Rate);
	}

	float SurgeDensityAt(float OwnDensity, float Share)
	{
		// Eased, so it thickens slowly then rolls in: the wreck's smog climbing the Rim.
		return FMath::Lerp(OwnDensity, FMath::Max(OwnDensity, SurgeDensity), FMath::SmoothStep(0.f, 1.f, Share));
	}

	float SurgeStartAt(float OwnStart, float Share)
	{
		return FMath::Lerp(OwnStart, FMath::Min(OwnStart, SurgeStart), FMath::SmoothStep(0.f, 1.f, Share));
	}

	FLinearColor SmogTint(const FLinearColor& Own, float Corruption)
	{
		// Soot-black with a hint of the smog's violet: dark enough to read as smog on the dusk deck, never a hole (his rim and
		// the coal still draw him).
		const FLinearColor Smog(0.035f, 0.03f, 0.045f, Own.A);
		return FMath::Lerp(Own, Smog, FMath::Clamp(Corruption, 0.f, 1.f));
	}

	FLinearColor CoalColor(float Corruption)
	{
		// The furnace's orange, burning hot in his black ribs; freed, the saint's light: gold-white, soft.
		const FLinearColor Furnace(1.f, 0.32f, 0.04f, 3.2f);
		const FLinearColor Freed(1.f, 0.86f, 0.58f, 2.2f);
		return Corruption > 0.f ? Furnace : Freed;
	}

	FName SixSavedEvent()
	{
		return FName(TEXT("Abel.SixSaved"));
	}
}

#include "World/RoostRules.h"

#define LOCTEXT_NAMESPACE "Roost"

namespace
{
	/** How many lines Hob has after a death (DeathWords). */
	constexpr int32 DeathLineCount = 12;

	/** Hob after a death, sardonic and secretly kind (Docs/Story.md: he comments every time). Drafts in the doc's voice. */
	FText DeathWords(int32 Index)
	{
		switch (Index)
		{
		case 0:  return LOCTEXT("Death0", "Up you get, sunshine. Dead's no excuse for lying about.");
		case 1:  return LOCTEXT("Death1", "Heavier than you look. My lads want feeding for that one.");
		case 2:  return LOCTEXT("Death2", "The debt's still owed. You don't get to stay down till it's paid.");
		case 3:  return LOCTEXT("Death3", "Mind the feathers. Next time, duck.");
		case 4:  return LOCTEXT("Death4", "Dragged you by the collar the whole way. Don't thank us.");
		case 5:  return LOCTEXT("Death5", "Sexton gives no refunds on the dead, sunshine. On your feet.");
		case 6:  return LOCTEXT("Death6", "The worms can wait. Seven embers can't.");
		case 7:  return LOCTEXT("Death7", "Shake it off. You've been deader than that.");
		case 8:  return LOCTEXT("Death8", "Saw that one coming three fence posts off. Next time, so will you.");
		case 9:  return LOCTEXT("Death9", "Back on your feet, all your bits attached. Mostly.");
		case 10: return LOCTEXT("Death10", "If you're going to keep dying, die nearer a roost. My lads' wings are tired.");
		default: return LOCTEXT("Death11", "Easy. Breathe. Well, pretend to.");
		}
	}

	FStoryLine HobSays(const FText& Words)
	{
		// Long enough to read (FStoryLine's own reading time): he says it as Ellis gets up.
		return FStoryLine::Make(RoostRules::HobName(), Words);
	}
}

namespace RoostRules
{
	float NearFade(float Distance, float Gone, float Full)
	{
		if (Full <= Gone)
		{
			return Distance >= Gone ? 1.f : 0.f;
		}
		return FMath::SmoothStep(Gone, Full, Distance);
	}

	float BeamRise(float Age)
	{
		if (Age < 0.f || Age >= RiseSeconds)
		{
			return 1.f;
		}
		// It shoots up out of the post and slows as it reaches the sky: most of its height in the first half second.
		const float Alpha = Age / RiseSeconds;
		return 1.f - FMath::Pow(1.f - Alpha, 3.f);
	}

	bool PulseAt(float Age, float& OutHeight, float& OutStrength)
	{
		OutHeight = 0.f;
		OutStrength = 0.f;
		if (Age < 0.f || Age >= PulseSeconds)
		{
			return false;
		}
		const float Alpha = Age / PulseSeconds;
		// Climbing a little faster at first, as a light thrown up a column would; bright in its middle, gone at the top.
		OutHeight = FMath::Pow(Alpha, 0.8f);
		OutStrength = FMath::Pow(FMath::Sin(UE_PI * Alpha), 0.7f);
		return true;
	}

	FText HobName()
	{
		// As his speaker point and the wanted posters name him.
		return NSLOCTEXT("LooterStory", "HobName", "Hob");
	}

	int32 NumDeathLines()
	{
		return DeathLineCount;
	}

	FStoryLine DeathLine(int32 Index)
	{
		const int32 Wrapped = ((Index % DeathLineCount) + DeathLineCount) % DeathLineCount;
		return HobSays(DeathWords(Wrapped));
	}

	FStoryLine FirstDeathLine()
	{
		return HobSays(LOCTEXT("FirstDeath", "Told you, sunshine. You can't stay dead with the debt unpaid. Wherever you drop, my crows drag you home."));
	}

	FStoryLine FirstRoostLine()
	{
		return HobSays(LOCTEXT("FirstRoost", "That post's one of mine. Find my roosts and my crows'll carry you between them. Drop dead, and they'll fetch you back to the last one you touched."));
	}

	FStoryLine FirstTravelLine()
	{
		return HobSays(LOCTEXT("FirstTravel", "Don't wriggle. My lads have never dropped anyone. Not on purpose."));
	}

	int32 NextLine(TArray<int32>& Order, int32& Last, int32 Count, FRandomStream& Random)
	{
		if (Count <= 0)
		{
			return INDEX_NONE;
		}
		if (Count == 1)
		{
			Last = 0;
			return 0;
		}
		// A round no longer fits a different count of lines: start a new one.
		if (Order.ContainsByPredicate([Count](int32 Each) { return Each < 0 || Each >= Count; }))
		{
			Order.Reset();
		}
		if (Order.IsEmpty())
		{
			for (int32 Index = 0; Index < Count; ++Index)
			{
				Order.Add(Index);
			}
			for (int32 Index = Order.Num() - 1; Index > 0; --Index)
			{
				Order.Swap(Index, Random.RandRange(0, Index));
			}
			// The round is taken from its end: the last said never opens the next one.
			if (Order.Last() == Last)
			{
				Order.Swap(Order.Num() - 1, 0);
			}
		}
		Last = Order.Pop(EAllowShrinking::No);
		return Last;
	}
}

#undef LOCTEXT_NAMESPACE

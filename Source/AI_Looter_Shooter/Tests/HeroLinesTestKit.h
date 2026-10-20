#pragma once

#include "CoreMinimal.h"
#include "Embers/HeroDefinition.h"
#include "Internationalization/Regex.h"

/** What the Looter.Lines tests share (HeroLinesTests.cpp, HeroLinesFamilyTests.cpp). */
namespace HeroLinesTestKit
{
	/** Odessa, Crane, Gauge, Pike: every roster hero but Ellis. */
	inline TArray<FName> OtherHeroes()
	{
		TArray<FName> Heroes = UHeroDefinition::RosterIds();
		Heroes.Remove(UHeroDefinition::DefaultId());
		return Heroes;
	}

	/**
	 * The words only Ellis may read (Saved/Lines/DESIGN.md's list): the family ("Pa", "Grandma", "your father"), Ellis's
	 * own name, and what the town called Ellis ("the Ransom kid", "Abel's young'un"). A phrase in Allowed is about someone
	 * else's family ("Tilly's Pa's car": Tilly's own father) and isn't counted.
	 */
	class FFamilyWords
	{
	public:
		FFamilyWords()
			: Pattern(TEXT("\\b(Pa|Pa's|El|Ellis|Grandma|grandmother|your father|young'un|Ransom kid|Ransom's child|Abel's child)\\b"))
		{
		}

		/** The first such word in Words, or empty when there's none. */
		FString FindIn(const FString& Words) const
		{
			FString Checked = Words;
			for (const TCHAR* Allowed : { TEXT("Tilly's Pa's car") })
			{
				Checked.ReplaceInline(Allowed, TEXT(" "), ESearchCase::CaseSensitive);
			}
			FRegexMatcher Matcher(Pattern, Checked);
			return Matcher.FindNext() ? Matcher.GetCaptureGroup(1) : FString();
		}

	private:
		FRegexPattern Pattern;
	};
}

#include "Audio/CreatureVoiceBarks.h"
#include "Creatures/CreatureRank.h"
#include "Misc/Crc.h"

namespace
{
	/** The words and stops of a line, in order: a word (its syllables) or a pause that follows it. */
	struct FToken
	{
		int32 Syllables = 0;
		/** The pause after it (s): none, a word's gap, a comma's, a stop's, a dash's. */
		float Pause = 0.f;
		/** Its phrase ends on a question or a shout. */
		bool bQuestion = false;
		bool bShout = false;
		/** The last word before a stop or the line's end. */
		bool bPhraseEnd = false;
	};

	bool IsWordChar(TCHAR Char)
	{
		return FChar::IsAlnum(Char) || Char == TEXT('\'');
	}

	bool IsVowel(TCHAR Char)
	{
		switch (FChar::ToLower(Char))
		{
		case TEXT('a'): case TEXT('e'): case TEXT('i'): case TEXT('o'): case TEXT('u'): case TEXT('y'):
			return true;
		default:
			return false;
		}
	}

	/** The line as words with the pauses after them. */
	TArray<FToken> Tokenize(const FString& Text)
	{
		TArray<FToken> Tokens;
		int32 Index = 0;
		const int32 Length = Text.Len();
		while (Index < Length)
		{
			if (!IsWordChar(Text[Index]))
			{
				++Index;
				continue;
			}
			const int32 Start = Index;
			while (Index < Length && IsWordChar(Text[Index]))
			{
				++Index;
			}
			FToken Word;
			Word.Syllables = CreatureBarks::CountSyllables(Text.Mid(Start, Index - Start));
			Word.Pause = CreatureBarks::WordGapSeconds;
			// What follows the word up to the next one: the strongest stop in it decides the pause.
			int32 Dots = 0;
			while (Index < Length && !IsWordChar(Text[Index]))
			{
				const TCHAR Char = Text[Index];
				if (Char == TEXT('.'))
				{
					++Dots;
					Word.Pause = FMath::Max(Word.Pause, Dots >= 2 ? CreatureBarks::DashSeconds : CreatureBarks::StopSeconds);
					Word.bPhraseEnd = true;
				}
				else if (Char == TEXT('?') || Char == TEXT('!'))
				{
					Word.Pause = FMath::Max(Word.Pause, CreatureBarks::StopSeconds);
					Word.bPhraseEnd = true;
					Word.bQuestion |= Char == TEXT('?');
					Word.bShout |= Char == TEXT('!');
				}
				else if (Char == TEXT(',') || Char == TEXT(';') || Char == TEXT(':'))
				{
					Word.Pause = FMath::Max(Word.Pause, CreatureBarks::CommaSeconds);
				}
				else if (Char == TEXT('-') || Char == 0x2014 || Char == 0x2026)
				{
					// A dash or an ellipsis: the voice trails off and comes back.
					Word.Pause = FMath::Max(Word.Pause, CreatureBarks::DashSeconds);
				}
				++Index;
			}
			Tokens.Add(Word);
		}
		if (Tokens.Num() > 0)
		{
			Tokens.Last().bPhraseEnd = true;
		}
		return Tokens;
	}
}

// ---------------------------------------------------------------------------
// Lines
// ---------------------------------------------------------------------------

bool CreatureBarks::CanBarkRank(ECreatureRank Rank)
{
	return Rank != ECreatureRank::Boss;
}

bool CreatureBarks::IsAngryRank(ECreatureRank Rank)
{
	return Rank == ECreatureRank::Rare || Rank == ECreatureRank::Epic || Rank == ECreatureRank::Legendary;
}

int32 CreatureBarks::PickLine(ECreatureBark Situation, ECreatureRank Rank, FRandomStream& Random, TConstArrayView<int32> Recent)
{
	if (!CanBarkRank(Rank))
	{
		return INDEX_NONE;
	}
	const TConstArrayView<FCreatureBarkLine> All = AllLines();
	// A Basic soul only grumbles; one fed on the dark is angry most of the time.
	const bool bWantAngry = IsAngryRank(Rank) && Random.FRand() < AngryShare;
	auto Gather = [&](bool bAngry, bool bSkipRecent)
	{
		TArray<int32, TInlineAllocator<16>> Found;
		for (int32 Index = 0; Index < All.Num(); ++Index)
		{
			if (All[Index].Situation == Situation && All[Index].bAngry == bAngry && !(bSkipRecent && Recent.Contains(Index)))
			{
				Found.Add(Index);
			}
		}
		return Found;
	};
	TArray<int32, TInlineAllocator<16>> Pool = Gather(bWantAngry, true);
	if (Pool.IsEmpty())
	{
		// Every line of the tier was said lately: one of them again rather than nothing.
		Pool = Gather(bWantAngry, false);
	}
	if (Pool.IsEmpty() && IsAngryRank(Rank))
	{
		Pool = Gather(!bWantAngry, true);
	}
	return Pool.IsEmpty() ? INDEX_NONE : Pool[Random.RandRange(0, Pool.Num() - 1)];
}

float CreatureBarks::ChanceFor(ECreatureBark Situation)
{
	switch (Situation)
	{
	case ECreatureBark::Idle: return 0.35f;
	case ECreatureBark::Hurt: return 0.3f;
	case ECreatureBark::Spot: return 0.6f;
	case ECreatureBark::PackmateDeath: return 0.75f;
	case ECreatureBark::Death: return 0.45f;
	}
	return 0.f;
}

float CreatureBarks::SituationCooldown(ECreatureBark Situation)
{
	switch (Situation)
	{
	case ECreatureBark::Idle: return 18.f;
	case ECreatureBark::Hurt: return 9.f;
	case ECreatureBark::Spot: return 7.f;
	case ECreatureBark::PackmateDeath: return 6.f;
	case ECreatureBark::Death: return 5.f;
	}
	return 10.f;
}

int32 CreatureBarks::Priority(ECreatureBark Situation)
{
	return static_cast<int32>(Situation);
}

float CreatureBarks::DelayFor(ECreatureBark Situation)
{
	switch (Situation)
	{
	// After the alert's wail has risen (it starts within 0.35 s and swells for about a second).
	case ECreatureBark::Spot: return 1.1f;
	// After the short hurt cry.
	case ECreatureBark::Hurt: return 0.45f;
	// A beat after the fall, as it takes it in.
	case ECreatureBark::PackmateDeath: return 0.6f;
	// At once, over the start of the death wail.
	case ECreatureBark::Death: return 0.15f;
	case ECreatureBark::Idle: return 0.f;
	}
	return 0.f;
}

bool CreatureBarks::IsWhispered(ECreatureBark Situation)
{
	return Situation == ECreatureBark::Idle || Situation == ECreatureBark::Death;
}

// ---------------------------------------------------------------------------
// The murmur
// ---------------------------------------------------------------------------

int32 CreatureBarks::CountSyllables(const FString& Word)
{
	FString Letters;
	for (const TCHAR Char : Word)
	{
		if (FChar::IsAlpha(Char))
		{
			Letters.AppendChar(FChar::ToLower(Char));
		}
	}
	int32 Groups = 0;
	bool bInVowels = false;
	for (const TCHAR Char : Letters)
	{
		const bool bVowel = IsVowel(Char);
		if (bVowel && !bInVowels)
		{
			++Groups;
		}
		bInVowels = bVowel;
	}
	// A silent final e ("came", "home"), unless it's what makes the syllable ("the", "little").
	const int32 Length = Letters.Len();
	if (Groups > 1 && Length >= 3 && Letters[Length - 1] == TEXT('e') && !IsVowel(Letters[Length - 2])
		&& !(Letters[Length - 2] == TEXT('l') && !IsVowel(Letters[Length - 3])))
	{
		--Groups;
	}
	return FMath::Max(1, Groups);
}

TArray<FMurmurGrain> CreatureBarks::BuildMurmur(const FString& Text, uint32 Seed)
{
	TArray<FToken> Tokens = Tokenize(Text);
	// A long line keeps its words: their extra syllables go first.
	int32 Total = 0;
	for (const FToken& Token : Tokens)
	{
		Total += Token.Syllables;
	}
	for (int32 Cap = 3; Total > MaxGrains && Cap >= 1; --Cap)
	{
		Total = 0;
		for (FToken& Token : Tokens)
		{
			Token.Syllables = FMath::Min(Token.Syllables, Cap);
			Total += Token.Syllables;
		}
	}

	FRandomStream Random(static_cast<int32>(Seed));
	TArray<FMurmurGrain> Grains;
	Grains.Reserve(FMath::Min(Total, MaxGrains));
	const int32 Count = FMath::Min(Total, MaxGrains);
	float Time = 0.f;
	int32 Said = 0;
	for (int32 TokenIndex = 0; TokenIndex < Tokens.Num() && Said < Count; ++TokenIndex)
	{
		const FToken& Token = Tokens[TokenIndex];
		// The shout or question of the phrase this word is in (the next stop's mark).
		bool bQuestion = false;
		bool bShout = false;
		for (int32 Ahead = TokenIndex; Ahead < Tokens.Num(); ++Ahead)
		{
			if (Tokens[Ahead].bPhraseEnd)
			{
				bQuestion = Tokens[Ahead].bQuestion;
				bShout = Tokens[Ahead].bShout;
				break;
			}
		}
		for (int32 Syllable = 0; Syllable < Token.Syllables && Said < Count; ++Syllable)
		{
			FMurmurGrain Grain;
			Grain.Time = Time;
			// The voice falls through the line, as speech does.
			const float Through = Count > 1 ? static_cast<float>(Said) / static_cast<float>(Count - 1) : 0.f;
			Grain.Pitch = 1.06f - 0.12f * Through;
			// A word's first syllable carries its stress.
			if (Token.Syllables > 1 && Syllable == 0)
			{
				Grain.Pitch += 0.04f;
			}
			const bool bLastOfPhrase = Token.bPhraseEnd && Syllable == Token.Syllables - 1;
			if (bShout)
			{
				Grain.Pitch += 0.05f;
				Grain.Volume = 1.15f;
			}
			if (bLastOfPhrase)
			{
				// A question lifts at its end, a shout holds; anything else trails off.
				if (bQuestion)
				{
					Grain.Pitch += 0.16f;
				}
				else if (!bShout)
				{
					Grain.Pitch -= 0.03f;
					Grain.Volume *= 0.85f;
				}
			}
			else if (bQuestion && Token.bPhraseEnd)
			{
				Grain.Pitch += 0.08f;
			}
			Grains.Add(Grain);
			++Said;
			Time += SyllableSeconds * Random.FRandRange(0.85f, 1.15f);
		}
		// The gap after the word has the syllable's own time in it already.
		Time += Token.Pause;
	}
	if (Grains.Num() > 0)
	{
		Grains.Last().Volume *= 0.9f;
	}
	return Grains;
}

float CreatureBarks::DisplaySeconds(const FString& Text, float MurmurSeconds)
{
	const float Reading = 0.9f + 0.055f * static_cast<float>(Text.Len());
	return FMath::Clamp(FMath::Max(Reading, MurmurSeconds + 0.7f), 1.6f, 4.5f);
}

float CreatureBarks::VoicePitchFor(FName Speaker)
{
	const uint32 Hash = FCrc::StrCrc32(*Speaker.ToString());
	return 0.88f + 0.26f * static_cast<float>(Hash % 1024u) / 1023.f;
}

// ---------------------------------------------------------------------------
// The board
// ---------------------------------------------------------------------------

FCreatureBarkBoard::EVerdict FCreatureBarkBoard::Check(uint32 InSpeaker, ECreatureBark InSituation, const FVector& Where,
	const FVector& Listener, double Now) const
{
	if (FVector::DistSquared(Where, Listener) > FMath::Square(static_cast<double>(CreatureBarks::HearRadius)))
	{
		return EVerdict::TooFar;
	}
	const int32 Kind = static_cast<int32>(InSituation);
	if (IsShowing(Now))
	{
		// One bark on screen at a time: only one that matters more cuts in, once the one there has been read a moment;
		// a creature's own last words always replace what it was saying.
		const bool bOwnDeath = InSpeaker == Speaker && InSituation == ECreatureBark::Death;
		const bool bMatters = CreatureBarks::Priority(InSituation) > CreatureBarks::Priority(Situation);
		if (!bOwnDeath && !(bMatters && Now - Since >= CreatureBarks::MinShownSeconds))
		{
			return EVerdict::Busy;
		}
		return Now < NextBySituation[Kind] ? EVerdict::Cooling : EVerdict::Allowed;
	}
	// Last words and a fallen packmate are events: they skip the pause after a bark, but not their own kind's rest.
	const bool bEvent = InSituation == ECreatureBark::Death || InSituation == ECreatureBark::PackmateDeath;
	if ((!bEvent && Now < NextAny) || Now < NextBySituation[Kind])
	{
		return EVerdict::Cooling;
	}
	return EVerdict::Allowed;
}

void FCreatureBarkBoard::Start(uint32 InSpeaker, ECreatureBark InSituation, const FVector& Where, double Now, float Seconds)
{
	Speaker = InSpeaker;
	Situation = InSituation;
	Since = Now;
	Until = Now + FMath::Max(Seconds, 0.1f);
	NextAny = Until + CreatureBarks::GapAfter;
	NextBySituation[static_cast<int32>(InSituation)] = Now + CreatureBarks::SituationCooldown(InSituation);
}

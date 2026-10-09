#pragma once

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"
#include "Math/RandomStream.h"

enum class ECreatureRank : uint8;

/**
 * When a creature says something (a bark): the Unpaid mutter bits of their lives, and the town's dead were never paid
 * their due. Ordered by how much it matters: a bark may cut in over one that matters less (Priority).
 */
enum class ECreatureBark : uint8
{
	/** Muttered to itself while it stands or drifts about, rarely. */
	Idle,
	/** Hit by a bullet (now and then, with its hurt cry). */
	Hurt,
	/** It has seen the player and turns on them. */
	Spot,
	/** One of its pack fell near it. */
	PackmateDeath,
	/** Its last words. */
	Death,
};

/** One line a creature can say: when, whether it's one of the angrier ones (a Restless or Gravebound soul's), and the words. */
struct FCreatureBarkLine
{
	ECreatureBark Situation = ECreatureBark::Idle;
	bool bAngry = false;
	const TCHAR* Text = TEXT("");
};

/** One syllable of a murmured line: when it starts (s from the line's start), its pitch and its loudness against the voice's own. */
struct FMurmurGrain
{
	float Time = 0.f;
	float Pitch = 1.f;
	float Volume = 1.f;
};

/**
 * The barks' rules apart from the world, so the tests can run them: the lines (CreatureVoiceBarkLines.cpp), which one a
 * creature of a rank says, how often each kind of bark may come, and the murmur that carries a line: one breathy syllable
 * per syllable of its words, in their rhythm (pauses at commas and dots, a rise on a question), never speech.
 */
namespace CreatureBarks
{
	inline constexpr int32 NumSituations = static_cast<int32>(ECreatureBark::Death) + 1;

	/** Every line, in a fixed order (an index names a line). */
	AI_LOOTER_SHOOTER_API TConstArrayView<FCreatureBarkLine> AllLines();

	/** Whether a creature of this rank barks at all: a boss has its own words (Abel's scene), so it never does. */
	AI_LOOTER_SHOOTER_API bool CanBarkRank(ECreatureRank Rank);

	/** Whether this rank says the angrier lines (Restless, Gravebound and Soulfed souls, fed longer on the dark). */
	AI_LOOTER_SHOOTER_API bool IsAngryRank(ECreatureRank Rank);

	/**
	 * The line (an index into AllLines) a creature of Rank says in Situation, never one of Recent while another is left.
	 * An angry rank says an angry line most of the time (AngryShare), a plain one now and then; a Basic one only plain
	 * lines. INDEX_NONE for a rank that never barks.
	 */
	AI_LOOTER_SHOOTER_API int32 PickLine(ECreatureBark Situation, ECreatureRank Rank, FRandomStream& Random,
		TConstArrayView<int32> Recent = {});

	/** How likely a creature is to say something when its moment comes (0-1): a spot often, a hurt now and then. */
	AI_LOOTER_SHOOTER_API float ChanceFor(ECreatureBark Situation);

	/** Seconds after a bark of this kind before any creature says another of its kind. */
	AI_LOOTER_SHOOTER_API float SituationCooldown(ECreatureBark Situation);

	/** Higher cuts in over lower: its last words over a call to its pack, over a spot, over a hurt, over a mutter. */
	AI_LOOTER_SHOOTER_API int32 Priority(ECreatureBark Situation);

	/** Seconds after the moment before the bark comes: after the cry it comes with (the alert's wail, the hurt cry). */
	AI_LOOTER_SHOOTER_API float DelayFor(ECreatureBark Situation);

	/** Whether the murmur is whispered (a mutter to itself, a dying breath) rather than voiced. */
	AI_LOOTER_SHOOTER_API bool IsWhispered(ECreatureBark Situation);

	/** A word's syllables, as a reader would say it (its vowel groups, a silent final e dropped), at least 1. */
	AI_LOOTER_SHOOTER_API int32 CountSyllables(const FString& Word);

	/**
	 * The murmur of a line: a syllable for each of its words' syllables, about SyllableSeconds apart, a little further
	 * between words, held at commas, dots and dashes; falling through the line, rising at a question, louder at a shout.
	 * Seed varies the pace a little. At most MaxGrains syllables (a long line's words lose their extra syllables first).
	 */
	AI_LOOTER_SHOOTER_API TArray<FMurmurGrain> BuildMurmur(const FString& Text, uint32 Seed);

	/** How long a line stays over its speaker: long enough to read, and past its murmur's end (s). */
	AI_LOOTER_SHOOTER_API float DisplaySeconds(const FString& Text, float MurmurSeconds);

	/** A creature's own voice against its kind's, from its name: each Unpaid sounds like somebody (0.88-1.14). */
	AI_LOOTER_SHOOTER_API float VoicePitchFor(FName Speaker);

	inline constexpr float SyllableSeconds = 0.155f;
	inline constexpr float WordGapSeconds = 0.04f;
	inline constexpr float CommaSeconds = 0.22f;
	inline constexpr float StopSeconds = 0.32f;
	inline constexpr float DashSeconds = 0.42f;
	inline constexpr int32 MaxGrains = 18;

	/** An angry rank's share of angry lines. */
	inline constexpr float AngryShare = 0.7f;

	/** The lines said lately that aren't said again while others are left. */
	inline constexpr int32 RecentLines = 8;

	/** Barks are heard and read only this near the player (cm): farther, nothing is said. */
	inline constexpr float HearRadius = 2500.f;

	/** A creature says nothing more for this long after a bark of its own (s), unless it dies. */
	inline constexpr float SpeakerRest = 10.f;

	/** After a bark leaves the screen, the next waits this long (s), so barks never chain into chatter. */
	inline constexpr float GapAfter = 1.5f;

	/** A bark that matters more cuts in only after the one on screen has been read this long (s). */
	inline constexpr float MinShownSeconds = 0.5f;
}

/**
 * Who may bark now (UCreatureVoiceDirector keeps one per level): at most one bark on screen near the player at a time, a
 * pause after each, and each kind of bark resting a while after it's said. Times are the world's seconds.
 */
struct AI_LOOTER_SHOOTER_API FCreatureBarkBoard
{
	enum class EVerdict : uint8
	{
		Allowed,
		/** Farther from the player than HearRadius. */
		TooFar,
		/** Another bark is on screen, and this one matters no more than it (or it was only just said). */
		Busy,
		/** The pause after the last bark, or this kind's rest, isn't over. */
		Cooling,
	};

	/** Whether Speaker (any id; its object's unique id in the game) may say a bark of Situation at Where, heard at Listener. */
	EVerdict Check(uint32 Speaker, ECreatureBark Situation, const FVector& Where, const FVector& Listener, double Now) const;

	/** Speaker's bark of Situation goes on screen for Seconds (replacing whatever was there). */
	void Start(uint32 Speaker, ECreatureBark Situation, const FVector& Where, double Now, float Seconds);

	/** A bark is on screen now. */
	bool IsShowing(double Now) const { return Now < Until; }

	/** Who says the bark on screen (0: none), and what kind it is. */
	uint32 GetSpeaker(double Now) const { return IsShowing(Now) ? Speaker : 0; }
	ECreatureBark GetSituation() const { return Situation; }

	/** No bark of any kind before this (s). */
	double GetNextAny() const { return NextAny; }

private:
	uint32 Speaker = 0;
	ECreatureBark Situation = ECreatureBark::Idle;
	double Since = -1.0e9;
	double Until = -1.0e9;
	double NextAny = -1.0e9;
	double NextBySituation[CreatureBarks::NumSituations] = { -1.0e9, -1.0e9, -1.0e9, -1.0e9, -1.0e9 };
};

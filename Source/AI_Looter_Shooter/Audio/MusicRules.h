#pragma once

#include "CoreMinimal.h"
#include "Audio/AmbienceRules.h"

enum class ECreatureRank : uint8;

/** What the score is doing: the area's theme (resting between plays), a fight laid over it, or a boss's fight. */
enum class EMusicMood : uint8
{
	Calm,
	Combat,
	Boss,
};

/** A boss fight's theme: Abel's (the Unpaid's), or the Gravemother's (the spiders'). */
enum class EBossTheme : uint8
{
	Keeper,
	Gravemother,
};

/**
 * A piece of the score as Art/Sounds/recipes/music.py builds it: its cue, its bars, beats per bar and how long a bar
 * lasts. The director crossfades on these bar lines, so a change here goes with one there.
 */
struct FMusicPiece
{
	FName Cue;
	int32 Bars = 16;
	int32 BeatsPerBar = 4;
	float BarSeconds = 2.f;

	float BeatSeconds() const { return BarSeconds / FMath::Max(BeatsPerBar, 1); }
	double LoopSeconds() const { return static_cast<double>(Bars) * BarSeconds; }
};

/**
 * Whether the player is being hunted, as the music hears it: a fight starts the moment a creature turns on them and
 * goes on for CalmAfter seconds after the last one lets go, so a creature losing sight for a moment doesn't drop the
 * music out and bring it straight back.
 */
struct AI_LOOTER_SHOOTER_API FCombatMemory
{
	bool bInCombat = false;
	double LastHunted = 0.0;
	double StartedAt = 0.0;

	/** Hears whether anything hunts the player now; true when that turned the fight on or off. */
	bool Update(bool bHunted, double Now, float CalmAfter);

	/** How long the fight ran (from the first creature turning to the last letting go). */
	double FightSeconds() const { return FMath::Max(0.0, LastHunted - StartedAt); }
};

/** The music director's rules as plain functions and numbers, apart from the engine's audio so the tests can run them. */
namespace MusicRules
{
	/** The pieces (music.py's bars and tempos). */
	AI_LOOTER_SHOOTER_API FMusicPiece ExploreFor(ELooterAudioArea Area);
	AI_LOOTER_SHOOTER_API FMusicPiece Combat();
	AI_LOOTER_SHOOTER_API FMusicPiece BossFor(EBossTheme Theme);
	AI_LOOTER_SHOOTER_API TArray<FMusicPiece> AllPieces();

	/**
	 * The first bar line (or beat: Step) at or after Now on a grid starting at Origin. A line passed less than
	 * BoundaryGrace ago counts as now: starting a hair late beats waiting a whole bar.
	 */
	AI_LOOTER_SHOOTER_API double NextBoundary(double Now, double Origin, double Step);
	inline constexpr double BoundaryGrace = 0.015;

	/** The mood wanted: a boss's fight over everything, then any fight, else calm. */
	AI_LOOTER_SHOOTER_API EMusicMood Resolve(bool bBossFight, bool bInCombat);

	/** The music's gain under a scene (a cutscene's lines and moments come first) or a paused game (quieter, still going). */
	AI_LOOTER_SHOOTER_API float DuckFor(bool bScenePlaying, bool bPaused);
	inline constexpr float SceneDuck = 0.35f;
	inline constexpr float PauseDuck = 0.45f;

	/** A fight ended in victory worth a sting: it lasted a while and something died in it (not a creature giving up). */
	AI_LOOTER_SHOOTER_API bool EarnsVictory(double FightSeconds, int32 Kills);
	inline constexpr float VictoryMinFightSeconds = 12.f;
	inline constexpr float VictoryCooldown = 45.f;

	/** A creature of high rank (Gravebound, Soulfed) is announced by a sting the first time it comes for the player. */
	AI_LOOTER_SHOOTER_API bool IsElite(ECreatureRank Rank);
	inline constexpr float EliteStingCooldown = 30.f;

	/** Which theme a boss's fight plays: the Unpaid's (Abel) or the spiders' (the Gravemother, and any other). */
	AI_LOOTER_SHOOTER_API EBossTheme ThemeFor(bool bUnpaidBoss);

	/** In calm, the theme plays two or three times through (Roll), then rests for a while (Roll across 40-90 s): space
	 *  between the music's appearances, as between its phrases, so the world's own sound gets its turn. */
	AI_LOOTER_SHOOTER_API int32 CalmLoops(float Roll);
	AI_LOOTER_SHOOTER_API float CalmRestSeconds(float Roll);
	inline constexpr float MinRestSeconds = 40.f;
	inline constexpr float MaxRestSeconds = 90.f;

	/** How the layers move (seconds unless bars). */
	inline constexpr float CalmAfterSeconds = 6.f;
	inline constexpr float PollSeconds = 0.25f;
	inline constexpr float UnderCombat = 0.7f;
	inline constexpr float CombatFadeInSeconds = 0.2f;
	inline constexpr int32 CombatOutBars = 2;
	inline constexpr float ExploreFadeInSeconds = 3.f;
	inline constexpr int32 ExploreOutBars = 2;
	inline constexpr float FirstThemeDelay = 2.f;
	inline constexpr float BossOutSeconds = 2.5f;
	inline constexpr float ThemeAfterBossSeconds = 5.f;
	inline constexpr float DuckSeconds = 0.8f;
	/** The phase sting's hit lands this far in (music.py's sting_phase), so a boss theme starts on it. */
	inline constexpr float PhaseStingHit = 0.75f;
	/** A boss theme dips under a phase sting for this long, at this gain. */
	inline constexpr float PhaseDuckSeconds = 2.5f;
	inline constexpr float PhaseDuck = 0.5f;
}

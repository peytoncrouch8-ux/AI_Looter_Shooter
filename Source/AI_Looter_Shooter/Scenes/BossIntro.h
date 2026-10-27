#pragma once

#include "CoreMinimal.h"
#include "Story/StoryLine.h"

class AAbelKeeper;
class AActor;
class AForemanCreature;
class AGravemotherCreature;
class UBossComponent;
struct FCampaignRecord;

/**
 * The boss intros (the user's pick, playtest round 4, 2026-10-27): a short in-engine scene (8-15 s) as each of Ransom's
 * Rest's three boss fights first starts, all in C++ with no logic in Sequencer, played by USceneSubsystem: a camera move
 * that shows the arena and the boss's entrance, a name card (its name and an epithet), one or two caption lines, and the
 * fight on as it ends. Skippable like every scene; once per session per boss (a campaign flag, BossIntro.Seen.<Kind>); a
 * rematch from a vigil stone (UBossComponent::IsRematch) never plays it.
 *
 *  - The Gravemother: the Sink from behind the player, her among her graves; she dives into the floor, the ground heaves
 *    toward the player in a line of bursts, and she bursts out of the Sink's floor between them and screams.
 *  - Cyrus Teague, the Foreman: the stope out of the dark (the scene camera's exposure rising), his carbide lamp the one
 *    light at the fall where he digs, his mutter; then he turns from the fall into its light, pick raised (his own entrance).
 *  - Abel Ransom, the Keeper: the burial boards at dusk from the Keeper's Gate, him at the far end among the lantern posts
 *    against the sunset; then before him as he raises his ghost lantern (his own entrance). His fight's first line is
 *    its own, said once as it starts; Hob's caption follows it.
 *
 * How (BossIntroScene.cpp): the fight has already started (the hook is each boss's HandleFightStarted), so the scene
 * holds the player (no control: the scene's rules), makes them untouchable (FBossIntroGuard: through the scene and
 * GraceSeconds after it) and freezes the boss and every creature round it and round the player (FBossIntroFreeze: their
 * own clocks slowed to almost nothing, a boss's fight clock with it; a second boss nearby too). At its Live moment the
 * boss alone runs again and does its own entrance on camera (the Gravemother's climb out of the dirt, the Foreman's slam,
 * Abel's lantern); the scene ends as that entrance ends, the view eases back to the player's eyes facing the boss, the
 * boss bar sweeps in again, and the rest wake. Skipped, the same happens at once. The card's epithet is the bar's title.
 *
 * The words are /Game/Data/Story/DA_Lines_BossIntro<Kind>'s (create_story_lines.py) when the set has them all, else
 * ScriptLines' (the same words; Looter.Scenes.BossIntro.Lines checks they match). No family words: the same for every hero.
 */

/** Which boss's intro. */
enum class EBossIntro : uint8
{
	Gravemother,
	Foreman,
	Abel,
};

/** What a camera point is placed from: the boss's feet, the player's eyes, or where the boss bursts out (the Gravemother). */
enum class EBossIntroAnchor : uint8
{
	Boss,
	Player,
	Burst,
};

/**
 * A camera point in the intro's frame: Offset from its Anchor along the frame's axes (cm): X away from the boss toward the
 * player (flat), Y to the right of that, Z up.
 */
struct FBossIntroPoint
{
	EBossIntroAnchor Anchor = EBossIntroAnchor::Boss;
	FVector Offset = FVector::ZeroVector;
};

/** One shot: the eye and what it looks at eased from From to To over its span, the lens (degrees) and the exposure (EV). */
struct FBossIntroShot
{
	FName Name;
	float Start = 0.f;
	float Duration = 0.f;
	FBossIntroPoint FromEye;
	FBossIntroPoint ToEye;
	FBossIntroPoint FromLook;
	FBossIntroPoint ToLook;
	float FromFov = 60.f;
	float ToFov = 55.f;
	/** The scene camera's exposure bias (EV): below 0 reads darker (the Foreman's stope coming out of the dark). */
	float FromExposure = 0.f;
	float ToExposure = 0.f;
	/** Its look follows the boss as it moves (its live entrance). */
	bool bTrackBoss = false;
};

/** A caption: at At seconds, ScriptLines' Line. */
struct FBossIntroCaption
{
	float At = 0.f;
	int32 Line = INDEX_NONE;
};

/** A boss's intro: its timing, shots and captions. Every time is seconds from the scene's start. */
struct FBossIntroScript
{
	/** Its whole length when nothing is skipped. */
	float Seconds = 10.f;
	/** The boss runs again and does its own entrance on camera (the Gravemother bursts out here). */
	float LiveAt = 6.f;
	/** The name card: up at CardAt for CardSeconds. */
	float CardAt = 6.5f;
	float CardSeconds = 3.f;
	/** The view starts back to the player's eyes (over BossIntro::HandBackSeconds), the letterbox goes, the bar sweeps in. */
	float HandBackAt = 9.f;
	/** The Gravemother dives into the floor here (negative: no dive). */
	float DiveAt = -1.f;
	/** The ground bursting along her way under it toward the burst: this many puffs, from TrailFrom to TrailTo. */
	int32 TrailPuffs = 0;
	float TrailFrom = 0.f;
	float TrailTo = 0.f;
	/** Its music (2D, from 0, faded out at LiveAt under the reveal's hit). */
	const TCHAR* Music = nullptr;
	/** Back to back from 0 to Seconds. */
	TArray<FBossIntroShot> Shots;
	TArray<FBossIntroCaption> Captions;
};

/** Where an intro's frame stands in the world (FBossIntroFrame::Make). */
struct AI_LOOTER_SHOOTER_API FBossIntroFrame
{
	FVector Boss = FVector::ZeroVector;
	FVector Player = FVector::ZeroVector;
	FVector Burst = FVector::ZeroVector;
	/** Flat, from the boss toward the player; Right square to it. */
	FVector Forward = FVector::ForwardVector;
	FVector Right = FVector::RightVector;

	/** From the boss's feet, the player's eyes and where it bursts out; BossFacing when the player stands right over it. */
	static FBossIntroFrame Make(const FVector& BossFeet, const FVector& PlayerEye, const FVector& BurstFeet, const FVector& BossFacing);

	/** A point of the script in the world. */
	FVector Resolve(const FBossIntroPoint& Point) const;
};

/** A camera's place, aim, lens and exposure. */
struct FBossIntroView
{
	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	float Fov = 60.f;
	float Exposure = 0.f;
};

/**
 * What one boss does in its intro besides the script's camera, card, words and freeze (BossIntroBosses.cpp fills it). Each
 * moment is told whether the scene is skipping to its end: a skip leaves out what only a played scene shows or says, and
 * still leaves the world where the fight needs it.
 */
struct FBossIntroActs
{
	/** Into the ground (the Gravemother). */
	TFunction<void(bool /*bSkipping*/)> Dive;
	/** The ground bursting at Where (puff Puff of the script's TrailPuffs, along her way from her feet to the burst). */
	TFunction<void(int32 /*Puff*/, const FVector& /*Where*/, bool /*bSkipping*/)> Trail;
	/** Its entrance, right after it runs again (the Gravemother put at the burst and climbing out). */
	TFunction<void(bool /*bSkipping*/)> Live;
	/** The scene has ended, played or skipped (after the player is given back). */
	TFunction<void(bool /*bSkipped*/)> Ended;
	/** Where it bursts out (the Burst anchor); without one, its feet. */
	FVector BurstFeet = FVector::ZeroVector;
	bool bHasBurst = false;
};

/**
 * The intros' sounds (Art/Sounds/recipes/bossintro.py; the same names belong in Audio/LooterSoundCues.h's boss block, so
 * the bank's check knows the code plays them). Music: each boss's bed from the scene's start, swelling to its reveal at
 * the script's LiveAt (2D, faded out there); the reveal's hit (2D). The Gravemother's dive into the floor and her burst out
 * of it (3D, at her).
 */
namespace BossIntroCue
{
	inline constexpr const TCHAR* Gravemother = TEXT("Music.BossIntro.Gravemother");
	inline constexpr const TCHAR* Foreman = TEXT("Music.BossIntro.Foreman");
	inline constexpr const TCHAR* Abel = TEXT("Music.BossIntro.Abel");
	inline constexpr const TCHAR* Reveal = TEXT("Music.BossIntro.Reveal");
	inline constexpr const TCHAR* Burrow = TEXT("Boss.Gravemother.Burrow");
	inline constexpr const TCHAR* Emerge = TEXT("Boss.Gravemother.Emerge");
}

namespace BossIntro
{
	// --- Rules and numbers ---

	/**
	 * Abel's fight says its first line as it starts (AbelRules::PhaseLine(0), 3.4 s): his intro's words come after it, so
	 * neither cuts the other off (Looter.Scenes.BossIntro.Script).
	 */
	inline constexpr float FightLineSeconds = 3.4f;
	/** The user's ask: short. Every script is held to it (Looter.Scenes.BossIntro.Script). */
	inline constexpr float ShortestSeconds = 8.f;
	inline constexpr float LongestSeconds = 15.f;
	/** A cut between shots dips to black this long either side: a hard cut across an arena jars. */
	inline constexpr float CutDipSeconds = 0.25f;
	/** The view eases back to the player's eyes over this long; the scene ends as it's there. */
	inline constexpr float HandBackSeconds = 0.8f;
	/** The view eases from the player's eyes to the first shot over this long: the camera leaves them, no cut. */
	inline constexpr float EnterBlendSeconds = 0.6f;
	/**
	 * A frozen creature's clock runs this fast (its CustomTimeDilation): almost still, never 0 (a tick of zero seconds is
	 * divided by in places). Ten seconds of scene move it a hundredth of a second.
	 */
	inline constexpr float FreezeDilation = 0.001f;
	/** Every living creature this near the boss (cm) is frozen with it: a wave its first phase raised at once can't creep up. */
	inline constexpr float FreezeRadius = 6000.f;
	/** A shot's eye stays this far (cm) in front of any wall or ground between it and what it looks at. */
	inline constexpr float BoomMargin = 40.f;
	/** ...and never nearer than this to what it looks at, whatever is in the way. */
	inline constexpr float BoomShortest = 150.f;
	/** The eye is swept as a ball this wide (cm) so the lens's near plane never cuts into a board or a wall. */
	inline constexpr float BoomRadius = 20.f;
	/** The eye stays at least this high (cm) over the floor under what it looks at (Abel's deck, the stope's floor). */
	inline constexpr float MinEyeOverFloor = 60.f;
	/**
	 * After an intro the player can't be hurt for this long (s): the view has only just come back, and a boss's first move
	 * (Abel's buckshot) must not land before they can move. They can't be hurt during the intro either.
	 */
	inline constexpr float GraceSeconds = 1.5f;
	/** Every hostile this near the player (cm) is held too: a second boss or a pack close by can't reach the held player. */
	inline constexpr float PlayerFreezeRadius = 6000.f;

	/** The Gravemother's burst: this share of the way from her to the player... */
	inline constexpr float BurstShare = 0.45f;
	/** ...never nearer the player than this (cm: room to see her whole as she climbs out, out of her bite and quake)... */
	inline constexpr float BurstMinFromPlayer = 1000.f;
	/** ...nor farther from her home than this (cm: well inside her leash, on her own ground). */
	inline constexpr float BurstMaxFromHome = 1600.f;
	/** The ground found there must be within this (cm) of her feet's height (her level of the Sink, not a ledge or a hole). */
	inline constexpr float BurstMaxRise = 300.f;

	/** Looter.BossIntro: 1 plays the intros (the default), 0 never. */
	AI_LOOTER_SHOOTER_API bool IsOn();
	/** Looter.BossIntro.Replay: 1 plays one at every first-fight start, seen or not (a rematch still never): for checks. */
	AI_LOOTER_SHOOTER_API bool IsReplayOn();
	/** Due: on, not a rematch, and not seen in this session (Campaign null: a world with no story, due). */
	AI_LOOTER_SHOOTER_API bool IsDue(const FCampaignRecord* Campaign, EBossIntro Kind, bool bRematch);

	// --- Names ---

	/** "Gravemother", "Foreman", "Abel". */
	AI_LOOTER_SHOOTER_API const TCHAR* KindName(EBossIntro Kind);
	/** The kind a name says (any case); false when it says none. */
	AI_LOOTER_SHOOTER_API bool ParseKind(const FString& Name, EBossIntro& OutKind);
	/** "BossIntro.Gravemother": the scene's name (the missions hear Scene.BossIntro.<Kind>; nothing waits for it). */
	AI_LOOTER_SHOOTER_API FName SceneName(EBossIntro Kind);
	/** "BossIntro.Seen.Gravemother": the campaign's story flag once it has played or been skipped. */
	AI_LOOTER_SHOOTER_API FName SeenFlag(EBossIntro Kind);
	/** Its line set's asset path (DA_Lines_BossIntro<Kind>). */
	AI_LOOTER_SHOOTER_API FString LinesAssetPath(EBossIntro Kind);

	// --- The script (BossIntro.cpp) ---

	/** Its lines as written (a speaker each: captions, shown as Subtitles say). */
	AI_LOOTER_SHOOTER_API const TArray<FStoryLine>& ScriptLines(EBossIntro Kind);
	/** The words to say: the line set's when it has every line, else ScriptLines (the captions resolve them for the hero). */
	AI_LOOTER_SHOOTER_API TArray<FStoryLine> Lines(EBossIntro Kind);
	/** The name card: the boss's name, in capitals. */
	AI_LOOTER_SHOOTER_API FText CardTitle(EBossIntro Kind);
	/**
	 * Its epithet, in capitals: the boss bar's own title (FBossShow::Title, spelled under the bar as it sweeps in), so the
	 * card and the bar say the same; Fight null or untitled: the same words as the bars have them (BossIntro.cpp).
	 */
	AI_LOOTER_SHOOTER_API FText CardSub(EBossIntro Kind, const UBossComponent* Fight = nullptr);
	AI_LOOTER_SHOOTER_API const FBossIntroScript& Script(EBossIntro Kind);

	// --- The camera's math (BossIntro.cpp) ---

	/** Smootherstep: every move eases in and out. */
	AI_LOOTER_SHOOTER_API float Ease(float Alpha);
	/** Where a shot puts the camera at Alpha (0-1 through it); BossShift: how far the boss has moved since the frame was made. */
	AI_LOOTER_SHOOTER_API FBossIntroView ViewAlong(const FBossIntroShot& Shot, const FBossIntroFrame& Frame, float Alpha,
		const FVector& BossShift = FVector::ZeroVector);
	/**
	 * The eye pulled in toward Look when something stands between them: HitShare is how far along Look -> Eye the first
	 * thing in the way is (1: nothing). The eye stays Margin in front of it, and never nearer Look than Shortest.
	 */
	AI_LOOTER_SHOOTER_API FVector BoomClip(const FVector& Look, const FVector& Eye, float HitShare, float Margin = BoomMargin,
		float Shortest = BoomShortest);
	/**
	 * Where the Gravemother bursts out (before the ground is found under it): BurstShare of the way from her toward the
	 * player, no nearer them than BurstMinFromPlayer, no farther from her home than BurstMaxFromHome; at her feet's height.
	 */
	AI_LOOTER_SHOOTER_API FVector BurstAlong(const FVector& HerFeet, const FVector& PlayerFeet, const FVector& Home);

	// --- Playing them (BossIntroScene.cpp, BossIntroBosses.cpp) ---

	/**
	 * Plays Kind's intro now on Boss (its body: frozen, framed; Fight: its bar put away and swept in again) with its own
	 * Acts. False when nothing played (scenes off, another playing, no world). Records nothing: PlayIfDue does.
	 */
	AI_LOOTER_SHOOTER_API bool Play(EBossIntro Kind, AActor& Boss, UBossComponent* Fight, FBossIntroActs&& Acts);

	/**
	 * The hooks, each from its boss's HandleFightStarted (its fight has just started): plays its intro in a game world when
	 * due, and records it seen. False when it didn't play (the fight goes on as before).
	 */
	AI_LOOTER_SHOOTER_API bool PlayIfDue(AGravemotherCreature& Boss);
	AI_LOOTER_SHOOTER_API bool PlayIfDue(AForemanCreature& Boss);
	AI_LOOTER_SHOOTER_API bool PlayIfDue(AAbelKeeper& Boss);

	/** The same now, seen or not (Looter.BossIntro.Play on a fight already on). Records it seen. */
	AI_LOOTER_SHOOTER_API bool PlayNow(AGravemotherCreature& Boss);
	AI_LOOTER_SHOOTER_API bool PlayNow(AForemanCreature& Boss);
	AI_LOOTER_SHOOTER_API bool PlayNow(AAbelKeeper& Boss);
}

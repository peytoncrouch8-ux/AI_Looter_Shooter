#pragma once

#include "CoreMinimal.h"
#include "Scenes/SceneSubsystem.h"
#include "Story/StoryLine.h"

class UWorld;
struct FCampaignRecord;

/**
 * The new game's intro (the user's playtest note 3, 2026-10-27: "what is the game about, how did we get here"): about 78 s
 * of short captions over in-engine shots, before the cold open and before the player has control, once per session,
 * skippable. Story from Docs/Story.md, no new canon: Teropa, the Reaches, the dead given to the wind, the saints, the
 * robbery, the dead coming home, who the player is, and the hook ("Seven outlaws killed you. Something sent you back to
 * bring them in."); the title card at its end is the cold open's own, REVENANT rising through the white it ends on.
 *
 * Two parts in two worlds:
 *  - Teropa (PlanetSeconds): in the menu world, right after the hero is picked, when the menu is PLANET's scene of Teropa
 *    from orbit (Lvl_MenuTeropa): the menu fades, the camera leaves the menu's framing for the planet and pushes in on
 *    the dusk line, the white comes up, and the level opens (UNewGameIntroSubsystem::PlayPlanet, from
 *    USessionSubsystem::StartNewGame).
 *  - Ransom's Rest (RestSeconds): on the story's first level, before the cold open (UColdOpenSubsystem::BeginIfDue calls
 *    PlayRestIfDue): the white thins on a crane over Ransom's Point and the valley, the dark chapel, the farm, the
 *    player's own grave; then the white again, which the cold open's title rises through.
 * Where the planet part couldn't play (the menu isn't the planet scene yet, a console new game, the old Skyreach cast-off)
 * the Rest's part opens with Westward instead: the planet part's captions over the canyon and the plains into the low sun.
 *
 * The words are /Game/Data/Story/DA_Lines_NewGameIntro's (create_story_lines.py) when it has them all, else ScriptLines'
 * (the same words, written here so the intro plays before the script runs; Looter.Intro.Lines checks they match). The
 * cards' place names and the hero's name card are the code's. Nothing here names Ellis's family: the same for every hero.
 */

/** What a caption slot shows: a narration line, or a card (a place's name, or the hero's). */
enum class EIntroCard : uint8
{
	None,
	Teropa,
	Reaches,
	Rest,
	Hero,
};

/** One caption: from At (seconds from its part's start) for Seconds, a card or narration line Line (ScriptLines' index). */
struct FIntroCaption
{
	float At = 0.f;
	float Seconds = 0.f;
	EIntroCard Card = EIntroCard::None;
	int32 Line = INDEX_NONE;
};

/** A camera on one shot: the eye and what it looks at, eased from From to To; the lens in degrees. World cm. */
struct FIntroMove
{
	FName Name;
	float Start = 0.f;
	float Duration = 0.f;
	FVector FromEye = FVector::ZeroVector;
	FVector ToEye = FVector::ZeroVector;
	FVector FromLook = FVector::ZeroVector;
	FVector ToLook = FVector::ZeroVector;
	float FromFov = 70.f;
	float ToFov = 70.f;
};

/** Where a camera is, which way it looks and its lens. */
struct FIntroView
{
	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	float Fov = 70.f;
};

/** The menu's planet, as the planet shot frames it: its middle, its radius, which way its sun is. */
struct FPlanetFrame
{
	FVector Center = FVector::ZeroVector;
	float Radius = 0.f;
	/** Toward the sun (unit): the dusk line is the great circle square to it. */
	FVector ToSun = FVector(0.3, -1.0, 0.2).GetSafeNormal();
	/** A planet was found; without one the shot only turns and narrows the menu's own view. */
	bool bFound = false;
};

/** The Rest part's makings (PlayRest fills it from the world). */
struct FIntroRestSetup
{
	/** Open on Westward (the planet part didn't play for this session). */
	bool bWestward = true;
	/** The narration, ScriptLines' order, already resolved for the hero. */
	TArray<FStoryLine> Lines;
	/** The name card: the hero's name and role in capitals (empty: no name card). */
	FText HeroTitle;
	FText HeroRole;
	/** The player's grave, to frame the last shot on (GraveWake's spots: its headboard's face, its foot); none: the layout's spot. */
	bool bHasGrave = false;
	FVector GraveLook = FVector::ZeroVector;
	FVector GraveFoot = FVector::ZeroVector;
};

namespace NewGameIntro
{
	// --- Timing (seconds). The captions' slots below are set against these; Looter.Intro.Script checks them. ---

	/** The planet part (or Westward in its place): 28 s, the length of Music.Intro.Planet. */
	inline constexpr float PlanetSeconds = 28.f;
	/** The Rest part: four shots, the white rising at its end; the length of Music.Intro.Rest before its tail. */
	inline constexpr float RestSeconds = 50.5f;
	/** The menu fades out this long before the planet camera moves: a cut from the menu would read as a glitch. */
	inline constexpr float MenuFadeSeconds = 0.6f;
	/** The planet camera turns from centring the planet to the dusk line here (one long move, no cut). */
	inline constexpr float PlanetTurnAt = 12.f;
	/** The white rises over WhiteSeconds from here in each part: after the last caption, so no words go under it. */
	inline constexpr float PlanetWhiteAt = 26.f;
	inline constexpr float RestWhiteAt = 48.2f;
	inline constexpr float WhiteSeconds = 2.f;
	/** A skip brings the white up this fast: quick, but not a flash. */
	inline constexpr float SkipWhiteSeconds = 0.5f;
	/** Each cut between shots dips to black this long either side: a hard cut between far places jars. */
	inline constexpr float CutDipSeconds = 0.3f;
	/** How far above the ground under it a shot's eye stays (cm): a layout guess must never put the lens in a hill. */
	inline constexpr float MinClearance = 250.f;
	/** The 60-90 s the user's note asked for; the script is held to it. */
	inline constexpr float ShortestTotal = 60.f;
	inline constexpr float LongestTotal = 90.f;

	// --- Names ---

	/** "NewGameIntro": the scene's name (the missions hear Scene.NewGameIntro, nothing waits for it). */
	AI_LOOTER_SHOOTER_API FName SceneName();
	/** "Intro.Seen": the campaign's story flag once it has played, been skipped or passed. */
	AI_LOOTER_SHOOTER_API FName SeenFlag();
	/** The line set's asset path (DA_Lines_NewGameIntro). */
	AI_LOOTER_SHOOTER_API FString LinesAssetPath();
	/** "MenuPlanet": the tag PLANET's planet actor wears so the shot finds it (else by name: Teropa, Planet). */
	AI_LOOTER_SHOOTER_API FName PlanetTag();

	// --- Rules ---

	/** Looter.Intro: 1 plays it (the default), 0 passes it (recorded seen). */
	AI_LOOTER_SHOOTER_API bool IsOn();
	/** Looter.Intro.Letterbox: each bar's share of the screen's height. */
	AI_LOOTER_SHOOTER_API float LetterboxFraction();
	/** Due in Campaign: the story has begun (the first cast-off), the cold open is still to come, the intro not seen. */
	AI_LOOTER_SHOOTER_API bool IsDue(const FCampaignRecord& Campaign);

	// --- The script ---

	/** The narration as written (11 lines, no speaker: narration shows with Subtitles off too). */
	AI_LOOTER_SHOOTER_API const TArray<FStoryLine>& ScriptLines();
	/** The words to show: the line set's when it has every line, else ScriptLines; resolved for the played hero. */
	AI_LOOTER_SHOOTER_API TArray<FStoryLine> Lines();
	/** The planet part's captions (Westward's too), from its start. */
	AI_LOOTER_SHOOTER_API const TArray<FIntroCaption>& PlanetCaptions();
	/** The Rest part's captions, from its own start. */
	AI_LOOTER_SHOOTER_API const TArray<FIntroCaption>& RestCaptions();
	/** A place card's name and the line under it (the hero's card is the caller's). */
	AI_LOOTER_SHOOTER_API FText CardTitle(EIntroCard Card);
	AI_LOOTER_SHOOTER_API FText CardSub(EIntroCard Card);
	/** The Rest scene's length: RestSeconds, after PlanetSeconds of Westward when it opens with it. */
	AI_LOOTER_SHOOTER_API float RestSceneSeconds(bool bWestward);

	// --- The shots (NewGameIntroShots.cpp) ---

	/** The Rest scene's moves, back to back from 0 (Westward first when asked), in Lvl_RansomsRest's layout cm. */
	AI_LOOTER_SHOOTER_API TArray<FIntroMove> RestMoves(bool bWestward);
	/** The last shot framed on the grave: from past its foot to the headboard (Look), the Keeper's frosted grave beside. */
	AI_LOOTER_SHOOTER_API FIntroMove GraveMove(const FVector& Look, const FVector& Foot, float Start, float Duration);
	/** Smootherstep: every move eases in and out. */
	AI_LOOTER_SHOOTER_API float Ease(float Alpha);
	/** Where a move puts the camera at Alpha (0-1 through it). */
	AI_LOOTER_SHOOTER_API FIntroView ViewAlong(const FIntroMove& Move, float Alpha);
	/** How far to raise a whole move so its eye keeps MinClearance over the ground all along it (0 when it does). */
	AI_LOOTER_SHOOTER_API float ClearanceLift(const UWorld* World, const FIntroMove& Move);
	/** The planet shot at Time seconds into the part, from the menu's view Base. */
	AI_LOOTER_SHOOTER_API FIntroView PlanetView(const FIntroView& Base, const FPlanetFrame& Planet, float Time);
	/** The menu world's planet and sun, if it has them. */
	AI_LOOTER_SHOOTER_API FPlanetFrame FindPlanet(const UWorld* World);
	/** The menu world is the planet scene: a menu world with a planet, or the map Lvl_MenuTeropa. */
	AI_LOOTER_SHOOTER_API bool IsPlanetWorld(const UWorld* World);

	// --- Playing the Rest part (NewGameIntroRest.cpp) ---

	/**
	 * As the story's first level opens (UColdOpenSubsystem::BeginIfDue, before the cold open): plays the Rest part when due
	 * and scenes are on; Then runs once it has ended (played or skipped), the player still held under the white for the
	 * cold open. False when it doesn't play: passed (Looter.Intro 0, skipped whole in the menu: recorded seen) or not due;
	 * the caller goes on as before.
	 */
	AI_LOOTER_SHOOTER_API bool PlayRestIfDue(UWorld& World, TFunction<void()> Then);

	/** Plays the Rest part now (Looter.Intro.Play): AfterEnd runs once it ends; nothing is recorded. */
	AI_LOOTER_SHOOTER_API bool PlayRest(UWorld& World, bool bWestward, TFunction<void()> AfterEnd);

	/** The Rest part as a scene: it holds the player after its end (the cold open takes them over). */
	AI_LOOTER_SHOOTER_API FScenePlay MakeRest(USceneSubsystem& Scenes, const FIntroRestSetup& Setup);
}

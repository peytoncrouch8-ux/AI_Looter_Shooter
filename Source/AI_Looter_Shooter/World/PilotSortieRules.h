#pragma once

#include "CoreMinimal.h"

/**
 * Where the Lamplighter is in Main 6, "The Pilot Light" (APilotSortie; Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section
 * 10): from the ledge to the barge and home again. Not saved: a level works it out from Main 6's step (PilotLight::PhaseForStory).
 */
enum class EPilotPhase : uint8
{
	/** Not Main 6 (or it's done): she's the crew's ship on the Mooring Ledge, nothing to fly. */
	Idle,
	/** Main 6's first step: moored and anchored, the winch offering "Cast off", Junie's launchers on her starboard rail. */
	WaitCastOff,
	/** Off the ledge: the anchor up, a rise clear of the cliff and a turn onto her course. */
	LiftOff,
	/** Her course east over the farm and the town toward the barge; the Company's two tenders come at her on it. */
	Cruise,
	/** From the course's end to her standoff beside the barge, about 15 m off its side. */
	Approach,
	/** Main 6's third step: holding off the barge while the player fires the two launchers. */
	Alongside,
	/** Both grapples bit: the chains haul her into the berth. */
	Pulling,
	/** Steps 3-6: in the berth, dead still, the plank across to the barge's deck. */
	Boarded,
	/** Step 7: the grapples cut, she hangs under the broken gangway, waiting for the jump. */
	Jump,
	/** BargeFall plays from her deck. */
	Falling,
	/** Home on her course reversed. */
	Home,
	/** Down onto the ledge at Moored. */
	Settling,
};

/**
 * Main 6's rules apart from the world (the contract's names, exact: Docs/Work/2026-10-31-Wardens/main67_src/CONTRACT67.md),
 * so the tests check them and the barge (W3) and the missions (W1) share one set of names and numbers.
 */
namespace PilotLight
{
	// --- Names ---

	/** "Main6": The Pilot Light keeps the old mission id (FName(PilotLight::Mission), or MissionId()). */
	inline constexpr const TCHAR* Mission = TEXT("Main6");
	AI_LOOTER_SHOOTER_API FName MissionId();
	/** "Pilot.TendersDown": both Company tenders fallen and every boarder dead (step 1). */
	AI_LOOTER_SHOOTER_API FName TendersDownEvent();
	/** "Pilot.DrumJammed": the barge's hose drum jammed (step 3's optional; the barge sets it). */
	AI_LOOTER_SHOOTER_API FName DrumJammedFlag();
	/** "Pilot.BargeFell": BargeFall has played (the barge sets it as it falls; the wreck shows from then). */
	AI_LOOTER_SHOOTER_API FName BargeFellFlag();
	/** "Lamplighter_GrappleLauncher": each of Junie's two launchers on her starboard rail (step 2, held, count 2). */
	AI_LOOTER_SHOOTER_API FName LauncherTag();
	/** "Lamplighter_Deck": the marker on her deck step 7's Reach looks for. */
	AI_LOOTER_SHOOTER_API FName DeckMarkerTag();
	/** "Barge_Pilot": the walkable barge over the chapel knoll (W3's ABargeBoarding); the Great Lantern leans toward it in
	 *  the sortie, Abel's soul burning aboard. */
	AI_LOOTER_SHOOTER_API FName BargeTag();

	// --- Main 6's steps (from 0; the contract's table) ---

	inline constexpr int32 CastOffStep = 0;
	inline constexpr int32 TendersStep = 1;
	inline constexpr int32 GrappleStep = 2;
	inline constexpr int32 DeckStep = 3;
	inline constexpr int32 ManifestStep = 4;
	inline constexpr int32 FurnaceStep = 5;
	inline constexpr int32 CageStep = 6;
	inline constexpr int32 EscapeStep = 7;
	/** The step past the last: ready to turn in to Holloway. */
	inline constexpr int32 ReadyStep = 8;

	// --- Line sets (W1's create_story_lines.py; /Game/Data/Story/<name>) ---

	AI_LOOTER_SHOOTER_API FName CastOffLines();
	AI_LOOTER_SHOOTER_API FName TendersLines();
	AI_LOOTER_SHOOTER_API FName TenderDownLines(int32 Fallen);
	AI_LOOTER_SHOOTER_API FName BoardersLines();
	AI_LOOTER_SHOOTER_API FName LaunchersLines();
	AI_LOOTER_SHOOTER_API FName FlickerLines();
	AI_LOOTER_SHOOTER_API FName PlankLines();
	AI_LOOTER_SHOOTER_API FName JumpLines();

	// --- Where a level begins her ---

	/** The phase's name for the log and the console. */
	AI_LOOTER_SHOOTER_API const TCHAR* PhaseName(EPilotPhase Phase);

	/** A phase by its name ("Grappled" and "Alongside" read the same as the console's words), or false. */
	AI_LOOTER_SHOOTER_API bool ParsePhase(const FString& Words, EPilotPhase& OutPhase);

	/**
	 * Where a level (or a load) finds her from Main 6: not running or ready to turn in, she's moored (Idle); its first step,
	 * waiting to cast off; the tenders' or the launchers' step, flying again from the lift (a save mid-air flies again); the
	 * barge's steps, in the berth; the escape, under the gangway.
	 */
	AI_LOOTER_SHOOTER_API EPilotPhase PhaseForStory(bool bRunning, int32 Step, bool bReady);

	/** She's in the air in Phase (saves held, the player kept aboard where it's hers). */
	AI_LOOTER_SHOOTER_API bool IsFlying(EPilotPhase Phase);

	/** The player is hers to keep on deck in Phase (not while they're over on the barge, nor mid-jump). */
	AI_LOOTER_SHOOTER_API bool KeepsPlayerAboard(EPilotPhase Phase);

	// --- Numbers (cm, s, cm/s) ---

	/** The lift: this long, this high over Moored before she turns away from the cliff. */
	inline constexpr float LiftSeconds = 6.5f;
	inline constexpr float LiftRise = 900.f;
	/** Her pace out: a cruise over the farm and the town, slowed while the tenders fight her (a bigger target, steadier aim). */
	inline constexpr float CruiseSpeed = 560.f;
	inline constexpr float FightSpeed = 320.f;
	inline constexpr float HomeSpeed = 780.f;
	/** The tenders are called this share of her course out (at least TenderCallLeast along it). */
	inline constexpr float TenderCallShare = 0.16f;
	inline constexpr float TenderCallLeast = 2500.f;
	/** From the course's end to her standoff; the haul into the berth; the glide to the jump; the settle onto the ledge. */
	inline constexpr float ApproachSeconds = 9.f;
	inline constexpr float PullSeconds = 3.2f;
	inline constexpr float JumpGlideSeconds = 5.f;
	inline constexpr float SettleSeconds = 7.f;
	/** The standoff: this far off her berth, away from the barge (her port), and this much higher. */
	inline constexpr float StandoffOut = 1500.f;
	inline constexpr float StandoffUp = 150.f;
	/** Main 6's Dredgers' level (the barge's fights: level 6; the tenders' boarders the same). */
	inline constexpr int32 FightLevel = 6;
	/** A tender's gas-bag's damage at level 1 (grown with the area's enemies' growth to FightLevel). */
	inline constexpr float BagBaseHealth = 230.f;
	/** The jump's catch: falling this far under her deck beside her, Hob has them. */
	inline constexpr float CatchBelow = 150.f;
	/** The player standing aboard this long ends the jump (the fall plays). */
	inline constexpr float LandedSeconds = 0.3f;

	/** Her speed toward Wanted with Remaining cm of course left: never faster than she can stop in (her bag's lazy pull). */
	AI_LOOTER_SHOOTER_API float SpeedFor(float Remaining, float Wanted);

	/** A gas-bag's damage at Level (BagBaseHealth grown as the area's enemies grow). */
	AI_LOOTER_SHOOTER_API float BagHealthAt(int32 Level);

	/** An eased glide from From to To at Alpha (0-1): place, turn, and scale kept from From. */
	AI_LOOTER_SHOOTER_API FTransform Glide(const FTransform& From, const FTransform& To, float Alpha);

	/**
	 * The lift off the ledge Alpha (0-1) through: straight up first (clear of the cliff and the gangway), then out and round
	 * onto the course's start, Start; a little nose-up and a list as the bag takes her weight.
	 */
	AI_LOOTER_SHOOTER_API FTransform LiftPose(const FTransform& Moored, const FTransform& Start, float Alpha);

	/** Her standoff beside the barge: StandoffOut off her berth to her port (away from it), StandoffUp higher. */
	AI_LOOTER_SHOOTER_API FTransform StandoffFor(const FTransform& Berth);

	/** The settle onto the ledge Alpha through: a glide down, the last metre slow, level at the end. */
	AI_LOOTER_SHOOTER_API FTransform SettlePose(const FTransform& From, const FTransform& Moored, float Alpha);

	/**
	 * The jump's catch, in her own frame (Local, cm from her keel's middle, her starboard +Y toward the barge): beside her or
	 * under her (not over on the barge's side), fallen CatchBelow under her deck, and falling.
	 */
	AI_LOOTER_SHOOTER_API bool IsCaughtBelow(const FVector& Local, bool bFalling, float DeckTop, float RailOut);
}

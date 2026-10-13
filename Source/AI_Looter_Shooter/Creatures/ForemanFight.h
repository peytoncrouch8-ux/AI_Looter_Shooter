#pragma once

#include "CoreMinimal.h"
#include "Bosses/BossTypes.h"
#include "Math/RandomStream.h"
#include "Story/StoryLine.h"

/** The Foreman's numbers that never change in play (Docs/Areas/RansomsRest_Expansion.md, section 6 "The Providence"). */
namespace Foreman
{
	/** His size against an Unpaid's, as seen: his BodyScale is this over his rank's own size (Soulfed's 1.4). */
	inline constexpr float Size = 1.5f;
	inline constexpr float RankSize = 1.4f;

	/** His lair's id (AEncounterSpawner::SpawnerId and LegendaryId; Back Pay's KillNamed and his vigil stone's BossKey). */
	inline const FName LairId(TEXT("Foreman"));

	/** The stope's lanterns his dark blows out (actor tags), and their glowing parts besides their lights (component tags). */
	inline const FName LanternTag(TEXT("StopeLantern"));
	inline const FName LanternGlowTag(TEXT("LanternGlow"));

	/** Where the shift rises: the fall's rubble (an actor so tagged in the stope); without one, the wall he digs at. */
	inline const FName RubbleTag(TEXT("Foreman_Rubble"));

	/** The shift: three dead miners each call, at two thirds and one third of his health. */
	inline constexpr int32 ShiftSize = 3;
	inline constexpr int32 NumShifts = 2;

	/** His phases' shares of health: the shift at 66%, the dark at 50%, the shift again at 33%. */
	inline constexpr float ShiftShare = 0.66f;
	inline constexpr float DarkShare = 0.5f;
	inline constexpr float LastShiftShare = 0.33f;
	inline constexpr int32 ShiftPhase = 1;
	inline constexpr int32 DarkPhase = 2;
	inline constexpr int32 LastShiftPhase = 3;

	/** His first fuse comes this long after his entrance, his first roof this long after his shift is called (s). */
	inline constexpr float FirstFuseDelay = 3.f;
	inline constexpr float FirstRoofDelay = 6.f;

	/** His carbide lamp burns this much brighter than a dead miner's. */
	inline constexpr float OwnLampBrightness = 1.6f;
}

/**
 * How the Foreman fights in each phase of his boss fight (his UBossComponent's), as plain data and rules so the tests
 * check them without a fight:
 *  - "Fire in the Hole" (100-66%): his pick handle up close, and fuse sticks lobbed at a player who keeps off (the fuse lit
 *    in his raised fist, sparks and a hiss for 2 s with his coal open, then the lob, a moment's fuse on the ground over a
 *    ring of cracks, and a 6 m burst).
 *  - "The Shift" (66-50%): three dead miners rise out of the fall's rubble; from now on he strikes the roof every 18 s or
 *    so (dust trickling on four marked spots for 2 s, then rock falls there, splintering the timber props it lands on), his
 *    coal open for 3 s after each strike.
 *  - "Lights Out" (50-33%): the stope's lanterns blow out for 12 s: his lamp, the miners' lamps and their coals are all the
 *    light there is (the exposure lifts, so it reads dark, not black).
 *  - "Last Shift" (33-0%): three more dead miners; he fuses and strikes the roof more often, and swings quicker.
 */
namespace ForemanFight
{
	/** How he fights in one phase. */
	struct FPace
	{
		/** Seconds from one fuse stick to the next. */
		float FuseCooldown = 7.f;
		/** He strikes the roof (below 66%), every this many seconds or so. */
		bool bRoof = false;
		float RoofEvery = 18.f;
		/** His chase and his swing's wind-up against his own. */
		float ChaseScale = 1.f;
		float SwingWindupScale = 1.f;
	};

	/** His four phases, highest share first: their names on his bar. */
	AI_LOOTER_SHOOTER_API TArray<FBossPhase> MakePhases();

	/** How he fights in Phase (0-3; past the last, the last's). */
	AI_LOOTER_SHOOTER_API FPace PaceFor(int32 Phase);

	/** His bar's show: his title, his cries, the shakes, his death's slow beat and the bar's last word. */
	AI_LOOTER_SHOOTER_API FBossShow MakeShow();

	/** His coal (open only in its windows) staggers him: 6% of his health in crits within a few seconds. */
	AI_LOOTER_SHOOTER_API FBossStagger MakeStagger();

	/** His loot's shower (the Legendary table's guns and ammo, two boxes more), after his last word. */
	AI_LOOTER_SHOOTER_API FBossLootShowerSettings MakeLootShower();

	/** Seconds he'll go with nobody to fight (the player out of his stope) before his fight stands down and he goes home. */
	inline constexpr float StandDownSeconds = 8.f;

	/** His coal is open: lighting a fuse, within RoofOpenSeconds after a roof strike (SinceRoofStrike), or staggered. */
	AI_LOOTER_SHOOTER_API bool IsCoalOpen(bool bLightingFuse, float SinceRoofStrike, float RoofOpenSeconds, bool bStaggered);

	/** A fuse stick's burst at Distance (cm) from its heart to a body's side: 1 in its core, EdgeShare at Radius, 0 past it. */
	AI_LOOTER_SHOOTER_API float BurstFalloff(float Distance, float Radius, float CoreRadius, float EdgeShare);

	/**
	 * Where a roof strike brings rock down (flat; Z the player's): on the prop he struck (if any), on the player's spot, on
	 * the prop the player is using for cover (if any, within reach of them), and round the player to make up Count, each at
	 * least Spacing from the others, all inside the arena (ArenaRadius round ArenaCenter). Random picks the fill's angles.
	 */
	AI_LOOTER_SHOOTER_API TArray<FVector> RoofSpots(const FVector& Player, const TOptional<FVector>& StruckProp, const TOptional<FVector>& CoverProp,
		const FVector& ArenaCenter, float ArenaRadius, int32 Count, float Spacing, float FillDistance, FRandomStream& Random);

	/** Who says his lines, and the lines (Back Pay's): his fight's start, the shift called, a fuse, the dark, his last word. */
	AI_LOOTER_SHOOTER_API FText SpeakerName();
	AI_LOOTER_SHOOTER_API FStoryLine IntroLine();
	AI_LOOTER_SHOOTER_API FStoryLine ShiftLine();
	AI_LOOTER_SHOOTER_API FStoryLine FuseLine();
	AI_LOOTER_SHOOTER_API FStoryLine DarkLine();
	AI_LOOTER_SHOOTER_API FStoryLine LastWord();
}

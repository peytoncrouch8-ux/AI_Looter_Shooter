#pragma once

#include "CoreMinimal.h"

class AActor;
class APawn;
class UWorld;

/** What an enemy shot sounds like as it passes close by the player's head. */
enum class ENearMissSound : uint8
{
	/** By who fired it (NearMiss::SoundOf): venom hisses, the rest whiz. */
	Auto,
	/** Silent. */
	None,
	/** A pellet or a ball of buckshot: a sharp whiz-crack past the ear. */
	Whiz,
	/** A glob of venom: a wet, sputtering hiss going by. */
	Hiss
};

/**
 * Enemy shots that miss the player by a little (Abel's buckshot, the Gravemother's spit, the Spitter's glob, any pellet of
 * UEnemyProjectileSubsystem) are heard going by: once a shot, when its path comes within Radius of the player's head
 * (never for a shot of the player's own, and never one that hits them: the hit is heard). The cue plays where the shot
 * came nearest, so it's heard from the side it passed.
 */
namespace NearMiss
{
	/** How near a shot must pass the head to be heard (cm). */
	inline constexpr float Radius = 150.f;

	/** A shot this near where it was fired isn't "passing" (point blank): it's heard as the shot itself. */
	inline constexpr float MinFlight = 200.f;

	/** Seconds between two fly-by sounds (a volley passing together is one whiz, not five). */
	inline constexpr double SoundGap = 0.08;

	/** How near the path From -> To comes to Head (cm), and the point of the path nearest it. */
	AI_LOOTER_SHOOTER_API float ClosestApproach(const FVector& From, const FVector& To, const FVector& Head, FVector& OutNearest);

	/** The sound Shooter's shots make passing: a spider's or a slime's venom hisses, everything else whizzes. */
	AI_LOOTER_SHOOTER_API ENearMissSound SoundOf(const AActor* Shooter);

	/** The cue of a sound (Weapon.NearMiss.Whiz, .Hiss); none for None (Auto counts as a whiz). */
	AI_LOOTER_SHOOTER_API FName CueOf(ENearMissSound Sound);

	/** Where a listener's head is: its view point. */
	AI_LOOTER_SHOOTER_API FVector HeadOf(const APawn& Listener);

	/**
	 * The player a shot moving From -> To this step went by: the pawn of a local player that is alive, didn't fire it, and
	 * whose head the path passed within Radius of (Flown cm into the shot's flight at From, so a point-blank shot is
	 * left out). Null when it went by nobody. OutNearest is where it came nearest.
	 */
	AI_LOOTER_SHOOTER_API APawn* FindPassed(const UWorld& World, const FVector& From, const FVector& To, float Flown, const AActor* Shooter,
		FVector& OutNearest);

	/** Plays a fly-by's cue where the shot came nearest (Auto: by its shooter). */
	AI_LOOTER_SHOOTER_API void Play(const UObject* WorldContext, ENearMissSound Sound, const AActor* Shooter, const FVector& Nearest);

	/**
	 * For a projectile of its own (the Spitter's glob): if this step From -> To went by the player and it hasn't yet
	 * (bInOutHeard), plays its sound and remembers it. True the step it's heard.
	 */
	AI_LOOTER_SHOOTER_API bool HearFlyBy(const UWorld& World, const FVector& From, const FVector& To, float Flown, const AActor* Shooter,
		ENearMissSound Sound, bool& bInOutHeard);
}

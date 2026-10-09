#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"
#include "Story/SpeakerPointComponent.h"

class AActor;

/**
 * The town's held breath, as plain rules (UTownLifeSubsystem plays them; the tests run them without a world). Ransom's Rest
 * is half empty since the saint went dark, and the half that stayed hides from the walking corpse behind its shutters
 * (Docs/Areas/RansomsRest.md: townsfolk are heard, not seen). Each lived-in house is a household with its own sounds,
 * heard through its walls, rarely, as the player passes near: muffled voices, a cough, a music box, a latch, a creak, a
 * hush, Tilly's saw; and a dog barks far off now and then. At three townsfolk's doors (Pruitt's store, the north and
 * south cottages) somebody mutters a line as the player passes, shown as a caption, as the story goes on.
 *
 * The households, as the story has them (who lives where; nobody here is a named character but those already written):
 *  - Ransom: Grandma Delia's farmhouse. She lives alone and won't open the door: a creak, the latch, an old woman's cough.
 *  - Bright: Bright & Daughter, Tilly's shop. Tilly alone, at her work: a saw and a hammer, a creak, the latch.
 *  - Pruitt: Pruitt's General Store, the storekeeper's family: hushed voices, a cough, a hush, a creak, the latch.
 *  - CottageNorth: a couple and an old father: voices, the old man's cough, a creak, the latch.
 *  - CottageSouth: a mother and her small child: a music box, a hush, a few words, a creak, the latch.
 *  - Cottage: any other lived-in cottage: voices, a cough, a creak, the latch.
 */
namespace TownLifeRules
{
	/**
	 * A household: its id, the sounds heard from it (a cue listed twice comes twice as often), its voices' pitch, and
	 * whether a dog is kept there (the dog barking far off is a town family's; Delia and Tilly keep none).
	 */
	struct FHousehold
	{
		FName Id;
		TArray<FName> Sounds;
		float VoicePitch = 1.f;
		bool bKeepsDog = false;
	};

	/** Every household, and one by id (null: none so called). */
	AI_LOOTER_SHOOTER_API const TArray<FHousehold>& Households();
	AI_LOOTER_SHOOTER_API const FHousehold* FindHousehold(FName Id);

	/**
	 * The household of a lived-in house from its model, for a house whose lights name none: Delia's farmhouse, Tilly's shop,
	 * Pruitt's store, a settler's cottage (the generic one). None for anything else (the tutorial island's houses have
	 * nobody in them).
	 */
	AI_LOOTER_SHOOTER_API FName HouseholdForMesh(const FString& MeshName);

	/** The same for a house actor: its first static mesh's name. */
	AI_LOOTER_SHOOTER_API FName HouseholdForHouse(const AActor* House);

	/** Whether a cue is a voice (its pitch follows the household's), rather than a thing (a latch, a board, a saw). */
	AI_LOOTER_SHOOTER_API bool IsVoice(FName Cue);

	/** What the townsfolk behind a household's door mutter, by the story: the first topic that holds is said, a line at a time. */
	AI_LOOTER_SHOOTER_API TArray<FSpeakerTopic> MutterTopicsFor(FName Household);

	/** Seconds between looks round the player; how near a house is heard (cm); how near the dog's house must be at least and at most. */
	inline constexpr float CheckSeconds = 1.f;
	inline constexpr float HearRadius = 1200.f;
	inline constexpr float DogNear = 1800.f;
	inline constexpr float DogFar = 4000.f;

	/** The player is in town while within this of any house (cm): the dog barks only then. */
	inline constexpr float TownRadius = 4500.f;

	/** A house heard on the first pass in a while (AwaySeconds away), and on each look after while the player lingers. */
	inline constexpr float FirstChance = 0.45f;
	inline constexpr float AgainChance = 0.06f;
	inline constexpr float AwaySeconds = 40.f;

	/** The pauses after a house's sound (s): any house's, then that house's own; and the dog's. */
	inline constexpr float GapMin = 14.f;
	inline constexpr float GapMax = 22.f;
	inline constexpr float HouseRestMin = 45.f;
	inline constexpr float HouseRestMax = 80.f;
	inline constexpr float DogChance = 0.04f;
	inline constexpr float DogRestMin = 50.f;
	inline constexpr float DogRestMax = 100.f;

	/** Mutters: how likely on a look once one may come, the pause after one (s), and a door's own rest. */
	inline constexpr float MutterChance = 0.6f;
	inline constexpr float MutterGapMin = 20.f;
	inline constexpr float MutterGapMax = 30.f;
	inline constexpr float DoorRest = 75.f;

	/** A voice's level and the house's own sounds' (the cues are quiet already; a door's mutter is heard under its caption). */
	inline constexpr float MutterVoiceVolume = 0.8f;
}

/** The town's clock: when the next sound may come, from any house and from each, and whether the player has been away. */
struct AI_LOOTER_SHOOTER_API FTownLifeClock
{
	/** The player is near Household now (they'll have been away if they weren't near it for AwaySeconds). */
	void MarkNear(FName Household, double Now);

	/** Whether the player comes to Household fresh: never near it, or away from it for AwaySeconds before Now. */
	bool IsFreshVisit(FName Household, double Now) const;

	/** Whether Household makes a sound on this look: the pauses over, and its chance (a fresh visit's, or a lingering one's). */
	bool WantsSound(FName Household, double Now, bool bFresh, FRandomStream& Random) const;

	/** Its sound to play: any of its own, never the one it made last when it has another. */
	FName PickSound(const TownLifeRules::FHousehold& Household, FRandomStream& Random);

	/** Household made a sound at Now: the pauses start. */
	void Played(FName Household, double Now, FRandomStream& Random);

	/** The dog: whether it barks on this look, and its rest after. */
	bool WantsDog(double Now, FRandomStream& Random) const;
	void DogBarked(double Now, FRandomStream& Random);

	/** Mutters: whether one may come now (the pause over), and the pause after one. */
	bool MayMutter(double Now) const { return Now >= NextMutter; }
	void Muttered(double Now, FRandomStream& Random);

	double NextSound = 0.0;
	double NextDog = 0.0;
	double NextMutter = 0.0;
	TMap<FName, double> HouseNext;
	TMap<FName, double> LastNear;
	TMap<FName, int32> LastPick;
};

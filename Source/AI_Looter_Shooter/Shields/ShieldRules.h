#pragma once

#include "CoreMinimal.h"
#include "Shields/ShieldTypes.h"

class UShieldDefinition;
struct FShieldTuning;

/**
 * The player's ward as plain rules, no world (the tests run every one): rolling a ward, its numbers, how it soaks a hit and
 * breaks, how it refills, what its traits do, and how two compare. UPlayerShieldComponent plays them; the numbers and why
 * are in Docs/Work/2026-10-22-Next/SHIELD.md.
 */
namespace ShieldRules
{
	// --- Rolling a ward ---

	/** How a build leans the definition's numbers (times each). */
	struct FBuildLean
	{
		float Capacity = 1.f;
		float Delay = 1.f;
		float Rate = 1.f;
	};
	AI_LOOTER_SHOOTER_API FBuildLean LeanOf(EShieldBuild Build);

	/** The build a seed picks: a third each. */
	AI_LOOTER_SHOOTER_API EShieldBuild BuildFor(int32 Seed);

	/** The trait a seed picks for a ward of Rarity: none below Rare; then the rarity's TraitChance, each trait alike. */
	AI_LOOTER_SHOOTER_API EShieldTrait TraitFor(const UShieldDefinition& Definition, EWeaponRarity Rarity, int32 Seed);

	/** Last Breath's element from the seed: Ember, Soul-light or Grave-salt alike. */
	AI_LOOTER_SHOOTER_API ELooterElement NovaElementFor(int32 Seed);

	/**
	 * A ward's numbers: base x rarity x level x build x (1 +- the definition's variance, seeded):
	 *   capacity = BaseCapacity x rarity x (1 + CapacityPerLevel x (Level - 1)) x lean x var
	 *   delay    = BaseRechargeDelay x rarity x lean x var
	 *   rate     = capacity x BaseRechargeShare x rarity x lean x var   (points a second)
	 */
	AI_LOOTER_SHOOTER_API FShieldStats ComputeStats(const UShieldDefinition& Definition, EWeaponRarity Rarity, int32 Level, int32 Seed,
		EShieldBuild Build);

	/** ComputeStats for a ward as it is (its definition's; zeros without one). */
	AI_LOOTER_SHOOTER_API FShieldStats ComputeStats(const FShieldInstanceData& Ward);

	/** A ward whose build, trait and element come from its seed, its stats computed. */
	AI_LOOTER_SHOOTER_API FShieldInstanceData Make(UShieldDefinition* Definition, EWeaponRarity Rarity, int32 Level, int32 Seed);

	/** A ward with everything given (the dev command, tests), its stats computed. A non-LastBreath trait has no element. */
	AI_LOOTER_SHOOTER_API FShieldInstanceData MakeWith(UShieldDefinition* Definition, EWeaponRarity Rarity, int32 Level, int32 Seed,
		EShieldBuild Build, EShieldTrait Trait, ELooterElement Element = ELooterElement::None);

	/** The first gun's starting ward: a plain Common ward box of Level (Standard build, no trait). */
	AI_LOOTER_SHOOTER_API FShieldInstanceData Starter(UShieldDefinition* Definition, int32 Level);

	/** A rarity by the definition's weights, luck shifting them up as a gun's (UWeaponRollLibrary::RollRarityWith). */
	AI_LOOTER_SHOOTER_API EWeaponRarity RollRarity(const UShieldDefinition& Definition, float Luck, FRandomStream& Random);

	/** A gun pick turns into a ward: Roll (0-1) under Share. */
	inline bool IsWardDrop(float Roll, float Share) { return Share > 0.f && Roll < Share; }

	// --- The live numbers (a ward's stats times the tuning's scales) ---

	AI_LOOTER_SHOOTER_API float LiveCapacity(const FShieldStats& Stats, const FShieldTuning& Tuning);
	AI_LOOTER_SHOOTER_API float LiveDelay(const FShieldStats& Stats, const FShieldTuning& Tuning);
	AI_LOOTER_SHOOTER_API float LiveRate(const FShieldStats& Stats, const FShieldTuning& Tuning);

	// --- Soaking a hit ---

	struct FShieldSoak
	{
		/** The ward's charge after the hit. */
		float ChargeAfter = 0.f;
		/** What the ward took. */
		float Absorbed = 0.f;
		/** What reaches health. */
		float ToHealth = 0.f;
		/** The hit emptied a ward that had charge. */
		bool bBroke = false;
	};

	/**
	 * A hit of Damage on a ward holding Charge: all of it while the ward holds more; the hit that empties it breaks it, and
	 * only Spill of its overflow reaches health (FShieldTuning::BreakSpill); an empty ward takes nothing.
	 */
	AI_LOOTER_SHOOTER_API FShieldSoak Soak(float Charge, float Damage, float Spill);

	// --- Refilling ---

	/** What a refill step did. */
	enum class ERefill : uint8
	{
		None,
		/** The wait ran out: it starts filling. */
		Started,
		/** It reached full this step. */
		Filled,
	};

	/** A ward's charge and its wait: Hurt starts the wait over, Step runs the wait, then fills at the rate. */
	struct AI_LOOTER_SHOOTER_API FShieldCharge
	{
		float Charge = 0.f;
		/** Seconds since the last hit (held at the delay once it's met). */
		float Quiet = 0.f;
		/** Filling now (the wait met, not full). */
		bool bFilling = false;

		/** Any hit, soaked or not: the whole wait again. */
		void Hurt();
		/** Moves on DeltaSeconds and says what happened (a step can start and fill: Filled wins). */
		ERefill Step(float DeltaSeconds, float InCapacity, float InDelay, float InRate);
		bool IsFull(float InCapacity) const;
		/** Full at once (a load, a respawn, a ward worn from the start). */
		void Fill(float InCapacity);
	};

	// --- Traits ---

	/** The trait's strength for a ward of Rarity (its definition's TraitStrength: Rare 1, Epic 1.25, Legendary 1.6). */
	AI_LOOTER_SHOOTER_API float TraitStrength(const UShieldDefinition* Definition, EWeaponRarity Rarity);

	/**
	 * Last Breath's damage to a body Distance (cm, its nearest side) from the player: NovaDamage x Strength x LevelScale (the
	 * player's level's enemy scale), whole within NovaCore, falling straight to NovaEdge at NovaRadius, nothing past it.
	 */
	AI_LOOTER_SHOOTER_API float NovaDamageAt(float Distance, float Strength, float LevelScale, const FShieldTuning& Tuning);

	/** Barbwire's cut on a biter: BarbShare x the ward's capacity x Strength. */
	AI_LOOTER_SHOOTER_API float BarbDamage(float InCapacity, float Strength, const FShieldTuning& Tuning);

	/** Lodestone: the chance a hit gives rounds (LodestoneChance x Strength, at most 0.9). */
	AI_LOOTER_SHOOTER_API float LodestoneOdds(float Strength, const FShieldTuning& Tuning);

	/** Lodestone: the rounds a hit gives for a gun of MagazineSize (at least 1; none without a gun). */
	AI_LOOTER_SHOOTER_API int32 LodestoneRounds(int32 MagazineSize, float Strength, const FShieldTuning& Tuning);

	/** Harvest: what a kill puts back (HarvestShare x Strength of the capacity, at most all of it). */
	AI_LOOTER_SHOOTER_API float HarvestRestore(float InCapacity, float Strength, const FShieldTuning& Tuning);

	/** Saint's Grace answers: ready, the ward broken (empty), health under GraceBelow and alive. */
	AI_LOOTER_SHOOTER_API bool GraceAnswers(bool bReady, float WardCharge, float HealthShare, const FShieldTuning& Tuning);

	/** Saint's Grace's flare: the ward back to GraceRefill x Strength of its capacity (at most full). */
	AI_LOOTER_SHOOTER_API float GraceCharge(float InCapacity, float Strength, const FShieldTuning& Tuning);

	// --- Comparing ---

	/** Better (1), worse (-1) or about the same (0, within 3% of capacity and refill), Than may be null (anything is better). */
	AI_LOOTER_SHOOTER_API int32 Compare(const FShieldInstanceData& Ward, const FShieldInstanceData* Than);

	/** One ward's worth to compare by: its capacity, plus what it refills in the first two seconds after its wait. */
	AI_LOOTER_SHOOTER_API float Worth(const FShieldStats& Stats);

	/** A ward's identity for the screens (NEW marks, a card sliding in): its definition, rarity, level, seed and build. */
	AI_LOOTER_SHOOTER_API uint32 Identity(const FShieldInstanceData& Ward);
}

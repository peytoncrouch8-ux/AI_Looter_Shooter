#pragma once

#include "CoreMinimal.h"
#include "Elements/ElementTuning.h"
#include "Elements/LooterElement.h"
#include "Weapons/WeaponTypes.h"

class AActor;
class UClass;
class UDamageType;

/** What a creature is made of, as the elements see it (ElementRules::BodyOf). */
enum class EElementBody : uint8
{
	/** Anything the table doesn't name: neither weak nor resisting. */
	Other,
	/** Spiders: the plain spider, the Spitter, the Gravemother (flesh and chitin). */
	Chitin,
	/** Slimes. */
	Gel,
	/** The Unpaid: a coal burning in a shroud of the dead. */
	Unpaid,
	/** The rest of the risen dead: dead miners, the Bell-Ringer, the Foreman, Abel and what he raises. */
	Undead
};

/** Where a creature stands against an element: the weakness table's answer. */
enum class EElementAffinity : uint8
{
	Neutral,
	/** Weak to it: the effect's damage and chance x Looter.Element.Weak (1.75). */
	Weak,
	/** Resists it: x Looter.Element.Resist (0.5). */
	Resist
};

/** A target as the elements see it. */
struct FElementTarget
{
	EElementBody Body = EElementBody::Other;
	/** A spirit: a Warded or Phasing trait, or the Ethereal tier (Grave-salt's prey). */
	bool bSpirit = false;
	/** A boss (or a creature built round a boss fight): statuses last half as long, no slow. */
	bool bBoss = false;
};

/** The sounds an element's status makes (Element.<Element>.<Event>, GUNS-SOUND's cues). */
enum class EElementCue : uint8
{
	Apply,
	Tick,
	Kill
};

/**
 * The elements' plain rules (numbers in, numbers out; the tests drive them without a world): the weakness table, the
 * chance a hit sets its status, the words and colours, and the damage types. The table (Docs/Pipeline.md "Loot: elements"):
 *   Ember      weak: spiders, the Spitter, slimes, the Gravemother        resists: the Unpaid
 *   Soul-light weak: the Unpaid, dead miners, the Bell-Ringer, Abel's risen  resists: slimes
 *   Grave-salt weak: spirits (Warded, Phasing, Ethereal), an Unpaid's coal   resists: spiders
 * A weakness wins over a resistance (a Warded spider is a spirit before it's a spider to the salt).
 */
namespace ElementRules
{
	/** The body of a creature class (a bestiary page's), or Other. */
	AI_LOOTER_SHOOTER_API EElementBody BodyOf(const UClass* CreatureClass);

	/** The target as it stands now: its body, whether it's a spirit (its traits and tier), whether it's a boss. */
	AI_LOOTER_SHOOTER_API FElementTarget TargetOf(const AActor* Target);

	/**
	 * Where Target stands against Element. bCritical: the hit was critical (on an Unpaid that is its coal, which Grave-salt
	 * finds weak).
	 */
	AI_LOOTER_SHOOTER_API EElementAffinity Affinity(ELooterElement Element, const FElementTarget& Target, bool bCritical);

	/** The weakness table's multiplier: Weak, Resist or 1. */
	AI_LOOTER_SHOOTER_API float AffinityScale(EElementAffinity Affinity, const FElementTuning& Tuning);

	/** The chance a hit of this kind of gun sets its element (before the weakness): the kind's, plus its rarity's steps. */
	AI_LOOTER_SHOOTER_API float GunChance(EWeaponKind Kind, EWeaponRarity Rarity, const FElementTuning& Tuning);

	/** That chance on a target: times the weakness, at most ChanceMax. */
	AI_LOOTER_SHOOTER_API float HitChance(float Chance, EElementAffinity Affinity, const FElementTuning& Tuning);

	/** One of the dead (an Unpaid or the rest of the risen): a smite leaps between them. */
	AI_LOOTER_SHOOTER_API bool IsTheDead(EElementBody Body);

	/** "EMBER", "SOUL-LIGHT", "GRAVE-SALT" (cards and labels); empty for None. */
	AI_LOOTER_SHOOTER_API FString Word(ELooterElement Element);

	/** What it does, in a line for the loot card. Empty for None. */
	AI_LOOTER_SHOOTER_API FString Description(ELooterElement Element);

	/** The element a CSV cell names ("Ember", "SoulLight", "Soul-light", "GraveSalt", "Grave-salt"); None for anything else. */
	AI_LOOTER_SHOOTER_API ELooterElement Parse(const FString& Text);

	/** Its look's colour (linear, for FWeaponFX and tracers): ember orange-red, soul-light white-gold, salt pale cyan-white. */
	AI_LOOTER_SHOOTER_API FLinearColor FxColor(ELooterElement Element);

	/** Its interface colour (LooterUI::Color::ElementEmber and the rest). */
	AI_LOOTER_SHOOTER_API FLinearColor UiColor(ELooterElement Element);

	/** Element.<Ember|SoulLight|GraveSalt>.<Apply|Tick|Kill>; none for None. */
	AI_LOOTER_SHOOTER_API FName Cue(ELooterElement Element, EElementCue Event);

	/** The damage type its status deals: UEmberElementDamageType, USoulLightElementDamageType, and the grave-salt grenade's own. */
	AI_LOOTER_SHOOTER_API TSubclassOf<UDamageType> DamageTypeFor(ELooterElement Element);

	/** The element a damage type is (the grave-salt grenade's counts as Grave-salt); None for any other. */
	AI_LOOTER_SHOOTER_API ELooterElement ElementOf(const UDamageType* DamageType);

	/**
	 * A bestiary page's weakness line for a creature class: "WEAK TO EMBER · RESISTS GRAVE-SALT"; empty for a kind the
	 * table doesn't name. The Unpaid add their coal ("ITS COAL TO GRAVE-SALT"). Main wires it into the page.
	 */
	AI_LOOTER_SHOOTER_API FString WeaknessLine(const UClass* CreatureClass);
}

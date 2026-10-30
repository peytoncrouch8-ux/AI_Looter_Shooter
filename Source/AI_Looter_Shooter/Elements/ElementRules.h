#pragma once

#include "CoreMinimal.h"
#include "Elements/ElementBody.h"
#include "Elements/ElementTuning.h"
#include "Elements/LooterElement.h"
#include "Weapons/WeaponTypes.h"

class AActor;
class UClass;
class UDamageType;

/** Where a creature stands against an element: the weakness table's answer (immune wins, then weak, then resist). */
enum class EElementAffinity : uint8
{
	Neutral,
	/** Weak to it: the effect's damage and chance x Looter.Element.Weak (2), an element gun's hit x BulletWeak (1.75). */
	Weak,
	/** Resists it: the effect's x Looter.Element.Resist (0.33), an element gun's hit x BulletResist (0.5). */
	Resist,
	/**
	 * Immune (a spirit's, a stone's, a Touched creature's own element, a boss's story immunity): no status at all, an element
	 * gun's hit x Looter.Element.ImmuneHit (0.1).
	 */
	Immune
};

/** A target as the elements see it. The first three are the old ones, so a target written { Body, bSpirit, bBoss } reads as before. */
struct FElementTarget
{
	EElementBody Body = EElementBody::Flesh;
	/**
	 * A spirit on top of its body: the Ethereal tier or the Phasing trait (immune to Cinder, Rot and Thunder, weak to
	 * Wardbreak and Hallow; an Ethereal spider keeps chitin's other weaknesses). Warded no longer counts (2026-10-29 night).
	 */
	bool bSpirit = false;
	/** A boss (or a creature built round a boss fight): statuses last half as long, no slow. */
	bool bBoss = false;
	/** Touched by an element (CreatureTouched): immune to it, weak to its counter. None: not touched. */
	ELooterElement Touched = ELooterElement::None;
	/** Elements it's immune to beyond its body, spirit and touch (ElementRules::MaskOf bits): a boss's story immunity. */
	uint8 ImmuneMask = 0;
	/** Elements it's weak to beyond its body, spirit and touch (MaskOf bits). */
	uint8 WeakMask = 0;
	/**
	 * A Soulfed monster (the Legendary rank). It and a boss ignore their body's resistances and immunities, keeping its
	 * weaknesses and a boss's story immunity (the user, 2026-10-29 night: "bosses ignore their body's resistances").
	 */
	bool bSoulfed = false;
};

/** The sounds an element's status makes (Element.<Element>.<Event>, GUNS-SOUND's cues). */
enum class EElementCue : uint8
{
	Apply,
	Tick,
	Kill,
	/** A ward of the element breaking (the typed wards, 2026-10-29 night). */
	WardBreak
};

/**
 * The elements' plain rules (numbers in, numbers out; the tests drive them without a world): the bodies' table
 * (Elements/ElementBody.h), the chance a hit sets its status, an element gun's hit scaling, the words and colours, and the
 * damage types. On top of a body: a spirit (the Ethereal tier, Phasing) is immune to Cinder, Rot and Thunder and weak to
 * Wardbreak and Hallow; a Touched creature is immune to its element and weak to its counter (CreatureTouched::CounterOf);
 * a boss has its story immunity (the Gravemother Rot, the Foreman Cinder, Abel Hallow). An immunity wins, then a
 * weakness, then a resistance.
 */
namespace ElementRules
{
	/** The body of a creature class (its default's ACreatureBase::ElementBody), or Flesh. */
	AI_LOOTER_SHOOTER_API EElementBody BodyOf(const UClass* CreatureClass);

	/** The target as it stands now: its body, whether it's a spirit (its tier and Phasing), a boss, Touched, its story immunity. */
	AI_LOOTER_SHOOTER_API FElementTarget TargetOf(const AActor* Target);

	/**
	 * Where Target stands against Element. bCritical: the hit was critical (on the Unpaid that is their coal, which
	 * Wardbreak finds weak).
	 */
	AI_LOOTER_SHOOTER_API EElementAffinity Affinity(ELooterElement Element, const FElementTarget& Target, bool bCritical);

	/** The weakness table's multiplier for a status's damage and chance: Weak, Resist, 0 when immune, or 1. */
	AI_LOOTER_SHOOTER_API float AffinityScale(EElementAffinity Affinity, const FElementTuning& Tuning);

	/** An element gun's own hit's multiplier (the user's "bullets scale too, like Borderlands"): BulletWeak, BulletResist, ImmuneHit, or 1. */
	AI_LOOTER_SHOOTER_API float HitScale(EElementAffinity Affinity, const FElementTuning& Tuning);

	/**
	 * What an element gun's hit of Element on Target is multiplied by before it's dealt: HitScale at its affinity. 1 for a
	 * plain gun (Element None: never scaled), for anything that isn't a living creature, and while a ward stands in front of
	 * it (the ward takes the hit at its own rules: UCreatureTraitComponent).
	 */
	AI_LOOTER_SHOOTER_API float GunHitScale(ELooterElement Element, const AActor* Target, bool bCritical, const FElementTuning& Tuning);

	/** An element's bit in FElementTarget's masks (0 for None). */
	AI_LOOTER_SHOOTER_API uint8 MaskOf(ELooterElement Element);

	/** A boss's story immunity by class: the Gravemother Rot, the Foreman Cinder (Ember), Abel Hallow (SoulLight); None otherwise. */
	AI_LOOTER_SHOOTER_API ELooterElement StoryImmunityOf(const UClass* CreatureClass);

	/** The chance a hit of this kind of gun sets its element (before the weakness): the kind's, plus its rarity's steps. */
	AI_LOOTER_SHOOTER_API float GunChance(EWeaponKind Kind, EWeaponRarity Rarity, const FElementTuning& Tuning);

	/** That chance on a target: times the weakness, at most ChanceMax (0 when immune). */
	AI_LOOTER_SHOOTER_API float HitChance(float Chance, EElementAffinity Affinity, const FElementTuning& Tuning);

	/** One of the dead (the Unpaid's coal or the rest of the risen's bone): a smite leaps between them, a jolt between the others. */
	AI_LOOTER_SHOOTER_API bool IsTheDead(EElementBody Body);

	/** "CINDER", "HALLOW", "WARDBREAK", "THUNDER", "ROT" (cards and labels; the user's names, 2026-10-29); empty for None. */
	AI_LOOTER_SHOOTER_API FString Word(ELooterElement Element);

	/** The status a creature carries, for its first-time callout: "BURNING", "SMITTEN", "SALTED", "JOLTED", "ROTTING"; empty for None. */
	AI_LOOTER_SHOOTER_API FString StatusWord(ELooterElement Element);

	/** What it does, in a line for the loot card. Empty for None. */
	AI_LOOTER_SHOOTER_API FString Description(ELooterElement Element);

	/** The element a CSV cell names ("Ember"/"Cinder", "SoulLight"/"Hallow", "GraveSalt"/"Wardbreak", hyphens and spaces ignored); None for anything else. */
	AI_LOOTER_SHOOTER_API ELooterElement Parse(const FString& Text);

	/** Its look's colour (linear, for FWeaponFX and tracers): cinder orange-red, hallow white-gold, wardbreak teal, thunder deep blue, rot green. */
	AI_LOOTER_SHOOTER_API FLinearColor FxColor(ELooterElement Element);

	/** Its interface colour (LooterUI::Color::ElementEmber and the rest). */
	AI_LOOTER_SHOOTER_API FLinearColor UiColor(ELooterElement Element);

	/** Element.<Ember|SoulLight|GraveSalt|Thunder|Rot>.<Apply|Tick|Kill|WardBreak>; none for None. */
	AI_LOOTER_SHOOTER_API FName Cue(ELooterElement Element, EElementCue Event);

	/** The damage type its status deals: UEmberElementDamageType, USoulLightElementDamageType, and the grave-salt grenade's own. */
	AI_LOOTER_SHOOTER_API TSubclassOf<UDamageType> DamageTypeFor(ELooterElement Element);

	/** The element a damage type is (the grave-salt grenade's counts as Grave-salt); None for any other. */
	AI_LOOTER_SHOOTER_API ELooterElement ElementOf(const UDamageType* DamageType);

	/**
	 * A bestiary page's weakness line for a creature class: "WEAK TO CINDER AND ROT · RESISTS WARDBREAK", with "IMMUNE TO
	 * ..." when its body or its story makes it so; the Unpaid add their coal ("ITS COAL TO WARDBREAK"). Empty for no class.
	 */
	AI_LOOTER_SHOOTER_API FString WeaknessLine(const UClass* CreatureClass);
}

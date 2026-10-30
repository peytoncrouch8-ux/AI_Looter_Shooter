#include "Elements/ElementRules.h"
#include "Bosses/AbelKeeper.h"
#include "Combat/GraveSaltDamageType.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/CreatureTouched.h"
#include "Creatures/CreatureTrait.h"
#include "Creatures/CreatureTraitComponent.h"
#include "Creatures/ForemanCreature.h"
#include "Creatures/GravemotherCreature.h"
#include "Elements/ElementDamageType.h"
#include "UI/Style/LooterUIStyle.h"

namespace
{
	/** A body's three sides as element bits (ElementRules::MaskOf). */
	struct FBodySides
	{
		uint8 Weak = 0;
		uint8 Resist = 0;
		uint8 Immune = 0;
	};

	uint8 Bits(std::initializer_list<ELooterElement> Elements)
	{
		uint8 Mask = 0;
		for (const ELooterElement Element : Elements)
		{
			Mask |= ElementRules::MaskOf(Element);
		}
		return Mask;
	}

	/** The bodies' table (the user's, 2026-10-29 night; Elements/ElementBody.h). */
	FBodySides SidesOf(EElementBody Body)
	{
		using E = ELooterElement;
		FBodySides Sides;
		switch (Body)
		{
		case EElementBody::Flesh:
			// The living: red health, Cinder's layer; shocked and rotted as flesh is.
			Sides.Weak = Bits({ E::Thunder, E::Rot, E::Ember });
			break;
		case EElementBody::Chitin:
			Sides.Weak = Bits({ E::Ember, E::Rot });
			Sides.Resist = Bits({ E::GraveSalt });
			break;
		case EElementBody::Gel:
			Sides.Weak = Bits({ E::Ember, E::Thunder });
			Sides.Resist = Bits({ E::SoulLight, E::Rot });
			break;
		case EElementBody::Bone:
			Sides.Weak = Bits({ E::SoulLight });
			Sides.Resist = Bits({ E::Thunder });
			break;
		case EElementBody::Coal:
			// Wardbreak on the coal itself is a crit's (Affinity).
			Sides.Weak = Bits({ E::SoulLight });
			Sides.Resist = Bits({ E::Ember, E::Thunder });
			break;
		case EElementBody::Spirit:
			Sides.Weak = Bits({ E::GraveSalt, E::SoulLight });
			Sides.Immune = Bits({ E::Ember, E::Rot, E::Thunder });
			break;
		case EElementBody::Stone:
			Sides.Weak = Bits({ E::GraveSalt, E::Thunder });
			Sides.Resist = Bits({ E::Ember });
			Sides.Immune = Bits({ E::Rot });
			break;
		case EElementBody::Iron:
			Sides.Weak = Bits({ E::Thunder, E::Rot });
			Sides.Resist = Bits({ E::Ember });
			Sides.Immune = Bits({ E::GraveSalt });
			break;
		default:
			break;
		}
		return Sides;
	}

	constexpr ELooterElement Every[] = { ELooterElement::Ember, ELooterElement::SoulLight, ELooterElement::GraveSalt, ELooterElement::Thunder,
		ELooterElement::Rot };

	/** "A", "A AND B", "A, B AND C". */
	FString JoinWords(const TArray<FString>& Words)
	{
		if (Words.Num() <= 2)
		{
			return FString::Join(Words, TEXT(" AND "));
		}
		TArray<FString> Head(Words.GetData(), Words.Num() - 1);
		return FString::Join(Head, TEXT(", ")) + TEXT(" AND ") + Words.Last();
	}
}

EElementBody ElementRules::BodyOf(const UClass* CreatureClass)
{
	// Each kind says what it is made of in its constructor; Blueprint children keep their parent's.
	if (!CreatureClass || !CreatureClass->IsChildOf(ACreatureBase::StaticClass()))
	{
		return EElementBody::Flesh;
	}
	const ACreatureBase* Defaults = CreatureClass->GetDefaultObject<ACreatureBase>();
	return Defaults ? Defaults->GetElementBody() : EElementBody::Flesh;
}

ELooterElement ElementRules::StoryImmunityOf(const UClass* CreatureClass)
{
	if (!CreatureClass)
	{
		return ELooterElement::None;
	}
	// The bosses' own (the user's, 2026-10-29): the Gravemother rots nothing, the Foreman was fire's, Abel is light's.
	if (CreatureClass->IsChildOf(AGravemotherCreature::StaticClass()))
	{
		return ELooterElement::Rot;
	}
	if (CreatureClass->IsChildOf(AForemanCreature::StaticClass()))
	{
		return ELooterElement::Ember;
	}
	if (CreatureClass->IsChildOf(AAbelKeeper::StaticClass()))
	{
		return ELooterElement::SoulLight;
	}
	return ELooterElement::None;
}

uint8 ElementRules::MaskOf(ELooterElement Element)
{
	return Element == ELooterElement::None ? 0 : static_cast<uint8>(1u << static_cast<uint8>(Element));
}

FElementTarget ElementRules::TargetOf(const AActor* Target)
{
	FElementTarget Result;
	const ACreatureBase* Creature = Cast<ACreatureBase>(Target);
	if (!Creature)
	{
		return Result;
	}
	Result.Body = Creature->GetElementBody();
	Result.bBoss = Creature->IsBossCreature() || Creature->GetRank() == ECreatureRank::Boss;
	Result.bSoulfed = Creature->GetRank() == ECreatureRank::Legendary;
	// A spirit: the Ethereal tier, or a soul that phases. A ward is its own layer now (typed wards), not a spirit.
	const UCreatureTraitComponent* Traits = Creature->GetTraitComponent();
	Result.bSpirit = Creature->GetRank() == ECreatureRank::Ethereal || (Traits && Traits->HasTrait(ECreatureTrait::Phasing));
	Result.Touched = Creature->GetTouchedElement();
	Result.ImmuneMask = MaskOf(StoryImmunityOf(Creature->GetClass()));
	return Result;
}

EElementAffinity ElementRules::Affinity(ELooterElement Element, const FElementTarget& Target, bool bCritical)
{
	const uint8 Bit = MaskOf(Element);
	if (Bit == 0)
	{
		return EElementAffinity::Neutral;
	}
	const FBodySides Sides = SidesOf(Target.Body);
	// A boss or a Soulfed monster ignores its body's resistances and immunities (the user, 2026-10-29 night: "bosses ignore
	// their body's resistances", weak where the body is weak): only a story immunity (ImmuneMask) stands against the player.
	const bool bBodyShieldsOff = Target.bBoss || Target.bSoulfed;
	const uint8 BodyResist = bBodyShieldsOff ? 0 : Sides.Resist;
	uint8 Immune = (bBodyShieldsOff ? 0 : Sides.Immune) | Target.ImmuneMask;
	uint8 Weak = Sides.Weak | Target.WeakMask;
	if (Target.bSpirit)
	{
		// A spirit on top of its body: what burns, rots or shocks flesh passes through it; salt and light find it.
		Immune |= Bits({ ELooterElement::Ember, ELooterElement::Rot, ELooterElement::Thunder });
		Weak |= Bits({ ELooterElement::GraveSalt, ELooterElement::SoulLight });
	}
	if (Target.Touched != ELooterElement::None)
	{
		Immune |= MaskOf(Target.Touched);
		Weak |= MaskOf(CreatureTouched::CounterOf(Target.Touched));
	}
	// The Unpaid's coal (their critical spot) is a spirit's heart to the salt.
	if (bCritical && Target.Body == EElementBody::Coal)
	{
		Weak |= MaskOf(ELooterElement::GraveSalt);
	}
	// Immune wins, then weak, then resist.
	if (Immune & Bit)
	{
		return EElementAffinity::Immune;
	}
	if (Weak & Bit)
	{
		return EElementAffinity::Weak;
	}
	return (BodyResist & Bit) ? EElementAffinity::Resist : EElementAffinity::Neutral;
}

float ElementRules::AffinityScale(EElementAffinity Affinity, const FElementTuning& Tuning)
{
	switch (Affinity)
	{
	case EElementAffinity::Weak: return Tuning.Weak;
	case EElementAffinity::Resist: return Tuning.Resist;
	// No status at all on what is immune to it.
	case EElementAffinity::Immune: return 0.f;
	default: return 1.f;
	}
}

float ElementRules::HitScale(EElementAffinity Affinity, const FElementTuning& Tuning)
{
	// The bullet's own numbers, not the status's (the user, 2026-10-29 night: bullets x1.75 / x0.5, statuses x2 / x0.33).
	switch (Affinity)
	{
	case EElementAffinity::Weak: return Tuning.BulletWeak;
	case EElementAffinity::Resist: return Tuning.BulletResist;
	// A scratch, so the player sees the gun is wrong for it rather than a miss.
	case EElementAffinity::Immune: return Tuning.ImmuneHit;
	default: return 1.f;
	}
}

float ElementRules::GunHitScale(ELooterElement Element, const AActor* Target, bool bCritical, const FElementTuning& Tuning)
{
	const ACreatureBase* Creature = Cast<ACreatureBase>(Target);
	if (Element == ELooterElement::None || !Creature || Creature->IsDead())
	{
		return 1.f;
	}
	// A standing ward takes the hit by its own type (UCreatureTraitComponent's ward); the body's weakness waits until it's down.
	const UCreatureTraitComponent* Traits = Creature->GetTraitComponent();
	if (Traits && Traits->IsWardUp())
	{
		return 1.f;
	}
	return HitScale(Affinity(Element, TargetOf(Creature), bCritical), Tuning);
}

float ElementRules::GunChance(EWeaponKind Kind, EWeaponRarity Rarity, const FElementTuning& Tuning)
{
	float Chance = 0.f;
	switch (Kind)
	{
	case EWeaponKind::Rifle: Chance = Tuning.ChanceRifle; break;
	case EWeaponKind::Shotgun: Chance = Tuning.ChanceShotgun; break;
	case EWeaponKind::Revolver: Chance = Tuning.ChanceRevolver; break;
	case EWeaponKind::SMG: Chance = Tuning.ChanceSMG; break;
	case EWeaponKind::Sniper: Chance = Tuning.ChanceSniper; break;
	case EWeaponKind::LMG: Chance = Tuning.ChanceLMG; break;
	case EWeaponKind::Launcher: Chance = Tuning.ChanceLauncher; break;
	default: Chance = Tuning.ChanceRifle; break;
	}
	const int32 Steps = FMath::Max(static_cast<int32>(Rarity) - static_cast<int32>(EWeaponRarity::Uncommon), 0);
	return FMath::Clamp(Chance + Steps * Tuning.ChanceRarityStep, 0.f, 1.f);
}

float ElementRules::HitChance(float Chance, EElementAffinity Affinity, const FElementTuning& Tuning)
{
	return FMath::Clamp(Chance * AffinityScale(Affinity, Tuning), 0.f, Tuning.ChanceMax);
}

bool ElementRules::IsTheDead(EElementBody Body)
{
	return Body == EElementBody::Coal || Body == EElementBody::Bone;
}

FString ElementRules::Word(ELooterElement Element)
{
	switch (Element)
	{
	// The user's names (2026-10-29): Cinder (was Ember, which the hero powers keep), Hallow (was Soul-light), Wardbreak (was
	// Grave-salt; the salt jar and grenade keep their names). The enum, cues, cvars and part keys keep the old words.
	case ELooterElement::Ember: return TEXT("CINDER");
	case ELooterElement::SoulLight: return TEXT("HALLOW");
	case ELooterElement::GraveSalt: return TEXT("WARDBREAK");
	case ELooterElement::Thunder: return TEXT("THUNDER");
	case ELooterElement::Rot: return TEXT("ROT");
	default: return FString();
	}
}

FString ElementRules::StatusWord(ELooterElement Element)
{
	switch (Element)
	{
	case ELooterElement::Ember: return TEXT("BURNING");
	case ELooterElement::SoulLight: return TEXT("SMITTEN");
	case ELooterElement::GraveSalt: return TEXT("SALTED");
	case ELooterElement::Thunder: return TEXT("JOLTED");
	case ELooterElement::Rot: return TEXT("ROTTING");
	default: return FString();
	}
}

FString ElementRules::Description(ELooterElement Element)
{
	switch (Element)
	{
	case ELooterElement::Ember: return TEXT("Sets them burning. Spiders, slimes and the living burn best; the Unpaid shrug it off.");
	case ELooterElement::SoulLight: return TEXT("Smites with a burst of light that leaps to the next of the dead. Slimes dull it.");
	case ELooterElement::GraveSalt: return TEXT("Salts and slows them; every ward crumbles. Spirits and stone fear it; spiders barely notice.");
	case ELooterElement::Thunder: return TEXT("Jolts them off their feet and arcs to the next living thing. Slimes and the living feel it most; the dead don't.");
	case ELooterElement::Rot: return TEXT("Eats at them, worse as it goes, and spreads from the dying. Spiders and the living rot; slimes don't.");
	default: return FString();
	}
}

ELooterElement ElementRules::Parse(const FString& Text)
{
	FString Key = Text.TrimStartAndEnd().Replace(TEXT("-"), TEXT("")).Replace(TEXT(" "), TEXT(""));
	if (Key.Equals(TEXT("Ember"), ESearchCase::IgnoreCase) || Key.Equals(TEXT("Cinder"), ESearchCase::IgnoreCase))
	{
		return ELooterElement::Ember;
	}
	if (Key.Equals(TEXT("SoulLight"), ESearchCase::IgnoreCase) || Key.Equals(TEXT("Hallow"), ESearchCase::IgnoreCase))
	{
		return ELooterElement::SoulLight;
	}
	if (Key.Equals(TEXT("GraveSalt"), ESearchCase::IgnoreCase) || Key.Equals(TEXT("Wardbreak"), ESearchCase::IgnoreCase))
	{
		return ELooterElement::GraveSalt;
	}
	if (Key.Equals(TEXT("Thunder"), ESearchCase::IgnoreCase) || Key.Equals(TEXT("Shock"), ESearchCase::IgnoreCase))
	{
		return ELooterElement::Thunder;
	}
	if (Key.Equals(TEXT("Rot"), ESearchCase::IgnoreCase) || Key.Equals(TEXT("Blight"), ESearchCase::IgnoreCase))
	{
		return ELooterElement::Rot;
	}
	return ELooterElement::None;
}

FLinearColor ElementRules::FxColor(ELooterElement Element)
{
	// Hues only: the glow material multiplies them by each particle's strength.
	switch (Element)
	{
	case ELooterElement::Ember: return FLinearColor(1.f, 0.3f, 0.06f);
	case ELooterElement::SoulLight: return FLinearColor(1.f, 0.85f, 0.52f);
	case ELooterElement::GraveSalt: return FLinearColor(0.3f, 0.95f, 0.85f);
	case ELooterElement::Thunder: return FLinearColor(0.28f, 0.4f, 1.f);
	case ELooterElement::Rot: return FLinearColor(0.55f, 0.95f, 0.25f);
	default: return FLinearColor::White;
	}
}

FLinearColor ElementRules::UiColor(ELooterElement Element)
{
	switch (Element)
	{
	case ELooterElement::Ember: return LooterUI::Color::ElementEmber();
	case ELooterElement::SoulLight: return LooterUI::Color::ElementSoulLight();
	case ELooterElement::GraveSalt: return LooterUI::Color::ElementGraveSalt();
	case ELooterElement::Thunder: return LooterUI::Color::ElementThunder();
	case ELooterElement::Rot: return LooterUI::Color::ElementRot();
	default: return LooterUI::Color::Text();
	}
}

FName ElementRules::Cue(ELooterElement Element, EElementCue Event)
{
	const TCHAR* Name = nullptr;
	switch (Element)
	{
	case ELooterElement::Ember: Name = TEXT("Ember"); break;
	case ELooterElement::SoulLight: Name = TEXT("SoulLight"); break;
	case ELooterElement::GraveSalt: Name = TEXT("GraveSalt"); break;
	case ELooterElement::Thunder: Name = TEXT("Thunder"); break;
	case ELooterElement::Rot: Name = TEXT("Rot"); break;
	default: return NAME_None;
	}
	const TCHAR* What = TEXT("Kill");
	switch (Event)
	{
	case EElementCue::Apply: What = TEXT("Apply"); break;
	case EElementCue::Tick: What = TEXT("Tick"); break;
	case EElementCue::WardBreak: What = TEXT("WardBreak"); break;
	default: break;
	}
	return FName(*FString::Printf(TEXT("Element.%s.%s"), Name, What));
}

TSubclassOf<UDamageType> ElementRules::DamageTypeFor(ELooterElement Element)
{
	switch (Element)
	{
	case ELooterElement::Ember: return UEmberElementDamageType::StaticClass();
	case ELooterElement::SoulLight: return USoulLightElementDamageType::StaticClass();
	case ELooterElement::GraveSalt: return UGraveSaltDamageType::StaticClass();
	case ELooterElement::Thunder: return UThunderElementDamageType::StaticClass();
	case ELooterElement::Rot: return URotElementDamageType::StaticClass();
	default: return UDamageType::StaticClass();
	}
}

ELooterElement ElementRules::ElementOf(const UDamageType* DamageType)
{
	if (const UElementDamageType* Element = Cast<UElementDamageType>(DamageType))
	{
		return Element->Element;
	}
	return DamageType && DamageType->IsA<UGraveSaltDamageType>() ? ELooterElement::GraveSalt : ELooterElement::None;
}

FString ElementRules::WeaknessLine(const UClass* CreatureClass)
{
	if (!CreatureClass)
	{
		return FString();
	}
	FElementTarget Target;
	Target.Body = BodyOf(CreatureClass);
	Target.ImmuneMask = MaskOf(StoryImmunityOf(CreatureClass));
	// A boss's page reads as the fight does: the three bosses are the ones with a story immunity.
	Target.bBoss = Target.ImmuneMask != 0;
	TArray<FString> Weak;
	TArray<FString> Resists;
	TArray<FString> Immune;
	for (const ELooterElement Element : Every)
	{
		switch (Affinity(Element, Target, false))
		{
		case EElementAffinity::Weak: Weak.Add(Word(Element)); break;
		case EElementAffinity::Resist: Resists.Add(Word(Element)); break;
		case EElementAffinity::Immune: Immune.Add(Word(Element)); break;
		default: break;
		}
	}
	TArray<FString> Parts;
	if (!Weak.IsEmpty())
	{
		Parts.Add(TEXT("WEAK TO ") + JoinWords(Weak));
	}
	if (Target.Body == EElementBody::Coal)
	{
		Parts.Add(TEXT("ITS COAL TO ") + Word(ELooterElement::GraveSalt));
	}
	if (!Resists.IsEmpty())
	{
		Parts.Add(TEXT("RESISTS ") + JoinWords(Resists));
	}
	if (!Immune.IsEmpty())
	{
		Parts.Add(TEXT("IMMUNE TO ") + JoinWords(Immune));
	}
	return FString::Join(Parts, TEXT(" · "));
}

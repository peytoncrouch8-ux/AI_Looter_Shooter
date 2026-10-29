#include "Elements/ElementRules.h"
#include "Bosses/AbelKeeper.h"
#include "Combat/GraveSaltDamageType.h"
#include "Creatures/BellRingerCreature.h"
#include "Creatures/DeadMinerCreature.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/CreatureTrait.h"
#include "Creatures/CreatureTraitComponent.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "Elements/ElementDamageType.h"
#include "UI/Style/LooterUIStyle.h"

EElementBody ElementRules::BodyOf(const UClass* CreatureClass)
{
	if (!CreatureClass)
	{
		return EElementBody::Other;
	}
	// The Spitter and the Gravemother are spiders (they derive from it).
	if (CreatureClass->IsChildOf(ASpiderCreature::StaticClass()))
	{
		return EElementBody::Chitin;
	}
	if (CreatureClass->IsChildOf(ASlimeCreature::StaticClass()))
	{
		return EElementBody::Gel;
	}
	if (CreatureClass->IsChildOf(AUnpaidCreature::StaticClass()))
	{
		// The Unpaid themselves are the coal's own (Ember finds it no harm); the kinds built on them (dead miners and the
		// Foreman, the Bell-Ringer, Abel) are the rest of the risen dead. Blueprint children keep their parent's body.
		const bool bRestOfTheDead = CreatureClass->IsChildOf(ADeadMinerCreature::StaticClass())
			|| CreatureClass->IsChildOf(ABellRingerCreature::StaticClass()) || CreatureClass->IsChildOf(AAbelKeeper::StaticClass());
		return bRestOfTheDead ? EElementBody::Undead : EElementBody::Unpaid;
	}
	return EElementBody::Other;
}

FElementTarget ElementRules::TargetOf(const AActor* Target)
{
	FElementTarget Result;
	const ACreatureBase* Creature = Cast<ACreatureBase>(Target);
	if (!Creature)
	{
		return Result;
	}
	Result.Body = BodyOf(Creature->GetClass());
	Result.bBoss = Creature->IsBossCreature() || Creature->GetRank() == ECreatureRank::Boss;
	const UCreatureTraitComponent* Traits = Creature->GetTraitComponent();
	Result.bSpirit = Creature->GetRank() == ECreatureRank::Ethereal
		|| (Traits && (Traits->HasTrait(ECreatureTrait::Warded) || Traits->HasTrait(ECreatureTrait::Phasing)));
	return Result;
}

EElementAffinity ElementRules::Affinity(ELooterElement Element, const FElementTarget& Target, bool bCritical)
{
	switch (Element)
	{
	case ELooterElement::Ember:
		if (Target.Body == EElementBody::Chitin || Target.Body == EElementBody::Gel)
		{
			return EElementAffinity::Weak;
		}
		return Target.Body == EElementBody::Unpaid ? EElementAffinity::Resist : EElementAffinity::Neutral;
	case ELooterElement::SoulLight:
		if (IsTheDead(Target.Body))
		{
			return EElementAffinity::Weak;
		}
		return Target.Body == EElementBody::Gel ? EElementAffinity::Resist : EElementAffinity::Neutral;
	case ELooterElement::GraveSalt:
		// A spirit before a spider: a weakness wins. An Unpaid's coal (its critical spot) is a spirit's heart.
		if (Target.bSpirit || (bCritical && Target.Body == EElementBody::Unpaid))
		{
			return EElementAffinity::Weak;
		}
		return Target.Body == EElementBody::Chitin ? EElementAffinity::Resist : EElementAffinity::Neutral;
	case ELooterElement::Thunder:
		// Wet and living things conduct; the dead don't feel it.
		if (Target.Body == EElementBody::Gel || Target.Body == EElementBody::Other)
		{
			return EElementAffinity::Weak;
		}
		return IsTheDead(Target.Body) ? EElementAffinity::Resist : EElementAffinity::Neutral;
	case ELooterElement::Rot:
		// Flesh and chitin rot; a slime is rot already.
		if (Target.Body == EElementBody::Chitin || Target.Body == EElementBody::Other)
		{
			return EElementAffinity::Weak;
		}
		return Target.Body == EElementBody::Gel ? EElementAffinity::Resist : EElementAffinity::Neutral;
	default:
		return EElementAffinity::Neutral;
	}
}

float ElementRules::AffinityScale(EElementAffinity Affinity, const FElementTuning& Tuning)
{
	switch (Affinity)
	{
	case EElementAffinity::Weak: return Tuning.Weak;
	case EElementAffinity::Resist: return Tuning.Resist;
	default: return 1.f;
	}
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
	return Body == EElementBody::Unpaid || Body == EElementBody::Undead;
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

FString ElementRules::Description(ELooterElement Element)
{
	switch (Element)
	{
	case ELooterElement::Ember: return TEXT("Sets them burning. Spiders and slimes burn best; the Unpaid shrug it off.");
	case ELooterElement::SoulLight: return TEXT("Smites with a burst of light that leaps to the next of the dead. Slimes dull it.");
	case ELooterElement::GraveSalt: return TEXT("Salts and slows them; wards crumble. Spirits fear it; spiders barely notice.");
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
	const TCHAR* What = Event == EElementCue::Apply ? TEXT("Apply") : (Event == EElementCue::Tick ? TEXT("Tick") : TEXT("Kill"));
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
	const EElementBody Body = BodyOf(CreatureClass);
	if (Body == EElementBody::Other)
	{
		return FString();
	}
	TArray<FString> Weak;
	TArray<FString> Resists;
	const FElementTarget Target{ Body, false, false };
	for (const ELooterElement Element : { ELooterElement::Ember, ELooterElement::SoulLight, ELooterElement::GraveSalt, ELooterElement::Thunder, ELooterElement::Rot })
	{
		const EElementAffinity Side = Affinity(Element, Target, false);
		if (Side == EElementAffinity::Weak)
		{
			Weak.Add(Word(Element));
		}
		else if (Side == EElementAffinity::Resist)
		{
			Resists.Add(Word(Element));
		}
	}
	TArray<FString> Parts;
	if (!Weak.IsEmpty())
	{
		Parts.Add(TEXT("WEAK TO ") + FString::Join(Weak, TEXT(" AND ")));
	}
	if (Body == EElementBody::Unpaid)
	{
		Parts.Add(TEXT("ITS COAL TO ") + Word(ELooterElement::GraveSalt));
	}
	if (!Resists.IsEmpty())
	{
		Parts.Add(TEXT("RESISTS ") + FString::Join(Resists, TEXT(" AND ")));
	}
	return FString::Join(Parts, TEXT(" · "));
}

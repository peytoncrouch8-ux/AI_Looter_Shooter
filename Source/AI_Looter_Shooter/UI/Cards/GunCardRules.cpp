#include "UI/Cards/GunCardRules.h"
#include "Elements/ElementRules.h"
#include "UI/Style/WeaponText.h"
#include "Weapons/LegendaryRules.h"
#include "Weapons/LegendaryTuning.h"
#include "Weapons/PartEffectRules.h"
#include "Weapons/WeaponCurses.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponNotches.h"
#include "Weapons/WeaponParts.h"
#include "Weapons/WeaponTypes.h"

// What the tag card shows and how its intro moves (Docs/Work/2026-10-28-UICharacter/CONTRACT.md, "Round 2"): the user
// wanted Borderlands' item card, "super nice but simple with all the needed details", in our own look.

using LoadoutRules::EStat;

namespace
{
	/** What an element does, as the card's line. */
	FString ElementLine(ELooterElement Element)
	{
		switch (Element)
		{
		// Led by the element's name: its rows above only show its glyph and colour.
		case ELooterElement::Ember: return TEXT("Cinder: sets them burning");
		case ELooterElement::SoulLight: return TEXT("Hallow: light leaps to the next of the dead");
		case ELooterElement::GraveSalt: return TEXT("Wardbreak: salts and slows them, wards crumble");
		case ELooterElement::Thunder: return TEXT("Thunder: jolts them, and the next living thing");
		case ELooterElement::Rot: return TEXT("Rot: eats at them, and spreads from the dying");
		default: return FString();
		}
	}

	/** A sentence without its full stop. */
	FString NoStop(FString Line)
	{
		Line.TrimEndInline();
		Line.RemoveFromEnd(TEXT("."));
		return Line;
	}

	FString TitleCase(const FString& Upper)
	{
		FString Out = Upper.ToLower();
		bool bStart = true;
		for (TCHAR& Char : Out)
		{
			if (bStart && FChar::IsAlpha(Char))
			{
				Char = FChar::ToUpper(Char);
			}
			bStart = Char == TEXT(' ') || Char == TEXT('-');
		}
		return Out;
	}

	float EaseOut(float Share)
	{
		const float T = FMath::Clamp(Share, 0.f, 1.f);
		return 1.f - (1.f - T) * (1.f - T) * (1.f - T);
	}
}

TArray<EStat> GunCardRules::CardStats(const FWeaponStats& /*Stats*/)
{
	// The same six for every gun: zoom is no row of the card (the user, 2026-10-29: "zoom doesn't have to be a stat"; the
	// inventory's Inspect still lists it). The card's own Element row follows them (UGunCardWidget::ApplyGun).
	return { EStat::Damage, EStat::Accuracy, EStat::Handling, EStat::Reload, EStat::FireRate, EStat::Magazine };
}

FString GunCardRules::StatLabel(EStat Stat)
{
	switch (Stat)
	{
	case EStat::Damage: return TEXT("Damage");
	case EStat::Accuracy: return TEXT("Accuracy");
	case EStat::Handling: return TEXT("Handling");
	case EStat::Reload: return TEXT("Reload");
	case EStat::FireRate: return TEXT("Fire rate");
	case EStat::Magazine: return TEXT("Magazine");
	case EStat::Zoom: return TEXT("Zoom");
	default: return FString(LoadoutRules::StatName(Stat));
	}
}

FString GunCardRules::StatValueText(EStat Stat, const FWeaponStats& Stats)
{
	switch (Stat)
	{
	case EStat::Damage:
		return Stats.PelletsPerShot > 1 ? FString::Printf(TEXT("%d × %d"), FMath::RoundToInt(Stats.Damage), Stats.PelletsPerShot)
			: FString::FromInt(FMath::RoundToInt(Stats.Damage));
	case EStat::Accuracy: return FString::Printf(TEXT("%d%%"), FMath::RoundToInt(LoadoutRules::StatRating(EStat::Accuracy, Stats) * 100.f));
	case EStat::Handling: return FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Stats.Handling * 100.f));
	case EStat::Reload: return FString::Printf(TEXT("%.1f s"), Stats.ReloadTime);
	case EStat::FireRate: return FString::Printf(TEXT("%d rpm"), FMath::RoundToInt(Stats.FireRate));
	case EStat::Magazine: return FString::FromInt(Stats.MagazineSize);
	case EStat::Zoom:
		return FMath::IsNearlyEqual(Stats.Zoom, FMath::RoundToFloat(Stats.Zoom), 0.05f) ? FString::Printf(TEXT("%d×"), FMath::RoundToInt(Stats.Zoom))
			: FString::Printf(TEXT("%.1f×"), Stats.Zoom);
	default: return LoadoutRules::StatText(Stat, Stats);
	}
}

TArray<GunCardRules::FElementRow> GunCardRules::ElementRows(const FWeaponInstanceData& Gun, const FElementTuning& Tuning)
{
	const ELooterElement Element = WeaponParts::ElementOf(Gun);
	const EWeaponKind Kind = Gun.Definition ? Gun.Definition->Kind : EWeaponKind::None;
	const FString Chance = FString::Printf(TEXT("%d%%"), FMath::RoundToInt(ElementRules::GunChance(Kind, Gun.Rarity, Tuning) * 100.f));
	// A status is a share of the hit (one bullet, one pellet) that set it.
	const float Hit = Gun.Stats.Damage;
	auto PerSecond = [Hit](float Share, float Seconds) { return FString::Printf(TEXT("%.1f"), Hit * Share / FMath::Max(Seconds, 0.1f)); };
	switch (Element)
	{
	case ELooterElement::Ember:
		return { { TEXT("Burn dmg/sec"), PerSecond(Tuning.EmberShare, Tuning.EmberSeconds) }, { TEXT("Chance to burn"), Chance } };
	case ELooterElement::SoulLight:
		return { { TEXT("Smite damage"), FString::FromInt(FMath::RoundToInt(Hit * Tuning.SoulLightShare)) }, { TEXT("Chance to smite"), Chance } };
	case ELooterElement::GraveSalt:
		return { { TEXT("Salt dmg/sec"), PerSecond(Tuning.GraveSaltShare, Tuning.GraveSaltSeconds) }, { TEXT("Chance to salt"), Chance } };
	case ELooterElement::Thunder:
		return { { TEXT("Jolt damage"), FString::FromInt(FMath::RoundToInt(Hit * Tuning.ThunderShare)) }, { TEXT("Chance to jolt"), Chance } };
	case ELooterElement::Rot:
		return { { TEXT("Rot dmg/sec"), PerSecond(Tuning.RotShare, Tuning.RotSeconds) }, { TEXT("Chance to rot"), Chance } };
	default:
		// Just the word by the empty ring (the user, 2026-10-29: not "Element ... Normal").
		return { { TEXT("Normal"), FString() } };
	}
}

TArray<FString> GunCardRules::Lines(const FWeaponInstanceData& Gun)
{
	// Only what sets the gun apart (the user, 2026-10-29: "basic information like that isn't needed, only special
	// characteristics"): no maker's trait line, its stats already show its trades.
	TArray<FString> Out;
	const FString Element = ElementLine(WeaponParts::ElementOf(Gun));
	if (!Element.IsEmpty())
	{
		Out.Add(Element);
	}
	const EPartEffect Effect = WeaponParts::EffectOf(Gun);
	if (Effect != EPartEffect::None)
	{
		Out.Add(NoStop(PartEffectRules::Description(Effect)));
	}
	// A unique legendary's own trick, with its live numbers (Gravesilk's silk, Company Scrip's pay, Lamplighter's lantern).
	const FString Signature = LegendaryRules::Description(LegendaryRules::SignatureOf(Gun), FLegendaryTuning::Live());
	if (!Signature.IsEmpty())
	{
		Out.Add(NoStop(Signature));
	}
	if (const FWeaponCurse* Curse = WeaponCurses::Of(Gun))
	{
		const FString Name = TitleCase(Curse->Name.ToString());
		Out.Add(FString::Printf(TEXT("%s%s: %s"), *Name, Gun.bCurseLifted ? TEXT(" (lifted)") : TEXT(""), *NoStop(Curse->Perk.ToString())));
		if (!Gun.bCurseLifted)
		{
			Out.Add(NoStop(Curse->Drawback.ToString()));
		}
	}
	if (Gun.Kills > 0)
	{
		const FText Tier = WeaponNotches::TierName(WeaponNotches::TierFor(Gun.Kills));
		const FString Count = FString::Printf(TEXT("%d %s"), Gun.Kills, Gun.Kills == 1 ? TEXT("notch") : TEXT("notches"));
		Out.Add(Tier.IsEmpty() ? Count : Count + TEXT(": ") + Tier.ToString());
	}
	return Out;
}

FString GunCardRules::FlavorLine(const FWeaponInstanceData& Gun)
{
	return LooterWeaponText::FlavorLine(Gun);
}

bool GunCardRules::SameKind(const FWeaponInstanceData& A, const FWeaponInstanceData& B)
{
	return A.Definition && B.Definition && A.Definition->Kind == B.Definition->Kind;
}

const FWeaponInstanceData* GunCardRules::PickCompare(const FWeaponInstanceData& Gun, const FWeaponInstanceData* InHand,
	TConstArrayView<const FWeaponInstanceData*> Carried)
{
	if (InHand && SameKind(Gun, *InHand))
	{
		return InHand;
	}
	for (const FWeaponInstanceData* Other : Carried)
	{
		if (Other && SameKind(Gun, *Other))
		{
			return Other;
		}
	}
	return nullptr;
}

FString GunCardRules::VerdictWord(LoadoutRules::EVerdict Verdict)
{
	return Verdict == LoadoutRules::EVerdict::Upgrade ? FString(TEXT("UPGRADE")) : FString();
}

TArray<GunCardRules::FKeyHint> GunCardRules::KeyHints(const FGunCardKeys& Keys)
{
	TArray<FKeyHint> Hints;
	const FString HoldCap = FString::Printf(TEXT("HOLD %s"), *Keys.Key);
	// A hold always takes it straight in hand (EquipPickup), so both show (the user, 2026-10-28: "press E to take, hold E
	// to equip").
	if (!Keys.bSlotsFull)
	{
		Hints.Add({ Keys.Key, TEXT("TAKE"), false, false });
		Hints.Add({ HoldCap, TEXT("EQUIP"), true, false });
	}
	else if (!Keys.bBackpackFull)
	{
		Hints.Add({ Keys.Key, TEXT("TO BACKPACK"), false, false });
		Hints.Add({ HoldCap, TEXT("EQUIP"), true, false });
	}
	else
	{
		// Both full: the tap swaps it with the gun in hand, which drops where the player stands.
		Hints.Add({ Keys.Key, TEXT("SWAP"), false, false });
		Hints.Add({ FString(), TEXT("BACKPACK FULL"), false, true });
	}
	return Hints;
}

float GunCardRules::UnfoldScale(float Age)
{
	// Up past full by 0.08 s (a slight overshoot), then settles: the card snaps open rather than slides.
	constexpr float Peak = 0.08f;
	if (Age <= 0.f)
	{
		return 0.15f;
	}
	if (Age < Peak)
	{
		return FMath::Lerp(0.15f, 1.06f, EaseOut(Age / Peak));
	}
	if (Age < UnfoldSeconds)
	{
		return FMath::Lerp(1.06f, 1.f, (Age - Peak) / (UnfoldSeconds - Peak));
	}
	return 1.f;
}

float GunCardRules::UnfoldOpacity(float Age)
{
	return FMath::Clamp(Age / (UnfoldSeconds * 0.6f), 0.f, 1.f);
}

float GunCardRules::RowShare(float Age, int32 Index)
{
	const float Start = RowDelay + RowStagger * static_cast<float>(FMath::Max(0, Index));
	return EaseOut((Age - Start) / RowSeconds);
}

float GunCardRules::CountShare(float Age)
{
	return EaseOut(Age / CountSeconds);
}

float GunCardRules::GemPop(float Age)
{
	// The gem lands last: it swells a fifth and settles as the rows finish.
	constexpr float Start = 0.28f;
	constexpr float Length = 0.2f;
	const float T = (Age - Start) / Length;
	return T <= 0.f || T >= 1.f ? 1.f : 1.f + 0.2f * FMath::Sin(T * UE_PI);
}

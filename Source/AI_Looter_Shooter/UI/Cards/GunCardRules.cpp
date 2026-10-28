#include "UI/Cards/GunCardRules.h"
#include "Elements/ElementRules.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Weapons/PartEffectRules.h"
#include "Weapons/WeaponCurses.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponNotches.h"
#include "Weapons/WeaponParts.h"
#include "Weapons/WeaponTypes.h"

// What the gun card shows and how its intro moves (Docs/Work/2026-10-28-UICharacter/CONTRACT.md). The user's rule for it:
// "don't overload the user with too much information on simple things like guns".

namespace
{
	/** An element's few words on the card (its full line is the inventory's Inspect). */
	FString ElementWords(ELooterElement Element)
	{
		switch (Element)
		{
		case ELooterElement::Ember: return TEXT("sets them burning");
		case ELooterElement::SoulLight: return TEXT("light leaps to the dead");
		case ELooterElement::GraveSalt: return TEXT("salts and slows them");
		default: return FString();
		}
	}

	/** The first sentence, lower-cased at the start and without its full stop: "rounds burst where they land". */
	FString Short(const FString& Line)
	{
		FString First = Line;
		int32 Stop = INDEX_NONE;
		if (First.FindChar(TEXT('.'), Stop))
		{
			First.LeftInline(Stop);
		}
		if (!First.IsEmpty())
		{
			First[0] = FChar::ToLower(First[0]);
		}
		return First;
	}

	float EaseOut(float Share)
	{
		const float T = FMath::Clamp(Share, 0.f, 1.f);
		return 1.f - (1.f - T) * (1.f - T) * (1.f - T);
	}
}

TConstArrayView<LoadoutRules::EStat> GunCardRules::CardStats()
{
	static const LoadoutRules::EStat Stats[] = { LoadoutRules::EStat::FireRate, LoadoutRules::EStat::Magazine, LoadoutRules::EStat::Reload };
	return Stats;
}

FString GunCardRules::StatLabel(LoadoutRules::EStat Stat)
{
	switch (Stat)
	{
	case LoadoutRules::EStat::FireRate: return TEXT("FIRE RATE");
	case LoadoutRules::EStat::Magazine: return TEXT("MAGAZINE");
	case LoadoutRules::EStat::Reload: return TEXT("RELOAD");
	default: return FString(LoadoutRules::StatName(Stat)).ToUpper();
	}
}

GunCardRules::FSpecialLine GunCardRules::SpecialLine(const FWeaponInstanceData& Gun)
{
	FSpecialLine Line;
	const ELooterElement Element = WeaponParts::ElementOf(Gun);
	if (Element != ELooterElement::None)
	{
		Line.Word = ElementRules::Word(Element).ToUpper();
		Line.Words = ElementWords(Element);
		Line.Color = ElementRules::UiColor(Element);
		return Line;
	}
	const EPartEffect Effect = WeaponParts::EffectOf(Gun);
	if (Effect != EPartEffect::None)
	{
		Line.Word = PartEffectRules::Word(Effect).ToUpper();
		Line.Words = Short(PartEffectRules::Description(Effect));
		Line.Color = LooterUI::Color::Accent();
		return Line;
	}
	if (const FWeaponCurse* Curse = WeaponCurses::Of(Gun))
	{
		Line.Word = LooterWeaponText::CurseString(Gun);
		Line.Words = Gun.bCurseLifted ? FString() : Curse->Perk.ToString();
		Line.Color = LooterUI::Color::Curse();
		return Line;
	}
	if (Gun.Kills > 0)
	{
		Line.Word = LooterWeaponText::NotchesString(Gun);
		Line.Color = LoadoutParts::NotchTierColor(WeaponNotches::TierFor(Gun.Kills));
	}
	return Line;
}

FString GunCardRules::VerdictWord(LoadoutRules::EVerdict Verdict)
{
	return Verdict == LoadoutRules::EVerdict::Upgrade ? FString(TEXT("UPGRADE")) : FString();
}

FString GunCardRules::KindLine(const FWeaponInstanceData& Gun)
{
	const FString Kind = Gun.Definition ? Gun.Definition->DisplayName.ToString() : FString(TEXT("Gun"));
	return FString::Printf(TEXT("Level %d · %s"), Gun.Level, *Kind);
}

FString GunCardRules::DamageNumber(const FWeaponInstanceData& Gun)
{
	return FText::AsNumber(FMath::RoundToInt(Gun.Stats.Damage)).ToString();
}

FString GunCardRules::PelletText(const FWeaponInstanceData& Gun)
{
	return Gun.Stats.PelletsPerShot > 1 ? FString::Printf(TEXT("×%d"), Gun.Stats.PelletsPerShot) : FString();
}

TArray<GunCardRules::FKeyHint> GunCardRules::KeyHints(const FGunCardKeys& Keys)
{
	TArray<FKeyHint> Hints;
	const FString HoldCap = FString::Printf(TEXT("HOLD %s"), *Keys.Key);
	if (!Keys.bSlotsFull)
	{
		Hints.Add({ Keys.Key, TEXT("TAKE"), false, false });
	}
	else if (!Keys.bBackpackFull)
	{
		Hints.Add({ Keys.Key, TEXT("TO BACKPACK"), false, false });
		Hints.Add({ HoldCap, TEXT("EQUIP"), true, true });
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

float GunCardRules::BarShare(float Age, int32 Index)
{
	const float Start = BarDelay + BarStagger * static_cast<float>(FMath::Max(0, Index));
	return EaseOut((Age - Start) / BarSeconds);
}

float GunCardRules::CountShare(float Age)
{
	return EaseOut(Age / CountSeconds);
}

float GunCardRules::ShinePosition(float Age)
{
	if (Age < ShineStart)
	{
		return -1.f;
	}
	if (Age > ShineEnd)
	{
		return 2.f;
	}
	return (Age - ShineStart) / (ShineEnd - ShineStart);
}

bool GunCardRules::HasShine(const FWeaponInstanceData& Gun)
{
	return Gun.Rarity >= EWeaponRarity::Epic;
}

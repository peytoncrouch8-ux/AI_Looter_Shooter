#pragma once

#include "CoreMinimal.h"
#include "Elements/ElementRules.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/PartEffectRules.h"
#include "Weapons/WeaponCurses.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponNotches.h"
#include "Weapons/WeaponParts.h"
#include "Affixes/WeaponMakers.h"
#include "Affixes/WeaponRollLibrary.h"
#include "UI/Style/LooterUIStyle.h"
#include "Weapons/WeaponTypes.h"

/** Shared formatting for weapon names/stats across the HUD, loot labels and inventory. */
namespace LooterWeaponText
{
	/**
	 * "Scoped Assault Rifle": the word the gun's parts give it (if any) and the kind of gun. No rarity word: every name is
	 * shown in its rarity's color (Color below), which says it at a glance (the user's call). A named gun goes by its own
	 * name ("Heirloom") instead. Once a gun is Named by its notches its nickname follows in quotes: Scoped Bullpup
	 * "Lantern Jaw".
	 */
	inline FString Name(const FWeaponInstanceData& Instance)
	{
		const FString Base = WeaponParts::BaseName(Instance);
		const FString Nickname = WeaponNotches::Nickname(Instance);
		return Nickname.IsEmpty() ? Base : FString::Printf(TEXT("%s \"%s\""), *Base, *Nickname);
	}

	/** "137 NOTCHES" ("1 NOTCH"), for the loot and loadout cards; empty before its first kill. */
	inline FString NotchesString(const FWeaponInstanceData& Instance)
	{
		if (Instance.Kills < 1)
		{
			return FString();
		}
		return FString::Printf(TEXT("%s %s"), *FText::AsNumber(Instance.Kills).ToString(), Instance.Kills == 1 ? TEXT("NOTCH") : TEXT("NOTCHES"));
	}

	/** The curse's name for labels ("HUNGRY"), empty when not cursed; "HUNGRY · LIFTED" once lifted. */
	inline FString CurseString(const FWeaponInstanceData& Instance)
	{
		const FWeaponCurse* Curse = WeaponCurses::Of(Instance);
		if (!Curse)
		{
			return FString();
		}
		const FString Word = Curse->Name.ToString().ToUpper();
		return Instance.bCurseLifted ? Word + TEXT(" · LIFTED") : Word;
	}

	/** A named gun's flavor line ("Hold the door."), shown under its name on its cards and label; empty for other guns. */
	inline FString FlavorLine(const FWeaponInstanceData& Instance)
	{
		return Instance.Named ? Instance.Named->FlavorText.ToString() : FString();
	}

	/** The color a flavor line is shown in, wherever it shows: the kit's accent, as the line is a named gun's highlight. */
	inline FLinearColor FlavorColor()
	{
		return LooterUI::Color::Accent();
	}

	/**
	 * A flavor line's type on a card: the kit's regular face rather than the labels' bold, so it reads as words said. Over
	 * the world (the loot label) it stays floating text, whose bold carries the outline.
	 */
	inline FSlateFontInfo FlavorFont(int32 Size)
	{
		return LooterUI::Font(Size, /*bBold*/ false);
	}

	inline FLinearColor Color(const FWeaponInstanceData& Instance)
	{
		return UWeaponRollLibrary::GetRarityColor(Instance.Definition, Instance.Rarity);
	}

	/**
	 * The weapon's damage, per pellet for shotguns ("7 x9"). This is the number the weapon rolled with; each hit then
	 * lands within LooterCombat::DamageVariance of it (and critical hits deal 1.5x), which the card doesn't spell out.
	 */
	inline FString DamageString(const FWeaponStats& Stats)
	{
		const FString Damage = FString::FromInt(FMath::Max(1, FMath::RoundToInt(Stats.Damage)));
		return Stats.PelletsPerShot > 1 ? FString::Printf(TEXT("%s x%d"), *Damage, Stats.PelletsPerShot) : Damage;
	}

	/** Magnification as people say it: "1.25x", "4x". */
	inline FString ZoomString(const FWeaponStats& Stats)
	{
		const int32 Hundredths = FMath::RoundToInt(Stats.Zoom * 100.f);
		const int32 Decimals = Hundredths % 100 == 0 ? 0 : (Hundredths % 10 == 0 ? 1 : 2);
		return FString::Printf(TEXT("%.*fx"), Decimals, Hundredths / 100.f);
	}

	inline FString FireModeName(const FWeaponInstanceData& Instance)
	{
		return Instance.Definition ? UEnum::GetDisplayValueAsText(Instance.Definition->FireMode).ToString() : FString();
	}

	// --- Its element and part effect (Phase 12.3): cards and labels ---

	/** "EMBER", "SOUL-LIGHT", "GRAVE-SALT"; empty for a gun without one. */
	inline FString ElementString(const FWeaponInstanceData& Instance)
	{
		return ElementRules::Word(WeaponParts::ElementOf(Instance));
	}

	/** "RICOCHET", "BURST"...; empty for a gun without one. */
	inline FString EffectString(const FWeaponInstanceData& Instance)
	{
		return PartEffectRules::Word(WeaponParts::EffectOf(Instance));
	}

	/** Both, for a label's one line: "EMBER · RICOCHET", "GRAVE-SALT", "TWIN"; empty for a gun with neither. */
	inline FString TraitLine(const FWeaponInstanceData& Instance)
	{
		const FString Element = ElementString(Instance);
		const FString Effect = EffectString(Instance);
		if (Element.IsEmpty() || Effect.IsEmpty())
		{
			return Element + Effect;
		}
		return Element + TEXT(" · ") + Effect;
	}

	/** The line's colour: its element's (LooterUI::Color::Element*), or the kit's accent for an effect alone. */
	inline FLinearColor TraitColor(const FWeaponInstanceData& Instance)
	{
		const ELooterElement Element = WeaponParts::ElementOf(Instance);
		return Element == ELooterElement::None ? LooterUI::Color::Accent() : ElementRules::UiColor(Element);
	}

	// --- Its maker (the user's call, 2026-10-22; Affixes/WeaponMakers): cards and labels ---

	/** "MERIDIAN", the line over the gun's name on its cards and label; empty for a named gun or one without a maker. */
	inline FString MakerString(const FWeaponInstanceData& Instance)
	{
		const FWeaponMaker* Maker = WeaponMakers::Of(Instance);
		return Maker ? Maker->Name.ToString().ToUpper() : FString();
	}

	/** The maker line's colour: the kit's dim text, so the gun's own name (in its rarity's colour) leads. */
	inline FLinearColor MakerColor()
	{
		return LooterUI::Color::TextDim();
	}

	/** The maker's mark as an Inked icon (drawn white beside the maker line), or null. */
	inline const LooterUI::FInkedIcon* MakerMark(const FWeaponInstanceData& Instance)
	{
		const FWeaponMaker* Maker = WeaponMakers::Of(Instance);
		return Maker ? LooterUI::FindInkedIcon(Maker->MarkIcon) : nullptr;
	}

	/** "Meridian Instrument Co. · Highwater Landing", for the inspect card; empty without a maker. */
	inline FString MakerFullLine(const FWeaponInstanceData& Instance)
	{
		const FWeaponMaker* Maker = WeaponMakers::Of(Instance);
		return Maker ? FString::Printf(TEXT("%s · %s"), *Maker->FullName.ToString(), *Maker->Place.ToString()) : FString();
	}

	/** The maker's one-line fiction, for the inspect card; empty without a maker. */
	inline FString MakerFiction(const FWeaponInstanceData& Instance)
	{
		const FWeaponMaker* Maker = WeaponMakers::Of(Instance);
		return Maker ? Maker->Fiction.ToString() : FString();
	}

	/**
	 * "TRUE GLASS: accuracy +30%, range +20%, handling +10%, fire rate -10%, magazine -20%": the quirk's name and its live
	 * numbers (a revolver's or launcher's leaves out the magazine; leanings read "element parts x3"). Empty without a maker.
	 */
	inline FString MakerQuirkLine(const FWeaponInstanceData& Instance)
	{
		const FWeaponMaker* Maker = WeaponMakers::Of(Instance);
		if (!Maker)
		{
			return FString();
		}
		const FWeaponMakerQuirk Quirk = WeaponMakers::QuirkOf(*Maker);
		const EWeaponKind Kind = Instance.Definition ? Instance.Definition->Kind : EWeaponKind::None;
		TArray<FString> Parts;
		auto Percent = [&Parts](const TCHAR* Word, float Value)
		{
			const int32 Rounded = FMath::RoundToInt(Value);
			if (Rounded != 0)
			{
				Parts.Add(FString::Printf(TEXT("%s %s%d%%"), Word, Rounded > 0 ? TEXT("+") : TEXT("-"), FMath::Abs(Rounded)));
			}
		};
		Percent(TEXT("damage"), Quirk.Damage);
		Percent(TEXT("accuracy"), Quirk.Accuracy);
		Percent(TEXT("range"), Quirk.Range);
		Percent(TEXT("fire rate"), Quirk.FireRate);
		Percent(TEXT("reload time"), Quirk.Reload);
		Percent(TEXT("recoil"), Quirk.Recoil);
		Percent(TEXT("handling"), Quirk.Handling);
		if (WeaponMakers::ChangesMagazine(Kind))
		{
			Percent(TEXT("magazine"), Quirk.Magazine);
		}
		if (!FMath::IsNearlyEqual(Quirk.ElementPartBias, 1.f))
		{
			Parts.Add(FString::Printf(TEXT("element parts x%s"), *FString::SanitizeFloat(FMath::RoundToFloat(Quirk.ElementPartBias * 10.f) / 10.f, 0)));
		}
		if (!FMath::IsNearlyEqual(Quirk.EffectPartBias, 1.f))
		{
			Parts.Add(FString::Printf(TEXT("effect parts x%s"), *FString::SanitizeFloat(FMath::RoundToFloat(Quirk.EffectPartBias * 10.f) / 10.f, 0)));
		}
		const FString Name = Maker->QuirkName.ToString().ToUpper();
		return Parts.IsEmpty() ? Name : FString::Printf(TEXT("%s: %s"), *Name, *FString::Join(Parts, TEXT(", ")));
	}

	/** 0-1 ratings for stat bars (rough scale across all weapon types). */
	inline float DamageRating(const FWeaponStats& S)   { return FMath::Clamp(S.Damage * S.PelletsPerShot / 90.f, 0.f, 1.f); }
	inline float FireRateRating(const FWeaponStats& S) { return FMath::Clamp(S.FireRate / 900.f, 0.f, 1.f); }
	inline float MagazineRating(const FWeaponStats& S) { return FMath::Clamp(S.MagazineSize / 50.f, 0.f, 1.f); }
	inline float ReloadRating(const FWeaponStats& S)   { return FMath::Clamp(1.f - (S.ReloadTime - 1.f) / 3.f, 0.f, 1.f); }
	inline float AccuracyRating(const FWeaponStats& S) { return FMath::Clamp(1.f - S.Spread / 6.f, 0.f, 1.f); }
	inline float RangeRating(const FWeaponStats& S)    { return FMath::Clamp(S.Range / 6000.f, 0.f, 1.f); }
	/** Recoil and handling are multipliers around 1 (the gun's parts move them by up to about half). */
	inline float RecoilRating(const FWeaponStats& S)   { return FMath::Clamp(1.5f - S.Recoil, 0.f, 1.f); }
	inline float HandlingRating(const FWeaponStats& S) { return FMath::Clamp(S.Handling - 0.5f, 0.f, 1.f); }
	inline float ZoomRating(const FWeaponStats& S)     { return FMath::Clamp((S.Zoom - 1.f) / 7.f, 0.f, 1.f); }
}

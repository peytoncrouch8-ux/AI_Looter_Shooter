#include "UI/Cards/GunCardStyle.h"
#include "Affixes/WeaponMakers.h"
#include "Audio/LooterSoundCues.h"
#include "UI/Style/LooterUIStyle.h"
#include "Weapons/WeaponTypes.h"

// Each maker's card look. The words and the mark come from the makers' table (Affixes/WeaponMakers.cpp: one place for a
// maker's names); the colours, deco and sound are the card's own, picked to match the paint each maker puts on its guns.

using namespace LooterUI;

namespace
{
	GunCardStyle::FStyle MakeStyle(FName Key, TArray<FLinearColor> Plates, TArray<float> Stops, const FLinearColor& Ink,
		const FLinearColor& Trim, GunCardStyle::EDeco Deco, const TCHAR* Cue)
	{
		GunCardStyle::FStyle Style;
		Style.Key = Key;
		Style.Plates = MoveTemp(Plates);
		Style.PlateStops = MoveTemp(Stops);
		Style.Ink = Ink;
		Style.Trim = Trim;
		Style.Deco = Deco;
		Style.OpenCue = FName(Cue);
		if (const FWeaponMaker* Maker = WeaponMakers::Find(Key))
		{
			Style.Word = FText::FromString(Maker->Name.ToString().ToUpper());
			Style.Quirk = FText::FromString(Maker->QuirkName.ToString().ToUpper());
			Style.MarkIcon = Maker->MarkIcon;
		}
		return Style;
	}

	/** Built on first use: the makers' table is ready by then (it's static data, but its texts are made at startup). */
	const TArray<GunCardStyle::FStyle>& Styles()
	{
		static const TArray<GunCardStyle::FStyle> Table = []
		{
			namespace Cue = LooterSoundCue::Card::Maker;
			using GunCardStyle::EDeco;
			TArray<GunCardStyle::FStyle> All;
			All.Add(MakeStyle(TEXT("Roundhouse"), { Color::MakerRoundhouse(), Color::MakerRoundhouseDark() }, {},
				Color::MakerRoundhouseInk(), Color::MakerRoundhouseTrim(), EDeco::Rivets, Cue::Roundhouse));
			All.Add(MakeStyle(TEXT("SaintMaud"), { Color::MakerSaintMaud(), Color::MakerSaintMaudDark() }, {},
				Color::MakerSaintMaudInk(), Color::MakerSaintMaudTrim(), EDeco::Gilt, Cue::SaintMaud));
			// Windfall's guns are bolted from whatever the wind dropped: its sweep is three plates that don't match.
			All.Add(MakeStyle(TEXT("Windfall"), { Color::MakerWindfallTeal(), Color::MakerWindfallRust(), Color::MakerWindfallOlive() },
				{ 0.42f, 0.70f }, Color::MakerWindfallInk(), Color::MakerWindfallTrim(), EDeco::Patch, Cue::Windfall));
			All.Add(MakeStyle(TEXT("Meridian"), { Color::MakerMeridian(), Color::MakerMeridianDark() }, {},
				Color::MakerMeridianInk(), Color::MakerMeridianTrim(), EDeco::Ticks, Cue::Meridian));
			return All;
		}();
		return Table;
	}

	FText RarityWord(EWeaponRarity Rarity)
	{
		const UEnum* Enum = StaticEnum<EWeaponRarity>();
		return FText::FromString(Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(Rarity)).ToString().ToUpper() : FString());
	}
}

const GunCardStyle::FStyle& GunCardStyle::Plain()
{
	static const FStyle Style = []
	{
		FStyle Made;
		Made.Plates = { Color::MakerPlain(), Color::MakerPlainDark() };
		Made.Ink = Color::Text();
		Made.Trim = Color::MakerPlainTrim();
		return Made;
	}();
	return Style;
}

const GunCardStyle::FStyle& GunCardStyle::ForMaker(FName MakerKey)
{
	if (!MakerKey.IsNone())
	{
		for (const FStyle& Style : Styles())
		{
			if (Style.Key == MakerKey)
			{
				return Style;
			}
		}
	}
	return Plain();
}

const GunCardStyle::FStyle& GunCardStyle::For(const FWeaponInstanceData& Gun)
{
	// A named gun keeps no maker's look (it was made by hand, by someone in the story): plain, with its rarity's word.
	const FWeaponMaker* Maker = WeaponMakers::Of(Gun);
	if (Maker)
	{
		return ForMaker(Maker->Key);
	}
	// The plain look's word is the rarity's: one per rarity, made once and never moved (backdrops keep a pointer to it).
	static const TArray<FStyle> ByRarity = []
	{
		TArray<FStyle> Made;
		for (int32 Rank = 0; Rank <= static_cast<int32>(EWeaponRarity::Legendary); ++Rank)
		{
			FStyle& Style = Made.Add_GetRef(Plain());
			Style.Word = RarityWord(static_cast<EWeaponRarity>(Rank));
		}
		return Made;
	}();
	const int32 Rank = static_cast<int32>(Gun.Rarity);
	return ByRarity.IsValidIndex(Rank) ? ByRarity[Rank] : Plain();
}

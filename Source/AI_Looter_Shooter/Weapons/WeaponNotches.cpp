#include "Weapons/WeaponNotches.h"
#include "Weapons/WeaponCurses.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponParts.h"

namespace
{
	/**
	 * The nicknames guns earn at NamedKills, one per gun picked by its seed. A gun's nickname is its list's entry at its
	 * seed, so the lists stay as they are once guns are named: reordering, removing or adding names would rename guns
	 * already named. Original weird-west words, none borrowed from other games or films.
	 */
	const TCHAR* const RifleNicknames[] = {
		TEXT("Lantern Jaw"),
		TEXT("Gravel Psalm"),
		TEXT("Buzzard's Due"),
		TEXT("Sundown Tally"),
		TEXT("Coffin Chatter"),
		TEXT("Rust Deacon"),
		TEXT("Mesa Whistle"),
		TEXT("Long Supper"),
		TEXT("Tin Sermon"),
		TEXT("Dust Ledger"),
		TEXT("Saint of Small Hours"),
		TEXT("Bone Waltz"),
	};

	const TCHAR* const ShotgunNicknames[] = {
		TEXT("Porch Preacher"),
		TEXT("Grave Rake"),
		TEXT("Buckshot Bishop"),
		TEXT("Iron Lullaby"),
		TEXT("Old Stubborn"),
		TEXT("Crow Scatter"),
		TEXT("Chapel Thunder"),
		TEXT("Widow's Welcome"),
		TEXT("Tumbleweed Ruin"),
		TEXT("Rattlebox"),
		TEXT("Salt and Sorrow"),
		TEXT("Hailmouth"),
	};

	const TCHAR* const RevolverNicknames[] = {
		TEXT("Six Feet Even"),
		TEXT("Last Word"),
		TEXT("Parlor Thunder"),
		TEXT("Undertaker's Friend"),
		TEXT("Graveside Manner"),
		TEXT("Hangtree Waltz"),
		TEXT("Penny for the Ferryman"),
		TEXT("Quiet Supper"),
		TEXT("Short Sermon"),
		TEXT("Brass Psalter"),
		TEXT("Midnight Ledger"),
		TEXT("Closing Hymn"),
		TEXT("Sweet Dismissal"),
		TEXT("Whistling Widow"),
	};

	/** The nickname the gun's seed picks from its kind's list, whatever its kills; empty for a named gun (it has a name). */
	FString PickNickname(const FWeaponInstanceData& Gun)
	{
		if (Gun.Named)
		{
			return FString();
		}
		const TConstArrayView<const TCHAR*> List = WeaponNotches::Nicknames(Gun.Definition ? Gun.Definition->Kind : EWeaponKind::None);
		if (List.IsEmpty())
		{
			return FString();
		}
		// A stream of its own from the seed, so the nickname never follows the parts', the stats' or the curse's rolls.
		FRandomStream Random(static_cast<int32>(HashCombine(static_cast<uint32>(Gun.Seed), 0x4E1C4A3Eu)));
		return List[Random.RandHelper(List.Num())];
	}

	/** The gun's name as its messages say it: with its nickname once it has one. */
	FString MessageName(const FWeaponInstanceData& Gun)
	{
		const FString Base = WeaponParts::BaseName(Gun);
		const FString Earned = WeaponNotches::Nickname(Gun);
		return Earned.IsEmpty() ? Base : FString::Printf(TEXT("%s \"%s\""), *Base, *Earned);
	}

	/** The damage a gun has in all at the tier, in whole percent (3, 6, 10). */
	int32 TierDamagePercent(ENotchTier Tier)
	{
		const int32 Kills = Tier == ENotchTier::SoulForged ? WeaponNotches::SoulForgedKills
			: Tier == ENotchTier::Named ? WeaponNotches::NamedKills
			: Tier == ENotchTier::Blooded ? WeaponNotches::BloodedKills : 0;
		return FMath::RoundToInt((WeaponNotches::DamageMultiplier(Kills) - 1.f) * 100.f);
	}
}

ENotchTier WeaponNotches::TierFor(int32 Kills)
{
	if (Kills >= SoulForgedKills)
	{
		return ENotchTier::SoulForged;
	}
	if (Kills >= NamedKills)
	{
		return ENotchTier::Named;
	}
	return Kills >= BloodedKills ? ENotchTier::Blooded : ENotchTier::None;
}

float WeaponNotches::DamageMultiplier(int32 Kills)
{
	// +3% at each of the first two milestones and +4% at the last: 10% in all for a gun that has killed a thousand.
	switch (TierFor(Kills))
	{
	case ENotchTier::Blooded:
		return 1.03f;
	case ENotchTier::Named:
		return 1.06f;
	case ENotchTier::SoulForged:
		return 1.10f;
	default:
		return 1.f;
	}
}

int32 WeaponNotches::Marks(int32 Kills)
{
	return FMath::Clamp(Kills / KillsPerMark, 0, MaxMarks);
}

FText WeaponNotches::TierName(ENotchTier Tier)
{
	switch (Tier)
	{
	case ENotchTier::Blooded:
		return FText::FromString(TEXT("Blooded"));
	case ENotchTier::Named:
		return FText::FromString(TEXT("Named"));
	case ENotchTier::SoulForged:
		return FText::FromString(TEXT("Soul-forged"));
	default:
		return FText::GetEmpty();
	}
}

WeaponNotches::FKillResult WeaponNotches::AddKill(FWeaponInstanceData& Gun)
{
	FKillResult Result;
	const ENotchTier Before = TierFor(Gun.Kills);
	Gun.Kills = Gun.Kills < MAX_int32 ? FMath::Max(Gun.Kills, 0) + 1 : Gun.Kills;
	const ENotchTier After = TierFor(Gun.Kills);
	Result.Reached = After != Before ? After : ENotchTier::None;

	// A cursed iron carried through its curse earns its way out of the drawback; the perk stays.
	if (Gun.Kills >= CurseLiftKills && !Gun.bCurseLifted && WeaponCurses::Of(Gun))
	{
		Gun.bCurseLifted = true;
		Result.bCurseLifted = true;
	}
	return Result;
}

FString WeaponNotches::Nickname(const FWeaponInstanceData& Gun)
{
	return TierFor(Gun.Kills) >= ENotchTier::Named ? PickNickname(Gun) : FString();
}

TConstArrayView<const TCHAR*> WeaponNotches::Nicknames(EWeaponKind Kind)
{
	if (Kind == EWeaponKind::Shotgun)
	{
		return MakeArrayView(ShotgunNicknames);
	}
	if (Kind == EWeaponKind::Revolver)
	{
		return MakeArrayView(RevolverNicknames);
	}
	return MakeArrayView(RifleNicknames);
}

FText WeaponNotches::MilestoneMessage(const FWeaponInstanceData& Gun, ENotchTier Tier)
{
	// Worded like the inventory's messages ("Heirloom sent to backpack"); the HUD's message line shows them in capitals.
	const int32 Percent = TierDamagePercent(Tier);
	switch (Tier)
	{
	case ENotchTier::Blooded:
		return FText::FromString(FString::Printf(TEXT("%s is Blooded · +%d%% damage"), *MessageName(Gun), Percent));
	case ENotchTier::Named:
	{
		// The nickname is the news here, so it follows the word rather than the gun's name.
		const FString Earned = PickNickname(Gun);
		return FText::FromString(Earned.IsEmpty()
			? FString::Printf(TEXT("%s is Named · +%d%% damage"), *WeaponParts::BaseName(Gun), Percent)
			: FString::Printf(TEXT("%s is Named \"%s\" · +%d%% damage"), *WeaponParts::BaseName(Gun), *Earned, Percent));
	}
	case ENotchTier::SoulForged:
		return FText::FromString(FString::Printf(TEXT("%s is Soul-forged · +%d%% damage"), *MessageName(Gun), Percent));
	default:
		return FText::GetEmpty();
	}
}

FText WeaponNotches::CurseLiftedMessage(const FWeaponInstanceData& Gun)
{
	return FText::FromString(FString::Printf(TEXT("%s's curse is lifted"), *MessageName(Gun)));
}

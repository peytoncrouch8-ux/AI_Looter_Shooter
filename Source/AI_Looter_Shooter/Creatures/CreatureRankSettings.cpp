#include "Creatures/CreatureRankSettings.h"
#include "AI_Looter_Shooter.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootTable.h"
#include "Misc/PackageName.h"
#include "UI/Style/LooterUIStyle.h"

namespace
{
	/** Where each rank's table asset lives, beside the default one (none for Basic: the creature's own table). */
	const TCHAR* RankTablePaths[] = {
		nullptr,
		TEXT("/Game/Weapons/Data/DA_LootTable_Rare.DA_LootTable_Rare"),
		TEXT("/Game/Weapons/Data/DA_LootTable_Epic.DA_LootTable_Epic"),
		TEXT("/Game/Weapons/Data/DA_LootTable_Legendary.DA_LootTable_Legendary"),
		TEXT("/Game/Weapons/Data/DA_LootTable_Boss.DA_LootTable_Boss") };
	static_assert(UE_ARRAY_COUNT(RankTablePaths) == LooterRanks::NumRanks, "A table path per rank");

	FCreatureRankInfo MakeRank(const TCHAR* Word, const FLinearColor& Color, float Size, float Health, float Damage, float XP,
		int32 Levels, ECreatureRank Rank)
	{
		FCreatureRankInfo Info;
		Info.Word = Word ? FText::FromString(Word) : FText::GetEmpty();
		Info.Color = Color;
		Info.Size = Size;
		Info.HealthMultiplier = Health;
		Info.DamageMultiplier = Damage;
		Info.XPMultiplier = XP;
		Info.LevelOffset = Levels;
		if (const TCHAR* Path = RankTablePaths[static_cast<int32>(Rank)])
		{
			Info.LootTable = TSoftObjectPtr<ULootTable>(FSoftObjectPath(Path));
		}
		return Info;
	}
}

UCreatureRankSettings::UCreatureRankSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("Creature Ranks");

	// The tag colors are the rarity colors of the loot beams (UWeaponDefinition's rarity table, as sRGB): soul-light uses
	// the game's one color code. Sizes, stats and levels are the design's (Docs/Story.md); bosses set their own size.
	// Each: word, color, size, health, damage and experience multipliers, levels on top.
	Basic = MakeRank(nullptr, LooterUI::Color::Text(), 1.f, 1.f, 1.f, 1.f, 0, ECreatureRank::Basic);
	Rare = MakeRank(TEXT("Restless"), LooterUI::Hex(124, 170, 255), 1.1f, 2.5f, 1.2f, 2.f, 1, ECreatureRank::Rare);
	Epic = MakeRank(TEXT("Gravebound"), LooterUI::Hex(204, 124, 244), 1.2f, 5.f, 1.4f, 4.f, 1, ECreatureRank::Epic);
	Legendary = MakeRank(TEXT("Soulfed"), LooterUI::Hex(255, 204, 0), 1.4f, 12.f, 1.6f, 10.f, 2, ECreatureRank::Legendary);
	Boss = MakeRank(nullptr, LooterUI::Hex(255, 204, 0), 1.f, 1.f, 1.f, 20.f, 1, ECreatureRank::Boss);

	// Gravebound and Soulfed spiders call every spider near them (Docs/Areas/RansomsRest.md, "Enemies by rank").
	Epic.PackCallRadius = 3000.f;
	Legendary.PackCallRadius = 3000.f;

	// Legendary monsters and bosses come back only when their area says so (after 20 minutes of play, or never).
	Legendary.bRespawns = false;
	Boss.bRespawns = false;
}

const FCreatureRankInfo& UCreatureRankSettings::Get(ECreatureRank Rank)
{
	const UCreatureRankSettings* Settings = GetDefault<UCreatureRankSettings>();
	switch (Rank)
	{
	case ECreatureRank::Rare:
		return Settings->Rare;
	case ECreatureRank::Epic:
		return Settings->Epic;
	case ECreatureRank::Legendary:
		return Settings->Legendary;
	case ECreatureRank::Boss:
		return Settings->Boss;
	default:
		return Settings->Basic;
	}
}

UCreatureRankSettings::FRankLootOdds UCreatureRankSettings::DesignLootOdds(ECreatureRank Rank)
{
	// Docs/Story.md: the guns come more often and in greater numbers up the ranks, at most Luck 1.3, where Common is still
	// the likeliest rarity (Uncommon passes it above 1.4). Each rank drops more ammo pickups, each still 18-36 rounds.
	// Each: chance a kill drops guns, fewest and most guns, luck, fewest and most ammo pickups.
	switch (Rank)
	{
	case ECreatureRank::Rare:
		return { 0.6f, 1, 1, 0.5f, 2, 3 };
	case ECreatureRank::Epic:
		return { 1.f, 1, 2, 1.f, 3, 4 };
	case ECreatureRank::Legendary:
		return { 1.f, 2, 2, 1.3f, 4, 6 };
	case ECreatureRank::Boss:
		return { 1.f, 3, 3, 1.3f, 8, 10 };
	default:
		// DA_LootTable_Default's, which stays as it is.
		return { 0.3f, 1, 1, 0.f, 1, 2 };
	}
}

ULootTable* UCreatureRankSettings::GetLootTable(ECreatureRank Rank)
{
	const TSoftObjectPtr<ULootTable>& Asset = Get(Rank).LootTable;
	if (ULootTable* Loaded = Asset.Get())
	{
		return Loaded;
	}
	// Only load what's there: a missing asset would be looked for (and warned about) on every call.
	if (!Asset.IsNull() && FPackageName::DoesPackageExist(Asset.GetLongPackageName()))
	{
		if (ULootTable* Loaded = Asset.LoadSynchronous())
		{
			return Loaded;
		}
	}
	if (Rank == ECreatureRank::Basic)
	{
		return ULootLibrary::GetDefaultLootTable();
	}

	// No asset yet: a stand-in with the design's odds and the default table's guns and ammo, made once.
	UCreatureRankSettings* Settings = GetMutableDefault<UCreatureRankSettings>();
	const int32 Index = static_cast<int32>(Rank);
	if (Settings->StandInTables.Num() < LooterRanks::NumRanks)
	{
		Settings->StandInTables.SetNum(LooterRanks::NumRanks);
	}
	if (ULootTable* StandIn = Settings->StandInTables[Index])
	{
		return StandIn;
	}
	ULootTable* StandIn = NewObject<ULootTable>(Settings, *FString::Printf(TEXT("StandIn_LootTable_%s"), *GetRankName(Rank)), RF_Transient);
	if (const ULootTable* Default = ULootLibrary::GetDefaultLootTable())
	{
		StandIn->Entries = Default->Entries;
		StandIn->AmmoTypes = Default->AmmoTypes;
		StandIn->AmmoDropChance = Default->AmmoDropChance;
		StandIn->KillWeaponAmmoBias = Default->KillWeaponAmmoBias;
		StandIn->AmmoAmountMin = Default->AmmoAmountMin;
		StandIn->AmmoAmountMax = Default->AmmoAmountMax;
	}
	const FRankLootOdds Odds = DesignLootOdds(Rank);
	StandIn->WeaponDropChance = Odds.WeaponDropChance;
	StandIn->MinWeaponDrops = Odds.MinWeaponDrops;
	StandIn->MaxWeaponDrops = Odds.MaxWeaponDrops;
	StandIn->Luck = Odds.Luck;
	StandIn->MinAmmoDrops = Odds.MinAmmoDrops;
	StandIn->MaxAmmoDrops = Odds.MaxAmmoDrops;
	Settings->StandInTables[Index] = StandIn;
	UE_LOG(LogLooter, Log, TEXT("Creature ranks: %s has no loot table asset yet (%s); using a stand-in with the same odds. ")
		TEXT("Make the assets with Tools/Unreal/create_rank_assets.py."), *GetRankName(Rank), *Asset.ToString());
	return StandIn;
}

FString UCreatureRankSettings::GetRankName(ECreatureRank Rank)
{
	return StaticEnum<ECreatureRank>()->GetNameStringByValue(static_cast<int64>(Rank));
}

bool UCreatureRankSettings::ParseRank(const FString& Text, ECreatureRank& OutRank)
{
	const FString Wanted = Text.TrimStartAndEnd();
	for (const ECreatureRank Candidate : LooterRanks::All)
	{
		const FText& Word = Get(Candidate).Word;
		if (Wanted.Equals(GetRankName(Candidate), ESearchCase::IgnoreCase)
			|| (!Word.IsEmpty() && Wanted.Equals(Word.ToString(), ESearchCase::IgnoreCase)))
		{
			OutRank = Candidate;
			return true;
		}
	}
	return false;
}

#include "Creatures/CreatureRankSettings.h"
#include "AI_Looter_Shooter.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootTable.h"
#include "Misc/PackageName.h"
#include "UI/Style/LooterUIStyle.h"

namespace
{
	/** Where each rank's table asset lives, beside the default one (none for Basic: the creature's own table). */
	const TCHAR* RankTablePath(ECreatureRank Rank)
	{
		switch (Rank)
		{
		case ECreatureRank::Rare: return TEXT("/Game/Weapons/Data/DA_LootTable_Rare.DA_LootTable_Rare");
		case ECreatureRank::Epic: return TEXT("/Game/Weapons/Data/DA_LootTable_Epic.DA_LootTable_Epic");
		case ECreatureRank::Hallowed: return TEXT("/Game/Weapons/Data/DA_LootTable_Hallowed.DA_LootTable_Hallowed");
		case ECreatureRank::Ethereal: return TEXT("/Game/Weapons/Data/DA_LootTable_Ethereal.DA_LootTable_Ethereal");
		case ECreatureRank::Legendary: return TEXT("/Game/Weapons/Data/DA_LootTable_Legendary.DA_LootTable_Legendary");
		case ECreatureRank::Boss: return TEXT("/Game/Weapons/Data/DA_LootTable_Boss.DA_LootTable_Boss");
		default: return nullptr;
		}
	}

	/** One rank's tag, body and stats: word, color, size, health, damage and experience multipliers, levels on top, defense. */
	FCreatureRankInfo MakeRank(const TCHAR* Word, const FLinearColor& Color, float Size, float Health, float Damage, float XP,
		int32 Levels, float Defense, ECreatureRank Rank)
	{
		FCreatureRankInfo Info;
		Info.Word = Word ? FText::FromString(Word) : FText::GetEmpty();
		Info.Color = Color;
		Info.GlowColor = Color;
		Info.Size = Size;
		Info.HealthMultiplier = Health;
		Info.DamageMultiplier = Damage;
		Info.XPMultiplier = XP;
		Info.LevelOffset = Levels;
		Info.Defense = Defense;
		if (const TCHAR* Path = RankTablePath(Rank))
		{
			Info.LootTable = TSoftObjectPtr<ULootTable>(FSoftObjectPath(Path));
		}
		return Info;
	}

	/** A tier's traits, silver, spawn chances (Ransom's Rest, the late areas) and cap. */
	void SetTier(FCreatureRankInfo& Info, int32 Traits, float Silver, float First, float Late, int32 MaxAlive)
	{
		Info.TraitCount = Traits;
		Info.SilverMultiplier = Silver;
		Info.SpawnChanceFirst = First;
		Info.SpawnChanceLate = Late;
		Info.MaxAlivePerArea = MaxAlive;
	}
}

UCreatureRankSettings::UCreatureRankSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("Creature Ranks");

	// The design the user approved on 2026-10-20 (Saved/Tiers/DESIGN.md, Docs/Story.md): five tiers for every regular enemy,
	// each tougher (health, defense, damage, levels, size), rarer and better paid; Soulfed and bosses as they were. At the
	// valley's middle level, effective health against a Regular one is 2.4, 4.0, 5.5 and 7.0 up the tiers.
	// Each: word, color, size, health, damage and experience multipliers, levels on top, defense.
	Basic = MakeRank(nullptr, LooterUI::Color::TierRegular(), 1.f, 1.f, 1.f, 1.f, 0, 0.f, ECreatureRank::Basic);
	Rare = MakeRank(TEXT("Restless"), LooterUI::Color::TierRestless(), 1.08f, 2.f, 1.15f, 2.f, 1, 0.1f, ECreatureRank::Rare);
	Epic = MakeRank(TEXT("Gravebound"), LooterUI::Color::TierGravebound(), 1.15f, 3.f, 1.3f, 3.5f, 1, 0.2f, ECreatureRank::Epic);
	// Health 3.4, was 3.5: with growth compounding (+9% a level, 2026-10-29 night) its two levels on top grew more, and 3.4
	// keeps the approved 5.5 effective health (5.6; 3.5 gave 5.78).
	Hallowed = MakeRank(TEXT("Hallowed"), LooterUI::Color::TierHallowed(), 1.22f, 3.4f, 1.45f, 6.f, 2, 0.28f, ECreatureRank::Hallowed);
	Ethereal = MakeRank(TEXT("Ethereal"), LooterUI::Color::TierEthereal(), 1.3f, 4.f, 1.6f, 9.f, 2, 0.35f, ECreatureRank::Ethereal);
	// Soulfed's and a boss's tag: the legendary orange, #FF9900 (Main's call, 2026-10-20: the old #FFCC00 gold sat too near
	// Hallowed's saint-gold #F5C842).
	Legendary = MakeRank(TEXT("Soulfed"), LooterUI::Color::TierSoulfed(), 1.4f, 12.f, 1.6f, 10.f, 2, 0.f, ECreatureRank::Legendary);
	Boss = MakeRank(nullptr, LooterUI::Color::TierSoulfed(), 1.f, 1.f, 1.f, 20.f, 1, 0.f, ECreatureRank::Boss);

	// Each tier: traits, silver (Restless and Gravebound pay their own amounts, as before the top tiers: 1; Hallowed and
	// Ethereal Gravebound's times this: LooterEconomy::KillOdds), its share of regular spawns on Ransom's Rest and by the late
	// areas, and the most alive at once in an area for a roll to make another.
	SetTier(Rare, 0, 1.f, 0.08f, 0.12f, 0);
	SetTier(Epic, 1, 1.f, 0.02f, 0.04f, 4);
	SetTier(Hallowed, 2, 1.6f, 0.004f, 0.01f, 2);
	SetTier(Ethereal, 3, 2.5f, 0.0008f, 0.0025f, 1);

	// Gravebound and better spiders call every spider near them (Docs/Areas/RansomsRest.md, "Enemies by rank").
	Epic.PackCallRadius = 3000.f;
	Hallowed.PackCallRadius = 3000.f;
	Ethereal.PackCallRadius = 3000.f;
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
	case ECreatureRank::Hallowed:
		return Settings->Hallowed;
	case ECreatureRank::Ethereal:
		return Settings->Ethereal;
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
	// The design (Docs/Story.md): the guns come more often and in greater numbers up the tiers, at most Luck 1.3, where
	// Common is still the likeliest rarity (Uncommon passes it above 1.4). Each tier drops more ammo pickups, each still
	// 18-36 rounds. Each: chance a kill drops guns, fewest and most guns, luck, fewest and most ammo pickups, and whether its
	// legendaries keep their odds in the early areas (the user, 2026-10-27: "Rarer, bosses still drop": Soulfed and bosses do;
	// the tiers' are capped there, ULootLibrary::LegendaryCapFor).
	switch (Rank)
	{
	case ECreatureRank::Rare:
		return { 0.6f, 1, 1, 0.5f, 2, 3 };
	case ECreatureRank::Epic:
		return { 0.9f, 1, 1, 0.8f, 2, 4 };
	case ECreatureRank::Hallowed:
		return { 1.f, 1, 2, 1.1f, 3, 5 };
	case ECreatureRank::Ethereal:
		return { 1.f, 2, 2, 1.3f, 4, 6 };
	case ECreatureRank::Legendary:
		return { 1.f, 2, 2, 1.3f, 4, 6, true };
	case ECreatureRank::Boss:
		return { 1.f, 3, 3, 1.3f, 8, 10, true };
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

	// No asset yet: a stand-in with the design's odds and the default table's guns and ammo, made once (kept by the rank's place).
	UCreatureRankSettings* Settings = GetMutableDefault<UCreatureRankSettings>();
	const int32 Index = LooterRanks::Order(Rank);
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
	StandIn->bKeepsLegendaryOdds = Odds.bKeepsLegendaryOdds;
	Settings->StandInTables[Index] = StandIn;
	UE_LOG(LogLooter, Log, TEXT("Creature ranks: %s has no loot table asset yet (%s); using a stand-in with the same odds. ")
		TEXT("Make the assets with Tools/Unreal/create_rank_assets.py."), *GetRankName(Rank), *Asset.ToString());
	return StandIn;
}

FString UCreatureRankSettings::GetRankName(ECreatureRank Rank)
{
	return StaticEnum<ECreatureRank>()->GetNameStringByValue(static_cast<int64>(Rank));
}

FString UCreatureRankSettings::GetTierName(ECreatureRank Rank)
{
	if (Rank == ECreatureRank::Basic)
	{
		return TEXT("Regular");
	}
	const FText& Word = Get(Rank).Word;
	return Word.IsEmpty() ? GetRankName(Rank) : Word.ToString();
}

bool UCreatureRankSettings::ParseRank(const FString& Text, ECreatureRank& OutRank)
{
	const FString Wanted = Text.TrimStartAndEnd();
	for (const ECreatureRank Candidate : LooterRanks::All)
	{
		const FText& Word = Get(Candidate).Word;
		if (Wanted.Equals(GetRankName(Candidate), ESearchCase::IgnoreCase) || Wanted.Equals(GetTierName(Candidate), ESearchCase::IgnoreCase)
			|| (!Word.IsEmpty() && Wanted.Equals(Word.ToString(), ESearchCase::IgnoreCase)))
		{
			OutRank = Candidate;
			return true;
		}
	}
	return false;
}

// ---------------------------------------------------------------------------
// Spawns
// ---------------------------------------------------------------------------

float UCreatureRankSettings::CampaignRise(int32 MinLevel)
{
	const UCreatureRankSettings* Settings = GetDefault<UCreatureRankSettings>();
	const int32 First = FMath::Max(Settings->FirstAreaLevel, 1);
	const int32 Late = FMath::Max(Settings->LateAreaLevel, First + 1);
	return FMath::Clamp(static_cast<float>(MinLevel - First) / static_cast<float>(Late - First), 0.f, 1.f);
}

float UCreatureRankSettings::SpawnChance(ECreatureRank Rank, float Rise)
{
	if (!LooterRanks::IsTier(Rank) || Rank == ECreatureRank::Basic)
	{
		return 0.f;
	}
	const FCreatureRankInfo& Info = Get(Rank);
	return FMath::Clamp(FMath::Lerp(Info.SpawnChanceFirst, Info.SpawnChanceLate, FMath::Clamp(Rise, 0.f, 1.f)), 0.f, 1.f);
}

ECreatureRank UCreatureRankSettings::PickTier(float Roll, float Rise)
{
	// The rarest first, so chances that add up past 1 still give it its share.
	float Below = 0.f;
	for (int32 Index = LooterRanks::NumTiers - 1; Index >= 1; --Index)
	{
		const ECreatureRank Tier = LooterRanks::Tiers[Index];
		Below += SpawnChance(Tier, Rise);
		if (Roll < Below)
		{
			return Tier;
		}
	}
	return ECreatureRank::Basic;
}

ECreatureRank UCreatureRankSettings::KeepUnderCaps(ECreatureRank Rolled, TFunctionRef<int32(ECreatureRank)> AliveOf)
{
	ECreatureRank Kept = Rolled;
	while (LooterRanks::IsTier(Kept) && Kept != ECreatureRank::Basic)
	{
		const int32 Cap = Get(Kept).MaxAlivePerArea;
		if (Cap <= 0 || AliveOf(Kept) < Cap)
		{
			break;
		}
		Kept = LooterRanks::TierBelow(Kept);
	}
	return Kept;
}

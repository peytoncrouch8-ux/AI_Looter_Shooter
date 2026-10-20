// Developer console command for the enemy tiers (not in shipping builds): Looter.Tier.Odds prints what each tier is and
// gives, from the same rules the game rolls (UCreatureRankSettings, the loot tables, LooterEconomy).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Areas/AreaDefinition.h"
#include "Combat/HealthComponent.h"
#include "Combat/LooterDamageTypes.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Economy/EconomyRules.h"
#include "Loot/LootOdds.h"
#include "Loot/LootTable.h"
#include "Progression/ProgressionSettings.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RandomStream.h"

namespace
{
	/** Spawns rolled at Rise from a fixed seed: each tier's share against its chance, for the log. */
	FString DescribeSpawns(float Rise, int32 Spawns)
	{
		int32 Counts[LooterRanks::NumTiers] = {};
		FRandomStream Random(20261020);
		for (int32 Each = 0; Each < Spawns; ++Each)
		{
			++Counts[FMath::Max(LooterRanks::TierIndex(UCreatureRankSettings::PickTier(Random.FRand(), Rise)), 0)];
		}
		FString Text;
		for (int32 Tier = 1; Tier < LooterRanks::NumTiers; ++Tier)
		{
			const ECreatureRank Rank = LooterRanks::Tiers[Tier];
			const double Share = static_cast<double>(Counts[Tier]) / FMath::Max(Spawns, 1);
			const double Chance = UCreatureRankSettings::SpawnChance(Rank, Rise);
			Text += FString::Printf(TEXT("%s%s %.3f%% (chance %.3f%%, 1 in %.0f)"), Text.IsEmpty() ? TEXT("") : TEXT(", "),
				*UCreatureRankSettings::GetTierName(Rank), Share * 100.0, Chance * 100.0, Chance > 0.0 ? 1.0 / Chance : 0.0);
		}
		return Text;
	}

	void TierOdds(const TArray<FString>& Args)
	{
		const int32 Kills = Args.Num() > 0 ? FMath::Clamp(FCString::Atoi(*Args[0]), 1000, 5000000) : 200000;

		// Spawns: Ransom's Rest (its place in the campaign 0), the areas between, and the late ones (1).
		UE_LOG(LogLooter, Display, TEXT("Looter.Tier.Odds: %d spawns and kills a tier (seeded: the same numbers every time)."), Kills);
		UE_LOG(LogLooter, Display, TEXT("  spawns on Ransom's Rest: %s"), *DescribeSpawns(0.f, Kills));
		for (const UAreaDefinition* Area : UAreaDefinition::LoadAll())
		{
			if (Area->HasPromotions() && Area->GetCampaignRise() > 0.f)
			{
				UE_LOG(LogLooter, Display, TEXT("  spawns in %s (levels from %d, place %.2f): %s"), *Area->DisplayName.ToString(), Area->MinLevel,
					Area->GetCampaignRise(), *DescribeSpawns(Area->GetCampaignRise(), Kills));
			}
		}
		UE_LOG(LogLooter, Display, TEXT("  spawns by the late areas (levels from %d): %s"), GetDefault<UCreatureRankSettings>()->LateAreaLevel,
			*DescribeSpawns(1.f, Kills));

		// Each rank: toughness at the valley's middle level, then what a kill gives.
		const FLevelRules Levels = GetDefault<UProgressionSettings>()->GetLevelRules();
		constexpr int32 MidLevel = 4;
		for (const ECreatureRank Rank : LooterRanks::All)
		{
			const FCreatureRankInfo& Info = UCreatureRankSettings::Get(Rank);
			const float LevelGrowth = Levels.EnemyScale(MidLevel + Info.LevelOffset) / Levels.EnemyScale(MidLevel);
			const float Effective = Info.HealthMultiplier * LevelGrowth / FMath::Max(1.f - Info.Defense, 0.1f);
			UE_LOG(LogLooter, Display, TEXT("  %s (%s): health x%.2f, defense %.0f%% (%.0f%% on a crit), damage x%.2f, +%d levels, size %.2f, ")
				TEXT("%d traits, XP x%.1f, silver x%.1f; at level %d it takes %.2fx a Regular one's body shots; cap %d alive"),
				*UCreatureRankSettings::GetTierName(Rank), *UCreatureRankSettings::GetRankName(Rank), Info.HealthMultiplier, Info.Defense * 100.f,
				Info.Defense * 50.f, Info.DamageMultiplier, Info.LevelOffset, Info.Size, Info.TraitCount, Info.XPMultiplier, Info.SilverMultiplier,
				MidLevel, Effective, Info.MaxAlivePerArea);

			const ULootTable* Table = UCreatureRankSettings::GetLootTable(Rank);
			if (!Table)
			{
				continue;
			}
			const LootOdds::FTally Tally = LootOdds::Simulate(Table, Kills, 20261020 + LooterRanks::Order(Rank));
			const LootOdds::FExpected Expected = LootOdds::Expected(Table);
			double Shares[LootOdds::NumRarities];
			for (int32 Tier = 0; Tier < LootOdds::NumRarities; ++Tier)
			{
				Shares[Tier] = Tally.RarityShare(Tier);
			}
			const double Legendary = Tally.PerKill(Tally.KillsWithLegendary);
			UE_LOG(LogLooter, Display, TEXT("    loot (%s%s): guns on %.1f%% of kills, %.2f a kill; a legendary on %.2f%% (1 in %.1f; table %.2f%%); %s; ")
				TEXT("%.2f ammo pickups; silver %.2f an Unpaid, %.2f a beast (level 1)"), *Table->GetName(),
				Table->HasAnyFlags(RF_Transient) ? TEXT(", a stand-in") : TEXT(""), Tally.PerKill(Tally.KillsWithWeapons) * 100.0,
				Tally.PerKill(Tally.Weapons), Legendary * 100.0, Legendary > 0.0 ? 1.0 / Legendary : 0.0, Expected.LegendaryPerKill * 100.0,
				*LootOdds::DescribeShares(Shares), Tally.PerKill(Tally.AmmoPickups), LooterEconomy::KillOdds(Rank, true).Average(),
				LooterEconomy::KillOdds(Rank, false).Average());
		}
	}

	/** The game world the command is for: the one it was typed in, or the running play session when typed in the editor. */
	UWorld* FindGameWorld(UWorld* World)
	{
		if (World && World->IsGameWorld())
		{
			return World;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && Context.World()->IsGameWorld())
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	/** Looter.Tier.Hit [damage] [crit]: the nearest living creature takes one hit as the player's shot, to see its defense at work. */
	void TierHit(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Pawn)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Tier.Hit: start the game first."));
			return;
		}
		const float Damage = Args.Num() > 0 ? FMath::Max(FCString::Atof(*Args[0]), 1.f) : 100.f;
		const bool bCritical = Args.ContainsByPredicate([](const FString& Arg) { return Arg.Equals(TEXT("crit"), ESearchCase::IgnoreCase); });
		ACreatureBase* Nearest = nullptr;
		double NearestSquared = TNumericLimits<double>::Max();
		for (TActorIterator<ACreatureBase> It(GameWorld); It; ++It)
		{
			const double Squared = FVector::DistSquared(It->GetActorLocation(), Pawn->GetActorLocation());
			if (!It->IsDead() && Squared < NearestSquared)
			{
				Nearest = *It;
				NearestSquared = Squared;
			}
		}
		UHealthComponent* Health = Nearest ? Nearest->FindComponentByClass<UHealthComponent>() : nullptr;
		if (!Health)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Tier.Hit: no living creature."));
			return;
		}
		const float Before = Health->GetHealth();
		UGameplayStatics::ApplyPointDamage(Nearest, Damage, (Nearest->GetActorLocation() - Pawn->GetActorLocation()).GetSafeNormal(),
			FHitResult(Nearest, nullptr, Nearest->GetActorLocation(), FVector::UpVector), Controller, Pawn,
			bCritical ? UWeaponCritDamageType::StaticClass() : UWeaponDamageType::StaticClass());
		UE_LOG(LogLooter, Log, TEXT("Looter.Tier.Hit: %s (%s, %.0f%% defense) took %.1f of a %s for %.0f: %.1f -> %.1f of %.0f."), *Nearest->GetName(),
			*UCreatureRankSettings::GetTierName(Nearest->GetRank()), Nearest->GetDefense() * 100.f, Before - Health->GetHealth(),
			bCritical ? TEXT("crit") : TEXT("body shot"), Damage, Before, Health->GetHealth(), Health->GetMaxHealth());
	}

	FAutoConsoleCommandWithWorldAndArgs TierHitCommand(
		TEXT("Looter.Tier.Hit"),
		TEXT("Looter.Tier.Hit [damage, 100 by default] [crit]: the nearest living creature takes one hit as the player's shot (a crit ")
		TEXT("with crit), and the log says what its tier's defense let through."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TierHit));

	FAutoConsoleCommand TierOddsCommand(
		TEXT("Looter.Tier.Odds"),
		TEXT("Looter.Tier.Odds [kills, 200000 by default]: rolls that many regular spawns on Ransom's Rest, in each area between and ")
		TEXT("by the late areas (each tier's share against its chance), then each rank's toughness at level 4 and that many kills of ")
		TEXT("its loot table (guns, legendaries, rarities, ammo) and its silver."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&TierOdds));
}

#endif

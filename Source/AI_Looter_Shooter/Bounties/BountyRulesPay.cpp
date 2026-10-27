// BountyRules: the timer (rotations, waits in words), the pay (purse, experience, the loot handed over at the board) and
// the names in code for logs and commands.

#include "Bounties/BountyRules.h"
#include "Bounties/BountyTuning.h"
#include "Economy/EconomyRules.h"
#include "Loot/Chest.h"
#include "Progression/LevelRules.h"
#include "Progression/XPCurve.h"

#define LOCTEXT_NAMESPACE "LooterBounty"

namespace
{
	constexpr EBountyTwist AllTwists[] = { EBountyTwist::Fleet, EBountyTwist::Warded, EBountyTwist::Mending, EBountyTwist::Rallying,
		EBountyTwist::Vengeful, EBountyTwist::GraveBurst, EBountyTwist::BigPosse, EBountyTwist::SwiftPosse };
	constexpr EBountyTier AllTiers[] = { EBountyTier::Wanted, EBountyTier::Dangerous, EBountyTier::Deadly };
}

// ---------------------------------------------------------------------------
// The timer
// ---------------------------------------------------------------------------

bool BountyRules::IsRotationDue(double RotatedAt, double Now, float RotateMinutes)
{
	return RotatedAt < 0.0 || Now - RotatedAt >= FMath::Max(RotateMinutes, 1.f) * 60.0;
}

double BountyRules::RotationIn(double RotatedAt, double Now, float RotateMinutes)
{
	return RotatedAt < 0.0 ? 0.0 : FMath::Max(RotatedAt + FMath::Max(RotateMinutes, 1.f) * 60.0 - Now, 0.0);
}

FText BountyRules::DescribeWait(double Seconds)
{
	const int32 Minutes = FMath::CeilToInt32(FMath::Max(Seconds, 0.0) / 60.0);
	if (Seconds < 60.0)
	{
		return LOCTEXT("WaitSoon", "under a minute");
	}
	if (Minutes < 60)
	{
		return FText::Format(LOCTEXT("WaitMinutes", "{0} min"), FText::AsNumber(Minutes));
	}
	return FText::Format(LOCTEXT("WaitHours", "{0} h {1} min"), FText::AsNumber(Minutes / 60),
		FText::FromString(FString::Printf(TEXT("%02d"), Minutes % 60)));
}

// ---------------------------------------------------------------------------
// The pay
// ---------------------------------------------------------------------------

int32 BountyRules::PurseFor(EBountyTier Tier, int32 Level, const FBountyTuning& Tuning)
{
	return LooterEconomy::AtLevel(Tuning.PurseFor(Tier), FMath::Max(Level, 1));
}

int64 BountyRules::ExperienceFor(EBountyTier Tier, int32 ArenaLevel, int32 PlayerLevel, const FXPCurve& Curve,
	const FLevelRules& Rules, const FBountyTuning& Tuning)
{
	if (Curve.IsMaxLevel(PlayerLevel))
	{
		return 0;
	}
	const int32 Level = FMath::Max(ArenaLevel, 1);
	// The step at the arena's level (the side jobs' share where they're met), less the kills' falloff for a player past it.
	const double Step = static_cast<double>(Curve.XPToNextLevel(Level));
	const double Share = static_cast<double>(Tuning.XPShareFor(Tier)) * Rules.KillXPShare(Level, FMath::Max(PlayerLevel, 1));
	return FMath::Max<int64>(FMath::RoundToInt64(Step * Share), 0);
}

FChestLootOverride BountyRules::RewardLoot(EBountyTier Tier, const FBountyTuning& Tuning)
{
	FChestLootOverride Loot;
	Loot.bOverride = true;
	Loot.Guns = Tuning.GunsFor(Tier);
	Loot.GunChance = 1.f;
	Loot.Luck = Tuning.RewardLuck;
	Loot.AmmoPickups = Tuning.RewardAmmo;
	Loot.MoteChance = 0.f;
	return Loot;
}

// ---------------------------------------------------------------------------
// Names in code
// ---------------------------------------------------------------------------

FString BountyRules::TierName(EBountyTier Tier)
{
	switch (Tier)
	{
	case EBountyTier::Dangerous:
		return TEXT("Dangerous");
	case EBountyTier::Deadly:
		return TEXT("Deadly");
	default:
		return TEXT("Wanted");
	}
}

FString BountyRules::TwistName(EBountyTwist Twist)
{
	switch (Twist)
	{
	case EBountyTwist::Fleet:
		return TEXT("Fleet");
	case EBountyTwist::Warded:
		return TEXT("Warded");
	case EBountyTwist::Mending:
		return TEXT("Mending");
	case EBountyTwist::Rallying:
		return TEXT("Rallying");
	case EBountyTwist::Vengeful:
		return TEXT("Vengeful");
	case EBountyTwist::GraveBurst:
		return TEXT("GraveBurst");
	case EBountyTwist::BigPosse:
		return TEXT("BigPosse");
	default:
		return TEXT("SwiftPosse");
	}
}

FString BountyRules::StateName(EBountyState State)
{
	switch (State)
	{
	case EBountyState::Taken:
		return TEXT("Taken");
	case EBountyState::Won:
		return TEXT("Won");
	default:
		return TEXT("Posted");
	}
}

bool BountyRules::ParseTier(const FString& Text, EBountyTier& OutTier)
{
	for (const EBountyTier Tier : AllTiers)
	{
		if (Text.Equals(TierName(Tier), ESearchCase::IgnoreCase))
		{
			OutTier = Tier;
			return true;
		}
	}
	return false;
}

bool BountyRules::ParseTwist(const FString& Text, EBountyTwist& OutTwist)
{
	for (const EBountyTwist Twist : AllTwists)
	{
		if (Text.Equals(TwistName(Twist), ESearchCase::IgnoreCase) || Text.Equals(TwistWord(Twist).ToString(), ESearchCase::IgnoreCase))
		{
			OutTwist = Twist;
			return true;
		}
	}
	return false;
}

#undef LOCTEXT_NAMESPACE

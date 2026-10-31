#include "Player/PlayerFlaskRules.h"

float FFlaskRules::KillSoul(ECreatureRank Rank, bool bBoss, bool bBossFight)
{
	// Tougher kills fill it faster, so a hard fight pays for the drinks it costs; a boss puts the whole flask back.
	float Soul = 15.f;
	switch (Rank)
	{
	case ECreatureRank::Rare: Soul = 30.f; break;
	case ECreatureRank::Epic: Soul = 45.f; break;
	case ECreatureRank::Hallowed: Soul = 60.f; break;
	case ECreatureRank::Ethereal: Soul = 60.f; break;
	case ECreatureRank::Legendary: Soul = 100.f; break;
	case ECreatureRank::Boss: Soul = BossKillSoul; break;
	default: break;
	}
	if (bBoss)
	{
		Soul = BossKillSoul;
	}
	// A boss's helpers, Basic or not, keep the flask coming in the fight it's for (the user's ask: it works in boss fights).
	if (bBossFight)
	{
		Soul = FMath::Max(Soul, BossFightKillSoul);
	}
	return Soul;
}

float FFlaskRules::HealShareAt(float Seconds)
{
	// Eased in and out, so the health bar swells like a swallow rather than ticking up at a flat rate.
	return FMath::SmoothStep(SipAt, HealEndSeconds, Seconds);
}

int32 FFlaskRules::Pour(int32& InOutCharges, float& InOutSoul, float Added)
{
	InOutCharges = FMath::Clamp(InOutCharges, 0, MaxCharges);
	if (InOutCharges >= MaxCharges)
	{
		// Full: nothing is kept toward a charge it couldn't hold.
		InOutSoul = 0.f;
		return 0;
	}
	float Soul = FMath::Max(InOutSoul, 0.f) + FMath::Max(Added, 0.f);
	int32 Filled = 0;
	while (Soul >= SoulPerCharge && InOutCharges < MaxCharges)
	{
		Soul -= SoulPerCharge;
		++InOutCharges;
		++Filled;
	}
	InOutSoul = InOutCharges >= MaxCharges ? 0.f : Soul;
	return Filled;
}

EFlaskBlock FFlaskRules::WhyBlocked(const FFlaskGateInput& In)
{
	if (!In.bAlive)
	{
		return EFlaskBlock::Dead;
	}
	if (In.bInScene)
	{
		return EFlaskBlock::Scene;
	}
	if (In.bMenuOpen)
	{
		return EFlaskBlock::Menu;
	}
	if (In.bTraversing)
	{
		return EFlaskBlock::Moving;
	}
	if (In.bDrinking || In.bHandsBusy || In.Now - In.LastDrinkEnd < RepeatSeconds)
	{
		return EFlaskBlock::Busy;
	}
	// Told apart, empty first: a dry flask is the news worth hearing even at full health.
	if (In.Charges <= 0)
	{
		return EFlaskBlock::Empty;
	}
	if (In.bFullHealth)
	{
		return EFlaskBlock::FullHealth;
	}
	return EFlaskBlock::None;
}

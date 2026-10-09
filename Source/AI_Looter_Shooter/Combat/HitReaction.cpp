#include "Combat/HitReaction.h"

namespace
{
	/** How many FMeleeScopes are open: a strike's damage being dealt (never more than one, but nesting is harmless). */
	int32 OpenMeleeScopes = 0;
}

FHitReaction::FMeleeScope::FMeleeScope()
{
	++OpenMeleeScopes;
}

FHitReaction::FMeleeScope::~FMeleeScope()
{
	OpenMeleeScopes = FMath::Max(OpenMeleeScopes - 1, 0);
}

bool FHitReaction::FMeleeScope::IsOpen()
{
	return OpenMeleeScopes > 0;
}

float FHitReaction::StaggerSecondsFor(ECreatureRank Rank)
{
	switch (Rank)
	{
	case ECreatureRank::Basic: return 0.32f;
	case ECreatureRank::Rare: return 0.26f;
	case ECreatureRank::Epic: return 0.2f;
	default: return 0.f;
	}
}

bool FHitReaction::HitStopsFor(ECreatureRank Rank)
{
	return Rank != ECreatureRank::Boss;
}

FHitReaction::FResponse FHitReaction::OnHit(double Now, float Damage, float MaxHealth, bool bCritical, bool bKilled, ECreatureRank Rank, bool bMelee)
{
	FResponse Response;
	if (Damage <= 0.f)
	{
		return Response;
	}
	const bool bMeleeHit = bMelee || FMeleeScope::IsOpen();

	// Pellets landing together add up: one blast is one hit as far as a stagger goes.
	if (Now - BurstStart > BurstWindow)
	{
		BurstStart = Now;
		BurstDamage = 0.f;
	}
	BurstDamage += Damage;

	if (HitStopsFor(Rank))
	{
		if (bKilled)
		{
			// The kill always lands with weight, whatever came just before it.
			Response.HitStopSeconds = bCritical ? CritKillHitStop : KillHitStop;
		}
		else if (bCritical && Now >= HitStopReady)
		{
			Response.HitStopSeconds = CritHitStop;
			HitStopReady = Now + HitStopCooldown;
		}
		// A strike always lands with weight: it comes no more often than the strike's own cooldown.
		if (bMeleeHit)
		{
			Response.HitStopSeconds = FMath::Max(Response.HitStopSeconds, MeleeHitStop);
		}
		if (Response.HitStopSeconds > 0.f)
		{
			HitStopEnd = FMath::Max(HitStopEnd, Now + Response.HitStopSeconds);
		}
	}

	const float StaggerFor = StaggerSecondsFor(Rank);
	Response.BurstShare = BurstDamage / FMath::Max(MaxHealth, 1.f);
	const bool bHeavy = bCritical || Response.BurstShare >= HeavyShare;
	// A strike staggers whatever its share and skips the cooldown (it's the answer to a lunge); a gun's hit needs to be
	// heavy and wait the cooldown out. Neither starts one while one runs.
	const bool bCanStagger = bMeleeHit ? true : (bHeavy && Now >= StaggerReady);
	if (!bKilled && StaggerFor > 0.f && bCanStagger && !IsStaggered(Now))
	{
		Response.StaggerSeconds = bMeleeHit ? StaggerFor * MeleeStaggerScale : StaggerFor;
		StaggerEnd = Now + Response.StaggerSeconds;
		StaggerReady = StaggerEnd + StaggerCooldown;
	}
	return Response;
}

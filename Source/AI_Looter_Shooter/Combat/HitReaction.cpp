#include "Combat/HitReaction.h"

namespace
{
	/** How many FMeleeScopes are open: a strike's damage being dealt (never more than one, but nesting is harmless). */
	int32 OpenMeleeScopes = 0;

	/** The least a hit flinches (of today's flinch), whatever its share. */
	constexpr float FlinchFloor = 0.6f;
}

float FCreatureHit::FlinchStrength(const FGunfeelReactTuning& Tuning) const
{
	if (!bFresh)
	{
		return 1.f;
	}
	// A boss keeps today's flinch times BossScale whatever the hit: its health is so big that a share-sized flinch would
	// all but vanish, and a bigger one must never get in the way of its moves.
	if (bBoss)
	{
		return FMath::Max(Tuning.FlinchBossScale, 0.f);
	}
	// The killing hit's flinch would fight the death throe (a spider's rear): the death shows the kill.
	if (bKilled)
	{
		return 1.f;
	}
	const float Sized = FMath::Max(BurstShare, Share);
	float Strength = Tuning.FlinchScale * FMath::Clamp(FlinchFloor + Tuning.FlinchDamageGain * Sized, FlinchFloor,
		FMath::Max(Tuning.FlinchMax, FlinchFloor));
	if (bCritical)
	{
		Strength *= Tuning.FlinchCritScale;
	}
	return FMath::Max(Strength, 0.f);
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

float FHitReaction::StaggerSecondsFor(ECreatureRank Rank, float BasicSeconds)
{
	// Each rank in its old proportion of a Basic one's (0.32 s then: Restless 0.26, Gravebound 0.2, Hallowed 0.16,
	// Ethereal 0.12), so tuning the Basic's stagger tunes them all. A Soulfed monster or a boss never staggers.
	switch (Rank)
	{
	case ECreatureRank::Basic: return BasicSeconds;
	case ECreatureRank::Rare: return BasicSeconds * (0.26f / 0.32f);
	case ECreatureRank::Epic: return BasicSeconds * (0.2f / 0.32f);
	case ECreatureRank::Hallowed: return BasicSeconds * (0.16f / 0.32f);
	case ECreatureRank::Ethereal: return BasicSeconds * (0.12f / 0.32f);
	default: return 0.f;
	}
}

float FHitReaction::StaggerSecondsFor(ECreatureRank Rank)
{
	return StaggerSecondsFor(Rank, FGunfeelReactTuning().StaggerSeconds);
}

bool FHitReaction::HitStopsFor(ECreatureRank Rank)
{
	return Rank != ECreatureRank::Boss;
}

FHitReaction::FResponse FHitReaction::OnHit(double Now, float Damage, float MaxHealth, bool bCritical, bool bKilled, ECreatureRank Rank, bool bMelee)
{
	FHit Hit;
	Hit.Damage = Damage;
	Hit.MaxHealth = MaxHealth;
	Hit.bCritical = bCritical;
	Hit.bKilled = bKilled;
	Hit.Rank = Rank;
	Hit.bMelee = bMelee;
	return OnHit(Now, Hit);
}

void FHitReaction::BeginStagger(double Now, float Seconds, FResponse& Response, EJob Job)
{
	Response.StaggerSeconds = Seconds;
	Response.Job = Job;
	StaggerStart = Now;
	StaggerEnd = Now + Seconds;
	StaggerReady = StaggerEnd + Tuning.StaggerCooldown;
}

FHitReaction::FResponse FHitReaction::OnHit(double Now, const FHit& Hit)
{
	FResponse Response;
	if (Hit.Damage <= 0.f)
	{
		return Response;
	}
	const bool bMeleeHit = Hit.bMelee || FMeleeScope::IsOpen();

	// Pellets landing together add up: one blast is one hit as far as a stagger goes.
	if (Now - BurstStart > BurstWindow)
	{
		BurstStart = Now;
		BurstDamage = 0.f;
		bBurstPowered = false;
	}
	BurstDamage += Hit.Damage;
	Response.BurstShare = BurstDamage / FMath::Max(Hit.MaxHealth, 1.f);
	const bool bHeavy = Hit.bCritical || Response.BurstShare >= Tuning.StaggerHeavyShare;

	// The rifle's pressure: its hits on this one within the window, this one counted.
	bool bPressure = false;
	if (Hit.Kind == EWeaponKind::Rifle && !Hit.bKilled)
	{
		RifleHits.Add(Now);
		while (RifleHits.Num() > 0 && Now - RifleHits[0] > Tuning.RiflePressureWindow)
		{
			RifleHits.RemoveAt(0, 1, EAllowShrinking::No);
		}
		const int32 Needed = FMath::Max(Tuning.RiflePressureHits, 1);
		while (RifleHits.Num() > Needed)
		{
			RifleHits.RemoveAt(0, 1, EAllowShrinking::No);
		}
		bPressure = RifleHits.Num() >= Needed;
	}

	if (HitStopsFor(Hit.Rank))
	{
		if (Hit.bKilled)
		{
			// The kill always lands with weight, whatever came just before it.
			Response.HitStopSeconds = Hit.bCritical ? Tuning.HitStopCritKill : Tuning.HitStopKill;
		}
		else if (Hit.bCritical && Now >= HitStopReady)
		{
			// The revolver is the precision gun: its crit holds longest.
			Response.HitStopSeconds = Hit.Kind == EWeaponKind::Revolver ? Tuning.RevolverCritHitStop : Tuning.HitStopCrit;
			HitStopReady = Now + HitStopCooldown;
		}
		else if (bHeavy && Now >= HitStopReady && Tuning.HitStopHeavy > 0.f)
		{
			// A heavy body hit (a revolver round, a close blast) lands with weight too, a touch shorter than a crit.
			Response.HitStopSeconds = Tuning.HitStopHeavy;
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

	const float StaggerFor = StaggerSecondsFor(Hit.Rank, Tuning.StaggerSeconds);
	if (Hit.bKilled || StaggerFor <= 0.f)
	{
		return Response;
	}
	if (bMeleeHit)
	{
		// A strike staggers whatever its share and skips the cooldown (it's the answer to a lunge); not while one runs.
		if (!IsStaggered(Now))
		{
			BeginStagger(Now, StaggerFor * MeleeStaggerScale, Response, EJob::Melee);
		}
		return Response;
	}

	// The shotgun's power: once a blast, close and heavy enough. A stagger this same blast began gives way to it; one from
	// an earlier hit still running holds it off.
	const bool bStaggeredBefore = IsStaggered(Now) && StaggerStart < BurstStart;
	const bool bClose = Hit.Distance >= 0.f && Hit.Distance <= Tuning.ShotgunKnockRange;
	if (Hit.Kind == EWeaponKind::Shotgun && !bBurstPowered && bClose && Response.BurstShare >= Tuning.ShotgunKnockShare
		&& !bStaggeredBefore)
	{
		bBurstPowered = true;
		if (Hit.Size <= Tuning.ShotgunKnockSize)
		{
			// Thrown back: the knock holds its brain itself (UCreatureKnockComponent). Any stagger this blast began ends.
			Response.bKnockBack = true;
			Response.Job = EJob::Power;
			StaggerEnd = FMath::Min(StaggerEnd, Now);
			StaggerReady = FMath::Max(StaggerReady, Now + Tuning.StaggerCooldown);
		}
		else
		{
			BeginStagger(Now, StaggerFor, Response, EJob::Power);
		}
		return Response;
	}

	// A heavy hit, or the rifle's pressure: one stagger, then the cooldown.
	if ((bHeavy || bPressure) && Now >= StaggerReady && !IsStaggered(Now))
	{
		BeginStagger(Now, StaggerFor, Response, bHeavy ? EJob::Heavy : EJob::Pressure);
		RifleHits.Reset();
	}
	return Response;
}

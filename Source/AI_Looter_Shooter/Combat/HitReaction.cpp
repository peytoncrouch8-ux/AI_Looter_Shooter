#include "Combat/HitReaction.h"

namespace
{
	/** How many FMeleeScopes are open: a strike's damage being dealt (never more than one, but nesting is harmless). */
	int32 OpenMeleeScopes = 0;

	/** A shot within 60 degrees of its facing is from the front, past 120 from behind (cosines). */
	constexpr float FrontCosine = 0.5f;
	constexpr float BackCosine = -0.5f;

	/** 0 to 1 eased both ways (smoothstep). */
	float Smooth(float X)
	{
		const float T = FMath::Clamp(X, 0.f, 1.f);
		return T * T * (3.f - 2.f * T);
	}

	/** Gravebound and tougher: the ranks the stagger gap holds (no stun-lock on an elite: the fight's harder, not easier). */
	bool IsElite(ECreatureRank Rank)
	{
		return LooterRanks::IsAtLeast(Rank, ECreatureRank::Epic);
	}
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
	const float Floor = FMath::Max(Tuning.FlinchFloor, 0.f);
	const float Most = FMath::Max(Tuning.FlinchMax, Floor);
	const float Sized = FMath::Max(BurstShare, Share);
	// A far shooter sees a flinch smaller on screen: the far boost makes it up, but never past the cap (a sniper's 70% hit
	// at 30 m would otherwise throw a body through itself).
	const float Boosted = FMath::Min(FMath::Clamp(Floor + Tuning.FlinchDamageGain * Sized, Floor, Most) * FMath::Max(FarBoost, 0.f), Most);
	float Strength = Tuning.FlinchScale * Boosted;
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

float FHitReaction::PoiseScaleFor(ECreatureRank Rank)
{
	// A tougher one takes more to rock off its feet, so poise never makes an elite's fight easier (HITS' guard).
	switch (Rank)
	{
	case ECreatureRank::Basic: return 1.f;
	case ECreatureRank::Rare: return 1.15f;
	case ECreatureRank::Epic: return 1.35f;
	case ECreatureRank::Hallowed: return 1.6f;
	case ECreatureRank::Ethereal: return 1.9f;
	default: return 0.f;
	}
}

EHitWeight FHitReaction::WeightOf(const FHit& Hit, float BurstShare, const FGunfeelReactTuning& InTuning, const FHitFeelTuning& InFeel)
{
	const float Share = FMath::Max(Hit.Damage, 0.f) / FMath::Max(Hit.MaxHealth, 1.f);
	// Big: one hit that takes a quarter of it, the guns built for one big hit (a sniper's round, a rocket), or a shotgun
	// blast that earns its power (close and heavy enough).
	const bool bCloseBlast = Hit.Kind == EWeaponKind::Shotgun && Hit.Distance >= 0.f && Hit.Distance <= InTuning.ShotgunKnockRange
		&& BurstShare >= InTuning.ShotgunKnockShare;
	const bool bBig = Share >= InFeel.BigShare || Hit.Kind == EWeaponKind::Sniper || Hit.Kind == EWeaponKind::Launcher || bCloseBlast;
	if (Hit.bKilled)
	{
		// A kill that went well past dead, or a Big hit's, is the body giving way all at once.
		const bool bOverkill = InFeel.KillStyles && (Hit.Overkill >= InFeel.OverkillShare || bBig);
		return bOverkill ? EHitWeight::Overkill : EHitWeight::Kill;
	}
	if (bBig)
	{
		return EHitWeight::Big;
	}
	// A melee strike always lands heavy (the panic button against a lunge).
	const bool bMelee = Hit.bMelee || FMeleeScope::IsOpen();
	if (Hit.bCritical || bMelee || BurstShare >= InTuning.StaggerHeavyShare)
	{
		return EHitWeight::Heavy;
	}
	return EHitWeight::Body;
}

float FHitReaction::GetStaggerAlpha(double Now) const
{
	if (!bStaggerPose || !IsStaggered(Now))
	{
		return 0.f;
	}
	return FStaggerCurve::Alpha(static_cast<float>(Now - StaggerStart), static_cast<float>(StaggerEnd - StaggerStart));
}

float FHitReaction::GetPoise(double Now) const
{
	const float Since = static_cast<float>(FMath::Max(Now - PoiseAt, 0.0));
	return FMath::Max(Poise - Feel.PoiseDrain * Since, 0.f);
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
	// Whatever brought it on, the stagger spends its poise: it builds again from none once the next can come.
	Poise = 0.f;
	PoiseAt = Now;
	bStaggerPose = Feel.StaggerPose;
}

bool FHitReaction::OnJolt(double Now, float BasicSeconds, ECreatureRank Rank)
{
	const float Seconds = StaggerSecondsFor(Rank, BasicSeconds);
	if (Seconds <= 0.f || !CanStagger(Now, Rank))
	{
		return false;
	}
	FResponse Response;
	BeginStagger(Now, Seconds, Response, EJob::Power);
	return true;
}

bool FHitReaction::OnWardBreak(double Now, ECreatureRank Rank)
{
	// The reward for breaking it is always there: no cooldown or gap holds it back (its own cooldown follows as any's).
	const float Seconds = StaggerSecondsFor(Rank, Tuning.StaggerSeconds);
	if (Seconds <= 0.f)
	{
		return false;
	}
	FResponse Response;
	BeginStagger(Now, Seconds, Response, EJob::Power);
	return true;
}

bool FHitReaction::CanStagger(double Now, ECreatureRank Rank) const
{
	if (IsStaggered(Now) || Now < StaggerReady)
	{
		return false;
	}
	// An elite gets its fight back between staggers, however hard it's hit.
	return !IsElite(Rank) || Now - StaggerStart >= Feel.StaggerGap;
}

void FHitReaction::TakePoise(double Now, const FHit& Hit, float StaggerFor, FResponse& Response)
{
	Poise = GetPoise(Now);
	PoiseAt = Now;
	// Poise builds only while a stagger could come: hits during one, its cooldown or an elite's gap don't bank a stagger
	// for the moment it ends (steady fire would otherwise chain them).
	const float Break = Feel.PoiseBreak * PoiseScaleFor(Hit.Rank);
	if (Break <= 0.f || !CanStagger(Now, Hit.Rank))
	{
		return;
	}
	Poise += FMath::Max(Hit.Damage, 0.f) / FMath::Max(Hit.MaxHealth, 1.f);
	if (Poise >= Break)
	{
		BeginStagger(Now, StaggerFor, Response, EJob::Poise);
	}
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
	Response.Weight = WeightOf(Hit, Response.BurstShare, Tuning, Feel);

	// The rifle's pressure (the LMG's too: it holds the line the same way): its hits on this one within the window, this one counted.
	bool bPressure = false;
	if ((Hit.Kind == EWeaponKind::Rifle || Hit.Kind == EWeaponKind::LMG) && !Hit.bKilled)
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
			// The kill always lands with weight, whatever came just before it; an overkill holds longest (the anticipation
			// frame before the body gives way).
			Response.HitStopSeconds = Hit.bCritical ? Tuning.HitStopCritKill : Tuning.HitStopKill;
			if (Response.Weight == EHitWeight::Overkill)
			{
				Response.HitStopSeconds = FMath::Max(Response.HitStopSeconds, Feel.HitStopOverkill);
			}
		}
		else
		{
			// A crit (the precision guns', the revolver's and the sniper's, longest), a Big hit, or a heavy body hit (a
			// revolver round, a close blast), a touch shorter than a crit: one every cooldown, whichever is longest.
			float Wanted = 0.f;
			if (Hit.bCritical)
			{
				const bool bPrecise = Hit.Kind == EWeaponKind::Revolver || Hit.Kind == EWeaponKind::Sniper;
				Wanted = bPrecise ? Tuning.RevolverCritHitStop : Tuning.HitStopCrit;
			}
			else if (bHeavy)
			{
				Wanted = Tuning.HitStopHeavy;
			}
			if (Response.Weight == EHitWeight::Big)
			{
				Wanted = FMath::Max(Wanted, Feel.HitStopBig);
			}
			if (Wanted > 0.f && Now >= HitStopReady)
			{
				Response.HitStopSeconds = Wanted;
				HitStopReady = Now + HitStopCooldown;
			}
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
			// Thrown back: the knock holds its brain itself (UCreatureKnockComponent). Any stagger this blast began ends,
			// and the throw spends its poise.
			Response.bKnockBack = true;
			Response.Job = EJob::Power;
			StaggerEnd = FMath::Min(StaggerEnd, Now);
			StaggerReady = FMath::Max(StaggerReady, Now + Tuning.StaggerCooldown);
			Poise = 0.f;
			PoiseAt = Now;
		}
		else if (!IsElite(Hit.Rank) || Now - StaggerStart >= Feel.StaggerGap)
		{
			BeginStagger(Now, StaggerFor, Response, EJob::Power);
		}
		return Response;
	}

	// A heavy hit, or the rifle's pressure: one stagger, then the cooldown.
	if ((bHeavy || bPressure) && CanStagger(Now, Hit.Rank))
	{
		BeginStagger(Now, StaggerFor, Response, bHeavy ? EJob::Heavy : EJob::Pressure);
		RifleHits.Reset();
		return Response;
	}
	// Else damage piling up fast enough breaks its poise.
	TakePoise(Now, Hit, StaggerFor, Response);
	return Response;
}

float FStaggerCurve::Alpha(float Time, float Seconds)
{
	if (Seconds <= 0.f || Time <= 0.f || Time >= Seconds)
	{
		return 0.f;
	}
	// A short stagger (an Ethereal one's 0.15 s) still rises, holds and eases back, its rise shortened to fit.
	const float HoldEnd = HoldShare * Seconds;
	const float RiseEnd = FMath::Min(Rise, HoldEnd);
	if (Time < RiseEnd)
	{
		// Eased out: the jolt comes at once and settles into the pose.
		const float X = Time / RiseEnd;
		return 1.f - (1.f - X) * (1.f - X);
	}
	if (Time <= HoldEnd)
	{
		return 1.f;
	}
	return 1.f - Smooth((Time - HoldEnd) / (Seconds - HoldEnd));
}

EHitSide HitFeel::SideOf(const FVector& Facing, const FVector& Direction)
{
	const FVector Face = Facing.GetSafeNormal2D();
	const FVector ToShooter = (-Direction).GetSafeNormal2D();
	if (Face.IsNearlyZero() || ToShooter.IsNearlyZero())
	{
		return EHitSide::Front;
	}
	const float Cosine = static_cast<float>(FVector::DotProduct(Face, ToShooter));
	if (Cosine > FrontCosine)
	{
		return EHitSide::Front;
	}
	if (Cosine < BackCosine)
	{
		return EHitSide::Back;
	}
	// Its right as it faces (Up x Forward is the right in Unreal's frame).
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Face);
	return FVector::DotProduct(Right, ToShooter) >= 0.0 ? EHitSide::Right : EHitSide::Left;
}

float HitFeel::LateralOffsetOf(const FVector& Center, const FVector& Direction, const FVector& Location)
{
	const FVector Way = Direction.GetSafeNormal2D();
	if (Way.IsNearlyZero())
	{
		return 0.f;
	}
	// Across the shot, to the right of a body turned to face it: its distance that way is the shot's lever on it (the
	// shot's torque about the upright axis comes out + yaw for + offset whichever side it came from).
	const FVector Across = FVector::CrossProduct(FVector::UpVector, -Way);
	return static_cast<float>(FVector::DotProduct(Location - Center, Across));
}

float HitFeel::FarBoostAt(float Distance, const FHitFeelTuning& Feel)
{
	if (Distance < 0.f || Feel.FarBoost <= 1.f)
	{
		return 1.f;
	}
	const float Span = FMath::Max(Feel.FarFar - Feel.FarNear, 1.f);
	return FMath::Lerp(1.f, Feel.FarBoost, FMath::Clamp((Distance - Feel.FarNear) / Span, 0.f, 1.f));
}

float HitFeel::OverkillOf(float Damage, float HealthBefore, float MaxHealth)
{
	return FMath::Max(Damage - FMath::Max(HealthBefore, 0.f), 0.f) / FMath::Max(MaxHealth, 1.f);
}

const TCHAR* HitFeel::ZoneName(ECreatureHitZone Zone)
{
	switch (Zone)
	{
	case ECreatureHitZone::Head: return TEXT("head");
	case ECreatureHitZone::Abdomen: return TEXT("abdomen");
	case ECreatureHitZone::Leg: return TEXT("leg");
	case ECreatureHitZone::Arm: return TEXT("arm");
	case ECreatureHitZone::Core: return TEXT("core");
	case ECreatureHitZone::Shroud: return TEXT("shroud");
	case ECreatureHitZone::Weak: return TEXT("weak");
	default: return TEXT("body");
	}
}

const TCHAR* HitFeel::WeightName(EHitWeight Weight)
{
	switch (Weight)
	{
	case EHitWeight::Heavy: return TEXT("heavy");
	case EHitWeight::Big: return TEXT("big");
	case EHitWeight::Kill: return TEXT("kill");
	case EHitWeight::Overkill: return TEXT("overkill");
	default: return TEXT("body");
	}
}

const TCHAR* HitFeel::SideName(EHitSide Side)
{
	switch (Side)
	{
	case EHitSide::Left: return TEXT("left");
	case EHitSide::Right: return TEXT("right");
	case EHitSide::Back: return TEXT("back");
	default: return TEXT("front");
	}
}

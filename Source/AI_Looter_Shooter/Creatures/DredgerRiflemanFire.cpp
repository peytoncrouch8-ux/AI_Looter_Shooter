// ADredgerRifleman: his rounds (real bullets the player sees: UEnemyProjectileSubsystem's pellets, which hit by their flight
// and stop on the world, with a tracer streak drawn along each and a puff where it lands), his burst's tell and its pace,
// his reload (the magazine out and in, his call) and the butt when caught close.

#include "Creatures/DredgerRifleman.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Creatures/DredgerLook.h"
#include "Weapons/ImpactSurface.h"
#include "Weapons/VfxTypes.h"
#include "Weapons/WeaponFX.h"
#include "Weapons/WeaponModelComponent.h"
#include "World/WorldQueries.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** A round flies this far (cm) and is this big (cm: how near the player's capsule it must pass to hit, and its glow). */
	constexpr float RoundRange = 4500.f;
	constexpr float RoundRadius = 4.5f;
	/** A round on target is aimed this far off the chest at most (cm), and led this share of its flight. */
	constexpr float OnTargetJitter = 10.f;
	constexpr float LeadShare = 0.7f;
	/** The tracer: its tail's length (cm), width (cm) and glow; at most this many in flight from one rifleman. */
	constexpr float TracerTail = 260.f;
	constexpr float TracerWidth = 2.5f;
	constexpr float TracerGlow = 5.f;
	constexpr int32 MostTracers = 8;
	/** The muzzle's glint through the wind-up: from this size and glow to the sum (cm, strength), swelling. */
	constexpr float GlintSize = 4.f;
	constexpr float GlintGrow = 11.f;
	constexpr float GlintGlow = 2.f;
	constexpr float GlintGlowGrow = 12.f;
	/** The magazine's travel out and back in (cm), and the reload's shares where it goes and comes. */
	constexpr float MagTravel = 18.f;
	/** The butt reaches a little past where it starts (share), lands only in front, and shoves this hard. */
	constexpr float ButtReachShare = 1.2f;

	FWeaponFX* EffectsOf(UWorld* World)
	{
		UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
		if (!Bullets)
		{
			return nullptr;
		}
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		return &Effects;
	}

	/** The magazine's place in a reload Share (0-1) through: out, gone (the new one fetched), back in. */
	void MagazineAt(float Share, float& OutTravel, bool& bOutShown)
	{
		bOutShown = true;
		OutTravel = 0.f;
		if (Share >= 0.15f && Share < 0.35f)
		{
			OutTravel = MagTravel * (Share - 0.15f) / 0.2f;
		}
		else if (Share >= 0.35f && Share < 0.62f)
		{
			bOutShown = false;
			OutTravel = MagTravel;
		}
		else if (Share >= 0.62f && Share < 0.8f)
		{
			OutTravel = MagTravel * (1.f - (Share - 0.62f) / 0.18f);
		}
	}
}

// ---------------------------------------------------------------------------
// The burst
// ---------------------------------------------------------------------------

void ADredgerRifleman::TickBurst(float DeltaSeconds)
{
	if (GetCreatureState() != ECreatureState::Attack || !bShotAttack)
	{
		return;
	}
	const float Windup = FMath::Max(AttackWindup, 0.05f);
	if (GetStateTime() < Windup)
	{
		// The tell: a gold glint swelling at the muzzle through the wind-up, drawn each frame so it rides the rising rifle.
		if (FWeaponFX* Effects = EffectsOf(GetWorld()))
		{
			const float Share = FMath::Clamp(GetStateTime() / Windup, 0.f, 1.f);
			const float Swell = Share * Share;
			Effects->AddGlow(GetMuzzle(), GlintSize + GlintGrow * Swell, DredgerLook::Tracer, GlintGlow + GlintGlowGrow * Swell);
		}
		return;
	}
	// Staggered or knocked mid-burst: the rest of it is lost.
	if (IsHeldByHit())
	{
		ShotsLeft = 0;
		return;
	}
	BurstClock -= DeltaSeconds;
	const float Gap = FDredgerTuning::Live().BurstGap;
	while (ShotsLeft > 0 && BurstClock <= 0.f)
	{
		FireRound();
		BurstClock += Gap;
	}
}

void ADredgerRifleman::FireRound()
{
	APawn* Victim = GetTarget();
	UWorld* World = GetWorld();
	UEnemyProjectileSubsystem* Shots = World ? World->GetSubsystem<UEnemyProjectileSubsystem>() : nullptr;
	if (!Victim || !Shots || !IsValidTarget(Victim) || Rounds <= 0)
	{
		ShotsLeft = 0;
		return;
	}
	const FDredgerTuning Tuning = FDredgerTuning::Live();
	const FVector Muzzle = GetMuzzle();
	const FVector Chest = Victim->GetActorLocation() + FVector(0.f, 0.f, DredgerRules::ChestRise);
	const FVector Lead = DredgerRules::LeadPoint(Muzzle, Chest, Victim->GetVelocity(), Tuning.BulletSpeed, LeadShare);
	// How true: by the distance and how fast the target crosses his line; a round off target cracks past them instead.
	const FVector Line = (Chest - Muzzle).GetSafeNormal2D();
	const FVector Moving = Victim->GetVelocity();
	const float Crossing = static_cast<float>((Moving - Line * FVector::DotProduct(Moving, Line)).Size2D());
	const float Distance = static_cast<float>(FVector::Dist2D(Muzzle, Chest));
	const bool bOnTarget = Dice.FRand() < DredgerRules::HitChance(Distance, Crossing, Tuning);
	const FVector AimAt = bOnTarget ? Lead + Dice.GetUnitVector() * (OnTargetJitter * Dice.FRand())
		: DredgerRules::MissPoint(Lead, Muzzle, Dice.FRand(), Dice.FRand());

	FEnemyShot Shot;
	Shot.Start = Muzzle;
	Shot.Direction = (AimAt - Muzzle).GetSafeNormal();
	Shot.Speed = Tuning.BulletSpeed;
	Shot.Range = RoundRange;
	Shot.Radius = RoundRadius;
	// His level and rank are in AttackDamage; the fight's difficulty on top, as every creature's blow.
	Shot.Damage = AttackDamage * Tuning.BulletScale * GetFightDamageScale();
	Shot.Color = DredgerLook::Tracer;
	Shot.Shooter = this;
	Shot.Instigator = GetController();
	Shot.FlyBySound = ENearMissSound::Whiz;
	Shots->Fire(Shot);

	// Its tracer: drawn along the round's flight to where its line meets the world (a puff there), or its range.
	FTracer Tracer;
	Tracer.Start = Muzzle;
	Tracer.Way = Shot.Direction;
	Tracer.Speed = Shot.Speed;
	Tracer.End = RoundRange;
	FHitResult Hit;
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("DredgerRound"), this, false);
	if (World->LineTraceSingleByObjectType(Hit, Muzzle, Muzzle + Shot.Direction * RoundRange, FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		Tracer.End = Hit.Distance;
		Tracer.bHitsWorld = true;
		Tracer.Normal = Hit.ImpactNormal;
	}
	if (Tracers.Num() >= MostTracers)
	{
		Tracers.RemoveAt(0, 1, EAllowShrinking::No);
	}
	Tracers.Add(Tracer);

	// The shot as the world sees it: the flash and its smoke, the crack, the kick through his body and the rifle.
	if (FWeaponFX* Effects = EffectsOf(World))
	{
		FVfxMuzzle Flash;
		Flash.Location = Muzzle;
		Flash.Forward = Shot.Direction;
		Flash.Up = Held ? Held->GetUpVector() : FVector::UpVector;
		Flash.Kind = EWeaponKind::Rifle;
		Flash.Scale = 0.9f;
		Flash.InBurst = Tuning.BurstCount - ShotsLeft;
		Effects->SpawnMuzzleBurst(Flash);
		Effects->SpawnFlash(Muzzle, Shot.Direction, 0.8f);
	}
	LooterSound::PlayAt(this, LooterSoundCue::DredgerRifleFire, Muzzle);
	KickRecoil(1.f);
	--Rounds;
	--ShotsLeft;
	++ShotsFired;
}

void ADredgerRifleman::TickTracers(float DeltaSeconds)
{
	if (Tracers.IsEmpty())
	{
		return;
	}
	FWeaponFX* Effects = EffectsOf(GetWorld());
	for (int32 Index = Tracers.Num() - 1; Index >= 0; --Index)
	{
		FTracer& Tracer = Tracers[Index];
		Tracer.Age += DeltaSeconds;
		const float Flown = Tracer.Age * Tracer.Speed;
		if (Flown >= Tracer.End)
		{
			// Landed on the world: a puff and chips where it struck (the near miss's proof).
			if (Tracer.bHitsWorld && Effects)
			{
				Effects->SpawnSurfaceImpact(Tracer.Start + Tracer.Way * Tracer.End, Tracer.Normal, Tracer.Way, EImpactMaterial::Unknown, 0.7f, false);
			}
			Tracers.RemoveAt(Index, 1, EAllowShrinking::No);
			continue;
		}
		if (Effects)
		{
			const FVector Head = Tracer.Start + Tracer.Way * Flown;
			const FVector Tail = Tracer.Start + Tracer.Way * FMath::Max(Flown - TracerTail, 0.f);
			Effects->AddStreak(Tail, Head, TracerWidth, DredgerLook::Tracer, TracerGlow, FWeaponFX::EStreakLook::Tracer);
		}
	}
}

// ---------------------------------------------------------------------------
// The reload
// ---------------------------------------------------------------------------

void ADredgerRifleman::StartReload()
{
	if (ReloadLeft > 0.f || IsDead())
	{
		return;
	}
	ReloadLength = FDredgerTuning::Live().ReloadSeconds;
	ReloadLeft = ReloadLength;
	LooterSound::PlayAttached(LooterSoundCue::DredgerRifleReload, Held ? Held.Get() : GetRootComponent());
	// His call: the player's moment to push.
	Bark(LooterSoundCue::DredgerBark, 0.6f);
}

void ADredgerRifleman::TickReload(float DeltaSeconds)
{
	if (ReloadLeft <= 0.f)
	{
		Wants.ReloadWeight = 0.f;
		Wants.ReloadShare = 0.f;
		return;
	}
	// A hit that holds him holds his hands too.
	if (!IsHeldByHit())
	{
		ReloadLeft = FMath::Max(0.f, ReloadLeft - DeltaSeconds);
	}
	const float Share = 1.f - ReloadLeft / FMath::Max(ReloadLength, 0.01f);
	Wants.ReloadWeight = 1.f;
	Wants.ReloadShare = Share;
	if (bRifleBuilt)
	{
		float Travel = 0.f;
		bool bShown = true;
		MagazineAt(Share, Travel, bShown);
		Rifle->SetReloadTravel(Travel, bShown);
	}
	if (ReloadLeft <= 0.f)
	{
		Rounds = FDredgerTuning::Live().Magazine;
		Wants.ReloadWeight = 0.f;
		if (bRifleBuilt)
		{
			Rifle->SetReloadTravel(0.f, true);
		}
	}
}

// ---------------------------------------------------------------------------
// The butt
// ---------------------------------------------------------------------------

void ADredgerRifleman::ButtStrike()
{
	APawn* Victim = GetTarget();
	if (!IsValidTarget(Victim))
	{
		return;
	}
	const FVector ToVictim = Victim->GetActorLocation() - GetActorLocation();
	const bool bInReach = ToVictim.Size2D() <= FDredgerTuning::Live().ButtReach * GetSizeScale() * ButtReachShare
		&& FMath::Abs(static_cast<float>(ToVictim.Z)) < 200.f * GetSizeScale();
	const bool bInFront = FVector::DotProduct(GetActorForwardVector(), ToVictim.GetSafeNormal2D()) > 0.4f;
	if (!bInReach || !bInFront)
	{
		return;
	}
	// Nothing solid between (the world only): a butt through a wall never lands.
	const UWorld* World = GetWorld();
	FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("DredgerButt"), this, false);
	Params.AddIgnoredActor(Victim);
	if (World && World->LineTraceTestByObjectType(GetChestPoint(), Victim->GetActorLocation(), FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		return;
	}
	HitWithAttack(Victim, ToVictim.GetSafeNormal2D(), 1.f);
	LooterSound::PlayAt(this, LooterSoundCue::Melee::Hit, Victim->GetActorLocation(), 0.8f);
	KickRecoil(0.6f);
}

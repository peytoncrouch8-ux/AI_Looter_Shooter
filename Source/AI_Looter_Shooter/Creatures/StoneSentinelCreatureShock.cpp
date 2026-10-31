// AStoneSentinelCreature's ground-slam shockwave (the user's pick, 2026-10-31): the tell (it rears back with its arms spread
// wide, a deep grind rising, a ring of cracks spreading round its feet and the ground trembling under those near), then both
// fists driven into the ground: a boom, stone and dust bursting up round them, the view shaken, and a ring of rock spikes
// running out along the ground (AStoneSentinelShockwave) with gaps never where its target stood. Stuck with its fists in the
// ground after it: open (SetExposed), the window.

#include "Creatures/StoneSentinelCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundRules.h"
#include "Bosses/BossCameraShake.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/GravemotherFX.h"
#include "Creatures/GroundCrack.h"
#include "Creatures/StoneSentinelFX.h"
#include "Creatures/StoneSentinelShockwave.h"
#include "Weapons/WeaponFX.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** The tell's cracks (cm wide at size 1), and how fast they close once it's struck or called off (s). */
	constexpr float TellCrackWidth = 16.f;
	constexpr float TellCloseSeconds = 0.3f;
	/** The tell's tremor: a kick this often (s), at most this strong (0-1, growing through the rear), felt within this (cm at size 1). */
	constexpr float TremorEvery = 0.3f;
	constexpr float TremorShake = 0.07f;
	constexpr float TremorRadius = 1000.f;
	/** The blow: how long the tell's cracks stay open after it (s), its shake and how far it's felt. */
	constexpr float BlowCrackLingers = 1.6f;
	constexpr float BlowShake = 0.5f;
	constexpr float BlowShakeSeconds = 0.6f;
	constexpr float BlowShakeRadius = 1800.f;
	/** The dust ring off the blow, low and quick (cm at size 1). */
	constexpr float RingFrom = 60.f;
	constexpr float RingTo = 320.f;
	constexpr float RingWidth = 50.f;
	const FLinearColor DustRingColor(0.45f, 0.41f, 0.36f);
}

void AStoneSentinelCreature::BeginShock()
{
	AttackWindup = ShockTuning.Windup;
	AttackRecovery = ShockTuning.Recovery;
	AttackCooldown = ShockTuning.Breath;
	TremorClock = 0.f;
	const float Scale = GetSizeScale();
	LooterSound::PlayAttached(FName(StoneSentinelCue::ShockWindUp), GetRootComponent(), NAME_None, 1.f, LooterSoundRules::PitchForSize(Scale));
	// The tell on the ground: a ring of cracks spreading out from its feet over the wind-up, its seam glowing its tier's colour.
	EndShockTell();
	const FVector Feet = GroundUnder(GetFeet());
	if (AGroundCrack* Ring = AGroundCrack::Spawn(GetWorld(), Feet, Feet + GetActorForwardVector(), TellCrackWidth * Scale,
		UCreatureRankSettings::Get(GetRank()).Color, FMath::Rand()))
	{
		Ring->BurstSeconds = FMath::Max(ShockTuning.Windup, 0.2f);
		Ring->Burst(Feet, ShockTuning.TellRadius * Scale);
		ShockTellCrack = Ring;
	}
	StoneDust(Feet, 4, 80.f * Scale, 60.f * Scale, /*bFloor*/ true);
}

void AStoneSentinelCreature::TickShockTell(float DeltaSeconds)
{
	const float Time = GetStateTime();
	if (Time >= AttackWindup)
	{
		return;
	}
	TremorClock -= DeltaSeconds;
	if (TremorClock > 0.f)
	{
		return;
	}
	TremorClock = TremorEvery;
	// The ground trembling under those near as it rears, more as it gathers; dust shaken up off the cracking ground.
	const float Scale = GetSizeScale();
	const float Share = FMath::Clamp(Time / FMath::Max(AttackWindup, 0.05f), 0.f, 1.f);
	const FVector Feet = GetFeet();
	BossCameraShake::Kick(this, Feet, TremorShake * Share, TremorEvery, TremorRadius * Scale);
	const FVector Somewhere = Feet + FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f).Vector()
		* (FMath::FRandRange(0.4f, 1.f) * ShockTuning.TellRadius * Scale);
	StoneDust(GroundUnder(Somewhere), 1, 50.f * Scale, 30.f * Scale, /*bFloor*/ true);
}

void AStoneSentinelCreature::ShockStrike()
{
	// The tell's ring of cracks stays open a while as the blow's own (no second crack actor).
	if (AGroundCrack* Tell = ShockTellCrack.Get())
	{
		Tell->Close(BlowCrackLingers);
	}
	ShockTellCrack.Reset();
	ShockReadyIn = ShockTuning.Cooldown;
	UWorld* World = GetWorld();
	const float Scale = GetSizeScale();
	const FVector Center = GroundUnder(GetFeet());
	const FVector Fists = GroundUnder(GetFistsPoint());

	// The front: out from its feet every way, its gaps turned away from where its target stands now.
	FSentinelShockLaunch Launch;
	Launch.Center = Center;
	Launch.Size = Scale;
	Launch.Damage = AttackDamage * ShockTuning.Strength * GetFightDamageScale();
	Launch.Causer = this;
	Launch.PassesOver = Plinth.Get();
	Launch.Instigator = GetController();
	Launch.Tuning = ShockTuning;
	Launch.Seed = FMath::Rand();
	const APawn* Victim = GetTarget();
	const FVector Toward = Victim ? Victim->GetActorLocation() - Center : GetActorForwardVector();
	const float Bearing = FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(Toward.Y), static_cast<float>(Toward.X)));
	Launch.Gaps = StoneSentinelMoves::MakeGaps(Bearing, ShockTuning, Launch.Seed);
	// Its own wave sent again when it's idle (nothing new made); a new one kept the first time, a spare one if its own is still out.
	if (AStoneSentinelShockwave* Own = KeptWave.Get(); Own && Own->IsIdle())
	{
		Own->Relaunch(Launch);
	}
	else
	{
		AStoneSentinelShockwave* Sent = AStoneSentinelShockwave::Spawn(World, Launch, /*bInKeep*/ !Own);
		if (!Own)
		{
			KeptWave = Sent;
		}
	}

	// The blow itself: the ground bursting round its fists, stone and dust thrown up, a low ring of dust, the boom.
	ThrowChips(Fists + FVector(0.0, 0.0, 10.0), FVector::UpVector, 6, 460.f, 5.f, 14.f);
	StoneDust(Fists, 4, 110.f * Scale, 100.f * Scale, /*bFloor*/ true);
	if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
	{
		if (StoneSentinelFX::IsClearOfLens(this, Fists + FVector(0.0, 0.0, 100.0 * Scale), 110.f * Scale, 200.f * Scale))
		{
			Effects->Jet(Fists, 110.f * Scale, 200.f * Scale, 0.6f);
		}
		Effects->Chunks(EGraveChunk::StoneChip, Fists, FVector::UpVector, 4, 460.f, 0.7f);
		Effects->Chunks(EGraveChunk::Debris, Fists, FVector::UpVector, 2, 400.f, 0.8f);
	}
	if (World && World->IsGameWorld())
	{
		if (UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>())
		{
			FWeaponFX& Effects = Bullets->GetEffects();
			Effects.Initialize(World);
			if (StoneSentinelFX::IsClearOfLens(this, Fists, 130.f, 120.f))
			{
				Effects.SpawnDirt(Fists, FVector::UpVector, 0.7f);
			}
			Effects.SpawnShockRing(Center + FVector(0.0, 0.0, 10.0), FVector::UpVector, RingFrom * Scale, RingTo * Scale, RingWidth * Scale,
				DustRingColor, 0.6f, 0.3f);
		}
	}
	LooterSound::PlayAt(this, FName(StoneSentinelCue::ShockSlam), Fists, 1.f, LooterSoundRules::PitchForSize(Scale));
	BossCameraShake::Kick(this, Fists, BlowShake, BlowShakeSeconds, BlowShakeRadius * Scale);
	// Its fists stuck in the ground: open, the soul-light pouring out of its back.
	SetExposed(ShockTuning.ExposedSeconds);
	ShedGrit(4);
	UE_LOG(LogLooter, Verbose, TEXT("%s sends its shockwave (%d gaps)."), *GetName(), Launch.Gaps.Num());
}

void AStoneSentinelCreature::EndShockTell()
{
	if (AGroundCrack* Ring = ShockTellCrack.Get())
	{
		Ring->Close(TellCloseSeconds);
	}
	ShockTellCrack.Reset();
}

// ADeadMinerCreature's swing, in place of the Unpaid's shriek and lunge: it plants itself, heaves the pick overhead through
// its wind-up (the lamp flaring: the tell), brings it down on whoever stands in reach in front, and leaves it in the floor a
// moment (the window). A shot on the lamp in the wind-up breaks the swing: the blow lands on nothing and it reels.

#include "Creatures/DeadMinerCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossCameraShake.h"
#include "Combat/BulletSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** A blow lands on a target this far above or below its middle at most (cm, at size 1 and up), as the bite's does. */
	constexpr float SwingMaxRise = 220.f;

	/** The blow's jolt on the view of a player this near it (cm), and how hard (0-1). */
	constexpr float BlowShakeRadius = 900.f;
	constexpr float BlowShake = 0.18f;
}

bool ADeadMinerCreature::IsWindingUp() const
{
	return GetCreatureState() == ECreatureState::Attack && GetStateTime() < AttackWindup && !bSwingBroken && Act != EAct::Reel;
}

bool ADeadMinerCreature::CanStartAttack() const
{
	return Act != EAct::Reel && Super::CanStartAttack();
}

void ADeadMinerCreature::OnAttackStarted()
{
	// No shriek and no lunge (the Unpaid's): the wind-up is the pick heaved overhead (DeadMinerCreaturePose.cpp) and the
	// lamp flaring (WantedLampFlare), its own timing at its rank whatever a reel borrowed.
	bSwingBroken = false;
	AttackWindup = SwingWindup;
	AttackRecovery = SwingRecovery;
	if (Act != EAct::None)
	{
		SetAct(EAct::None);
	}
	LooterSound::PlayAttached(LooterSoundCue::DeadMiner::Heave, GetRootComponent(), NAME_None, 1.f, Voice->GetPitch());
}

void ADeadMinerCreature::Strike()
{
	// Broken by a shot on the lamp: the pick comes down on nothing.
	if (bSwingBroken)
	{
		OnAttackStrike(false);
		return;
	}
	SwingBlow(1.f);
}

void ADeadMinerCreature::SwingBlow(float Strength)
{
	const float Scale = GetSizeScale();
	APawn* Victim = GetTarget();
	bool bHit = false;
	if (IsValidTarget(Victim))
	{
		const FVector ToVictim = Victim->GetActorLocation() - GetActorLocation();
		if (DeadMinerRules::IsSwingHit(GetActorForwardVector(), ToVictim, Tuning.SwingReach * Scale, Tuning.SwingFront,
			SwingMaxRise * FMath::Max(Scale, 1.f)))
		{
			HitWithAttack(Victim, ToVictim.GetSafeNormal2D(), Strength);
			bHit = true;
		}
	}
	// The pick bites the floor in front of it: the whoosh down, iron on rock, sparks and grit thrown up.
	const FVector Bite = GroundUnder(GetActorLocation() + GetActorForwardVector() * (Tuning.SwingImpactAhead * Scale));
	LooterSound::PlayAttached(LooterSoundCue::DeadMiner::Swing, GetRootComponent(), NAME_None, 1.f, Voice->GetPitch());
	LooterSound::PlayAt(this, LooterSoundCue::DeadMiner::PickBite, Bite, 1.f, Voice->GetPitch());
	KickDirt(Bite, 0.45f * FMath::Clamp(Strength, 0.5f, 2.f));
	UWorld* World = GetWorld();
	if (UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr)
	{
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		Effects.SpawnImpact(Bite + FVector(0.0, 0.0, 3.0), FVector::UpVector, FVector::DownVector, EImpactSurface::World, false);
	}
	BossCameraShake::Kick(this, Bite, BlowShake * Strength, 0.35f, BlowShakeRadius);
	// A Gravebound miner's blow rings out through the rock as the Unpaid's shriek does, slowing whoever it reaches.
	Shriek();
	OnAttackStrike(bHit);
}

// ---------------------------------------------------------------------------
// The reel: its lamp shot in the wind-up
// ---------------------------------------------------------------------------

void ADeadMinerCreature::BeginReel()
{
	bSwingBroken = true;
	SetAct(EAct::Reel);
	if (GetCreatureState() == ECreatureState::Attack)
	{
		// The swing is lost: its strike falls on nothing at the next frame, and the attack holds it there, reeling.
		AttackWindup = FMath::Min(AttackWindup, GetStateTime() + 0.01f);
		AttackRecovery = Tuning.ReelSeconds;
	}
	LooterSound::PlayAttached(LooterSoundCue::DeadMiner::Reel, GetRootComponent(), NAME_None, 1.f, Voice->GetPitch());
	UE_LOG(LogLooter, Verbose, TEXT("%s's lamp is shot in its wind-up: the swing is lost."), *GetName());
}

void ADeadMinerCreature::TickReel(float DeltaSeconds)
{
	if (Act == EAct::Reel && ActTime >= Tuning.ReelSeconds)
	{
		SetAct(EAct::None);
	}
}

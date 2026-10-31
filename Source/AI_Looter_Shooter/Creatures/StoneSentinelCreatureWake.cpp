// AStoneSentinelCreature's sleep and waking: a statue on its plinth (held passive, no tag, no traits), woken by a player
// within WakeDistance or a shot; it cracks loose, stretches free of the years and grinds round to face them while its brain
// is held, drops into its stance with a thud, then fights. Calm again by its plinth, it climbs back on and is a statue once
// more, whole. Its death's crumble is timed here too.

#include "Creatures/StoneSentinelCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Audio/LooterSoundRules.h"
#include "Bosses/BossCameraShake.h"
#include "Combat/HealthComponent.h"
#include "Creatures/GravemotherFX.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** How often a statue looks for a player near it (s). */
	constexpr float LookInterval = 0.2f;
	/** While it grinds round, grit trickles off it about this often (a second). */
	constexpr float GritRate = 5.f;
	/** Climbing back on its plinth it rises this much over the straight line (cm, at size 1): a step up, not a glide. */
	constexpr float SettleLift = 30.f;
	/** The share of its waking at which it drops into its stance (StoneSentinelRules::WakePoseAt's drop ends with it). */
	constexpr float StanceAt = 0.96f;
}

void AStoneSentinelCreature::SetPhase(EStoneSentinelPhase NewPhase)
{
	Phase = NewPhase;
	PhaseTime = 0.f;
}

void AStoneSentinelCreature::BecomeStatue()
{
	SetPhase(EStoneSentinelPhase::Dormant);
	SetPassive(true);
	bShowsHealthTag = false;
	ShowTraits(false);
	AwakeWeight = 0.f;
	Walking = 0.f;
	CalmTime = 0.f;
	HeadYaw = 0.f;
	LookClock = FMath::FRandRange(0.f, LookInterval);
}

void AStoneSentinelCreature::Wake(APawn* By)
{
	if (Phase != EStoneSentinelPhase::Dormant || IsDead())
	{
		return;
	}
	SetPhase(EStoneSentinelPhase::Waking);
	SetPassive(false);
	bShowsHealthTag = true;
	// Its tier's traits come on as it wakes: a ward's cage forms round the stone as it cracks loose.
	ShowTraits(true);
	// Its brain waits out the waking (it stands, turning): no chase or slam until it's free of the stone. Its other moves
	// come ready a moment after it's loose (their first delays), so its first blow is a slam or a short wait.
	HoldBrain(Tuning.WakeSeconds);
	bStanceTaken = false;
	ShockReadyIn = FMath::Max(ShockReadyIn, ShockTuning.FirstDelay);
	BoulderReadyIn = FMath::Max(BoulderReadyIn, BoulderTuning.FirstDelay);
	if (By)
	{
		// As if it had been hurt by them: a hunt, its bestiary page open (a shot's hunt follows from the hit itself).
		AlertTo(By);
	}

	// The stone cracks loose: its cue, chips and dust bursting off it, the dust of the years shaken down round its feet.
	const float Scale = GetSizeScale();
	const FVector Middle = GetActorLocation();
	LooterSound::PlayAttached(FName(StoneSentinelCue::Wake), GetRootComponent(), NAME_None, 1.f, LooterSoundRules::PitchForSize(Scale));
	ThrowChips(Middle + FVector(0.0, 0.0, 50.0 * Scale), FVector::UpVector, Tuning.WakeChips, 260.f, 5.f, 14.f);
	StoneDust(Middle + FVector(0.0, 0.0, 60.0 * Scale), 3, 70.f * Scale, 40.f * Scale);
	StoneDust(GetFeet(), 3, 100.f * Scale, 70.f * Scale, /*bFloor*/ true);
	if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
	{
		Effects->Chunks(EGraveChunk::StoneChip, Middle, FVector::UpVector, 4, 300.f, 0.7f);
	}
	UE_LOG(LogLooter, Log, TEXT("%s wakes%s."), *GetName(), By ? *FString::Printf(TEXT(" for %s"), *By->GetName()) : TEXT(" (shot)"));
}

void AStoneSentinelCreature::TickDormant(float DeltaSeconds)
{
	LookClock -= DeltaSeconds;
	if (LookClock > 0.f || IsDead())
	{
		return;
	}
	LookClock = LookInterval;
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// A player it could hunt (alive, on its ground, out of every safe zone) within its waking distance. No sight needed: a
	// statue feels the footsteps on its grave.
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		if (!Pawn || !IsValidTarget(Pawn))
		{
			continue;
		}
		const FVector To = Pawn->GetActorLocation() - GetActorLocation();
		if (StoneSentinelRules::ShouldWake(static_cast<float>(To.Size2D()), static_cast<float>(To.Z), Tuning))
		{
			Wake(Pawn);
			return;
		}
	}
}

void AStoneSentinelCreature::TickWaking(float DeltaSeconds)
{
	AwakeWeight = StoneSentinelRules::WakeShare(PhaseTime, Tuning.WakeSeconds);
	// It grinds round to face them, slowly (its brain is held: nothing else turns it), stone dust trickling off it.
	if (const APawn* Prey = GetTarget())
	{
		const FVector To = Prey->GetActorLocation() - GetActorLocation();
		if (To.SizeSquared2D() > 1.0)
		{
			const float Wanted = static_cast<float>(To.Rotation().Yaw);
			const float Now = static_cast<float>(GetActorRotation().Yaw);
			const float Turned = FMath::FixedTurn(Now, Wanted, Tuning.WakeTurnRate * DeltaSeconds);
			SetActorRotation(FRotator(0.f, Turned, 0.f));
		}
	}
	if (FMath::FRand() < GritRate * DeltaSeconds)
	{
		const float Scale = GetSizeScale();
		StoneDust(GetActorLocation() + FVector(0.0, 0.0, FMath::FRandRange(-40.f, 80.f) * Scale), 1, 35.f * Scale, 25.f * Scale);
	}
	// The last of the waking: its weight its own again, it drops into its stance (on its Wake cue's settle).
	if (!bStanceTaken && PhaseTime >= Tuning.WakeSeconds * StanceAt)
	{
		SettleIntoStance();
	}
	if (PhaseTime >= Tuning.WakeSeconds)
	{
		AwakeWeight = 1.f;
		CalmTime = 0.f;
		SetPhase(EStoneSentinelPhase::Awake);
	}
}

void AStoneSentinelCreature::SettleIntoStance()
{
	bStanceTaken = true;
	const float Scale = GetSizeScale();
	const FVector Feet = GetFeet();
	// Tons settling: dust pushed out round its feet, pebbles shaken off its joints, a thud felt underfoot nearby.
	StoneDust(Feet, 3, 110.f * Scale, 90.f * Scale, /*bFloor*/ true);
	ShedGrit(4);
	if (Tuning.WakeSettleShake > 0.f)
	{
		BossCameraShake::Kick(this, Feet, Tuning.WakeSettleShake, 0.35f, Tuning.WakeSettleShakeRadius * Scale);
	}
	Jolt.X += 1.5f;
}

void AStoneSentinelCreature::TickAwake(float DeltaSeconds)
{
	AwakeWeight = 1.f;
	TickGait(DeltaSeconds);
	// Calm (after nobody, standing or walking home) by its plinth long enough: back up onto it, a statue again.
	const ECreatureState BrainState = GetCreatureState();
	const bool bCalm = !GetTarget() && (BrainState == ECreatureState::Idle || BrainState == ECreatureState::Return || BrainState == ECreatureState::Wander);
	CalmTime = bCalm ? CalmTime + DeltaSeconds : 0.f;
	if (CalmTime < Tuning.RestAfter || IsHeldByHit())
	{
		return;
	}
	const FTransform& HomeSpot = GetHome();
	if (FVector::Dist2D(GetActorLocation(), HomeSpot.GetLocation()) > Tuning.SettleReach * GetSizeScale())
	{
		return;
	}
	// It stops being anyone's foe at once (a shot now stops the climb: OnHurt), then climbs (TickSettling).
	SetPhase(EStoneSentinelPhase::Settling);
	SetPassive(true);
	bShowsHealthTag = false;
	ShowTraits(false);
	SettleFrom = GetActorLocation();
	SettleFromYaw = static_cast<float>(GetActorRotation().Yaw);
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->StopMovementImmediately();
	Movement->DisableMovement();
	LooterSound::PlayAt(this, FName(StoneSentinelCue::Step), GetFeet(), 0.8f, LooterSoundRules::PitchForSize(GetSizeScale()));
}

void AStoneSentinelCreature::TickSettling(float DeltaSeconds)
{
	const float T = FMath::Clamp(PhaseTime / FMath::Max(Tuning.SettleSeconds, 0.05f), 0.f, 1.f);
	const float Eased = FMath::SmoothStep(0.f, 1.f, T);
	const FTransform& HomeSpot = GetHome();
	// Up and back onto its plinth along a short arc, turning to the way it stood.
	const FVector Line = FMath::Lerp(SettleFrom, HomeSpot.GetLocation(), static_cast<double>(Eased));
	const FVector Arc = Line + FVector(0.0, 0.0, FMath::Sin(T * PI) * SettleLift * GetSizeScale());
	const FQuat From = FRotator(0.f, SettleFromYaw, 0.f).Quaternion();
	const FQuat To = FRotator(0.f, static_cast<float>(HomeSpot.Rotator().Yaw), 0.f).Quaternion();
	SetActorLocationAndRotation(Arc, FQuat::Slerp(From, To, Eased), false, nullptr, ETeleportType::TeleportPhysics);
	// Its body stiffens back into the statue's pose as it goes.
	AwakeWeight = 1.f - Eased;
	if (T < 1.f)
	{
		return;
	}
	// On: its feet down with a thud and a puff, whole again (the stone knits while it sleeps), a statue.
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	Health->ResetHealth();
	const float Scale = GetSizeScale();
	StoneDust(GetFeet(), 4, 70.f * Scale, 40.f * Scale, /*bFloor*/ true);
	LooterSound::PlayAt(this, FName(StoneSentinelCue::Step), GetFeet(), 1.f, LooterSoundRules::PitchForSize(Scale));
	UE_LOG(LogLooter, Log, TEXT("%s is a statue again."), *GetName());
	BecomeStatue();
}

void AStoneSentinelCreature::TickCrumbling(float DeltaSeconds)
{
	if (!bCrumbled && PhaseTime >= Tuning.CrumbleSeconds)
	{
		Crumble();
	}
}

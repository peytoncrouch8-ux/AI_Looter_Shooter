// AStoneSentinelCreature's choice of attack (the user, 2026-10-31: a threat at every distance, "some other attack than just a
// melee"): the slam up close, the shockwave near, the boulder out in the open (StoneSentinelMoves::Choose), each the creature's
// one attack with its own timing and turn; the moves' clocks; its lob's clear way; its open window's start. The moves
// themselves are StoneSentinelCreatureSlam.cpp's, ...Shock.cpp's and ...Boulder.cpp's.

#include "Creatures/StoneSentinelCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundRules.h"
#include "Creatures/SpitterGlob.h"
#include "Creatures/StoneSentinelBoulder.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** Its lob's way is looked at this often (s) while a throw is open, along this share of the flight, in this many pieces. */
	constexpr float LobCheckEvery = 0.25f;
	constexpr float LobCheckShare = 0.9f;
	constexpr int32 LobCheckSegments = 6;
	/** Where the rock leaves its hands, against its middle (cm at size 1): overhead, a little in front. */
	constexpr float ReleaseUp = 160.f;
	constexpr float ReleaseAhead = 30.f;
	/** Its shockwave's tell turns after its target through this share of the wind-up (the ring goes every way anyway). */
	constexpr float ShockAimShare = 0.5f;

	/** Main's PIE check: every sentinel makes this attack whenever it's in its reach (cooldowns aside). */
	TAutoConsoleVariable<int32> CVarSentinelMove(TEXT("Looter.Sentinel.Move"), 0,
		TEXT("Stone Sentinels' attack: 0 chosen by range and cooldowns, 1 always the slam, 2 the shockwave, 3 the boulder (when in its reach)."),
		ECVF_Default);

	/** Where a pawn's feet are. */
	FVector FeetOf(const APawn& Pawn)
	{
		const ACharacter* Character = Cast<ACharacter>(&Pawn);
		const float Half = Character ? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 88.f;
		return Pawn.GetActorLocation() - FVector(0.0, 0.0, Half);
	}
}

// ---------------------------------------------------------------------------
// Which attack
// ---------------------------------------------------------------------------

EStoneSentinelMove AStoneSentinelCreature::PlanMove() const
{
	const APawn* Victim = GetTarget();
	if (Phase != EStoneSentinelPhase::Awake || !Victim || IsDead() || !IsValidTarget(Victim))
	{
		return EStoneSentinelMove::None;
	}
	const FVector To = Victim->GetActorLocation() - GetActorLocation();
	FStoneSentinelMoveAsk Ask;
	Ask.Distance = static_cast<float>(To.Size2D());
	Ask.Rise = static_cast<float>(To.Z);
	Ask.Size = GetSizeScale();
	Ask.bShockReady = ShockReadyIn <= 0.f;
	Ask.bBoulderReady = BoulderReadyIn <= 0.f;
	// The arc is looked along only when a throw could be made at all (its traces cost).
	Ask.bLobClear = Ask.bBoulderReady && Ask.Distance >= BoulderTuning.MinStart && Ask.Distance <= BoulderTuning.MaxStart && IsLobClear(*Victim);
	Ask.Last = LastMove;
	Ask.Streak = MoveStreak;
	// A forced attack (Looter.Sentinel.Move) whenever it fits, whatever its cooldown.
	const int32 Forced = CVarSentinelMove.GetValueOnGameThread();
	if (Forced >= 1 && Forced <= 3)
	{
		const EStoneSentinelMove Want = static_cast<EStoneSentinelMove>(Forced);
		const bool bInReach = Ask.Distance <= StoneSentinelMoves::StartRange(Want, ShockTuning, BoulderTuning, Ask.Size);
		const bool bThrowable = Want != EStoneSentinelMove::Boulder || (Ask.Distance >= BoulderTuning.MinStart && IsLobClear(*Victim));
		return bInReach && bThrowable ? Want : EStoneSentinelMove::None;
	}
	return StoneSentinelMoves::Choose(Ask, ShockTuning, BoulderTuning);
}

float AStoneSentinelCreature::GetAttackStartRange() const
{
	// The attack it would make reaches from here; with none open, its slam's (its chase closes in for that).
	const EStoneSentinelMove Planned = PlanMove();
	return Planned == EStoneSentinelMove::None ? GetAttackRange()
		: StoneSentinelMoves::StartRange(Planned, ShockTuning, BoulderTuning, GetSizeScale());
}

EFightRole AStoneSentinelCreature::GetAttackTokenRole() const
{
	const EStoneSentinelMove Now = GetCreatureState() == ECreatureState::Attack ? Move : PlanMove();
	return StoneSentinelMoves::TokenRoleFor(Now);
}

bool AStoneSentinelCreature::CanStartAttack() const
{
	return Phase == EStoneSentinelPhase::Awake && Super::CanStartAttack() && PlanMove() != EStoneSentinelMove::None;
}

bool AStoneSentinelCreature::CanReachHeight(float Rise) const
{
	if (Super::CanReachHeight(Rise))
	{
		return true;
	}
	// A rock reaches a player up on a roof or a ledge, if they're far enough off for an arc.
	const APawn* Victim = GetTarget();
	return Victim && Rise <= BoulderTuning.MaxRise
		&& FVector::Dist2D(Victim->GetActorLocation(), GetActorLocation()) >= BoulderTuning.MinStart;
}

bool AStoneSentinelCreature::TracksTargetInWindup() const
{
	const float Time = GetStateTime();
	switch (Move)
	{
	case EStoneSentinelMove::Shockwave:
		return StoneSentinelRules::TracksAim(Time, AttackWindup, ShockAimShare);
	case EStoneSentinelMove::Boulder:
		// Squared up to its target as it winds back (the rock is led at release); not while it reels.
		return !bBoulderCountered && StoneSentinelRules::TracksAim(Time, AttackWindup, BoulderTuning.AimShare);
	default:
		// It turns after them for the first part of the heave, then its aim is set: a late sidestep dodges the blow.
		return StoneSentinelRules::TracksAim(Time, AttackWindup, Tuning.SlamAimShare);
	}
}

bool AStoneSentinelCreature::IsLobClear(const APawn& Victim) const
{
	if (LobCheckedAt >= 0.f && Clock - LobCheckedAt < LobCheckEvery)
	{
		return bLobClear;
	}
	LobCheckedAt = Clock;
	const UWorld* World = GetWorld();
	if (!World)
	{
		bLobClear = false;
		return false;
	}
	// The arc it would throw now, looked along in a few straight pieces up to where it comes down: a lob clears graves and
	// low walls, not a roof or a crypt's side.
	const float Scale = GetSizeScale();
	const FVector Start = GetActorLocation() + FVector(0.0, 0.0, ReleaseUp * Scale) + GetActorForwardVector() * (ReleaseAhead * Scale);
	const FVector LandAt = FeetOf(Victim);
	const float Seconds = StoneSentinelMoves::FlightSeconds(static_cast<float>(FVector::Dist2D(Start, LandAt)), BoulderTuning);
	const FVector Velocity = StoneSentinelMoves::LobVelocity(Start, LandAt, Seconds, BoulderTuning);
	FCollisionQueryParams Query = ASpitterGlob::MakeWorldQuery(World, this);
	if (const AStaticMeshActor* Stand = Plinth.Get())
	{
		Query.AddIgnoredActor(Stand);
	}
	FVector From = Start;
	for (int32 Piece = 1; Piece <= LobCheckSegments; ++Piece)
	{
		const FVector To = StoneSentinelMoves::LobPoint(Start, Velocity, Seconds * LobCheckShare * Piece / LobCheckSegments, BoulderTuning);
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, From, To, FCollisionObjectQueryParams(ECC_WorldStatic), Query))
		{
			bLobClear = false;
			return false;
		}
		From = To;
	}
	bLobClear = true;
	return true;
}

// ---------------------------------------------------------------------------
// The attack's frame: begun, struck, called off
// ---------------------------------------------------------------------------

void AStoneSentinelCreature::OnAttackStarted()
{
	// What it plans now is what it makes (the brain asked the same this frame); the slam if somehow nothing.
	const EStoneSentinelMove Planned = PlanMove();
	Move = Planned == EStoneSentinelMove::None ? EStoneSentinelMove::Slam : Planned;
	MoveStreak = Move == LastMove ? MoveStreak + 1 : 1;
	LastMove = Move;
	// It closes up as it moves again: the window was the one before.
	ExposedLeft = 0.f;
	bBoulderTorn = false;
	bBoulderThrown = false;
	bBoulderCountered = false;
	switch (Move)
	{
	case EStoneSentinelMove::Shockwave: BeginShock(); break;
	case EStoneSentinelMove::Boulder: BeginThrow(); break;
	default: BeginSlam(); break;
	}
}

void AStoneSentinelCreature::Strike()
{
	switch (Move)
	{
	case EStoneSentinelMove::Shockwave: ShockStrike(); break;
	case EStoneSentinelMove::Boulder: ReleaseBoulder(); break;
	default: SlamStrike(); break;
	}
}

void AStoneSentinelCreature::OnAttackCalledOff()
{
	// Hit off its turn in its wind-up and no turn to be had: its fists come down on nothing, a rock in its grip falls, the
	// tell's cracks close (the pose eases back to standing).
	DropBoulder();
	EndShockTell();
	UE_LOG(LogLooter, Verbose, TEXT("%s's %s is called off."), *GetName(), *UEnum::GetValueAsString(Move));
}

// ---------------------------------------------------------------------------
// Its clocks, and the beats inside a wind-up
// ---------------------------------------------------------------------------

void AStoneSentinelCreature::TickMoves(float DeltaSeconds)
{
	// Its moves come ready while it's after someone; a target lost (or a fresh one) leaves it their first delays, so it
	// sizes up whoever it turns on next.
	const bool bHasTarget = GetTarget() != nullptr && !IsDead() && Phase == EStoneSentinelPhase::Awake;
	if (bHasTarget)
	{
		ShockReadyIn = FMath::Max(0.f, ShockReadyIn - DeltaSeconds);
		BoulderReadyIn = FMath::Max(0.f, BoulderReadyIn - DeltaSeconds);
	}
	else if (bHadTarget)
	{
		ShockReadyIn = FMath::Max(ShockReadyIn, ShockTuning.FirstDelay);
		BoulderReadyIn = FMath::Max(BoulderReadyIn, BoulderTuning.FirstDelay);
	}
	bHadTarget = bHasTarget;

	if (GetCreatureState() == ECreatureState::Attack && !IsDead())
	{
		if (Move == EStoneSentinelMove::Boulder)
		{
			TickThrow(DeltaSeconds);
		}
		else if (Move == EStoneSentinelMove::Shockwave)
		{
			TickShockTell(DeltaSeconds);
		}
		return;
	}
	// An attack that ended any other way (a death, a stagger's end, its target gone) leaves nothing of itself behind.
	DropBoulder();
	EndShockTell();
}

void AStoneSentinelCreature::SetExposed(float Seconds)
{
	if (Seconds <= 0.f || IsDead())
	{
		return;
	}
	const bool bWasOpen = IsExposed();
	ExposedLeft = FMath::Max(ExposedLeft, Seconds);
	ApplyStoneDefense();
	if (!bWasOpen)
	{
		// "Now": a hollow crack through the block and the soul-light's hum, a burst of grit off its back.
		EmberClock = 0.f;
		LooterSound::PlayAttached(FName(StoneSentinelCue::Exposed), GetRootComponent(), NAME_None, 1.f, LooterSoundRules::PitchForSize(GetSizeScale()));
		ShedGrit(4, Rig.Spine2);
	}
}

AStoneSentinelBoulder* AStoneSentinelCreature::GetBoulder() const
{
	return HeldBoulder.Get();
}

// ACreatureBase: its turn in a fight (Phase 12.2, FIGHT-AI; the user's call, 2026-10-21: packs surround the player and take
// turns, ranged kinds use their range, Borderlands-normal difficulty). Its chase asks the fight director
// (UFightDirectorSubsystem) for a turn before it attacks: a melee one asks as soon as it's ready and near, and without one it
// waits on the ring round the player (its slot, facing them, feinting now and then) while those with a turn come in; a ranged
// one holds its spot in its band and spits only with a ranged turn. A hit that staggers or knocks it gives its turn back;
// its attack's end gives it back with a short rest. Bosses and Legendary monsters keep their own fights (no turns, their own
// speed and damage); a regular kind chases Looter.Fight.ChaseScale faster and hits Looter.Fight.DamageScale harder.

#include "Creatures/CreatureBase.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/CreatureHitReactionComponent.h"
#include "Creatures/CreatureKnockComponent.h"
#include "Creatures/CreaturePackComponent.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/FightRules.h"
#include "Creatures/FightTuning.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "ProfilingDebugging/CsvProfiler.h"

namespace
{
	/** Within this of its ring spot (cm, at size 1) it stands there; farther, it walks to it at this many times the gap a second (at most its chase). */
	constexpr float RingArrive = 30.f;
	constexpr float RingCatchUpRate = 2.5f;
	/** It walks round the ring toward a point this far ahead (cm, at size 1) along its way. */
	constexpr float RingLookAhead = 250.f;
	/** An attack called off leaves it at least this long (s) before its next. */
	constexpr float CalledOffCooldown = 0.4f;
}

// ---------------------------------------------------------------------------
// Who takes turns, and how hard they come
// ---------------------------------------------------------------------------

bool ACreatureBase::KeepsOwnFight() const
{
	return FightRules::KeepsOwnFight(CurrentRank, KeepsBossRules());
}

bool ACreatureBase::UsesFightDirector() const
{
	return !bPassive && !KeepsOwnFight();
}

bool ACreatureBase::IsHeldByHit() const
{
	return HitReaction->IsStaggered() || Knock->HoldsBrain();
}

float ACreatureBase::GetChaseSpeedNow() const
{
	// Its own boost on top (the Unpaid closing in, or its pack's frenzy: UFightDirectorSubsystem::SetChaseBoost); 1 for anyone else.
	const UFightDirectorSubsystem* Director = !KeepsOwnFight() ? UFightDirectorSubsystem::Get(this) : nullptr;
	const float Boost = Director ? Director->GetChaseBoost(*this) : 1.f;
	return FightRules::ChaseSpeedFor(ChaseSpeed, KeepsOwnFight(), FFightTuning::LiveChaseScale()) * Boost;
}

float ACreatureBase::GetFightDamageScale() const
{
	return FightRules::DamageScaleFor(KeepsOwnFight(), FFightTuning::LiveDamageScale());
}

// ---------------------------------------------------------------------------
// Its chase: its turn, or its wait
// ---------------------------------------------------------------------------

void ACreatureBase::TickChaseFight(const APawn& Victim, float DeltaSeconds)
{
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(CreatureFight);
	const FVector Here = GetActorLocation();
	const FVector Prey = Victim.GetActorLocation();
	const float Distance = static_cast<float>(FVector::Dist2D(Here, Prey));
	// Its pack: its flank among those chasing the same player, or breaking off once its pack is gone (it then runs and
	// starts no attack: CreaturePackComponent).
	Pack->TickChase(DeltaSeconds);
	const bool bBreakingOff = Pack->IsRetreating();
	const bool bBossRules = KeepsBossRules();
	const bool bOwnFight = FightRules::KeepsOwnFight(CurrentRank, bBossRules);
	UFightDirectorSubsystem* Director = !bOwnFight && !bPassive ? UFightDirectorSubsystem::Get(this) : nullptr;
	// Its own boost rides on the fight's chase scale (the Unpaid closing in and its pack's frenzy: SetChaseBoost).
	const float Chase = FightRules::ChaseSpeedFor(ChaseSpeed, bOwnFight, FFightTuning::LiveChaseScale())
		* (Director ? Director->GetChaseBoost(*this) : 1.f);
	// An attack of its own may start from farther out than its reach (a charge); otherwise it closes in to bite. Never at a
	// player up on a rock over it, which its strike can't reach: it stands there instead, and soon finds them out of its
	// reach (CreatureBasePursuit.cpp). A boss's attacks keep their own rules.
	const bool bReady = !bBreakingOff && CooldownRemaining <= 0.f
		&& (CanReachHeight(static_cast<float>(Prey.Z - Here.Z)) || bBossRules);
	const float StartRange = GetAttackStartRange();
	const bool bCanStart = bReady && Distance <= StartRange && CanStartAttack();

	if (!Director)
	{
		// Its own fight (a boss, a Legendary monster), or no director: it attacks whenever it can, as creatures always did.
		bWaitingTurn = false;
		SetWaitFacing(false);
		if (bCanStart)
		{
			SetState(ECreatureState::Attack);
		}
		else if (bBreakingOff || Distance > GetChaseHold())
		{
			MoveToward(Pack->GetChaseGoal(Victim), bBreakingOff ? Chase * Pack->GetRetreatSpeedShare() : Chase, DeltaSeconds);
		}
		else
		{
			// Close but still recovering: square up for the next bite.
			FaceToward(Prey, DeltaSeconds);
		}
		return;
	}

	if (bBreakingOff)
	{
		// Running for a moment: no turn, no place on the ring.
		bWaitingTurn = false;
		SetWaitFacing(false);
		Director->ReleaseToken(*this);
		MoveToward(Pack->GetChaseGoal(Victim), Chase * Pack->GetRetreatSpeedShare(), DeltaSeconds);
		return;
	}

	// Its turn. A melee one asks as soon as it's ready and near the ring, and its turn brings it in from there; a ranged or
	// support one asks when it could attack this moment. The longest waiter gets the next free turn.
	const EFightRole Place = GetFightRole();
	const float Approach = Director->GetApproachDistance(*this);
	const bool bAsks = Place == EFightRole::Melee ? bReady && Distance <= FMath::Max(StartRange, Approach) : bCanStart;
	const bool bTurn = Director->HoldsToken(*this) || (bAsks && Director->TryTakeToken(*this, Victim, GetAttackTokenRole()));
	FVector Spot;
	const bool bHasSpot = !bTurn && Director->GetWaitSpot(*this, Victim, Spot);
	if (bTurn && bCanStart)
	{
		bWaitingTurn = false;
		SetWaitFacing(false);
		SetState(ECreatureState::Attack);
		return;
	}

	if (bHasSpot)
	{
		if (Place == EFightRole::Ranged)
		{
			// Out past its band it comes in toward its spot; inside it, its kind keeps its distance and walks to its spot
			// (the Spitter's keep-away), facing its target.
			bWaitingTurn = false;
			SetWaitFacing(false);
			if (Distance > GetChaseHold())
			{
				MoveToward(Spot, Chase, DeltaSeconds);
			}
			else
			{
				FaceToward(Prey, DeltaSeconds);
			}
			return;
		}
		if (Distance > Approach)
		{
			// Still on its way in: the pack's flank fan brings it to the ring.
			bWaitingTurn = false;
			SetWaitFacing(false);
			MoveToward(Pack->GetChaseGoal(Victim), Chase, DeltaSeconds);
			return;
		}
		// On the ring: round to its slot (never across the player), facing them, and standing there until its turn.
		bWaitingTurn = true;
		SetWaitFacing(true);
		const float Gap = static_cast<float>(FVector::Dist2D(Here, Spot));
		if (Gap > RingArrive * SizeScale)
		{
			const FVector Way = FightRules::OrbitWay(Here, Prey, Spot);
			const FVector Goal = Here + Way * FMath::Min(Gap, RingLookAhead * SizeScale);
			MoveToward(Goal, FMath::Min(Chase, Gap * RingCatchUpRate), DeltaSeconds);
		}
		FaceToward(Prey, DeltaSeconds);
		return;
	}

	// Its turn: straight in, the shortest way. Nowhere to wait (a pair, a support kind, no slot open): it comes as before.
	bWaitingTurn = false;
	SetWaitFacing(false);
	if (Distance > GetChaseHold())
	{
		MoveToward(bTurn ? Prey : Pack->GetChaseGoal(Victim), Chase, DeltaSeconds);
	}
	else
	{
		FaceToward(Prey, DeltaSeconds);
	}
}

void ACreatureBase::SetWaitFacing(bool bFacing)
{
	if (bWaitFacing == bFacing)
	{
		return;
	}
	bWaitFacing = bFacing;
	// Round the ring it sidles with its eyes on its prey (its brain turns it), rather than turning to where it walks.
	GetCharacterMovement()->bOrientRotationToMovement = !bFacing;
}

// ---------------------------------------------------------------------------
// Its turn's end: played out, left, or hit off it
// ---------------------------------------------------------------------------

void ACreatureBase::EndFightTurn()
{
	if (UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(this))
	{
		Director->EndTurn(*this);
	}
}

void ACreatureBase::LeaveFightTurn()
{
	UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(this);
	if (Director && Director->HoldsToken(*this))
	{
		Director->ReleaseToken(*this);
	}
}

void ACreatureBase::LoseTurnToHit()
{
	UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(this);
	if (!Director || !Director->HoldsToken(*this))
	{
		return;
	}
	Director->GiveBackForHit(*this);
	bWaitingTurn = false;
	// Hit in its wind-up (held, not landed): when it comes to, its attack goes on only if it gets its turn back.
	if (State == ECreatureState::Attack && !bStruck)
	{
		bAttackNeedsTurn = true;
	}
}

void ACreatureBase::ResumeAfterHit()
{
	if (!bAttackNeedsTurn)
	{
		return;
	}
	bAttackNeedsTurn = false;
	if (State != ECreatureState::Attack || bStruck)
	{
		return;
	}
	// First in line after a short stagger, it usually has it back at once; after a long knock the next one went in.
	UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(this);
	const APawn* Victim = Target.Get();
	if (!Director || (Victim && Director->TryTakeToken(*this, *Victim, GetAttackTokenRole())))
	{
		return;
	}
	UE_LOG(LogLooter, Verbose, TEXT("%s's attack is called off: hit off its turn, and none to be had."), *GetName());
	OnAttackCalledOff();
	CooldownRemaining = FMath::Max(CooldownRemaining, CalledOffCooldown);
	ResumeHunt();
}

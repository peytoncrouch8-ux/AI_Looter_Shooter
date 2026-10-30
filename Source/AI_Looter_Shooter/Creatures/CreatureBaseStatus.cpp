// ACreatureBase's brain under the element statuses (the "Elements matter" job, 2026-10-29): its status pose's component
// woken as a status takes it (UCreatureStatusReactComponent eases the pose each kind lays on its body), and the burn's
// panic: a Basic or Restless creature a burn takes may bolt from the player for 1.5 s (Looter.Element.BurnPanic, then a
// cooldown), never a boss, a Soulfed or a spider (the user's rule: the spiders' fight stays untouched, they get the pose
// only), nor one that fire hardly hurts (the Unpaid's coal, stone, iron, spirits).
//
// TickBrain must call this before its BrainHold check (after ResumeAfterHit is right): while the run lasts, it moves the
// creature itself and sets BrainHold for the frame (as an entrance does), so that check right after it skips the state's
// own moves, perception and clock this frame. Called after that check, the brain would chase and run in the same frame
// and hold every other one. A stagger or a knock still holds it (the run's clock goes on).

#include "Creatures/CreatureBase.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Creatures/CreaturePackComponent.h"
#include "Creatures/CreatureStatusPose.h"
#include "Creatures/CreatureStatusReactComponent.h"
#include "Creatures/PackRules.h"
#include "Creatures/SpiderCreature.h"
#include "Elements/ElementRules.h"
#include "Elements/ElementStatusComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** The run's cry, a little under a hurt cry's. */
	constexpr float PanicCryVolume = 0.9f;
	/** It runs this far past where its speed takes it in the run, so it never arrives and stands before the run's end. */
	constexpr float PanicReachShare = 1.3f;
}

void ACreatureBase::UpdateStatusBrain(float DeltaSeconds)
{
	const UElementStatusComponent* Statuses = UElementStatusComponent::Find(this);
	UCreatureStatusReactComponent* Reacts = UCreatureStatusReactComponent::Find(this);
	const bool bStatuses = Statuses && Statuses->GetStatus().IsActive();
	if (!Reacts)
	{
		if (!bStatuses)
		{
			return;
		}
		Reacts = UCreatureStatusReactComponent::FindOrAdd(*this);
	}
	if (bStatuses)
	{
		Reacts->Wake();
	}

	UCreatureStatusReactComponent::FBurnPanic& Panic = Reacts->GetPanic();
	Panic.CooldownLeft = FMath::Max(Panic.CooldownLeft - DeltaSeconds, 0.f);
	const int32 Burns = Statuses ? Statuses->GetStatus().NumBurns() : 0;
	const bool bNewBurn = Burns > Panic.LastBurns;
	Panic.LastBurns = Burns;
	const APawn* Prey = Target.Get();
	const bool bHunting = Prey && (State == ECreatureState::Chase || State == ECreatureState::Search);

	if (Panic.Left > 0.f)
	{
		Panic.Left -= DeltaSeconds;
		// Its prey gone, dead, held back, or the run over: it stops, and its state's moves take it on from where it stands.
		if (IsDead() || !bHunting || bPassive || Panic.Left <= 0.f)
		{
			Panic.Left = 0.f;
			StuckTime = 0.f;
			SteerTimer = 0.f;
			return;
		}
		// Staggered or knocked: the hit holds it (TickBrain's own hold), its run's clock going on.
		if (IsHeldByHit())
		{
			return;
		}
		MoveToward(Panic.To, GetChaseSpeedNow() * CreatureStatusPose::PanicSpeedShare, DeltaSeconds);
		// Its state's own moves and clock wait this frame (TickBrain's hold, as for an entrance).
		BrainHold = FMath::Max(BrainHold, UE_KINDA_SMALL_NUMBER);
		return;
	}
	if (!bNewBurn || !Prey)
	{
		return;
	}

	CreatureStatusPose::FPanicAsk Ask;
	Ask.Rank = CurrentRank;
	Ask.bBoss = IsBossCreature();
	Ask.bOwnFight = KeepsOwnFight();
	Ask.bSpider = IsA<ASpiderCreature>();
	Ask.bFireShrugged = CreatureStatusPose::ShrugsFire(
		ElementRules::Affinity(ELooterElement::Ember, ElementRules::TargetOf(this), /*bCritical*/ false));
	Ask.bAlive = !IsDead();
	Ask.bHunting = bHunting;
	Ask.bAttacking = State == ECreatureState::Attack;
	Ask.bPassive = bPassive || BrainHold > 0.f;
	Ask.bRunning = Pack->IsRetreating();
	Ask.CooldownLeft = Panic.CooldownLeft;
	if (!CreatureStatusPose::Panics(Ask, Reacts->GetRolls().FRand(), CreatureStatusPose::LiveBurnPanicChance()))
	{
		return;
	}

	// It bolts straight away from them (bent toward home where that would leave its ground, as a pack's break-off runs),
	// its turn and place in the fight given up, turning to where it runs.
	const FVector Here = GetActorLocation();
	const float Reach = GetChaseSpeedNow() * CreatureStatusPose::PanicSpeedShare * CreatureStatusPose::PanicSeconds * PanicReachShare;
	Panic.To = PackRules::RetreatGoal(Here, Prey->GetActorLocation(), Home.GetLocation(), Reach,
		[this](const FVector& Point) { return IsOnItsGround(Point); });
	Panic.Left = CreatureStatusPose::PanicSeconds;
	Panic.CooldownLeft = CreatureStatusPose::LiveBurnPanicCooldown();
	LeaveFightTurn();
	bWaitingTurn = false;
	SetWaitFacing(false);
	StuckTime = 0.f;
	SteerTimer = 0.f;
	Voice->Play(UCreatureVoiceComponent::CriesFor(*this).Hurt, PanicCryVolume);
	UE_LOG(LogLooter, Verbose, TEXT("%s: the burn sends it running for %.1f s."), *GetName(), Panic.Left);
}

// AUnpaidCreature's varied fight (2026-10-29; the user: "in play the Unpaid just lunge on repeat"). Each turn picks its move
// by range and situation (UnpaidMoveRules: weighted, each move its own cooldown, never a third time in a row, never a move a
// pack-mate is committing at the same player): the lunge; the rake up close (a close move planned from the ring closes in
// first); a feint that flies part way, stops short and slides aside; a phase-strike that fades round to the player's side or
// back and claws after a clear tell; a low swipe skimming in at the feet that a jump clears; or it hangs back, circling wide,
// until a pack-mate commits. One pressures while another flanks: a phase-strike weighs more while a pack-mate comes from the
// front. A lunge that misses may turn into a shriek (UnpaidCreatureShriek.cpp). Each move's tell is its own: the lunge's
// shriek pose and coal flare (the feint shares it: that's the feint), the low swipe's sinking crouch and dragging claws, the
// phase-strike's raised claw and whisper once it's back. Numbers: FUnpaidMoveTuning (Looter.Unpaid.*); Looter.Unpaid.Variety 0
// is the kit as it was. Plain Unpaid with their kit only (FUnpaidMoveWork::IsOn).
// This file: the turn's pick, its plan and closing in, the timings, the hang-back and the pack's board; the moves in their
// frames are UnpaidCreatureVarietyMoves.cpp.

#include "Creatures/UnpaidKitWork.h"
#include "Creatures/UnpaidCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/FightRules.h"
#include "Creatures/PackRules.h"
#include "Creatures/UnpaidBodyParts.h"
#include "Creatures/UnpaidFightRules.h"
#include "Creatures/UnpaidFightTuning.h"
#include "Creatures/UnpaidLifeWork.h"
#include "Creatures/UnpaidMoveRules.h"
#include "Creatures/UnpaidMoveTuning.h"
#include "Creatures/UnpaidPackSubsystem.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	namespace Cue = LooterSoundCue::UnpaidScream;

	/** A turn's plan begun this far (cm, at size 1) inside a move's start still counts as in reach: the frame it was made. */
	constexpr float StartSlack = 30.f;

	/** Hanging back, it asks the director to keep it out of the turns' line this long at a time (s), renewed under RestRenew. */
	constexpr float HangRest = 0.6f;
	constexpr float HangRenew = 0.3f;
	/** Circling, it heads for a point this far round the circle ahead of it (radians, about 29 degrees). */
	constexpr float HangLead = 0.5f;

	/** A closing-in that gets nowhere lets its turn go for this long (s). */
	constexpr float GiveUpRest = 0.6f;

	/** The tally entry for a move. */
	EUnpaidTally TallyOf(EUnpaidMove Move)
	{
		switch (Move)
		{
		case EUnpaidMove::Rake: return EUnpaidTally::Rake;
		case EUnpaidMove::Feint: return EUnpaidTally::Feint;
		case EUnpaidMove::PhaseStrike: return EUnpaidTally::PhaseStrike;
		case EUnpaidMove::LowSwipe: return EUnpaidTally::LowSwipe;
		case EUnpaidMove::HangBack: return EUnpaidTally::HangBack;
		default: return EUnpaidTally::Lunge;
		}
	}

	float CooldownOf(EUnpaidMove Move, const FUnpaidMoveTuning& Tuning)
	{
		switch (Move)
		{
		case EUnpaidMove::Rake: return Tuning.RakeCooldown;
		case EUnpaidMove::Feint: return Tuning.FeintCooldown;
		case EUnpaidMove::PhaseStrike: return Tuning.PhaseStrikeCooldown;
		case EUnpaidMove::LowSwipe: return Tuning.LowSwipeCooldown;
		case EUnpaidMove::HangBack: return Tuning.HangBackCooldown;
		default: return 0.f;
		}
	}
}

bool AUnpaidCreature::StartsRingMove(const APawn& Victim, float Distance)
{
	return FUnpaidMoveWork::PlanTurn(*this, Victim, Distance);
}

bool FUnpaidMoveWork::IsOn(const AUnpaidCreature& Unpaid)
{
	// The fight director too: the varied fight is a pack's (its turns, its board, the blink gate). Without one (Looter.Fight.Director
	// 0, a test level that made none) the kit is as it was.
	return FUnpaidMoveTuning::IsVarietyOn() && Unpaid.Kit.BossAdd != 1 && Unpaid.HasFightKit() && UFightDirectorSubsystem::Get(&Unpaid) != nullptr;
}

// ---------------------------------------------------------------------------
// Its turn: the pick
// ---------------------------------------------------------------------------

namespace
{
	/** What its turn knows as it picks: its range, its history and cooldowns, its pack's commits, the player's read. */
	FUnpaidMoveContext ContextFor(AUnpaidCreature& Unpaid, const APawn& Victim, bool bNow, const FUnpaidFightTuning& Kit,
		const FUnpaidMoveState& State)
	{
		FUnpaidMoveContext Context;
		const float Scale = FMath::Max(Unpaid.GetSizeScale(), 0.1f);
		Context.Distance = static_cast<float>(FVector::Dist2D(Unpaid.GetActorLocation(), Victim.GetActorLocation())) / Scale;
		Context.LungeStart = Kit.LungeStart;
		Context.RakeRange = Kit.RakeRange;
		Context.Last = State.Last;
		Context.BeforeLast = State.BeforeLast;
		for (int32 Slot = 0; Slot < NumUnpaidMoves; ++Slot)
		{
			Context.Cooldowns[Slot] = State.Cooldowns[Slot];
		}
		if (const UUnpaidPackSubsystem* Packs = UUnpaidPackSubsystem::Get(&Unpaid))
		{
			const FUnpaidMoveBoard& Board = Packs->BoardFor(Victim);
			const uint32 Id = Unpaid.GetUniqueID();
			Context.Taken = Board.TakenBy(Id);
			Context.bMateFront = Board.AnyFront(Id);
			Context.bMateSide = Board.AnySide(Id);
		}
		if (const UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(&Unpaid))
		{
			Context.Mates = FMath::Max(Director->CountPack(Victim, Unpaid.PackTag) - 1, 0);
		}
		Context.bLastLungeMissed = State.bLastLungeMissed;
		Context.bNow = bNow;
		// The player open (reloading, swapping, empty, drinking): quick commits, no hang-back; its punisher's turn its punish
		// (UnpaidCreatureOpening.cpp). In front of their view (frontness over a half) its punish blinks round to their back.
		Context.bOpening = FUnpaidOpeningWork::IsOpen(Unpaid, Victim);
		Context.bPunisher = FUnpaidOpeningWork::IsPunisher(Unpaid, Victim);
		Context.bInFront = FightRules::Frontness(PackRules::BearingAround(Victim.GetActorLocation(), Unpaid.GetActorLocation()),
			static_cast<float>(Victim.GetViewRotation().Yaw)) > 0.5f;
		return Context;
	}
}

bool FUnpaidMoveWork::PlanTurn(AUnpaidCreature& Unpaid, const APawn& Victim, float Distance)
{
	if (!IsOn(Unpaid))
	{
		return false;
	}
	FUnpaidMoveState& State = Unpaid.Kit.Moves;
	// Busy with something of its kit's own (a flank readied, a blink, a cinder, a scream, a fade-back, a flight, a hang-back):
	// the base chase and the kit carry on.
	if (Unpaid.Kit.bFlankArmed || Unpaid.IsPhasing() || Unpaid.Kit.Action != EUnpaidAction::None || Unpaid.IsFadingBack() || Unpaid.Kit.bLunging
		|| State.HangLeft > 0.f)
	{
		return false;
	}
	const FUnpaidMoveTuning Tuning = FUnpaidMoveTuning::Live();
	const FUnpaidFightTuning Kit = FUnpaidFightTuning::Live();
	const float Scale = Unpaid.GetSizeScale();
	UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(&Unpaid);
	if (State.Planned == EUnpaidMove::None)
	{
		// Planned only when its attack could begin now, or a close move could start closing in: near, seeing them, past its
		// arrival hold.
		if (Distance > FMath::Max(Unpaid.GetAttackStartRange(), Tuning.CloseInRange * Scale) || !Unpaid.CanStartAttack())
		{
			return false;
		}
		FUnpaidMoveContext Context = ContextFor(Unpaid, Victim, false, Kit, State);
		Context.bMayBlink = Unpaid.MayPackStep(Victim);
		Context.bCamping = Unpaid.IsTargetCamping(Victim);
		const EUnpaidMove Move = UnpaidMoveRules::Choose(Context, Tuning, FMath::FRand());
		if (Move == EUnpaidMove::HangBack)
		{
			// It gives its turn back and circles wide, out of the turns' line, until a pack-mate commits (TickKit).
			State.HangLeft = Tuning.HangBackSeconds;
			State.HangTime = 0.f;
			State.HangSince = Unpaid.GetWorld() ? Unpaid.GetWorld()->GetTimeSeconds() : 0.0;
			State.HangWay = FMath::RandBool() ? 1.f : -1.f;
			State.Cooldowns[UnpaidMoveRules::Index(EUnpaidMove::HangBack)] = Tuning.HangBackCooldown;
			if (Director)
			{
				Director->RestFromTurns(Unpaid, HangRest);
			}
			State.HangRest = HangRest;
			UUnpaidPackSubsystem::Note(&Unpaid, EUnpaidTally::HangBack);
			UE_LOG(LogLooter, Verbose, TEXT("%s hangs back from %s, waiting for its pack."), *Unpaid.GetName(), *Victim.GetName());
			return true;
		}
		State.Planned = Move;
		State.PlanTime = 0.f;
		// On the board at once: a pack-mate's pick this frame sees it.
		if (UUnpaidPackSubsystem* Packs = UUnpaidPackSubsystem::Get(&Unpaid))
		{
			Packs->Commit(Victim, Unpaid.GetUniqueID(), Move);
			State.Told = Move;
		}
		UE_LOG(LogLooter, Verbose, TEXT("%s plans a %s at %s (%.1f m)."), *Unpaid.GetName(), UnpaidMoveRules::Name(Move), *Victim.GetName(),
			Distance / 100.f);
	}
	const UWorld* World = Unpaid.GetWorld();
	State.PlanTime += World ? World->GetDeltaSeconds() : 0.f;
	if (!UnpaidMoveRules::ClosesIn(State.Planned))
	{
		// It begins from here: the chase starts the attack (or walks it the last of the way in).
		return false;
	}
	FUnpaidMoveContext Reach;
	Reach.LungeStart = Kit.LungeStart;
	Reach.RakeRange = Kit.RakeRange;
	if (Distance <= UnpaidMoveRules::StartDistance(State.Planned, Reach, Tuning) * Scale)
	{
		return false;
	}
	if (State.PlanTime > Tuning.CloseInSeconds)
	{
		// Kept off (a backpedal, a fence): it lunges from here if it may, else lets its turn go for another.
		const bool bLunge = Distance <= Unpaid.GetAttackStartRange() && !UnpaidMoveRules::WouldRepeat(EUnpaidMove::Lunge, State.Last, State.BeforeLast);
		State.Planned = bLunge ? EUnpaidMove::Lunge : EUnpaidMove::None;
		if (!bLunge && Director)
		{
			Director->RestFromTurns(Unpaid, GiveUpRest);
		}
		return false;
	}
	// Closing in for it: straight at them, a little faster than its chase (it faces where it goes).
	FVector Way = Victim.GetActorLocation() - Unpaid.GetActorLocation();
	Way.Z = 0.f;
	UCharacterMovementComponent* Movement = Unpaid.GetCharacterMovement();
	Movement->MaxWalkSpeed = FMath::Max(Unpaid.GetChaseSpeedNow() * Tuning.CloseInPace, 1.f);
	Unpaid.AddMovementInput(Way.GetSafeNormal());
	return true;
}

// ---------------------------------------------------------------------------
// Its attack: the move, its timings, its strike
// ---------------------------------------------------------------------------

bool FUnpaidMoveWork::BlinkToStrike(AUnpaidCreature& Unpaid, const APawn& Victim, const FUnpaidMoveTuning& Tuning, int32& OutArm)
{
	// PhaseStrikeLand off them, at their side or behind (the flank's bearings), its own side first.
	const FVector Center = Victim.GetActorLocation();
	const float ViewYaw = static_cast<float>(Victim.GetViewRotation().Yaw);
	const float Bearing = PackRules::BearingAround(Center, Unpaid.GetActorLocation());
	const float VictimFeet = static_cast<float>(Center.Z) - Victim.GetSimpleCollisionHalfHeight();
	const float Radius = Tuning.PhaseStrikeLand * Unpaid.GetSizeScale();
	for (const float Around : UnpaidFightRules::FlankBearings(ViewYaw, Bearing))
	{
		const float Radians = FMath::DegreesToRadians(Around);
		const FVector Candidate(Center.X + FMath::Cos(Radians) * Radius, Center.Y + FMath::Sin(Radians) * Radius, VictimFeet);
		FVector Feet;
		if (!Unpaid.HuntingGround.ContainsSpot(Candidate, Unpaid.GetHome().GetLocation(), Unpaid.GetGroundReach())
			|| !Unpaid.CanStandAt(Candidate, Feet) || FMath::Abs(Feet.Z - VictimFeet) > Unpaid.PhaseStep.MaxRise)
		{
			continue;
		}
		Unpaid.StartStepTo(Feet);
		// It claws with the arm away from the player's middle of view: across them, the way it came round.
		OutArm = FMath::UnwindDegrees(Around - ViewYaw) > 0.f ? 0 : 1;
		UE_LOG(LogLooter, Verbose, TEXT("%s phase-strikes %s: back %.0f degrees round from their view."), *Unpaid.GetName(), *Victim.GetName(),
			FMath::Abs(FMath::UnwindDegrees(Around - ViewYaw)));
		return true;
	}
	return false;
}

bool FUnpaidMoveWork::Choose(AUnpaidCreature& Unpaid)
{
	if (!IsOn(Unpaid))
	{
		return false;
	}
	APawn* Victim = Unpaid.GetTarget();
	if (!Victim)
	{
		return false;
	}
	FUnpaidMoveState& State = Unpaid.Kit.Moves;
	const FUnpaidMoveTuning Tuning = FUnpaidMoveTuning::Live();
	const FUnpaidFightTuning Kit = FUnpaidFightTuning::Live();
	const float Scale = Unpaid.GetSizeScale();
	State.bMissNoted = false;
	State.FlightSpeed = 0.f;
	State.SlideLeft = 0.f;
	State.bStrikeTold = false;
	EUnpaidMove Move = State.Planned;
	State.Planned = EUnpaidMove::None;
	// Its plan, if it still begins from here; else (unplanned: a test, a plan dropped) picked now from what begins here.
	const float Distance = static_cast<float>(FVector::Dist2D(Unpaid.GetActorLocation(), Victim->GetActorLocation()));
	FUnpaidMoveContext Context = ContextFor(Unpaid, *Victim, true, Kit, State);
	if (!UnpaidMoveRules::IsAttack(Move) || Distance > (UnpaidMoveRules::StartDistance(Move, Context, Tuning) + StartSlack) * Scale)
	{
		Context.bMayBlink = Unpaid.MayPackStep(*Victim);
		Context.bCamping = Unpaid.IsTargetCamping(*Victim);
		Move = UnpaidMoveRules::Choose(Context, Tuning, FMath::FRand());
	}
	// The phase-strike's blink: the pack's gate asked again (another may have blinked since it planned), then a spot at their
	// side or behind; shut out or with nowhere to land it rakes if near, else lunges (never a third: then the other).
	if (Move == EUnpaidMove::PhaseStrike && (!Unpaid.MayPackStep(*Victim) || !BlinkToStrike(Unpaid, *Victim, Tuning, State.StrikeArm)))
	{
		Move = Distance <= Kit.RakeRange * Scale ? EUnpaidMove::Rake : EUnpaidMove::Lunge;
		if (UnpaidMoveRules::WouldRepeat(Move, State.Last, State.BeforeLast))
		{
			Move = Move == EUnpaidMove::Rake ? EUnpaidMove::Lunge : EUnpaidMove::Feint;
		}
	}
	if (!UnpaidMoveRules::IsAttack(Move))
	{
		Move = EUnpaidMove::Lunge;
	}
	Unpaid.Kit.Move = Move;
	// Each move's own timings, its rank's quickening on its tell; Fleet's trait then quickens whatever is set here.
	const float Windup = Unpaid.Traits.WindupScale;
	switch (Move)
	{
	case EUnpaidMove::Rake:
		Unpaid.Kit.LeadArm = FMath::RandBool() ? 1 : 0;
		Unpaid.AttackWindup = FMath::Max(Kit.RakeWindup * Windup, 0.05f);
		Unpaid.AttackRecovery = Kit.RakeGap + Kit.RakeRecovery;
		break;
	case EUnpaidMove::Feint:
		// The lunge's own tell (that is the feint), then a short flight, the slide aside and a beat.
		Unpaid.AttackWindup = Unpaid.BaseWindup * Windup;
		Unpaid.AttackRecovery = 0.3f + Tuning.FeintSlideSeconds + 0.3f;
		break;
	case EUnpaidMove::LowSwipe:
		Unpaid.AttackWindup = FMath::Max(Tuning.LowSwipeWindup * Windup, 0.1f);
		Unpaid.AttackRecovery = Tuning.LowSwipeDistance / FMath::Max(Tuning.LowSwipeSpeed, 1.f) + Tuning.LowSwipeRecovery;
		// Its tell: claws dragging through the dirt as it sinks low.
		Unpaid.Voice->Play(FName(Cue::LowSwipe));
		break;
	case EUnpaidMove::PhaseStrike:
		// Gone and back (the step's fades), then its own clear tell before the claw.
		Unpaid.AttackWindup = Unpaid.PhaseFadeOutSeconds + Unpaid.PhaseFadeInSeconds + FMath::Max(Tuning.PhaseStrikeTell * Windup, 0.25f);
		Unpaid.AttackRecovery = Tuning.PhaseStrikeRecovery;
		break;
	default:
		Unpaid.AttackWindup = Unpaid.BaseWindup * Windup;
		Unpaid.AttackRecovery = Unpaid.Kit.BaseRecovery;
		break;
	}
	State.BeforeLast = State.Last;
	State.Last = Move;
	State.Cooldowns[UnpaidMoveRules::Index(Move)] = CooldownOf(Move, Tuning);
	UUnpaidPackSubsystem::Note(&Unpaid, TallyOf(Move));
	return true;
}

bool FUnpaidMoveWork::OnTurnPlayedOut(AUnpaidCreature& Unpaid)
{
	// Its board entry goes at the next sync (TickKit); a shriek owed comes before any fade-back.
	return IsOn(Unpaid) && Unpaid.Kit.Moves.bShriekOwed;
}

bool FUnpaidMoveWork::HoldsAttack(const AUnpaidCreature& Unpaid)
{
	return Unpaid.Kit.Moves.HangLeft > 0.f;
}

bool FUnpaidMoveWork::IsRallied(const AUnpaidCreature& Unpaid)
{
	return Unpaid.Kit.Moves.RalliedLeft > 0.f;
}

EUnpaidMove FUnpaidMoveWork::CommittedNow(const AUnpaidCreature& Unpaid)
{
	// Its attack, its plan, or a blink coming round them (a flank, a step).
	const FUnpaidMoveState& State = Unpaid.Kit.Moves;
	if (Unpaid.GetCreatureState() == ECreatureState::Attack && UnpaidMoveRules::IsAttack(Unpaid.Kit.Move))
	{
		return Unpaid.Kit.Move;
	}
	if (State.Planned != EUnpaidMove::None)
	{
		return State.Planned;
	}
	// (A rise's fade-in is no blink round them.)
	return Unpaid.IsPhasing() && !Unpaid.IsFadingBack() && !Unpaid.IsRising() ? EUnpaidMove::PhaseStrike : EUnpaidMove::None;
}

bool FUnpaidMoveWork::TickKit(AUnpaidCreature& Unpaid, float DeltaSeconds, const APawn& Victim)
{
	FUnpaidMoveState& State = Unpaid.Kit.Moves;
	UUnpaidPackSubsystem* Packs = UUnpaidPackSubsystem::Get(&Unpaid);
	if (!IsOn(Unpaid))
	{
		if (State.Told != EUnpaidMove::None && Packs)
		{
			Packs->Release(Unpaid.GetUniqueID());
		}
		State.Told = EUnpaidMove::None;
		State.Planned = EUnpaidMove::None;
		State.HangLeft = 0.f;
		State.bShriekOwed = false;
		return false;
	}
	for (float& Cooldown : State.Cooldowns)
	{
		Cooldown = FMath::Max(Cooldown - DeltaSeconds, 0.f);
	}
	State.RalliedLeft = FMath::Max(State.RalliedLeft - DeltaSeconds, 0.f);
	State.SinceShriek = FMath::Min(State.SinceShriek + DeltaSeconds, 1000.f);
	if (State.ExposedLeft > 0.f)
	{
		// Its coal laid bare after a choked shriek: it sputters bright, the window to finish it.
		State.ExposedLeft = FMath::Max(State.ExposedLeft - DeltaSeconds, 0.f);
		Unpaid.Heat = FMath::Max(Unpaid.Heat, 0.55f + 0.35f * FMath::Abs(FMath::Sin(State.ExposedLeft * 23.f)));
	}

	// A plan outlives no turn: its turn gone (hit off it, taken back, out of reach), the plan goes with it.
	const UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(&Unpaid);
	if (State.Planned != EUnpaidMove::None && Unpaid.GetCreatureState() != ECreatureState::Attack && (!Director || !Director->HoldsToken(Unpaid)))
	{
		State.Planned = EUnpaidMove::None;
	}
	const EUnpaidMove Now = CommittedNow(Unpaid);
	if (Now != State.Told && Packs)
	{
		if (Now == EUnpaidMove::None)
		{
			Packs->Release(Unpaid.GetUniqueID());
		}
		else
		{
			Packs->Commit(Victim, Unpaid.GetUniqueID(), Now);
		}
		State.Told = Now;
	}

	// A missed lunge's shriek, as soon as its turn has played out and it chases again; its fight moved on, it's dropped.
	const ECreatureState BrainState = Unpaid.GetCreatureState();
	if (State.bShriekOwed && BrainState == ECreatureState::Chase && Unpaid.Kit.Action == EUnpaidAction::None)
	{
		State.bShriekOwed = false;
		if (FUnpaidShriekWork::Start(Unpaid))
		{
			return true;
		}
	}
	else if (State.bShriekOwed && BrainState != ECreatureState::Attack && BrainState != ECreatureState::Chase)
	{
		State.bShriekOwed = false;
	}

	// The hang-back: out of the turns' line, circling wide round them, until a pack-mate commits or its time is up.
	if (State.HangLeft > 0.f)
	{
		const FUnpaidMoveTuning Tuning = FUnpaidMoveTuning::Live();
		State.HangLeft -= DeltaSeconds;
		State.HangTime += DeltaSeconds;
		State.HangRest -= DeltaSeconds;
		const FUnpaidMoveBoard* Board = Packs ? &Packs->BoardFor(Victim) : nullptr;
		const uint32 Id = Unpaid.GetUniqueID();
		// What it waits for: a pack-mate committing an attack since it began to hang back.
		const bool bMateIn = Board && Board->AnyCommittedSince(Id, State.HangSince);
		if (BrainState != ECreatureState::Chase || Unpaid.IsPhasing() || Unpaid.Kit.Action != EUnpaidAction::None || State.HangLeft <= 0.f
			|| (bMateIn && State.HangTime >= Tuning.HangBackMin))
		{
			State.HangLeft = 0.f;
			return false;
		}
		UFightDirectorSubsystem* Turns = UFightDirectorSubsystem::Get(&Unpaid);
		if (Turns && State.HangRest <= HangRenew)
		{
			Turns->RestFromTurns(Unpaid, HangRest);
			State.HangRest = HangRest;
		}
		// A point on a circle outside the ring, a little ahead of it round them; its ring walk this frame is replaced.
		const float Scale = Unpaid.GetSizeScale();
		const FVector Center = Victim.GetActorLocation();
		const FVector Here = Unpaid.GetActorLocation();
		const float Ring = Turns ? Turns->GetRingReach(Unpaid) : 480.f * Scale;
		const float Radius = Ring + Tuning.HangBackOut * Scale;
		const float Bearing = FMath::Atan2(static_cast<float>(Here.Y - Center.Y), static_cast<float>(Here.X - Center.X)) + State.HangWay * HangLead;
		const FVector Goal(Center.X + FMath::Cos(Bearing) * Radius, Center.Y + FMath::Sin(Bearing) * Radius, Here.Z);
		Unpaid.ConsumeMovementInputVector();
		UCharacterMovementComponent* Movement = Unpaid.GetCharacterMovement();
		Movement->MaxWalkSpeed = FMath::Max(Unpaid.GetChaseSpeedNow() * Tuning.HangBackPace, 1.f);
		Unpaid.AddMovementInput((Goal - Here).GetSafeNormal2D());
	}
	return false;
}

void FUnpaidMoveWork::Forget(AUnpaidCreature& Unpaid)
{
	FUnpaidMoveState& State = Unpaid.Kit.Moves;
	if (UUnpaidPackSubsystem* Packs = UUnpaidPackSubsystem::Get(&Unpaid))
	{
		Packs->Release(Unpaid.GetUniqueID());
		if (State.Light != INDEX_NONE)
		{
			Packs->GiveLight(State.Light);
		}
	}
	FUnpaidShriekWork::SettleHat(Unpaid);
	State.Reset();
}

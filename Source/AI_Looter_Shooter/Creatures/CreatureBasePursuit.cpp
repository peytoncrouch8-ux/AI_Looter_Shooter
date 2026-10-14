// ACreatureBase: its hunt going on once it has begun (the user's call, 2026-10-12: creatures follow the player until there's
// a real reason not to). FCreaturePursuit has the rules; this gives it the creature's looks and walks its steps. A look
// comes with its senses' timer (five times a second) and costs at most one sight line, none within SenseRadius; the walking
// is its usual steering. Bosses keep their fights' own rules: they let go as creatures always did.

#include "Creatures/CreatureBase.h"
#include "AI_Looter_Shooter.h"
#include "Bosses/BossComponent.h"
#include "Creatures/CreaturePackComponent.h"
#include "Creatures/EncounterSettings.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** A prowl first walks to its edge (or as near as it gets) for this long at most (s), then paces where it got to. */
	constexpr float ProwlApproachSeconds = 6.f;
	/** It stops a body's length short of leaving its ground (cm, at size 1), and paces this far either way along its edge. */
	constexpr float ProwlEdgeLook = 200.f;
	constexpr float ProwlPaceDistance = 250.f;
	/** Its target gone this much farther off (cm, at size 1) than when it stopped at its edge: it follows along it. */
	constexpr float ProwlFollowDistance = 600.f;
	/** A sniff's spot is this near (cm, at size 1) when it's there: it stands and noses about until the next. */
	constexpr float SniffThere = 80.f;
	/** A boss lets go of a target this far away (cm), where every creature gave up before the pursuit came in. */
	constexpr float BossLetGoDistance = 5500.f;
	/** A target above or below its reach this near (cm flat, at size 1; or within its hold, if farther) is out of its reach. */
	constexpr float OverheadNear = 400.f;
}

bool ACreatureBase::CanReachHeight(float Rise) const
{
	return FMath::Abs(Rise) < GetStrikeHeight();
}

/** A boss (Boss rank, or a creature a boss fight is built round) keeps its fight's own rules: no search, no prowl. */
bool ACreatureBase::KeepsBossRules() const
{
	return CurrentRank == ECreatureRank::Boss || FindComponentByClass<UBossComponent>() != nullptr;
}

bool ACreatureBase::IsFightState(ECreatureState CreatureState)
{
	// Search (tracking where it lost them, sniffing there, watching from its edge) is a hunt but no fight: nothing is coming at them.
	return CreatureState == ECreatureState::Chase || CreatureState == ECreatureState::Attack;
}

bool ACreatureBase::IsFightingWith(const APawn* Pawn) const
{
	return Pawn && Target.Get() == Pawn && IsFightState(State);
}

// ---------------------------------------------------------------------------
// Taking up a hunt
// ---------------------------------------------------------------------------

/** Sets it on Prey, a new hunt (one off its ground is watched from its edge). */
void ACreatureBase::BeginHunt(APawn* Prey)
{
	Target = Prey;
	Pursuit.Begin(Prey->GetActorLocation());
	if (State != ECreatureState::Attack)
	{
		SetState(ECreatureState::Chase);
	}
	// Turned on one off its ground (a shot from past its edge, a pack's call): it goes to its edge to watch, not after them.
	if (!KeepsBossRules() && CheckHunt(Prey) == EHuntCheck::OffGround && State == ECreatureState::Chase)
	{
		UpdatePursuit(0.f);
	}
}

/** A hit (bHit) or a pack's call from Prey: a new hunt, or word of where its target is. True when it newly turned on them. */
bool ACreatureBase::TakeUpHunt(APawn* Prey, bool bHit)
{
	if (bPassive || State == ECreatureState::Dead || !Prey)
	{
		return false;
	}
	if (KeepsBossRules())
	{
		// As it always was: a hit or a call sets it on a player it may hunt, if it's after nobody (its fight does the rest).
		if (Target.IsValid() || !IsValidTarget(Prey))
		{
			return false;
		}
		Target = Prey;
		Pursuit.Begin(Prey->GetActorLocation());
		if (State != ECreatureState::Attack)
		{
			SetState(ECreatureState::Chase);
		}
		return true;
	}
	// A hit from beyond its sight still turns it, and one from off its ground sends it to its edge; nobody in a safe zone.
	const EHuntCheck Check = CheckHunt(Prey);
	if (Check != EHuntCheck::Huntable && Check != EHuntCheck::OffGround)
	{
		return false;
	}
	if (!Target.IsValid())
	{
		BeginHunt(Prey);
		return true;
	}
	if (Target.Get() != Prey)
	{
		return false;
	}
	// Already after them: now it knows where they are (shot at, its patience fills again), and a search is a hunt again.
	if (bHit)
	{
		Pursuit.NoteHit(Prey->GetActorLocation());
	}
	else
	{
		Pursuit.Reacquire(Prey->GetActorLocation());
	}
	if (State == ECreatureState::Search && Check == EHuntCheck::Huntable && Pursuit.GetStep() == EPursuitStep::Hunt)
	{
		SetHuntState(ECreatureState::Chase);
	}
	return false;
}

// ---------------------------------------------------------------------------
// Its looks while it hunts
// ---------------------------------------------------------------------------

/** Its look at its target (every perception, Interval apart): the hunt goes on, turns to a search or a wait, or ends. */
void ACreatureBase::UpdatePursuit(float Interval)
{
	APawn* Prey = Target.Get();
	if (!Prey)
	{
		Pursuit.Stop(EPursuitEnd::Gone);
		GiveUpHunt();
		return;
	}
	const FVector Here = GetActorLocation();
	const FVector Spot = Prey->GetActorLocation();
	const float Distance = static_cast<float>(FVector::Dist(Here, Spot));
	if (KeepsBossRules())
	{
		// A boss lets go as creatures always did, at once: off its ground, in a safe zone, dead, or as far away as creatures
		// used to give up. Its fight (its seal, its leash, standing down) decides the rest.
		if (!IsValidTarget(Prey) || Distance > FMath::Min(LoseInterestRadius, BossLetGoDistance))
		{
			Target.Reset();
			SetState(ECreatureState::Return);
		}
		return;
	}

	const UEncounterSettings& Settings = UEncounterSettings::Get();
	const EHuntCheck Check = CheckHunt(Prey);
	FCreaturePursuit::FLook Look;
	Look.Interval = Interval;
	Look.Here = Here;
	Look.Prey = Spot;
	Look.SizeScale = SizeScale;
	Look.bGone = Check == EHuntCheck::NotPrey || Distance > LoseInterestRadius;
	Look.bSheltered = Check == EHuntCheck::Sheltered;
	Look.bInScene = IsSceneHoldingPlayer();
	Look.bOffGround = Check == EHuntCheck::OffGround;
	// Its eyes: near enough it hears them; farther, one sight line a look (none at all once they're out of its hunt).
	const bool bOutOfHunt = Look.bGone || Look.bSheltered || Look.bInScene || Look.bOffGround;
	Look.bSeen = !bOutOfHunt && (Distance <= Settings.SenseRadius * SizeScale || HasLineOfSight(Prey));
	Look.bBeyondSenses = Distance > AggroRadius;
	Look.HoldDistance = GetChaseHold();
	const float Flat = static_cast<float>(FVector::Dist2D(Here, Spot));
	Look.bFarOff = Flat > Look.HoldDistance;
	Look.bBreakingOff = Pack->IsRetreating();
	// Up on a rock or a roof over it (round the rock's foot counts, not only right under them), out of its strike.
	Look.bOverhead = Flat <= FMath::Max(Look.HoldDistance, OverheadNear * SizeScale) && !CanReachHeight(static_cast<float>(Spot.Z - Here.Z));

	const EPursuitStep Before = Pursuit.GetStep();
	const EPursuitStep Now = Pursuit.Look(Look, Settings.MakePursuitRules());
	switch (Now)
	{
	case EPursuitStep::Hunt:
		if (State == ECreatureState::Search)
		{
			SetHuntState(ECreatureState::Chase);
		}
		else if (State != ECreatureState::Chase && State != ECreatureState::Attack)
		{
			SetState(ECreatureState::Chase);
		}
		break;
	case EPursuitStep::GiveUp:
		GiveUpHunt();
		break;
	default:
		if (State == ECreatureState::Chase)
		{
			SetHuntState(ECreatureState::Search);
		}
		else if (State != ECreatureState::Search && State != ECreatureState::Attack)
		{
			SetState(ECreatureState::Search);
		}
		else if (Now != Before)
		{
			// A new step of its search: its way is looked for again at once.
			SteerTimer = 0.f;
		}
		break;
	}
}

/** The hunt is over (its pursuit says why): it lets go and walks home, calmly. */
void ACreatureBase::GiveUpHunt()
{
	APawn* Prey = Target.Get();
	const EPursuitEnd Why = Pursuit.GetEnd();
	// One it couldn't get to, it leaves be a while if they stay put, or it would be back under them at its next look.
	if (Prey && Why == EPursuitEnd::OutOfReach && GetWorld())
	{
		Pursuit.Spurn(Prey->GetUniqueID(), Prey->GetActorLocation(), GetWorld()->GetTimeSeconds() + UEncounterSettings::Get().SpurnSeconds);
	}
	UE_LOG(LogLooter, Verbose, TEXT("%s gives up on %s: %s."), *GetName(), *GetNameSafe(Prey), LexToString(Why));
	// Calmly home: no rush, nothing healed (it comes back as it is).
	Target.Reset();
	SetState(ECreatureState::Return);
}

/** An attack played out: after its target again, or as its pursuit says if they're out of its hunt. */
void ACreatureBase::ResumeHunt()
{
	APawn* Prey = Target.Get();
	if (IsValidTarget(Prey))
	{
		SetState(ECreatureState::Chase);
		return;
	}
	if (!Prey || KeepsBossRules())
	{
		// As it always was: its senses let go of them at their next look.
		SetState(ECreatureState::Return);
		return;
	}
	// Off its ground, sheltered or gone while it attacked: its pursuit says what now.
	SetState(ECreatureState::Chase);
	UpdatePursuit(0.f);
}

/** Between chasing and searching: one hunt, so its way round what's in the way carries on (SetState starts it afresh). */
void ACreatureBase::SetHuntState(ECreatureState NewState)
{
	const float KeptStuck = StuckTime;
	const float KeptSteer = SteerTimer;
	SetState(NewState);
	StuckTime = KeptStuck;
	SteerTimer = KeptSteer;
}

// ---------------------------------------------------------------------------
// Searching and waiting
// ---------------------------------------------------------------------------

void ACreatureBase::TickSearch(float DeltaSeconds)
{
	const APawn* Prey = Target.Get();
	if (!Prey)
	{
		Pursuit.Stop(EPursuitEnd::Gone);
		GiveUpHunt();
		return;
	}
	switch (Pursuit.GetStep())
	{
	case EPursuitStep::Track:
		// Where it last knew them to be, at a run (one that hangs back stops at its distance from it: its look says).
		MoveToward(Pursuit.GetLastKnown(), ChaseSpeed, DeltaSeconds);
		break;
	case EPursuitStep::Sniff:
		TickSniff(DeltaSeconds);
		break;
	case EPursuitStep::Prowl:
		TickProwl(DeltaSeconds, *Prey);
		break;
	default:
		break;
	}
}

void ACreatureBase::TickSniff(float DeltaSeconds)
{
	// Nosing round where it lost them: a spot near it every second or two (on its ground), walked to, stood at. One that hangs
	// back (the Spitter, the Bell-Ringer) noses round where it stopped, at its distance, rather than walking into the spot.
	FCreaturePursuit::FWalk& Walk = Pursuit.Walk;
	const float Radius = UEncounterSettings::Get().SniffRadius * SizeScale;
	if (!Walk.bAnchored)
	{
		Walk.bAnchored = true;
		Walk.Anchor = GetChaseHold() > Radius ? GetActorLocation() : Pursuit.GetLastKnown();
	}
	Walk.Clock -= DeltaSeconds;
	if (!Walk.bHasGoal || Walk.Clock <= 0.f)
	{
		const FVector2D Offset = FMath::RandPointInCircle(Radius);
		Walk.Goal = Walk.Anchor + FVector(Offset, 0.f);
		if (HuntingGround.IsSet() && !HuntingGround.Contains(Walk.Goal, Home.GetLocation()))
		{
			Walk.Goal = Walk.Anchor;
		}
		Walk.bHasGoal = true;
		Walk.Clock = FMath::FRandRange(1.2f, 2.2f);
	}
	if (FVector::DistSquared2D(GetActorLocation(), Walk.Goal) > FMath::Square(SniffThere * SizeScale))
	{
		MoveToward(Walk.Goal, WalkSpeed, DeltaSeconds);
	}
}

void ACreatureBase::TickProwl(float DeltaSeconds, const APawn& Prey)
{
	FCreaturePursuit::FWalk& Walk = Pursuit.Walk;
	const FVector Here = GetActorLocation();
	const FVector Spot = Prey.GetActorLocation();
	FVector Toward(Spot.X - Here.X, Spot.Y - Here.Y, 0.0);
	const float Flat = static_cast<float>(Toward.Size());
	Toward = Flat > 1.f ? Toward / Flat : GetActorForwardVector().GetSafeNormal2D();

	const bool bOffGround = Pursuit.IsProwlingOffGround();
	if (Walk.bAnchored && bOffGround && Flat > Walk.AnchorReach + ProwlFollowDistance * SizeScale)
	{
		// They went on along outside its ground: it follows along its side of the edge.
		Walk.bAnchored = false;
		Walk.ApproachTime = 0.f;
	}
	if (!Walk.bAnchored)
	{
		// Off its ground, it first comes up to its edge facing them: as far as a body's length short of leaving it, as near
		// as it closes in to, or as far as it gets in a few seconds. Under one out of its reach it waits where it is.
		Walk.ApproachTime += DeltaSeconds;
		const bool bAtEdge = HuntingGround.IsSet() && !HuntingGround.Contains(Here + Toward * (ProwlEdgeLook * SizeScale), Home.GetLocation());
		if (bOffGround && !bAtEdge && Flat > GetChaseHold() && Walk.ApproachTime < ProwlApproachSeconds)
		{
			MoveToward(Spot, WalkSpeed * 1.5f, DeltaSeconds);
			return;
		}
		Walk.bAnchored = true;
		Walk.Anchor = Here;
		Walk.AnchorReach = Flat;
		Walk.bPacing = false;
		Walk.Clock = FMath::FRandRange(0.6f, 1.2f);
	}

	// Pacing there: a few steps one way along its edge, a pause watching them, a few steps the other way.
	Walk.Clock -= DeltaSeconds;
	if (Walk.Clock <= 0.f)
	{
		Walk.bPacing = !Walk.bPacing;
		Walk.Clock = Walk.bPacing ? FMath::FRandRange(1.2f, 2.f) : FMath::FRandRange(0.8f, 1.6f);
		if (Walk.bPacing)
		{
			Walk.PaceSign = -Walk.PaceSign;
			const FVector Side(-Toward.Y, Toward.X, 0.0);
			Walk.Goal = Walk.Anchor + Side * (Walk.PaceSign * ProwlPaceDistance * SizeScale);
			if (HuntingGround.IsSet() && !HuntingGround.Contains(Walk.Goal, Home.GetLocation()))
			{
				Walk.Goal = Walk.Anchor;
			}
		}
	}
	if (Walk.bPacing && FVector::DistSquared2D(Here, Walk.Goal) > FMath::Square(SniffThere * SizeScale))
	{
		MoveToward(Walk.Goal, WalkSpeed * 0.8f, DeltaSeconds);
	}
	else
	{
		FaceToward(Spot, DeltaSeconds);
	}
}

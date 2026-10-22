// ASpitterSpider keeping its distance. Its brain (ACreatureBase's Chase) brings it in to three quarters of its attack
// range, CloseInTo, and there only turns it to face its target; inside that, this moves it: backing away from a target
// nearer than BackOffWithin, sidestepping round one in its band in bursts, always facing it (its walking movement stops
// turning it to where it walks, and never steps it off a ledge it can't see behind it). Its ways are the steering's own
// looks (LookAlong: something in the way, or no ground ahead), kept to its hunting ground; with no way back it's cornered.
// In a played fight the director gives it a spot in its band (off the melee pack's side, a clear line at its target, a new
// one after a shot now and then): it walks round to it and holds there instead of strafing (TickToSpot).

#include "Creatures/SpitterSpider.h"
#include "Creatures/CreaturePackComponent.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/FightRules.h"
#include "Creatures/FightTuning.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** Its ways are looked for again this often (s), at once when it's newly too close. */
	constexpr float KeepLookEvery = 0.2f;
	/** At its fight spot within this (cm, at size 1); its lob blocked there this long (s) with its spit ready, it asks for another. */
	constexpr float SpotArrive = 120.f;
	constexpr float LobBlockedMove = 1.5f;
	/** A strafe's burst and the pause after it (s, each rolled between these). */
	constexpr float StrafeMin = 1.f;
	constexpr float StrafeMax = 2.2f;
	constexpr float PauseMin = 0.5f;
	constexpr float PauseMax = 1.1f;
	/** A strafe drifts out a little as it goes round (share of a step away for each step sideways). */
	constexpr float StrafeDrift = 0.15f;
}

void ASpitterSpider::TickKeepAway(float DeltaSeconds)
{
	const APawn* Victim = GetTarget();
	// In its notice beat it rears where it is first (CreatureBaseReact.cpp), before it keeps its distance.
	const bool bHunting = Victim && !IsDead() && GetCreatureState() == ECreatureState::Chase && !GetPack()->IsRetreating() && !IsNoticing();
	const float Distance = Victim ? static_cast<float>(FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation())) : TNumericLimits<float>::Max();
	const float Hold = GetAttackRange() * 0.75f;
	if (!bHunting || Distance > Hold)
	{
		// Calm, attacking, or too far: its brain moves it (and turns it to where it walks).
		SetKeepingDistance(false);
		Keep = ESpitterKeep::Close;
		CorneredFor = 0.f;
		KeepLookIn = 0.f;
		return;
	}
	SetKeepingDistance(true);

	// Its strafe's rhythm: a burst one way, a pause, a burst (now and then the other way).
	StrafeLeft -= DeltaSeconds;
	if (StrafeLeft <= 0.f)
	{
		bStrafing = !bStrafing;
		StrafeLeft = bStrafing ? FMath::FRandRange(StrafeMin, StrafeMax) : FMath::FRandRange(PauseMin, PauseMax);
		if (bStrafing && FMath::RandBool())
		{
			StrafeSign = -StrafeSign;
		}
		KeepLookIn = 0.f;
	}

	FVector Away = GetActorLocation() - Victim->GetActorLocation();
	Away.Z = 0.0;
	Away = Away.IsNearlyZero() ? -GetActorForwardVector().GetSafeNormal2D() : Away.GetSafeNormal();
	const FVector Side = FVector(-Away.Y, Away.X, 0.0) * StrafeSign;
	const float BackOff = GetBackOffDistance();
	const bool bTooClose = Distance < BackOff;
	const bool bWasBacking = Keep == ESpitterKeep::BackOff || Keep == ESpitterKeep::Cornered;

	// Its spot in the fight (off the melee pack's side, a clear line at its target): in its band it walks round to it and
	// holds there, rather than strafing about (with no director, or no spot to be had, it strafes as it always did).
	UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(this);
	FVector Spot;
	const bool bHasSpot = !bTooClose && Director && Director->GetWaitSpot(*this, *Victim, Spot);
	if (bHasSpot)
	{
		TickToSpot(*Victim, Spot, DeltaSeconds, *Director);
		return;
	}
	LobBlockedFor = 0.f;

	KeepLookIn -= DeltaSeconds;
	if (KeepLookIn <= 0.f || bTooClose != bWasBacking || Keep == ESpitterKeep::Reposition)
	{
		KeepLookIn = KeepLookEvery;
		if (bTooClose)
		{
			// Straight back first, then back and to its side, then out sideways: whichever is clear.
			const float Turn = StrafeSign;
			const FVector Ways[] = { Away, Away.RotateAngleAxis(35.f * Turn, FVector::UpVector), Away.RotateAngleAxis(-35.f * Turn, FVector::UpVector),
				Away.RotateAngleAxis(70.f * Turn, FVector::UpVector), Away.RotateAngleAxis(-70.f * Turn, FVector::UpVector), Side, -Side };
			KeepWay = FirstClearWay(Ways);
		}
		else if (bStrafing)
		{
			// Round its target, its own way first, else the other; neither clear, it stands for now.
			const FVector Ways[] = { (Side + Away * StrafeDrift).GetSafeNormal(), (-Side + Away * StrafeDrift).GetSafeNormal() };
			KeepWay = FirstClearWay(Ways);
			if (FVector::DotProduct(KeepWay, Side) < 0.0)
			{
				StrafeSign = -StrafeSign;
			}
		}
		else
		{
			KeepWay = FVector::ZeroVector;
		}
	}

	const bool bHasWay = !KeepWay.IsNearlyZero();
	Keep = SpitterRules::ChooseKeep(Distance, BackOff, Hold, !bTooClose || bHasWay, bStrafing && bHasWay);
	CorneredFor = Keep == ESpitterKeep::Cornered ? CorneredFor + DeltaSeconds : 0.f;
	if (Keep == ESpitterKeep::BackOff || Keep == ESpitterKeep::Strafe)
	{
		GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed * (Keep == ESpitterKeep::BackOff ? BackOffSpeedShare : StrafeSpeedShare);
		AddMovementInput(KeepWay);
	}
}

void ASpitterSpider::TickToSpot(const APawn& Victim, const FVector& Spot, float DeltaSeconds, UFightDirectorSubsystem& Director)
{
	CorneredFor = 0.f;
	const FVector Here = GetActorLocation();
	const float Gap = static_cast<float>(FVector::Dist2D(Here, Spot));
	if (Gap <= SpotArrive * GetSizeScale())
	{
		// There: it stands and watches. A spit that's ready but has no clear lob from here for a while: it asks for another spot.
		Keep = ESpitterKeep::Hold;
		KeepWay = FVector::ZeroVector;
		LobBlockedFor = IsSpitReady() && !IsLobClear(Victim) ? LobBlockedFor + DeltaSeconds : 0.f;
		if (LobBlockedFor >= LobBlockedMove)
		{
			LobBlockedFor = 0.f;
			Director.RequestNewSpot(*this);
		}
		return;
	}
	LobBlockedFor = 0.f;
	// Round its target to it (never across them), its way looked for a few times a second as the strafe's is: the way
	// round, or a little either side of it.
	KeepLookIn -= DeltaSeconds;
	if (KeepLookIn <= 0.f || Keep != ESpitterKeep::Reposition)
	{
		KeepLookIn = KeepLookEvery;
		const FVector Way = FightRules::OrbitWay(Here, Victim.GetActorLocation(), Spot);
		const FVector Ways[] = { Way, Way.RotateAngleAxis(30.f, FVector::UpVector), Way.RotateAngleAxis(-30.f, FVector::UpVector) };
		KeepWay = FirstClearWay(Ways);
	}
	if (KeepWay.IsNearlyZero())
	{
		Keep = ESpitterKeep::Hold;
		return;
	}
	Keep = ESpitterKeep::Reposition;
	GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed * FFightTuning::LiveRangedMoveShare();
	AddMovementInput(KeepWay);
}

FVector ASpitterSpider::FirstClearWay(TConstArrayView<FVector> Candidates) const
{
	const float Look = GetSteerProbes().SweepLength;
	const FVector Here = GetActorLocation();
	const FVector Middle = GetHome().GetLocation();
	for (const FVector& Way : Candidates)
	{
		if (Way.IsNearlyZero())
		{
			continue;
		}
		// Never off its hunting ground (its spawner's), whose edge it would only turn back from.
		if (HuntingGround.IsSet() && !HuntingGround.Contains(Here + Way * Look, Middle))
		{
			continue;
		}
		if (LookAlong(Way, Look).bClear)
		{
			return Way;
		}
	}
	return FVector::ZeroVector;
}

void ASpitterSpider::SetKeepingDistance(bool bKeeping)
{
	if (bKeepingDistance == bKeeping)
	{
		return;
	}
	bKeepingDistance = bKeeping;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	// Keeping its distance it faces its target (its brain turns it) whichever way it walks, and walks off no ledge: backing
	// away, it can't see what's behind it as its steering's looks would.
	Movement->bOrientRotationToMovement = !bKeeping;
	Movement->bCanWalkOffLedges = !bKeeping;
}

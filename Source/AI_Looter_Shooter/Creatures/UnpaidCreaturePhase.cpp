// AUnpaidCreature's phase-step: stuck, or fallen far behind while it chases, it fades out and comes back 3 to 5 m nearer
// its target. UnpaidRules has the rules (when, how far, which spot first); this is where they meet the level: ground it
// can stand on, room for its body, its target in sight from there, no safe zone and nothing past the playable area. Its
// material does the fading (Phase, in the mesh's custom primitive data): the solid body goes first and the shroud's
// ends last, and coming back the ends come first. Out of its target's sight and too near to gain ground, it may step to
// where it sees them instead (UnpaidCreatureSight.cpp); one step or flank at a player at a time across the pack (MayPackStep).

#include "Creatures/UnpaidCreature.h"
#include "Creatures/UnpaidFightTuning.h"
#include "Creatures/UnpaidLifeWork.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "World/PlayableArea.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** Ground steeper than about 50 degrees (the creatures' walkable floor angle) is no place to come back on. */
	constexpr double WalkableGroundNormalZ = 0.64;

	/** What stands on the ground (rocks, wagons, buildings) carries this tag for the minimap: it never lands on top of one. */
	const FName StandingObstacleTag(TEXT("Obstacle"));

	/** The room a spot needs is checked this far up from the ground (cm, at size 1), so a walkable slope doesn't touch it. */
	constexpr float RoomLift = 25.f;

	/** Where it can see its target from: about its eyes (a share of its height over the ground). */
	constexpr float EyeShare = 1.6f;

	/** Nowhere to come back on: it looks again after this long (s), not every frame. */
	constexpr float RetrySeconds = 0.75f;

	/** A step to sight may land on what its target stands on, this near their feet's height (cm). */
	constexpr float TheirFloorBand = 60.f;

	/** Its target's feet (world Z): where they stand. */
	float FeetZOf(const APawn& Victim)
	{
		return static_cast<float>(Victim.GetActorLocation().Z) - Victim.GetSimpleCollisionHalfHeight();
	}
}

bool AUnpaidCreature::IsPhasing() const
{
	return Kit.PhaseState != EPhaseState::None;
}

void AUnpaidCreature::TickPhaseStep(float DeltaSeconds)
{
	TickSight(DeltaSeconds);
	if (Kit.PhaseState == EPhaseState::None)
	{
		Kit.SinceLastStep = FMath::Min(Kit.SinceLastStep + DeltaSeconds, 1000.f);
		Kit.RetryIn = FMath::Max(Kit.RetryIn - DeltaSeconds, 0.f);
		WatchProgress(DeltaSeconds);
		// One blink at a player at a time across the pack (the gate asked last: the step is wanted).
		if (WantsToStep() && MayPackStep(*GetTarget()))
		{
			StartPhaseStep();
		}
		return;
	}

	// Buried for an ambush's rise, it stays gone until its hand breaks the ground (UnpaidCreatureRise.cpp starts the fade then).
	if (Kit.PhaseState == EPhaseState::In && FUnpaidRiseWork::HoldsPhase(*this))
	{
		PhaseAmount = 1.f;
		return;
	}
	Kit.PhaseTime += DeltaSeconds;
	if (Kit.PhaseState == EPhaseState::Out)
	{
		PhaseAmount = FMath::Clamp(Kit.PhaseTime / PhaseFadeOutSeconds, 0.f, 1.f);
		if (PhaseAmount >= 1.f)
		{
			Arrive();
		}
		return;
	}
	PhaseAmount = 1.f - FMath::Clamp(Kit.PhaseTime / PhaseFadeInSeconds, 0.f, 1.f);
	if (PhaseAmount <= 0.f)
	{
		Kit.PhaseState = EPhaseState::None;
		Kit.SinceLastStep = 0.f;
		ResetProgress();
	}
}

void AUnpaidCreature::WatchProgress(float DeltaSeconds)
{
	const APawn* Victim = GetTarget();
	if (GetCreatureState() != ECreatureState::Chase || !Victim)
	{
		ResetProgress();
		return;
	}
	// Getting nearer than it has been is progress; a player who keeps their distance (outrunning it) is as good as a wall.
	const float Distance = static_cast<float>(FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation()));
	if (Distance < Kit.BestDistance - PhaseStep.ProgressNeeded)
	{
		Kit.BestDistance = Distance;
		Kit.StallTime = 0.f;
	}
	else
	{
		Kit.StallTime += DeltaSeconds;
	}
}

void AUnpaidCreature::ResetProgress()
{
	Kit.StallTime = 0.f;
	Kit.BestDistance = TNumericLimits<float>::Max();
}

bool AUnpaidCreature::WantsToStep() const
{
	const APawn* Victim = GetTarget();
	// Fading back it has stepped away on purpose (UnpaidCreatureFadeBack.cpp): no step in until its wait is done.
	// Nor while it climbs out of the ground: it comes for them once it stands.
	if (!Victim || Kit.bLunging || Kit.RetryIn > 0.f || IsFadingBack() || IsRising() || GetCreatureState() != ECreatureState::Chase)
	{
		return false;
	}
	const float Distance = static_cast<float>(FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation()));
	// Pressed by a hit from past its edge: it blinks in as soon as it may, stuck or not (a long way short counts as stuck).
	const float Stuck = Kit.bPressStepOwed ? FMath::Max(Kit.StallTime, PhaseStep.StuckSeconds) : Kit.StallTime;
	// Waiting its turn on the fight's ring it holds back on purpose: that's no stall to step out of (CreatureBaseFight.cpp),
	// unless the wait goes nowhere (asked last: its trace only once the rest says step).
	if (UnpaidRules::WantsPhaseStep(PhaseStep, Distance, Stuck, Kit.SinceLastStep)
		&& (!IsWaitingTurn() || Kit.bPressStepOwed || WaitGoesNowhere(*Victim)))
	{
		return true;
	}
	// Stalled out of its target's sight, too near for a step that gains ground (outside the nave's wall, behind a door leaf):
	// a step to where it sees them, nearer or not (FindPhaseSpot). The cheap asks first, its sight last.
	return UnpaidRules::WantsSightStep(PhaseStep, false, Stuck, Kit.SinceLastStep) && NeedsSight()
		&& FUnpaidFightTuning::Live().BlindStep != 0 && !SeesTarget(*Victim);
}

bool AUnpaidCreature::WaitGoesNowhere(const APawn& Victim) const
{
	if (!HasFightKit() || FUnpaidFightTuning::Live().BlindStep == 0)
	{
		return false;
	}
	// Pressed, its target past its own edge: a fence or a wall it can't walk through holds it at the edge.
	if (IsPressed() && HuntingGround.IsSet() && !HuntingGround.Contains(Victim.GetActorLocation(), GetHome().GetLocation()))
	{
		return true;
	}
	// Out of their sight where it waits (a wall between: the player inside a building): its turn would never get it there.
	return !SeesTarget(Victim);
}

float AUnpaidCreature::GetPressReach() const
{
	return HasFightKit() ? FUnpaidFightTuning::Live().PressReach : 0.f;
}

float AUnpaidCreature::GetPressSeconds() const
{
	return HasFightKit() ? FUnpaidFightTuning::Live().PressSeconds : 0.f;
}

void AUnpaidCreature::OnPressedByHit(const APawn& By)
{
	// Only the one hit owes the blink: its pack walks in after it, so the doorway never fills with nine at once. Logged once
	// a debt (every hit of a burst came here: 202 lines in one play-check).
	if (!Kit.bPressStepOwed)
	{
		UE_LOG(LogLooter, Verbose, TEXT("%s is pressed past its edge by %s: it comes in."), *GetName(), *By.GetName());
	}
	Kit.bPressStepOwed = true;
}

FString AUnpaidCreature::DescribePhaseRefusals() const
{
	const APawn* Victim = GetTarget();
	if (!Victim)
	{
		return TEXT("no target");
	}
	const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	const float TheirFeet = FeetZOf(*Victim);
	// Each candidate's first refusal: 0 off its ground, 1 no ground, 2 steep or an obstacle's top, 3 no room, 4 no sight,
	// 5 safe zone or past the area, 6 another level (a step to sight: neither its own level nor its target's).
	auto Count = [&](const TArray<FVector>& Candidates, bool bToSight)
	{
		int32 Counts[7] = {};
		int32 Fine = 0;
		for (const FVector& Candidate : Candidates)
		{
			if (!HuntingGround.ContainsSpot(Candidate, GetHome().GetLocation(), GetGroundReach()))
			{
				++Counts[0];
				continue;
			}
			FVector Ground;
			int32 Refusal = 0;
			if (!CanStandAt(Candidate, Ground, &Refusal, bToSight))
			{
				++Counts[FMath::Clamp(Refusal, 1, 5)];
				continue;
			}
			const bool bLevel = bToSight
				? UnpaidRules::IsSightStepLevel(PhaseStep, static_cast<float>(Feet.Z), TheirFeet, static_cast<float>(Ground.Z))
				: FMath::Abs(Ground.Z - Feet.Z) <= PhaseStep.MaxRise;
			if (!bLevel)
			{
				++Counts[6];
				continue;
			}
			++Fine;
		}
		return FString::Printf(TEXT("%d spots: fine %d, off ground %d, no ground %d, steep/obstacle %d, no room %d, no sight %d, safe/area %d, other level %d"),
			Candidates.Num(), Fine, Counts[0], Counts[1], Counts[2], Counts[3], Counts[4], Counts[5], Counts[6]);
	};
	const FString Gaining = Count(UnpaidRules::PhaseCandidates(PhaseStep, Feet, Victim->GetActorLocation()), false);
	const FString ToSight = Count(UnpaidRules::SightCandidates(PhaseStep, Feet, Victim->GetActorLocation(), TheirFeet), true);
	return FString::Printf(TEXT("at %.1f m, %.0f cm under them (reach %.0f, waiting %d, pressed %d, sees %d, blind spell %d). Gaining: %s. To sight: %s"),
		FVector::Dist2D(Feet, Victim->GetActorLocation()) / 100.f, TheirFeet - Feet.Z, GetGroundReach(), IsWaitingTurn() ? 1 : 0,
		IsPressed() ? 1 : 0, SeesTarget(*Victim) ? 1 : 0, Kit.Sight.bBlindSpell ? 1 : 0, *Gaining, *ToSight);
}

void AUnpaidCreature::StartPhaseStep()
{
	FVector Feet;
	if (!FindPhaseSpot(Feet))
	{
		Kit.RetryIn = RetrySeconds;
		if (Kit.bPressStepOwed && UE_LOG_ACTIVE(LogLooter, Verbose))
		{
			UE_LOG(LogLooter, Verbose, TEXT("%s owes a step in, nowhere to land: %s"), *GetName(), *DescribePhaseRefusals());
		}
		return;
	}
	Kit.bPressStepOwed = false;
	UE_LOG(LogLooter, Verbose, TEXT("%s phase-steps %.1f m nearer its target (stuck %.1f s)."), *GetName(),
		(FVector::Dist2D(GetActorLocation(), GetTarget()->GetActorLocation()) - FVector::Dist2D(Feet, GetTarget()->GetActorLocation())) / 100.0,
		Kit.StallTime);
	StartStepTo(Feet);
}

void AUnpaidCreature::StartStepTo(const FVector& Feet)
{
	Kit.PhaseDestination = Feet;
	Kit.PhaseState = EPhaseState::Out;
	Kit.PhaseTime = 0.f;
	GetCharacterMovement()->StopMovementImmediately();
	// Counted at its target's step gate (a phase-step or a flank alike): the next of the pack waits Looter.Unpaid.PackStepGap.
	UFightDirectorSubsystem* Director = !IsBossCreature() ? UFightDirectorSubsystem::Get(this) : nullptr;
	if (const APawn* Victim = GetTarget(); Director && Victim)
	{
		Director->NoteStep(*this, *Victim);
	}
}

bool AUnpaidCreature::FindPhaseSpot(FVector& OutFeet) const
{
	const APawn* Victim = GetTarget();
	if (!GetWorld() || !Victim)
	{
		return false;
	}
	const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	auto CanStand = [this](const FVector& Candidate, FVector& OutGround) { return CanStandAt(Candidate, OutGround); };
	if (UnpaidRules::ChoosePhaseSpot(PhaseStep, Feet, Victim->GetActorLocation(), HuntingGround, GetHome().GetLocation(), CanStand,
		OutFeet, GetGroundReach()))
	{
		return true;
	}
	// Out of their sight with no step that gains ground: one to where it sees them (CanStandAt's sight), nearer or not, on
	// its own floor or theirs, onto what they stand on too (the chapel's raised dais, 2026-10-28: every step to sight landed
	// back outside).
	auto CanStandToSight = [this](const FVector& Candidate, FVector& OutGround) { return CanStandAt(Candidate, OutGround, nullptr, true); };
	return NeedsSight() && FUnpaidFightTuning::Live().BlindStep != 0 && !SeesTarget(*Victim)
		&& UnpaidRules::ChooseSightSpot(PhaseStep, Feet, Victim->GetActorLocation(), FeetZOf(*Victim), HuntingGround, GetHome().GetLocation(),
			CanStandToSight, OutFeet, GetGroundReach());
}

bool AUnpaidCreature::CanStandAt(const FVector& Candidate, FVector& OutGround, int32* OutRefusal, bool bTheirFloorToo, const APawn* For) const
{
	int32 Ignored = 0;
	int32& Refusal = OutRefusal ? *OutRefusal : Ignored;
	UWorld* World = GetWorld();
	// Its target's sight, or (an ambush's rise, before it has one) the player it rises round.
	const APawn* Victim = For ? For : GetTarget();
	if (!World || !Victim)
	{
		return false;
	}
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float Scale = GetSizeScale();
	const FVector TargetEye = Victim->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const float Reach = PhaseStep.MaxRise + 50.f;

	// World-static ground only (terrain and solid props, never volumes); the room and sight checks see the trees and rocks
	// too, and never itself or its target.
	const FCollisionQueryParams GroundParams = LooterWorld::StaticGeometryParams(World, TEXT("UnpaidPhaseGround"), this);
	FCollisionQueryParams OpenParams(SCENE_QUERY_STAT(UnpaidPhaseRoom), false, this);
	OpenParams.AddIgnoredActor(Victim);

	// Ground under it near its own level, walkable, and not the top of a rock, a wagon or a roof.
	FHitResult Hit;
	if (!World->LineTraceSingleByObjectType(Hit, Candidate + FVector(0.f, 0.f, Reach), Candidate - FVector(0.f, 0.f, Reach),
		FCollisionObjectQueryParams(ECC_WorldStatic), GroundParams))
	{
		Refusal = 1;
		return false;
	}
	const AActor* Under = Hit.GetActor();
	// What its target stands on is ground for a step to sight, tagged an obstacle or not (a dais, a platform), near their feet.
	const ACharacter* Walker = bTheirFloorToo ? Cast<ACharacter>(Victim) : nullptr;
	const UPrimitiveComponent* TheirBase = Walker ? Walker->GetMovementBase() : nullptr;
	const bool bTheirFloor = TheirBase && Under && TheirBase->GetOwner() == Under
		&& FMath::Abs(static_cast<float>(Hit.ImpactPoint.Z) - FeetZOf(*Victim)) <= TheirFloorBand;
	if (Hit.ImpactNormal.Z < WalkableGroundNormalZ || (Under && Under->ActorHasTag(StandingObstacleTag) && !bTheirFloor))
	{
		Refusal = 2;
		return false;
	}
	const FVector Ground = Hit.ImpactPoint;
	// Room for its body: never inside a wall, a rock, a tree or another creature.
	const FCollisionShape Room = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), HalfHeight);
	if (World->OverlapBlockingTestByChannel(Ground + FVector(0.f, 0.f, HalfHeight + RoomLift * Scale), FQuat::Identity, ECC_Pawn, Room,
		OpenParams))
	{
		Refusal = 3;
		return false;
	}
	// In the open, with its target in sight: never shut in a room or behind a wall from them. Real geometry only: the level's
	// volumes answer world-static traces too (the scatter's PCG volume covers all of Ransom's Rest), so a plain trace saw
	// nothing at all (2026-10-28, the press's play-checks: 0 phase-steps).
	FCollisionQueryParams SightParams = LooterWorld::StaticGeometryParams(World, TEXT("UnpaidPhaseSight"), this);
	SightParams.AddIgnoredActor(Victim);
	if (World->LineTraceTestByObjectType(Ground + FVector(0.f, 0.f, HalfHeight * EyeShare), TargetEye,
		FCollisionObjectQueryParams(ECC_WorldStatic), SightParams))
	{
		Refusal = 4;
		return false;
	}
	// Not into a safe zone (Delia's salt line), nor past the playable area's edge.
	const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this);
	const APlayableArea* Playable = APlayableArea::Find(World);
	if ((Encounters && Encounters->IsInSafeZone(Ground)) || (Playable && !Playable->Contains(Ground)))
	{
		Refusal = 5;
		return false;
	}
	OutGround = Ground;
	return true;
}

void AUnpaidCreature::Arrive()
{
	// Gone from sight: it moves while unseen, and comes back facing its target.
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	double Yaw = GetActorRotation().Yaw;
	// How near them it lands decides its arrival hold (CanStartAttack: a longer beat right by them).
	Kit.LandedGap = TNumericLimits<float>::Max();
	if (const APawn* Victim = GetTarget())
	{
		Yaw = (Victim->GetActorLocation() - Kit.PhaseDestination).Rotation().Yaw;
		Kit.LandedGap = static_cast<float>(FVector::Dist2D(Kit.PhaseDestination, Victim->GetActorLocation()));
	}
	SetActorLocationAndRotation(Kit.PhaseDestination + FVector(0.f, 0.f, HalfHeight + 2.f), FRotator(0.0, Yaw, 0.0), false, nullptr,
		ETeleportType::TeleportPhysics);
	GetCharacterMovement()->StopMovementImmediately();
	Kit.PhaseState = EPhaseState::In;
	Kit.PhaseTime = 0.f;
	ResetProgress();
	// Its shroud comes back hanging still, not swinging in from where it was; and the turn isn't a spin.
	for (FShroudChain& Chain : Chains)
	{
		Chain.Settle();
	}
	LastYaw = static_cast<float>(Yaw);
}

// AGravemotherCreature under the bosses' shared attack rules (round 5; Bosses/BossAttackRules.h): which move she may make
// now in her own order (the roar, the Furrow, the quake, the clod, her stomp, the bite) skipping a third of the same in a
// row, where her target must be for each (its reach, in front of her for a frontal one), her log's clocks, the stomp that
// breaks a run of bites when nothing else is in reach, her moves readied after a long idle stretch, and her hurt zones as
// they stand (what Looter.Boss.HurtZones 1 draws). GravemotherCreatureMoves.cpp starts and strikes the move chosen here.

#include "Creatures/GravemotherCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossAttackRules.h"
#include "Bosses/BossComponent.h"
#include "Creatures/GravemotherClod.h"
#include "Creatures/GravemotherFight.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

namespace
{
	/** ACreatureBase::Strike's own front for a bite (the dot it checks): its wedge is drawn this wide. */
	constexpr float BiteFrontDot = 0.4f;
	/** A stomp or a quake reaches a player no more than this far above or below her middle, past her half height (cm). */
	constexpr float SlamLevel = 150.f;
	/** The quake's ring reaches a player's middle this far past its radius (cm: the player's body), as IsInQuake counts it. */
	constexpr float QuakeBodySlack = 34.f;
	/** A player's body's radius when it isn't a character (cm): a furrow's and a stomp's reach count it. */
	constexpr float DefaultBodyRadius = 40.f;

	float BodyRadius(const APawn* Pawn)
	{
		const ACharacter* Body = Cast<ACharacter>(Pawn);
		return Body && Body->GetCapsuleComponent() ? Body->GetCapsuleComponent()->GetScaledCapsuleRadius() : DefaultBodyRadius;
	}
}

// ---------------------------------------------------------------------------
// Which move she may make
// ---------------------------------------------------------------------------

bool AGravemotherCreature::ChooseMove(EGravemotherMove& OutMove) const
{
	const FBossAttackLog* Log = Boss ? &Boss->AttackLog : nullptr;
	auto Free = [Log](EGravemotherMove Each)
	{
		return !Log || !BossAttackRules::IsBlocked(GravemotherFight::AttackId(Each), *Log);
	};
	// Her phase's tell first, whatever came before: it strikes nobody, so it's never held back.
	if (bRoarWanted)
	{
		OutMove = EGravemotherMove::Roar;
		return true;
	}
	// A furrow from well out; the quake at whoever hugs her; a clod at whoever keeps away; her stomp when the rules call for
	// it; else she closes in to bite. Each as before (its own cooldown, reach and way clear), unless it would be a third.
	if (WantsToFurrow() && Free(EGravemotherMove::Furrow))
	{
		OutMove = EGravemotherMove::Furrow;
		return true;
	}
	if (WantsToQuake() && Free(EGravemotherMove::Quake))
	{
		OutMove = EGravemotherMove::Quake;
		return true;
	}
	if (WantsToClod() && Free(EGravemotherMove::Clod))
	{
		OutMove = EGravemotherMove::Clod;
		return true;
	}
	if (bStompWanted && Free(EGravemotherMove::Stomp))
	{
		OutMove = EGravemotherMove::Stomp;
		return true;
	}
	if (Free(EGravemotherMove::Bite))
	{
		OutMove = EGravemotherMove::Bite;
		return true;
	}
	return false;
}

FBossAttackSpec AGravemotherCreature::SpecFor(EGravemotherMove Which) const
{
	const float Scale = GetSizeScale();
	const FName Id = GravemotherFight::AttackId(Which);
	const float Front = BossAttackRules::FrontDegrees();
	switch (Which)
	{
	case EGravemotherMove::Roar:
		// From anywhere, any way she faces: she rears and turns as she screams.
		return FBossAttackSpec(Id, 0.f, 0.f, 180.f);
	case EGravemotherMove::Furrow:
		// Her line runs at them from her front: the heave reads as hers only when she faces them.
		return FBossAttackSpec(Id, Furrows.MinDistance * Scale, Furrows.Range * Scale, Front);
	case EGravemotherMove::Quake:
		// A ring all round her.
		return FBossAttackSpec(Id, 0.f, QuakeRange * Scale, 180.f);
	case EGravemotherMove::Clod:
		// A lob she turns to aim through her 1.2 s tell.
		return FBossAttackSpec(Id, Clods.MinDistance * Scale, Clods.MaxDistance * Scale, BossAttackRules::AimedDegrees());
	case EGravemotherMove::Stomp:
		// Her forelegs come down in front of her: whoever crowds her there.
		return FBossAttackSpec(Id, 0.f, static_cast<float>(FVector::Dist2D(StompSpot(), GetActorLocation())) + StompRadius * Scale, Front);
	default:
		return FBossAttackSpec(Id, 0.f, GetAttackRange(), Front);
	}
}

bool AGravemotherCreature::IsBusyWithAttack() const
{
	// A furrow outlasts its attack's frame (the plough and her rise after it); a clod in her grip is a move under way.
	return GetCreatureState() == ECreatureState::Attack || FurrowState != EGravemotherFurrow::None || HeldClod.IsValid();
}

// ---------------------------------------------------------------------------
// Her log, the run-breaker, the idle cap
// ---------------------------------------------------------------------------

void AGravemotherCreature::TickAttackRules(float DeltaSeconds)
{
	if (!Boss)
	{
		return;
	}
	FBossAttackLog& Log = Boss->AttackLog;
	const bool bBusy = IsBusyWithAttack();
	if (Log.IsAttacking() && !bBusy)
	{
		Log.NoteEnded();
	}
	const APawn* Victim = GetTarget();
	const bool bFree = Boss->IsFighting() && Victim && !IsDead() && !bReeling && !bBusy;
	Log.Advance(DeltaSeconds, bFree);
	if (!BossAttackRules::IsOn() || !bFree)
	{
		return;
	}
	// Two bites in a row and nothing else she may make (her quake, ready and in reach, would have been chosen): she rears and
	// stomps instead, a tell the player hasn't just seen twice.
	EGravemotherMove Next = EGravemotherMove::Bite;
	if (!bStompWanted && !ChooseMove(Next))
	{
		bStompWanted = true;
		UE_LOG(LogLooter, Verbose, TEXT("%s: two bites in a row; she'll stomp."), *GetName());
	}
	// Too long free without an attack (her moves cooling, her target where none reaches): her ranged moves are ready again,
	// so she lobs or ploughs at them rather than circling.
	if (BossAttackRules::IsOverdue(Log))
	{
		ReadyClod();
		ReadyFurrow();
		if (Pace.bQuake)
		{
			ReadyQuake();
		}
		Log.NoteNudged();
		UE_LOG(LogLooter, Verbose, TEXT("%s: idle %.1f s; her moves are readied."), *GetName(), BossAttackRules::MaxIdle());
	}
}

// ---------------------------------------------------------------------------
// The stomp
// ---------------------------------------------------------------------------

FVector AGravemotherCreature::StompSpot() const
{
	return GetFeet() + GetActorForwardVector().GetSafeNormal2D() * (GetCapsuleComponent()->GetScaledCapsuleRadius() * 1.2f);
}

void AGravemotherCreature::BeginStomp()
{
	// She rears high on her hind legs without a scream (the spider's attack pose through its wind-up), dirt sliding off her
	// forelegs: then the slam.
	Move = EGravemotherMove::Stomp;
	bStompWanted = false;
	AttackWindup = StompWindup;
	AttackRecovery = StompRecovery;
	KickDirt(StompSpot(), 0.35f);
	Voice->Play(LooterSoundCue::SpiderAttack, 1.2f);
	UE_LOG(LogLooter, Verbose, TEXT("%s rears to stomp."), *GetName());
}

void AGravemotherCreature::StompSlam()
{
	const float Scale = GetSizeScale();
	const FVector Spot = StompSpot();
	APawn* Victim = GetTarget();
	bool bHit = false;
	if (IsValidTarget(Victim))
	{
		const FVector Where = Victim->GetActorLocation();
		const bool bInReach = FVector::Dist2D(Where, Spot) <= StompRadius * Scale + BodyRadius(Victim)
			&& FMath::Abs(Where.Z - GetActorLocation().Z) <= GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + SlamLevel;
		// Nothing solid between her and them (a stomp through a wall never lands), as her bite's strike asks.
		bool bClear = true;
		if (bInReach && GetWorld())
		{
			FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(GetWorld(), TEXT("GravemotherStomp"), this, false);
			Params.AddIgnoredActor(Victim);
			bClear = !GetWorld()->LineTraceTestByObjectType(GetActorLocation(), Where, FCollisionObjectQueryParams(ECC_WorldStatic), Params);
		}
		if (bInReach && bClear)
		{
			FVector Away = Where - GetActorLocation();
			Away.Z = 0.0;
			HitWithAttack(Victim, Away.IsNearlyZero() ? GetActorForwardVector() : Away.GetSafeNormal(), StompStrength);
			bHit = true;
			UE_LOG(LogLooter, Log, TEXT("%s's stomp caught %s."), *GetName(), *GetNameSafe(Victim));
		}
	}
	KickDirtRing(Spot, StompRadius * Scale * 0.6f, 5, 0.75f);
	LooterSound::PlayAt(this, LooterSoundCue::GravemotherSlam, Spot, 1.f, 1.1f);
	ShakeFrom(Spot, bHit ? 0.7f : 0.5f, 0.55f);
}

// ---------------------------------------------------------------------------
// Her hurt zones
// ---------------------------------------------------------------------------

void AGravemotherCreature::GatherHurtZones(TArray<FBossHurtZone>& Out) const
{
	if (IsDead())
	{
		return;
	}
	const float Scale = GetSizeScale();
	const FVector Feet = GetFeet();
	const float Half = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float Tall = Half * 2.f + SlamLevel;
	const FName Id = GravemotherFight::AttackId(Move);

	// Her attack's frame: grey through the wind-up, red a moment as it lands. A stagger breaks it off: nothing lands.
	if (GetCreatureState() == ECreatureState::Attack && !bReeling)
	{
		const EBossAttackWindow Window = BossAttackRules::WindowAt(GetStateTime(), AttackWindup, BossAttackRules::ActiveSeconds(), AttackRecovery);
		if (Window == EBossAttackWindow::Windup || Window == EBossAttackWindow::Active)
		{
			switch (Move)
			{
			case EGravemotherMove::Bite:
				Out.Add(FBossHurtZone::Wedge(Id, Window, Feet, GetActorForwardVector(), GetStrikeReach(),
					BossAttackRules::DegreesForDot(BiteFrontDot), Tall));
				break;
			case EGravemotherMove::Quake:
				Out.Add(FBossHurtZone::Disc(Id, Window, Feet, QuakeRadius * Scale + QuakeBodySlack, Tall));
				break;
			case EGravemotherMove::Stomp:
				Out.Add(FBossHurtZone::Disc(Id, Window, StompSpot(), StompRadius * Scale + DefaultBodyRadius, Tall));
				break;
			default:
				// The roar strikes nobody; the furrow's and the clod's zones are their own (below).
				break;
			}
		}
	}

	// Her furrow: its line grey through the heave once she has her aim, red ahead of her while she ploughs.
	const float FurrowReach = GetCapsuleComponent()->GetScaledCapsuleRadius() + DefaultBodyRadius + Furrows.HitMargin * Scale;
	const FName FurrowId = GravemotherFight::AttackId(EGravemotherMove::Furrow);
	const FVector LineStart(FurrowStart.X, FurrowStart.Y, Feet.Z);
	const FVector LineEnd = LineStart + FurrowDirection.GetSafeNormal2D() * FurrowLength;
	if (FurrowState == EGravemotherFurrow::Heave && bAimTaken)
	{
		Out.Add(FBossHurtZone::Lane(FurrowId, EBossAttackWindow::Windup, LineStart, LineEnd, FurrowReach, Tall));
	}
	else if (FurrowState == EGravemotherFurrow::Plough && !bFurrowHit)
	{
		Out.Add(FBossHurtZone::Lane(FurrowId, EBossAttackWindow::Active, Feet, LineEnd, FurrowReach, Tall));
	}

	// A clod in flight: where its shadow grows, grey, then red as it comes down.
	if (const AGravemotherClod* Thrown = ThrownClod.Get(); Thrown && Thrown->IsFlying())
	{
		const float Left = Thrown->GetPlannedSeconds() - Thrown->GetFlightTime();
		const EBossAttackWindow Window = Left <= BossAttackRules::ActiveSeconds() ? EBossAttackWindow::Active : EBossAttackWindow::Windup;
		Out.Add(FBossHurtZone::Disc(GravemotherFight::AttackId(EGravemotherMove::Clod), Window, Thrown->GetPlannedLanding(), Clods.BurstRadius, Tall));
	}
}

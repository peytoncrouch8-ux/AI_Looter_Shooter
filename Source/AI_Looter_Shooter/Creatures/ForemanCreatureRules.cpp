// AForemanCreature's choice of move under the bosses' shared attack rules (round 5; Bosses/BossAttackRules.h): his
// entrance, the roof, a fuse, his stamp or the pick handle, in his own order, skipping a third of the same in a row; where
// his target must be for each (its reach, in front of him for the handle); his log's clocks; the stamp that breaks a run of
// swings when nothing else is in reach; a fuse readied after a long idle stretch; and his hurt zones as they stand (what
// Looter.Boss.HurtZones 1 draws). Moved here from ForemanCreature.cpp with the rules, to keep that under the size guide.

#include "Creatures/ForemanCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossAttackRules.h"
#include "Bosses/BossCameraShake.h"
#include "Bosses/BossComponent.h"
#include "Creatures/ForemanFuseStick.h"
#include "Creatures/ForemanRockFall.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

namespace
{
	/** His entrance and the roof start from anywhere they're wanted: they strike nobody where he stands. */
	constexpr float FromAnywhere = 1.0e6f;

	/** The handle's stamp comes down this far in front of his feet (cm at an Unpaid's size), where his entrance's slam does. */
	constexpr float StampAhead = 110.f;

	/** A stamp reaches a player this far above or below his middle at most (cm, at size 1 and up), as his swing's blow does. */
	constexpr float StampMaxRise = 220.f;

	/** Dust shaken off the roof by a stamp: fewer spots than his entrance's slam. */
	constexpr int32 StampDustSpots = 3;

	/** A player's body's radius when it isn't a character (cm). */
	constexpr float DefaultBodyRadius = 40.f;

	float BodyRadius(const APawn* Pawn)
	{
		const ACharacter* Body = Cast<ACharacter>(Pawn);
		return Body && Body->GetCapsuleComponent() ? Body->GetCapsuleComponent()->GetScaledCapsuleRadius() : DefaultBodyRadius;
	}
}

// ---------------------------------------------------------------------------
// Which move
// ---------------------------------------------------------------------------

FName AForemanCreature::AttackId(EForemanMove Which)
{
	static const FName Swing(TEXT("Swing"));
	static const FName Intro(TEXT("Intro"));
	static const FName Fuse(TEXT("Fuse"));
	static const FName Roof(TEXT("Roof"));
	static const FName Stamp(TEXT("Stamp"));
	switch (Which)
	{
	case EForemanMove::Intro: return Intro;
	case EForemanMove::Fuse: return Fuse;
	case EForemanMove::Roof: return Roof;
	case EForemanMove::Stamp: return Stamp;
	default: return Swing;
	}
}

bool AForemanCreature::ChooseMove(EForemanMove& OutMove) const
{
	const FBossAttackLog* Log = Boss ? &Boss->AttackLog : nullptr;
	auto Free = [Log](EForemanMove Each)
	{
		return !Log || !BossAttackRules::IsBlocked(AttackId(Each), *Log);
	};
	// His entrance first (it hurts nobody: never held back); the roof when it's due; a fuse at a player who keeps off; his
	// stamp when the rules call for it; else he closes in with the pick handle. Each unless it would be a third in a row.
	if (bIntroWanted)
	{
		OutMove = EForemanMove::Intro;
		return true;
	}
	if (WantsRoof() && Free(EForemanMove::Roof))
	{
		OutMove = EForemanMove::Roof;
		return true;
	}
	if (WantsFuse() && Free(EForemanMove::Fuse))
	{
		OutMove = EForemanMove::Fuse;
		return true;
	}
	if (bStampWanted && Free(EForemanMove::Stamp))
	{
		OutMove = EForemanMove::Stamp;
		return true;
	}
	if (Free(EForemanMove::Swing))
	{
		OutMove = EForemanMove::Swing;
		return true;
	}
	return false;
}

FBossAttackSpec AForemanCreature::SpecFor(EForemanMove Which) const
{
	const float Scale = GetSizeScale();
	const FName Id = AttackId(Which);
	const float Front = BossAttackRules::FrontDegrees();
	switch (Which)
	{
	case EForemanMove::Intro:
	case EForemanMove::Roof:
		// His entrance turns him to the player; the roof's blow goes at his prop (he turns to it himself).
		return FBossAttackSpec(Id, 0.f, 0.f, 180.f);
	case EForemanMove::Fuse:
		// A lob he aims through the fuse's 2 s hiss.
		return FBossAttackSpec(Id, FuseMinDistance * Scale, FuseMaxDistance * Scale, BossAttackRules::AimedDegrees());
	case EForemanMove::Stamp:
		return FBossAttackSpec(Id, 0.f, (StampAhead + StampRadius) * Scale, Front);
	default:
		return FBossAttackSpec(Id, 0.f, GetAttackRange(), Front);
	}
}

float AForemanCreature::GetAttackStartRange() const
{
	if (bStaggeredNow)
	{
		return 0.f;
	}
	// His entrance and the roof from anywhere; a fuse at a player who keeps off; his stamp and the pick handle up close.
	// None he may make now: he starts nothing (he closes in, or squares up, until one comes).
	EForemanMove Next = EForemanMove::Swing;
	if (!ChooseMove(Next))
	{
		return 0.f;
	}
	switch (Next)
	{
	case EForemanMove::Intro:
	case EForemanMove::Roof:
		return FromAnywhere;
	case EForemanMove::Fuse:
		return FuseMaxDistance * GetSizeScale();
	case EForemanMove::Stamp:
		return SpecFor(EForemanMove::Stamp).MaxReach;
	default:
		return Super::GetAttackStartRange();
	}
}

bool AForemanCreature::CanStartAttack() const
{
	if (bStaggeredNow || HeldStick.IsValid() || !Super::CanStartAttack())
	{
		return false;
	}
	// The shared rules: a breath after his last attack, and his target where the move he'd make wants them (in front of him
	// for the handle: else he turns to them first).
	const APawn* Victim = GetTarget();
	if (!Boss || !Victim)
	{
		return true;
	}
	EForemanMove Next = EForemanMove::Swing;
	return ChooseMove(Next) && BossAttackRules::IsGapOpen(Boss->AttackLog)
		&& BossAttackRules::IsPlaced(SpecFor(Next), GetActorForwardVector(), Victim->GetActorLocation() - GetActorLocation());
}

bool AForemanCreature::TracksTargetInWindup() const
{
	// The roof's blow is aimed at his prop, not the player: he turns to it himself (FaceStruckProp).
	return Move != EForemanMove::Roof;
}

void AForemanCreature::OnAttackStarted()
{
	// The same attack, several ways, each with its own timing: the entrance, the roof, a fuse, the stamp, or the pick handle.
	EForemanMove Next = EForemanMove::Swing;
	ChooseMove(Next);
	switch (Next)
	{
	case EForemanMove::Intro:
		BeginIntro();
		break;
	case EForemanMove::Roof:
		BeginRoof();
		break;
	case EForemanMove::Fuse:
		BeginFuse();
		break;
	case EForemanMove::Stamp:
		BeginStamp();
		break;
	default:
		Move = EForemanMove::Swing;
		// The dead miners' heave and its timing at his pace.
		Super::OnAttackStarted();
		break;
	}
	if (Boss)
	{
		Boss->AttackLog.NoteStarted(AttackId(Move));
	}
}

void AForemanCreature::Strike()
{
	if (bStaggeredNow)
	{
		return;
	}
	switch (Move)
	{
	case EForemanMove::Intro:
		IntroSlam();
		return;
	case EForemanMove::Fuse:
		LobFuse();
		return;
	case EForemanMove::Roof:
		RoofStrike();
		return;
	case EForemanMove::Stamp:
		StampSlam();
		return;
	default:
		break;
	}
	Super::Strike();
}

// ---------------------------------------------------------------------------
// His log, the run-breaker, the idle cap
// ---------------------------------------------------------------------------

void AForemanCreature::TickAttackRules(float DeltaSeconds)
{
	ThrownSticks.RemoveAll([](const TWeakObjectPtr<AForemanFuseStick>& Each) { return !Each.IsValid(); });
	if (!Boss)
	{
		return;
	}
	FBossAttackLog& Log = Boss->AttackLog;
	const bool bBusy = GetCreatureState() == ECreatureState::Attack || HeldStick.IsValid();
	if (Log.IsAttacking() && !bBusy)
	{
		Log.NoteEnded();
	}
	const bool bFree = Boss->IsFighting() && GetTarget() && !IsDead() && !bStaggeredNow && !bBusy;
	Log.Advance(DeltaSeconds, bFree);
	if (!BossAttackRules::IsOn() || !bFree)
	{
		return;
	}
	// Two swings in a row and nothing else he may make: the handle raised two-handed and stamped down instead.
	EForemanMove Next = EForemanMove::Swing;
	if (!bStampWanted && !ChooseMove(Next))
	{
		bStampWanted = true;
		UE_LOG(LogLooter, Verbose, TEXT("%s: two swings in a row; he'll stamp."), *GetName());
	}
	// Too long free without an attack: a fuse is ready again (it reaches a player who keeps off; up close, the handle comes).
	if (BossAttackRules::IsOverdue(Log))
	{
		ReadyFuse();
		Log.NoteNudged();
		UE_LOG(LogLooter, Verbose, TEXT("%s: idle %.1f s; a fuse readied."), *GetName(), BossAttackRules::MaxIdle());
	}
}

// ---------------------------------------------------------------------------
// The stamp
// ---------------------------------------------------------------------------

FVector AForemanCreature::StampSpot() const
{
	return GroundUnder(GetActorLocation() + GetActorForwardVector().GetSafeNormal2D() * (StampAhead * GetSizeScale()));
}

void AForemanCreature::BeginStamp()
{
	// The handle comes up two-handed over his head (his entrance's pose, his lamp flaring), with no line: then he drives it down.
	Move = EForemanMove::Stamp;
	bStampWanted = false;
	AttackWindup = StampWindup;
	AttackRecovery = StampRecovery;
	SetAct(EAct::None);
	LooterSound::PlayAttached(LooterSoundCue::DeadMiner::Heave, GetRootComponent(), NAME_None, 1.1f, 0.85f * Voice->GetPitch());
	UE_LOG(LogLooter, Verbose, TEXT("%s raises the handle to stamp."), *GetName());
}

void AForemanCreature::StampSlam()
{
	const float Scale = GetSizeScale();
	const FVector Spot = StampSpot();
	APawn* Victim = GetTarget();
	bool bHit = false;
	if (IsValidTarget(Victim))
	{
		const FVector Where = Victim->GetActorLocation();
		const bool bInReach = FVector::Dist2D(Where, Spot) <= StampRadius * Scale + BodyRadius(Victim)
			&& FMath::Abs(Where.Z - GetActorLocation().Z) <= StampMaxRise * FMath::Max(Scale, 1.f);
		// Nothing solid between him and them: no stamp through the barricade.
		bool bClear = true;
		if (bInReach && GetWorld())
		{
			FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(GetWorld(), TEXT("ForemanStamp"), this, false);
			Params.AddIgnoredActor(Victim);
			bClear = !GetWorld()->LineTraceTestByObjectType(GetActorLocation(), Where, FCollisionObjectQueryParams(ECC_WorldStatic), Params);
		}
		if (bInReach && bClear)
		{
			FVector Away = Where - GetActorLocation();
			Away.Z = 0.0;
			HitWithAttack(Victim, Away.IsNearlyZero() ? GetActorForwardVector() : Away.GetSafeNormal(), StampStrength);
			bHit = true;
			UE_LOG(LogLooter, Log, TEXT("%s's stamp caught %s."), *GetName(), *GetNameSafe(Victim));
		}
	}
	// The handle in the floor: dirt thrown up round it, dust off the roof, the view jolted.
	KickDirt(Spot, 0.9f);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FVector Out = FVector::ForwardVector.RotateAngleAxis(45.f + 90.f * Index, FVector::UpVector);
		KickDirt(Spot + Out * (StampRadius * 0.5f * Scale), 0.5f);
	}
	ShakeRoofDust(StampDustSpots);
	LooterSound::PlayAt(this, LooterSoundCue::Foreman::Slam, Spot, 0.9f, 1.08f);
	BossCameraShake::Kick(this, Spot, bHit ? 0.5f : 0.35f, 0.5f);
}

// ---------------------------------------------------------------------------
// His hurt zones
// ---------------------------------------------------------------------------

void AForemanCreature::GatherHurtZones(TArray<FBossHurtZone>& Out) const
{
	if (IsDead())
	{
		return;
	}
	const float Scale = GetSizeScale();
	const FVector Feet = GetFeet();
	const float Tall = StampMaxRise * FMath::Max(Scale, 1.f);
	const float Active = BossAttackRules::ActiveSeconds();

	// His attack's frame: grey through the heave, red a moment as the blow lands. A stagger breaks it off.
	if (GetCreatureState() == ECreatureState::Attack && !bStaggeredNow)
	{
		const EBossAttackWindow Window = BossAttackRules::WindowAt(GetStateTime(), AttackWindup, Active, AttackRecovery);
		if (Window == EBossAttackWindow::Windup || Window == EBossAttackWindow::Active)
		{
			if (Move == EForemanMove::Swing)
			{
				Out.Add(FBossHurtZone::Wedge(AttackId(Move), Window, Feet, GetActorForwardVector(), Tuning.SwingReach * Scale,
					BossAttackRules::DegreesForDot(Tuning.SwingFront), Tall));
			}
			else if (Move == EForemanMove::Stamp)
			{
				Out.Add(FBossHurtZone::Disc(AttackId(Move), Window, StampSpot(), StampRadius * Scale + DefaultBodyRadius, Tall));
			}
			// His entrance strikes nobody; a fuse's and the roof's zones are where they come down (below).
		}
	}

	// His sticks on the floor: their ring grey while the fuse burns, red as they burst.
	const FName FuseId = AttackId(EForemanMove::Fuse);
	for (const TWeakObjectPtr<AForemanFuseStick>& Each : ThrownSticks)
	{
		const AForemanFuseStick* Stick = Each.Get();
		if (!Stick)
		{
			continue;
		}
		if (Stick->GetStage() == AForemanFuseStick::EStage::Landed)
		{
			const EBossAttackWindow Window = Stick->GetFuseLeft() <= Active ? EBossAttackWindow::Active : EBossAttackWindow::Windup;
			Out.Add(FBossHurtZone::Disc(FuseId, Window, Stick->GetLanding(), Stick->Radius, Tall));
		}
		else if (Stick->GetStage() == AForemanFuseStick::EStage::Burst && Stick->GetBurstAge() < Active)
		{
			Out.Add(FBossHurtZone::Disc(FuseId, EBossAttackWindow::Active, Stick->GetLanding(), Stick->Radius, Tall));
		}
	}

	// The roof's marked spots: grey while the dust sifts, red as the rock drops.
	const FName RoofId = AttackId(EForemanMove::Roof);
	for (const TWeakObjectPtr<AForemanRockFall>& Each : RockFalls)
	{
		const AForemanRockFall* Fall = Each.Get();
		if (!Fall || Fall->HasFallen())
		{
			continue;
		}
		const EBossAttackWindow Window = Fall->GetTimeToFall() <= AForemanRockFall::DropSeconds ? EBossAttackWindow::Active : EBossAttackWindow::Windup;
		Out.Add(FBossHurtZone::Disc(RoofId, Window, Fall->GetSpot(), Fall->GetRadius(), Tall));
	}
}

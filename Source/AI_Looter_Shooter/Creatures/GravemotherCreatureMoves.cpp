// AGravemotherCreature's boss fight (GravemotherFight has its phases and pace): which of her attacks she makes (her one
// attack, wound up, struck and recovered, each move with its own timing), her roar, the Gravequake and her venom; her reel
// when her boss is staggered; her fury's burning cracks; and her boss's events. GravemotherCreatureCharge.cpp is her charge.

#include "Creatures/GravemotherCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossCameraShake.h"
#include "Bosses/BossComponent.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/GroundCrack.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

namespace
{
	/** A roar starts wherever its target is: it strikes nobody. */
	constexpr float RoarFromAnywhere = 1.0e6f;

	/** The spit's glow at her fangs grows to this many times a pellet's size. */
	constexpr float SpitGlowSize = 2.2f;

	/** The quake's ring of cracks, this many times as wide as a charge's crack. */
	constexpr float QuakeCrackWidth = 1.3f;

	/** After the quake's slam its ring closes over this long (s); a broken-off quake's at once. */
	constexpr float QuakeCrackLingers = 1.2f;

	/** A quake reaches a player no more than this far above or below her middle, past her half height (cm). */
	constexpr float QuakeLevel = 150.f;

	/** A later phase's quake comes no sooner than this after the phase starts (s): her roar first. */
	constexpr float QuakeAfterPhase = 3.f;

	/**
	 * Her sink as she reels: her body pushed down as a hit from this high over her pushes it, the point swaying this far
	 * side to side this fast (radians a second), so she trembles as she sinks.
	 */
	constexpr float SinkAbove = 1000.f;
	constexpr float SinkSway = 140.f;
	constexpr float SinkSwaySpeed = 18.f;

	/** A burning crack's seam hurts a player this near it (cm, past half its width and their body). */
	constexpr float BurnReach = 20.f;
}

// ---------------------------------------------------------------------------
// Which attack
// ---------------------------------------------------------------------------

float AGravemotherCreature::GetAttackStartRange() const
{
	if (bReeling)
	{
		return 0.f;
	}
	// A roar from anywhere; her charge from well out; the quake at whoever hugs her; venom at whoever keeps away; else she
	// closes in to bite.
	if (bRoarWanted)
	{
		return RoarFromAnywhere;
	}
	const float Scale = GetSizeScale();
	if (WantsToCharge())
	{
		return ChargeRange * Scale;
	}
	if (WantsToQuake())
	{
		return QuakeRange * Scale;
	}
	if (WantsToSpit())
	{
		return SpitMaxDistance * Scale;
	}
	return Super::GetAttackStartRange();
}

bool AGravemotherCreature::CanStartAttack() const
{
	return !bReeling && ChargeState == EGravemotherCharge::None && Super::CanStartAttack();
}

void AGravemotherCreature::OnAttackStarted()
{
	// The same attack, several ways, each with its own timing: the roar, a charge, the quake, the spit, or her bite.
	if (bRoarWanted)
	{
		BeginRoar();
	}
	else if (WantsToCharge())
	{
		BeginTelegraph();
	}
	else if (WantsToQuake())
	{
		BeginQuake();
	}
	else if (WantsToSpit())
	{
		BeginSpit();
	}
	else
	{
		Move = EGravemotherMove::Bite;
		AttackWindup = BiteWindup;
		AttackRecovery = BiteRecovery;
	}
	Super::OnAttackStarted();
}

void AGravemotherCreature::Strike()
{
	// Broken off by a stagger: nothing lands.
	if (bReeling)
	{
		return;
	}
	switch (Move)
	{
	case EGravemotherMove::Roar:
		RoarPeak();
		return;
	case EGravemotherMove::Quake:
		QuakeSlam();
		return;
	case EGravemotherMove::Spit:
		SpitRelease();
		return;
	case EGravemotherMove::Charge:
		if (ChargeState == EGravemotherCharge::Telegraph)
		{
			BeginDash();
		}
		return;
	default:
		break;
	}
	// A bite.
	Super::Strike();
}

bool AGravemotherCreature::IsInQuake(const FVector& Where) const
{
	const float Scale = GetSizeScale();
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float Reach = QuakeRadius * Scale + 34.f;
	return FVector::Dist2D(Where, GetActorLocation()) <= Reach
		&& FMath::Abs(Where.Z - GetActorLocation().Z) <= Capsule->GetScaledCapsuleHalfHeight() + QuakeLevel;
}

bool AGravemotherCreature::WantsToQuake() const
{
	const APawn* Victim = GetTarget();
	if (!Pace.bQuake || QuakeCooldownLeft > 0.f || !Victim || IsDead())
	{
		return false;
	}
	return FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation()) <= QuakeRange * GetSizeScale()
		&& IsInQuake(Victim->GetActorLocation());
}

bool AGravemotherCreature::WantsToSpit() const
{
	const APawn* Victim = GetTarget();
	// Her venom flies at her fight's player (her boss's volley): only while her fight is on.
	if (SpitCooldownLeft > 0.f || !Victim || IsDead() || !Boss || !Boss->IsFighting() || Boss->IsVolleyWindingUp())
	{
		return false;
	}
	const float Scale = GetSizeScale();
	const float Distance = static_cast<float>(FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation()));
	return Distance >= SpitMinDistance * Scale && Distance <= SpitMaxDistance * Scale;
}

// ---------------------------------------------------------------------------
// The roar
// ---------------------------------------------------------------------------

void AGravemotherCreature::BeginRoar()
{
	// She rears (the spider's attack pose through its wind-up) and screams, then slams her forelegs down.
	Move = EGravemotherMove::Roar;
	bRoarWanted = false;
	AttackWindup = RoarWindup;
	AttackRecovery = RoarRecovery;
	LooterSound::PlayAttached(LooterSoundCue::GravemotherRoar, GetRootComponent());
	KickDirt(GetFeet() - GetActorForwardVector() * GetCapsuleComponent()->GetScaledCapsuleRadius(), 0.5f);
	UE_LOG(LogLooter, Verbose, TEXT("%s rears and screams."), *GetName());
}

void AGravemotherCreature::RoarPeak()
{
	const float Scale = GetSizeScale();
	const FVector Slam = GetFeet() + GetActorForwardVector() * (GetCapsuleComponent()->GetScaledCapsuleRadius() * 1.2f);
	KickDirtRing(Slam, 90.f * Scale, 5, 0.7f);
	LooterSound::PlayAt(this, LooterSoundCue::GravemotherSlam, Slam);
	ShakeFrom(Slam, 0.55f, 0.7f);
}

// ---------------------------------------------------------------------------
// The Gravequake
// ---------------------------------------------------------------------------

void AGravemotherCreature::BeginQuake()
{
	// She rears high while a ring of cracks spreads out under her to the quake's reach (the time to get out of it).
	Move = EGravemotherMove::Quake;
	AttackWindup = QuakeWindup;
	AttackRecovery = QuakeRecovery;
	const float Scale = GetSizeScale();
	const FVector Feet = GetFeet();
	if (AGroundCrack* Old = QuakeCrack.Get())
	{
		Old->Close(0.f);
	}
	AGroundCrack* Ring = AGroundCrack::Spawn(GetWorld(), Feet, Feet + GetActorForwardVector(), ChargeCrackWidth * Scale * QuakeCrackWidth,
		UCreatureRankSettings::Get(GetRank()).Color, FMath::Rand());
	if (Ring)
	{
		Ring->BurstSeconds = QuakeWindup;
		Ring->Burst(Feet, QuakeRadius * Scale);
	}
	QuakeCrack = Ring;
	LooterSound::PlayAttached(LooterSoundCue::GravemotherQuake, GetRootComponent());
	KickDirtRing(Feet, QuakeRadius * Scale * 0.5f, 4, 0.3f);
	UE_LOG(LogLooter, Verbose, TEXT("%s rears for the Gravequake."), *GetName());
}

void AGravemotherCreature::QuakeSlam()
{
	const float Scale = GetSizeScale();
	const FVector Feet = GetFeet();
	QuakeCooldownLeft = Pace.QuakeCooldown;
	// Whoever is still inside the ring is thrown out of it.
	APawn* Victim = GetTarget();
	bool bHit = false;
	if (IsValidTarget(Victim) && IsInQuake(Victim->GetActorLocation()))
	{
		FVector Away = Victim->GetActorLocation() - GetActorLocation();
		Away.Z = 0.0;
		HitWithAttack(Victim, Away.IsNearlyZero() ? GetActorForwardVector() : Away.GetSafeNormal(), QuakeStrength);
		bHit = true;
		UE_LOG(LogLooter, Log, TEXT("%s's Gravequake caught %s."), *GetName(), *GetNameSafe(Victim));
	}
	if (AGroundCrack* Ring = QuakeCrack.Get())
	{
		Ring->Close(QuakeCrackLingers);
	}
	QuakeCrack = nullptr;
	KickDirt(Feet, 1.f);
	KickDirtRing(Feet, QuakeRadius * Scale * 0.85f, 8, 0.8f);
	LooterSound::PlayAt(this, LooterSoundCue::GravemotherSlam, Feet, 1.2f, 0.85f);
	ShakeFrom(Feet, bHit ? 0.9f : 0.65f, 0.8f);
}

// ---------------------------------------------------------------------------
// Her venom
// ---------------------------------------------------------------------------

void AGravemotherCreature::BeginSpit()
{
	// She rears with the venom glowing up at her fangs: the tell to get behind something.
	Move = EGravemotherMove::Spit;
	AttackWindup = SpitWindup;
	AttackRecovery = SpitRecovery;
	const FBossVolley Volley = GravemotherFight::Spit(Pace.SpitPellets, Pace.SpitSpread);
	UWorld* World = GetWorld();
	if (UEnemyProjectileSubsystem* Shots = World ? World->GetSubsystem<UEnemyProjectileSubsystem>() : nullptr)
	{
		Shots->ShowCharge(this, Volley.Muzzle, SpitWindup, Volley.Radius * SpitGlowSize, Volley.Color);
	}
	LooterSound::PlayAttached(LooterSoundCue::GravemotherGurgle, GetRootComponent());
}

void AGravemotherCreature::SpitRelease()
{
	SpitCooldownLeft = Pace.SpitCooldown;
	if (!Boss || !Boss->IsFighting())
	{
		return;
	}
	const FBossVolley Volley = GravemotherFight::Spit(Pace.SpitPellets, Pace.SpitSpread);
	Boss->FireVolley(Volley);
	LooterSound::PlayAt(this, LooterSoundCue::GravemotherSpit, GetActorTransform().TransformPosition(Volley.Muzzle));
}

// ---------------------------------------------------------------------------
// Her moves' clocks, the reel, the burning cracks
// ---------------------------------------------------------------------------

void AGravemotherCreature::TickMoves(float DeltaSeconds)
{
	// Their cooldowns run while she's after someone, not while she dozes in her den.
	if (GetTarget())
	{
		QuakeCooldownLeft = FMath::Max(0.f, QuakeCooldownLeft - DeltaSeconds);
		SpitCooldownLeft = FMath::Max(0.f, SpitCooldownLeft - DeltaSeconds);
	}
	// A quake broken off (a stagger, her death): its ring closes.
	if (Move == EGravemotherMove::Quake && GetCreatureState() != ECreatureState::Attack)
	{
		if (AGroundCrack* Ring = QuakeCrack.Get())
		{
			Ring->Close(0.f);
		}
		QuakeCrack = nullptr;
	}
	TickBurn(DeltaSeconds);
	// Reeling, or her forelegs in the dirt after a charge that missed: sunk and trembling, open to punishment.
	SinkClock += DeltaSeconds;
	if (!IsDead() && (bReeling || (ChargeState == EGravemotherCharge::Recover && !bChargeHit)))
	{
		Sink();
	}
}

void AGravemotherCreature::Sink()
{
	// The spider's own flinch, from a point high over her (swaying side to side): her body pushed down and rocked on her
	// planted legs, every frame while it lasts. Her brood calls are her own OnHurt's, so the spider's is called straight.
	const FVector Above = GetActorLocation() + GetActorRightVector() * (FMath::Sin(SinkClock * SinkSwaySpeed) * SinkSway)
		+ FVector(0.0, 0.0, SinkAbove);
	ASpiderCreature::OnHurt(true, Above);
}

void AGravemotherCreature::TickBurn(float DeltaSeconds)
{
	AGroundCrack* Burning = BurningCrack.Get();
	if (!Burning || BurnLeft <= 0.f || IsDead())
	{
		BurnLeft = 0.f;
		BurningCrack = nullptr;
		return;
	}
	BurnLeft -= DeltaSeconds;
	BurnTick -= DeltaSeconds;
	if (BurnTick > 0.f)
	{
		return;
	}
	BurnTick = BurnEvery;
	APawn* Victim = GetTarget();
	if (!IsValidTarget(Victim))
	{
		return;
	}
	// Standing on the seam: along the crack's line, within its width and their body, on its level.
	const FVector Where = Victim->GetActorLocation();
	const FVector Start = Burning->GetStart();
	const FVector End = Burning->GetEnd();
	const FVector2D Nearest = FMath::ClosestPointOnSegment2D(FVector2D(Where.X, Where.Y), FVector2D(Start.X, Start.Y), FVector2D(End.X, End.Y));
	const ACharacter* Body = Cast<ACharacter>(Victim);
	const float Half = Body ? Body->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 88.f;
	const float Radius = Body ? Body->GetCapsuleComponent()->GetScaledCapsuleRadius() : 34.f;
	const float Reach = ChargeCrackWidth * GetSizeScale() * 0.5f + Radius + BurnReach;
	const bool bLevel = FMath::Abs((Where.Z - Half) - FMath::Min(Start.Z, End.Z)) <= 120.f;
	if (bLevel && FVector2D::Distance(FVector2D(Where.X, Where.Y), Nearest) <= Reach)
	{
		HitWithAttack(Victim, FVector::ZeroVector, BurnStrength);
		LooterSound::PlayAt(this, LooterSoundCue::GravemotherCrackBurn, FVector(Nearest.X, Nearest.Y, Where.Z - Half));
	}
}

void AGravemotherCreature::BeginReel()
{
	bReeling = true;
	SinkClock = 0.f;
	// Whatever she was at is broken off: its strike never lands (Strike does nothing while she reels) and her attack ends at
	// its next frame (its wind-up and recovery cut short), so a charge's telegraph or dash stops where it is.
	if (GetCreatureState() == ECreatureState::Attack)
	{
		AttackWindup = FMath::Min(AttackWindup, 0.05f);
		AttackRecovery = 0.05f;
	}
	if (AGroundCrack* Ring = QuakeCrack.Get())
	{
		Ring->Close(0.f);
	}
	QuakeCrack = nullptr;
	// Held still while she reels (her pace's chase is nothing).
	ApplyPace(PacePhase);
	Voice->Play(LooterSoundCue::SpiderHurt, 1.3f);
	KickDirtRing(GetFeet(), GetCapsuleComponent()->GetScaledCapsuleRadius(), 4, 0.5f);
	UE_LOG(LogLooter, Log, TEXT("%s reels, staggered."), *GetName());
}

void AGravemotherCreature::EndReel()
{
	bReeling = false;
	ApplyPace(PacePhase);
}

void AGravemotherCreature::ApplyPace(int32 Phase)
{
	PacePhase = FMath::Clamp(Phase, 0, 2);
	Pace = GravemotherFight::PaceFor(PacePhase);
	if (!bOwnPaceCaptured)
	{
		return;
	}
	ChargeCooldown = OwnChargeCooldown * Pace.ChargeCooldownScale;
	ChargeTelegraph = OwnChargeTelegraph * Pace.ChargeTelegraphScale;
	ChargeAimSeconds = OwnChargeAim * Pace.ChargeTelegraphScale;
	// Held still while she reels.
	ChaseSpeed = bReeling ? 0.f : OwnChaseSpeed * Pace.ChaseScale;
}

// ---------------------------------------------------------------------------
// Her boss's events
// ---------------------------------------------------------------------------

void AGravemotherCreature::HandleFightStarted()
{
	// Her bar sweeps in as she rears and screams (her next attack).
	bRoarWanted = true;
}

void AGravemotherCreature::HandleFightReset()
{
	// The player fell: she's home and healed (her boss's reset; her brood went with it).
	bRoarWanted = false;
	bReeling = false;
	ApplyPace(0);
}

void AGravemotherCreature::HandlePhaseChanged(int32 NewPhase, int32 OldPhase)
{
	ApplyPace(NewPhase);
	if (OldPhase != INDEX_NONE && NewPhase > OldPhase)
	{
		// The phase's tell: she screams as her brood claws up round her; the quake comes after it.
		bRoarWanted = true;
		QuakeCooldownLeft = FMath::Max(QuakeCooldownLeft, QuakeAfterPhase);
	}
}

void AGravemotherCreature::HandleStaggered(bool bStaggered)
{
	if (bStaggered)
	{
		BeginReel();
	}
	else
	{
		EndReel();
	}
}

void AGravemotherCreature::ShakeFrom(const FVector& Where, float Strength, float Seconds) const
{
	BossCameraShake::Kick(this, Where, Strength, Seconds);
}

void AGravemotherCreature::KickDirtRing(const FVector& Center, float Radius, int32 Count, float Strength)
{
	KickDirt(Center, Strength);
	const float Start = FMath::FRandRange(0.f, 360.f);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector Out = FVector::ForwardVector.RotateAngleAxis(Start + 360.f * static_cast<float>(Index) / static_cast<float>(FMath::Max(Count, 1)),
			FVector::UpVector);
		KickDirt(Center + Out * Radius, Strength * 0.7f);
	}
}

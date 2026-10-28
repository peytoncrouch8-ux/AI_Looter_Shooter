// AUnpaidCreature's fighting kit (round 2, 2026-10-27; the user: "Unpaid have terrible ai, need better attacks and targeting
// sequences", "most mobs are not frightening enough as enemies, gotta raise the stakes!"). Every number is a Looter.Unpaid.*
// cvar (FUnpaidFightTuning); the rules are plain functions (UnpaidFightRules). Fair: each move has a tell the player can read
// and answer (the coal flaring through every wind-up, an arm drawn back, a glow swelling at its chest, a whisper from where
// it will come back, a scream before the pack comes).
//  - Its attack (OnAttackStarted, its turn from the fight director): close in, a rake (two swipes, one arm then the other,
//    drifting in); else the lunge, aimed where the target will be (UnpaidCreatureAttack.cpp).
//  - Its turn, while it's in front of the player's eyes: now and then it fades and comes back at their side or behind them
//    (the phase-step's fade, UnpaidCreaturePhase.cpp), a whisper heard from there first.
//  - Each frame: its tells (the coal), its turn's flank, its look at the ring, its pace.
//  - Waiting on the ring: cinders and the rally scream (UnpaidCreatureRing.cpp).
//  - Its pace: farther off it closes in faster, and faster still in its pack's frenzy (the director's chase boost).
// Kinds on its rig with attacks of their own (miners, the Bell-Ringer, Abel) and a boss's adds keep the old way.

#include "Creatures/UnpaidCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossComponent.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/FightRules.h"
#include "Creatures/PackRules.h"
#include "Creatures/UnpaidFightRules.h"
#include "Creatures/UnpaidFightTuning.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** It looks at its choices on the ring this often (s, jittered by up to ThinkJitter): not every frame. */
	constexpr float ThinkSeconds = 0.3f;
	constexpr float ThinkJitter = 0.2f;

	/** The rake's drift in carries on this long (s) past the second swipe. */
	constexpr float RakeDriftTail = 0.12f;

	/** A swipe reaches up or down this far (cm, at size 1 or more), as the lunge does. */
	constexpr float ReachUpDown = 220.f;

	/** The rally's scream rings its pack's coals this bright while the frenzy lasts (a share of the tell's flare). */
	constexpr float FrenzyGlowShare = 0.8f;

	/** A flank's whisper is heard from this high over the spot it will come back on (cm): about where its head will be. */
	constexpr float FlankWhisperHeight = 130.f;

	float KitEase(float Alpha)
	{
		return FMath::InterpEaseInOut(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f), 2.f);
	}

	/** Nothing solid between its middle and Victim's (or their feet): as a bite's strike asks (CreatureBaseAttack.cpp). */
	bool IsClawClear(const AActor& Creature, const APawn& Victim)
	{
		const UWorld* World = Creature.GetWorld();
		if (!World)
		{
			return true;
		}
		FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("UnpaidClaw"), &Creature, false);
		Params.AddIgnoredActor(&Victim);
		const FCollisionObjectQueryParams Solid(ECC_WorldStatic);
		const float Half = Victim.GetSimpleCollisionHalfHeight();
		for (const FVector& To : { Victim.GetActorLocation(), Victim.GetActorLocation() - FVector(0.f, 0.f, Half * 0.7f) })
		{
			if (!World->LineTraceTestByObjectType(Creature.GetActorLocation(), To, Solid, Params))
			{
				return true;
			}
		}
		return false;
	}
}

// ---------------------------------------------------------------------------
// Who fights with the kit
// ---------------------------------------------------------------------------

bool AUnpaidCreature::HasFightKit() const
{
	if (!FUnpaidFightTuning::IsKitOn() || Kit.BossAdd == 1 || IsBossCreature())
	{
		return false;
	}
	// Its own class or a data-only child of it (BP_Unpaid): the kinds built on its rig with attacks of their own (the
	// miners' pick, the Bell-Ringer's bell, Abel's pump) are native classes of their own and keep theirs.
	const UClass* Native = GetClass();
	while (Native && !Native->HasAnyClassFlags(CLASS_Native))
	{
		Native = Native->GetSuperClass();
	}
	return Native == AUnpaidCreature::StaticClass();
}

bool AUnpaidCreature::IsBossAdd()
{
	if (Kit.BossAdd >= 0)
	{
		return Kit.BossAdd == 1;
	}
	Kit.BossAdd = 0;
	// Only ones spawned in play can be (a placed one comes back; an add never does), so a placed pack never looks.
	if (WillRespawn())
	{
		return false;
	}
	auto Check = [this](const ACreatureBase* Each)
	{
		const UBossComponent* Boss = Each && Each != this && Each->IsBossCreature() ? Each->FindComponentByClass<UBossComponent>() : nullptr;
		if (Boss && Boss->GetAliveAdds().Contains(this))
		{
			Kit.BossAdd = 1;
		}
	};
	if (const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this))
	{
		for (const TWeakObjectPtr<ACreatureBase>& Each : Encounters->GetTrackedCreatures())
		{
			Check(Each.Get());
		}
	}
	else if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ACreatureBase> It(World); It; ++It)
		{
			Check(*It);
		}
	}
	// A boss fight's pacing is its own (BOSSES tunes Abel's adds with him): his risen keep the old shriek and lunge.
	return Kit.BossAdd == 1;
}

// ---------------------------------------------------------------------------
// Each frame
// ---------------------------------------------------------------------------

void AUnpaidCreature::TickFightKit(float DeltaSeconds)
{
	Kit.CinderCooldown = FMath::Max(Kit.CinderCooldown - DeltaSeconds, 0.f);
	Kit.FlankCooldown = FMath::Max(Kit.FlankCooldown - DeltaSeconds, 0.f);
	const APawn* Victim = GetTarget();
	bool bKit = Victim && !IsDead() && HasFightKit();
	if (bKit && Kit.BossAdd < 0 && IsBossAdd())
	{
		bKit = false;
	}
	if (!bKit)
	{
		if (Kit.Action != EUnpaidAction::None)
		{
			EndKitAction(true);
		}
		Kit.WaitTime = 0.f;
		Kit.bFlankRolled = false;
		Kit.bFlankArmed = false;
		UpdateChaseBoost(nullptr, false);
		return;
	}
	const FUnpaidFightTuning& Tuning = FUnpaidFightTuning::Live();
	const ECreatureState BrainState = GetCreatureState();
	UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(this);

	// Its attack: the coal flares up through every wind-up (the tell), the rake swipes on.
	if (BrainState == ECreatureState::Attack)
	{
		if (GetStateTime() < AttackWindup)
		{
			Heat = FMath::Max(Heat, Tuning.TellFlare * KitEase(GetStateTime() / FMath::Max(AttackWindup, 0.01f)));
		}
		if (Kit.Move == EUnpaidMove::Rake)
		{
			TickRake();
		}
	}
	else
	{
		Kit.Move = EUnpaidMove::None;
		Kit.bSecondSwipe = false;
	}

	// Its pack's frenzy: every coal burns high while the pack comes on together.
	const bool bFrenzy = Director && Director->GetFrenzyLeft(*Victim, PackTag) > 0.f;
	if (bFrenzy)
	{
		Heat = FMath::Max(Heat, Tuning.TellFlare * FrenzyGlowShare);
	}
	UpdateChaseBoost(Victim, bFrenzy);

	if (Kit.Action != EUnpaidAction::None)
	{
		TickKitAction(DeltaSeconds, *Victim);
		return;
	}

	const bool bChasing = BrainState == ECreatureState::Chase;
	const bool bWaiting = bChasing && IsWaitingTurn();
	Kit.WaitTime = bWaiting ? Kit.WaitTime + DeltaSeconds : 0.f;
	if (!bWaiting)
	{
		// A new wait on the ring rolls for a flank afresh.
		Kit.bFlankRolled = false;
	}
	if (!Director || !bChasing)
	{
		// Out of the chase (its attack can't have started with a flank ready, so this is a search, a return, a call-off).
		Kit.bFlankArmed = false;
		return;
	}
	if (IsPhasing() || bLunging || IsHeldByHit())
	{
		return;
	}

	// Its turn has come with a flank ready (ThinkOnRing armed it while it waited in front of the player's eyes): it fades
	// round to their side; with no spot round them, once they've turned from it, or with another of the pack just blinked
	// (one at a player at a time: MayPackStep), it attacks from where it is next frame. Out of their sight (a wall between)
	// it lets the turn go for another to take (UnpaidCreatureSight.cpp).
	if (Director->HoldsToken(*this))
	{
		const bool bFlank = Kit.bFlankArmed;
		Kit.bFlankArmed = false;
		if (!(bFlank && MayPackStep(*Victim) && TryFlank(*Victim)))
		{
			TickBlindTurn(*Director, *Victim);
		}
		return;
	}
	// Out of their sight since it let a turn go: it stays out of the line until it sees them.
	TickBlindTurn(*Director, *Victim);
	if (!bWaiting)
	{
		// Off the ring with no turn (the ring broke up): no flank held for it.
		Kit.bFlankArmed = false;
	}

	// Waiting on the ring: now and then it looks at its choices (a rally, a flank for its next turn, a cinder).
	if (IsWaitingTurn())
	{
		Kit.ThinkIn -= DeltaSeconds;
		if (Kit.ThinkIn <= 0.f)
		{
			Kit.ThinkIn = ThinkSeconds + FMath::FRand() * ThinkJitter;
			ThinkOnRing(*Victim);
		}
	}
}

void AUnpaidCreature::UpdateChaseBoost(const APawn* Victim, bool bFrenzy)
{
	float Boost = 1.f;
	if (Victim && GetCreatureState() == ECreatureState::Chase)
	{
		const FUnpaidFightTuning& Tuning = FUnpaidFightTuning::Live();
		const float Distance = static_cast<float>(FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation()));
		Boost = UnpaidFightRules::ChaseBoost(Distance, IsWaitingTurn(), Tuning.ClosingRange * GetSizeScale(), Tuning.ClosingScale, bFrenzy,
			Tuning.FrenzyChase);
	}
	if (FMath::IsNearlyEqual(Boost, Kit.ToldBoost, 0.001f))
	{
		return;
	}
	// Written only when it changes: the director keeps it until then (and forgets it with the creature).
	if (UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(this))
	{
		Director->SetChaseBoost(*this, Boost);
		Kit.ToldBoost = Boost;
	}
}

// ---------------------------------------------------------------------------
// Its attack: the rake, or the lunge
// ---------------------------------------------------------------------------

void AUnpaidCreature::ChooseKitMove()
{
	Kit.Move = EUnpaidMove::None;
	Kit.bSecondSwipe = false;
	Kit.LungeReach = 0.f;
	if (!HasFightKit() || IsBossAdd())
	{
		return;
	}
	const FUnpaidFightTuning& Tuning = FUnpaidFightTuning::Live();
	const APawn* Victim = GetTarget();
	const float Distance = Victim ? static_cast<float>(FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation())) : TNumericLimits<float>::Max();
	Kit.Move = UnpaidFightRules::ChooseMove(Distance, Tuning.RakeRange * GetSizeScale(), Tuning.RakeChance, FMath::FRand());
	// Each move's own timings, its rank's quickening on the wind-up (a Restless one rakes sooner too); Fleet's trait then
	// quickens whatever was set here (UCreatureTraitComponent).
	if (Kit.Move == EUnpaidMove::Rake)
	{
		Kit.LeadArm = FMath::RandBool() ? 1 : 0;
		AttackWindup = FMath::Max(Tuning.RakeWindup * Traits.WindupScale, 0.05f);
		AttackRecovery = Tuning.RakeGap + Tuning.RakeRecovery;
	}
	else
	{
		AttackWindup = BaseWindup * Traits.WindupScale;
		AttackRecovery = Kit.BaseRecovery;
	}
}

void AUnpaidCreature::TickRake()
{
	const FUnpaidFightTuning& Tuning = FUnpaidFightTuning::Live();
	const float After = GetStateTime() - AttackWindup;
	if (After < 0.f || IsHeldByHit())
	{
		return;
	}
	// It drifts in through its swipes, so a slow backpedal still gets caught; flown like the lunge, off the walls.
	if (After < Tuning.RakeGap + RakeDriftTail)
	{
		const float Speed = Tuning.RakeDrift * GetSizeScale();
		UCharacterMovementComponent* Movement = GetCharacterMovement();
		Movement->MaxWalkSpeed = FMath::Max(Speed, 1.f);
		Movement->Velocity = GetActorForwardVector() * Speed + FVector(0.f, 0.f, Movement->Velocity.Z);
		AddMovementInput(GetActorForwardVector());
	}
	if (!Kit.bSecondSwipe && After >= Tuning.RakeGap)
	{
		Kit.bSecondSwipe = true;
		Swipe(1 - Kit.LeadArm);
	}
}

bool AUnpaidCreature::Swipe(int32 Arm)
{
	const FUnpaidFightTuning& Tuning = FUnpaidFightTuning::Live();
	const float Scale = GetSizeScale();
	Voice->Play(FName(UnpaidKitCue::Rake));
	APawn* Victim = GetTarget();
	if (!IsValidTarget(Victim))
	{
		return false;
	}
	const FVector Here = GetActorLocation();
	const FVector There = Victim->GetActorLocation();
	if (!UnpaidFightRules::InSwipe(Here, GetActorForwardVector(), There, Tuning.RakeRadius * Scale, Tuning.RakeArc,
		ReachUpDown * FMath::Max(Scale, 1.f)) || !IsClawClear(*this, *Victim))
	{
		return false;
	}
	// The swipe knocks them aside, the way the arm swept: the left arm sweeps to the right and on.
	const FVector Across = GetActorRightVector() * (Arm == 0 ? 1.f : -1.f);
	HitWithAttack(Victim, ((There - Here).GetSafeNormal2D() + 0.6f * Across).GetSafeNormal2D(), Tuning.RakeDamageShare);
	UE_LOG(LogLooter, Verbose, TEXT("%s rakes %s (%s arm)."), *GetName(), *GetNameSafe(Victim), Arm == 0 ? TEXT("left") : TEXT("right"));
	return true;
}

// ---------------------------------------------------------------------------
// Its turn: the flank
// ---------------------------------------------------------------------------

bool AUnpaidCreature::TryFlank(const APawn& Victim)
{
	// Its chance and cooldown were settled as it was armed (ThinkOnRing); here only whether they're still in front of it.
	const FUnpaidFightTuning& Tuning = FUnpaidFightTuning::Live();
	const FVector Center = Victim.GetActorLocation();
	const float Distance = static_cast<float>(FVector::Dist2D(GetActorLocation(), Center));
	const float ViewYaw = static_cast<float>(Victim.GetViewRotation().Yaw);
	const float Bearing = PackRules::BearingAround(Center, GetActorLocation());
	if (!UnpaidFightRules::WantsFlank(FightRules::Frontness(Bearing, ViewYaw), Distance, Tuning.FlankFront, Tuning.FlankMinRange,
		Tuning.FlankMaxRange))
	{
		return false;
	}
	// Round them on their own level of ground, never off its hunting ground: the first spot the level allows, its own side first.
	const float VictimFeet = static_cast<float>(Center.Z) - Victim.GetSimpleCollisionHalfHeight();
	const float Radius = Tuning.FlankRadius * GetSizeScale();
	for (const float Around : UnpaidFightRules::FlankBearings(ViewYaw, Bearing))
	{
		const float Radians = FMath::DegreesToRadians(Around);
		const FVector Candidate(Center.X + FMath::Cos(Radians) * Radius, Center.Y + FMath::Sin(Radians) * Radius, VictimFeet);
		FVector Feet;
		if (!HuntingGround.ContainsSpot(Candidate, GetHome().GetLocation()) || !CanStandAt(Candidate, Feet)
			|| FMath::Abs(Feet.Z - VictimFeet) > PhaseStep.MaxRise)
		{
			continue;
		}
		StartStepTo(Feet);
		Kit.FlankCooldown = Tuning.FlankCooldown;
		// The tell: a cold whisper from where it will come back, before it's there.
		LooterSound::PlayAt(this, FName(UnpaidKitCue::Flank), Feet + FVector(0.f, 0.f, FlankWhisperHeight), 1.f, Voice->GetPitch());
		UE_LOG(LogLooter, Verbose, TEXT("%s flanks %s: back %.0f degrees round from their view."), *GetName(), *Victim.GetName(),
			FMath::Abs(FMath::UnwindDegrees(Around - ViewYaw)));
		return true;
	}
	return false;
}


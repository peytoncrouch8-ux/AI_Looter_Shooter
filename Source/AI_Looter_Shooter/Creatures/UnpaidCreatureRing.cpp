// AUnpaidCreature's kit on the ring (round 2, 2026-10-27; UnpaidCreatureMoves.cpp has the rest): waiting its turn it doesn't
// just stand. Now and then it throws a cinder (the coal swells at its chest first: the tell; a slow glowing pellet led at
// where the player will be, one at a time at a player: UFightDirectorSubsystem::TryTakeCinder). Three or more after one
// player, one screams the pack on: a ring of its coal's light runs out, and after the scream the pack comes on together for
// a while (the director's frenzy: more turns at once, a faster chase). A hit that staggers it calls either off. In front
// of the player's eyes it may ready a flank for its next turn (UnpaidCreatureMoves.cpp sends it round when the turn comes).
// A led pack (FIGHTS.md lane 5, 2026-10-29) screams with its leader's voice only, and not at all for a while after its leader
// falls (the director's lull). A camping player (still or hidden too long) draws cinders sooner and more flanks.

#include "Creatures/UnpaidCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/CreaturePackComponent.h"
#include "Creatures/FightRules.h"
#include "Creatures/PackRules.h"
#include "Creatures/ShriekRing.h"
#include "Creatures/UnpaidFightRules.h"
#include "Creatures/UnpaidFightTuning.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** After its cinder leaves its hand the throw follows through this long (s) before it's back to the ring. */
	constexpr float CinderFollowThrough = 0.3f;

	/** The cinder's tell: the glow at its chest swells to this many times the cinder's size. */
	constexpr float CinderGlowShare = 1.4f;

	/** A cinder leaves this far in front of its coal (cm, at size 1), clear of its own body. */
	constexpr float CinderOut = 25.f;

	/** The rally scream's coal comes to its brightest over this long (s). */
	constexpr float RallyFlareSeconds = 0.25f;

	float RingEase(float Alpha)
	{
		return FMath::InterpEaseInOut(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f), 2.f);
	}
}

void AUnpaidCreature::ThinkOnRing(const APawn& Victim)
{
	UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(this);
	if (!Director)
	{
		return;
	}
	// At a camping player the ring presses harder: cinders sooner, more flanks (FUnpaidFightTuning::ForCamping).
	const FUnpaidFightTuning Tuning = FUnpaidFightTuning::Live().ForCamping(IsTargetCamping(Victim));
	// The pack's scream first: three or more after one player, none screamed lately, no frenzy on; in a led pack only its
	// leader screams, and none in the lull after a leader's fall (asked last: the director's walk of its leaders).
	if (Kit.WaitTime >= Tuning.RallyWait && Director->CountPack(Victim, PackTag) >= Tuning.RallyMinPack
		&& Director->GetFrenzyLeft(Victim, PackTag) <= 0.f)
	{
		const float Cooldown = UnpaidFightRules::RallyCooldownFor(Director->IsPackLed(*this), Pack->IsLeader(), Director->IsInLeaderLull(*this),
			Tuning.RallyCooldown, Tuning.LeaderRallyCooldown);
		if (Cooldown >= 0.f && Director->TryStartRally(Victim, Cooldown))
		{
			StartKitAction(EUnpaidAction::Rally);
			return;
		}
	}
	const float Distance = static_cast<float>(FVector::Dist2D(GetActorLocation(), Victim.GetActorLocation()));
	// In front of the player's eyes: once this wait, the dice say whether its next turn opens with a flank (it then holds
	// its attack until the turn comes: CanStartAttack; TickFightKit sends it round).
	if (!Kit.bFlankRolled && !Kit.bFlankArmed && Kit.FlankCooldown <= 0.f)
	{
		const float Bearing = PackRules::BearingAround(Victim.GetActorLocation(), GetActorLocation());
		const float Front = FightRules::Frontness(Bearing, static_cast<float>(Victim.GetViewRotation().Yaw));
		if (UnpaidFightRules::WantsFlank(Front, Distance, Tuning.FlankFront, Tuning.FlankMinRange, Tuning.FlankMaxRange))
		{
			Kit.bFlankRolled = true;
			Kit.bFlankArmed = FMath::FRand() < Tuning.FlankChance;
			UE_LOG(LogLooter, Verbose, TEXT("%s waits in front of %s: %s."), *GetName(), *Victim.GetName(),
				Kit.bFlankArmed ? TEXT("its next turn opens with a flank") : TEXT("no flank this time"));
		}
	}
	if (Kit.WaitTime < Tuning.CinderWait || Kit.CinderCooldown > 0.f)
	{
		return;
	}
	if (Distance < Tuning.CinderMinRange || Distance > Tuning.CinderMaxRange)
	{
		return;
	}
	// A clear line from its coal to them (one trace, only when the rest says yes), then one cinder at a time at a player.
	const UWorld* World = GetWorld();
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("UnpaidCinderLine"), this, false);
	if (!World || World->LineTraceTestByObjectType(GetCoalLocation(), Victim.GetActorLocation(), FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		return;
	}
	if (Director->TryTakeCinder(Victim, Tuning.CinderGap))
	{
		StartKitAction(EUnpaidAction::Cinder);
	}
}

bool AUnpaidCreature::StartKitAction(EUnpaidAction Action)
{
	const APawn* Victim = GetTarget();
	if (Action == EUnpaidAction::None || Kit.Action != EUnpaidAction::None || !Victim || IsDead() || IsPhasing() || Kit.bLunging
		|| GetCreatureState() == ECreatureState::Attack)
	{
		return false;
	}
	const FUnpaidFightTuning& Tuning = FUnpaidFightTuning::Live();
	UWorld* World = GetWorld();
	const float Scale = GetSizeScale();
	Kit.Action = Action;
	Kit.ActionTime = 0.f;
	Kit.bThrown = false;
	if (Action == EUnpaidAction::Cinder)
	{
		// The tell: the coal swells at its chest (a glow that follows it) with a crackle and a breath drawn in.
		if (UEnemyProjectileSubsystem* Shots = World ? World->GetSubsystem<UEnemyProjectileSubsystem>() : nullptr)
		{
			const FVector Chest = GetActorTransform().InverseTransformPosition(GetCoalLocation() + GetActorForwardVector() * (CinderOut * Scale));
			Shots->ShowCharge(this, Chest, Tuning.CinderTell, Tuning.CinderRadius * CinderGlowShare * Scale, GetCoalColor());
		}
		Voice->Play(FName(UnpaidKitCue::Cinder));
		UE_LOG(LogLooter, Verbose, TEXT("%s readies a cinder for %s."), *GetName(), *Victim->GetName());
	}
	else
	{
		// The scream that carries, and a ring of its coal's light running out over the ground (it slows nobody).
		LooterSound::PlayAt(this, FName(UnpaidKitCue::Rally), GetActorLocation(), 1.f, Voice->GetPitch());
		const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		AShriekRing::Spawn(World, Feet, Tuning.RallyRing * Scale, GetCoalColor(), nullptr, 1.f, 0.f);
		UE_LOG(LogLooter, Verbose, TEXT("%s screams its pack on at %s."), *GetName(), *Victim->GetName());
	}
	return true;
}

void AUnpaidCreature::TickKitAction(float DeltaSeconds, const APawn& Victim)
{
	const FUnpaidFightTuning& Tuning = FUnpaidFightTuning::Live();
	Kit.ActionTime += DeltaSeconds;
	// Answered: a hit that holds it (a stagger, a knock), or its fight moving on, calls the tell off.
	if (IsHeldByHit() || GetCreatureState() != ECreatureState::Chase || IsPhasing())
	{
		EndKitAction(!Kit.bThrown);
		return;
	}
	if (Kit.Action == EUnpaidAction::Cinder)
	{
		Heat = FMath::Max(Heat, Tuning.TellFlare * RingEase(Kit.ActionTime / Tuning.CinderTell));
		if (!Kit.bThrown && Kit.ActionTime >= Tuning.CinderTell)
		{
			Kit.bThrown = ThrowCinder();
			if (!Kit.bThrown)
			{
				EndKitAction(true);
				return;
			}
		}
		if (Kit.ActionTime >= Tuning.CinderTell + CinderFollowThrough)
		{
			EndKitAction(false);
		}
		return;
	}
	// The rally: it screams, still, facing them, its coal at its brightest; then the pack comes.
	Heat = FMath::Max(Heat, RingEase(Kit.ActionTime / RallyFlareSeconds));
	if (Kit.ActionTime >= Tuning.RallyTell)
	{
		// Its leader fell during the scream: the pack doesn't come (the director's frenzy doesn't look at the lull itself).
		UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(this);
		if (Director && !Director->IsInLeaderLull(*this))
		{
			Director->OpenFrenzy(Victim, PackTag, Tuning.FrenzySeconds, Tuning.FrenzyTokens);
		}
		EndKitAction(false);
	}
}

bool AUnpaidCreature::ThrowCinder()
{
	const APawn* Victim = GetTarget();
	UWorld* World = GetWorld();
	UEnemyProjectileSubsystem* Shots = World ? World->GetSubsystem<UEnemyProjectileSubsystem>() : nullptr;
	if (!Victim || !Shots || IsDead())
	{
		return false;
	}
	const FUnpaidFightTuning& Tuning = FUnpaidFightTuning::Live();
	const float Scale = GetSizeScale();
	FEnemyShot Shot;
	Shot.Start = GetCoalLocation() + GetActorForwardVector() * (CinderOut * Scale);
	// Led at where they'll be, as its lunge is: a player who stops short or cuts back makes it miss.
	const FVector Aim = UnpaidFightRules::LeadPoint(Shot.Start, Victim->GetActorLocation(), Victim->GetVelocity(), Tuning.CinderSpeed,
		Tuning.LeadShare, Tuning.LeadMaxSeconds);
	Shot.Direction = (Aim - Shot.Start).GetSafeNormal();
	if (Shot.Direction.IsNearlyZero())
	{
		Shot.Direction = GetActorForwardVector();
	}
	Shot.Speed = Tuning.CinderSpeed;
	Shot.Range = Tuning.CinderMaxRange * 1.5f;
	Shot.Radius = Tuning.CinderRadius * Scale;
	Shot.Damage = AttackDamage * Tuning.CinderDamageShare * GetFightDamageScale();
	Shot.Color = GetCoalColor();
	Shot.Shooter = this;
	Shot.Instigator = GetController();
	Shots->Fire(Shot);
	// Flung from its hand with a breath of cold air.
	Voice->Play(LooterSoundCue::UnpaidLunge, 0.6f);
	Kit.CinderCooldown = Tuning.CinderCooldown * FMath::FRandRange(0.8f, 1.2f);
	UE_LOG(LogLooter, Verbose, TEXT("%s throws a cinder at %s (%.1f before the roll)."), *GetName(), *Victim->GetName(), Shot.Damage);
	return true;
}

void AUnpaidCreature::EndKitAction(bool bCalledOff)
{
	// A cinder's tell put out before it left the hand: its glow goes with it (nothing of it is in flight yet: a cinder's
	// cooldown is several times its flight).
	if (bCalledOff && Kit.Action == EUnpaidAction::Cinder && !Kit.bThrown)
	{
		UWorld* World = GetWorld();
		if (UEnemyProjectileSubsystem* Shots = World ? World->GetSubsystem<UEnemyProjectileSubsystem>() : nullptr)
		{
			Shots->ClearShotsFrom(this);
		}
	}
	Kit.Action = EUnpaidAction::None;
	Kit.ActionTime = 0.f;
	Kit.bThrown = false;
	Kit.WaitTime = 0.f;
}

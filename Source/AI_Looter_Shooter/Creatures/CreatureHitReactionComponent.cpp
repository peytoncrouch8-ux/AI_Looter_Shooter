// UCreatureHitReactionComponent (CreatureHitReactionComponent.h): the hits (hit-stop, slow, stagger, knock, Big shove, HITS'
// weight, zone, side, offset and far boost). Its death (the burst, the killer's punch, the corpse's knock) is in
// CreatureHitReactionComponentDeath.cpp.

#include "Creatures/CreatureHitReactionComponent.h"
#include "AI_Looter_Shooter.h"
#include "Bosses/BossComponent.h"
#include "Combat/HealthComponent.h"
#include "Combat/MovementSlowComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureHitZones.h"
#include "Creatures/CreatureKnockComponent.h"
#include "Elements/ElementStatusComponent.h"
#include "Weapons/GunKindsTuning.h"
#include "Weapons/GunshotSound.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

namespace
{
	/**
	 * A boss: Boss rank, or any creature a boss fight is built round (its UBossComponent: the Gravemother). Its show
	 * (UBossComponent's stagger, slow-motion death, loot shower) runs its big moments, so none of these reactions apply.
	 */
	bool IsBoss(const ACreatureBase& Creature)
	{
		return Creature.GetRank() == ECreatureRank::Boss || Creature.FindComponentByClass<UBossComponent>() != nullptr;
	}

	/** A stagger's shove at each rank, of a Basic's: a tougher creature gives less ground. */
	float ShoveShare(ECreatureRank Rank)
	{
		switch (Rank)
		{
		case ECreatureRank::Rare: return 0.85f;
		case ECreatureRank::Epic: return 0.7f;
		case ECreatureRank::Hallowed: return 0.6f;
		case ECreatureRank::Ethereal: return 0.5f;
		default: return 1.f;
		}
	}

	const TCHAR* JobName(FHitReaction::EJob Job)
	{
		switch (Job)
		{
		case FHitReaction::EJob::Heavy: return TEXT("heavy");
		case FHitReaction::EJob::Melee: return TEXT("melee");
		case FHitReaction::EJob::Pressure: return TEXT("rifle pressure");
		case FHitReaction::EJob::Power: return TEXT("shotgun power");
		case FHitReaction::EJob::Poise: return TEXT("poise broken");
		default: return TEXT("none");
		}
	}
}

UCreatureHitReactionComponent::UCreatureHitReactionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UCreatureHitReactionComponent::BeginPlay()
{
	Super::BeginPlay();
	// Hits come from ACreatureBase::HandleDamaged (TakeHit, before its kind's flinch); the death straight from the health.
	if (UHealthComponent* Found = GetOwner() ? GetOwner()->FindComponentByClass<UHealthComponent>() : nullptr)
	{
		Found->OnDeath.AddDynamic(this, &UCreatureHitReactionComponent::HandleDeath);
		Health = Found;
	}
}

void UCreatureHitReactionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitStopTimer);
	}
	EndHitStop();
	if (UHealthComponent* Found = Health.Get())
	{
		Found->OnDeath.RemoveDynamic(this, &UCreatureHitReactionComponent::HandleDeath);
	}
	Super::EndPlay(EndPlayReason);
}

bool UCreatureHitReactionComponent::IsStaggered() const
{
	const UWorld* World = GetWorld();
	return World && Reaction.IsStaggered(World->GetTimeSeconds());
}

float UCreatureHitReactionComponent::GetStaggerAlpha() const
{
	// A boss's hits never reach the rules (TakeHit), so its alpha stays 0.
	const UWorld* World = GetWorld();
	return World ? Reaction.GetStaggerAlpha(World->GetTimeSeconds()) : 0.f;
}

bool UCreatureHitReactionComponent::IsHitStopped() const
{
	const UWorld* World = GetWorld();
	return World && Reaction.IsHitStopped(World->GetTimeSeconds());
}

EWeaponKind UCreatureHitReactionComponent::KindOf(const AActor* DamageCauser)
{
	// A bullet's damage is dealt with the gun that fired it as its causer (UBulletSubsystem).
	const AWeaponBase* Weapon = Cast<AWeaponBase>(DamageCauser);
	const UWeaponDefinition* Definition = Weapon ? Weapon->GetInstance().Definition.Get() : nullptr;
	return Definition ? Definition->Kind : EWeaponKind::None;
}

float UCreatureHitReactionComponent::CorpseKnockFor(EWeaponKind Kind, bool bCritical, const FGunfeelReactTuning& Tuning)
{
	float Speed = CorpseKnock;
	switch (Kind)
	{
	case EWeaponKind::Shotgun: Speed = Tuning.KillShotgunKnock; break;
	// The automatic guns crumple it where it stands, as the rifle does.
	case EWeaponKind::Rifle:
	case EWeaponKind::SMG:
	case EWeaponKind::LMG: Speed = Tuning.KillRifleKnock; break;
	// A rocket's kill is thrown by the burst (Looter.Gunfeel.Kill.LauncherKnock).
	case EWeaponKind::Launcher: Speed = GunKinds::KillLauncherKnock(); break;
	default: break;
	}
	// A crit kill keeps its old knock where that's the larger (a rifle's headshot still rocks it).
	return bCritical ? FMath::Max(Speed, CritCorpseKnock) : Speed;
}

float UCreatureHitReactionComponent::CorpseLiftFor(EWeaponKind Kind, const FGunfeelReactTuning& Tuning)
{
	switch (Kind)
	{
	case EWeaponKind::Shotgun: return Tuning.KillShotgunLift;
	case EWeaponKind::Launcher: return GunKinds::KillLauncherLift();
	default: return 0.f;
	}
}

float UCreatureHitReactionComponent::CorpseKnockFor(EWeaponKind Kind, bool bCritical, EHitWeight Weight, const FGunfeelReactTuning& Tuning,
	const FHitFeelTuning& Feel)
{
	// An overkill's body goes farther: the shot had more than enough in it.
	const float Speed = CorpseKnockFor(Kind, bCritical, Tuning);
	return Feel.KillStyles && Weight == EHitWeight::Overkill ? Speed * Feel.OverkillKnock : Speed;
}

float UCreatureHitReactionComponent::CorpseLiftFor(EWeaponKind Kind, EHitWeight Weight, const FGunfeelReactTuning& Tuning, const FHitFeelTuning& Feel)
{
	// A blast's overkill throws it up off its feet; a bullet's (a sniper's) slides it, as ever.
	const float Lift = CorpseLiftFor(Kind, Tuning);
	const bool bBlast = Kind == EWeaponKind::Shotgun || Kind == EWeaponKind::Launcher;
	return Feel.KillStyles && Weight == EHitWeight::Overkill && bBlast ? FMath::Max(Lift, Feel.OverkillLift) : Lift;
}

// ---------------------------------------------------------------------------
// Hits: the hit-stop, the slow, the stagger and the knock
// ---------------------------------------------------------------------------

void UCreatureHitReactionComponent::PlaceHit(const ACreatureBase& Creature, FCreatureHit& Hit, const UPrimitiveComponent* HitComponent,
	const FHitFeelTuning& Feel) const
{
	// The layer's switch, and a status's tick (a burn has no wound to twist round): a plain front body hit.
	if (!Feel.Zones || Hit.Element != ELooterElement::None)
	{
		Hit.Zone = ECreatureHitZone::Body;
		Hit.Side = EHitSide::Front;
		Hit.LateralOffset = 0.f;
		return;
	}
	Hit.Zone = CreatureHitZones::ZoneOfHit(Creature, HitComponent, Hit.Bone, Hit.bCritical);
	Hit.Side = HitFeel::SideOf(Creature.GetActorForwardVector(), Hit.Direction);
	Hit.LateralOffset = HitFeel::LateralOffsetOf(Creature.GetActorLocation(), Hit.Direction, Hit.Location);
}

FCreatureHit UCreatureHitReactionComponent::TakeHit(float Damage, bool bCritical, const FVector& HitLocation, AController* InstigatedBy,
	AActor* DamageCauser, FName Bone, const UPrimitiveComponent* HitComponent)
{
	FCreatureHit Hit;
	ACreatureBase* Creature = Cast<ACreatureBase>(GetOwner());
	const UHealthComponent* Life = Health.Get();
	const UWorld* World = GetWorld();
	if (!Creature || !Life || !World)
	{
		return Hit;
	}
	// Read once a hit, so a change in PIE counts from the next one.
	const FGunfeelReactTuning Tuning = FGunfeelReactTuning::Live();
	const FHitFeelTuning Feel = FHitFeelTuning::Live();
	const float MaxHealth = FMath::Max(Life->GetMaxHealth(), 1.f);
	Hit.Share = FMath::Max(Damage, 0.f) / MaxHealth;
	Hit.BurstShare = Hit.Share;
	Hit.Kind = KindOf(DamageCauser);
	// A status's piece or smite (the gun that set it is the causer, for the notch) is no shot of that gun: not its kind's
	// stagger, power or knock, and its kill is the element's death.
	Hit.Element = UElementStatusComponent::DealingElement();
	if (Hit.Element != ELooterElement::None)
	{
		Hit.Kind = EWeaponKind::None;
	}
	Hit.bCritical = bCritical;
	Hit.Location = HitLocation;
	Hit.bBoss = IsBoss(*Creature);
	// A shot stopped by the capsule beside the body counts on the bone nearest it, so its zone (and an arm's fling, a
	// spider's leg) is the part it struck, not a plain body hit.
	Hit.Bone = CreatureHitZones::BoneOfHit(*Creature, HitComponent, Bone, HitLocation);

	// The shot's way: from whoever fired it (else what dealt it) through the wound.
	const APawn* Shooter = InstigatedBy ? InstigatedBy->GetPawn() : nullptr;
	const FVector From = Shooter ? Shooter->GetActorLocation() : (DamageCauser ? DamageCauser->GetActorLocation() : HitLocation);
	const FVector Way = HitLocation - From;
	Hit.Direction = Way.IsNearlyZero() ? (Creature->GetActorLocation() - From).GetSafeNormal() : Way.GetSafeNormal();
	Hit.Distance = Shooter ? static_cast<float>(FVector::Dist(Shooter->GetActorLocation(), Creature->GetActorLocation())) : -1.f;
	// A flinch 25 m off reads as half of one at 10 m: the far boost makes it up (a boss's flinch keeps its own size).
	Hit.FarBoost = HitFeel::FarBoostAt(Hit.Distance, Feel);
	// Where on its body, and from which side: a boss's too (the Gravemother's legs buckle; looks only).
	PlaceHit(*Creature, Hit, HitComponent, Feel);

	// The precision guns' crits (the revolver's, the sniper's) ring for their shooter, a boss too: it's the shot's reward,
	// not the creature's reaction.
	APlayerController* Player = Cast<APlayerController>(InstigatedBy);
	const bool bPreciseGun = Hit.Kind == EWeaponKind::Revolver || Hit.Kind == EWeaponKind::Sniper;
	if (bCritical && bPreciseGun && Player && Player->IsLocalController())
	{
		GunshotSound::PlayCritRing(Player);
	}
	if (Hit.bBoss)
	{
		LastHit = Hit;
		return Hit;
	}

	// The health has already dropped; the death event comes right after this one.
	const bool bKilled = Life->GetHealth() <= 0.f;
	Hit.bKilled = bKilled;
	Hit.Overkill = bKilled ? HitFeel::OverkillOf(Damage, Life->GetHealthBeforeLastHit(), MaxHealth) : 0.f;
	FHitReaction::FHit Rule;
	Rule.Damage = Damage;
	Rule.MaxHealth = MaxHealth;
	Rule.bCritical = bCritical;
	Rule.bKilled = bKilled;
	Rule.Rank = Creature->GetRank();
	Rule.Kind = Hit.Kind;
	Rule.Distance = Hit.Distance;
	Rule.Size = Creature->GetSizeScale();
	Rule.Overkill = Hit.Overkill;
	Reaction.SetTuning(Tuning);
	Reaction.SetFeel(Feel);
	const double Now = World->GetTimeSeconds();
	const FHitReaction::FResponse Response = Reaction.OnHit(Now, Rule);
	Hit.BurstShare = FMath::Max(Response.BurstShare, Hit.Share);
	Hit.Weight = Response.Weight;
	LastHit = Hit;
	UE_LOG(LogLooter, Verbose, TEXT("%s: %s hit on the %s (bone %s) from the %s, %.0f cm off its axis, far x%.2f, poise %.2f"),
		*Creature->GetName(), HitFeel::WeightName(Hit.Weight), HitFeel::ZoneName(Hit.Zone), *Hit.Bone.ToString(),
		HitFeel::SideName(Hit.Side), Hit.LateralOffset, Hit.FarBoost, Reaction.GetPoise(Now));

	if (Response.HitStopSeconds > 0.f)
	{
		StartHitStop();
	}
	// A creature held back (a boss's spell) is walking home, not fighting: no shove, no slow.
	if (bKilled || Creature->IsPassive())
	{
		return Hit;
	}
	if (Response.bKnockBack)
	{
		KnockBack(*Creature, Hit.Direction, Tuning);
	}
	else if (Response.StaggerSeconds > 0.f)
	{
		Shove(*Creature, Hit.Direction, Response.BurstShare, Tuning.StaggerShove);
	}
	else if (Hit.Weight == EHitWeight::Big && Feel.BigShove > 0.f && Creature->GetSizeScale() <= Tuning.ShotgunKnockSize)
	{
		// A Big hit that didn't stagger still gives ground: a step back along the shot, its brain never held (a knock here
		// would stun it on every sniper round, and fights must not get easier).
		Shove(*Creature, Hit.Direction, Response.BurstShare, Feel.BigShove);
	}
	// Every hit holds it back a moment: a charging creature visibly catches the bullet (strongest slow wins, never stacks).
	// A kind with a gap between its slows (the Unpaid, 2026-10-28: steady rifle fire kept every lunge short) takes one, then
	// none for the gap, whatever lands in between.
	const UCreatureKnockComponent* Knock = Creature->GetKnock();
	const bool bSlowDue = Now - LastSlowAt >= Creature->GetHitSlowGap();
	if (Tuning.FlinchSlow < 1.f && Tuning.FlinchSlowSeconds > 0.f && bSlowDue && !(Knock && Knock->IsKnocked()))
	{
		LastSlowAt = Now;
		const bool bFirst = Creature->FindComponentByClass<UMovementSlowComponent>() == nullptr;
		UMovementSlowComponent* Slow = UMovementSlowComponent::Apply(Creature, Tuning.FlinchSlow, Tuning.FlinchSlowSeconds);
		if (Slow && bFirst)
		{
			// A creature's brain sets its walking speed in its own frame (after physics, every step it steers): the slow
			// has to have its say after that, before the next frame's move, or the brain's speed would win every frame.
			Slow->SetTickGroup(TG_PostPhysics);
			Slow->AddTickPrerequisiteActor(Creature);
		}
	}
	if (Response.Job != FHitReaction::EJob::None)
	{
		UE_LOG(LogLooter, Verbose, TEXT("%s: %s (%.0f%% of its health in the burst)%s"), *Creature->GetName(), JobName(Response.Job),
			Response.BurstShare * 100.f, Response.bKnockBack ? TEXT(", knocked back") : TEXT(""));
	}
	return Hit;
}

void UCreatureHitReactionComponent::StartHitStop()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return;
	}
	// Its own clock only (its brain, movement, pose and animation): the world, the player and every other creature run on.
	Owner->CustomTimeDilation = FHitReaction::HitStopDilation;
	const float Seconds = static_cast<float>(Reaction.GetHitStopEnd() - World->GetTimeSeconds());
	World->GetTimerManager().SetTimer(HitStopTimer, this, &UCreatureHitReactionComponent::EndHitStop, FMath::Max(Seconds, 0.001f), false);
}

void UCreatureHitReactionComponent::EndHitStop()
{
	AActor* Owner = GetOwner();
	// Only ours to undo: anything else that slowed it since keeps its say.
	if (Owner && FMath::IsNearlyEqual(Owner->CustomTimeDilation, FHitReaction::HitStopDilation))
	{
		Owner->CustomTimeDilation = 1.f;
	}
}

void UCreatureHitReactionComponent::Shove(ACreatureBase& Creature, const FVector& Direction, float BurstShare, float ShoveSpeed)
{
	UCharacterMovementComponent* Movement = Creature.GetCharacterMovement();
	const FVector Away = Direction.GetSafeNormal2D();
	// In the air (a slime's hop), its hop carries on; on the ground it gives a step, never over a drop.
	if (!Movement || !Movement->IsMovingOnGround() || Away.IsNearlyZero())
	{
		return;
	}
	const float Size = FMath::Max(Creature.GetSizeScale(), 0.3f);
	FVector Ground;
	if (!FindGround(Creature.GetActorLocation() + Away * 70.f * Size, 60.f * Size, 160.f * Size, Ground))
	{
		return;
	}
	// A bigger burst (a point-blank blast) shoves harder; a bigger body gives less.
	const float Strength = FMath::Clamp(0.7f + BurstShare * 1.5f, 0.7f, 1.4f) * ShoveShare(Creature.GetRank());
	Movement->Velocity += Away * (ShoveSpeed * Strength / FMath::Sqrt(Size));
}

void UCreatureHitReactionComponent::KnockBack(ACreatureBase& Creature, const FVector& Direction, const FGunfeelReactTuning& Tuning)
{
	UCreatureKnockComponent* Knock = Creature.GetKnock();
	if (!Knock || Knock->IsKnocked())
	{
		return;
	}
	// The launch's speed across the ground (Shotgun.KnockSpeed) as the distance its arc covers; the knock shortens it for a
	// tougher rank and a bigger body, and to where there's ground to land on.
	const UCharacterMovementComponent* Movement = Creature.GetCharacterMovement();
	const float Gravity = Movement ? FMath::Max(-Movement->GetGravityZ(), 1.f) : 980.f;
	const float Distance = GunfeelReact::KnockDistance(Tuning.ShotgunKnockSpeed, Tuning.ShotgunKnockHeight, Gravity);
	const FVector Launch = Knock->Knock(Direction, Distance, Tuning.ShotgunKnockHeight, Tuning.ShotgunKnockHold, ECreatureKnockStyle::Shove);
	if (Launch.IsNearlyZero())
	{
		// Nowhere to be thrown (a ledge behind it): a step back at least.
		Shove(Creature, Direction, Tuning.ShotgunKnockShare, Tuning.StaggerShove);
	}
}

bool UCreatureHitReactionComponent::FindGround(const FVector& Point, float Above, float Below, FVector& OutGround) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	// World-static only: terrain and solid props, never grass, pawns, loot or volumes (as a creature finds its ground).
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("CreatureHitGround"), GetOwner());
	FHitResult Hit;
	if (World->LineTraceSingleByObjectType(Hit, Point + FVector(0.f, 0.f, Above), Point - FVector(0.f, 0.f, Below),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		OutGround = Hit.ImpactPoint;
		return true;
	}
	return false;
}

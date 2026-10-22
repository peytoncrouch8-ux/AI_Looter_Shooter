#include "Creatures/CreatureHitReactionComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundRules.h"
#include "Bosses/BossComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Combat/MovementSlowComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureKnockComponent.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "Player/CameraShakeModifier.h"
#include "Player/ViewKick.h"
#include "Weapons/CreatureBlood.h"
#include "Weapons/GunshotSound.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

namespace
{
	/** Chitin as the burst's plates show it: the spider's brown hide, darker than its lit back. */
	const FColor SpiderChitin(0x4E, 0x34, 0x1E);
	/** Slime.py's gel tint. */
	const FColor SlimeGel(0xB8, 0xF0, 0x88);

	/** A revolver's crit kill bursts at the crit spot only if that's this near the body's middle (cm at size 1). */
	constexpr float CritSpotReach = 250.f;

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
	case EWeaponKind::Rifle: Speed = Tuning.KillRifleKnock; break;
	default: break;
	}
	// A crit kill keeps its old knock where that's the larger (a rifle's headshot still rocks it).
	return bCritical ? FMath::Max(Speed, CritCorpseKnock) : Speed;
}

float UCreatureHitReactionComponent::CorpseLiftFor(EWeaponKind Kind, const FGunfeelReactTuning& Tuning)
{
	return Kind == EWeaponKind::Shotgun ? Tuning.KillShotgunLift : 0.f;
}

// ---------------------------------------------------------------------------
// Hits: the hit-stop, the slow, the stagger and the knock
// ---------------------------------------------------------------------------

FCreatureHit UCreatureHitReactionComponent::TakeHit(float Damage, bool bCritical, const FVector& HitLocation, AController* InstigatedBy,
	AActor* DamageCauser)
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
	const float MaxHealth = FMath::Max(Life->GetMaxHealth(), 1.f);
	Hit.Share = FMath::Max(Damage, 0.f) / MaxHealth;
	Hit.BurstShare = Hit.Share;
	Hit.Kind = KindOf(DamageCauser);
	Hit.bCritical = bCritical;
	Hit.Location = HitLocation;
	Hit.bBoss = IsBoss(*Creature);

	// The shot's way: from whoever fired it (else what dealt it) through the wound.
	const APawn* Shooter = InstigatedBy ? InstigatedBy->GetPawn() : nullptr;
	const FVector From = Shooter ? Shooter->GetActorLocation() : (DamageCauser ? DamageCauser->GetActorLocation() : HitLocation);
	const FVector Way = HitLocation - From;
	Hit.Direction = Way.IsNearlyZero() ? (Creature->GetActorLocation() - From).GetSafeNormal() : Way.GetSafeNormal();

	// The revolver's crit rings for its shooter, a boss too: it's the shot's reward, not the creature's reaction.
	APlayerController* Player = Cast<APlayerController>(InstigatedBy);
	if (bCritical && Hit.Kind == EWeaponKind::Revolver && Player && Player->IsLocalController())
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
	FHitReaction::FHit Rule;
	Rule.Damage = Damage;
	Rule.MaxHealth = MaxHealth;
	Rule.bCritical = bCritical;
	Rule.bKilled = bKilled;
	Rule.Rank = Creature->GetRank();
	Rule.Kind = Hit.Kind;
	Rule.Distance = Shooter ? static_cast<float>(FVector::Dist(Shooter->GetActorLocation(), Creature->GetActorLocation())) : -1.f;
	Rule.Size = Creature->GetSizeScale();
	Reaction.SetTuning(Tuning);
	const FHitReaction::FResponse Response = Reaction.OnHit(World->GetTimeSeconds(), Rule);
	Hit.BurstShare = FMath::Max(Response.BurstShare, Hit.Share);
	LastHit = Hit;

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
	// Every hit holds it back a moment: a charging creature visibly catches the bullet (strongest slow wins, never stacks).
	const UCreatureKnockComponent* Knock = Creature->GetKnock();
	if (Tuning.FlinchSlow < 1.f && Tuning.FlinchSlowSeconds > 0.f && !(Knock && Knock->IsKnocked()))
	{
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

// ---------------------------------------------------------------------------
// Death: the burst, the killer's punch, the corpse's knock
// ---------------------------------------------------------------------------

EDeathBurst UCreatureHitReactionComponent::DeathBurstOf(const ACreatureBase& Creature)
{
	if (Creature.IsA<AUnpaidCreature>())
	{
		return EDeathBurst::SoulLight;
	}
	if (Creature.IsA<ASlimeCreature>())
	{
		return EDeathBurst::Gel;
	}
	if (Creature.IsA<ASpiderCreature>())
	{
		return EDeathBurst::Shell;
	}
	return EDeathBurst::None;
}

FLinearColor UCreatureHitReactionComponent::DeathBurstTint(const ACreatureBase& Creature)
{
	if (const AUnpaidCreature* Unpaid = Cast<AUnpaidCreature>(&Creature))
	{
		return Unpaid->GetCoalColor();
	}
	if (Creature.IsA<ASlimeCreature>())
	{
		return FLinearColor::FromSRGBColor(SlimeGel);
	}
	return FLinearColor::FromSRGBColor(SpiderChitin);
}

FName UCreatureHitReactionComponent::DeathBurstCue(EDeathBurst Kind)
{
	switch (Kind)
	{
	case EDeathBurst::Shell: return LooterSoundCue::SpiderBurst;
	case EDeathBurst::Gel: return LooterSoundCue::SlimeSplat;
	case EDeathBurst::SoulLight: return LooterSoundCue::UnpaidDissolve;
	default: return NAME_None;
	}
}

void UCreatureHitReactionComponent::HandleDeath(AController* Killer)
{
	ACreatureBase* Creature = Cast<ACreatureBase>(GetOwner());
	if (!Creature || IsBoss(*Creature))
	{
		return;
	}
	const FGunfeelReactTuning Tuning = FGunfeelReactTuning::Live();
	SpawnDeathBurst(*Creature, Tuning);

	// The killer's view punches in: a crit kill, or a Gravebound or worse, harder.
	APlayerController* Player = Cast<APlayerController>(Killer);
	if (Player && Player->IsLocalController())
	{
		const bool bHeavy = LastHit.bCritical || LooterRanks::IsAtLeast(Creature->GetRank(), ECreatureRank::Epic);
		UCameraShakeModifier::Kick(Player, ViewKicks::ForKill(bHeavy));
	}
	StartCorpseKnock(*Creature, Tuning);
}

void UCreatureHitReactionComponent::SpawnDeathBurst(ACreatureBase& Creature, const FGunfeelReactTuning& Tuning)
{
	UWorld* World = GetWorld();
	const EDeathBurst Kind = DeathBurstOf(Creature);
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (Kind == EDeathBurst::None || !Bullets)
	{
		return;
	}
	// From the middle of the body as drawn (a spider's legs spread wide, but its middle is its body).
	const USkeletalMeshComponent* Body = Creature.GetMesh();
	FVector Center = Body ? FVector(Body->Bounds.Origin) : Creature.GetActorLocation();
	float Size = 100.f * Creature.GetSizeScale();
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);

	// The revolver's precision: a crit kill bursts bigger, from the crit spot itself, with a last spurt out of it.
	const bool bPrecise = LastHit.Kind == EWeaponKind::Revolver && LastHit.bCritical && !LastHit.Location.IsNearlyZero()
		&& FVector::Dist(LastHit.Location, Center) < CritSpotReach * FMath::Max(Creature.GetSizeScale(), 0.3f);
	if (bPrecise)
	{
		Center = LastHit.Location;
		Size *= Tuning.KillRevolverBurst;
		Effects.SpawnCreatureHit(Center, -LastHit.Direction, LastHit.Direction, CreatureBlood::Of(&Creature), CreatureBlood::TintOf(&Creature),
			Tuning.KillRevolverBurst, true);
	}
	Effects.SpawnDeathBurst(Kind, Center, LastHit.Direction, Size, DeathBurstTint(Creature));
	LooterSound::PlayAt(this, DeathBurstCue(Kind), Center, 1.f, LooterSoundRules::PitchForSize(Creature.GetSizeScale()));
	UE_LOG(LogLooter, Verbose, TEXT("%s: death burst %d%s"), *Creature.GetName(), static_cast<int32>(Kind), bPrecise ? TEXT(" (revolver crit)") : TEXT(""));
}

void UCreatureHitReactionComponent::StartCorpseKnock(ACreatureBase& Creature, const FGunfeelReactTuning& Tuning)
{
	// A big body takes a killing shot without sliding off (a Soulfed monster's own death is its show).
	const FVector Away = LastHit.Direction.GetSafeNormal2D();
	if (Away.IsNearlyZero() || LooterRanks::IsAtLeast(Creature.GetRank(), ECreatureRank::Legendary))
	{
		return;
	}
	const float Size = FMath::Max(Creature.GetSizeScale(), 0.3f);
	// By the gun: a shotgun throws it with a lift, a rifle crumples it where it stands, anything else as ever.
	KnockVelocity = Away * (CorpseKnockFor(LastHit.Kind, LastHit.bCritical, Tuning) / FMath::Sqrt(Size));
	KnockLift = CorpseLiftFor(LastHit.Kind, Tuning);
	KnockLiftNow = 0.f;
	KnockTime = 0.f;
	// Its height over the ground is measured on the first step, after the body's own death has settled it (a slime
	// killed mid-hop drops to the ground first).
	bKnockMeasured = false;
	// Ticks on the corpse's own clock: the kill's hit-stop holds it, then it goes.
	SetComponentTickEnabled(true);
}

void UCreatureHitReactionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	StepCorpseKnock(DeltaTime);
}

void UCreatureHitReactionComponent::StepCorpseKnock(float DeltaTime)
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	KnockTime += DeltaTime;
	if (!Owner || !World || KnockTime > CorpseKnockSeconds || KnockVelocity.SizeSquared() < 100.f)
	{
		EndCorpseKnock();
		return;
	}
	// Where it lies under its lift: the ground rules go by that, the lift only rides on top.
	const FVector From = Owner->GetActorLocation() - FVector(0.0, 0.0, KnockLiftNow);
	const ACreatureBase* Creature = Cast<ACreatureBase>(Owner);
	const float Size = Creature ? FMath::Max(Creature->GetSizeScale(), 0.3f) : 1.f;
	FVector Ground;
	if (!bKnockMeasured)
	{
		bKnockMeasured = true;
		if (!FindGround(From, 50.f * Size, 300.f * Size, Ground))
		{
			EndCorpseKnock();
			return;
		}
		KnockGroundOffset = static_cast<float>(From.Z - Ground.Z);
	}
	const FVector To = From + KnockVelocity * DeltaTime;
	KnockVelocity *= FMath::Exp(-9.f * DeltaTime);

	// Never through anything solid: a short sweep at the body's height (a volume it starts inside doesn't count)...
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CorpseKnock), false, Owner);
	FHitResult Hit;
	const bool bBlocked = World->SweepSingleByObjectType(Hit, From, To, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldStatic),
		FCollisionShape::MakeSphere(20.f * Size), Params) && !Hit.bStartPenetrating;
	// ...and never off a drop or up a step: it keeps its height over the ground it lies on.
	const bool bGround = !bBlocked && FindGround(To, 60.f * Size, 120.f * Size, Ground);
	const float NewZ = bGround ? static_cast<float>(Ground.Z) + KnockGroundOffset : 0.f;
	if (!bGround || FMath::Abs(NewZ - static_cast<float>(From.Z)) > 40.f * Size)
	{
		EndCorpseKnock();
		return;
	}
	// A shotgun's throw lifts it in a low arc over the slide, back down on the ground as it ends.
	KnockLiftNow = KnockLift * FMath::Sin(UE_PI * FMath::Clamp(KnockTime / CorpseKnockSeconds, 0.f, 1.f));
	Owner->SetActorLocation(FVector(To.X, To.Y, NewZ + KnockLiftNow));
}

void UCreatureHitReactionComponent::EndCorpseKnock()
{
	AActor* Owner = GetOwner();
	if (Owner && KnockLiftNow > 0.f)
	{
		Owner->SetActorLocation(Owner->GetActorLocation() - FVector(0.0, 0.0, KnockLiftNow));
	}
	KnockLiftNow = 0.f;
	KnockVelocity = FVector::ZeroVector;
	SetComponentTickEnabled(false);
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

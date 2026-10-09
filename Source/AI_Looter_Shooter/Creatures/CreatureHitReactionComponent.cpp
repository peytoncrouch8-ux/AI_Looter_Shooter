#include "Creatures/CreatureHitReactionComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundRules.h"
#include "Bosses/BossComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "Player/CameraShakeModifier.h"
#include "Player/ViewKick.h"
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
		default: return 1.f;
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
	if (UHealthComponent* Found = GetOwner() ? GetOwner()->FindComponentByClass<UHealthComponent>() : nullptr)
	{
		Found->OnDamaged.AddDynamic(this, &UCreatureHitReactionComponent::HandleDamaged);
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
		Found->OnDamaged.RemoveDynamic(this, &UCreatureHitReactionComponent::HandleDamaged);
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

// ---------------------------------------------------------------------------
// Hits: the hit-stop and the stagger
// ---------------------------------------------------------------------------

void UCreatureHitReactionComponent::HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser)
{
	ACreatureBase* Creature = Cast<ACreatureBase>(GetOwner());
	const UHealthComponent* Life = Health.Get();
	const UWorld* World = GetWorld();
	if (!Creature || !Life || !World || IsBoss(*Creature))
	{
		return;
	}
	// The shot's way: from whoever fired it (else what dealt it) through the wound.
	const APawn* Shooter = InstigatedBy ? InstigatedBy->GetPawn() : nullptr;
	const FVector From = Shooter ? Shooter->GetActorLocation() : (DamageCauser ? DamageCauser->GetActorLocation() : HitLocation);
	const FVector Way = HitLocation - From;
	LastShotDirection = Way.IsNearlyZero() ? (Creature->GetActorLocation() - From).GetSafeNormal() : Way.GetSafeNormal();
	bLastHitCritical = bCritical;

	// The health has already dropped; the death event comes right after this one.
	const bool bKilled = Life->GetHealth() <= 0.f;
	const FHitReaction::FResponse Response = Reaction.OnHit(World->GetTimeSeconds(), Damage, Life->GetMaxHealth(), bCritical, bKilled,
		Creature->GetRank());
	if (Response.HitStopSeconds > 0.f)
	{
		StartHitStop();
	}
	// A creature held back (a boss's spell) is walking home, not fighting: no shove.
	if (Response.StaggerSeconds > 0.f && !Creature->IsPassive())
	{
		Shove(*Creature, LastShotDirection, Response.BurstShare);
	}
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

void UCreatureHitReactionComponent::Shove(ACreatureBase& Creature, const FVector& Direction, float BurstShare)
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
	Movement->Velocity += Away * (StaggerShove * Strength / FMath::Sqrt(Size));
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
	SpawnDeathBurst(*Creature);

	// The killer's view punches in: a crit kill, or a Gravebound or worse, harder.
	APlayerController* Player = Cast<APlayerController>(Killer);
	if (Player && Player->IsLocalController())
	{
		const bool bHeavy = bLastHitCritical || Creature->GetRank() >= ECreatureRank::Epic;
		UCameraShakeModifier::Kick(Player, ViewKicks::ForKill(bHeavy));
	}
	StartCorpseKnock(*Creature);
}

void UCreatureHitReactionComponent::SpawnDeathBurst(ACreatureBase& Creature)
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
	const FVector Center = Body ? FVector(Body->Bounds.Origin) : Creature.GetActorLocation();
	const float Size = 100.f * Creature.GetSizeScale();
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	Effects.SpawnDeathBurst(Kind, Center, LastShotDirection, Size, DeathBurstTint(Creature));
	LooterSound::PlayAt(this, DeathBurstCue(Kind), Center, 1.f, LooterSoundRules::PitchForSize(Creature.GetSizeScale()));
	UE_LOG(LogLooter, Verbose, TEXT("%s: death burst %d"), *Creature.GetName(), static_cast<int32>(Kind));
}

void UCreatureHitReactionComponent::StartCorpseKnock(ACreatureBase& Creature)
{
	// A big body takes a killing shot without sliding off (a Soulfed monster's own death is its show).
	const FVector Away = LastShotDirection.GetSafeNormal2D();
	if (Away.IsNearlyZero() || Creature.GetRank() >= ECreatureRank::Legendary)
	{
		return;
	}
	const float Size = FMath::Max(Creature.GetSizeScale(), 0.3f);
	KnockVelocity = Away * ((bLastHitCritical ? CritCorpseKnock : CorpseKnock) / FMath::Sqrt(Size));
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
		SetComponentTickEnabled(false);
		return;
	}
	const FVector From = Owner->GetActorLocation();
	const ACreatureBase* Creature = Cast<ACreatureBase>(Owner);
	const float Size = Creature ? FMath::Max(Creature->GetSizeScale(), 0.3f) : 1.f;
	FVector Ground;
	if (!bKnockMeasured)
	{
		bKnockMeasured = true;
		if (!FindGround(From, 50.f * Size, 300.f * Size, Ground))
		{
			SetComponentTickEnabled(false);
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
		SetComponentTickEnabled(false);
		return;
	}
	Owner->SetActorLocation(FVector(To.X, To.Y, NewZ));
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

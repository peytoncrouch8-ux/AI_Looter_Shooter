#include "Creatures/CreatureBase.h"
#include "AI_Looter_Shooter.h"
#include "Combat/CombatRules.h"
#include "Combat/HealthComponent.h"
#include "Combat/LooterDamageTypes.h"
#include "Creatures/CreatureRankSettings.h"
#include "Loot/LootDropComponent.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "UI/World/CreatureHealthBarWidget.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
	constexpr float PerceptionInterval = 0.2f;
}

ACreatureBase::ACreatureBase()
{
	PrimaryActorTick.bCanEverTick = true;
	// Tick after movement so the procedural body animates against this frame's position (no leg lag).
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();
	bUseControllerRotationYaw = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 320.f, 0.f);
	Movement->MaxWalkSpeed = WalkSpeed;
	Movement->SetWalkableFloorAngle(50.f);
	Movement->MaxStepHeight = 55.f;

	// Shots are taken by the hit zones on the subclass's mesh, never by the movement capsule.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	Loot = CreateDefaultSubobject<ULootDropComponent>(TEXT("Loot"));

	HealthBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
	HealthBar->SetupAttachment(GetCapsuleComponent());
	HealthBar->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBar->SetDrawAtDesiredSize(true);
	HealthBar->SetPivot(FVector2D(0.5f, 1.f));
	HealthBar->SetWidgetClass(UCreatureHealthBarWidget::StaticClass());
	HealthBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HealthBar->SetVisibility(false);
}

void ACreatureBase::BeginPlay()
{
	Super::BeginPlay();

	// Creatures are authored at their real size and always stand upright, whatever the placement tool did. Their area
	// then gives them their level and may promote them for this arrival, and their rank sets their stats, loot and size
	// (BodyScale times the rank's), before they settle on the ground.
	SetActorScale3D(FVector::OneVector);
	SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
	BeginRankAndLevel();
	SnapToGround();
	Home = GetActorTransform();

	// Loot: the drop component falls back to the default loot table (ammo every kill, weapons on some); a ranked creature
	// carries its rank's instead (ApplyRank).
	Health->OnDamaged.AddDynamic(this, &ACreatureBase::HandleDamaged);
	Health->OnDeath.AddDynamic(this, &ACreatureBase::HandleDeath);

	HealthBar->SetRelativeLocation(FVector(0.f, 0.f, HealthBarHeight));
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	SetState(ECreatureState::Idle);
	IdleDuration = FMath::FRandRange(1.f, 3.f);
	PerceptionTimer = FMath::FRandRange(0.f, PerceptionInterval);

	// Update rate: starts at every frame and picks its band at the first check, spread out so the creatures don't all
	// check on the same frame. The mesh's own animation settings are what it goes back to after a frozen spell.
	AwakeAnimTickOption = GetMesh()->VisibilityBasedAnimTickOption;
	bAwakeUsesScreenRenderState = GetMesh()->bUseScreenRenderStateForUpdate != 0;
	UpdateRateCheckTime = FMath::FRandRange(0.f, UpdateRate.CheckInterval);
}

void ACreatureBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	// AI controllers aren't cleaned up with their pawn; don't leave one behind per removed creature.
	if (AController* Brain = GetController())
	{
		if (!Brain->IsA<APlayerController>())
		{
			Brain->Destroy();
		}
	}
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// Critical spots
// ---------------------------------------------------------------------------

bool ACreatureBase::IsCriticalSpot(const FHitResult& Hit) const
{
	return Hit.GetComponent() == GetMesh() && !Hit.BoneName.IsNone() && CriticalSpotBones.Contains(Hit.BoneName);
}

// ---------------------------------------------------------------------------
// Brain
// ---------------------------------------------------------------------------

void ACreatureBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// First, so the subclass knows whether to pose its body this update.
	TickUpdateRate(DeltaSeconds);
	TickBrain(DeltaSeconds);
	UpdateHealthBar(DeltaSeconds);
}

void ACreatureBase::SetState(ECreatureState NewState)
{
	const ECreatureState OldState = State;
	if (OldState != NewState)
	{
		UE_LOG(LogLooter, Verbose, TEXT("%s: %s -> %s"), *GetName(), *UEnum::GetValueAsString(OldState), *UEnum::GetValueAsString(NewState));
	}
	State = NewState;
	StateTime = 0.f;
	StuckTime = 0.f;
	EscapeTime = 0.f;
	SteerTimer = 0.f;

	// Hunting and attacking always run every frame, wherever it is.
	if (NewState == ECreatureState::Chase || NewState == ECreatureState::Attack)
	{
		WakeUpdateRate();
	}

	if (NewState == ECreatureState::Attack)
	{
		bStruck = false;
		GetCharacterMovement()->StopMovementImmediately();
		OnAttackStarted();
	}
}

void ACreatureBase::TickBrain(float DeltaSeconds)
{
	StateTime += DeltaSeconds;
	CooldownRemaining = FMath::Max(0.f, CooldownRemaining - DeltaSeconds);
	if (State == ECreatureState::Dead)
	{
		return;
	}

	PerceptionTimer -= DeltaSeconds;
	if (PerceptionTimer <= 0.f)
	{
		PerceptionTimer = PerceptionInterval;
		UpdatePerception();
	}

	switch (State)
	{
	case ECreatureState::Idle:
		if (StateTime >= IdleDuration)
		{
			if (PickWanderGoal())
			{
				SetState(ECreatureState::Wander);
			}
			else
			{
				StateTime = 0.f;
			}
		}
		break;

	case ECreatureState::Wander:
		MoveToward(WanderGoal, WalkSpeed, DeltaSeconds);
		if (FVector::DistSquared2D(GetActorLocation(), WanderGoal) < FMath::Square(90.f * SizeScale) || StateTime > 10.f)
		{
			SetState(ECreatureState::Idle);
			IdleDuration = FMath::FRandRange(2.f, 5.f);
		}
		break;

	case ECreatureState::Chase:
	{
		const APawn* Victim = Target.Get();
		if (!Victim)
		{
			SetState(ECreatureState::Return);
			break;
		}
		const float Distance = FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation());
		const float StrikeFrom = GetAttackRange();
		if (Distance <= StrikeFrom && CooldownRemaining <= 0.f && CanStartAttack())
		{
			SetState(ECreatureState::Attack);
		}
		else if (Distance > StrikeFrom * 0.75f)
		{
			MoveToward(Victim->GetActorLocation(), ChaseSpeed, DeltaSeconds);
		}
		else
		{
			// Close but still recovering: square up for the next bite.
			FaceToward(Victim->GetActorLocation(), DeltaSeconds);
		}
		break;
	}

	case ECreatureState::Attack:
		TickAttack(DeltaSeconds);
		break;

	case ECreatureState::Return:
		MoveToward(Home.GetLocation(), WalkSpeed * 1.5f, DeltaSeconds);
		if (FVector::DistSquared2D(GetActorLocation(), Home.GetLocation()) < FMath::Square(120.f * SizeScale) || StateTime > 20.f)
		{
			SetState(ECreatureState::Idle);
			IdleDuration = 2.f;
		}
		break;

	default:
		break;
	}
}

void ACreatureBase::UpdatePerception()
{
	if (State == ECreatureState::Attack)
	{
		return;
	}

	if (const APawn* Current = Target.Get())
	{
		const bool bLost = !IsValidTarget(Current)
			|| FVector::Dist(GetActorLocation(), Current->GetActorLocation()) > LoseInterestRadius;
		if (bLost)
		{
			Target.Reset();
			SetState(ECreatureState::Return);
		}
		return;
	}

	if (APawn* Seen = FindVisibleTarget())
	{
		Target = Seen;
		SetState(ECreatureState::Chase);
		// Being hunted counts as meeting it: its bestiary page opens.
		UPlayerProgressionSubsystem::RecordEncounter(Seen->GetController(), this);
	}
}

APawn* ACreatureBase::FindVisibleTarget() const
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		if (IsValidTarget(Pawn)
			&& FVector::DistSquared(GetActorLocation(), Pawn->GetActorLocation()) <= FMath::Square(AggroRadius)
			&& HasLineOfSight(Pawn))
		{
			return Pawn;
		}
	}
	return nullptr;
}

bool ACreatureBase::IsValidTarget(const APawn* Pawn) const
{
	// Only living characters (not spectator cameras, not other creatures).
	if (!Pawn || !Pawn->IsA<ACharacter>() || Pawn->IsA<ACreatureBase>())
	{
		return false;
	}
	const UHealthComponent* TargetHealth = Pawn->FindComponentByClass<UHealthComponent>();
	return TargetHealth && !TargetHealth->IsDead();
}

bool ACreatureBase::HasLineOfSight(const AActor* Other) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CreatureSight), false, this);
	FHitResult Hit;
	const FVector Eye = GetActorLocation() + FVector(0.f, 0.f, 40.f * SizeScale);
	const FVector TargetPoint = Other->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	return !GetWorld()->LineTraceSingleByChannel(Hit, Eye, TargetPoint, ECC_Visibility, Params) || Hit.GetActor() == Other;
}

// ---------------------------------------------------------------------------
// Attack
// ---------------------------------------------------------------------------

void ACreatureBase::TickAttack(float DeltaSeconds)
{
	APawn* Victim = Target.Get();
	if (Victim && StateTime < AttackWindup)
	{
		// Track the target through the wind-up; commit to the direction once the strike starts.
		FaceToward(Victim->GetActorLocation(), DeltaSeconds);
	}

	if (!bStruck && StateTime >= AttackWindup)
	{
		bStruck = true;
		Strike();
	}

	if (StateTime >= AttackWindup + AttackRecovery)
	{
		CooldownRemaining = AttackCooldown;
		SetState(IsValidTarget(Victim) ? ECreatureState::Chase : ECreatureState::Return);
	}
}

void ACreatureBase::Strike()
{
	bool bConnected = false;
	APawn* Victim = Target.Get();
	if (IsValidTarget(Victim))
	{
		const FVector ToVictim = Victim->GetActorLocation() - GetActorLocation();
		// Up and down, a small creature still reaches a player standing over it, as a full-size one does.
		const bool bInReach = ToVictim.Size2D() <= GetStrikeReach() && FMath::Abs(ToVictim.Z) < 220.f * FMath::Max(SizeScale, 1.f);
		const bool bInFront = FVector::DotProduct(GetActorForwardVector(), ToVictim.GetSafeNormal2D()) > 0.4f;
		if (bInReach && bInFront)
		{
			HitWithAttack(Victim, ToVictim.GetSafeNormal2D());
			bConnected = true;
		}
	}
	// The lunge is purely visual (the subclass animates it); a physical shove would push the creature into
	// and past a target standing in melee range.
	OnAttackStrike(bConnected);
}

void ACreatureBase::HitWithAttack(APawn* Victim, const FVector& Push)
{
	// Attacks roll in the same damage range as player weapons.
	const float Damage = LooterCombat::RollHitDamage(AttackDamage, false);
	UGameplayStatics::ApplyDamage(Victim, Damage, GetController(), this, UCreatureAttackDamageType::StaticClass());
	UE_LOG(LogLooter, Verbose, TEXT("%s hit %s for %.1f"), *GetName(), *GetNameSafe(Victim), Damage);
	// Shove the victim back so the hit is felt, not just read on the health bar.
	if (ACharacter* VictimCharacter = Cast<ACharacter>(Victim))
	{
		VictimCharacter->LaunchCharacter(Push * 450.f + FVector(0.f, 0.f, 180.f), true, false);
	}
}

// ---------------------------------------------------------------------------
// Damage, death and respawn
// ---------------------------------------------------------------------------

void ACreatureBase::HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser)
{
	UE_LOG(LogLooter, Verbose, TEXT("%s took %.1f%s (%.0f / %.0f)"), *GetName(), Damage, bCritical ? TEXT(" CRIT") : TEXT(""),
		Health->GetHealth(), Health->GetMaxHealth());
	HealthBarTime = 6.f;
	// A hurt creature updates every frame for a while, wherever it is, so its flinch (and death) plays smoothly. Before
	// OnHurt: a frozen body is set up afresh first.
	FullRateTime = UpdateRate.HurtFullRateTime;
	WakeUpdateRate();
	OnHurt(bCritical, HitLocation);

	if (State == ECreatureState::Dead)
	{
		return;
	}
	// Getting shot always gets its attention, even from beyond its sight range.
	APawn* Attacker = InstigatedBy ? InstigatedBy->GetPawn() : nullptr;
	if (!Target.IsValid() && IsValidTarget(Attacker))
	{
		Target = Attacker;
		if (State != ECreatureState::Attack)
		{
			SetState(ECreatureState::Chase);
		}
	}
	// A pack turns on whoever hurts one of them: every creature of its pack (its PackTag) within its call, which a rank
	// can widen (a Gravebound spider calls every spider near it).
	const float CallRadius = GetPackCallRadius();
	if (CallRadius > 0.f && IsValidTarget(Attacker))
	{
		for (TActorIterator<ACreatureBase> It(GetWorld()); It; ++It)
		{
			if (*It != this && It->SharesPackWith(*this)
				&& FVector::DistSquared(It->GetActorLocation(), GetActorLocation()) <= FMath::Square(CallRadius))
			{
				It->AlertTo(Attacker);
			}
		}
	}
}

void ACreatureBase::AlertTo(APawn* Attacker)
{
	if (State == ECreatureState::Dead || Target.IsValid() || !IsValidTarget(Attacker))
	{
		return;
	}
	Target = Attacker;
	if (State != ECreatureState::Attack)
	{
		SetState(ECreatureState::Chase);
	}
	UPlayerProgressionSubsystem::RecordEncounter(Attacker->GetController(), this);
}

void ACreatureBase::HandleDeath(AController* Killer)
{
	// Loot drops on its own (ULootDropComponent listens to the same death event). The health component reports a death
	// once until the respawn resets it, so each kill gives its experience once.
	UPlayerProgressionSubsystem::AwardKill(Killer, this);
	Target.Reset();
	SetState(ECreatureState::Dead);
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->StopMovementImmediately();
	Movement->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetHitVolumesEnabled(false);
	HealthBarTime = 0.f;
	OnDied();

	FTimerManager& Timers = GetWorldTimerManager();
	// The subclass sinks the corpse after CorpseTime; hide it once it's under the ground. One spawned in play is gone for
	// good, so it goes then (a placed one that won't come back stays, hidden, for its area to bring back).
	Timers.SetTimer(HideTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		SetActorHiddenInGame(true);
		if (bSpawnedAtRuntime)
		{
			Destroy();
		}
	}), CorpseTime + 2.5f, false);
	if (WillRespawn())
	{
		Timers.SetTimer(RespawnTimer, this, &ACreatureBase::Respawn, CorpseTime + 2.5f + RespawnDelay, false);
	}
}

void ACreatureBase::Respawn()
{
	// It comes back as it started (a promotion lasts one life), at a level that follows the player's now. (Before the move
	// home: the size it comes back at is the one its home spot was found for.)
	RespawnRankAndLevel();
	SetActorLocationAndRotation(Home.GetLocation(), Home.GetRotation(), false, nullptr, ETeleportType::ResetPhysics);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	Health->ResetHealth();
	SetHitVolumesEnabled(true);
	SetActorHiddenInGame(false);
	CooldownRemaining = 0.f;
	SetState(ECreatureState::Idle);
	IdleDuration = 2.f;
	OnRespawned();
}

void ACreatureBase::UpdateHealthBar(float DeltaSeconds)
{
	HealthBarTime = FMath::Max(0.f, HealthBarTime - DeltaSeconds);
	const bool bShow = State != ECreatureState::Dead && (HealthBarTime > 0.f || Target.IsValid());
	HealthBar->SetVisibility(bShow);
	if (bShow)
	{
		if (UCreatureHealthBarWidget* Bar = Cast<UCreatureHealthBarWidget>(HealthBar->GetUserWidgetObject()))
		{
			const FCreatureRankInfo& RankInfo = UCreatureRankSettings::Get(CurrentRank);
			Bar->SetCreature(DisplayName, Level, RankInfo.Word, RankInfo.Color);
			Bar->SetHealth(Health->GetHealth(), Health->GetMaxHealth());
		}
	}
}

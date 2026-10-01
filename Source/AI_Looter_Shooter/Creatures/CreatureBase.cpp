#include "Creatures/CreatureBase.h"
#include "AI_Looter_Shooter.h"
#include "Combat/CombatRules.h"
#include "Combat/HealthComponent.h"
#include "Combat/LooterDamageTypes.h"
#include "Loot/LootDropComponent.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "UI/World/CreatureHealthBarWidget.h"
#include "World/WorldQueries.h"
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
	constexpr float SteerInterval = 0.1f;
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

	// Creatures are authored at their real size and always stand upright, whatever the placement tool did.
	SetActorScale3D(FVector::OneVector);
	SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
	SnapToGround();
	Home = GetActorTransform();

	// Loot: the drop component falls back to the default loot table (ammo every kill, weapons on some).
	Health->OnDamaged.AddDynamic(this, &ACreatureBase::HandleDamaged);
	Health->OnDeath.AddDynamic(this, &ACreatureBase::HandleDeath);

	HealthBar->SetRelativeLocation(FVector(0.f, 0.f, HealthBarHeight));
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	SetState(ECreatureState::Idle);
	IdleDuration = FMath::FRandRange(1.f, 3.f);
	PerceptionTimer = FMath::FRandRange(0.f, PerceptionInterval);
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
		if (FVector::DistSquared2D(GetActorLocation(), WanderGoal) < FMath::Square(90.f) || StateTime > 10.f)
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
		if (Distance <= AttackRange && CooldownRemaining <= 0.f && CanStartAttack())
		{
			SetState(ECreatureState::Attack);
		}
		else if (Distance > AttackRange * 0.75f)
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
		if (FVector::DistSquared2D(GetActorLocation(), Home.GetLocation()) < FMath::Square(120.f) || StateTime > 20.f)
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
	const FVector Eye = GetActorLocation() + FVector(0.f, 0.f, 40.f);
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
		const bool bInReach = ToVictim.Size2D() <= AttackRange + 70.f && FMath::Abs(ToVictim.Z) < 220.f;
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
// Movement
// ---------------------------------------------------------------------------

void ACreatureBase::MoveToward(const FVector& Goal, float Speed, float DeltaSeconds)
{
	FVector ToGoal = Goal - GetActorLocation();
	ToGoal.Z = 0.f;
	if (ToGoal.SizeSquared() < 1.f)
	{
		return;
	}
	GetCharacterMovement()->MaxWalkSpeed = Speed;

	if (EscapeTime > 0.f)
	{
		EscapeTime -= DeltaSeconds;
		AddMovementInput(EscapeDirection);
		return;
	}

	SteerTimer -= DeltaSeconds;
	if (SteerTimer <= 0.f)
	{
		SteerTimer = SteerInterval;
		SteerDirection = ChooseDirection(ToGoal.GetSafeNormal());
	}
	if (!SteerDirection.IsNearlyZero())
	{
		AddMovementInput(SteerDirection);
	}

	// Wedged against something the probes didn't see: take a short detour in a random clear direction.
	StuckTime = IsStuck(Speed) ? StuckTime + DeltaSeconds : 0.f;
	if (StuckTime > 0.8f)
	{
		StuckTime = 0.f;
		for (int32 Attempt = 0; Attempt < 8; ++Attempt)
		{
			const FVector Candidate = ToGoal.GetSafeNormal().RotateAngleAxis(FMath::FRandRange(60.f, 150.f) * (FMath::RandBool() ? 1.f : -1.f), FVector::UpVector);
			if (IsDirectionClear(Candidate))
			{
				EscapeDirection = Candidate;
				EscapeTime = 0.9f;
				break;
			}
		}
	}
}

bool ACreatureBase::IsStuck(float Speed) const
{
	return GetVelocity().Size2D() < Speed * 0.15f;
}

FVector ACreatureBase::ChooseDirection(const FVector& Desired)
{
	// Straight at the goal if possible, otherwise fan out, favoring the side that worked last time so it
	// commits to going around an obstacle instead of dithering.
	static const float Offsets[] = { 0.f, 30.f, 60.f, 90.f, 125.f };
	for (const float Offset : Offsets)
	{
		for (const float Side : { PreferredSide, -PreferredSide })
		{
			const FVector Candidate = Desired.RotateAngleAxis(Offset * Side, FVector::UpVector);
			if (IsDirectionClear(Candidate))
			{
				if (Offset > 0.f)
				{
					PreferredSide = Side;
				}
				return Candidate;
			}
			if (Offset == 0.f)
			{
				break;
			}
		}
	}
	return FVector::ZeroVector;
}

bool ACreatureBase::IsDirectionClear(const FVector& Direction) const
{
	const FVector Start = GetActorLocation();
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(CreatureSteer), false, this);
	if (const APawn* Victim = Target.Get())
	{
		Params.AddIgnoredActor(Victim);
	}

	// Obstacles: anything that blocks a walking pawn and is too steep to walk up.
	FHitResult Hit;
	const FCollisionShape Probe = FCollisionShape::MakeSphere(Capsule->GetScaledCapsuleRadius() * 0.8f);
	if (GetWorld()->SweepSingleByChannel(Hit, Start, Start + Direction * 220.f, FQuat::Identity, ECC_Pawn, Probe, Params)
		&& Hit.ImpactNormal.Z < 0.65f)
	{
		return false;
	}

	// Ledges: never walk off the edge of an island.
	FVector Ground;
	return FindGround(Start + Direction * 160.f, 0.f, HalfHeight + 350.f, Ground);
}

void ACreatureBase::FaceToward(const FVector& Point, float DeltaSeconds)
{
	const FVector To = Point - GetActorLocation();
	if (To.SizeSquared2D() < 1.f)
	{
		return;
	}
	const FRotator Wanted(0.f, To.Rotation().Yaw, 0.f);
	SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), Wanted, DeltaSeconds, 540.f));
}

bool ACreatureBase::PickWanderGoal()
{
	for (int32 Attempt = 0; Attempt < 6; ++Attempt)
	{
		const FVector2D Offset = FMath::RandPointInCircle(WanderRadius);
		FVector Ground;
		if (Offset.Size() > 150.f && FindGround(Home.GetLocation() + FVector(Offset, 0.f), 300.f, 600.f, Ground))
		{
			WanderGoal = Ground;
			return true;
		}
	}
	return false;
}

bool ACreatureBase::FindGround(const FVector& Point, float Above, float Below, FVector& OutGround) const
{
	// World-static only: terrain and solid props, never grass, pawns, loot or volumes.
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(GetWorld(), TEXT("CreatureGround"), this);
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByObjectType(Hit, Point + FVector(0.f, 0.f, Above), Point - FVector(0.f, 0.f, Below),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		OutGround = Hit.ImpactPoint;
		return true;
	}
	return false;
}

void ACreatureBase::SnapToGround()
{
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector Ground;
	if (FindGround(GetActorLocation(), HalfHeight + 150.f, 1500.f, Ground))
	{
		SetActorLocation(Ground + FVector(0.f, 0.f, HalfHeight + 2.f), false, nullptr, ETeleportType::TeleportPhysics);
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
	// A pack turns on whoever hurts one of them.
	if (PackAlertRadius > 0.f && IsValidTarget(Attacker))
	{
		for (TActorIterator<ACreatureBase> It(GetWorld()); It; ++It)
		{
			if (*It != this && It->GetClass() == GetClass()
				&& FVector::DistSquared(It->GetActorLocation(), GetActorLocation()) <= FMath::Square(PackAlertRadius))
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
	// The subclass sinks the corpse after CorpseTime; hide it once it's under the ground.
	Timers.SetTimer(HideTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { SetActorHiddenInGame(true); }), CorpseTime + 2.5f, false);
	if (bRespawns)
	{
		Timers.SetTimer(RespawnTimer, this, &ACreatureBase::Respawn, CorpseTime + 2.5f + RespawnDelay, false);
	}
}

void ACreatureBase::Respawn()
{
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
			Bar->SetCreature(DisplayName, Level);
			Bar->SetHealth(Health->GetHealth(), Health->GetMaxHealth());
		}
	}
}

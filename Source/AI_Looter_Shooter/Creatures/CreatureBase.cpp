#include "Creatures/CreatureBase.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Combat/CombatRules.h"
#include "Combat/HealthComponent.h"
#include "Combat/LooterDamageTypes.h"
#include "Creatures/CreatureHitReactionComponent.h"
#include "Creatures/CreaturePackComponent.h"
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
	Voice = CreateDefaultSubobject<UCreatureVoiceComponent>(TEXT("Voice"));
	HitReaction = CreateDefaultSubobject<UCreatureHitReactionComponent>(TEXT("HitReaction"));
	Pack = CreateDefaultSubobject<UCreaturePackComponent>(TEXT("Pack"));

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
	// Standing still, it shuffles out of a neighbour's footprint (moving, its steering keeps clear: CreatureBaseSpacing.cpp).
	KeepSpacing(DeltaSeconds);
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
	SteerTimer = 0.f;
	// Heard as it turns on a player, and at an attack's wind-up (its tell); a ranked one's sting, and its pack's flank.
	Voice->HandleStateChanged(OldState, NewState);
	Pack->HandleStateChanged(OldState, NewState);

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
	// Staggered by a heavy hit: its brain waits with its state's clock held, so a wind-up holds rather than lands.
	if (HitReaction->IsStaggered())
	{
		StateTime -= DeltaSeconds;
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
		// A patrol's creature sets off as soon as its anchor walks on.
		if (StateTime >= IdleDuration || Pack->WantsToRoam(GetActorLocation()))
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
	{
		// A patrol's creature walks with its anchor (at its pace) and stands only once the patrol rests.
		FVector Goal = WanderGoal;
		float Pace = WalkSpeed;
		const bool bRoaming = Pack->GetRoamMove(GetActorLocation(), WalkSpeed, Goal, Pace);
		MoveToward(Goal, Pace, DeltaSeconds);
		const bool bThere = FVector::DistSquared2D(GetActorLocation(), Goal) < FMath::Square(90.f * SizeScale);
		if (bRoaming ? (bThere && !Pack->IsRoamAnchorMoving()) : (bThere || StateTime > 10.f))
		{
			SetState(ECreatureState::Idle);
			IdleDuration = FMath::FRandRange(2.f, 5.f);
		}
		break;
	}

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
		// Its pack: its flank among those chasing the same player, or breaking off once its pack is gone (it then runs and
		// starts no attack: CreaturePackComponent).
		Pack->TickChase(DeltaSeconds);
		const bool bBreakingOff = Pack->IsRetreating();
		// An attack of its own may start from farther out than its reach (a charge); otherwise it closes in to bite.
		if (!bBreakingOff && Distance <= GetAttackStartRange() && CooldownRemaining <= 0.f && CanStartAttack())
		{
			SetState(ECreatureState::Attack);
		}
		else if (bBreakingOff || Distance > StrikeFrom * 0.75f)
		{
			MoveToward(Pack->GetChaseGoal(*Victim), bBreakingOff ? ChaseSpeed * Pack->GetRetreatSpeedShare() : ChaseSpeed, DeltaSeconds);
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
	{
		// Home, or for a patrol's creature its place in the file, wherever the patrol has got to.
		const FVector Back = Pack->HasRoamAnchor() ? Pack->GetRoamAnchor() : Home.GetLocation();
		MoveToward(Back, WalkSpeed * 1.5f, DeltaSeconds);
		if (FVector::DistSquared2D(GetActorLocation(), Back) < FMath::Square(120.f * SizeScale) || StateTime > 20.f)
		{
			SetState(ECreatureState::Idle);
			IdleDuration = 2.f;
		}
		break;
	}

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
	// Held back (a boss under a spell): it lets go of whoever it was after and hunts nobody.
	if (bPassive)
	{
		if (Target.IsValid())
		{
			Target.Reset();
			SetState(ECreatureState::Return);
		}
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
	if (Victim && StateTime < AttackWindup && TracksTargetInWindup())
	{
		// Track the target through the wind-up; commit to the direction once the strike starts (or once a charge has
		// taken its aim).
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

void ACreatureBase::HitWithAttack(APawn* Victim, const FVector& Push, float Strength)
{
	// Attacks roll in the same damage range as player weapons.
	const float Hardness = FMath::Max(Strength, 0.f);
	const float Damage = LooterCombat::RollHitDamage(AttackDamage * Hardness, false);
	UGameplayStatics::ApplyDamage(Victim, Damage, GetController(), this, UCreatureAttackDamageType::StaticClass());
	UE_LOG(LogLooter, Verbose, TEXT("%s hit %s for %.1f"), *GetName(), *GetNameSafe(Victim), Damage);
	// Shove the victim back so the hit is felt, not just read on the health bar.
	if (ACharacter* VictimCharacter = Cast<ACharacter>(Victim))
	{
		VictimCharacter->LaunchCharacter((Push * 450.f + FVector(0.f, 0.f, 180.f)) * Hardness, true, false);
	}
}

// ---------------------------------------------------------------------------
// Death and respawn (being hurt, its pack's call and the tag over it: CreatureBaseHurt.cpp)
// ---------------------------------------------------------------------------

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
	// A new life: its rank may sting again, and it may break off again.
	Pack->ResetLife();
	OnRespawned();
}

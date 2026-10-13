#include "Creatures/ForemanFuseStick.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossCameraShake.h"
#include "Bosses/BossComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/CombatRules.h"
#include "Combat/GraveSaltBurst.h"
#include "Combat/HealthComponent.h"
#include "Combat/LooterDamageTypes.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/ForemanFight.h"
#include "Creatures/GroundCrack.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const TCHAR* const AForemanFuseStick::ModelPath = TEXT("/Game/Art/Creatures/SM_ForemanFuseStick.SM_ForemanFuseStick");

namespace
{
	const TCHAR* const CylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");

	/** The stand-in stick: 4 cm thick, 22 cm long; the fuse's end this far up its axis from its middle (cm). */
	constexpr float StandInThick = 4.f;
	constexpr float StandInLong = 22.f;
	constexpr float FuseEndUp = 13.f;

	/** Its fall: the world's gravity, a 5 cm ball for its sweep. */
	constexpr float Gravity = 980.f;
	constexpr float SweepRadius = 5.f;

	/** The fuse's sparks: orange, a few short streaks a frame. */
	const FLinearColor SparkColor(1.f, 0.55f, 0.15f);
	constexpr int32 SparksPerFrame = 3;

	/** Its little light: a fuse's flicker (candela) and reach (cm). */
	constexpr float GlowCandela = 5.f;
	constexpr float GlowRadius = 320.f;

	/** The burst's mark on the floor, as the roof's (a ring of cracks), in the powder's hot orange. */
	const FLinearColor MarkColor(1.f, 0.45f, 0.12f);
	constexpr float MarkWidth = 16.f;

	/** The knock off a burst's heart (cm/s, out and up), at full strength; the view's jolt and how far it's felt. */
	constexpr float KnockOut = 520.f;
	constexpr float KnockUp = 260.f;
	constexpr float BurstShake = 0.75f;
	constexpr float BurstShakeRadius = 3500.f;

	/** A dropped stick's burst hurts a creature this many times its damage to a player (powder in a crowd). */
	constexpr float CreatureScale = 3.f;

	/** It goes this long after its burst (its sound and smoke are the effects' own). */
	constexpr float GoneAfterBurst = 0.2f;

	template <typename T>
	T* FindIfMade(const TCHAR* ObjectPath)
	{
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath))))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<T> Finder(ObjectPath);
		return Finder.Object;
	}

	FWeaponFX* EffectsIn(UWorld* World)
	{
		UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
		if (!Bullets)
		{
			return nullptr;
		}
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		return &Effects;
	}
}

AForemanFuseStick::AForemanFuseStick()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	SetCanBeDamaged(false);

	static UStaticMesh* const Model = FindIfMade<UStaticMesh>(ModelPath);
	static UStaticMesh* const Cylinder = FindIfMade<UStaticMesh>(CylinderPath);
	Stick = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Stick"));
	RootComponent = Stick;
	Stick->SetMobility(EComponentMobility::Movable);
	Stick->SetStaticMesh(Model ? Model : Cylinder);
	if (!Model)
	{
		Stick->SetRelativeScale3D(FVector(StandInThick / 100.f, StandInThick / 100.f, StandInLong / 100.f));
	}
	// Nothing meets it: it flies by its own sweep and hurts by its own rule.
	Stick->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Stick->SetCanEverAffectNavigation(false);
	Stick->SetCastShadow(false);

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Stick);
	Glow->SetMobility(EComponentMobility::Movable);
	Glow->SetCastShadows(false);
	Glow->SetIntensityUnits(ELightUnits::Candelas);
	Glow->SetIntensity(GlowCandela);
	Glow->SetAttenuationRadius(GlowRadius);
	Glow->SetLightColor(SparkColor);
	// The light sits at the fuse whatever the stick's scale (absolute scale, so the stand-in's thinness doesn't squash it).
	Glow->SetUsingAbsoluteScale(true);
}

AForemanFuseStick* AForemanFuseStick::SpawnLit(UWorld* World, AActor* By, USceneComponent* Hand, FName Bone, const FVector& OnHand)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Where = Hand ? Hand->GetSocketLocation(Bone) : FVector::ZeroVector;
	AForemanFuseStick* Lit = World->SpawnActor<AForemanFuseStick>(Where, FRotator::ZeroRotator, Params);
	if (!Lit)
	{
		return nullptr;
	}
	Lit->Thrower = By;
	if (Hand)
	{
		Lit->AttachToComponent(Hand, FAttachmentTransformRules::KeepRelativeTransform, Bone);
		Lit->SetActorRelativeLocation(OnHand);
	}
	Lit->Stage = EStage::Held;
	// The match struck and the fuse catching, then its hiss for as long as it burns.
	LooterSound::PlayAt(Lit, LooterSoundCue::Foreman::FuseLight, Lit->GetFuseEnd());
	Lit->Hiss = LooterSound::Start(Lit, LooterSoundCue::Foreman::FuseHiss, Lit->Stick);
	return Lit;
}

FVector AForemanFuseStick::GetFuseEnd() const
{
	return Stick->GetComponentLocation() + Stick->GetUpVector() * FuseEndUp;
}

void AForemanFuseStick::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopHiss();
	if (AGroundCrack* Ring = Mark.Get())
	{
		Ring->Close(0.f);
	}
	Mark.Reset();
	Super::EndPlay(EndPlayReason);
}

void AForemanFuseStick::StopHiss()
{
	LooterSound::Stop(Hiss.Get(), 0.05f);
	Hiss.Reset();
}

// ---------------------------------------------------------------------------
// Thrown, dropped, put out
// ---------------------------------------------------------------------------

void AForemanFuseStick::Lob(const FVector& From, const FVector& Target, float FlightSeconds)
{
	if (Stage != EStage::Held)
	{
		return;
	}
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorLocation(From);
	const float Time = FMath::Max(FlightSeconds, 0.2f);
	// Thrown so its arc comes down on the target after Time: across at an even pace, up enough to fall the rest.
	Velocity = (Target - From) / Time + FVector(0.0, 0.0, 0.5 * Gravity * Time);
	Stage = EStage::Flying;
	Flight = 0.f;
	LooterSound::PlayAt(this, LooterSoundCue::Foreman::FuseThrow, From);
}

void AForemanFuseStick::Drop(AController* CreditTo)
{
	if (Stage != EStage::Held && Stage != EStage::Flying)
	{
		return;
	}
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	bDropped = true;
	Credit = CreditTo;
	Velocity = FVector(0.0, 0.0, -50.0);
	Stage = EStage::Flying;
	Flight = 0.f;
}

void AForemanFuseStick::Defuse()
{
	if (Stage == EStage::Burst || Stage == EStage::Out)
	{
		return;
	}
	Stage = EStage::Out;
	StopHiss();
	Destroy();
}

// ---------------------------------------------------------------------------
// Frame by frame
// ---------------------------------------------------------------------------

void AForemanFuseStick::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void AForemanFuseStick::Advance(float DeltaSeconds)
{
	Clock += DeltaSeconds;
	switch (Stage)
	{
	case EStage::Held:
		Spark(DeltaSeconds);
		break;
	case EStage::Flying:
	{
		Spark(DeltaSeconds);
		Flight += DeltaSeconds;
		UWorld* World = GetWorld();
		// Short steps, so a fast lob never steps through a floor; it tumbles end over end as it goes.
		const int32 Steps = FMath::Clamp(FMath::CeilToInt32(DeltaSeconds * 60.f), 1, 6);
		const float Step = DeltaSeconds / static_cast<float>(Steps);
		for (int32 Each = 0; Each < Steps && Stage == EStage::Flying; ++Each)
		{
			const FVector From = GetActorLocation();
			Velocity.Z -= Gravity * Step;
			const FVector To = From + Velocity * Step;
			FHitResult Hit;
			const bool bHit = World && World->SweepSingleByObjectType(Hit, From, To, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldStatic),
				FCollisionShape::MakeSphere(SweepRadius), LooterWorld::StaticGeometryParams(World, TEXT("FuseStick"), this));
			if (bHit)
			{
				// Off a wall it drops; on the floor it lies where it came down.
				if (Hit.ImpactNormal.Z > 0.6)
				{
					Land(Hit.ImpactPoint);
					break;
				}
				SetActorLocation(Hit.Location);
				Velocity = FVector(Velocity.X * -0.25, Velocity.Y * -0.25, FMath::Min(Velocity.Z, 0.0));
				continue;
			}
			SetActorLocation(To);
		}
		if (Stage == EStage::Flying)
		{
			AddActorWorldRotation(FRotator(-540.f * DeltaSeconds, 0.f, 0.f));
			if (Flight >= MaxFlightSeconds || (bDropped && Flight >= 0.6f))
			{
				// Never came down (a test level's empty air, a pit): it bursts where it is.
				Land(GetActorLocation());
			}
		}
		break;
	}
	case EStage::Landed:
		Spark(DeltaSeconds);
		FuseLeft -= DeltaSeconds;
		if (FuseLeft <= 0.f)
		{
			Burst();
		}
		break;
	case EStage::Burst:
		BurstAge += DeltaSeconds;
		if (BurstAge >= GoneAfterBurst)
		{
			Stage = EStage::Out;
			Destroy();
		}
		break;
	default:
		break;
	}
}

void AForemanFuseStick::Spark(float DeltaSeconds)
{
	const FVector End = GetFuseEnd();
	// The light flickers as the fuse spits.
	Glow->SetWorldLocation(End);
	Glow->SetIntensity(GlowCandela * FMath::FRandRange(0.6f, 1.3f));
	if (FWeaponFX* Effects = EffectsIn(GetWorld()))
	{
		for (int32 Each = 0; Each < SparksPerFrame; ++Each)
		{
			const FVector Way = (FMath::VRand() + GetActorUpVector() * 0.8f).GetSafeNormal();
			Effects->AddStreak(End, End + Way * FMath::FRandRange(3.f, 9.f), 0.6f, SparkColor, 12.f);
		}
	}
}

void AForemanFuseStick::Land(const FVector& Floor)
{
	Stage = EStage::Landed;
	Landing = Floor;
	Velocity = FVector::ZeroVector;
	SetActorLocationAndRotation(Floor + FVector(0.0, 0.0, StandInThick * 0.5f), FRotator(90.0, FMath::FRandRange(0.f, 360.f), 0.0));
	FuseLeft = GroundSeconds;
	LooterSound::PlayAt(this, LooterSoundCue::Foreman::FuseLand, Floor);
	// The mark: cracks spreading on the floor to its reach as its fuse burns down. Get out of the ring.
	if (AGroundCrack* Ring = AGroundCrack::Spawn(GetWorld(), Floor, Floor + FVector::ForwardVector, MarkWidth, MarkColor, FMath::Rand()))
	{
		Ring->BurstSeconds = FMath::Max(GroundSeconds, 0.05f);
		Ring->Burst(Floor, Radius);
		Mark = Ring;
	}
}

void AForemanFuseStick::Burst()
{
	Stage = EStage::Burst;
	BurstAge = 0.f;
	StopHiss();
	Glow->SetVisibility(false);
	Stick->SetVisibility(false);
	if (AGroundCrack* Ring = Mark.Get())
	{
		Ring->Close(0.6f);
	}
	Mark.Reset();
	const FVector Center = (Landing.IsZero() ? GetActorLocation() : Landing) + FVector(0.0, 0.0, 20.0);
	bHitPlayer = HurtAround(Center);
	// A hard flash, rock and dirt thrown up, black powder smoke rolling out, the boom, the view jolted.
	if (FWeaponFX* Effects = EffectsIn(GetWorld()))
	{
		Effects->SpawnFlash(Center, FVector::UpVector, 3.f);
		Effects->SpawnDirt(Center, FVector::UpVector, 1.f);
		for (int32 Puff = 0; Puff < 8; ++Puff)
		{
			const FVector Out = FVector::ForwardVector.RotateAngleAxis(45.f * Puff + FMath::FRandRange(-15.f, 15.f), FVector::UpVector);
			Effects->SpawnDustPuff(Center + Out * 40.f, Out * FMath::FRandRange(220.f, 380.f) + FVector(0.0, 0.0, 80.0),
				FLinearColor(0.08f, 0.075f, 0.07f), 0.7f, 50.f, 220.f, 2.6f);
		}
		for (int32 Chip = 0; Chip < 14; ++Chip)
		{
			Effects->SpawnGrit(Center, FMath::VRand() * 420.f + FVector(0.0, 0.0, 360.0), FMath::FRandRange(3.f, 8.f), 1.8f);
		}
	}
	LooterSound::PlayAt(this, LooterSoundCue::Foreman::FuseBurst, Center);
	BossCameraShake::Kick(this, Center, BurstShake, 0.7f, BurstShakeRadius);
}

bool AForemanFuseStick::HurtAround(const FVector& Center)
{
	UWorld* World = GetWorld();
	if (!World || Damage <= 0.f)
	{
		return false;
	}
	AActor* By = Thrower.Get();
	const APawn* ByPawn = Cast<APawn>(By);
	AController* ByController = ByPawn ? ByPawn->GetController() : nullptr;

	// The players: every local player's pawn, and the encounters' player (a test's stand-in has no controller).
	TArray<APawn*, TInlineAllocator<4>> Players;
	if (const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this))
	{
		if (APawn* Player = Encounters->GetPlayer())
		{
			Players.AddUnique(Player);
		}
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (const APlayerController* Controller = It->Get(); Controller && Controller->GetPawn())
		{
			Players.AddUnique(Controller->GetPawn());
		}
	}
	bool bCaught = false;
	for (APawn* Player : Players)
	{
		const UHealthComponent* Health = Player ? Player->FindComponentByClass<UHealthComponent>() : nullptr;
		ACharacter* Body = Cast<ACharacter>(Player);
		if (!Health || Health->IsDead() || !Body)
		{
			continue;
		}
		const FGraveSaltBody Shape = GraveSaltBurst::BodyOf(*Body);
		const float Share = ForemanFight::BurstFalloff(FGraveSaltRules::DistanceToBody(Center, Shape), Radius, CoreRadius, EdgeShare);
		if (Share <= 0.f || !GraveSaltBurst::CanSee(*World, Center, Shape, Body, By))
		{
			continue;
		}
		const float Dealt = LooterCombat::RollHitDamage(Damage * Share, false);
		UGameplayStatics::ApplyDamage(Player, Dealt, ByController, By, UCreatureAttackDamageType::StaticClass());
		FVector Away = Body->GetActorLocation() - Center;
		Away.Z = 0.0;
		Away = Away.IsNearlyZero() ? FVector::ForwardVector : Away.GetSafeNormal();
		Body->LaunchCharacter((Away * KnockOut + FVector(0.0, 0.0, KnockUp)) * Share, true, false);
		UE_LOG(LogLooter, Log, TEXT("The Foreman's fuse stick catches %s for %.0f."), *GetNameSafe(Player), Dealt);
		bCaught = true;
	}

	// Dropped at his own feet, it hurts creatures too: his shift, and him (capped, as the grave salt caps a boss).
	if (bDropped)
	{
		if (const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this))
		{
			for (const TWeakObjectPtr<ACreatureBase>& Each : Encounters->GetTrackedCreatures())
			{
				ACreatureBase* Creature = Each.Get();
				if (!Creature || Creature->IsDead() || Creature->IsActorBeingDestroyed())
				{
					continue;
				}
				const FGraveSaltBody Shape = GraveSaltBurst::BodyOf(*Creature);
				const float Share = ForemanFight::BurstFalloff(FGraveSaltRules::DistanceToBody(Center, Shape), Radius, CoreRadius, EdgeShare);
				if (Share <= 0.f || !GraveSaltBurst::CanSee(*World, Center, Shape, Creature, nullptr))
				{
					continue;
				}
				float Dealt = LooterCombat::RollHitDamage(Damage * CreatureScale * Share, false);
				if (const UBossComponent* Boss = Creature->FindComponentByClass<UBossComponent>())
				{
					if (const UHealthComponent* BossHealth = Boss->GetBossHealth())
					{
						Dealt = FMath::Min(Dealt, BossHealth->GetMaxHealth() * BossShareCap);
					}
				}
				UGameplayStatics::ApplyDamage(Creature, Dealt, Credit.Get(), this, UDamageType::StaticClass());
			}
		}
	}
	return bCaught;
}

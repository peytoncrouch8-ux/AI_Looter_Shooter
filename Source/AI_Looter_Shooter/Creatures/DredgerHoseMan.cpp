// ADredgerHoseMan: his kit (the tank on his back, the nozzle in his hands, the soul-hose), his turns and his latch, his
// brace while it holds, his club after his tank has burst (the tank and its burst: DredgerHoseManTank.cpp).

#include "Creatures/DredgerHoseMan.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSoundCues.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/DredgerHoseComponent.h"
#include "Creatures/DredgerLook.h"
#include "Creatures/DredgerRules.h"
#include "Creatures/DredgerTuning.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Nearer than this (cm, full size) he clubs with the nozzle rather than throw; the club reaches this far and hits this hard. */
	constexpr float ClubWithin = 200.f;
	constexpr float ClubReach = 260.f;
	constexpr float ClubStrength = 1.3f;
	/** Latched he turns to his victim this fast (degrees a second). */
	constexpr float LatchedTurnRate = 300.f;
	/** His line's way is looked at this often (s). */
	constexpr double LineCheckEvery = 0.25;
	/** The tell's cyan swelling at the nozzle's mouth (cm across at its end). */
	constexpr float ChargeSize = 14.f;
	/** The gauge's glow: at rest, more as he drains, flaring through his tell (times DredgerLook::GaugeGlow), and its ease. */
	constexpr float GaugeDrain = 2.f;
	constexpr float GaugeTell = 3.f;
	constexpr float GaugeEase = 6.f;

	/** The tank and its gauge take shots (weapon and visibility traces only), as the body's hit zones do. */
	void MakeTankHitZone(UPrimitiveComponent* Part)
	{
		Part->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Part->SetCollisionObjectType(ECC_WorldDynamic);
		Part->SetCollisionResponseToAllChannels(ECR_Ignore);
		Part->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
		Part->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}

	/** A turn that lays an engine cylinder (its axis up) along the held thing's barrel (+X). */
	FQuat AlongBarrel()
	{
		return FQuat(FVector::YAxisVector, HALF_PI);
	}
}

ADredgerHoseMan::ADredgerHoseMan()
{
	DredgerRole = EDredgerRole::HoseMan;
	DisplayName = NSLOCTEXT("Dredger", "HoseManName", "Dredger Hose-man");
	const FDredgerTuning Defaults;
	// The class's own; the game's number is Project Settings > Game > Creature Kinds' (170 since 2026-11-03: UCreatureKindSettings).
	Health->MaxHealth = 220.f;
	// His club's blow after the burst; his drain grows from it as he levels (DredgerRules::DrainPerSecond).
	AttackDamage = 10.f;
	XPReward = 18;
	// Bulkier than a rifleman (1.85 m), and he walks in rather than runs.
	BodyScale = 1.03f;
	WalkSpeed = 150.f;
	ChaseSpeed = 290.f;
	AttackRange = Defaults.LatchRange;
	AttackWindup = Defaults.LatchWindup;
	AttackRecovery = 0.45f;
	AttackCooldown = 0.5f;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BodyMesh(DredgerLook::HoseManBodyMeshPath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TankMesh(DredgerLook::TankMeshPath);
	if (BodyMesh.Object)
	{
		GetMesh()->SetSkeletalMeshAsset(BodyMesh.Object);
	}

	// The tank on his back and its glass gauge: his weak spot (stand-ins: a barrel prop, a sphere; placed in PlaceJobKit).
	Tank = MakePart(TEXT("Tank"), TEXT("spine_05"));
	Tank->SetStaticMesh(TankMesh.Object);
	MakeTankHitZone(Tank);
	TankGauge = MakePart(TEXT("TankGauge"), TEXT("spine_05"));
	TankGauge->SetStaticMesh(SphereMesh);
	MakeTankHitZone(TankGauge);

	// The brass nozzle in his hands (the held thing's frame: +X its barrel, the grip at its back).
	NozzleBarrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NozzleBarrel"));
	NozzleBarrel->SetupAttachment(Held);
	NozzleBarrel->SetStaticMesh(CylinderMesh);
	NozzleMouth = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NozzleMouth"));
	NozzleMouth->SetupAttachment(Held);
	NozzleMouth->SetStaticMesh(CylinderMesh);
	for (UStaticMeshComponent* Part : { NozzleBarrel.Get(), NozzleMouth.Get() })
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
	}

	Hose = CreateDefaultSubobject<UDredgerHoseComponent>(TEXT("Hose"));
}

void ADredgerHoseMan::BeginPlay()
{
	Super::BeginPlay();
	ClassAttackRange = AttackRange;
	ClassChaseSpeed = ChaseSpeed;
}

void ADredgerHoseMan::PlaceJobKit()
{
	// The mesh's frame (+Y ahead, +Z up) from his upper chest at rest: the tank upright on his back, its gauge on its back.
	const FVector Chest = BoneAtRest(TEXT("spine_05")).GetLocation();
	PlacePart(Tank, TEXT("spine_05"), FitInBox(Tank->GetStaticMesh(), Chest + FVector(0.f, -25.f, -6.f), FVector(34.f, 34.f, 56.f)));
	PlacePart(TankGauge, TEXT("spine_05"), FitInBox(SphereMesh, Chest + FVector(0.f, -42.5f, 8.f), FVector(11.f, 5.f, 11.f)));
	UMaterialInstanceDynamic* BrassLook = MakeLook(DredgerLook::Brass, 0.f, 0.35f);
	GaugeLook = MakeLook(DredgerLook::SoulCyan, DredgerLook::GaugeGlow, 0.f);
	Tank->SetMaterial(0, BrassLook);
	for (int32 Slot = 1; Slot < Tank->GetNumMaterials(); ++Slot)
	{
		Tank->SetMaterial(Slot, BrassLook);
	}
	TankGauge->SetMaterial(0, GaugeLook);
	// The nozzle: a barrel from the grip to its mouth, the mouth wider, both brass.
	NozzleBarrel->SetRelativeTransform(FitInBox(CylinderMesh, FVector(22.f, 0.f, 0.f), FVector(40.f, 5.5f, 5.5f), AlongBarrel()));
	NozzleMouth->SetRelativeTransform(FitInBox(CylinderMesh, FVector(47.f, 0.f, 1.f), FVector(9.f, 9.5f, 9.5f), AlongBarrel()));
	NozzleBarrel->SetMaterial(0, BrassLook);
	NozzleMouth->SetMaterial(0, BrassLook);
}

bool ADredgerHoseMan::IsCriticalSpot(const FHitResult& Hit) const
{
	// The tank (and its gauge) while whole, and the mask as every Dredger's.
	const UPrimitiveComponent* Part = Hit.GetComponent();
	return (!bTankBurst && Part && (Part == Tank.Get() || Part == TankGauge.Get())) || Super::IsCriticalSpot(Hit);
}

void ADredgerHoseMan::SetHitVolumesEnabled(bool bEnabled)
{
	Super::SetHitVolumesEnabled(bEnabled);
	const ECollisionEnabled::Type Zones = bEnabled && !bTankBurst ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision;
	Tank->SetCollisionEnabled(Zones);
	TankGauge->SetCollisionEnabled(Zones);
}

float ADredgerHoseMan::GetDrainPerSecond() const
{
	const ADredgerHoseMan* Class = GetClass()->GetDefaultObject<ADredgerHoseMan>();
	return DredgerRules::DrainPerSecond(AttackDamage, Class ? Class->AttackDamage : AttackDamage, FDredgerTuning::Live());
}

FVector ADredgerHoseMan::GetNozzleBack() const
{
	return Held ? Held->GetComponentTransform().TransformPosition(FVector(-2.f, 0.f, -2.f)) : GetChestPoint();
}

FVector ADredgerHoseMan::GetTankFeed() const
{
	// Off the tank's top, on his right side, where the feed hose comes over his shoulder.
	const FBoxSphereBounds Bounds = Tank->Bounds;
	return Bounds.Origin + FVector(0.f, 0.f, Bounds.BoxExtent.Z * 0.75f) + GetActorRightVector() * 9.f;
}

void ADredgerHoseMan::NoteHoseLetGo()
{
	LatchRestLeft = FDredgerTuning::Live().LatchRest;
}

// ---------------------------------------------------------------------------
// His turns: the throw, or the club
// ---------------------------------------------------------------------------

bool ADredgerHoseMan::IsLineClear() const
{
	const UWorld* World = GetWorld();
	const APawn* Victim = GetTarget();
	if (!World || !Victim)
	{
		return false;
	}
	const double Now = World->GetTimeSeconds();
	if (Now - LineCheckedAt < LineCheckEvery)
	{
		return bLineClear;
	}
	LineCheckedAt = Now;
	FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("DredgerLatch"), this, false);
	Params.AddIgnoredActor(Victim);
	bLineClear = !World->LineTraceTestByObjectType(GetChestPoint(), Victim->GetActorLocation() + FVector(0.f, 0.f, DredgerRules::ChestRise),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params);
	return bLineClear;
}

float ADredgerHoseMan::GetAttackStartRange() const
{
	const APawn* Victim = GetTarget();
	if (!Victim || Hose->IsOut())
	{
		return 0.f;
	}
	const float Size = GetSizeScale();
	const float Distance = static_cast<float>(FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation()));
	if (bTankBurst || Distance < ClubWithin * Size)
	{
		return ClubWithin * Size;
	}
	return DredgerRules::CanLatch(Distance / Size, IsLineClear(), LatchRestLeft, FDredgerTuning::Live()) ? AttackRange * Size : 0.f;
}

bool ADredgerHoseMan::CanStartAttack() const
{
	return !Hose->IsOut();
}

bool ADredgerHoseMan::CanReachHeight(float Rise) const
{
	// The line reaches a player up on a crate or down a step; the club only what a blow does.
	return !bTankBurst || Super::CanReachHeight(Rise);
}

void ADredgerHoseMan::OnAttackStarted()
{
	const FDredgerTuning Tuning = FDredgerTuning::Live();
	const APawn* Victim = GetTarget();
	const float Distance = Victim ? static_cast<float>(FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation())) : 0.f;
	bThrowAttack = !bTankBurst && Distance >= ClubWithin * GetSizeScale();
	Wants.bShouldered = true;
	if (!bThrowAttack)
	{
		// The club: a short heave of the nozzle (angrier and quicker once his tank is gone).
		AttackWindup = 0.5f;
		AttackRecovery = 0.55f;
		AttackCooldown = bTankBurst ? 0.8f : 1.2f;
		Bark(LooterSoundCue::DredgerBark, 0.4f);
		return;
	}
	// The throw's tell: he plants, the nozzle comes up, its mouth swells cyan, the gauge and his lenses flare, a call.
	AttackWindup = Tuning.LatchWindup;
	AttackRecovery = 0.45f;
	AttackCooldown = 0.5f;
	FlareLenses(Tuning.LatchWindup);
	Bark(LooterSoundCue::DredgerBark, 0.5f);
	if (UEnemyProjectileSubsystem* Shots = GetWorld() ? GetWorld()->GetSubsystem<UEnemyProjectileSubsystem>() : nullptr)
	{
		Shots->ShowCharge(this, GetActorTransform().InverseTransformPosition(GetNozzleMouth()), Tuning.LatchWindup, ChargeSize, DredgerLook::SoulCyan);
	}
}

void ADredgerHoseMan::Strike()
{
	APawn* Victim = GetTarget();
	if (bThrowAttack)
	{
		if (Victim && IsValidTarget(Victim))
		{
			Hose->Fire(Victim);
		}
		return;
	}
	// The club: in reach, in front, nothing solid between.
	if (!IsValidTarget(Victim))
	{
		return;
	}
	const FVector ToVictim = Victim->GetActorLocation() - GetActorLocation();
	const bool bInReach = ToVictim.Size2D() <= ClubReach * GetSizeScale() && FMath::Abs(static_cast<float>(ToVictim.Z)) < 200.f * GetSizeScale();
	const bool bInFront = FVector::DotProduct(GetActorForwardVector(), ToVictim.GetSafeNormal2D()) > 0.4f;
	const UWorld* World = GetWorld();
	FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("DredgerClub"), this, false);
	Params.AddIgnoredActor(Victim);
	const bool bClear = World && !World->LineTraceTestByObjectType(GetChestPoint(), Victim->GetActorLocation(),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params);
	if (bInReach && bInFront && bClear)
	{
		HitWithAttack(Victim, ToVictim.GetSafeNormal2D(), ClubStrength);
		KickRecoil(0.8f);
	}
}

void ADredgerHoseMan::OnAttackCalledOff()
{
	// Hit off his turn in the tell: the cyan at the nozzle goes out, nothing thrown.
	if (UEnemyProjectileSubsystem* Shots = GetWorld() ? GetWorld()->GetSubsystem<UEnemyProjectileSubsystem>() : nullptr)
	{
		Shots->ClearShotsFrom(this);
	}
	bThrowAttack = false;
}

// ---------------------------------------------------------------------------
// Each frame
// ---------------------------------------------------------------------------

void ADredgerHoseMan::TickBeforeBrain(float DeltaSeconds)
{
	// Latched, he stands his ground and reels: his brain waits (no chase, no other attack) until the line lets go.
	if (Hose->IsLatched())
	{
		HoldBrain(0.1f);
	}
}

void ADredgerHoseMan::TickJob(float DeltaSeconds)
{
	LatchRestLeft = FMath::Max(0.f, LatchRestLeft - DeltaSeconds);
	const bool bThrowing = GetCreatureState() == ECreatureState::Attack && bThrowAttack;
	if (Hose->IsLatched())
	{
		// Square to his victim, braced against the line, the tug in his body.
		if (const APawn* Victim = Hose->GetVictim())
		{
			const float Want = static_cast<float>((Victim->GetActorLocation() - GetActorLocation()).Rotation().Yaw);
			const float Now = static_cast<float>(GetActorRotation().Yaw);
			const float Turned = Now + FMath::Clamp(static_cast<float>(FRotator::NormalizeAxis(Want - Now)), -LatchedTurnRate * DeltaSeconds,
				LatchedTurnRate * DeltaSeconds);
			SetActorRotation(FRotator(0.f, Turned, 0.f));
		}
		Wants.Brace = 1.f;
		Wants.Tug = Hose->GetTug();
		Wants.bShouldered = true;
	}
	else
	{
		Wants.Brace = bThrowing ? 0.4f : 0.f;
		Wants.Tug = 0.f;
	}
	Wants.Crouch = 0.f;
	TickGauge(DeltaSeconds);
}

void ADredgerHoseMan::TickGauge(float DeltaSeconds)
{
	if (!GaugeLook || bTankBurst)
	{
		return;
	}
	float Want = 1.f;
	if (Hose->IsLatched())
	{
		Want += GaugeDrain * (0.7f + 0.3f * FMath::Sin(static_cast<float>(GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0) * 9.f));
	}
	if (GetCreatureState() == ECreatureState::Attack && bThrowAttack)
	{
		Want += GaugeTell * FMath::Clamp(GetStateTime() / FMath::Max(AttackWindup, 0.05f), 0.f, 1.f);
	}
	GaugeNow = FMath::FInterpTo(GaugeNow, Want, DeltaSeconds, GaugeEase);
	GaugeLook->SetScalarParameterValue(TEXT("Glow"), DredgerLook::GaugeGlow * GaugeNow);
}

void ADredgerHoseMan::OnHurt(bool bCritical, const FVector& HitLocation)
{
	Super::OnHurt(bCritical, HitLocation);
	const UPrimitiveComponent* Part = Health->GetLastHitComponent();
	if (bTankBurst || !Part || (Part != Tank.Get() && Part != TankGauge.Get()))
	{
		return;
	}
	// A round on the tank: sparks off the brass, the gauge jumps; enough of them and it bursts.
	if (UBulletSubsystem* Bullets = GetWorld() ? GetWorld()->GetSubsystem<UBulletSubsystem>() : nullptr)
	{
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(GetWorld());
		Effects.SpawnRicochet(HitLocation, -GetLastHit().Direction, 0.6f);
	}
	GaugeNow += 1.5f;
	const APawn* Shooter = GetTarget();
	AController* By = Shooter ? Shooter->GetController() : UGameplayStatics::GetPlayerController(this, 0);
	NoteTankHit(Health->GetLastHealthLoss(), By);
}

void ADredgerHoseMan::OnDied()
{
	Super::OnDied();
	Hose->Break(EDredgerHoseBreak::OwnerDown);
}

void ADredgerHoseMan::OnRespawned()
{
	Super::OnRespawned();
	bTankBurst = false;
	TankDamage = 0.f;
	LatchRestLeft = 0.f;
	bThrowAttack = false;
	AttackRange = ClassAttackRange;
	ChaseSpeed = ClassChaseSpeed;
	// The tank is his weak spot, not dressing: it shows whatever the kit's switch says.
	Tank->SetVisibility(true);
	TankGauge->SetVisibility(true);
	SetHitVolumesEnabled(true);
	Hose->Break(EDredgerHoseBreak::None, /*bQuiet*/ true);
	Hose->SetTorn(false);
}

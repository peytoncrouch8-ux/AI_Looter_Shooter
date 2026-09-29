#include "Combat/BulletSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Combat/CombatRules.h"
#include "Combat/CriticalSpotTarget.h"
#include "Combat/HealthComponent.h"
#include "Combat/LooterDamageTypes.h"
#include "Creatures/CreatureBase.h"
#include "Weapons/WeaponBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

namespace
{
	/** Tracer glow strength (the material multiplies the color by it). */
	constexpr float TracerGlow = 30.f;
	/** Instant (hitscan) shots flash a thinner beam along their whole path for a frame. */
	constexpr float InstantTracerWidthScale = 0.6f;
	/**
	 * Tracers never get thinner on screen than this (radians across): a bullet flying away from the shooter is seen end on,
	 * and a few cm of streak would shrink below a pixel within a few meters.
	 */
	constexpr float MinTracerAngularWidth = 0.0024f;
	/** Streaks are at least as long as the bullet flies in this long (seconds), like motion blur. */
	constexpr float TracerStreakTime = 0.035f;

	TAutoConsoleVariable<bool> CVarDebugTracers(TEXT("Looter.Bullets.DebugDraw"), false,
		TEXT("Draw every bullet's tracer as a debug line (and its impact point)."));
}

bool UBulletSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Editor preview worlds too, for the automated tests (nothing fires there otherwise).
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UBulletSubsystem::Deinitialize()
{
	Bullets.Reset();
	const UWorld* World = GetWorld();
	if (!World || !World->bIsTearingDown)
	{
		Effects.Shutdown();
	}
	Super::Deinitialize();
}

int32 UBulletSubsystem::NumBulletsInFlight() const
{
	return Bullets.FilterByPredicate([](const FBullet& Bullet) { return !Bullet.bSpent; }).Num();
}

EImpactSurface UBulletSubsystem::SurfaceOf(const AActor* Actor)
{
	if (Cast<ACreatureBase>(Actor))
	{
		return EImpactSurface::Flesh;
	}
	if (Actor && Actor->FindComponentByClass<UHealthComponent>())
	{
		return EImpactSurface::Target;
	}
	return EImpactSurface::World;
}

FVector UBulletSubsystem::TracerPoint(const FBulletShot& Shot, float Distance)
{
	const FVector Offset = Shot.VisualStart - Shot.Start;
	return Shot.Start + Shot.Direction * Distance + Offset * (1.f - FMath::SmoothStep(0.f, ConvergeDistance, Distance));
}

void UBulletSubsystem::Fire(const FBulletShot& Shot)
{
	FBullet& Bullet = Bullets.AddDefaulted_GetRef();
	Bullet.Shot = Shot;
	Bullet.Shot.Direction = Shot.Direction.GetSafeNormal();
	Bullet.Position = Shot.Start;
	if (Shot.Speed <= 0.f)
	{
		// Instant: the whole flight happens now (the tracer still flashes along it this frame).
		Advance(Bullet, 0.f);
	}
}

void UBulletSubsystem::Tick(float DeltaTime)
{
	// Last frame's spent bullets have had their final draw.
	Bullets.RemoveAll([](const FBullet& Bullet) { return Bullet.bSpent && Bullet.bDrawn; });

	for (FBullet& Bullet : Bullets)
	{
		if (!Bullet.bSpent)
		{
			Advance(Bullet, DeltaTime);
		}
	}

	if (Bullets.IsEmpty() && Effects.IsIdle())
	{
		return;
	}
	Effects.Initialize(GetWorld());
	const FVector Camera = GetCameraLocation();
	for (FBullet& Bullet : Bullets)
	{
		DrawTracer(Bullet, Camera);
		Bullet.bDrawn = true;
	}
	Effects.Tick(DeltaTime, Camera);
}

void UBulletSubsystem::Advance(FBullet& Bullet, float DeltaSeconds)
{
	UWorld* World = GetWorld();
	const FBulletShot& Shot = Bullet.Shot;
	const float Step = Shot.Speed > 0.f ? Shot.Speed * DeltaSeconds : Shot.Range;
	const float Distance = FMath::Min(Step, Shot.Range - Bullet.Traveled);
	const FVector From = Bullet.Position;
	const FVector To = From + Shot.Direction * Distance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(BulletTrace), /*bTraceComplex*/ true);
	Params.bReturnPhysicalMaterial = true;
	if (AActor* Shooter = Shot.Shooter.Get())
	{
		Params.AddIgnoredActor(Shooter);
	}
	if (AWeaponBase* Weapon = Shot.Weapon.Get())
	{
		Params.AddIgnoredActor(Weapon);
	}

	FHitResult Hit;
	const bool bHit = World && Distance > 0.f && World->LineTraceSingleByChannel(Hit, From, To, Shot.Channel, Params);
	if (Shot.bDrawDebug && World)
	{
		DrawDebugLine(World, From, bHit ? Hit.ImpactPoint : To, bHit ? FColor::Red : FColor::Green, false, 1.f, 0, 0.5f);
		if (bHit)
		{
			DrawDebugPoint(World, Hit.ImpactPoint, 8.f, FColor::Red, false, 1.f);
		}
	}

	if (bHit)
	{
		Bullet.Position = Hit.ImpactPoint;
		Bullet.Traveled += (Hit.ImpactPoint - From).Size();
		Bullet.bSpent = true;
		ResolveHit(Bullet, Hit);
		return;
	}
	Bullet.Position = To;
	Bullet.Traveled += Distance;
	Bullet.bSpent = Bullet.Traveled >= Shot.Range - UE_KINDA_SMALL_NUMBER;
}

void UBulletSubsystem::ResolveHit(const FBullet& Bullet, const FHitResult& Hit)
{
	const FBulletShot& Shot = Bullet.Shot;
	AActor* HitActor = Hit.GetActor();
	AWeaponBase* Weapon = Shot.Weapon.Get();
	const EImpactSurface Surface = SurfaceOf(HitActor);

	// The target says where its critical spots are; the damage rules (range roll, x1.5 crit) are the same for every weapon.
	const ICriticalSpotTarget* Target = Cast<ICriticalSpotTarget>(HitActor);
	const bool bCritical = Target && Target->IsCriticalSpot(Hit);
	const float Roll = FMath::FRand();
	const float Damage = LooterCombat::HitDamage(Shot.Damage, bCritical, Roll);
	UE_LOG(LogLooter, Verbose, TEXT("Hit %s on %s after %.0f cm: %.1f (range %.1f-%.1f, roll %.2f) x%.2f = %.1f%s"), *GetNameSafe(HitActor),
		*GetNameSafe(Hit.GetComponent()), Bullet.Traveled, Shot.Damage, LooterCombat::MinHitDamage(Shot.Damage),
		LooterCombat::MaxHitDamage(Shot.Damage), Roll, bCritical ? LooterCombat::CriticalHitMultiplier : 1.f, Damage,
		bCritical ? TEXT(" CRIT") : TEXT(""));

	// Looks first: the damage below may kill the target.
	if (UNiagaraSystem* ImpactFX = Shot.ImpactFX.Get())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactFX, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	}
	else
	{
		Effects.Initialize(GetWorld());
		Effects.SpawnImpact(Hit.ImpactPoint, Hit.ImpactNormal, Shot.Direction, Surface, bCritical);
	}

	if (HitActor)
	{
		// The damage type tells receivers (health, damage numbers) whether this was a critical hit.
		const TSubclassOf<UDamageType> DamageType = bCritical ? UWeaponCritDamageType::StaticClass() : UWeaponDamageType::StaticClass();
		UGameplayStatics::ApplyPointDamage(HitActor, Damage, Shot.Direction, Hit, Shot.Instigator.Get(), Weapon, DamageType);
	}

	UPrimitiveComponent* HitComponent = Hit.GetComponent();
	if (HitComponent && HitComponent->IsSimulatingPhysics() && Shot.HitImpulse > 0.f)
	{
		HitComponent->AddImpulseAtLocation(Shot.Direction * Shot.HitImpulse, Hit.ImpactPoint, Hit.BoneName);
	}

	if (Weapon)
	{
		Weapon->NotifyBulletHit(Hit, Damage, bCritical);
	}
	OnBulletHit.Broadcast(Hit, Damage, bCritical);
}

void UBulletSubsystem::DrawTracer(const FBullet& Bullet, const FVector& Camera)
{
	const FBulletShot& Shot = Bullet.Shot;
	FVector From;
	FVector Head;
	float Width = Shot.TracerWidth;
	if (Shot.Speed <= 0.f)
	{
		// Instant shots: a beam from the muzzle to wherever it stopped.
		From = Shot.VisualStart;
		Head = Bullet.Position;
		Width *= InstantTracerWidthScale;
	}
	else
	{
		const float Length = FMath::Max(Shot.TracerLength, Shot.Speed * TracerStreakTime);
		const float Tail = FMath::Max(Bullet.Traveled - Length, 0.f);
		if (Bullet.Traveled - Tail < 1.f)
		{
			return;
		}
		From = TracerPoint(Shot, Tail);
		// A spent bullet's tracer ends exactly where it hit, even if it hadn't quite joined the aim line yet.
		Head = Bullet.bSpent ? Bullet.Position : TracerPoint(Shot, Bullet.Traveled);
	}
	// Keep far tracers visible: never thinner on screen than a pixel or two.
	if (!Camera.IsZero())
	{
		Width = FMath::Max(Width, FVector::Dist(Camera, Head) * MinTracerAngularWidth);
	}
	Effects.AddStreak(From, Head, Width, Shot.TracerColor, TracerGlow);

	if (CVarDebugTracers.GetValueOnGameThread())
	{
		DrawDebugLine(GetWorld(), From, Head, FColor::Cyan, false, -1.f, 0, 0.5f);
		if (Bullet.bSpent)
		{
			DrawDebugPoint(GetWorld(), Bullet.Position, 10.f, FColor::Red, false, 1.f);
		}
	}
}

FVector UBulletSubsystem::GetCameraLocation() const
{
	const UWorld* World = GetWorld();
	const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
	return Player && Player->PlayerCameraManager ? Player->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
}

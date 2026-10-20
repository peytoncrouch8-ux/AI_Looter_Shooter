#include "Combat/BulletSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Combat/CombatRules.h"
#include "Combat/CriticalSpotTarget.h"
#include "Combat/LooterDamageTypes.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Camera/PlayerCameraManager.h"
#include "CoreGlobals.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

// UBulletSubsystem: firing, flight, damage and tracers. How a hit looks and sounds (surfaces, creatures' splashes,
// ricochets, marks) is in BulletSubsystemImpacts.cpp.

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
	/**
	 * The tracer's hot head: the round itself, a short stretch at the front of the streak (a share of it, at most so many
	 * cm), wider, brighter and whiter than the glow behind it.
	 */
	constexpr float TracerHeadShare = 0.18f;
	constexpr float TracerHeadMax = 60.f;
	constexpr float TracerHeadWidth = 1.35f;
	constexpr float TracerHeadGlow = 1.6f;
	constexpr float TracerHeadWhite = 0.55f;
	const FLinearColor TracerHeadHeat(1.f, 0.95f, 0.85f);

	/**
	 * The smoke thread behind a tracer: laid from a meter and a half out (clear of the eye) to ThreadEnd, in pieces of at
	 * most ThreadPiece cm that overlap a little; only the first ThreadPellets of a volley (a shotgun's nine would be a fog).
	 */
	constexpr float ThreadStart = 150.f;
	constexpr float ThreadEnd = 1800.f;
	constexpr float ThreadPiece = 150.f;
	constexpr float ThreadFadeIn = 300.f;
	constexpr float ThreadWidth = 2.5f;
	constexpr float ThreadOpacity = 0.12f;
	constexpr float ThreadLife = 0.45f;
	constexpr int32 ThreadPellets = 2;

	TAutoConsoleVariable<bool> CVarDebugTracers(TEXT("Looter.Bullets.DebugDraw"), false,
		TEXT("Draw every bullet's tracer as a debug line (and its impact point)."));
}

bool UBulletSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Editor preview worlds too, for the automated tests (nothing fires there otherwise).
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UBulletSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Each play starts from the materials as they are now (a surface map baked since the last one is seen).
	ImpactSurface::ResetCache();
}

void UBulletSubsystem::Deinitialize()
{
	Bullets.Reset();
	const UWorld* World = GetWorld();
	if (!World || !World->bIsTearingDown)
	{
		Effects.Shutdown();
		Marks.Shutdown();
		Brass.Shutdown();
	}
	Super::Deinitialize();
}

int32 UBulletSubsystem::NumBulletsInFlight() const
{
	return Bullets.FilterByPredicate([](const FBullet& Bullet) { return !Bullet.bSpent; }).Num();
}

FVector UBulletSubsystem::TracerPoint(const FBulletShot& Shot, float Distance)
{
	const FVector Offset = Shot.VisualStart - Shot.Start;
	return Shot.Start + Shot.Direction * Distance + Offset * (1.f - FMath::SmoothStep(0.f, ConvergeDistance, Distance));
}

void UBulletSubsystem::Fire(const FBulletShot& Shot)
{
	// Shots fired by one gun in one frame are one volley (a shotgun's pellets from one pull): they share the cap on marks.
	if (GFrameCounter != LastFireFrame || Shot.Weapon != LastFireWeapon || Shot.Shooter != LastFireShooter)
	{
		++VolleyCount;
		VolleyPellets = 0;
		LastFireFrame = GFrameCounter;
		LastFireWeapon = Shot.Weapon;
		LastFireShooter = Shot.Shooter;
	}
	const AWeaponBase* Gun = Shot.Weapon.Get();
	const UWeaponDefinition* Definition = Gun ? Gun->GetInstance().Definition.Get() : nullptr;

	FBullet& Bullet = Bullets.AddDefaulted_GetRef();
	Bullet.Shot = Shot;
	Bullet.Shot.Direction = Shot.Direction.GetSafeNormal();
	Bullet.Position = Shot.Start;
	Bullet.Volley = VolleyCount;
	Bullet.MarkScale = FBulletMarks::GunScale(Definition ? Definition->Kind : EWeaponKind::None);
	Bullet.bThread = VolleyPellets++ < ThreadPellets;
	if (Shot.Speed <= 0.f)
	{
		// Instant: the whole flight happens now (the tracer still flashes along it this frame).
		Advance(Bullet, 0.f);
	}
}

void UBulletSubsystem::EjectCasing(const FCasingLaunch& Launch)
{
	Brass.Initialize(GetWorld());
	Brass.Eject(Launch);
}

void UBulletSubsystem::Tick(float DeltaTime)
{
	// Last frame's spent bullets have had their final draw.
	Bullets.RemoveAll([](const FBullet& Bullet) { return Bullet.bSpent && Bullet.bDrawn; });

	for (FBullet& Bullet : Bullets)
	{
		if (!Bullet.bSpent)
		{
			// A bullet flies on its shooter's clock: under Sundown the world crawls (global time dilation) while the player's
			// own time is put back to full speed (their CustomTimeDilation), and so are their shots.
			const AActor* Shooter = Bullet.Shot.Shooter.Get();
			Advance(Bullet, DeltaTime * (Shooter ? FMath::Max(Shooter->CustomTimeDilation, 0.f) : 1.f));
		}
	}

	// Nothing flying, nothing drawn and no brass about: most of the game, at no cost.
	if (Bullets.IsEmpty() && Effects.IsIdle() && Brass.IsIdle())
	{
		return;
	}
	Effects.Initialize(GetWorld());
	const FVector Camera = GetCameraLocation();
	for (FBullet& Bullet : Bullets)
	{
		LayThread(Bullet);
		DrawTracer(Bullet, Camera);
		Bullet.bDrawn = true;
	}
	Effects.Tick(DeltaTime, Camera);
	if (!Brass.IsIdle())
	{
		Brass.Tick(DeltaTime, Camera);
	}
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
	// The face it hits names the material there, which picks the impact's sound (ImpactCueOf).
	Params.bReturnFaceIndex = true;
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

	// The target says where its critical spots are; the damage rules (range roll, x1.5 crit) are the same for every weapon,
	// but for a cursed iron's critical multiplier (the gun hands it over with the shot).
	const ICriticalSpotTarget* Target = Cast<ICriticalSpotTarget>(HitActor);
	const bool bCritical = Target && Target->IsCriticalSpot(Hit);
	const float Roll = FMath::FRand();
	// Past the gun's range the damage falls off with the distance flown.
	const float Falloff = Shot.FalloffStats.IsSet() ? Shot.FalloffStats->DamageAtDistance(Bullet.Traveled) : 1.f;
	const float Damage = LooterCombat::HitDamage(Shot.Damage * Falloff, bCritical, Roll, Shot.CritMultiplier);
	const FString Part = Hit.BoneName.IsNone() ? GetNameSafe(Hit.GetComponent())
		: FString::Printf(TEXT("%s bone %s"), *GetNameSafe(Hit.GetComponent()), *Hit.BoneName.ToString());
	UE_LOG(LogLooter, Verbose, TEXT("Hit %s on %s after %.0f cm: %.1f (range %.1f-%.1f, roll %.2f) x%.2f = %.1f%s"), *GetNameSafe(HitActor),
		*Part, Bullet.Traveled, Shot.Damage, LooterCombat::MinHitDamage(Shot.Damage),
		LooterCombat::MaxHitDamage(Shot.Damage), Roll, bCritical ? FMath::Max(Shot.CritMultiplier, 1.f) : 1.f, Damage,
		bCritical ? TEXT(" CRIT") : TEXT(""));

	// Looks, sound and the mark first: the damage below may kill the target. The world shows what it's made of; a glancing
	// round off stone or metal may go off (seen and heard). A thing that shows its own hits (OwnLookTag) gets none of it.
	const UPrimitiveComponent* Struck = Hit.GetComponent();
	const bool bOwnLook = Struck && Struck->ComponentHasTag(FName(OwnLookTag));
	const EImpactMaterial Material = Surface == EImpactSurface::World ? ImpactSurface::Of(Hit) : EImpactMaterial::Unknown;
	const bool bRicochet = !bOwnLook && Surface == EImpactSurface::World && RollRicochet(Hit, Material, Shot.Direction);
	if (bOwnLook)
	{
		// Its look and sound are its own (its TakeDamage plays them).
	}
	else if (UNiagaraSystem* ImpactFX = Shot.ImpactFX.Get())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactFX, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	}
	else
	{
		ShowHit(Bullet, Hit, Surface, Material, bCritical, bRicochet);
	}
	if (!bOwnLook)
	{
		PlayImpactSound(Hit, Surface, Material, bRicochet);
	}
	if (!bOwnLook && Surface == EImpactSurface::World)
	{
		LeaveMark(Bullet, Hit, Material);
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

void UBulletSubsystem::LayThread(FBullet& Bullet)
{
	if (!Bullet.bThread)
	{
		return;
	}
	// From clear of the eye out to its end, as far as the bullet has got: a soft line in overlapping pieces, fading in over
	// its first meters so it never starts in a hard edge at the gun.
	const FBulletShot& Shot = Bullet.Shot;
	const float End = FMath::Min(Bullet.Traveled, ThreadEnd);
	float From = FMath::Max(Bullet.ThreadLaid, ThreadStart);
	while (From < End - 1.f)
	{
		const float To = FMath::Min(From + ThreadPiece, End);
		const float Overlap = (To - From) * 0.25f;
		const FVector Tail = TracerPoint(Shot, FMath::Max(From - Overlap, ThreadStart));
		const FVector Head = TracerPoint(Shot, FMath::Min(To + Overlap, Bullet.Traveled));
		const float FadeIn = FMath::Clamp((From - ThreadStart) / ThreadFadeIn, 0.25f, 1.f);
		Effects.AddThread(Tail, Head, ThreadWidth, ThreadOpacity * FadeIn, ThreadLife * FMath::FRandRange(0.85f, 1.15f));
		From = To;
	}
	Bullet.ThreadLaid = FMath::Max(Bullet.ThreadLaid, End);
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

	// Its hot head: the round itself, white-hot at the front of the streak.
	const FVector Along = Head - From;
	const float Length = Along.Size();
	if (Length > 1.f)
	{
		const float HeadLength = FMath::Min(Length * TracerHeadShare, TracerHeadMax);
		Effects.AddStreak(Head - Along * (HeadLength / Length), Head, Width * TracerHeadWidth,
			FMath::Lerp(Shot.TracerColor, TracerHeadHeat, TracerHeadWhite), TracerGlow * TracerHeadGlow);
	}

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

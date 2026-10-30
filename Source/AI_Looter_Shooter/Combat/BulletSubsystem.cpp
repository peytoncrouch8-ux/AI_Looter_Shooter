#include "Combat/BulletSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Combat/CombatRules.h"
#include "Combat/CriticalSpotTarget.h"
#include "Combat/LooterDamageTypes.h"
#include "Elements/ElementCallout.h"
#include "Elements/ElementRules.h"
#include "Elements/ElementTuning.h"
#include "Weapons/GunfeelKickTuning.h"
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
	HeldShots.Reset();
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

void UBulletSubsystem::Fire(const FBulletShot& FiredShot)
{
	// An ember power may bend it first (Double or Nothing's roll, Lucky Streak's lucky round): the shot as it will fly.
	FBulletShot Shot = FiredShot;
	OnBulletFiring.Broadcast(Shot);

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
	// How its hit lands (GF-IMPACT): by its gun's kind, and an LMG's by its round's place in the belt (its tracer's beat).
	Bullet.Kind = Definition ? Definition->Kind : EWeaponKind::None;
	Bullet.RoundsLeft = Gun ? Gun->GetCurrentMagazine() : 0;
	Bullet.bThread = VolleyPellets++ < ThreadPellets;
	if (Shot.Speed <= 0.f)
	{
		// Instant: the whole flight happens now (the tracer still flashes along it this frame).
		Advance(Bullet, 0.f);
	}
	// What its hit's listeners fired meanwhile (a ricochet's bounce off an instant shot) goes now.
	if (AdvanceDepth == 0)
	{
		FireHeld();
	}
}

void UBulletSubsystem::FireSoon(const FBulletShot& Shot)
{
	// While bullets are being moved the list may not grow under them: held until their move is done.
	if (AdvanceDepth > 0)
	{
		HeldShots.Add(Shot);
		return;
	}
	Fire(Shot);
}

void UBulletSubsystem::FireHeld()
{
	// A held shot's own hit may hold another (each effect makes only one more generation, so this ends at once); the cap
	// is only a guard.
	for (int32 Round = 0; Round < 4 && !HeldShots.IsEmpty(); ++Round)
	{
		TArray<FBulletShot> Shots = MoveTemp(HeldShots);
		HeldShots.Reset();
		for (const FBulletShot& Shot : Shots)
		{
			Fire(Shot);
		}
	}
	HeldShots.Reset();
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
	// Bounces and the like their hits fired fly from this frame on.
	FireHeld();
	// A shotgun blast's dust sheet once its last pellet is down (GF-IMPACT; FWeaponFX gathers them as they land).
	if (const int32 SheetVolley = Effects.PendingSheetVolley())
	{
		const bool bStillFlying = Bullets.ContainsByPredicate([SheetVolley](const FBullet& Bullet) { return !Bullet.bSpent && Bullet.Volley == SheetVolley; });
		Effects.SettleBlastSheet(bStillFlying);
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
		LayElementTrail(Bullet);
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
	// A hit's listeners may fire more bullets meanwhile (FireSoon holds them till this is done).
	TGuardValue<int32> Moving(AdvanceDepth, AdvanceDepth + 1);
	UWorld* World = GetWorld();
	const float Step = Bullet.Shot.Speed > 0.f ? Bullet.Shot.Speed * DeltaSeconds : Bullet.Shot.Range;
	float Left = FMath::Min(Step, Bullet.Shot.Range - Bullet.Traveled);

	// Most bullets take one trace and stop at what they hit. A round flying on through (Piercing) traces on from there for
	// the rest of its step, past what it went through.
	for (int32 Pass = 0; Pass < 4; ++Pass)
	{
		const FBulletShot& Shot = Bullet.Shot;
		const FVector From = Bullet.Position;
		const FVector To = From + Shot.Direction * Left;

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
		if (AActor* Ignored = Shot.IgnoreActor.Get())
		{
			Params.AddIgnoredActor(Ignored);
		}
		for (const TWeakObjectPtr<AActor>& Through : Bullet.Passed)
		{
			if (AActor* Actor = Through.Get())
			{
				Params.AddIgnoredActor(Actor);
			}
		}

		FHitResult Hit;
		const bool bHit = World && Left > 0.f && World->LineTraceSingleByChannel(Hit, From, To, Shot.Channel, Params);
		if (Shot.bDrawDebug && World)
		{
			DrawDebugLine(World, From, bHit ? Hit.ImpactPoint : To, bHit ? FColor::Red : FColor::Green, false, 1.f, 0, 0.5f);
			if (bHit)
			{
				DrawDebugPoint(World, Hit.ImpactPoint, 8.f, FColor::Red, false, 1.f);
			}
		}

		if (!bHit)
		{
			Bullet.Position = To;
			Bullet.Traveled += Left;
			Bullet.bSpent = Bullet.Traveled >= Shot.Range - UE_KINDA_SMALL_NUMBER;
			return;
		}
		const float Moved = (Hit.ImpactPoint - From).Size();
		Bullet.Position = Hit.ImpactPoint;
		Bullet.Traveled += Moved;
		Left = FMath::Max(Left - Moved, 0.f);
		Bullet.bSpent = true;
		const FBulletStrike Strike = ResolveHit(Bullet, Hit);
		AActor* Struck = Hit.GetActor();
		if (!Strike.bPassThrough || !Struck)
		{
			return;
		}
		// On through it, weaker for it, and never into it again (the rest of its step now, or next frame).
		Bullet.bSpent = Bullet.Traveled >= Bullet.Shot.Range - UE_KINDA_SMALL_NUMBER;
		Bullet.Passed.Add(Struck);
		Bullet.Shot.PierceLeft = FMath::Max(Bullet.Shot.PierceLeft - 1, 0);
		Bullet.Shot.Damage *= FMath::Max(Strike.PassDamageScale, 0.f);
		if (Bullet.bSpent || Left <= 0.f)
		{
			return;
		}
	}
}

FBulletStrike UBulletSubsystem::ResolveHit(const FBullet& Bullet, const FHitResult& Hit)
{
	const FBulletShot& Shot = Bullet.Shot;
	AActor* HitActor = Hit.GetActor();
	AWeaponBase* Weapon = Shot.Weapon.Get();
	const EImpactSurface Surface = SurfaceOf(HitActor);

	// The target says where its critical spots are; the damage rules (range roll, x1.5 crit) are the same for every weapon,
	// but for a cursed iron's critical multiplier (the gun hands it over with the shot).
	const ICriticalSpotTarget* Target = Cast<ICriticalSpotTarget>(HitActor);
	bool bCritical = Shot.bSureCritical || (Target && Target->IsCriticalSpot(Hit));
	// An ember power has its say before the damage is rolled (Long Shot's distance and its marked creatures' sure critical).
	FBulletLanding Landing;
	Landing.Traveled = Bullet.Traveled;
	Landing.bCritical = bCritical;
	OnBulletLanding.Broadcast(Shot, Hit, Landing);
	bCritical = Landing.bCritical;
	const float Roll = FMath::FRand();
	// Past the gun's range the damage falls off with the distance flown (a shotgun's pellets sooner and steeper).
	const float Falloff = Shot.FalloffStats.IsSet() ? GunfeelKick::DamageShareAtDistance(GunfeelKick::KindOf(Weapon), *Shot.FalloffStats, Bullet.Traveled) : 1.f;
	const float Damage = Shot.bBlank ? 0.f
		: LooterCombat::HitDamage(Shot.Damage * Falloff * FMath::Max(Landing.DamageScale, 0.f), bCritical, Roll, Shot.CritMultiplier);
	const FString Part = Hit.BoneName.IsNone() ? GetNameSafe(Hit.GetComponent())
		: FString::Printf(TEXT("%s bone %s"), *GetNameSafe(Hit.GetComponent()), *Hit.BoneName.ToString());
	UE_LOG(LogLooter, Verbose, TEXT("Hit %s on %s after %.0f cm: %.1f (range %.1f-%.1f, roll %.2f) x%.2f = %.1f%s"), *GetNameSafe(HitActor),
		*Part, Bullet.Traveled, Shot.Damage, LooterCombat::MinHitDamage(Shot.Damage),
		LooterCombat::MaxHitDamage(Shot.Damage), Roll, bCritical ? FMath::Max(Shot.CritMultiplier, 1.f) : 1.f, Damage,
		bCritical ? TEXT(" CRIT") : TEXT(""));

	// Looks, sound and the mark first: the damage below may kill the target. The world shows what it's made of; a glancing
	// round off stone or metal may go off (seen and heard). A thing that shows its own hits (OwnLookTag) gets none of it.
	const UPrimitiveComponent* Struck = Hit.GetComponent();
	// A blank (Double or Nothing's "nothing") passing something with health shows and sounds nothing there either: a miss.
	const bool bOwnLook = (Struck && Struck->ComponentHasTag(FName(OwnLookTag))) || (Shot.bBlank && Surface != EImpactSurface::World);
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
		// Its gun's kind decides how heavy it lands (GF-IMPACT: an SMG's small, a sniper's punch, a blast's one sheet).
		FVfxRound Round;
		Round.Kind = Bullet.Kind;
		Round.RoundsLeft = Bullet.RoundsLeft;
		Round.Volley = Bullet.Volley;
		const FWeaponFX::FLandingScope LandingLook(Effects, Round);
		ShowHit(Bullet, Hit, Surface, Material, bCritical, bRicochet);
	}
	if (!bOwnLook)
	{
		PlayImpactSound(Bullet, Hit, Surface, Material, bRicochet);
	}
	if (!bOwnLook && Surface == EImpactSurface::World)
	{
		LeaveMark(Bullet, Hit, Material);
	}

	// An element gun's own hit scales by the creature's affinity to its element (the user, 2026-10-29 night: "bullets scale
	// too, like Borderlands"): weak x2, resist x0.33, immune x0.1; a plain gun's never, nor a hit into a standing ward (the
	// ward has its own rules). Only what is dealt: the strike below keeps the gun's hit, which the status scales itself.
	const float Dealt = Shot.bBlank ? 0.f : Damage * ElementRules::GunHitScale(Shot.Element, HitActor, bCritical, FElementTuning::Live());
	if (HitActor && !Shot.bBlank)
	{
		// The damage type tells receivers (health, damage numbers) whether this was a critical hit.
		const TSubclassOf<UDamageType> DamageType = bCritical ? UWeaponCritDamageType::StaticClass() : UWeaponDamageType::StaticClass();
		// The number over it knows an element gun's hit (the damage type is the same for every gun): its crit shows tinted, and
		// a creature weak to the element says so (ElementCallout).
		const FHitElementScope ElementOfThisHit(Shot.Element);
		UGameplayStatics::ApplyPointDamage(HitActor, Dealt, Shot.Direction, Hit, Shot.Instigator.Get(), Weapon, DamageType);
	}

	UPrimitiveComponent* HitComponent = Hit.GetComponent();
	if (HitComponent && HitComponent->IsSimulatingPhysics() && Shot.HitImpulse > 0.f)
	{
		HitComponent->AddImpulseAtLocation(Shot.Direction * Shot.HitImpulse, Hit.ImpactPoint, Hit.BoneName);
	}

	if (Weapon && !Shot.bBlank)
	{
		Weapon->NotifyBulletHit(Hit, Dealt, bCritical);
	}
	OnBulletHit.Broadcast(Hit, Dealt, bCritical);

	// The gun's parts have their say last (its element's status, a bounce, a splash, flying on through).
	FBulletStrike Strike;
	Strike.Damage = Damage;
	Strike.bCritical = bCritical;
	Strike.Traveled = Bullet.Traveled;
	OnBulletStruck.Broadcast(Shot, Hit, Strike);
	return Strike;
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
	// On the soft tapered streak (FWeaponFX::EStreakLook::Tracer): seen end-on by a close target its head was a flat diamond.
	Effects.AddStreak(From, Head, Width, Shot.TracerColor, TracerGlow, FWeaponFX::EStreakLook::Tracer);

	// Its hot head: the round itself, white-hot at the front of the streak.
	const FVector Along = Head - From;
	const float Length = Along.Size();
	if (Length > 1.f)
	{
		const float HeadLength = FMath::Min(Length * TracerHeadShare, TracerHeadMax);
		Effects.AddStreak(Head - Along * (HeadLength / Length), Head, Width * TracerHeadWidth,
			FMath::Lerp(Shot.TracerColor, TracerHeadHeat, TracerHeadWhite), TracerGlow * TracerHeadGlow, FWeaponFX::EStreakLook::Tracer);
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

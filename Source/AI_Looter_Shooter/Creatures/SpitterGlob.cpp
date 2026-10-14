#include "Creatures/SpitterGlob.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/CombatRules.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Combat/EnemyShotDamageType.h"
#include "Weapons/NearMiss.h"
#include "Weapons/WeaponFX.h"
#include "World/PlayableArea.h"
#include "Components/BrushComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/Volume.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** How brightly a glob glows (M_StylizedSurface's emissive multiplier): a hot green dot against sky or ground at dusk. */
	constexpr float GlobGlow = 7.f;
	/** It's drawn this much bigger than its hit radius (the eye forgives a near miss of something big and soft). */
	constexpr float DrawnSize = 1.3f;
	/** Its wobble: stretched along its flight this much on average, give or take Wobble, this fast (radians a second). */
	constexpr float Stretch = 1.2f;
	constexpr float Wobble = 0.12f;
	constexpr float WobbleSpeed = 24.f;
	/** Its streak behind it: as long as this many seconds of its flight, at most MaxTrail cm; how bright. */
	constexpr float TrailSeconds = 0.06f;
	constexpr float MaxTrail = 70.f;
	constexpr float TrailGlow = 4.f;
	/** A puff of green vapour this often (s) behind it. */
	constexpr float PuffEvery = 0.05f;
	/** A glob that lands on ground this steep or flatter lies there; on anything steeper it runs down to the ground below. */
	constexpr float FloorNormalZ = 0.6f;
	/** How far below a wall or a body it looks for ground to puddle on (cm). */
	constexpr float PuddleDrop = 400.f;
	/** Its splat: gel flung out like a slime's, this big against its size. */
	constexpr float SplatSize = 55.f;

	/** The game's code-drawn effects, in a world that draws them (a test level draws nothing). */
	FWeaponFX* EffectsIn(UWorld* World)
	{
		if (!World || !World->IsGameWorld())
		{
			return nullptr;
		}
		UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
		if (!Bullets)
		{
			return nullptr;
		}
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		return &Effects;
	}

	/** How far a point is from a character's capsule (its surface; negative inside). */
	float DistanceToCapsule(const FVector& Point, const UCapsuleComponent& Capsule)
	{
		const float Radius = Capsule.GetScaledCapsuleRadius();
		const float Core = FMath::Max(Capsule.GetScaledCapsuleHalfHeight() - Radius, 0.f);
		const FVector Middle = Capsule.GetComponentLocation();
		const FVector OnCore = FMath::ClosestPointOnSegment(Point, Middle - FVector(0.0, 0.0, Core), Middle + FVector(0.0, 0.0, Core));
		return static_cast<float>(FVector::Dist(Point, OnCore)) - Radius;
	}
}

ASpitterGlob::ASpitterGlob()
{
	PrimaryActorTick.bCanEverTick = true;
	SetCanBeDamaged(false);

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Glow(TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface"));
	GlowMaterial = Glow.Object;
	Body->SetStaticMesh(SphereMesh.Object);
	Body->SetMobility(EComponentMobility::Movable);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetGenerateOverlapEvents(false);
	Body->SetCastShadow(false);
	Body->SetCanEverAffectNavigation(false);
	Body->bReceivesDecals = false;
}

ASpitterGlob* ASpitterGlob::Launch(UWorld* World, const FSpitterGlobLaunch& Launch)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	ASpitterGlob* Glob = World->SpawnActor<ASpitterGlob>(ASpitterGlob::StaticClass(), FTransform(Launch.Start), Params);
	if (!Glob)
	{
		return nullptr;
	}
	Glob->Spec = Launch;
	Glob->Spec.Radius = FMath::Max(Launch.Radius, 1.f);
	Glob->Spec.Puddle.Spitter = Launch.Spitter;
	Glob->Spec.Puddle.Instigator = Launch.Instigator;
	Glob->Position = Launch.Start;
	Glob->Velocity = Launch.Velocity;
	// Its Spitter never stops it (nor would it, being a pawn, but a dead one's corpse is out of the way too).
	Glob->WorldQuery = MakeWorldQuery(World, Launch.Spitter.Get());
	Glob->WorldQuery.AddIgnoredActor(Glob);
	if (!Glob->HasActorBegunPlay())
	{
		Glob->DispatchBeginPlay();
	}
	Glob->Draw(0.f);
	return Glob;
}

FCollisionQueryParams ASpitterGlob::MakeWorldQuery(const UWorld* World, const AActor* Ignored)
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SpitterGlob), /*bTraceComplex*/ false, Ignored);
	if (!World)
	{
		return Params;
	}
	// A volume is an invisible box that still answers world-static queries; only its box is skipped, so what a PCG volume
	// scattered (rocks, trees) still stops a glob, as it stops a pellet.
	for (TActorIterator<AVolume> It(World); It; ++It)
	{
		if (const UBrushComponent* Brush = It->GetBrushComponent())
		{
			Params.AddIgnoredComponent(Brush);
		}
	}
	// The playable area's invisible walls only stop walking pawns.
	for (TActorIterator<APlayableArea> It(World); It; ++It)
	{
		Params.AddIgnoredActor(*It);
	}
	return Params;
}

void ASpitterGlob::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInterface* Base = GlowMaterial.Get())
	{
		Look = UMaterialInstanceDynamic::Create(Base, this);
		Look->SetVectorParameterValue(TEXT("Color"), Spec.Color);
		Look->SetScalarParameterValue(TEXT("Glow"), GlobGlow);
		Look->SetScalarParameterValue(TEXT("Variation"), 0.f);
		Body->SetMaterial(0, Look);
	}
}

void ASpitterGlob::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bLanded || DeltaSeconds <= 0.f)
	{
		return;
	}
	Fly(DeltaSeconds);
	if (!bLanded)
	{
		Draw(DeltaSeconds);
	}
}

// ---------------------------------------------------------------------------
// Flight
// ---------------------------------------------------------------------------

void ASpitterGlob::Fly(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// One step of its arc (exact under steady gravity: the average of the velocities before and after).
	const FVector From = Position;
	const FVector NewVelocity = Velocity - FVector(0.0, 0.0, Spec.Gravity * DeltaSeconds);
	const FVector To = From + (Velocity + NewVelocity) * (0.5f * DeltaSeconds);

	// The world stops it: terrain, rocks, trees, buildings.
	FHitResult WorldHit;
	const bool bHitWorld = World->LineTraceSingleByObjectType(WorldHit, From, To, FCollisionObjectQueryParams(ECC_WorldStatic), WorldQuery);
	const FVector Reach = bHitWorld ? FVector(WorldHit.Location) : To;

	// A player: its path, as far as the world lets it go, passing within its radius of their capsule. The nearest wins.
	ACharacter* Victim = nullptr;
	float VictimTime = 1.f;
	for (TActorIterator<ACharacter> It(World); It; ++It)
	{
		ACharacter* Character = *It;
		const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
		if (!Capsule || !UEnemyProjectileSubsystem::CanHurt(Spec.Spitter.Get(), Character))
		{
			continue;
		}
		float Time = 1.f;
		if (UEnemyProjectileSubsystem::SweepHitsCapsule(From, Reach, Spec.Radius, Capsule->GetComponentLocation(),
			Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight(), Time) && (!Victim || Time < VictimTime))
		{
			Victim = Character;
			VictimTime = Time;
		}
	}
	if (!Victim)
	{
		// Missing the player by a little, it hisses past their head (once).
		NearMiss::HearFlyBy(*World, From, Reach, static_cast<float>(FVector::Dist(Spec.Start, From)), Spec.Spitter.Get(),
			ENearMissSound::Hiss, bHeardFlyBy);
	}

	Velocity = NewVelocity;
	if (Victim)
	{
		Position = FMath::Lerp(From, Reach, VictimTime);
		Land(Position, -Velocity.GetSafeNormal(), Victim);
		return;
	}
	if (bHitWorld)
	{
		Position = WorldHit.ImpactPoint;
		Land(Position, WorldHit.ImpactNormal, nullptr);
		return;
	}
	Position = To;
	Age += DeltaSeconds;
	if (Age >= MaxFlight)
	{
		// Over a cliff's edge, into nothing: gone without a splat.
		bLanded = true;
		LandedAt = Position;
		Destroy();
	}
}

void ASpitterGlob::Land(const FVector& Where, const FVector& Normal, ACharacter* Victim)
{
	bLanded = true;
	LandedAt = Where;
	UWorld* World = GetWorld();
	const FVector Along = Velocity.GetSafeNormal();

	// The splat: gel flung out round where it struck, and its wet slap.
	if (FWeaponFX* Effects = EffectsIn(World))
	{
		Effects->SpawnDeathBurst(EDeathBurst::Gel, Where + Normal * 6.f, Along, SplatSize * Spec.Size, Spec.Color);
	}
	LooterSound::PlayAt(this, LooterSoundCue::Spitter::Splat, Where);

	// Squarely on a player: its full damage, blamed on its Spitter (the hit's direction points back along its lob).
	AActor* Causer = Spec.Spitter.IsValid() ? Spec.Spitter.Get() : this;
	if (Victim)
	{
		DirectVictim = Victim;
		const float Damage = LooterCombat::RollHitDamage(Spec.Damage, false);
		UE_LOG(LogLooter, Verbose, TEXT("%s's glob hit %s for %.1f"), *GetNameSafe(Spec.Spitter.Get()), *GetNameSafe(Victim), Damage);
		UGameplayStatics::ApplyDamage(Victim, Damage, Spec.Instigator.Get(), Causer, UEnemyShotDamageType::StaticClass());
	}
	// Its splash: anyone else near where it burst.
	if (World && Spec.SplashRadius > 0.f && Spec.SplashDamage > 0.f)
	{
		for (TActorIterator<ACharacter> It(World); It; ++It)
		{
			ACharacter* Character = *It;
			const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
			if (!Capsule || Character == Victim || !UEnemyProjectileSubsystem::CanHurt(Spec.Spitter.Get(), Character)
				|| DistanceToCapsule(Where, *Capsule) > Spec.SplashRadius)
			{
				continue;
			}
			++Splashed;
			UGameplayStatics::ApplyDamage(Character, LooterCombat::RollHitDamage(Spec.SplashDamage, false), Spec.Instigator.Get(), Causer,
				UEnemyShotDamageType::StaticClass());
		}
	}

	// The puddle it leaves: where it lies, or on the ground under the wall or the player it hit.
	FVector Ground;
	FVector GroundNormal;
	if (FindPuddleGround(Where, Normal, Victim != nullptr, Ground, GroundNormal))
	{
		Puddle = ASpitterPuddle::Spawn(World, Ground, GroundNormal, Spec.Puddle);
	}

	// Gone a moment later (the tests read what it did meanwhile).
	Body->SetHiddenInGame(true);
	SetActorTickEnabled(false);
	SetLifeSpan(0.2f);
}

bool ASpitterGlob::FindPuddleGround(const FVector& Where, const FVector& Normal, bool bOnBody, FVector& OutGround, FVector& OutNormal) const
{
	if (!bOnBody && Normal.Z >= FloorNormalZ)
	{
		OutGround = Where;
		OutNormal = Normal;
		return true;
	}
	const UWorld* World = GetWorld();
	FHitResult Ground;
	// Straight down from where it struck (a little off a wall, so the look doesn't start inside it); pawns aren't world-static,
	// so the player it hit doesn't stop the look.
	const FVector From = Where + FVector(Normal.X, Normal.Y, 0.0).GetSafeNormal() * 10.f + FVector(0.0, 0.0, 10.0);
	if (World && World->LineTraceSingleByObjectType(Ground, From, From - FVector(0.0, 0.0, PuddleDrop), FCollisionObjectQueryParams(ECC_WorldStatic),
		WorldQuery) && Ground.ImpactNormal.Z >= FloorNormalZ)
	{
		OutGround = Ground.ImpactPoint;
		OutNormal = Ground.ImpactNormal;
		return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
// Its look
// ---------------------------------------------------------------------------

void ASpitterGlob::Draw(float DeltaSeconds)
{
	const FVector Along = Velocity.IsNearlyZero() ? FVector::ForwardVector : Velocity.GetSafeNormal();
	const float Long = Stretch + Wobble * FMath::Sin(Age * WobbleSpeed);
	const float Across = 1.f / FMath::Sqrt(Long);
	const float Size = Spec.Radius * DrawnSize / 50.f;
	SetActorTransform(FTransform(FRotationMatrix::MakeFromX(Along).ToQuat(), Position, FVector(Size * Long, Size * Across, Size * Across)));

	FWeaponFX* Effects = EffectsIn(GetWorld());
	if (!Effects)
	{
		return;
	}
	// A streak of venom-light behind it, and a puff of green vapour now and then, hanging where it passed.
	const float Trail = FMath::Min(static_cast<float>(Velocity.Size()) * TrailSeconds, MaxTrail);
	Effects->AddStreak(Position - Along * Trail, Position, Spec.Radius * 1.2f, Spec.Color, TrailGlow);
	PuffIn -= DeltaSeconds;
	if (PuffIn <= 0.f)
	{
		PuffIn += PuffEvery;
		Effects->SpawnDustPuff(Position, Velocity * 0.08f + FVector(0.0, 0.0, 15.0), Spec.Color * 0.45f, 0.3f, Spec.Radius * 1.1f,
			Spec.Radius * 3.f, 0.45f);
	}
}

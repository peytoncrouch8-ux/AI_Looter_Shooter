#include "Combat/GraveSaltCinderBed.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/GraveSaltBurst.h"
#include "Combat/GraveSaltRules.h"
#include "Combat/GraveSaltTuning.h"
#include "Combat/GrenadeKinds.h"
#include "Combat/GrenadeKindsTuning.h"
#include "Combat/HealthComponent.h"
#include "Elements/ElementDamageType.h"
#include "Elements/ElementRules.h"
#include "Elements/ElementTuning.h"
#include "Elements/LooterElement.h"
#include "Weapons/DeathSplats.h"
#include "Weapons/NiagaraFX.h"
#include "Weapons/VfxSubsystem.h"
#include "Weapons/VfxTuning.h"
#include "Weapons/VfxTypes.h"
#include "Weapons/WeaponFX.h"
#include "CollisionQueryParams.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"

namespace
{
	/** The coals fall this far at most to the ground under a burst in the air (cm); higher, they're lost on the way down. */
	constexpr float GroundSearch = 800.f;
	/** Bodies are measured from this far over the ground (cm): a creature standing in it is caught by its feet. */
	constexpr float BurnHeight = 20.f;
	/** The search ball reaches this far past the radius, as the burst's does: a big body's middle may lie well out. */
	constexpr float SearchMargin = 150.f;
	/** Its last second it dies down: flames, light and burns alike (the burns stop at its end). */
	constexpr float DieDownSeconds = 1.f;

	/** The flames: this many a second over a bed of 2.6 m (more over a bigger one), each a lick of fire rolling into smoke. */
	constexpr float FlamesPerSecond = 14.f;
	constexpr float FlameRise = 70.f;
	const FLinearColor FlameFire(1.f, 0.4f, 0.1f);
	const FLinearColor FlameSmoke(0.06f, 0.05f, 0.045f);
	/** Embers rising off it, as a burn's look on a body (FWeaponFX::SpawnElementAura): this many puffs a second, a few pieces each. */
	constexpr float EmberPuffsPerSecond = 4.f;
	constexpr int32 EmberPieces = 2;
	/** A body it burns catches: this many pieces of a burn's look on it each burn. */
	constexpr int32 BodyEmbers = 2;
	/** The ring of ember light at its edge (read at a glance: "this far it burns"): its width (cm) and glow. */
	constexpr float EdgeWidth = 7.f;
	constexpr float EdgeGlow = 1.6f;
	constexpr float EdgeLift = 4.f;
	const FLinearColor EdgeColor(1.f, 0.38f, 0.1f);
	/** Its light: dim, warm, flickering; no shadows. Candelas, and its reach as a share of the bed's radius. */
	constexpr float GlowCandelas = 70.f;
	constexpr float GlowReachShare = 2.2f;
	constexpr float GlowLift = 40.f;
	const FLinearColor GlowColor(1.f, 0.48f, 0.18f);
	/** The scorch it leaves, a share of its radius. */
	constexpr float ScorchShare = 1.1f;
}

AGraveSaltCinderBed::AGraveSaltCinderBed()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Root);
	Glow->SetRelativeLocation(FVector(0.f, 0.f, GlowLift));
	Glow->SetIntensityUnits(ELightUnits::Candelas);
	Glow->SetIntensity(0.f);
	Glow->SetLightColor(GlowColor);
	Glow->SetCastShadows(false);
}

AGraveSaltCinderBed* AGraveSaltCinderBed::Spawn(UWorld* World, const FVector& Burst, APawn* Thrower, float LevelScale)
{
	if (!World)
	{
		return nullptr;
	}
	// The ground under the burst (the world's solid shapes only): a steep face or nothing below, and the coals are lost.
	const FGraveSaltBlast Ground = GraveSaltBurst::FindGround(*World, Burst, GroundSearch);
	if (!Ground.bOnGround)
	{
		UE_LOG(LogLooter, Verbose, TEXT("Cinder jar: no ground under the burst, no bed"));
		return nullptr;
	}
	const FGrenadeKindTuning Kinds = FGrenadeKindTuning::Current();
	const FTransform Transform(FRotationMatrix::MakeFromZ(Ground.Up).Rotator(), Ground.Ground);
	AGraveSaltCinderBed* Bed = World->SpawnActorDeferred<AGraveSaltCinderBed>(AGraveSaltCinderBed::StaticClass(), Transform, Thrower, Thrower,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Bed)
	{
		return nullptr;
	}
	Bed->Thrower = Thrower;
	Bed->LevelScale = FMath::Max(LevelScale, 0.f);
	Bed->Up = Ground.Up;
	Bed->Radius = Kinds.BedRadius;
	Bed->Seconds = Kinds.BedSeconds;
	Bed->TickEvery = Kinds.BedTick;
	Bed->FinishSpawning(Transform);
	return Bed;
}

void AGraveSaltCinderBed::BeginPlay()
{
	Super::BeginPlay();
	Glow->SetAttenuationRadius(Radius * GlowReachShare);
	// The coals left a scorch where they lie.
	if (UVfxSubsystem* Vfx = UVfxSubsystem::Get(this))
	{
		Vfx->AddSplat(GetActorLocation(), Up, EVfxSplat::Scorch, FLinearColor::Black, Radius * ScorchShare);
	}
	Crackle = LooterSound::Start(this, GrenadeKindCue::CinderBed, Root);
	// Never left burning, whatever happens to its clock.
	SetLifeSpan(Seconds + 2.f);
}

void AGraveSaltCinderBed::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	LooterSound::Stop(Crackle.Get(), 0.3f);
	Crackle.Reset();
	// Gone early (the level closing, a test's end): the Niagara fire burns out on its own rather than hanging there.
	StopNiagara();
	Super::EndPlay(EndPlayReason);
}

void AGraveSaltCinderBed::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Step(DeltaSeconds);
}

float AGraveSaltCinderBed::GetStrength() const
{
	return FMath::Clamp((Seconds - Age) / DieDownSeconds, 0.f, 1.f);
}

float AGraveSaltCinderBed::GetBossDealt(const AActor* Boss) const
{
	const float* Dealt = BossDealt.Find(TWeakObjectPtr<AActor>(const_cast<AActor*>(Boss)));
	return Dealt ? *Dealt : 0.f;
}

bool AGraveSaltCinderBed::IsDrawnByNiagara() const
{
	return bDrawnByNiagara;
}

void AGraveSaltCinderBed::StartNiagara()
{
	if (bNiagaraTried)
	{
		return;
	}
	// Decided once, as it starts: a cvar flipped mid-burn leaves this bed as it began (both looks never draw at once).
	bNiagaraTried = true;
	NiagaraFire = NiagaraFX::SpawnAt(GetWorld(), ENiagaraFX::CinderBed, GetActorLocation(), Up, Radius / NiagaraFX::BedDesignRadius);
	bDrawnByNiagara = NiagaraFire.IsValid();
}

void AGraveSaltCinderBed::StopNiagara()
{
	if (UNiagaraComponent* Fire = NiagaraFire.Get())
	{
		// Its emitters stop spawning and let what's alive finish (EmitterState "Complete"); it destroys itself after.
		Fire->Deactivate();
	}
	NiagaraFire.Reset();
}

void AGraveSaltCinderBed::Step(float DeltaSeconds)
{
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	Age += Dt;
	SinceBurn += Dt;
	// Burning on its beat; the first a beat after the burst (the burst itself has just hurt them).
	while (SinceBurn >= TickEvery && Age <= Seconds)
	{
		SinceBurn -= TickEvery;
		Burn(TickEvery);
	}
	DrawFire(Dt, GetStrength());
	if (Age >= Seconds)
	{
		LooterSound::Stop(Crackle.Get(), 0.4f);
		Crackle.Reset();
		Destroy();
	}
}

int32 AGraveSaltCinderBed::Burn(float BurnSeconds)
{
	UWorld* World = GetWorld();
	if (!World || BurnSeconds <= 0.f)
	{
		return 0;
	}
	APawn* By = Thrower.Get();
	const FVector Center = GetActorLocation() + Up * BurnHeight;
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	Objects.AddObjectTypesToQuery(ECC_PhysicsBody);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CinderBed), false, this);
	if (By)
	{
		Params.AddIgnoredActor(By);
	}
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, Objects, FCollisionShape::MakeSphere(Radius + SearchMargin), Params);

	const FGraveSaltTuning Base = FGraveSaltTuning::Current();
	const FGrenadeKindTuning Kinds = FGrenadeKindTuning::Current();
	const FElementTuning Elements = FElementTuning::Live();
	AController* ThrowerController = By ? By->GetController() : nullptr;
	UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
	TArray<const AActor*, TInlineAllocator<16>> Weighed;
	int32 Burned = 0;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (!Actor || Weighed.Contains(Actor))
		{
			continue;
		}
		Weighed.Add(Actor);
		// The same bodies the burst may hurt (never the thrower or a player), in the bed and not behind a wall.
		if (!GraveSaltBurst::CanHurt(Actor, By))
		{
			continue;
		}
		const FGraveSaltBody Body = GraveSaltBurst::BodyOf(*Actor);
		if (FGraveSaltRules::DistanceToBody(Center, Body) > Radius || !GraveSaltBurst::CanSee(*World, Center, Body, Actor, By))
		{
			continue;
		}
		// Fire, as the weakness table has it: spiders and slimes burn, the Unpaid shrug much of it off.
		const EElementAffinity Affinity = ElementRules::Affinity(ELooterElement::Ember, ElementRules::TargetOf(Actor), false);
		// Immune to fire (a spirit, a Cinder-touched spider, the Foreman): the bed doesn't touch it.
		if (Affinity == EElementAffinity::Immune)
		{
			continue;
		}
		float Damage = GrenadeKinds::BedDamage(BurnSeconds, LevelScale, ElementRules::AffinityScale(Affinity, Elements), Random.FRand(), Base, Kinds);
		UHealthComponent* Health = Actor->FindComponentByClass<UHealthComponent>();
		if (GraveSaltBurst::IsBoss(Actor))
		{
			float& Dealt = BossDealt.FindOrAdd(TWeakObjectPtr<AActor>(Actor));
			Damage = GrenadeKinds::CapLeft(Damage, Dealt, Health ? Health->GetMaxHealth() : 0.f, GrenadeKinds::BedBossCap(Base));
			Dealt += Damage;
		}
		if (Damage <= 0.f)
		{
			continue;
		}
		const FVector Point = FGraveSaltRules::NearestPoint(Center, Body);
		FVector Direction = (Point - Center).GetSafeNormal();
		if (Direction.IsNearlyZero())
		{
			Direction = Up;
		}
		const FHitResult Hit(Actor, Cast<UPrimitiveComponent>(Actor->GetRootComponent()), Point, -Direction);
		UGameplayStatics::ApplyPointDamage(Actor, Damage, Direction, Hit, ThrowerController, By, UEmberElementDamageType::StaticClass());
		++Burned;
		// It catches: a lick of the burn's look on the body.
		if (Bullets)
		{
			FWeaponFX& Effects = Bullets->GetEffects();
			Effects.Initialize(World);
			Effects.SpawnElementAura(ELooterElement::Ember, Body.Center, Body.Radius, Body.HalfHeight, BodyEmbers, 1.f);
		}
	}
	return Burned;
}

void AGraveSaltCinderBed::DrawFire(float DeltaSeconds, float Strength)
{
	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	// The Niagara fire, when it's the look (Looter.Fx.Niagara): started on the first look, dying down with the bed's last
	// second (its flames stop coming; those alive and their smoke finish after the bed is gone).
	StartNiagara();
	if (Strength < 1.f)
	{
		StopNiagara();
	}
	if (!Bullets || Strength <= 0.f)
	{
		Glow->SetIntensity(0.f);
		return;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	const FVfxTuning Vfx = FVfxTuning::Live();
	const FVector Ground = GetActorLocation();
	const FVector Across = FVector::CrossProduct(Up, FMath::Abs(Up.Z) < 0.9f ? FVector::UpVector : FVector::ForwardVector).GetSafeNormal();
	const FVector Other = FVector::CrossProduct(Up, Across).GetSafeNormal();
	const float Size = Radius / 260.f;

	// Its edge: a ring of ember light lying on the ground, so the player reads at a glance how far it burns (in both looks:
	// it's the rule, not the fire).
	Effects.AddGroundRing(Ground + Up * EdgeLift, Radius, EdgeWidth, EdgeColor, EdgeGlow * Strength * Vfx.Glow);
	// A dim warm flicker on what's round it (both looks: Niagara's light renderer would add a light per system).
	const float Flicker = 0.75f + 0.25f * FMath::Sin(Age * 23.f) * FMath::Sin(Age * 7.3f + 1.f);
	Glow->SetIntensity(GlowCandelas * Strength * Flicker);
	if (bDrawnByNiagara)
	{
		return;
	}

	// Flames licking up off it, spread evenly over the bed (the root of a random share, so the middle isn't crowded).
	FlamesOwed += DeltaSeconds * FlamesPerSecond * Size * Size * Strength;
	while (FlamesOwed >= 1.f)
	{
		FlamesOwed -= 1.f;
		const float Angle = Random.FRandRange(0.f, 2.f * PI);
		const float Out = Radius * FMath::Sqrt(Random.FRand()) * 0.92f;
		FVfxFlame Flame;
		Flame.Location = Ground + (Across * FMath::Cos(Angle) + Other * FMath::Sin(Angle)) * Out + Up * 6.f;
		Flame.Velocity = Up * FlameRise * Random.FRandRange(0.6f, 1.3f);
		Flame.StartSize = Random.FRandRange(22.f, 38.f) * Strength;
		Flame.EndSize = Random.FRandRange(60.f, 100.f) * Strength;
		Flame.Fire = FlameFire;
		Flame.Smoke = FlameSmoke;
		Flame.Opacity = 0.85f;
		Flame.Life = Random.FRandRange(0.8f, 1.2f);
		Flame.Burn = 0.55f;
		Flame.Gravity = -0.3f;
		Flame.Drag = 2.f;
		Effects.SpawnFlame(Flame);
	}
	// Embers rising off the whole bed.
	EmbersOwed += DeltaSeconds * EmberPuffsPerSecond * Strength;
	while (EmbersOwed >= 1.f)
	{
		EmbersOwed -= 1.f;
		Effects.SpawnElementAura(ELooterElement::Ember, Ground + Up * 30.f, Radius * 0.8f, 30.f, EmberPieces, 0.6f);
	}
}

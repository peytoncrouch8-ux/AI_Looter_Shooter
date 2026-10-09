#include "Combat/GraveSaltGrenade.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/GraveSaltBurst.h"
#include "Combat/GraveSaltRules.h"
#include "Combat/HealthComponent.h"
#include "Player/PlayerThrowComponent.h"
#include "Weapons/WeaponFX.h"
#include "CollisionQueryParams.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** The tin, imported from Art/Models/Throwables/SaltGrenade.py; the engine's cylinder until it is (sized like it). */
	const TCHAR* JarMeshPath = TEXT("/Game/Art/Throwables/SM_SaltGrenade.SM_SaltGrenade");
	const TCHAR* StandInPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const FVector StandInScale(0.07f, 0.07f, 0.1f);
	/** The tin's middle sits this far above its base (the model's pivot), so it tumbles about its middle. */
	constexpr float JarMiddle = 5.f;
	/** Where the fuse's tip is with no socket to say (above the base). */
	constexpr float FuseTipFallback = 13.f;

	/**
	 * What the jar sweeps through: the bullets' channel (Weapon in DefaultEngine.ini, AWeaponBase::TraceChannel), so it
	 * meets what a shot meets (terrain, rocks, trees, walls, a creature's body) and passes what a shot passes (triggers,
	 * the playable area's invisible walls, a pawn's capsule, grass).
	 */
	constexpr ECollisionChannel SweepChannel = ECC_GameTraceChannel2;

	/** No more than this many contacts in one step: a jar wedged in a corner can't loop forever. */
	constexpr int32 MaxContactsPerStep = 4;

	/** The burst's light: its candelas, reach, colour and how long it fades; then the actor goes. */
	constexpr float FlashCandelas = 160.f;
	constexpr float FlashRadius = 900.f;
	const FLinearColor FlashColor(1.f, 0.86f, 0.66f);
	constexpr float FlashSeconds = 0.22f;
	constexpr float LingerSeconds = 0.3f;

	/** The fuse's sparks: a few short hot streaks a frame, spat out of the tip. */
	const FLinearColor FuseSparkColor(1.f, 0.62f, 0.25f);
	constexpr int32 FuseSparksPerFrame = 3;
}

AGraveSaltGrenade::AGraveSaltGrenade()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Seen, never touched: the flight is its own sweep, so the model collides with nothing.
	Jar = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Jar"));
	Jar->SetupAttachment(Root);
	Jar->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Jar->SetCanEverAffectNavigation(false);
	Jar->SetGenerateOverlapEvents(false);

	Flash = CreateDefaultSubobject<UPointLightComponent>(TEXT("Flash"));
	Flash->SetupAttachment(Root);
	Flash->SetIntensityUnits(ELightUnits::Candelas);
	Flash->SetIntensity(0.f);
	Flash->SetAttenuationRadius(FlashRadius);
	Flash->SetLightColor(FlashColor);
	Flash->SetCastShadows(false);
	Flash->SetVisibility(false);
}

UStaticMesh* AGraveSaltGrenade::FindJarMesh()
{
	// Looked up when first needed rather than in a constructor, so a build before the model's import only stands in a
	// cylinder instead of failing to find it. /Game/Art/Throwables is cooked whole (DefaultGame.ini).
	// Looked for again while it isn't there (once a throw at most), so an import during a session is picked up.
	static TWeakObjectPtr<UStaticMesh> Mesh;
	if (!Mesh.IsValid())
	{
		Mesh = LoadObject<UStaticMesh>(nullptr, JarMeshPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	return Mesh.Get();
}

AGraveSaltGrenade* AGraveSaltGrenade::Throw(UWorld* World, const FVector& Start, const FVector& StartVelocity, APawn* By, float ByLevelScale)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = By;
	Params.Instigator = By;
	AGraveSaltGrenade* Grenade = World->SpawnActor<AGraveSaltGrenade>(AGraveSaltGrenade::StaticClass(), Start, StartVelocity.Rotation(), Params);
	if (Grenade)
	{
		Grenade->Launch(StartVelocity, By, ByLevelScale);
	}
	return Grenade;
}

void AGraveSaltGrenade::BeginPlay()
{
	Super::BeginPlay();
	if (UStaticMesh* Mesh = FindJarMesh())
	{
		Jar->SetStaticMesh(Mesh);
	}
	else
	{
		Jar->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, StandInPath));
		Jar->SetRelativeScale3D(StandInScale);
	}
	// About its middle, not its base: the pivot is under the tin.
	Jar->SetRelativeLocation(FVector(0.f, 0.f, -JarMiddle));
	// Never left lying about, whatever happens to its fuse.
	SetLifeSpan(FGraveSaltRules::FuseSeconds + 5.f);
}

void AGraveSaltGrenade::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopFuseSound();
	Super::EndPlay(EndPlayReason);
}

void AGraveSaltGrenade::Launch(const FVector& InVelocity, APawn* InThrower, float InLevelScale)
{
	Velocity = InVelocity;
	Thrower = InThrower;
	LevelScale = FMath::Max(InLevelScale, 0.f);
	bLaunched = true;
	Age = 0.f;
	// End over end, mostly about the throw's own sideways axis, as a lobbed tin goes, a little off it so no two match.
	Random.Initialize(static_cast<int32>(GetUniqueID()));
	const FVector Way = InVelocity.GetSafeNormal();
	FVector Side = FVector::CrossProduct(Way, FVector::UpVector).GetSafeNormal();
	if (Side.IsNearlyZero())
	{
		Side = FVector::RightVector;
	}
	SpinAxis = (Side + Random.GetUnitVector() * 0.3f).GetSafeNormal();
	SpinRate = FGraveSaltRules::TumbleDegrees * FMath::Clamp(static_cast<float>(InVelocity.Size()) / 1500.f, 0.3f, 1.5f);
	// The fizz rides the fuse till the burst.
	if (!FuseSound.IsValid())
	{
		FuseSound = LooterSound::Start(this, ThrowCue::Fuse, Jar, Jar->DoesSocketExist(FuseSocket()) ? FuseSocket() : NAME_None);
	}
}

float AGraveSaltGrenade::GetFuseLeft() const
{
	return FMath::Max(FGraveSaltRules::FuseSeconds - Age, 0.f);
}

void AGraveSaltGrenade::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Step(DeltaSeconds);
}

void AGraveSaltGrenade::Step(float DeltaSeconds)
{
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	if (bBurst)
	{
		// The flash fades fast, as a burst's does; then nothing is left of it.
		SinceBurst += Dt;
		const float Left = FMath::Clamp(1.f - SinceBurst / FlashSeconds, 0.f, 1.f);
		Flash->SetIntensity(FlashCandelas * Left * Left);
		if (Left <= 0.f)
		{
			Flash->SetVisibility(false);
		}
		if (SinceBurst >= LingerSeconds)
		{
			Destroy();
		}
		return;
	}
	if (!bLaunched)
	{
		return;
	}
	Age += Dt;
	if (!bResting)
	{
		Fly(Dt);
		if (bBurst)
		{
			return;
		}
		TumbleJar(Dt);
	}
	DrawFuse();
	if (Age >= FGraveSaltRules::FuseSeconds)
	{
		Detonate();
	}
}

void AGraveSaltGrenade::Fly(float Dt)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const float GravityZ = World->GetGravityZ();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GraveSaltFlight), /*bTraceComplex*/ false, this);
	if (APawn* By = Thrower.Get())
	{
		// Never the hand that threw it, nor the gun in that hand.
		Params.AddIgnoredActor(By);
		TArray<AActor*> Held;
		By->GetAttachedActors(Held);
		Params.AddIgnoredActors(Held);
	}
	const FCollisionShape Ball = FCollisionShape::MakeSphere(FGraveSaltRules::CollisionRadius);

	FVector Location = GetActorLocation();
	float Left = Dt;
	// A roll's drag is felt once a frame, for the whole frame, however many times the frame touches the ground.
	bool bRolledThisStep = false;
	for (int32 Touch = 0; Touch < MaxContactsPerStep && Left > 0.f; ++Touch)
	{
		FVector Next = Location;
		FVector NextVelocity = Velocity;
		FGraveSaltRules::Fly(Next, NextVelocity, Left, GravityZ);
		FHitResult Hit;
		if (!World->SweepSingleByChannel(Hit, Location, Next, FQuat::Identity, SweepChannel, Ball, Params))
		{
			Location = Next;
			Velocity = NextVelocity;
			break;
		}
		if (BurstsOn(Hit))
		{
			// A creature: it bursts on it, there and then.
			SetActorLocation(Hit.bStartPenetrating ? Location : FVector(Hit.Location));
			Detonate();
			return;
		}
		if (Hit.bStartPenetrating)
		{
			// Thrown from inside something (a wall right at the hand): out the way it faces, bouncing off it.
			Location += Hit.Normal * (Hit.PenetrationDepth + 0.5f);
			Velocity = FGraveSaltRules::Bounce(Velocity, Hit.Normal);
			continue;
		}
		// Up to the contact along the step (the arc is near enough straight over one frame), then off the surface.
		const float Used = Left * FMath::Clamp(Hit.Time, 0.f, 1.f);
		FVector Swept = Location;
		FGraveSaltRules::Fly(Swept, Velocity, Used, GravityZ);
		// Where the sweep stopped, a hair off the surface, rather than the arc's own point: never inside it.
		Location = Hit.Location + Hit.ImpactNormal * 0.1f;
		const float Into = -static_cast<float>(FVector::DotProduct(Velocity, Hit.ImpactNormal));
		bool bRolled = false;
		Velocity = FGraveSaltRules::Contact(Velocity, Hit.ImpactNormal, bRolledThisStep ? 0.f : Dt, bRolled);
		bRolledThisStep |= bRolled;
		// Every real knock is heard, the one it rolls away from too; a roll's skitter (a frame's fall into the ground) isn't.
		if (Into >= FGraveSaltRules::AudibleSpeed)
		{
			++Bounces;
			Clink(Hit.ImpactPoint, Into);
			// Each hard knock takes some of the spin out of it.
			SpinRate *= 0.6f;
		}
		if (FGraveSaltRules::ShouldRest(Velocity, Hit.ImpactNormal))
		{
			Velocity = FVector::ZeroVector;
			bResting = true;
			break;
		}
		// A contact right at the start of what's left still uses a sliver of time, so the loop always moves on.
		Left -= FMath::Max(Used, 0.0005f);
	}
	SetActorLocation(Location);
}

bool AGraveSaltGrenade::BurstsOn(const FHitResult& Hit) const
{
	const AActor* Actor = Hit.GetActor();
	if (!Actor || Actor == Thrower.Get())
	{
		return false;
	}
	const APawn* Pawn = Cast<APawn>(Actor);
	if (Pawn && Pawn->IsPlayerControlled())
	{
		return false;
	}
	const UHealthComponent* Health = Actor->FindComponentByClass<UHealthComponent>();
	return Health && !Health->IsDead();
}

void AGraveSaltGrenade::Clink(const FVector& Where, float Speed)
{
	// A tin knocked about: louder the harder, never a rattle (the cue's own concurrency, and a gap here).
	if (LastClink >= 0.f && Age - LastClink < 0.06f)
	{
		return;
	}
	LastClink = Age;
	const float Volume = FMath::GetMappedRangeValueClamped(FVector2f(FGraveSaltRules::AudibleSpeed, 900.f), FVector2f(0.35f, 1.f), Speed);
	LooterSound::PlayAt(this, ThrowCue::Bounce, Where, Volume, FMath::GetMappedRangeValueClamped(FVector2f(80.f, 900.f), FVector2f(1.08f, 0.96f), Speed));
}

void AGraveSaltGrenade::TumbleJar(float Dt)
{
	// Spinning while it flies and bounces; once it rolls it only turns over slowly, and lying still not at all.
	const float Rate = Velocity.Size() < FGraveSaltRules::RollSpeed * 2.f ? SpinRate * 0.25f : SpinRate;
	if (Rate <= 0.f)
	{
		return;
	}
	const FQuat Turn(SpinAxis, FMath::DegreesToRadians(Rate * Dt));
	SetActorRotation(Turn * GetActorQuat());
}

void AGraveSaltGrenade::DrawFuse()
{
	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Bullets)
	{
		return;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	const FVector Tip = Jar->DoesSocketExist(FuseSocket()) ? Jar->GetSocketLocation(FuseSocket())
		: GetActorTransform().TransformPosition(FVector(0.f, 0.f, FuseTipFallback - JarMiddle));
	// Burning down: brighter and busier as the fuse runs out, so a glance says how long is left.
	const float Burn = FMath::Clamp(Age / FGraveSaltRules::FuseSeconds, 0.f, 1.f);
	for (int32 Index = 0; Index < FuseSparksPerFrame; ++Index)
	{
		const FVector Out = (Random.GetUnitVector() + FVector::UpVector * 0.6f).GetSafeNormal();
		const float Length = Random.FRandRange(2.f, 6.f) * (1.f + Burn);
		Effects.AddStreak(Tip, Tip + Out * Length, 0.6f, FuseSparkColor, Random.FRandRange(10.f, 22.f) * (0.7f + 0.6f * Burn));
	}
}

void AGraveSaltGrenade::Detonate()
{
	if (bBurst)
	{
		return;
	}
	bBurst = true;
	SinceBurst = 0.f;
	StopFuseSound();
	Jar->SetVisibility(false);
	Flash->SetIntensity(FlashCandelas);
	Flash->SetVisibility(true);

	UWorld* World = GetWorld();
	APawn* By = Thrower.Get();
	const FGraveSaltBurstResult Result = GraveSaltBurst::Detonate(World, GetActorLocation(), FVector::UpVector, By, LevelScale);
	if (UPlayerThrowComponent* Throw = UPlayerThrowComponent::Find(By))
	{
		Throw->NotifyBurst(Result);
	}
	UE_LOG(LogLooter, Verbose, TEXT("Grave salt: burst after %.2f s, %d bounces, %d hit, %d killed"), Age, Bounces, Result.Hits, Result.Kills);
}

void AGraveSaltGrenade::StopFuseSound()
{
	LooterSound::Stop(FuseSound.Get(), 0.05f);
	FuseSound.Reset();
}

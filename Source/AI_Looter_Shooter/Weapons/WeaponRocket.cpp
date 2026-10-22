#include "Weapons/WeaponRocket.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Weapons/GunKindsTuning.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponRocketBlast.h"
#include "CollisionQueryParams.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** The drawn model closes onto the true path at this rate (1/s): within a few metres at the rocket's speed. */
	constexpr float VisualCloseRate = 12.f;
	/** Smoke is left behind the motor every this many seconds. */
	constexpr float SmokeEvery = 0.03f;
	/** Without a socket on its model, the motor's exhaust is this far behind its seat (cm, the model's length). */
	constexpr float ExhaustBehind = 44.f;
	/** After the burst it stays this long (its last drawn frame clears), then goes. */
	constexpr float LingerSeconds = 0.1f;
	const FName ExhaustSocket(TEXT("Exhaust"));
}

AWeaponRocket::AWeaponRocket()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetGenerateOverlapEvents(false);
	Body->SetCanEverAffectNavigation(false);
	Body->SetCastShadow(false);
}

AWeaponRocket* AWeaponRocket::Launch(UWorld* World, const FVector& Start, const FVector& Direction, const FVector& VisualStart, AWeaponBase* Gun)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWeaponRocket* Rocket = World->SpawnActor<AWeaponRocket>(AWeaponRocket::StaticClass(), FTransform(Direction.Rotation(), Start), Params);
	if (!Rocket)
	{
		return nullptr;
	}
	const FVector Way = Direction.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	Rocket->Weapon = Gun;
	Rocket->Shooter = Gun ? Gun->GetOwner() : nullptr;
	Rocket->InstigatorController = Gun ? Gun->GetInstigatorController() : nullptr;
	Rocket->Channel = Gun ? Gun->TraceChannel.GetValue() : ECC_Visibility;
	Rocket->Position = Start;
	Rocket->Velocity = Way * GunKinds::RocketSpeed();
	Rocket->VisualOffset = VisualStart - Start;
	Rocket->Damage = Gun ? Gun->GetStats().Damage : 0.f;
	const UWeaponDefinition* Definition = Gun ? Gun->GetInstance().Definition.Get() : nullptr;
	Rocket->Impulse = Definition ? Definition->HitImpulse : 0.f;
	if (UStaticMesh* Mesh = Definition ? Definition->ProjectileMesh.Get() : nullptr)
	{
		Rocket->Body->SetStaticMesh(Mesh);
	}
	else
	{
		// Before the rocket model's import: a stand-in shape, so the flight still reads.
		Rocket->Body->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
		Rocket->Body->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
		Rocket->Body->SetRelativeScale3D(FVector(0.08f, 0.08f, 0.4f));
	}
	Rocket->Random.Initialize(static_cast<int32>(GetTypeHash(Start)));
	Rocket->bLaunched = true;
	// Heard as it flies, on the rocket itself (the burst stops it).
	Rocket->FlightSound = LooterSound::Start(World, LooterSoundCue::Launcher::Flight, Rocket->Body);
	Rocket->DrawFlight();
	UE_LOG(LogLooter, Verbose, TEXT("Rocket from %s along %s, %.0f damage"), *Start.ToCompactString(), *Way.ToCompactString(), Rocket->Damage);
	return Rocket;
}

void AWeaponRocket::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Step(DeltaSeconds);
}

void AWeaponRocket::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	LooterSound::Stop(FlightSound.Get());
	Super::EndPlay(EndPlayReason);
}

void AWeaponRocket::Step(float DeltaSeconds)
{
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	if (bBurst)
	{
		SinceBurst += Dt;
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
	UWorld* World = GetWorld();
	Age += Dt;
	if (!World || Age >= GunKinds::RocketLife())
	{
		// Out of fuel and time: it bursts where it is (in the air, nothing struck).
		Burst(Position, -Velocity.GetSafeNormal(), nullptr);
		return;
	}

	// A straight flight with a slight drop, swept from where it was to where it's going: the first thing in the way bursts it.
	Velocity.Z -= GunKinds::RocketGravity() * Dt;
	const FVector Next = Position + Velocity * Dt;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RocketSweep), false, this);
	Params.bReturnPhysicalMaterial = true;
	if (AActor* From = Shooter.Get())
	{
		Params.AddIgnoredActor(From);
	}
	if (AWeaponBase* Gun = Weapon.Get())
	{
		Params.AddIgnoredActor(Gun);
	}
	FHitResult Hit;
	if (World->SweepSingleByChannel(Hit, Position, Next, FQuat::Identity, Channel, FCollisionShape::MakeSphere(CollisionRadius), Params))
	{
		// It bursts where it touched, a hair off the surface along its normal.
		const FVector Normal = Hit.ImpactNormal.IsNearlyZero() ? -Velocity.GetSafeNormal() : Hit.ImpactNormal;
		Position = Hit.Location;
		Burst(Hit.ImpactPoint, Normal, &Hit);
		return;
	}
	Position = Next;
	VisualOffset *= FMath::Exp(-VisualCloseRate * Dt);
	DrawFlight();

	SmokeIn -= Dt;
	if (SmokeIn <= 0.f)
	{
		SmokeIn = SmokeEvery;
		const FVector Way = Velocity.GetSafeNormal();
		const FVector Exhaust = Body->DoesSocketExist(ExhaustSocket) ? Body->GetSocketLocation(ExhaustSocket) : GetActorLocation() - Way * ExhaustBehind;
		WeaponRocketBlast::PuffSmoke(*World, Exhaust, Way, Random);
	}
}

void AWeaponRocket::DrawFlight()
{
	const FVector Way = Velocity.GetSafeNormal(UE_SMALL_NUMBER, GetActorForwardVector());
	SetActorLocationAndRotation(Position + VisualOffset, Way.Rotation());
	if (UWorld* World = GetWorld())
	{
		const FVector Exhaust = Body->DoesSocketExist(ExhaustSocket) ? Body->GetSocketLocation(ExhaustSocket) : GetActorLocation() - Way * ExhaustBehind;
		WeaponRocketBlast::DrawFlame(*World, Exhaust, Way);
	}
}

void AWeaponRocket::Burst(const FVector& Location, const FVector& Normal, const FHitResult* Hit)
{
	if (bBurst)
	{
		return;
	}
	bBurst = true;
	Body->SetVisibility(false);
	LooterSound::Stop(FlightSound.Get(), 0.05f);
	FlightSound.Reset();
	SetActorLocation(Location);
	WeaponRocketBlast::Detonate(GetWorld(), Location, Normal, Weapon.Get(), Shooter.Get(), InstigatorController.Get(), Damage, Impulse, Hit, this);
}

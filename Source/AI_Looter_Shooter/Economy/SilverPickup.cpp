// ASilverPickup: silver on the ground, its look, and its pull to the player.

#include "Economy/SilverPickup.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Combat/HealthComponent.h"
#include "Economy/WalletSubsystem.h"
#include "Loot/LootTossComponent.h"
#include "Loot/SoulMotePickup.h"
#include "World/LightBeam.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/Package.h"

namespace
{
	const TCHAR* ModelPath = TEXT("/Game/Art/Loot/SM_SilverCoins.SM_SilverCoins");
	const TCHAR* DiscPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");

	/** How often a pile that isn't pulled yet looks for the player (s). */
	constexpr float LookInterval = 0.15f;

	/** The beam: the ammo drops' white (colours on loot mean rarity), shorter, since a pile comes to the player anyway. */
	constexpr float BeamHeight = 110.f;
	constexpr float BeamRadius = 3.5f;
	constexpr float BeamGlow = 1.1f;

	/** The coins spin slowly lying there, fast as they're pulled. */
	constexpr float RestSpin = 110.f;
	constexpr float PulledSpin = 900.f;

	/** The glint as it's pulled: cool silver-white, a little over the pile's size, a hair above it. */
	const FLinearColor SilverGlintColor(0.85f, 0.93f, 1.f);
	constexpr float PullGlintSize = 28.f;
	constexpr double PullGlintLift = 4.0;

	/** The stand-in disc, until the model's imported: 13 cm across and 3 cm high at the pile's size. */
	const FVector DiscSize(13.0, 13.0, 3.0);

	/** An asset found only once it's imported, so the class works without it. In a constructor. */
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

	/** Every pile's beam looks the same: one shared material instance, as the ammo drops' (made again once all are gone). */
	UMaterialInterface* SharedBeamMaterial()
	{
		static TWeakObjectPtr<UMaterialInstanceDynamic> SharedInstance;
		if (!SharedInstance.IsValid())
		{
			UMaterialInstanceDynamic* Instance = LightBeams::CreateMaterial(GetTransientPackage(), FLinearColor::White, BeamGlow, BeamHeight);
			if (Instance)
			{
				Instance->SetFlags(RF_Transient);
			}
			SharedInstance = Instance;
		}
		return SharedInstance.Get();
	}
}

ASilverPickup::ASilverPickup()
{
	// It ticks only while it's pulled to the player.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Root: a small sphere that only bounces off world geometry while it's tossed (as the ammo pickups do).
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(8.f);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetRootComponent(Collision);

	static UStaticMesh* const Coins = FindIfMade<UStaticMesh>(ModelPath);
	static UStaticMesh* const Disc = FindIfMade<UStaticMesh>(DiscPath);
	Model = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Model"));
	Model->SetupAttachment(Collision);
	Model->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Model->SetCanEverAffectNavigation(false);
	if (Coins)
	{
		Model->SetStaticMesh(Coins);
		Model->SetRelativeLocation(FVector(0.0, 0.0, -7.0));
		Model->SetRelativeScale3D(FVector(ModelScale));
	}
	else
	{
		// The engine's cylinder (1 m, about its middle) squashed to a disc of coins.
		Model->SetStaticMesh(Disc);
		Model->SetRelativeLocation(FVector(0.0, 0.0, -7.0 + DiscSize.Z * 0.5));
		Model->SetRelativeScale3D(DiscSize / 100.0);
	}

	Beam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beam"));
	Beam->SetupAttachment(Collision);
	Beam->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Beam->SetCastShadow(false);
	Beam->SetCanEverAffectNavigation(false);

	TossMovement = CreateDefaultSubobject<ULootTossComponent>(TEXT("TossMovement"));
	TossMovement->SetUpdatedComponent(Collision);

	Spin = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("Spin"));
	Spin->RotationRate = FRotator(0.f, RestSpin, 0.f);
	Spin->SetUpdatedComponent(Model);
}

UStaticMesh* ASilverPickup::FindModel()
{
	return FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ModelPath)))
		? LoadObject<UStaticMesh>(nullptr, ModelPath) : nullptr;
}

ASilverPickup* ASilverPickup::SpawnSilver(UWorld* World, int32 Amount, const FVector& Location)
{
	if (!World || Amount <= 0)
	{
		return nullptr;
	}
	const FTransform Transform(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Location);
	ASilverPickup* Pickup = World->SpawnActorDeferred<ASilverPickup>(ASilverPickup::StaticClass(), Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Pickup)
	{
		Pickup->Amount = Amount;
		Pickup->FinishSpawning(Transform);
	}
	return Pickup;
}

void ASilverPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// The beam rises from the top of the coins, never through them.
	LightBeams::Setup(Beam, SharedBeamMaterial(), BeamHeight, BeamRadius);
	const UStaticMesh* Mesh = Model->GetStaticMesh();
	const double Top = Model->GetRelativeLocation().Z + (Mesh ? Mesh->GetBounds().GetBox().Max.Z * Model->GetRelativeScale3D().Z : 0.0);
	Beam->SetRelativeLocation(FVector(0.0, 0.0, Top + BeamHeight * 0.5));
}

void ASilverPickup::BeginPlay()
{
	Super::BeginPlay();
	SetLifeSpan(LifeSeconds);
	TossMovement->OnProjectileStop.AddDynamic(this, &ASilverPickup::HandleTossStopped);
	GetWorldTimerManager().SetTimer(LookTimer, this, &ASilverPickup::LookForPlayer, LookInterval, true, LookInterval);
}

void ASilverPickup::Toss(const FVector& Velocity)
{
	TossMovement->Throw(Velocity);
}

void ASilverPickup::HandleTossStopped(const FHitResult& ImpactResult)
{
	SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
}

void ASilverPickup::LookForPlayer()
{
	if (!bPulled)
	{
		Advance(LookInterval, FindPlayerPawn());
	}
}

APawn* ASilverPickup::FindPlayerPawn() const
{
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	return Controller ? Controller->GetPawn() : nullptr;
}

void ASilverPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds, FindPlayerPawn());
}

void ASilverPickup::StartPull(const APawn& Player)
{
	bPulled = true;
	Flight.Start(GetActorLocation(), PickupPull::TargetOf(Player), PullStartSpeed);
	LineCheckIn = LineCheckSeconds;
	// It flies by itself from here: the toss lets go of it, the beam goes out, the coins spin up.
	TossMovement->StopMovementImmediately();
	TossMovement->Deactivate();
	Beam->SetVisibility(false);
	Spin->RotationRate = FRotator(0.f, PulledSpin, 0.f);
	GetWorldTimerManager().ClearTimer(LookTimer);
	SetActorTickEnabled(true);
}

void ASilverPickup::Drop()
{
	bPulled = false;
	Flight = FPickupFlight();
	SetActorTickEnabled(false);
	Beam->SetVisibility(true);
	Spin->RotationRate = FRotator(0.f, RestSpin, 0.f);
	TossMovement->Activate(true);
	Toss(FVector::ZeroVector);
	GetWorldTimerManager().SetTimer(LookTimer, this, &ASilverPickup::LookForPlayer, LookInterval, true, LookInterval);
}

bool ASilverPickup::Advance(float DeltaSeconds, APawn* Player)
{
	if (bCollected || IsActorBeingDestroyed())
	{
		return false;
	}
	Age += DeltaSeconds;
	UWorld* World = GetWorld();
	const UHealthComponent* Health = Player ? Player->FindComponentByClass<UHealthComponent>() : nullptr;
	const bool bAlive = Player && World && !(Health && Health->IsDead());

	if (!bPulled)
	{
		// Near enough, and nothing solid between: a pile behind a wall waits for the player to come round it.
		if (!bAlive || Age < CollectDelay || !PickupPull::CanPull(*World, GetActorLocation(), *Player, MagnetRadius, this))
		{
			return false;
		}
		StartPull(*Player);
	}
	if (!bAlive)
	{
		// The player fell before it reached them: it drops where it is and waits again.
		Drop();
		return false;
	}
	// A wall come between them on the way (the player ducked round a corner): it drops there, never through it.
	LineCheckIn -= DeltaSeconds;
	if (LineCheckIn <= 0.f)
	{
		LineCheckIn = LineCheckSeconds;
		if (!PickupPull::HasClearLine(*World, GetActorLocation(), *Player, this))
		{
			Drop();
			return false;
		}
	}

	// Faster and faster for the player's middle, tossed in a low arc; theirs once it's at their body.
	const FVector Shown = GetActorLocation();
	const bool bArrived = Flight.Step(DeltaSeconds, PickupPull::TargetOf(*Player), PullAcceleration, PullMaxSpeed);
	if (bArrived || ASoulMotePickup::DistanceToBody(Flight.GetBase(), *Player) <= CollectDistance)
	{
		Collect();
		return true;
	}
	const FVector Next = Flight.GetLocation();
	SetActorLocation(Next, false, nullptr, ETeleportType::TeleportPhysics);
	// A glint as it sets off, the coins catching the light, and a faint trail behind it.
	PickupPull::DrawGlint(*World, Next + FVector(0.0, 0.0, PullGlintLift), PickupPull::GlintAt(Flight.GetAge()), PullGlintSize, SilverGlintColor);
	PickupPull::DrawTrail(*World, Shown, Next, 1.f - 0.6f * Flight.GetProgress(), SilverGlintColor);
	return false;
}

void ASilverPickup::Collect()
{
	if (bCollected)
	{
		return;
	}
	bCollected = true;
	if (UWalletSubsystem* Wallet = UWalletSubsystem::Get(this))
	{
		Wallet->Add(Amount, ESilverChange::Found);
	}
	// A few coins chime bright; a purse's worth rings a little lower and fuller.
	LooterSound::Play2D(this, LooterSoundCue::Economy::SilverPickup, 1.f, Amount >= 25 ? 0.92f : 1.f);
	UE_LOG(LogLooter, Verbose, TEXT("%s: %d silver picked up."), *GetName(), Amount);
	Destroy();
}

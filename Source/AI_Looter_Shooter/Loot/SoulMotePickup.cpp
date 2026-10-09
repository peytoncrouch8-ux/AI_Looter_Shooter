#include "Loot/SoulMotePickup.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/HealthComponent.h"
#include "Combat/PlayerVitalsSubsystem.h"
#include "UI/Style/LooterUIStyle.h"
#include "World/WorldQueries.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/Package.h"

namespace
{
	/** The stylized surface with its emissive multiplier (as the enemy pellets): opaque, so it never sorts or costs translucency. */
	const TCHAR* SurfaceMaterialPath = TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface");
	const TCHAR* SphereMeshPath = TEXT("/Engine/BasicShapes/Sphere.Sphere");

	/** How brightly the core and its trail glow (M_StylizedSurface's emissive multiplier of the color): the trail is the fainter. */
	constexpr float CoreGlow = 4.5f;
	constexpr float TrailGlow = 1.6f;

	/** The trailing spheres' sizes against the core's, and how quickly each eases after the one before it (1/s). */
	constexpr int32 TrailCount = 3;
	constexpr float TrailSizes[TrailCount] = { 0.72f, 0.5f, 0.32f };
	constexpr float TrailEase = 12.f;

	constexpr float TwoPi = static_cast<float>(UE_TWO_PI);

	/** The bob: a few centimeters up and down, about one breath every two seconds. */
	constexpr float BobAmplitude = 6.f;
	constexpr float BobHertz = 0.55f;

	/** The core swells and shrinks a little (the pulse), and flickers faster through its last seconds. */
	constexpr float PulseHertz = 1.7f;
	constexpr float PulseDepth = 0.07f;
	constexpr float FlickerHertz = 6.f;

	/**
	 * Floating: the height settles like a soft spring (a rise that overshoots a hair and comes to rest), and the drift it left
	 * the body with dies away in about a second.
	 */
	constexpr float HoverSpring = 36.f;
	constexpr float HoverDamping = 8.4f;
	constexpr float DriftDamping = 2.6f;
	/** Longest slice the motion is worked in (s), and the most time one call moves at all, so a long gap can't wind it up. */
	constexpr float MotionSlice = 0.05f;
	constexpr float MotionLongest = 1.f;

	/** The hover height is looked for in this span about the launch point (cm); with no ground found it floats this far over it. */
	constexpr float GroundSearchUp = 150.f;
	constexpr float GroundSearchDown = 1500.f;
	constexpr float NoGroundHover = 40.f;

	/** What one mote launches with (cm/s): a drift out along its heading, and a rise. */
	constexpr float DriftSpeedMin = 90.f;
	constexpr float DriftSpeedMax = 170.f;
	constexpr float RiseSpeedMin = 180.f;
	constexpr float RiseSpeedMax = 260.f;
	/** Motes start this far over the body's middle (cm), as dropped loot does. */
	constexpr float StartHeight = 60.f;

	/** One material instance for every core and one for every trail sphere: a fight leaves several motes, all alike. */
	UMaterialInterface* SharedGlow(bool bCore)
	{
		static TWeakObjectPtr<UMaterialInstanceDynamic> CoreInstance;
		static TWeakObjectPtr<UMaterialInstanceDynamic> TrailInstance;
		TWeakObjectPtr<UMaterialInstanceDynamic>& Slot = bCore ? CoreInstance : TrailInstance;
		if (!Slot.IsValid())
		{
			if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, SurfaceMaterialPath))
			{
				// In the transient package, kept alive by the motes that wear it and made again after the last is gone.
				UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(Base, GetTransientPackage());
				Instance->SetFlags(RF_Transient);
				// The HUD's heal green: the one color on a drop that isn't a rarity.
				Instance->SetVectorParameterValue(TEXT("Color"), LooterUI::Color::Heal());
				Instance->SetScalarParameterValue(TEXT("Glow"), bCore ? CoreGlow : TrailGlow);
				Instance->SetScalarParameterValue(TEXT("Variation"), 0.f);
				Slot = Instance;
			}
		}
		return Slot.Get();
	}

	UStaticMeshComponent* MakeSphere(AActor* Owner, const TCHAR* Name, USceneComponent* Parent, UStaticMesh* Mesh)
	{
		UStaticMeshComponent* Sphere = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Sphere->SetupAttachment(Parent);
		Sphere->SetStaticMesh(Mesh);
		Sphere->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Sphere->SetGenerateOverlapEvents(false);
		Sphere->SetCastShadow(false);
		Sphere->SetCanEverAffectNavigation(false);
		Sphere->bReceivesDecals = false;
		return Sphere;
	}
}

ASoulMotePickup::ASoulMotePickup()
{
	PrimaryActorTick.bCanEverTick = true;
	SetCanBeDamaged(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(SphereMeshPath);
	Core = MakeSphere(this, TEXT("Core"), Root, Sphere.Object);
	for (int32 Index = 0; Index < TrailCount; ++Index)
	{
		UStaticMeshComponent* Follower = MakeSphere(this, *FString::Printf(TEXT("Trail%d"), Index), Root, Sphere.Object);
		// They lag behind the core in the world, so they ignore where the actor goes.
		Follower->SetUsingAbsoluteLocation(true);
		Follower->SetUsingAbsoluteScale(true);
		Trail.Add(Follower);
	}
}

void ASoulMotePickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Core->SetMaterial(0, SharedGlow(true));
	for (UStaticMeshComponent* Follower : Trail)
	{
		Follower->SetMaterial(0, SharedGlow(false));
	}
	TrailAt.Init(Transform.GetLocation(), TrailCount);
	UpdateVisuals(0.f);
}

void ASoulMotePickup::BeginPlay()
{
	Super::BeginPlay();
	// A net under Advance's own end: nothing stays in the world past its life if a tick is ever missed.
	SetLifeSpan(Settings.MoteLifeSeconds + 5.f);
}

ASoulMotePickup* ASoulMotePickup::SpawnMote(UWorld* World, const FVector& Where, const FVector& Velocity)
{
	if (!World)
	{
		return nullptr;
	}
	const FTransform Transform(FRotator::ZeroRotator, Where);
	ASoulMotePickup* Mote = World->SpawnActorDeferred<ASoulMotePickup>(ASoulMotePickup::StaticClass(), Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Mote)
	{
		Mote->Settings = UPlayerVitalsSubsystem::SettingsFor(World);
		Mote->Launch(Where, Velocity);
		Mote->FinishSpawning(Transform);
	}
	return Mote;
}

TArray<ASoulMotePickup*> ASoulMotePickup::SpawnMotes(UWorld* World, const FVector& Where, int32 Count)
{
	TArray<ASoulMotePickup*> Motes;
	if (!World || Count <= 0)
	{
		return Motes;
	}
	// Each on its own heading round the ring, so two or three from a boss don't lie on top of each other.
	const float FirstYaw = FMath::FRandRange(0.f, 360.f);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector Heading = FRotator(0.f, FirstYaw + 360.f * Index / Count, 0.f).Vector();
		const FVector Velocity = Heading * FMath::FRandRange(DriftSpeedMin, DriftSpeedMax) + FVector(0.f, 0.f, FMath::FRandRange(RiseSpeedMin, RiseSpeedMax));
		if (ASoulMotePickup* Mote = SpawnMote(World, Where + FVector(0.f, 0.f, StartHeight), Velocity))
		{
			Motes.Add(Mote);
		}
	}
	if (!Motes.IsEmpty())
	{
		LooterSound::PlayAt(World, SoulMoteCue::Drop, Where);
		UE_LOG(LogLooter, Verbose, TEXT("%d soul-mote(s) dropped at %s"), Motes.Num(), *Where.ToCompactString());
	}
	return Motes;
}

void ASoulMotePickup::Launch(const FVector& From, const FVector& InVelocity)
{
	Velocity = InVelocity;
	BobPhase = FMath::FRandRange(0.f, TwoPi);

	// It floats over the ground below the body (the terrain and solid props, never the grass or the dead): chest height.
	UWorld* World = GetWorld();
	HoverZ = From.Z + NoGroundHover;
	if (World)
	{
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("SoulMoteGround"), this);
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, From + FVector(0.0, 0.0, GroundSearchUp), From - FVector(0.0, 0.0, GroundSearchDown),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			HoverZ = Hit.ImpactPoint.Z + Settings.MoteHoverHeight;
		}
	}
}

bool ASoulMotePickup::NeedsHealing(const UHealthComponent& Health)
{
	// Half a point short counts as full: a mote is not spent on a scratch.
	return !Health.IsDead() && Health.GetHealth() < Health.GetMaxHealth() - 0.5f;
}

float ASoulMotePickup::DistanceToBody(const FVector& Point, const APawn& Player)
{
	if (const ACharacter* Character = Cast<ACharacter>(&Player))
	{
		if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			// The capsule is a line segment thickened by its radius: the distance to its surface.
			const FVector Middle = Capsule->GetComponentLocation();
			const float Radius = Capsule->GetScaledCapsuleRadius();
			const float Straight = FMath::Max(Capsule->GetScaledCapsuleHalfHeight() - Radius, 0.f);
			const double ClosestZ = FMath::Clamp(Point.Z, Middle.Z - Straight, Middle.Z + Straight);
			return static_cast<float>(FMath::Max(FVector::Dist(Point, FVector(Middle.X, Middle.Y, ClosestZ)) - Radius, 0.0));
		}
	}
	return static_cast<float>(FVector::Dist(Point, Player.GetActorLocation()));
}

APawn* ASoulMotePickup::FindPlayerPawn() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	APawn* Nearest = nullptr;
	double NearestDistance = TNumericLimits<double>::Max();
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		APawn* Pawn = PC && PC->IsLocalController() ? PC->GetPawn() : nullptr;
		if (!Pawn)
		{
			continue;
		}
		const double Distance = FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation());
		if (Distance < NearestDistance)
		{
			NearestDistance = Distance;
			Nearest = Pawn;
		}
	}
	return Nearest;
}

void ASoulMotePickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds, FindPlayerPawn());
}

bool ASoulMotePickup::Advance(float DeltaSeconds, APawn* Player)
{
	DeltaSeconds = FMath::Max(DeltaSeconds, 0.f);
	Age += DeltaSeconds;
	if (Age >= Settings.MoteLifeSeconds)
	{
		// It has flickered out through its last seconds (UpdateVisuals): now it's gone.
		Destroy();
		return false;
	}

	UHealthComponent* Health = Player ? Player->FindComponentByClass<UHealthComponent>() : nullptr;
	if (Health && Age >= Settings.MoteCollectDelay && NeedsHealing(*Health))
	{
		FVector Position = GetActorLocation();
		float Distance = DistanceToBody(Position, *Player);
		if (Distance <= Settings.MoteCollectRadius)
		{
			Collect(*Player, *Health);
			return true;
		}
		if (Distance <= Settings.MoteMagnetRadius)
		{
			// Drifting to them, the faster the nearer: a player running past is caught, one walking up is met halfway.
			const float Closeness = 1.f - Distance / FMath::Max(Settings.MoteMagnetRadius, 1.f);
			const float Speed = FMath::Lerp(Settings.MoteMagnetSpeedMin, Settings.MoteMagnetSpeedMax, FMath::Clamp(Closeness, 0.f, 1.f));
			const FVector ToBody = Player->GetActorLocation() - Position;
			const double ToBodyLength = ToBody.Size();
			if (ToBodyLength > UE_KINDA_SMALL_NUMBER)
			{
				Position += ToBody / ToBodyLength * FMath::Min(static_cast<double>(Speed * DeltaSeconds), ToBodyLength);
				SetActorLocation(Position);
			}
			// The drift it had is spent: let go of the player, it settles back to hover from here.
			Velocity = FVector::ZeroVector;
			Distance = DistanceToBody(Position, *Player);
			if (Distance <= Settings.MoteCollectRadius)
			{
				Collect(*Player, *Health);
				return true;
			}
			UpdateVisuals(DeltaSeconds);
			return false;
		}
	}

	Float(DeltaSeconds);
	UpdateVisuals(DeltaSeconds);
	return false;
}

void ASoulMotePickup::Collect(APawn& Player, UHealthComponent& Health)
{
	const float Healed = Health.Heal(Settings.MoteHeal(Health.GetMaxHealth()));
	UE_LOG(LogLooter, Verbose, TEXT("%s took a soul-mote: healed %.1f (now %.1f / %.1f)"), *Player.GetName(), Healed, Health.GetHealth(), Health.GetMaxHealth());
	LooterSound::PlayAt(this, SoulMoteCue::Pickup, GetActorLocation());
	Destroy();
}

void ASoulMotePickup::Float(float DeltaSeconds)
{
	FVector Position = GetActorLocation();
	float Left = FMath::Min(DeltaSeconds, MotionLongest);
	while (Left > 0.f)
	{
		const float Slice = FMath::Min(Left, MotionSlice);
		Left -= Slice;
		const double Target = HoverZ + BobAmplitude * FMath::Sin(TwoPi * BobHertz * Age + BobPhase);
		Velocity.Z += (HoverSpring * (Target - Position.Z) - HoverDamping * Velocity.Z) * Slice;
		const float Drift = FMath::Exp(-DriftDamping * Slice);
		Velocity.X *= Drift;
		Velocity.Y *= Drift;
		Position += Velocity * Slice;
	}
	SetActorLocation(Position);
}

void ASoulMotePickup::UpdateVisuals(float DeltaSeconds)
{
	const float Visible = Settings.MoteVisible(Age);
	float Size = Visible * (1.f + PulseDepth * FMath::Sin(TwoPi * PulseHertz * Age));
	if (Visible < 1.f)
	{
		// Guttering out: it flickers between a third and full size as it shrinks.
		Size *= 0.65f + 0.35f * FMath::Sin(TwoPi * FlickerHertz * Age);
	}
	const float Scale = CoreDiameter / 100.f * FMath::Max(Size, 0.f);
	Core->SetRelativeScale3D(FVector(Scale));

	// Each trailing sphere eases after the one before it: a streak behind it when it moves, tucked inside it when it rests.
	FVector Leader = GetActorLocation();
	const float Follow = 1.f - FMath::Exp(-TrailEase * DeltaSeconds);
	for (int32 Index = 0; Index < Trail.Num() && Index < TrailAt.Num(); ++Index)
	{
		TrailAt[Index] = FMath::Lerp(TrailAt[Index], Leader, static_cast<double>(Follow));
		Leader = TrailAt[Index];
		Trail[Index]->SetWorldLocation(TrailAt[Index]);
		Trail[Index]->SetWorldScale3D(FVector(Scale * TrailSizes[Index]));
	}
}

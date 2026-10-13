#include "Creatures/ForemanRockFall.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossCameraShake.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/CombatRules.h"
#include "Combat/HealthComponent.h"
#include "Combat/LooterDamageTypes.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/ForemanTimberProp.h"
#include "Creatures/GroundCrack.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The level's own rocks: one of them drops, sized to RockSize. */
	const TCHAR* const RockPaths[] = {
		TEXT("/Game/Art/Rocks/SM_Rock_A.SM_Rock_A"),
		TEXT("/Game/Art/Rocks/SM_Rock_B.SM_Rock_B"),
		TEXT("/Game/Art/Rocks/SM_Rock_C.SM_Rock_C"),
		TEXT("/Game/Art/Rocks/SM_Rock_D.SM_Rock_D"),
	};

	/** The mark: a ring of cracks on the floor, its seam the dust's dull amber, and how wide its cracks are (cm). */
	const FLinearColor MarkColor(1.f, 0.55f, 0.22f);
	constexpr float MarkWidth = 14.f;

	/** Dust sifting down from the roof: a puff this often (s), unlit and dim. */
	constexpr float SiftEvery = 0.09f;
	const FLinearColor DustColor(0.3f, 0.27f, 0.23f);

	/** The roof is looked for this far up at most (cm): the stope is 7 m high. */
	constexpr float RoofSearch = 1200.f;

	/** It reaches a player whose feet are within this of the spot's floor, up or down (cm). */
	constexpr float ReachRise = 200.f;

	/** The fall knocks a player out from under it, and jolts the view of anyone near. */
	constexpr float KnockSpeed = 420.f;
	constexpr float ShakeStrength = 0.5f;

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

	FWeaponFX* EffectsIn(UWorld* World)
	{
		UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
		if (!Bullets)
		{
			return nullptr;
		}
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		return &Effects;
	}
}

AForemanRockFall::AForemanRockFall()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	SetCanBeDamaged(false);

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	static UStaticMesh* Rocks[UE_ARRAY_COUNT(RockPaths)] = {};
	static bool bLooked = false;
	if (!bLooked)
	{
		bLooked = true;
		for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(RockPaths)); ++Index)
		{
			Rocks[Index] = FindIfMade<UStaticMesh>(RockPaths[Index]);
		}
	}
	Rock = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rock"));
	Rock->SetupAttachment(Root);
	Rock->SetMobility(EComponentMobility::Movable);
	Rock->SetStaticMesh(Rocks[0]);
	// It hurts by its own rule (its reach), never by its collision: nothing meets it.
	Rock->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Rock->SetCanEverAffectNavigation(false);
	Rock->SetVisibility(false);
	// Any of the four, so a roof strike's falls don't all look alike.
	for (UStaticMesh* Each : Rocks)
	{
		if (Each && FMath::RandBool())
		{
			Rock->SetStaticMesh(Each);
		}
	}
}

AForemanRockFall* AForemanRockFall::Spawn(UWorld* World, const FVector& Floor, float InRadius, float InDamage, AActor* By, float InDustSeconds,
	const TArray<AForemanTimberProp*>& InProps)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AForemanRockFall* Fall = World->SpawnActor<AForemanRockFall>(Floor, FRotator(0.0, FMath::FRandRange(0.f, 360.f), 0.0), Params);
	if (!Fall)
	{
		return nullptr;
	}
	Fall->Spot = Floor;
	Fall->Radius = FMath::Max(InRadius, 10.f);
	Fall->Damage = FMath::Max(InDamage, 0.f);
	Fall->DustSeconds = FMath::Max(InDustSeconds, 0.f);
	Fall->BroughtBy = By;
	for (AForemanTimberProp* Prop : InProps)
	{
		Fall->Props.Add(Prop);
	}
	Fall->Begin();
	return Fall;
}

void AForemanRockFall::Begin()
{
	UWorld* World = GetWorld();
	// The roof over the spot: the stope's rock, found straight up.
	Roof = Spot + FVector(0.0, 0.0, RoofFallback);
	if (World)
	{
		FHitResult Hit;
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("ForemanRoof"), this);
		if (World->LineTraceSingleByObjectType(Hit, Spot + FVector(0.0, 0.0, 60.0), Spot + FVector(0.0, 0.0, RoofSearch),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			Roof = Hit.ImpactPoint - FVector(0.0, 0.0, 20.0);
		}
	}
	// The rock about RockSize across, whatever its model's own size.
	if (const UStaticMesh* Mesh = Rock->GetStaticMesh())
	{
		const float Across = static_cast<float>(Mesh->GetBounds().BoxExtent.GetMax()) * 2.f;
		RockScale = Across > 1.f ? RockSize / Across : 1.f;
	}
	Rock->SetRelativeScale3D(FVector(RockScale));
	// The mark: a ring of cracks spreading on the floor to the fall's reach while the dust sifts down.
	if (AGroundCrack* Ring = AGroundCrack::Spawn(World, Spot, Spot + FVector::ForwardVector, MarkWidth, MarkColor, FMath::Rand()))
	{
		Ring->BurstSeconds = FMath::Max(DustSeconds, 0.05f);
		Ring->Burst(Spot, Radius);
		Mark = Ring;
	}
	SiftDust();
}

void AForemanRockFall::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AGroundCrack* Ring = Mark.Get())
	{
		Ring->Close(0.f);
	}
	Mark.Reset();
	Super::EndPlay(EndPlayReason);
}

void AForemanRockFall::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void AForemanRockFall::Advance(float DeltaSeconds)
{
	Age += DeltaSeconds;
	if (!bFallen)
	{
		if (Age < DustSeconds)
		{
			// The dust thickens as the roof gives.
			DustClock -= DeltaSeconds;
			if (DustClock <= 0.f)
			{
				DustClock = SiftEvery * FMath::Lerp(1.6f, 0.6f, FMath::Clamp(Age / FMath::Max(DustSeconds, 0.01f), 0.f, 1.f));
				SiftDust();
			}
			return;
		}
		// The rock drops from the roof onto the spot.
		const float Drop = FMath::Clamp((Age - DustSeconds) / DropSeconds, 0.f, 1.f);
		Rock->SetVisibility(true);
		Rock->SetWorldLocation(FMath::Lerp(Roof, Spot + FVector(0.0, 0.0, RockSize * 0.35f), static_cast<double>(Drop * Drop)));
		if (Drop >= 1.f)
		{
			Land();
		}
		return;
	}
	// The rubble lies a while, then sinks away and goes.
	const float Lying = Age - DustSeconds - DropSeconds;
	if (Lying > RubbleSeconds)
	{
		const float Sink = FMath::Clamp((Lying - RubbleSeconds) / SinkSeconds, 0.f, 1.f);
		Rock->SetWorldLocation(Spot + FVector(0.0, 0.0, RockSize * (0.35f - 0.8f * Sink)));
		if (Sink >= 1.f)
		{
			Destroy();
		}
	}
}

void AForemanRockFall::SiftDust()
{
	if (FWeaponFX* Effects = EffectsIn(GetWorld()))
	{
		// From the roof over the spot, falling and spreading: the mark to step out from under.
		const FVector From = Roof + FVector(FMath::FRandRange(-0.4f, 0.4f) * Radius, FMath::FRandRange(-0.4f, 0.4f) * Radius, 0.0);
		Effects->SpawnDustPuff(From, FVector(0.0, 0.0, -FMath::FRandRange(220.f, 340.f)), DustColor, 0.45f, 10.f, 55.f, 1.8f);
		Effects->SpawnGrit(From, FVector(FMath::FRandRange(-20.f, 20.f), FMath::FRandRange(-20.f, 20.f), -120.f), 2.f, 2.f);
	}
}

void AForemanRockFall::Land()
{
	bFallen = true;
	if (AGroundCrack* Ring = Mark.Get())
	{
		Ring->Close(0.4f);
	}
	Mark.Reset();
	bHitPlayer = HurtPlayers();
	// The props it lands on splinter: the cover changes.
	for (const TWeakObjectPtr<AForemanTimberProp>& Each : Props)
	{
		AForemanTimberProp* Prop = Each.Get();
		if (Prop && FVector::Dist2D(Prop->GetFoot(), Spot) <= Radius + 60.f)
		{
			Prop->Splinter(Spot);
		}
	}
	// The crash: dirt and grit thrown out, dust rolling, the rock's slam and gravel's slide, the view jolted.
	if (FWeaponFX* Effects = EffectsIn(GetWorld()))
	{
		Effects->SpawnDirt(Spot, FVector::UpVector, 1.f);
		for (int32 Puff = 0; Puff < 6; ++Puff)
		{
			const FVector Out = FVector::ForwardVector.RotateAngleAxis(60.f * Puff + FMath::FRandRange(-15.f, 15.f), FVector::UpVector);
			Effects->SpawnDustPuff(Spot + Out * 30.f + FVector(0.0, 0.0, 25.0), Out * FMath::FRandRange(180.f, 300.f), DustColor, 0.6f, 40.f,
				170.f, 2.4f);
		}
		for (int32 Chip = 0; Chip < 10; ++Chip)
		{
			Effects->SpawnGrit(Spot + FVector(0.0, 0.0, 20.0), FMath::VRand() * 260.f + FVector(0.0, 0.0, 240.0), FMath::FRandRange(3.f, 8.f), 1.6f);
		}
	}
	LooterSound::PlayAt(this, LooterSoundCue::Foreman::RockFall, Spot);
	BossCameraShake::Kick(this, Spot, ShakeStrength, 0.6f, 2500.f);
}

bool AForemanRockFall::HurtPlayers()
{
	UWorld* World = GetWorld();
	if (!World || Damage <= 0.f)
	{
		return false;
	}
	// Whoever plays: every local player's pawn, and the encounters' player (a test's stand-in has no controller).
	TArray<APawn*, TInlineAllocator<4>> Players;
	if (const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this))
	{
		if (APawn* Player = Encounters->GetPlayer())
		{
			Players.AddUnique(Player);
		}
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (const APlayerController* Controller = It->Get(); Controller && Controller->GetPawn())
		{
			Players.AddUnique(Controller->GetPawn());
		}
	}
	AActor* By = BroughtBy.Get();
	AController* ByController = By ? By->GetInstigatorController() : nullptr;
	if (const APawn* ByPawn = Cast<APawn>(By))
	{
		ByController = ByPawn->GetController();
	}
	bool bHurt = false;
	for (APawn* Player : Players)
	{
		const UHealthComponent* Health = Player ? Player->FindComponentByClass<UHealthComponent>() : nullptr;
		ACharacter* Body = Cast<ACharacter>(Player);
		if (!Health || Health->IsDead() || !Body)
		{
			continue;
		}
		const UCapsuleComponent* Capsule = Body->GetCapsuleComponent();
		const FVector Middle = Body->GetActorLocation();
		const float Feet = static_cast<float>(Middle.Z - Capsule->GetScaledCapsuleHalfHeight());
		const float Side = FMath::Max(static_cast<float>(FVector::Dist2D(Middle, Spot)) - Capsule->GetScaledCapsuleRadius(), 0.f);
		if (Side > Radius || FMath::Abs(Feet - static_cast<float>(Spot.Z)) > ReachRise)
		{
			continue;
		}
		const float Dealt = LooterCombat::RollHitDamage(Damage, false);
		UGameplayStatics::ApplyDamage(Player, Dealt, ByController, By, UCreatureAttackDamageType::StaticClass());
		FVector Away = Middle - Spot;
		Away.Z = 0.0;
		Away = Away.IsNearlyZero() ? FVector::ForwardVector : Away.GetSafeNormal();
		Body->LaunchCharacter(Away * KnockSpeed + FVector(0.0, 0.0, 160.0), true, false);
		UE_LOG(LogLooter, Log, TEXT("Rock from the roof hits %s for %.0f."), *GetNameSafe(Player), Dealt);
		bHurt = true;
	}
	return bHurt;
}

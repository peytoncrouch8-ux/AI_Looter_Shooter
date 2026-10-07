// AEggSac: an egg sac in the Sink: its parts, its story, being shot and its hit flash. EggSacFall.cpp is its fall, the
// burst and its spiders.

#include "World/EggSac.h"
#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/SpiderCreature.h"
#include "Missions/MissionRunner.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "Templates/UnrealTemplate.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

const FName AEggSac::EggSacTag(TEXT("EggSac"));
const FName AEggSac::BurstEvent(TEXT("EggSac.Burst"));

namespace
{
	/** Sink.py's models: a hanging sac for one placed without the build (a test, the console), and the hatched one. */
	const TCHAR* IntactPath = TEXT("/Game/Art/Props/SM_EggSac_A.SM_EggSac_A");
	const TCHAR* BurstPath = TEXT("/Game/Art/Props/SM_EggSac_Burst.SM_EggSac_Burst");
	const TCHAR* HitFlashPath = TEXT("/Game/Weapons/FX/M_FX_HitFlash.M_FX_HitFlash");

	/** A few shots from a gun of the area's band: it's a prop, not an enemy. */
	constexpr float SacHealth = 120.f;

	/** The hit flash: the dummy's (ATargetDummy), a little warmer on the silk. */
	constexpr float HitFlashDuration = 0.16f;
	constexpr float HitFlashGlow = 1.6f;
	const FLinearColor HitFlashColor(1.f, 0.93f, 0.82f);
	const FLinearColor CriticalFlashColor(1.f, 0.72f, 0.2f);

	/** An asset found only once it's in this checkout, so the class works, and its tests run, without it. In a constructor. */
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
}

AEggSac::AEggSac()
{
	// Ticks only while it falls.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	static UStaticMesh* const IntactModel = FindIfMade<UStaticMesh>(IntactPath);
	static UStaticMesh* const BurstModel = FindIfMade<UStaticMesh>(BurstPath);

	Sac = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sac"));
	Sac->SetupAttachment(Root);
	Sac->SetMobility(EComponentMobility::Movable);
	Sac->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	Sac->SetGenerateOverlapEvents(false);
	Sac->SetCanEverAffectNavigation(false);
	Sac->SetStaticMesh(IntactModel);

	Burst = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Burst"));
	Burst->SetupAttachment(Root);
	Burst->SetMobility(EComponentMobility::Movable);
	Burst->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	Burst->SetGenerateOverlapEvents(false);
	Burst->SetCanEverAffectNavigation(false);
	Burst->SetStaticMesh(BurstModel);
	// Not there until it lands.
	Burst->SetVisibility(false);
	SetSolid(*Burst, false);

	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	Health->MaxHealth = SacHealth;

	SpiderClass = ASpiderCreature::StaticClass();
	SpawnSockets = { FName(TEXT("Spawn_1")), FName(TEXT("Spawn_2")) };
	Tags.Add(EggSacTag);
}

void AEggSac::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Placed or edited: it shows hanging intact, as the level begins before Main 5.
	bRestCaptured = false;
	ShowHanging();
}

void AEggSac::BeginPlay()
{
	Super::BeginPlay();
	CaptureRest();
	Health->OnDamaged.AddDynamic(this, &AEggSac::HandleDamaged);
	Health->OnDeath.AddDynamic(this, &AEggSac::HandleDeath);
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		MissionsChangedHandle = Runner->OnChanged.AddUObject(this, &AEggSac::HandleMissionsChanged);
	}
	RefreshStory();
}

void AEggSac::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(MissionsChangedHandle);
	}
	BoundRunner.Reset();
	MissionsChangedHandle.Reset();
	GetWorldTimerManager().ClearTimer(HitFlashTimer);
	Super::EndPlay(EndPlayReason);
}

void AEggSac::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// The story
// ---------------------------------------------------------------------------

bool AEggSac::CanBeShot() const
{
	if (State != EEggSacState::Hanging)
	{
		return false;
	}
	if (bForceShot || ShootableWhen.IsEmpty())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && ShootableWhen.IsMet(Runner->GetCampaign(), Runner);
}

bool AEggSac::IsStoryDown() const
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	if (!Runner)
	{
		return false;
	}
	return DownWhen.ContainsByPredicate([Runner](const FStoryCondition& When)
	{
		return !When.IsEmpty() && When.IsMet(Runner->GetCampaign(), Runner);
	});
}

void AEggSac::RefreshStory()
{
	if (State == EEggSacState::Hanging && IsStoryDown())
	{
		// The story is past it: it was shot down long ago, and its spiders are long gone.
		Land(/*bHatch*/ false);
		return;
	}
	if (State == EEggSacState::Burst && !IsStoryDown() && !ShootableWhen.IsEmpty())
	{
		const UMissionRunner* Runner = UMissionRunner::Get(this);
		const bool bShootable = Runner && ShootableWhen.IsMet(Runner->GetCampaign(), Runner);
		if (!bShootable)
		{
			// The story went back before its step (the console starting Main 5 over): it hangs again, as it did then.
			Hang();
		}
	}
}

void AEggSac::HandleMissionsChanged()
{
	RefreshStory();
}

// ---------------------------------------------------------------------------
// Being shot
// ---------------------------------------------------------------------------

float AEggSac::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// Out of its step the shot only strikes the silk: no health lost, no number, no hit for the missions.
	if (!CanBeShot())
	{
		return 0.f;
	}
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

bool AEggSac::ShootDown(AController* By, bool bForce)
{
	if (State != EEggSacState::Hanging)
	{
		return false;
	}
	TGuardValue<bool> Forced(bForceShot, bForce);
	if (!CanBeShot())
	{
		return false;
	}
	// As a killing shot: through its health, so it's down for the missions' arrow too.
	TakeDamage(Health->GetHealth() + 1.f, FDamageEvent(UDamageType::StaticClass()), By, nullptr);
	if (State == EEggSacState::Hanging)
	{
		// Its health never heard it (a test level, before its play began): down all the same.
		ShotBy = By;
		StartFall();
	}
	return true;
}

void AEggSac::HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser)
{
	FlashHit(bCritical);
}

void AEggSac::HandleDeath(AController* Killer)
{
	if (State != EEggSacState::Hanging)
	{
		return;
	}
	ShotBy = Killer;
	UE_LOG(LogLooter, Log, TEXT("%s: shot down%s."), *GetActorNameOrLabel(),
		Killer ? *FString::Printf(TEXT(" by %s"), *Killer->GetName()) : TEXT(""));
	StartFall();
}

void AEggSac::Hang()
{
	SetActorTickEnabled(false);
	State = EEggSacState::Hanging;
	Fallen = 0.0;
	FallSpeed = 0.0;
	ShotBy.Reset();
	Health->ResetHealth();
	ShowHanging();
}

// ---------------------------------------------------------------------------
// Hanging
// ---------------------------------------------------------------------------

void AEggSac::SetSolid(UStaticMeshComponent& Mesh, bool bSolid)
{
	// Its profile (BlockAllDynamic) makes it solid to shots (the Weapon channel), the Interact key's line and pawns, as a
	// prop is; world dynamic, so the traces that look for the ground among world-static things (creatures, spawners, the
	// minimap) never find it.
	Mesh.SetCollisionEnabled(bSolid ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}

void AEggSac::CaptureRest()
{
	if (bRestCaptured)
	{
		return;
	}
	bRestCaptured = true;
	SacRest = Sac->GetRelativeTransform();
}

void AEggSac::ShowHanging()
{
	CaptureRest();
	Sac->SetRelativeTransform(SacRest);
	Sac->SetVisibility(true);
	SetSolid(*Sac, true);
	Burst->SetVisibility(false);
	SetSolid(*Burst, false);
	Burst->SetRelativeTransform(FTransform::Identity);
}

// ---------------------------------------------------------------------------
// The hit flash
// ---------------------------------------------------------------------------

void AEggSac::FlashHit(bool bCritical)
{
	if (!HitFlashMaterial)
	{
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, HitFlashPath);
		if (!Base)
		{
			return;
		}
		HitFlashMaterial = UMaterialInstanceDynamic::Create(Base, this);
	}
	HitFlashMaterial->SetVectorParameterValue(TEXT("Color"), bCritical ? CriticalFlashColor : HitFlashColor);
	HitFlashStrength = bCritical ? 1.f : 0.6f;
	HitFlashStart = GetWorld()->GetTimeSeconds();
	Sac->SetOverlayMaterial(HitFlashMaterial);
	UpdateHitFlash();
	GetWorldTimerManager().SetTimer(HitFlashTimer, this, &AEggSac::UpdateHitFlash, 1.f / 60.f, true);
}

void AEggSac::UpdateHitFlash()
{
	const float T = static_cast<float>(GetWorld()->GetTimeSeconds() - HitFlashStart) / HitFlashDuration;
	if (T >= 1.f || !HitFlashMaterial)
	{
		Sac->SetOverlayMaterial(nullptr);
		GetWorldTimerManager().ClearTimer(HitFlashTimer);
		return;
	}
	HitFlashMaterial->SetScalarParameterValue(TEXT("Intensity"), HitFlashGlow * HitFlashStrength * FMath::Square(1.f - T));
}

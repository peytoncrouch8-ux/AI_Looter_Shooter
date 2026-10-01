#include "Combat/TargetDummy.h"
#include "Combat/HealthComponent.h"
#include "Loot/LootDropComponent.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"

namespace
{
	constexpr float HitFlashDuration = 0.16f;
	constexpr float HitFlashGlow = 1.8f;
	const FLinearColor HitFlashColor(1.f, 0.93f, 0.85f);
	const FLinearColor CriticalFlashColor(1.f, 0.72f, 0.2f);
}

ATargetDummy::ATargetDummy()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	// Physics-asset collision per bone, blocking weapon traces so bone names (e.g. "head") come through.
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	Mesh->SetGenerateOverlapEvents(false);
	SetRootComponent(Mesh);

	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	Health->MaxHealth = 500.f;

	Loot = CreateDefaultSubobject<ULootDropComponent>(TEXT("Loot"));
}

bool ATargetDummy::IsCriticalSpot(const FHitResult& Hit) const
{
	return !CriticalBoneKeyword.IsEmpty() && !Hit.BoneName.IsNone()
		&& Hit.BoneName.ToString().Contains(CriticalBoneKeyword, ESearchCase::IgnoreCase);
}

void ATargetDummy::BeginPlay()
{
	Super::BeginPlay();

	if (IdleAnimation)
	{
		Mesh->PlayAnimation(IdleAnimation, /*bLooping*/ true);
	}

	Health->OnDamaged.AddDynamic(this, &ATargetDummy::HandleDamaged);
	Health->OnDeath.AddDynamic(this, &ATargetDummy::HandleDeath);
}

void ATargetDummy::HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser)
{
	// Restart the heal countdown on every hit.
	GetWorldTimerManager().SetTimer(HealTimer, this, &ATargetDummy::HealToFull, FMath::Max(HealDelay, 0.01f), false);
	FlashHit(bCritical);
}

void ATargetDummy::FlashHit(bool bCritical)
{
	if (!HitFlashMaterial)
	{
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Weapons/FX/M_FX_HitFlash.M_FX_HitFlash"));
		if (!Base)
		{
			return;
		}
		HitFlashMaterial = UMaterialInstanceDynamic::Create(Base, this);
	}
	HitFlashMaterial->SetVectorParameterValue(TEXT("Color"), bCritical ? CriticalFlashColor : HitFlashColor);
	HitFlashStrength = bCritical ? 1.f : 0.6f;
	HitFlashStart = GetWorld()->GetTimeSeconds();
	Mesh->SetOverlayMaterial(HitFlashMaterial);
	UpdateHitFlash();
	GetWorldTimerManager().SetTimer(HitFlashTimer, this, &ATargetDummy::UpdateHitFlash, 1.f / 60.f, true);
}

void ATargetDummy::UpdateHitFlash()
{
	const float T = static_cast<float>(GetWorld()->GetTimeSeconds() - HitFlashStart) / HitFlashDuration;
	if (T >= 1.f || !HitFlashMaterial)
	{
		Mesh->SetOverlayMaterial(nullptr);
		GetWorldTimerManager().ClearTimer(HitFlashTimer);
		return;
	}
	HitFlashMaterial->SetScalarParameterValue(TEXT("Intensity"), HitFlashGlow * HitFlashStrength * FMath::Square(1.f - T));
}

void ATargetDummy::HandleDeath(AController* Killer)
{
	// LootDropComponent drops loot on its own from the same death event. A dummy gives no experience, but the bestiary
	// counts it.
	UPlayerProgressionSubsystem::AwardKill(Killer, this);
	GetWorldTimerManager().ClearTimer(HealTimer);
	SetPresent(false);
	GetWorldTimerManager().SetTimer(RespawnTimer, this, &ATargetDummy::Respawn, FMath::Max(RespawnDelay, 0.01f), false);
}

void ATargetDummy::HealToFull()
{
	if (!Health->IsDead())
	{
		Health->ResetHealth();
	}
}

void ATargetDummy::Respawn()
{
	Health->ResetHealth();
	SetPresent(true);
}

void ATargetDummy::SetPresent(bool bPresent)
{
	SetActorHiddenInGame(!bPresent);
	SetActorEnableCollision(bPresent);
}

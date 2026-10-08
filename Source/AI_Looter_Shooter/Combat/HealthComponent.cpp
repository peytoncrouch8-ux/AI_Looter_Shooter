#include "Combat/HealthComponent.h"
#include "Combat/LooterDamageTypes.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "UI/World/DamageNumberActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	DamageNumberClass = ADamageNumberActor::StaticClass();
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakePointDamage.AddDynamic(this, &UHealthComponent::HandlePointDamage);
		Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::HandleAnyDamage);
	}
}

void UHealthComponent::HandlePointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,
	UPrimitiveComponent* HitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser)
{
	bHasPendingHitLocation = true;
	PendingHitLocation = HitLocation;
}

void UHealthComponent::HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	const FVector HitLocation = bHasPendingHitLocation ? PendingHitLocation : GetOwner()->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	bHasPendingHitLocation = false;

	if (bDead || Damage <= 0.f)
	{
		return;
	}

	const bool bCritical = DamageType && DamageType->IsA<UWeaponCritDamageType>();
	LastDamageCauser = DamageCauser;
	// Hurting something counts as meeting it (the bestiary's unknown pages); only the player's hits count.
	UPlayerProgressionSubsystem::RecordEncounter(InstigatedBy, GetOwner());

	if (!bInvulnerable)
	{
		Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	}

	SpawnDamageNumber(Damage, bCritical, HitLocation, InstigatedBy);
	OnDamaged.Broadcast(Damage, bCritical, HitLocation, InstigatedBy, DamageCauser);
	OnHealthChanged.Broadcast(Health, MaxHealth);

	if (Health <= 0.f)
	{
		bDead = true;
		OnDeath.Broadcast(InstigatedBy);
	}
}

void UHealthComponent::ResetHealth()
{
	bDead = false;
	Health = MaxHealth;
	LastDamageCauser.Reset();
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void UHealthComponent::SetHealth(float NewHealth)
{
	bDead = false;
	Health = FMath::Clamp(NewHealth, FMath::Min(1.f, MaxHealth), MaxHealth);
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void UHealthComponent::SetMaxHealth(float NewMaxHealth)
{
	const float OldMaxHealth = MaxHealth;
	MaxHealth = FMath::Max(NewMaxHealth * MaxHealthScale, 1.f);
	if (!HasBegunPlay() || bDead || FMath::IsNearlyEqual(MaxHealth, OldMaxHealth))
	{
		return;
	}
	// A level-up's extra health comes with it, so the wound stays the same size; a smaller maximum takes as much away.
	Health = FMath::Clamp(Health + (MaxHealth - OldMaxHealth), FMath::Min(1.f, MaxHealth), MaxHealth);
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void UHealthComponent::SetMaxHealthScale(float Scale)
{
	const float NewScale = FMath::Max(Scale, 0.01f);
	if (FMath::IsNearlyEqual(NewScale, MaxHealthScale))
	{
		return;
	}
	const float OldMaxHealth = MaxHealth;
	MaxHealth = FMath::Max(MaxHealth / MaxHealthScale * NewScale, 1.f);
	MaxHealthScale = NewScale;
	if (!HasBegunPlay() || bDead)
	{
		return;
	}
	// The same share of the new maximum: a gun swapped in and out again leaves health where it was, and a share of a
	// living one's health is never none.
	Health = FMath::Min(Health * MaxHealth / FMath::Max(OldMaxHealth, 1.f), MaxHealth);
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

float UHealthComponent::Heal(float Amount)
{
	if (bDead || Amount <= 0.f || Health >= MaxHealth)
	{
		return 0.f;
	}
	const float Healed = FMath::Min(Amount, MaxHealth - Health);
	Health += Healed;
	OnHealthChanged.Broadcast(Health, MaxHealth);
	return Healed;
}

float UHealthComponent::Drain(float Amount)
{
	if (bDead || bInvulnerable || Amount <= 0.f)
	{
		return 0.f;
	}
	const float Taken = FMath::Clamp(Amount, 0.f, FMath::Max(Health - 1.f, 0.f));
	if (Taken <= 0.f)
	{
		return 0.f;
	}
	Health -= Taken;
	OnHealthChanged.Broadcast(Health, MaxHealth);
	return Taken;
}

void UHealthComponent::SpawnDamageNumber(float Damage, bool bCritical, const FVector& Location, AController* InstigatedBy) const
{
	if (!bShowDamageNumbers || !DamageNumberClass)
	{
		return;
	}

	// Only the player who dealt the damage sees the number.
	const APlayerController* PC = Cast<APlayerController>(InstigatedBy);
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ADamageNumberActor* Number = GetWorld()->SpawnActor<ADamageNumberActor>(DamageNumberClass, Location, FRotator::ZeroRotator, Params))
	{
		Number->Show(Damage, bCritical);
	}
}

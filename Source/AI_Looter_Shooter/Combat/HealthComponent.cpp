#include "Combat/HealthComponent.h"
#include "Combat/LooterDamageTypes.h"
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

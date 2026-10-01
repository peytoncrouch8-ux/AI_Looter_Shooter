#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponRack.generated.h"

class AAmmoPickup;
class AWeaponBase;
class UStaticMeshComponent;
class UWeaponDefinition;

/**
 * A table or rack with a weapon lying on it as ordinary loot (look at it and press the interact key), and a few boxes
 * of its ammo beside it. On the tutorial island it hands the player their first gun. When the weapon has been taken and
 * the player no longer carries one (dropped, lost), a fresh one appears after a moment, so the tutorial can't run out.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AWeaponRack : public AActor
{
	GENERATED_BODY()

public:
	AWeaponRack();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Rack")
	TObjectPtr<UStaticMeshComponent> Rack;

	/** The weapon it offers, rolled fresh each time. */
	UPROPERTY(EditAnywhere, Category = "Weapon Rack")
	TObjectPtr<UWeaponDefinition> Weapon;

	UPROPERTY(EditAnywhere, Category = "Weapon Rack")
	EWeaponRarity Rarity = EWeaponRarity::Common;

	UPROPERTY(EditAnywhere, Category = "Weapon Rack", meta = (ClampMin = "1"))
	int32 Level = 1;

	/** Magazines' worth of ammo in the boxes beside it. */
	UPROPERTY(EditAnywhere, Category = "Weapon Rack", meta = (ClampMin = "0"))
	int32 AmmoMagazines = 4;

	/** The rack mesh's socket the weapon lies on. */
	UPROPERTY(EditAnywhere, Category = "Weapon Rack")
	FName WeaponSocket = TEXT("Weapon");

	/** Seconds after the weapon is gone (and the player has none) before another appears. */
	UPROPERTY(EditAnywhere, Category = "Weapon Rack", meta = (ClampMin = "1"))
	float RestockSeconds = 15.f;

	/** The weapon lying on the rack now, if any. */
	AWeaponBase* GetOfferedWeapon() const { return Offered.Get(); }

private:
	void Restock();
	bool PlayerHasWeapon() const;

	TWeakObjectPtr<AWeaponBase> Offered;
	TArray<TWeakObjectPtr<AAmmoPickup>> AmmoBoxes;
	float RestockTimer = 0.f;
};

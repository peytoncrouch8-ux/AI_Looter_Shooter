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
 * A table or rack with a weapon lying on it as ordinary loot (look at it and press the interact key), and its ammo
 * lying beside it. On the tutorial island it hands the player their first gun. When the weapon has been taken and
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

	/** Magazines' worth of ammo in the pickups beside it. */
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

	// --- For saved sessions (USessionSubsystem) ---

	/** Its weapon is still lying on it. */
	bool IsWeaponOffered() const;

	/** How many of the ammo pickups it laid out are left. */
	int32 GetAmmoPickupsLeft() const;

	/** This loot is the rack's own: its weapon, or one of its ammo pickups. */
	bool Offers(const AActor* Loot) const;

	/**
	 * Takes back what the player had already taken when the session was saved: the weapon unless bWeaponOffered, and
	 * the ammo pickups beyond AmmoPickupsLeft. The rack stocked itself as the level began.
	 */
	void RestoreOffer(bool bWeaponOffered, int32 AmmoPickupsLeft);

private:
	void Restock();
	bool PlayerHasWeapon() const;

	TWeakObjectPtr<AWeaponBase> Offered;
	TArray<TWeakObjectPtr<AAmmoPickup>> AmmoPickups;
	float RestockTimer = 0.f;
};

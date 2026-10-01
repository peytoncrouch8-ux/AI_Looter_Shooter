#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHUDWidget.generated.h"

class AWeaponBase;
class UHealthComponent;
class UHudWeaponSlotsWidget;
class UImage;
class USizeBox;
class UTextBlock;
class UWidget;
class UWeaponManagerComponent;

/**
 * In-game HUD in the shared UI style, built to stay out of the way while playing:
 *  - bottom-left: a health cross, the number and a slim slanted segmented bar (with a trailing "damage chip")
 *  - bottom-right: the weapon slots as hexagons (UHudWeaponSlotsWidget) over the ammo: the magazine count with its
 *    ammo class and reserve, one tick per round (doubling as reload progress), and the fire mode and gun's name under it
 *  - top-right: the minimap (UHudMinimapWidget)
 *  - top-left: the frame rate (UHudFrameRateWidget)
 *  - bottom-right, over the ammo: the ammo pickup feed (UHudPickupFeedWidget)
 *  - bottom-center: the level and experience bar (UHudXPBarWidget), the only place the level shows
 *  - center: thin tick crosshair sized by the weapon's spread, diagonal hit marker
 * No backing panels; both corner clusters fade back when nothing is happening and come forward on
 * activity (firing, reloading, switching, taking damage). The loot comparison card and messages only
 * appear when relevant. Reads the possessed pawn every frame, so it survives respawns.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BindToPawn(UWeaponManagerComponent* Manager);
	void UpdateWeaponCluster(UWeaponManagerComponent* Manager, float DeltaTime);
	void UpdateVitals(UHealthComponent* Health, float DeltaTime);
	void UpdateCrosshair(const AWeaponBase* Active, float DeltaTime);
	void UpdatePickupCard(UWeaponManagerComponent* Manager);
	FString BoundKeyName(FName BindingId, const TCHAR* Fallback) const;

	UFUNCTION()
	void HandleHit(const FHitResult& Hit, float Damage, bool bCritical);

	UFUNCTION()
	void HandleReloadStarted(float Duration);

	UFUNCTION()
	void HandleMessage(const FText& Message);

	// Crosshair and hit marker
	UPROPERTY(Transient) TObjectPtr<USizeBox> CrosshairBox;
	UPROPERTY(Transient) TObjectPtr<UWidget> HitMarker;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> HitMarkerTicks;

	// Bottom-left: vitals
	UPROPERTY(Transient) TObjectPtr<UWidget> VitalsCluster;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HealthValue;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HealthMax;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> HealthSegments;

	// Bottom-right: weapon
	UPROPERTY(Transient) TObjectPtr<UWidget> WeaponCluster;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> AmmoText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ReserveText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> AmmoSegments;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> WeaponName;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> AmmoClassText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> FireModeText;
	UPROPERTY(Transient) TObjectPtr<UHudWeaponSlotsWidget> WeaponSlots;

	// Loot card and messages
	UPROPERTY(Transient) TObjectPtr<UWidget> PickupCard;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PickupName;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PickupLevel;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> PickupStatTexts;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PickupHint;
	UPROPERTY(Transient) TObjectPtr<UWidget> PickupHoldBar;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> PickupHoldSegments;
	UPROPERTY(Transient) TObjectPtr<UWidget> MessagePlate;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MessageText;

	TWeakObjectPtr<UWeaponManagerComponent> BoundManager;
	TWeakObjectPtr<AWeaponBase> BoundWeapon;

	// Health display state
	float ShownHealthFraction = -1.f;  // what the bar shows (snaps down on damage)
	float GhostHealthFraction = -1.f;  // trailing chip that drains after a hit
	float GhostHoldTime = 0.f;
	float VitalsActivity = 0.f;

	// Weapon display state
	int32 LastMagazine = INDEX_NONE;
	int32 LastReserve = INDEX_NONE;
	/** How many ticks the ammo strip shows (one per round, up to MaxAmmoTicks). */
	int32 ShownTickCount = INDEX_NONE;
	float WeaponActivity = 0.f;
	float ReloadDuration = 0.f;
	float ReloadElapsed = 0.f;
	float CrosshairSize = 0.f;
	float PulseTime = 0.f;

	float HitMarkerTime = 0.f;
	float MessageTime = 0.f;
};

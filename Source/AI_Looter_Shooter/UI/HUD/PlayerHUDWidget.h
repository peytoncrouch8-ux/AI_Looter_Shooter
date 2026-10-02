#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHUDWidget.generated.h"

class AWeaponBase;
enum class EAmmoType : uint8;
class UHealthComponent;
class UHudInteractPromptWidget;
class UHudMagazineWidget;
class UHudVitalsWidget;
class UHudWeaponSlotsWidget;
class UImage;
class UInteractionComponent;
class USizeBox;
class UTextBlock;
class UWidget;
class UWeaponManagerComponent;

/**
 * In-game HUD in the shared UI style, built to stay out of the way while playing:
 *  - bottom-left: health as a ring with the number inside and a solid bar running out of its lower side, with a
 *    trailing "damage chip" (UHudVitalsWidget)
 *  - bottom-right: the weapon slots as circles (UHudWeaponSlotsWidget) over the ammo: its status and ammo icon, the
 *    magazine as a cartridge that drains as the gun fires and fills with reload progress (UHudMagazineWidget), the
 *    reserve, and the fire mode and gun's name under it
 *  - top-right: the minimap (UHudMinimapWidget)
 *  - top-left: the frame rate (UHudFrameRateWidget)
 *  - left of the crosshair: the ammo pickup feed (UHudPickupFeedWidget)
 *  - bottom-center: the level and experience bar (UHudXPBarWidget), the only place the level shows
 *  - center: thin tick crosshair sized by the weapon's spread (it fades out while aiming through a sight in first
 *    person, where the sight's reticle is the aim point), diagonal hit marker
 *  - under the crosshair: what the Interact key does to the thing looked at (UHudInteractPromptWidget), for everything
 *    but loot, which has the comparison card
 * No backing panels; both corner clusters fade back when nothing is happening and come forward on
 * activity (firing, reloading, switching, taking damage). The loot comparison card and messages only
 * appear when relevant. Reads the possessed pawn every frame, so it survives respawns.
 *
 * PlayerHUDWidget.cpp builds it and runs the corners and crosshair; PlayerHUDWidgetPickupCard.cpp fills the loot card
 * and the interaction prompt from the player's interaction component.
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

	/** The loot card when the player looks at loot, the interaction prompt for anything else they could use. */
	void UpdatePickupCard(UWeaponManagerComponent* Manager, const UInteractionComponent* Interaction, float DeltaTime);
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
	UPROPERTY(Transient) TObjectPtr<UHudVitalsWidget> Vitals;

	// Bottom-right: weapon
	UPROPERTY(Transient) TObjectPtr<UWidget> WeaponCluster;
	UPROPERTY(Transient) TObjectPtr<UHudMagazineWidget> MagazineGauge;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ReserveText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> WeaponName;
	/** The ammo the gun in hand takes, as its Inked icon. */
	UPROPERTY(Transient) TObjectPtr<UImage> AmmoClassIcon;
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

	// The interaction prompt under the crosshair
	UPROPERTY(Transient) TObjectPtr<UHudInteractPromptWidget> InteractPrompt;

	TWeakObjectPtr<UWeaponManagerComponent> BoundManager;
	TWeakObjectPtr<AWeaponBase> BoundWeapon;

	// Weapon display state
	int32 LastMagazine = INDEX_NONE;
	int32 LastReserve = INDEX_NONE;
	/** The ammo icon drawn now, so it is only set again when the gun in hand takes another ammo. */
	TOptional<EAmmoType> ShownAmmoType;
	float WeaponActivity = 0.f;
	float ReloadDuration = 0.f;
	float ReloadElapsed = 0.f;
	float CrosshairSize = 0.f;
	float PulseTime = 0.f;

	float HitMarkerTime = 0.f;
	float MessageTime = 0.f;
};

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "PlayerHUDWidget.generated.h"

class APawn;
class AWeaponBase;
class UHealthComponent;
class UImage;
class USizeBox;
class UTextBlock;
class UTexture2D;
class UWidget;
class UWeaponManagerComponent;

/**
 * In-game HUD in the shared UI style, built to stay out of the way while playing:
 *  - bottom-left: health number + slim slanted segmented bar (with a trailing "damage chip")
 *  - bottom-right: magazine / reserve, slanted magazine bar (doubles as reload progress), weapon name
 *    underneath in its rarity color, and slot pips
 *  - top-right: round minimap that turns with the view (forward is up): the island from above, hostiles as
 *    red diamonds, loot as dots in its rarity color, ammo as small dots, and an N that circles the rim. Its size is the
 *    player's choice (Settings > Interface > Minimap size, 60-150%)
 *  - center: thin tick crosshair sized by the weapon's spread, diagonal hit marker
 * No backing panels; both corner clusters fade back when nothing is happening and come forward on
 * activity (firing, reloading, switching, taking damage). The loot comparison card and messages only
 * appear when relevant. Reads the possessed pawn every frame, so it survives respawns.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * The minimap's standard size on screen, and its gap from the screen's top-right corner. Its actual size follows the
	 * player's setting (Settings > Interface > Minimap size), and the settings menu previews it in the same spot.
	 */
	static constexpr float MinimapDiameter = 172.f;
	static constexpr float MinimapMargin = 36.f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BindToPawn(UWeaponManagerComponent* Manager);
	void UpdateWeaponCluster(UWeaponManagerComponent* Manager, float DeltaTime);
	void UpdateVitals(UHealthComponent* Health, float DeltaTime);
	void UpdateCrosshair(const AWeaponBase* Active, float DeltaTime);
	void UpdatePickupCard(UWeaponManagerComponent* Manager);
	void UpdateMinimap(const APawn* Pawn);
	/** Resizes the minimap: the map, its rim and the player arrow grow with it, markers a little less. */
	void ApplyMinimapScale(float Scale);
	float GetMinimapDiameter() const { return MinimapDiameter * MinimapScale; }
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
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> SlotPips;

	// Loot card and messages
	UPROPERTY(Transient) TObjectPtr<UWidget> PickupCard;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PickupName;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PickupLevel;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> PickupStatTexts;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PickupHint;
	UPROPERTY(Transient) TObjectPtr<UWidget> MessagePlate;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MessageText;

	// Top-right: minimap
	UPROPERTY(Transient) TObjectPtr<UWidget> MinimapCluster;
	UPROPERTY(Transient) TObjectPtr<USizeBox> MinimapSize;
	UPROPERTY(Transient) TObjectPtr<UImage> MinimapMap;
	UPROPERTY(Transient) TObjectPtr<UImage> MinimapArrow;
	UPROPERTY(Transient) TObjectPtr<UImage> MinimapNotch;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MinimapNorth;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> MinimapMarkers;
	/** The map picture the brush currently shows (owned by UMinimapSubsystem). */
	UPROPERTY(Transient) TObjectPtr<UTexture2D> MinimapTexture;
	FSlateBrush MinimapBrush;
	/** Per marker: what it last showed (kind + color), so brushes only change when needed. */
	TArray<uint32> MinimapMarkerKeys;
	/** The minimap's size right now, relative to MinimapDiameter. */
	float MinimapScale = 1.f;

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
	float WeaponActivity = 0.f;
	float ReloadDuration = 0.f;
	float ReloadElapsed = 0.f;
	float CrosshairSize = 0.f;
	float PulseTime = 0.f;

	float HitMarkerTime = 0.f;
	float MessageTime = 0.f;
};

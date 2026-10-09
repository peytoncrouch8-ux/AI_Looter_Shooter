#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHUDWidget.generated.h"

class AWeaponBase;
class UHealthComponent;
class UHudGrenadeWidget;
class UHudInteractPromptWidget;
class UHudPickupFeedWidget;
class UHudLevelUpBannerWidget;
class UHudMagazineWidget;
class UHudMissionCompleteWidget;
class UHudPlayerFrameWidget;
class UHudWeaponSlotsWidget;
class UImage;
class UInteractionComponent;
class UPlayerMeleeComponent;
class UPlayerThrowComponent;
class USizeBox;
class UTextBlock;
class UWidget;
class UWeaponManagerComponent;
enum class EGrenadeChange : uint8;

/**
 * In-game HUD in the shared UI style, built to stay out of the way while playing:
 *  - bottom-left: the player frame (UHudPlayerFrameWidget): the portrait in its gunmetal medallion, the health bar, the
 *    level gem and the experience bar
 *  - bottom-right, 48 px in from the edge: the grave-salt grenades carried over the column (UHudGrenadeWidget: key tab,
 *    the tin's icon, the count and its pips, once the player has grenades), then
 *    the weapon slots in a column, slot 1 on top, each with its key tab and ammo
 *    icon on its left (UHudWeaponSlotsWidget), and on their right the magazine as a cartridge standing tip up, which
 *    drains from the tip as the gun fires, fills with reload progress and holds the rounds and reserve by its base
 *    (UHudMagazineWidget); under them, right-aligned, the status ("RELOADING") and fire mode on one line, and the gun's
 *    name in its rarity's colour ending in a small rarity gem
 *  - top-right: the minimap (UHudMinimapWidget)
 *  - top-left: the frame rate (UHudFrameRateWidget)
 *  - top-centre, its gem 260 px down: the level-up banner (UHudLevelUpBannerWidget), when the frame's experience bar levels up;
 *    in the same place, taking turns with it, the mission-complete banner (UHudMissionCompleteWidget) as a mission is turned in
 *  - left of the crosshair: the ammo pickup feed (UHudPickupFeedWidget)
 *  - bottom-centre: nothing, on purpose
 *  - centre: thin tick crosshair sized by the weapon's spread, kicking out on every shot (it fades out while aiming
 *    through a sight in first person, where the sight's reticle is the aim point), diagonal hit marker (orange on a
 *    crit; on a kill red, bigger and held longer), and round it the red arcs pointing to where hits came from
 *    (UHudDamageIndicatorWidget)
 *  - under the crosshair: what the Interact key does to the thing looked at (UHudInteractPromptWidget), for everything
 *    but loot, which has the comparison card (right-middle); lower, the message plate (the weapon manager's messages)
 *  - behind everything: the screen edges' red flash on a hit and pulse at low health (UHudScreenEdgeWidget)
 * No backing panels; both corner clusters fade back when nothing is happening and come forward on activity (firing,
 * reloading, switching, taking damage). The loot comparison card and messages only appear when relevant. Reads the
 * possessed pawn every frame, so it survives respawns.
 *
 * PlayerHUDWidgetLayout.cpp builds it; PlayerHUDWidget.cpp runs the corners and crosshair; PlayerHUDWidgetPickupCard.cpp
 * fills the loot card and the interaction prompt from the player's interaction component.
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
	/** The status line beside the fire mode: reloading, a prompt when dry, or nothing. */
	void UpdateWeaponStatus(bool bReloading, int32 Magazine, int32 Reserve, float Pulse);
	void UpdatePlayerFrame(UHealthComponent* Health, float DeltaTime);
	void UpdateCrosshair(const AWeaponBase* Active, float DeltaTime);

	/** The loot card when the player looks at loot, the interaction prompt for anything else they could use. */
	void UpdatePickupCard(UWeaponManagerComponent* Manager, const UInteractionComponent* Interaction, float DeltaTime);
	FString BoundKeyName(FName BindingId, const TCHAR* Fallback) const;

	UFUNCTION()
	void HandleHit(const FHitResult& Hit, float Damage, bool bCritical);

	/** A melee strike landed: the hit marker for a fist (a gun's strike reports through the gun's OnHit). */
	UFUNCTION()
	void HandleMeleeHit(const FHitResult& Hit, float Damage, bool bCritical);

	/** A grenade's burst hurt a body: the hit marker (one sound for a burst's many hits). */
	UFUNCTION()
	void HandleGrenadeHit(const FHitResult& Hit, float Damage, bool bCritical);

	/** The grenades carried changed (or the key was pressed with none): the counter flashes, the feed tells of a find. */
	void HandleGrenadesChanged(int32 Count, int32 Delta, EGrenadeChange Why);

	/** Follows the pawn's throw component: its hits and its count. */
	void BindThrow(UPlayerThrowComponent* Throw);

	/** The grenade counter, frame by frame. */
	void UpdateGrenades(float DeltaTime);

	UFUNCTION()
	void HandleFired();

	UFUNCTION()
	void HandleReloadStarted(float Duration);

	UFUNCTION()
	void HandleMessage(const FText& Message);

	/** The player frame's experience bar reached a new level: the banner shows it. */
	void HandleLevelUp(int32 NewLevel);

	// Crosshair and hit marker
	UPROPERTY(Transient) TObjectPtr<USizeBox> CrosshairBox;
	UPROPERTY(Transient) TObjectPtr<UWidget> HitMarker;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> HitMarkerTicks;

	// Bottom-left: the player frame; top-centre: the level-up banner
	UPROPERTY(Transient) TObjectPtr<UHudPlayerFrameWidget> PlayerFrame;
	UPROPERTY(Transient) TObjectPtr<UHudLevelUpBannerWidget> LevelUpBanner;
	/** The mission-complete banner, in the level-up banner's place: a level-up during it waits for it. */
	UPROPERTY(Transient) TObjectPtr<UHudMissionCompleteWidget> MissionBanner;

	// Bottom-right: weapon
	UPROPERTY(Transient) TObjectPtr<UWidget> WeaponCluster;
	/** The grenades carried, over the weapon slots. */
	UPROPERTY(Transient) TObjectPtr<UHudGrenadeWidget> GrenadeCounter;
	/** Left of the crosshair: what was just picked up (a grenade found says so here too). */
	UPROPERTY(Transient) TObjectPtr<UHudPickupFeedWidget> PickupFeed;
	UPROPERTY(Transient) TObjectPtr<UHudWeaponSlotsWidget> WeaponSlots;
	UPROPERTY(Transient) TObjectPtr<UHudMagazineWidget> MagazineGauge;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> FireModeText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> WeaponName;
	/** The gun's name's type size, and the widest it may be: a longer name is set smaller to fit (FitWeaponName). */
	static constexpr int32 NameFontSize = 18;
	static constexpr float NameMaxWidth = 300.f;
	/** Sets the gun's name, smaller when it's too long, so it always ends at the cluster's right edge. */
	void FitWeaponName(const FString& Name);
	/** The small diamond after the gun's name, in its rarity's colour. */
	UPROPERTY(Transient) TObjectPtr<UImage> RarityGem;

	// Loot card and messages
	UPROPERTY(Transient) TObjectPtr<UWidget> PickupCard;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PickupName;
	/** A named gun's flavor line under its name; collapsed for any other gun. */
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PickupFlavor;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PickupLevel;
	/** The gun's notches and curse (LoadoutParts::MakeGunIdeasRows) under its level line; collapsed for a gun with neither. */
	UPROPERTY(Transient) TObjectPtr<USizeBox> PickupIdeas;
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
	TWeakObjectPtr<UPlayerMeleeComponent> BoundMelee;
	TWeakObjectPtr<UPlayerThrowComponent> BoundThrow;
	FDelegateHandle GrenadesChangedHandle;
	/** The frame the last grenade hit came in: a burst's other hits that frame show the marker without its sound again. */
	uint64 GrenadeHitFrame = 0;
	bool bMuteHitSound = false;

	/** What the loot card's notch and curse rows were built for: rebuilt only when another gun is looked at or a count in it changes. */
	TWeakObjectPtr<const AWeaponBase> IdeasPickup;
	int32 IdeasKills = 0;
	FName IdeasCurse;
	bool bIdeasLifted = false;

	// Weapon display state
	int32 LastMagazine = INDEX_NONE;
	int32 LastReserve = INDEX_NONE;
	/** The gun in hand's name, colour and fire mode need setting again (another gun came into hand). */
	bool bWeaponTextStale = true;
	/** The status line's state as last shown (EWeaponStatus in PlayerHUDWidget.cpp), so its words are only set on change. */
	uint8 ShownStatus = MAX_uint8;
	float WeaponActivity = 0.f;
	float ReloadDuration = 0.f;
	float ReloadElapsed = 0.f;
	float CrosshairSize = 0.f;
	/** Seconds since the last shot, while the crosshair's kick settles. */
	float CrosshairKickAge = 0.f;
	bool bCrosshairKicking = false;
	float PulseTime = 0.f;

	float HitMarkerTime = 0.f;
	/** The marker shows a kill (red, bigger, longer) until it fades. */
	bool bKillMarker = false;
	static constexpr float HitMarkerSeconds = 0.18f;
	static constexpr float KillMarkerSeconds = 0.34f;
	static constexpr float KillMarkerPop = 0.75f;
	float MessageTime = 0.f;
};

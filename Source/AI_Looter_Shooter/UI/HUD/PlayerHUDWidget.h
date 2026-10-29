#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/HudHitMarker.h"
#include "Weapons/WeaponTypes.h"
#include "PlayerHUDWidget.generated.h"

class AWeaponBase;
class UEmberComponent;
class UHealthComponent;
class UHudCrosshairWidget;
class UHudEmberSocketWidget;
class UHudFindBannerWidget;
class UHudGraveMarkedWidget;
class UHudGrenadeWidget;
class UHudInteractPromptWidget;
class UHudPickupFeedWidget;
class UHudLevelUpBannerWidget;
class UHudMagazineWidget;
class UHudMissionCompleteWidget;
class UHudNotchTallyWidget;
class UHudPlayerFrameWidget;
class UHudWeaponSlotsWidget;
class UGunCardWidget;
class UImage;
class UInteractionComponent;
class UPlayerMeleeComponent;
class UPlayerThrowComponent;
class USizeBox;
class UTextBlock;
class UWidget;
class UWeaponManagerComponent;
struct FFindBannerShow;
enum class EEmberDenied : uint8;
enum class EEmberState : uint8;
enum class EGrenadeChange : uint8;

/**
 * In-game HUD in the shared UI style, built to stay out of the way while playing:
 *  - bottom-left: the player frame (UHudPlayerFrameWidget): the portrait in its gunmetal medallion, the health bar, the
 *    level gem and the experience bar
 *  - bottom-right, 48 px in from the edge: at the column's top the grave-salt grenades carried (UHudGrenadeWidget: key tab,
 *    the tin's icon, the count and its pips, once the player has grenades; no silver readout: the user's call, 2026-10-27,
 *    the purse shows at the vendor's counter), then
 *    the weapon slots in a column, slot 1 on top, each with its key tab and ammo
 *    icon on its left (UHudWeaponSlotsWidget), left of them the equipped ember power's socket, bottom-aligned
 *    (UHudEmberSocketWidget: the Ember key's tab, its icon, its cooldown and run), and on their right the magazine as a cartridge standing tip up, which
 *    drains from the tip as the gun fires, fills with reload progress and holds the rounds and reserve by its base
 *    (UHudMagazineWidget); under them, right-aligned, the status ("RELOADING") and fire mode on one line, and the gun's
 *    name in its rarity's colour after a small rarity gem, with the gun's notches before the gem once it has any (a tally
 *    mark and the count, popping on each kill with it: UHudNotchTallyWidget)
 *  - top-right: the minimap (UHudMinimapWidget); under it the mission tracker (UHudMissionTrackerWidget, a viewport widget
 *    of its own that ALooterHUD makes, which steps aside while the in-hand card would cover it: GetLootCardBounds)
 *  - top-left: the frame rate (UHudFrameRateWidget)
 *  - top-centre, its gem 260 px down: the level-up banner (UHudLevelUpBannerWidget), when the frame's experience bar levels up;
 *    in the same place, taking turns with it, the mission-complete banner (UHudMissionCompleteWidget) as a mission is turned in;
 *    a little higher, the "GRAVE MARKED" banner (UHudGraveMarkedWidget) when the player walks up to a respawn grave for the first time
 *  - left of the crosshair: the ammo pickup feed (UHudPickupFeedWidget)
 *  - bottom-centre: nothing, on purpose
 *  - centre: the gun kind's crosshair (UHudCrosshairWidget, HudCrosshairRules: the user's pick, 2026-10-29; dot and ticks
 *    for the SMG, the classic cross for the rifle, brackets for the LMG, a chevron for the revolver, a broken ring for the
 *    shotgun, long hairs for the sniper, a diamond and drop marks for the launcher, the old four ticks with no gun), in the
 *    colour picked under Settings > Interface over a dark edge, opened by the weapon's spread, kicking out on every shot by
 *    its kind's amount and growing in when another kind comes into hand, round its centre dot (the lines
 *    fade out while aiming down the sights, with any gun in any view, and come back as the gun comes down: the user's
 *    call, 2026-10-14; the dot goes too in first person, where the sight's reticle is the aim point, and stays as the aim
 *    point in third person, where no sight is seen), diagonal hit marker (HudHitMarker: orange on a crit, cyan when a
 *    creature's ward took it or broke, red on a kill; sized by the hit's share of its health, a blast's pellets one
 *    marker; a kill and a break bigger and held longer), and round it the red arcs pointing to where hits came from (UHudDamageIndicatorWidget)
 *  - under the crosshair: what the Interact key does to the thing looked at (UHudInteractPromptWidget), for everything
 *    but loot, whose card stands over the gun (its label, UWeaponLabelWidget); lower, the message plate (the weapon
 *    manager's messages)
 *  - right-middle, while loot is looked at with a gun in hand: "IN HAND" over that gun's card (UGunCardWidget, InHand)
 *  - behind everything: the screen edges' red flash on a hit and pulse at low health (UHudScreenEdgeWidget)
 * No backing panels; both corner clusters fade back when nothing is happening and come forward on activity (firing,
 * reloading, switching, taking damage). The in-hand card and messages only appear when relevant. Reads the
 * possessed pawn every frame, so it survives respawns.
 *
 * PlayerHUDWidgetLayout.cpp builds it; PlayerHUDWidget.cpp runs the corners and crosshair; PlayerHUDWidgetPickupCard.cpp
 * fills the in-hand card and the interaction prompt from the player's interaction component.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Shows the "GRAVE MARKED" banner over GraveName (ALooterHUD::ShowGraveMarked): queued behind a banner showing. */
	void ShowGraveMarked(const FText& GraveName);

	/** The grave banner, for the tests; null before the HUD is built. */
	UHudGraveMarkedWidget* GetGraveBanner() const { return GraveBanner; }

	/** Shows the "FOUND" banner for a find (ALooterHUD::ShowFind): queued behind a banner showing. */
	void ShowFind(const FFindBannerShow& Find);

	/** The find banner, for the tests; null before the HUD is built. */
	UHudFindBannerWidget* GetFindBanner() const { return FindBanner; }

	/** The notches beside the gun's name, for the tests; null before the HUD is built. */
	UHudNotchTallyWidget* GetNotchTally() const { return NotchTally; }

	/** The gun's name, for the tests; null before the HUD is built. */
	UTextBlock* GetWeaponNameText() const { return WeaponName; }

	/** The ember socket and the weapon slots beside it, for the tests; null before the HUD is built. */
	UHudEmberSocketWidget* GetEmberSocket() const { return EmberSocket; }
	UHudWeaponSlotsWidget* GetWeaponSlots() const { return WeaponSlots; }

	/**
	 * The in-hand card's rectangle on the desktop (absolute pixels, as Slate lays it out) while it's up; false while it's
	 * away or not laid out yet. The mission tracker steps aside where it would overlap.
	 */
	bool GetLootCardBounds(FVector2D& OutMin, FVector2D& OutMax) const;

	/**
	 * How far down (HUD pixels) the in-hand card moves from the right edge's middle, where it stands, so a tall card keeps
	 * its top at ClearTop: under the minimap's frame and the place's name (the tracker's top, UHudMissionTrackerWidget::TopFor),
	 * but its bottom never goes past BottomKeep above the screen's foot (a card too tall for the room between stops there).
	 * 0 for a card that fits, or before anything is laid out.
	 */
	static float LootCardDrop(float ScreenHeight, float CardHeight, float ClearTop, float BottomKeep = 36.f);

	/**
	 * The in-hand card (the user's call, 2026-10-28): while loot is looked at with a gun in hand, that gun's card
	 * (UGunCardWidget, InHand) under "IN HAND" on the right edge, to hold against the looked-at gun's card over it (its
	 * label's, UWeaponLabelWidget); the interaction prompt under the crosshair for anything else. NativeTick calls it every
	 * frame with the pawn's components; tests call it.
	 */
	void UpdatePickupCard(UWeaponManagerComponent* Manager, const UInteractionComponent* Interaction, float DeltaTime);

	/** The in-hand card (null before the HUD is built), and whether it is up (tests). */
	UGunCardWidget* GetInHandCard() const { return InHandCard; }
	bool IsInHandCardShown() const;

	/**
	 * How far the gun is raised toward the sight (UPlayerViewComponent::GetAimAlpha, eased) when the crosshair is gone: it
	 * fades out over the first part of the raise, so it never shows beside the sight's own reticle.
	 */
	static constexpr float CrosshairGoneAtAim = 0.6f;

	/**
	 * How solid the crosshair's ticks are: gone while aiming down the sights (fading out over the raise and back as the gun
	 * comes down, with any gun in any view), while sprinting (the gun is down) and facing the camera (the front view).
	 */
	static float CrosshairOpacity(float AimAlpha, float SprintAlpha, bool bFacingCamera);

	/**
	 * The centre dot's: as the ticks in first person, where the sight's reticle takes over; through a third-person aim, which
	 * only zooms and shows no sight, it stays as the aim point. bHipDot false (a shape drawn with no dot: the rifle's cross,
	 * the launcher's diamond): only that third-person aim point, never beside the lines.
	 */
	static float AimDotOpacity(float AimAlpha, float SprintAlpha, bool bFacingCamera, bool bFirstPerson, bool bHipDot = true);

	/** The crosshair's lines (null before the HUD is built), for the tests. */
	UHudCrosshairWidget* GetCrosshair() const { return CrosshairLines; }

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

	FString BoundKeyName(FName BindingId, const TCHAR* Fallback) const;

	/** A melee strike's or a grenade's hit: the marker with no gun (never part of a blast). */
	UFUNCTION()
	void HandleHit(const FHitResult& Hit, float Damage, bool bCritical);

	/** The gun in hand's bullet hit: the marker, a pellet gun's pellets gathered into their blast's one. */
	UFUNCTION()
	void HandleGunHit(const FHitResult& Hit, float Damage, bool bCritical);

	/** Shows a hit's marker and plays its sound (HudHitMarker's rules); Gun: the gun whose bullet it was (none: melee, grenade). */
	void ShowHitMarker(const FHitResult& Hit, float Damage, bool bCritical, const AWeaponBase* Gun);

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

	/** Follows the pawn's ember component: its denied presses (the socket shakes or dims) and its state (a pop when ready). */
	void BindEmbers(UEmberComponent* Embers);

	/** The ember socket, frame by frame. */
	void UpdateEmbers(float DeltaTime);

	void HandleEmberDenied(EEmberDenied Why);
	void HandleEmberState(EEmberState NewState);

	UFUNCTION()
	void HandleFired();

	UFUNCTION()
	void HandleReloadStarted(float Duration);

	UFUNCTION()
	void HandleMessage(const FText& Message);

	/** The player frame's experience bar reached a new level: the banner shows it. */
	void HandleLevelUp(int32 NewLevel);

	// Crosshair and hit marker
	/** The crosshair's lines in their square (HudCrosshair::CanvasSize): the box kicks and grows in, the lines open. */
	UPROPERTY(Transient) TObjectPtr<USizeBox> CrosshairBox;
	UPROPERTY(Transient) TObjectPtr<UHudCrosshairWidget> CrosshairLines;
	/** The centre dot, apart from the lines: it stays through a third-person aim (AimDotOpacity) and doesn't kick. */
	UPROPERTY(Transient) TObjectPtr<UWidget> CrosshairDot;
	UPROPERTY(Transient) TObjectPtr<UHudCrosshairWidget> CrosshairDotShape;
	UPROPERTY(Transient) TObjectPtr<UWidget> HitMarker;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> HitMarkerTicks;

	// Bottom-left: the player frame; top-centre: the level-up banner
	UPROPERTY(Transient) TObjectPtr<UHudPlayerFrameWidget> PlayerFrame;
	UPROPERTY(Transient) TObjectPtr<UHudLevelUpBannerWidget> LevelUpBanner;
	/** The mission-complete banner, in the level-up banner's place: a level-up during it waits for it. */
	UPROPERTY(Transient) TObjectPtr<UHudMissionCompleteWidget> MissionBanner;
	/** The "GRAVE MARKED" banner, a little above them; it waits for them. */
	UPROPERTY(Transient) TObjectPtr<UHudGraveMarkedWidget> GraveBanner;
	/** The "FOUND" banner, in the same place; it waits for the three above (and the roost banner for it). */
	UPROPERTY(Transient) TObjectPtr<UHudFindBannerWidget> FindBanner;

	// Bottom-right: weapon
	UPROPERTY(Transient) TObjectPtr<UWidget> WeaponCluster;
	/** The grenades carried, over the weapon slots. */
	UPROPERTY(Transient) TObjectPtr<UHudGrenadeWidget> GrenadeCounter;
	/**
	 * The equipped ember power's socket, left of the weapon slots, and its row: apart from the weapon cluster (which hides
	 * with no gun in hand), so it shows armed or not; it fades with the cluster while the cluster shows.
	 */
	UPROPERTY(Transient) TObjectPtr<UHudEmberSocketWidget> EmberSocket;
	UPROPERTY(Transient) TObjectPtr<UWidget> EmberArea;
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
	/** The small diamond before the gun's name, in its rarity's colour. */
	UPROPERTY(Transient) TObjectPtr<UImage> RarityGem;
	/** The gun's notches before the gem (the user's call, 2026-10-14): collapsed for a gun with none. */
	UPROPERTY(Transient) TObjectPtr<UHudNotchTallyWidget> NotchTally;

	// The in-hand card and messages
	/** Right-middle: "IN HAND" over the gun in hand's card, while loot is looked at with a gun in hand. */
	UPROPERTY(Transient) TObjectPtr<UWidget> PickupCard;
	UPROPERTY(Transient) TObjectPtr<UGunCardWidget> InHandCard;
	UPROPERTY(Transient) TObjectPtr<UWidget> MessagePlate;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MessageText;

	// The interaction prompt under the crosshair
	UPROPERTY(Transient) TObjectPtr<UHudInteractPromptWidget> InteractPrompt;

	TWeakObjectPtr<UWeaponManagerComponent> BoundManager;
	TWeakObjectPtr<AWeaponBase> BoundWeapon;
	TWeakObjectPtr<UPlayerMeleeComponent> BoundMelee;
	TWeakObjectPtr<UPlayerThrowComponent> BoundThrow;
	FDelegateHandle GrenadesChangedHandle;
	TWeakObjectPtr<UEmberComponent> BoundEmbers;
	FDelegateHandle EmberDeniedHandle;
	FDelegateHandle EmberStateHandle;
	/** The frame the last grenade hit came in: a burst's other hits that frame show the marker without its sound again. */
	uint64 GrenadeHitFrame = 0;
	bool bMuteHitSound = false;

	/** The gun in hand the in-hand card shows (set again only when another comes into hand or the card shows again). */
	TWeakObjectPtr<const AWeaponBase> InHandShown;

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
	/** The spread's gap box (HudCrosshair::GapBoxFor), eased toward the gun's spread; the lines open by its growth. */
	float CrosshairSize = 0.f;
	/** Seconds since the last shot, while the crosshair's kick settles. */
	float CrosshairKickAge = 0.f;
	bool bCrosshairKicking = false;
	/** The gun kind whose shape is up, and seconds since it came up (another kind grows in: HudCrosshair::SwitchSeconds). */
	EWeaponKind CrosshairKind = EWeaponKind::None;
	bool bCrosshairKindShown = false;
	float CrosshairSwitchAge = 1.f;
	float PulseTime = 0.f;

	float HitMarkerTime = 0.f;
	/** What the marker shows until it fades (a kill's red is never taken back by a later pellet), how long, its pop and size. */
	EHudHitMark ShownMark = EHudHitMark::Body;
	float HitMarkerShowFor = 0.18f;
	float HitMarkerPop = 0.3f;
	float HitMarkerSize = 1.f;
	/** The pellet gun's blast whose hits are landing now: its first hit's time, its pellets on bodies, the strongest it sounded. */
	TWeakObjectPtr<const AWeaponBase> BlastGun;
	double BlastStarted = -1.0e9;
	int32 BlastLanded = 0;
	EHudHitMark BlastSounded = EHudHitMark::Body;
	/** The wards of the creatures last hit, so a hit can tell it broke one. */
	FHudWardMemory WardMemory;
	float MessageTime = 0.f;
};

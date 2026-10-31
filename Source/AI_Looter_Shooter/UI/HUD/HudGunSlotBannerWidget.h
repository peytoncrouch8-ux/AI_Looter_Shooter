#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudGunSlotBannerWidget.generated.h"

class UHudFindBannerWidget;
class UHudGraveMarkedWidget;
class UHudLevelUpBannerWidget;
class UHudMissionCompleteWidget;
class UTextBlock;
class UWidget;
enum class EGunSlotLock : uint8;

/**
 * "NEW GUN SLOT", the moment an equip slot opens for the player (GunSlotRules: the third at level 3, the fourth after
 * Ransom's Rest's story), in the level-up banner's "Signal Glass" family and its place (top-centre, 260 px down): floating
 * outlined type, salt hairlines with gold arrowheads, a gold bar, a soft dark shade its one background. In the middle the
 * slot itself as the HUD's column draws it, larger: shut, with its padlock, until the padlock pops off in a soul-gold
 * flash and the slot's number lands in it; under it the reward ("CARRY THREE GUNS") and what opened it ("LEVEL 3 REACHED").
 * It sounds UI.GunSlotUnlocked as it starts (UI.NotchMilestone, a milestone's sting, until that cue has its sounds).
 *
 * The weapon manager announces it (UWeaponManagerComponent::OnSlotUnlocked through UPlayerHUDWidget). Slots that open close
 * together show one after the other. It takes turns with every banner up top: it waits while the level-up, mission-complete,
 * roost or find banner shows (and they wait while it shows), then a short calm; a slot the level opened waits for the
 * level-up banner first (it comes as the experience bar gets there), so LEVEL UP 3 is followed by NEW GUN SLOT. It waits
 * while a menu is open, a scene holds the player or the game is paused (that time doesn't count). Collapsed while idle.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudGunSlotBannerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Its box, centred where the HUD places it (the level-up banner's middle). Under 768 wide, clear of the mission tracker. */
	static constexpr float Width = 760.f;
	static constexpr float Height = 400.f;

	/** One show: in, the padlock off, held and faded out over this long. */
	static constexpr float ShowSeconds = 3.3f;
	/** How long the others must have been quiet before it starts, and the longest a level's slot waits for LEVEL UP. */
	static constexpr float CalmSeconds = 0.35f;
	static constexpr float LevelUpWaitSeconds = 4.f;

	/** The banners it takes turns with (any may be null). */
	void SetOtherBanners(UHudLevelUpBannerWidget* InLevelUp, UHudMissionCompleteWidget* InMission, UHudGraveMarkedWidget* InGrave,
		UHudFindBannerWidget* InFind);

	/** Announces slot SlotIndex (from 0) open, Was what had shut it: queued behind any being shown. */
	void Announce(int32 SlotIndex, EGunSlotLock Was);

	/** It's on screen, or has an announcement waiting. */
	bool IsBusy() const { return Time >= 0.f || !Queue.IsEmpty(); }

	/** It's on screen now (the other banners wait while it is). */
	bool IsShowing() const { return Time >= 0.f; }

	/** Moves the show on by DeltaSeconds: the next announcement when it may start, the one showing toward its end (the tick calls it; tests call it). */
	void Advance(float DeltaSeconds);

	/** Announcements waiting behind the one showing. */
	int32 GetQueued() const { return Queue.Num(); }

	/** What it shows now, for the tests: the slot (from 0; INDEX_NONE idle) and the reward line's words. */
	int32 GetShownSlot() const { return Time >= 0.f ? ShownSlot : INDEX_NONE; }
	FString GetShownReward() const;

	/** The padlock is off and the slot's number in (OpenAt seconds into the show), for the tests. */
	bool IsOpenedInShow() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	struct FShow
	{
		int32 Slot = INDEX_NONE;
		EGunSlotLock Was{};
		/** Seconds it has waited at the front of the queue, and whether the level-up banner has shown meanwhile. */
		float Waited = 0.f;
		bool bLevelUpSeen = false;
	};

	/** A menu covers the game, a scene holds the player, or it's paused. */
	bool IsCovered() const;
	/** One of the other banners is on screen (or the mission-complete banner has one waiting). */
	bool IsOtherBannerShowing() const;
	bool IsLevelUpShowing() const;
	/** The front announcement may start now (the others quiet long enough; a level's slot after its LEVEL UP). */
	bool MayStart(FShow& Next, bool bOthers, float DeltaSeconds);
	void StartShow(const FShow& Show);
	/** Poses every piece for Time seconds into the show (hidden while idle or covered). */
	void PaintShow(bool bCovered);
	/** Its pieces (HudGunSlotBannerWidget.cpp, below the show). */
	void BuildTree();

	UPROPERTY(Transient) TObjectPtr<UWidget> Banner;
	UPROPERTY(Transient) TObjectPtr<UWidget> TitleText;
	UPROPERTY(Transient) TObjectPtr<UWidget> GoldBar;
	UPROPERTY(Transient) TObjectPtr<UWidget> LeftLine;
	UPROPERTY(Transient) TObjectPtr<UWidget> RightLine;
	UPROPERTY(Transient) TObjectPtr<UWidget> LeftArrow;
	UPROPERTY(Transient) TObjectPtr<UWidget> RightArrow;
	/** The slot, larger: its glow, its rings, the padlock, the flashing ring and the number. */
	UPROPERTY(Transient) TObjectPtr<UWidget> SlotGlow;
	UPROPERTY(Transient) TObjectPtr<UWidget> SlotMetal;
	UPROPERTY(Transient) TObjectPtr<UWidget> SlotHairline;
	UPROPERTY(Transient) TObjectPtr<UWidget> SlotLock;
	UPROPERTY(Transient) TObjectPtr<UWidget> SlotRing;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SlotNumber;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> RewardText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CauseText;

	TWeakObjectPtr<UHudLevelUpBannerWidget> LevelUpBanner;
	TWeakObjectPtr<UHudMissionCompleteWidget> MissionBanner;
	TWeakObjectPtr<UHudGraveMarkedWidget> GraveBanner;
	TWeakObjectPtr<UHudFindBannerWidget> FindBanner;

	TArray<FShow> Queue;
	int32 ShownSlot = INDEX_NONE;
	/** Seconds the other banners have been quiet (a new show waits for CalmSeconds of it). */
	float Calm = 0.f;
	/** Seconds into the show (below zero while idle); only time not covered counts. */
	float Time = -1.f;
};

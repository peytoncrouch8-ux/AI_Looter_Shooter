#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudGraveMarkedWidget.generated.h"

class UHudFindBannerWidget;
class UHudGunSlotBannerWidget;
class UHudLevelUpBannerWidget;
class UHudMissionCompleteWidget;
class UTextBlock;
class UWidget;

/**
 * The "POST LIT" banner, near the top of the screen (the user's call, 2026-10-31: the Wardens' lantern posts are the save,
 * respawn and fast-travel points, lit by walking up to them; Hob's roosts before, the respawn graves before those): a small
 * painted lantern post (its lantern just lit with its column rising, Hob's crow on the arm, the light's glow behind it), "POST
 * LIT" in orange between fading cyan rules, and the post's name in white under it. It eases in (a small pop, as the level-up
 * banner does), holds and eases out over 2.5 s. Floating outlined text and solid shapes, no panel. (Named for the graves.)
 *
 * ARespawnMarker announces a first find through ALooterHUD::ShowGraveMarked, which hands it here. Finds that come close
 * together show one after the other. It waits while a menu is open, a scene holds the player or the game is paused (that time
 * doesn't count, so the whole show is seen), and while the level-up or mission-complete banner shows, so they never overlap.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudGraveMarkedWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Its box, its middle placed where the banner's lantern and words centre. */
	static constexpr float Width = 640.f;
	static constexpr float Height = 150.f;

	/** One show: eased in, held and eased out over this long. */
	static constexpr float ShowSeconds = 2.5f;

	/** The banners it takes turns with: it waits while any shows (the find banner is optional). */
	void SetOtherBanners(UHudLevelUpBannerWidget* InLevelUpBanner, UHudMissionCompleteWidget* InMissionBanner,
		UHudFindBannerWidget* InFindBanner = nullptr);

	/** The "NEW GUN SLOT" banner, a little under this one: this waits while it shows (it waits while this shows). */
	void SetGunSlotBanner(UHudGunSlotBannerWidget* InGunSlotBanner);

	/** Announces a post lit: queued behind any being shown. */
	void Announce(const FText& GraveName);

	/** Its title, "POST LIT" (the tests read it). */
	static FText TitleWords();

	/** It's on screen, or has an announcement waiting. */
	bool IsBusy() const;

	/** It's on screen now (not covered by a menu or a scene). */
	bool IsShowing() const { return Time >= 0.f; }

	/** Moves the show on by DeltaSeconds: the next announcement when it's free, the one showing toward its end (the tick calls it; tests call it). */
	void Advance(float DeltaSeconds);

	/** The banner's opacity and scale Time seconds into a show: the ease in, the hold and the ease out (the tests check them). */
	static float OpacityAt(float Time);
	static float ScaleAt(float Time);

	/** What it shows now, for the tests: the roost's name as set. */
	FString GetShownName() const;

	/** Announcements waiting behind the one showing. */
	int32 GetQueued() const { return Queue.Num(); }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** A menu covers the game, a scene holds the player, or it's paused. */
	bool IsCovered() const;
	/** The level-up, mission-complete or find banner is on screen. */
	bool IsOtherBannerShowing() const;
	/** Scales and fades it for Time seconds into the show (hidden while idle or covered). */
	void PaintShow(bool bCovered);

	void BuildTree();

	UPROPERTY(Transient) TObjectPtr<UWidget> Banner;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(Transient) TObjectPtr<UWidget> Title;

	TWeakObjectPtr<UHudLevelUpBannerWidget> LevelUpBanner;
	TWeakObjectPtr<UHudMissionCompleteWidget> MissionBanner;
	TWeakObjectPtr<UHudFindBannerWidget> FindBanner;
	TWeakObjectPtr<UHudGunSlotBannerWidget> GunSlotBanner;

	TArray<FText> Queue;
	FText Shown;
	/** Seconds into the show (below zero while idle); only time not covered counts. */
	float Time = -1.f;
};

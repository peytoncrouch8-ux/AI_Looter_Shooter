#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Missions/MissionRewards.h"
#include "HudMissionCompleteWidget.generated.h"

class UHorizontalBox;
class UHudLevelUpBannerWidget;
class UImage;
class UMissionDefinition;
class UMissionRunner;
class UTextBlock;
class UWidget;

/**
 * The mission-complete banner, in the top-centre slot the level-up banner uses (never both at once): Borderlands' moment
 * when a mission is turned in. The ranger's star of the mission tracker, larger, in its gunmetal medal with orange rays
 * turning slowly behind it; "MISSION COMPLETE" in orange between fading cyan rules; the mission's name in white; and
 * what it gave on one line: "+40 XP", the reward gun's rarity in its colour, a named gun, the areas it opened. It pops
 * in with the mission-complete fanfare, the name and the rewards following a beat apart, holds and fades over 4.4 s.
 *
 * It hears the level's mission runner (UMissionRunner::OnMissionCompleted) and announces every main and side mission's
 * end, one at a time in the order they came. It waits while a menu is open, a scene holds the player or the game is
 * paused, and while the level-up banner shows; and a level-up that comes during it (a turn-in's experience) waits for it
 * to finish (DeferLevelUp), so the two fanfares and banners never overlap. Floating outlined text and solid shapes: the
 * medal's dark glass is its one background (the UI transparency setting fades it).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudMissionCompleteWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Its box, its middle placed where the level-up banner's gem centres. */
	static constexpr float Width = 1000.f;
	static constexpr float Height = 280.f;

	/** One show: popped in, held and faded out over this long. */
	static constexpr float ShowSeconds = 4.4f;

	/** The level-up banner it takes turns with: its level-ups wait for this, and this waits while it shows. */
	void SetLevelUpBanner(UHudLevelUpBannerWidget* InLevelUpBanner);

	/** Announces a mission's end (the runner's event calls it; tests may too): queued behind any being shown. */
	void Announce(const UMissionDefinition& Mission, const FMissionRewardsGiven& Given);

	/**
	 * A level-up reached while this is showing or has more to show: kept, and shown on the level-up banner once this is
	 * done. True when kept (the caller shows nothing now); false when this is idle and the level-up can show at once.
	 */
	bool DeferLevelUp(int32 NewLevel);

	/** It's on screen, or has an announcement waiting. */
	bool IsBusy() const;

	/** It's on screen now. */
	bool IsShowing() const { return Time >= 0.f; }

	/** Moves the show on by DeltaSeconds: the next announcement when it's free, the one showing toward its end (the tick calls it; tests call it). */
	void Advance(float DeltaSeconds);

	/** What the banner shows now, for the tests: the mission's name and the rewards line's words. */
	FString GetShownName() const;
	TArray<FString> GetShownRewards() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void BeginDestroy() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** One mission's end to announce. */
	struct FAnnouncement
	{
		FText Name;
		FMissionRewardsGiven Given;
	};

	void HandleMissionCompleted(const UMissionDefinition& Mission, const FMissionRewardsGiven& Given);
	/** Binds to the level's mission runner (once; again if the level's runner is another). */
	void BindRunner();
	/** A menu covers the game, a scene holds the player, or it's paused. */
	bool IsCovered() const;
	bool IsLevelUpShowing() const;
	/** Starts the next announcement: its words in place, the fanfare. */
	void StartNext();
	/** The show is over: the next one, or a level-up kept for it. */
	void EndShow();
	/** The words of Shown in the texts and the rewards row. */
	void ApplyTexts();
	/** One reward on the rewards row, after a diamond when it isn't the first. */
	void AddReward(const FString& Words, const FLinearColor& WordsColor);
	/** Scales and fades it all for Time seconds into the show (hidden while covered). */
	void PaintShow(bool bCovered);

	// --- Building it (HudMissionCompleteWidgetLayout.cpp) ---
	void BuildTree();

	UPROPERTY(Transient) TObjectPtr<UWidget> Banner;
	UPROPERTY(Transient) TObjectPtr<UWidget> Rays;
	UPROPERTY(Transient) TObjectPtr<UWidget> Title;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> RewardsRow;
	/** The rewards row's words, in order, as shown (for the tests). */
	TArray<FString> RewardWords;

	TWeakObjectPtr<UHudLevelUpBannerWidget> LevelUpBanner;
	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle CompletedHandle;

	TArray<FAnnouncement> Queue;
	FAnnouncement Shown;
	/** Seconds into the show (below zero while idle); only time not covered counts. */
	float Time = -1.f;
	/** A level-up kept for after the show (0: none). */
	int32 PendingLevel = 0;
};

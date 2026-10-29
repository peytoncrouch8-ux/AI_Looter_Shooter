#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/FindBannerShow.h"
#include "HudFindBannerWidget.generated.h"

class UHudGraveMarkedWidget;
class UHudLevelUpBannerWidget;
class UHudMissionCompleteWidget;
class UTextBlock;
class UWidget;

/**
 * The "FOUND" banner (Docs/Work/2026-10-28-Core/FINDS.md, section 3): the moment a thing off the path is found. The kit's
 * inked tally in soul-gold, "FOUND" in soul-gold between fading salt rules, the find's name in slate white, and under it
 * "4 / 15 · RANSOM'S REST" in salt with "+120 XP" counting up in soul-gold ("LEDGER PAGE · 2 / 6" for a page; the title
 * is "EVERY FIND IN RANSOM'S REST" for the last). The words come from FindRules, in FFindBannerShow. It eases in with a small pop, holds and eases out over 2.8 s. Floating outlined text and solid shapes, no
 * panel (so nothing is registered as a background).
 *
 * ALooterHUD::ShowFind hands each find here. Finds close together show one after the other; more than two waiting collapse
 * into one "3 FOUND" banner. It waits while a menu is open, a scene holds the player or the game is paused (that time doesn't
 * count), and while the level-up, mission-complete or roost banner shows (the roost banner waits for it in turn).
 * The sound is the subsystem's (World.Find.Discover), played the moment the find counts. As each show ends it tells the
 * subsystem (UFindSubsystem::BannerDone), which pays the find's experience then, so LEVEL UP follows the find.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudFindBannerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Its box, its middle placed where the banner's title centres. */
	static constexpr float Width = 640.f;
	static constexpr float Height = 150.f;

	/** One show: eased in, held and eased out over this long. */
	static constexpr float ShowSeconds = 2.8f;

	/** More than this many waiting collapse into one banner. */
	static constexpr int32 MaxQueued = 2;

	/** The banners it takes turns with: it waits while any shows. */
	void SetOtherBanners(UHudLevelUpBannerWidget* InLevelUpBanner, UHudMissionCompleteWidget* InMissionBanner, UHudGraveMarkedWidget* InRoostBanner);

	/** Announces a find: queued behind any being shown. */
	void Announce(const FFindBannerShow& Find);

	/** It's on screen, or has an announcement waiting. */
	bool IsBusy() const;

	/** It's on screen now (not covered by a menu or a scene). */
	bool IsShowing() const { return Time >= 0.f; }

	/** Moves the show on by DeltaSeconds: the next announcement when it's free, the one showing toward its end (the tick calls it; tests call it). */
	void Advance(float DeltaSeconds);

	/** The banner's opacity and scale Time seconds into a show (the tests check them). */
	static float OpacityAt(float Time);
	static float ScaleAt(float Time);

	/** The experience counted up to Time seconds into a show: 0 at first, all of Xp once the count ends. */
	static int32 XpAt(int32 Xp, float Time);

	/** The words: the title ("FOUND" unless the show names one; "3 FOUND" for a collapsed queue), the count line and the XP line ("+120 XP"; empty for none), in capitals. */
	static FString TitleWords(const FFindBannerShow& Find);
	static FString CountWords(const FFindBannerShow& Find);
	static FString XpWords(int32 Xp);

	/** The title's type size: 30 for a short title, smaller as it gets longer, so "EVERY FIND IN RANSOM'S REST" fits between its rules. */
	static int32 TitleSizeFor(const FString& Title);

	/** Many finds as one: the last one's count, the sum of their XP, Merged their number (the queue's collapse; the title becomes FindRules::CollapsedTitle). */
	static FFindBannerShow Merge(const TArray<FFindBannerShow>& Finds);

	/** What it shows now, for the tests. */
	FString GetShownTitle() const;
	FString GetShownName() const;
	FString GetShownCount() const;
	FString GetShownXp() const;

	/** Announcements waiting behind the one showing. */
	int32 GetQueued() const { return Queue.Num(); }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** A menu covers the game, a scene holds the player, or it's paused. */
	bool IsCovered() const;
	/** The level-up, mission-complete or roost banner is on screen. */
	bool IsOtherBannerShowing() const;
	/** Puts the shown find's words into the texts. */
	void ApplyWords();
	/** Scales and fades it for Time seconds into the show (hidden while idle or covered). */
	void PaintShow(bool bCovered);

	void BuildTree();

	UPROPERTY(Transient) TObjectPtr<UWidget> Banner;
	UPROPERTY(Transient) TObjectPtr<UWidget> Tally;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CountText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> XpText;

	TWeakObjectPtr<UHudLevelUpBannerWidget> LevelUpBanner;
	TWeakObjectPtr<UHudMissionCompleteWidget> MissionBanner;
	TWeakObjectPtr<UHudGraveMarkedWidget> RoostBanner;

	TArray<FFindBannerShow> Queue;
	FFindBannerShow Shown;
	/** Seconds into the show (below zero while idle); only time not covered counts. */
	float Time = -1.f;
	/** The XP number last put in the text, so the text is set only when the count moves. */
	int32 ShownXp = -1;
};

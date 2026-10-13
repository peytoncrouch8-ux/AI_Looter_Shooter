#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Menus/MenuNav.h"
#include "UI/Menus/MenuNavigator.h"
#include "UI/Style/LooterUIFocus.h"
#include "MissionOfferWidget.generated.h"

class AActor;
class ALooterHUD;
class UHorizontalBox;
class UImage;
class ULooterButton;
class UMissionDefinition;
class USizeBox;
class UTextBlock;
class UWidget;
class UWorld;

/** Who an offer comes from, as the card's words put it. */
enum class EMissionGiverKind : uint8
{
	/** Someone at a speaker point: "From Delia". */
	Someone,
	/** A notice board: "Posted on the Rim Rangers' board". */
	Board,
	/** Something found in the world (AMissionArtifact): "Found: a miner's tag". */
	Artifact,
};

/** One of the card's rewards: what it is (a small label) and how much, in its colour (a gun's rarity's). */
struct AI_LOOTER_SHOOTER_API FMissionOfferReward
{
	FText Label;
	FText Value;
	FLinearColor Color = FLinearColor::White;
};

/**
 * The offer card's words for a mission and its giver, apart from the widget so the tests read them: the kind line (MAIN
 * MISSION in gold, SIDE MISSION in white), the title, who offers it and where, the briefing (an artifact's own words as
 * a found document), the first step's objectives (later steps stay a surprise), the rewards, and the two choices.
 */
struct AI_LOOTER_SHOOTER_API FMissionOfferCard
{
	bool bMain = false;
	EMissionGiverKind GiverKind = EMissionGiverKind::Someone;
	/** "Main mission", "Side mission". */
	FText Kind;
	FText Title;
	/** "From Delia · Ransom Farm", "Posted on the notice board · Main Street", "Found: a miner's tag". */
	FText From;
	FText Briefing;
	/** The briefing is a found thing's own words (a letter, a tag): shown as a document. */
	bool bDocument = false;
	/** The first step's objectives, with the player's keys in them. */
	TArray<FString> Objectives;
	/** How many steps come after the first. */
	int32 MoreSteps = 0;
	TArray<FMissionOfferReward> Rewards;
	FText Accept;
	FText Decline;

	static FMissionOfferCard Make(const UMissionDefinition& Mission, const AActor* Giver, const UWorld* World);

	/** The kind of giver: by its actor's class, else by its tag's prefix (NoticeBoard_, Artifact_). */
	static EMissionGiverKind GiverKindOf(const UMissionDefinition& Mission, const AActor* Giver);

	/** The giver's name for people: the offer's own, else a board's or an artifact's title, else from the tag. */
	static FText GiverNameOf(const UMissionDefinition& Mission, const AActor* Giver);

	/** The named place on this level's map nearest Where ("Ransom Farm"), or empty (a level with no names, or none near). */
	static FText PlaceNear(const UWorld* World, const FVector& Where);

	/**
	 * Where to pick a mission up, for the Missions page: "Offered by Delia, at Ransom Farm", "Posted on the notice board,
	 * at Main Street", "A miner's tag, at Webwood"; with ElsewhereArea (it's another area's), "Offered by Delia, in
	 * Ransom's Rest".
	 */
	static FText WhereToGet(const UMissionDefinition& Mission, const AActor* Giver, const UWorld* World, const FText& ElsewhereArea);
};

/**
 * The mission offer card (Borderlands' way): a LooterUI panel over the dimmed world (or over a notice board's screen,
 * which stays behind it), opened by a giver (ALooterHUD::OpenMissionOffer): someone after their lines, a board's posting,
 * an artifact picked up, or the console (Looter.Mission.Offer). It shows the mission (FMissionOfferCard): its ! in gold
 * or white, the kind line, the title, who offers it and where, the story, the first step's objectives and the rewards,
 * with Accept (primary) and Not now.
 *
 * It slides up and fades in as it opens (Giver::Offer is heard), its ! pops. Accept takes the mission (the runner records
 * it, starts it and the tracker follows it): an ACCEPTED stamp slams down over the card (Giver::Accept) and it goes.
 * Not now closes it (Giver::Decline) and the mission stays offered. Accept needs a moment after it opens, so the key that
 * opened it can't take a mission unread.
 *
 * Keys: E (Interact) accepts, Esc or B is Not now; left and right (the arrows, A / D, the D-pad, the stick) move the focus
 * between the two buttons and Enter, Space or A presses the focused one. The mouse clicks either. The prompts follow the
 * device last used (LooterUIFocus.h). Every background it paints follows the UI transparency setting (MarkBackground).
 *
 * MissionOfferWidget.cpp builds it, accepts and declines; MissionOfferWidgetMotion.cpp moves it; MissionOfferWidgetCard.cpp
 * writes its words; MissionOfferWidgetInput.cpp is its keys, stick and mouse.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UMissionOfferWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Seconds after it opens before Accept takes a key. */
	static constexpr float ArmSeconds = 0.35f;
	/** Its slide and fade in. */
	static constexpr float OpenSeconds = 0.24f;
	/** Accepted: the stamp's slam, its hold, then the card fading out. */
	static constexpr float StampSlamSeconds = 0.12f;
	static constexpr float StampHoldSeconds = 0.42f;
	static constexpr float StampOutSeconds = 0.16f;
	/** Not now: the card fading out. */
	static constexpr float DeclineSeconds = 0.14f;

	/**
	 * Opens on Mission, offered by Giver (null: the console's), its words read afresh. bOverScreen: over another screen
	 * (a notice board's), which keeps its own dimming.
	 */
	void Open(ALooterHUD* InHUD, const UMissionDefinition* InMission, AActor* InGiver, bool bInOverScreen);

	/** Takes the mission: recorded, started, tracked; the stamp, then it closes. False (and a word on the card) when it can't be. */
	bool Accept();

	/** Not now: it closes and the mission stays offered. */
	void Decline();

	const UMissionDefinition* GetMission() const { return Mission.Get(); }
	AActor* GetGiver() const { return Giver.Get(); }
	const FMissionOfferCard& GetCard() const { return Card; }

	/** It was closed by Accept (the HUD tells a notice board behind it). */
	bool WasAccepted() const { return bAccepted; }

	/** Accept or Not now was chosen: it's on its way out and takes no more keys. */
	bool IsClosing() const { return bClosing; }

	/** Accept takes a key now (ArmSeconds after it opened, and not on its way out). */
	bool IsArmed() const { return !bClosing && Age >= ArmSeconds; }

	/** Moves its motion on (the tick does; tests call it). */
	void Advance(float DeltaSeconds);

protected:
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FNavigationReply NativeOnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent,
		const FNavigationReply& InDefaultReply) override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;

private:
	// --- MissionOfferWidget.cpp: the card ---

	void BuildCard();
	UWidget* MakeHeader();
	UWidget* MakeBriefing();
	UWidget* MakeObjectives();
	UWidget* MakeRewards();
	UWidget* MakeButtons();
	UWidget* MakeStamp();
	void SetStatus(const FText& Message);
	/** Its way out is over: the HUD takes it away. */
	void FinishClose();

	// --- MissionOfferWidgetMotion.cpp ---

	/** Puts it on screen as its motion stands: the slide, the fade, the ! and its glow, the stamp. */
	void ApplyMotion();

	// --- MissionOfferWidgetInput.cpp: keys, the stick, the mouse ---

	void HandleButton(ULooterButton* Button);
	void HandleHovered(ULooterButton* Button);
	/** The two buttons as the focus's stops, the focus on Accept. */
	void RebuildNav();
	/** The prompts at the bottom for the device last used, and the focus look shown or not. */
	void RefreshNavLook();
	/** A key in the menu: notes the device; false when this was the first key after the mouse (it only shows the focus). */
	bool RevealNav(const FKey& Key);
	void StepNav(MenuNav::EMove Move);
	void NotePointer();

	TWeakObjectPtr<ALooterHUD> OwningHUD;
	TWeakObjectPtr<const UMissionDefinition> Mission;
	TWeakObjectPtr<AActor> Giver;
	FMissionOfferCard Card;
	bool bOverScreen = false;
	bool bAccepted = false;
	bool bClosing = false;
	/** Seconds since it opened, and since Accept or Not now. */
	float Age = 0.f;
	float CloseAge = 0.f;

	FMenuNavigator Nav;
	FVector2D NavStick = FVector2D::ZeroVector;
	MenuNav::EMove NavStickMove = MenuNav::EMove::None;
	LooterUI::EMenuDevice ShownDevice = LooterUI::EMenuDevice::Mouse;

	UPROPERTY(Transient) TObjectPtr<UImage> Backdrop;
	UPROPERTY(Transient) TObjectPtr<USizeBox> PanelBox;
	/** What slides and fades: the panel with the stamp over it. */
	UPROPERTY(Transient) TObjectPtr<UWidget> CardBody;
	UPROPERTY(Transient) TObjectPtr<UImage> MarkImage;
	UPROPERTY(Transient) TObjectPtr<UImage> MarkHalo;
	UPROPERTY(Transient) TObjectPtr<UWidget> Stamp;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> AcceptButton;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> DeclineButton;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> PromptBar;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
};

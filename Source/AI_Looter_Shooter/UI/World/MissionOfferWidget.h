#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Menus/MenuNav.h"
#include "UI/Menus/MenuNavigator.h"
#include "UI/Style/LooterUIFocus.h"
#include "MissionOfferWidget.generated.h"

class AActor;
class ALooterHUD;
class UAudioComponent;
class UHorizontalBox;
class UImage;
class ULooterButton;
class ULooterProgressRing;
class UMissionDefinition;
class USizeBox;
class UTextBlock;
class UVerticalBox;
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

/** What the card is showing. */
enum class EMissionCardMode : uint8
{
	/** A mission offered: Accept or Not now. */
	Offer,
	/** A mission turned in with a choice of two rewards (FMissionTurnIn::Choices): hold one down to choose, or decide later. */
	Choice,
	/** Something read (AReadableProp): its words as a document, and Close. */
	Read,
};

/** One of the card's rewards: what it is (a small label) and how much, in its colour (a gun's rarity's). */
struct AI_LOOTER_SHOOTER_API FMissionOfferReward
{
	FText Label;
	FText Value;
	FLinearColor Color = FLinearColor::White;
};

/** One of a turn-in's two choices as the card shows it: its name, what it means, what it gives, its stamp. */
struct AI_LOOTER_SHOOTER_API FMissionChoiceCard
{
	FText Title;
	FText Line;
	TArray<FMissionOfferReward> Rewards;
	/** Stamped over the card as it's chosen ("PAID"). */
	FText Stamp;
};

/**
 * The offer card's words for a mission and its giver, apart from the widget so the tests read them: the kind line (MAIN
 * MISSION in gold, SIDE MISSION in white), the title, who offers it and where, the briefing (an artifact's own words as
 * a found document), the first step's objectives (later steps stay a surprise), the rewards, and the two buttons. In its
 * choice mode the two choices with what each gives and what's given either way; in its reading mode a readable thing's
 * words as a document.
 */
struct AI_LOOTER_SHOOTER_API FMissionOfferCard
{
	bool bMain = false;
	EMissionGiverKind GiverKind = EMissionGiverKind::Someone;
	/** "Main mission", "Side mission"; "Mission complete" for a choice; a readable's own heading. */
	FText Kind;
	FText Title;
	/** "From Delia · Ransom Farm", "Posted on the notice board · Main Street", "Found: a miner's tag". */
	FText From;
	FText Briefing;
	/** The briefing is a found thing's own words (a letter, a tag): shown as a document. */
	bool bDocument = false;
	/** The first step's objectives, with the player's keys in them (an optional one says so). */
	TArray<FString> Objectives;
	/** How many steps come after the first. */
	int32 MoreSteps = 0;
	TArray<FMissionOfferReward> Rewards;
	FText Accept;
	FText Decline;

	// --- The choice mode ---

	/** The question over the two ("Their wages, or the company's silver?"). */
	FText Question;
	/** What's given whichever is chosen (the experience, the optional objectives' bonus). */
	TArray<FMissionOfferReward> EitherWay;
	/** The two choices, in the data's order. */
	TArray<FMissionChoiceCard> Choices;

	static FMissionOfferCard Make(const UMissionDefinition& Mission, const AActor* Giver, const UWorld* World);

	/** The card for Mission's turn-in with its choice of two, turned in at Giver. */
	static FMissionOfferCard MakeChoice(const UMissionDefinition& Mission, const AActor* Giver, const UWorld* World);

	/** The card for reading Thing (AReadableProp): its heading, title and words as a document. */
	static FMissionOfferCard MakeReading(const AActor* Thing, const UWorld* World);

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
 * Its choice mode (OpenChoice, ALooterHUD::OpenTurnInChoice): a turn-in with a choice of two (Back Pay). The two side by
 * side, each with what it means and what it gives, what's given either way above them; the focused one is held down
 * (E, Enter, Space, A, or the mouse on it) for ChoiceHoldSeconds while its ring fills, so a choice that can't be undone is
 * never made by a stray key; its stamp slams down and the mission is turned in with it. Esc is "Decide later": the
 * mission waits, ready, for the next talk. Its reading mode (OpenReading): a readable thing's words as a document, Close.
 *
 * Keys: E (Interact) accepts, Esc or B is Not now; left and right (the arrows, A / D, the D-pad, the stick) move the focus
 * between the two buttons and Enter, Space or A presses the focused one. The mouse clicks either. The prompts follow the
 * device last used (LooterUIFocus.h). Every background it paints follows the UI transparency setting (MarkBackground).
 *
 * MissionOfferWidget.cpp builds it, accepts and declines; MissionOfferWidgetMotion.cpp moves it; MissionOfferWidgetCard.cpp
 * writes its words (every mode's); MissionOfferWidgetInput.cpp is its keys, stick and mouse; MissionOfferWidgetChoice.cpp
 * is the choice mode (its columns, the hold, choosing); MissionOfferWidgetRead.cpp the reading mode.
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
	/** A choice is held this long to be made; let go sooner, its ring drains back. */
	static constexpr float ChoiceHoldSeconds = 0.9f;
	/** A choice's stamp stays a little longer than ACCEPTED's: it's the end of the mission. */
	static constexpr float ChoiceStampHoldSeconds = 0.8f;

	/**
	 * Opens on Mission, offered by Giver (null: the console's), its words read afresh. bOverScreen: over another screen
	 * (a notice board's), which keeps its own dimming.
	 */
	void Open(ALooterHUD* InHUD, const UMissionDefinition* InMission, AActor* InGiver, bool bInOverScreen);

	/** Opens in its choice mode on Mission's turn-in at Giver (MissionOfferWidgetChoice.cpp). */
	void OpenChoice(ALooterHUD* InHUD, const UMissionDefinition* InMission, AActor* InGiver, bool bInOverScreen);

	/** Opens in its reading mode on Thing's words (MissionOfferWidgetRead.cpp). */
	void OpenReading(ALooterHUD* InHUD, AActor* InThing, bool bInOverScreen);

	/** Takes the mission: recorded, started, tracked; the stamp, then it closes. False (and a word on the card) when it can't be. */
	bool Accept();

	/** Not now (Decide later, Close): it closes and the mission stays offered (or ready to turn in). */
	void Decline();

	/**
	 * Makes the choice (0 or 1), as a hold that ran its time does: the mission is turned in with it (the runner), its stamp,
	 * its lines said once the card is gone, then it closes. False (and a word on the card) when it can't be.
	 */
	bool Choose(int32 Choice);

	/** Starts holding a choice down (a key, the mouse): it's focused, its ring fills. */
	void BeginHold(int32 Choice);

	/** Lets the hold go: its ring drains back. */
	void EndHold();

	EMissionCardMode GetMode() const { return Mode; }
	const UMissionDefinition* GetMission() const { return Mission.Get(); }
	AActor* GetGiver() const { return Giver.Get(); }
	const FMissionOfferCard& GetCard() const { return Card; }

	/** It was closed by Accept (the HUD tells a notice board behind it). */
	bool WasAccepted() const { return bAccepted && Mode == EMissionCardMode::Offer; }

	/** The choice made (0 or 1), or INDEX_NONE. */
	int32 GetChosen() const { return Chosen; }

	/** The choice being held down and how far its hold has run (0-1). */
	int32 GetHoldChoice() const { return HoldChoice; }
	float GetHoldProgress() const { return HoldProgress; }

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
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FNavigationReply NativeOnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent,
		const FNavigationReply& InDefaultReply) override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;

private:
	// --- MissionOfferWidget.cpp: the card ---

	/** Opens in Mode with Card already written: the common start of every mode. */
	void OpenAs(EMissionCardMode InMode, ALooterHUD* InHUD, const UMissionDefinition* InMission, AActor* InGiver, bool bInOverScreen);
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
	/** The seconds its way out takes, as it was closed. */
	float GetCloseSeconds() const;

	// --- MissionOfferWidgetChoice.cpp: the choice mode ---

	/** The choice mode's content: the question, what's given either way, the two columns. */
	void BuildChoiceContent(UVerticalBox* Content);
	UWidget* MakeChoiceColumn(int32 Index);
	/** Runs the hold on by DeltaSeconds and shows the columns as the focus and the hold stand. */
	void AdvanceChoice(float DeltaSeconds);
	/** A key in the choice mode: true when it took it. */
	bool HandleChoiceKeyDown(const FKeyEvent& InKeyEvent);
	bool HandleChoiceKeyUp(const FKeyEvent& InKeyEvent);
	/** The focused choice (0 or 1), or INDEX_NONE. */
	int32 GetFocusedChoice() const;
	UFUNCTION()
	void HandleFirstChoicePressed();
	UFUNCTION()
	void HandleSecondChoicePressed();
	UFUNCTION()
	void HandleChoiceReleased();

	// --- MissionOfferWidgetRead.cpp: the reading mode ---

	void BuildReadContent(UVerticalBox* Content);

	// --- MissionOfferWidgetMotion.cpp ---

	/** Puts it on screen as its motion stands: the slide, the fade, the ! and its glow, the stamp. */
	void ApplyMotion();

	// --- MissionOfferWidgetInput.cpp: keys, the stick, the mouse ---

	void HandleButton(ULooterButton* Button);
	void HandleHovered(ULooterButton* Button);
	/** The buttons as the focus's stops (Accept, the first choice or Close focused). */
	void RebuildNav();
	/** The prompts at the bottom for the device last used, and the focus look shown or not. */
	void RefreshNavLook();
	/** A key in the menu: notes the device; false when this was the first key after the mouse (it only shows the focus). */
	bool RevealNav(const FKey& Key);
	void StepNav(MenuNav::EMove Move);
	void NotePointer();
	/** The Interact key as the player bound it (E). */
	FKey GetInteractKey() const;

	TWeakObjectPtr<ALooterHUD> OwningHUD;
	TWeakObjectPtr<const UMissionDefinition> Mission;
	TWeakObjectPtr<AActor> Giver;
	FMissionOfferCard Card;
	EMissionCardMode Mode = EMissionCardMode::Offer;
	bool bOverScreen = false;
	bool bAccepted = false;
	bool bClosing = false;
	/** Seconds since it opened, and since Accept or Not now. */
	float Age = 0.f;
	float CloseAge = 0.f;

	/** The choice mode: the one made, the one held down and how far, whether a key or the mouse holds it. */
	int32 Chosen = INDEX_NONE;
	int32 HoldChoice = INDEX_NONE;
	float HoldProgress = 0.f;
	bool bHoldDown = false;
	FKey HoldKey;
	TWeakObjectPtr<UAudioComponent> HoldSound;

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
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StampWord;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> AcceptButton;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> DeclineButton;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> PromptBar;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;

	/** The choice mode's two columns: each a button holding its card, its edge, its hold row with the ring and key. */
	UPROPERTY(Transient) TArray<TObjectPtr<ULooterButton>> ChoiceButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> ChoiceBodies;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> ChoiceEdges;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> ChoiceFills;
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> ChoiceHoldRows;
	UPROPERTY(Transient) TArray<TObjectPtr<ULooterProgressRing>> ChoiceRings;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> ChoiceKeys;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> ChoiceHoldWords;
};

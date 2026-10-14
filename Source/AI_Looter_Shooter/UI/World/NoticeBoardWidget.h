#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Missions/MissionRewards.h"
#include "UI/Menus/MenuNav.h"
#include "UI/Menus/MenuNavigator.h"
#include "UI/Style/LooterUIFocus.h"
#include "World/NoticeBoard.h"
#include "NoticeBoardWidget.generated.h"

class ALooterHUD;
class UHorizontalBox;
class UImage;
class ULooterButton;
class UMissionDefinition;
class UMissionRunner;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UWidget;
struct FMissionRewards;

/**
 * A notice board's screen (ANoticeBoard: Skyreach's practice board, a town's bounty board), one LooterUI panel over the
 * dimmed world, opened by reading the board (ALooterHUD::OpenNoticeBoard). It lists the board's postings as they stand
 * (ANoticeBoard::GatherPostings), each a card pinned up: its title, its notice in the town's words, its count while open,
 * its reward (the offer card's numbers: "Reward: +1,300 XP · +60 silver"), and what can be done with it here:
 *  - one ready to turn in shows "[E] Turn in": turned in, its card says what it gave (the gun lands at the player's feet,
 *    in front of the board) in the reward's rarity colour, and a stamp and the mission's fanfare are heard;
 *  - one open and not tracked shows "[E] Track": the minimap and the HUD's tracker follow it;
 *  - one done is stamped done; one not up yet is faint, with the board's note (Skyreach's: bring a gun first);
 *  - one up for the taking (a bounty board's job: the board offers it) shows its ! and "[E] Take a look": the offer card
 *    opens over the board (ALooterHUD::OpenMissionOffer), which has the keys until it closes; the board shows it taken.
 * The cards that can be acted on are the stops of an FMenuNavigator: W / S, the arrows, the D-pad or the left stick choose
 * one (the kit's focus look on it while keys drive), E (or wherever Interact is bound), Enter, Space or A act on it, Esc
 * or B close. Under the mouse a card lights as it's pointed at; a click chooses a card, a click on the chosen one (or a
 * double click) acts on it, as the keys do. The prompts at the bottom name the device's keys. Every background it paints
 * follows the UI transparency setting (MarkBackground).
 *
 * NoticeBoardWidget.cpp builds it and opens and closes it; NoticeBoardWidgetCards.cpp makes the posting cards and acts on
 * them; NoticeBoardWidgetInput.cpp is its keys, stick, mouse and focus.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UNoticeBoardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Opens on Board, its postings read afresh. */
	void Open(ALooterHUD* InHUD, ANoticeBoard* InBoard);

	/** Closes it: the HUD hands the game its input back. */
	void Close();

	/** The postings shown, in order. */
	const TArray<FNoticeBoardPosting>& GetPostings() const { return Postings; }

	/** The card chosen now (what E acts on), or INDEX_NONE when none can be acted on. */
	int32 GetSelected() const { return Selected; }

	/**
	 * A click on card Index, as the mouse gives it: a card not chosen yet is chosen (as W and S choose); the chosen one is
	 * acted on, as E acts (so a double click does both). False for a card that can't be acted on here.
	 */
	bool ClickCard(int32 Index);

	/** The offer card opened from one of its postings closed (bAccepted: taken): the board reads its postings again and says so. */
	void HandleOfferClosed(FName MissionId, bool bAccepted);

	/**
	 * What a posting's reward is, in a few words for its card, with the numbers its offer card shows: "Reward: +1,300 XP ·
	 * +60 silver", "Reward: Pump Shotgun", a choice at the turn-in said too. A practice posting (Skyreach's, outside the
	 * story: EMissionKind::Tutorial) with nothing to give says so ("No reward: it's practice"); any other with nothing, empty.
	 */
	static FText DescribeReward(const UMissionDefinition& Mission);

	/**
	 * The same from the rewards alone: the purse the rewards' own, else the giver's kept by PurseId (MissionRewards::PurseOf);
	 * with nothing to give, the practice line when bPractice (Skyreach's postings, which the board was first made for), else
	 * empty.
	 */
	static FText DescribeReward(const FMissionRewards& Rewards, FName PurseId = NAME_None, bool bPractice = true);

	/** What a turn-in gave, for its card ("Rare Pump Shotgun, at your feet"); empty when it gave nothing. */
	static FText DescribeGiven(const FMissionRewards& Rewards, const FMissionRewardsGiven& Given);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
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
	/** A card's button (none for one that can't be acted on) and the parts that restyle as it's chosen or pointed at. */
	struct FCard
	{
		ULooterButton* Button = nullptr;
		UImage* Fill = nullptr;
		UImage* Line = nullptr;
		/** The kit's focus look over the card (LooterUI::AddFocusFrame), lit by the navigator while keys drive the board. */
		UWidget* Focus = nullptr;
		/** The keycap on its side naming the key that acts on it ("E", a pad's "A"). */
		UTextBlock* KeyText = nullptr;
	};

	// --- NoticeBoardWidget.cpp: the panel ---

	/** Reads the postings again and builds the panel for them, keeping the chosen card where it can. */
	void Refresh();
	void BuildPanel();
	/** Each card's fill and edge: chosen, pointed at, or as its state is. */
	void Restyle();
	void SetStatus(const FText& Message, const FLinearColor& TextColor);
	UMissionRunner* GetRunner() const;

	// --- NoticeBoardWidgetCards.cpp: the cards and acting on them ---

	UWidget* MakeCard(int32 Index);
	/** The card's state on its right: its word and colour ("Open  3 / 6", "Ready to turn in", "Done"). */
	FText StateWord(const FNoticeBoardPosting& Posting) const;
	FLinearColor StateColor(const FNoticeBoardPosting& Posting) const;
	/** Card Index can be acted on here: turned in (ready, given here), tracked (open, not tracked yet) or taken (offered). */
	bool CanAct(int32 Index) const;
	/** Turns in, tracks or offers card Index (the offer card over the board). */
	void Act(int32 Index);
	/** The runner finished a mission: a turn-in here keeps what it gave, for its card. */
	void HandleMissionCompleted(const UMissionDefinition& Mission, const FMissionRewardsGiven& Given);

	// --- NoticeBoardWidgetInput.cpp: choosing, the keys, the stick, the mouse ---

	/** Chooses card Index (one that can be acted on): it lights orange and the navigator's focus goes to it. */
	void Select(int32 Index);
	/** Moves the choice by Direction to the next card that can be acted on, round the ends. */
	void Step(int32 Direction);
	/** The cards that can be acted on as the navigator's stops, the chosen one focused. */
	void RebuildNav();
	/** The focus look shown or not, the keycaps and the prompts at the bottom, for the device last used. */
	void RefreshNavLook();
	void RefreshPrompts();
	/** A key used on the board: notes the device (the focus look shows). */
	void NoteKey(const FKey& Key);
	/** The mouse moved or clicked on the board: notes the device (the focus look goes). */
	void NotePointer();
	/** E, Enter, Space, a pad's A, or wherever the player bound Interact. */
	bool IsActKey(const FKey& Key) const;
	/** The keycap that acts on a card: a pad's "A", else the Interact key ("E"). */
	FString ActKeyName() const;
	/** The card whose button Widget is, or INDEX_NONE. */
	int32 CardOf(const UWidget* Widget) const;
	void HandleButton(ULooterButton* Button);

	TWeakObjectPtr<ALooterHUD> OwningHUD;
	TWeakObjectPtr<ANoticeBoard> Board;
	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle CompletedHandle;

	TArray<FNoticeBoardPosting> Postings;
	/** What each posting turned in while the screen was open gave, by mission id. */
	TMap<FName, FMissionRewardsGiven> GivenHere;

	UPROPERTY(Transient) TObjectPtr<USizeBox> PanelBox;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> CardsBox;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> PromptBar;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> CloseButton;

	TArray<FCard> Cards;
	int32 Selected = INDEX_NONE;
	/** The card under the mouse (its hover look), or INDEX_NONE. */
	int32 Hovered = INDEX_NONE;

	FMenuNavigator Nav;
	FVector2D NavStick = FVector2D::ZeroVector;
	MenuNav::EMove NavStickMove = MenuNav::EMove::None;
	LooterUI::EMenuDevice ShownDevice = LooterUI::EMenuDevice::Mouse;
};

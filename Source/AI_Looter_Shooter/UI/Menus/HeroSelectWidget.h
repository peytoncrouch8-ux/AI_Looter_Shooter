#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Menus/MenuNav.h"
#include "UI/Menus/MenuNavigator.h"
#include "UI/Style/LooterUIFocus.h"
#include "HeroSelectWidget.generated.h"

class UHeroDefinition;
class UHorizontalBox;
class UImage;
class ULooterButton;
class UOverlay;
class UTextBlock;
class UTexture2D;
class UVerticalBox;

/** The hero was confirmed: session Slot starts a new game as HeroId (after the confirm's short beat). */
DECLARE_DELEGATE_TwoParams(FOnHeroConfirmed, int32 /*Slot*/, FName /*HeroId*/);

/**
 * The hero select (the user's call, 2026-10-15: before a new game the player picks one of five, Borderlands' way),
 * opened by an empty session's New Game on the main menu, over the island, the main menu's column hidden. Laid out at
 * 1600 x 900 and scaled to the screen like the inventory's pages (Docs/HeroSelect/ui.html's idea):
 *  - top left: CHOOSE YOUR HERO, for which session
 *  - the stage: the chosen hero's painted portrait, large, a glow in their colour behind it (room for a live 3D stand
 *    later); greyed with an IN THE WORKS stamp while the hero isn't playable
 *  - under it the roster: the five as cards (the portrait's band at the eyes, a strip in their colour, name, role, the
 *    number key), the chosen one's edge orange; a hero in the works greyed and stamped
 *  - right: the hero's panel (role, name, story, the three ember powers with their words and clocks), Back and
 *    "Play as <hero>" (disabled, "In the works", for a hero not playable)
 *  - bottom left: what the keys do.
 * The keyboard and a controller drive it as the other menus (FMenuNavigator: the roster's row, then Back and Play): left
 * and right move along the roster and the choice follows, 1 to 5 and the bumpers choose too, A or Enter on a card or on
 * Play confirms, B or Esc goes back to the session picker. The mouse chooses a card by clicking it and confirms with
 * Play. Sounds: UI.Hover as the choice or the focus moves, UI.Equip and Ember.Ready on confirming, UI.Denied for a hero
 * in the works, UI.Back on the way out. Confirming plays a short beat (the portrait lifts, the glow brightens) before
 * OnConfirmed, which the main menu answers with USessionSubsystem::StartNewGame.
 *
 * Only playable heroes can be confirmed (UHeroDefinition::CanBePicked; Looter.Hero.Unlock 1 lets the rest through).
 * HeroSelectWidget.cpp builds it and handles choosing and confirming; HeroSelectRoster.cpp the roster's cards;
 * HeroSelectNav.cpp the keys, the mouse and the key hints; HeroSelectParts the portraits and power rows.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHeroSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Seconds from confirming to OnConfirmed: the beat that says the pick took. */
	static constexpr float ConfirmSeconds = 0.85f;

	/** Shows the roster for session InSlot's new game (0 to 2), Ellis chosen, the focus on their card. */
	void Open(int32 InSlot);

	int32 GetSlot() const { return SessionSlot; }

	// --- The roster and the choice (HeroSelectWidget.cpp) ---

	int32 NumHeroes() const { return Heroes.Num(); }
	const UHeroDefinition* GetHero(int32 Index) const;
	int32 GetSelected() const { return Selected; }
	FName GetSelectedId() const;

	/** Chooses hero Index (clamped): the stage and the panel show them. bSound: UI.Hover when it changed. */
	void Select(int32 Index, bool bSound = true);
	/** Chooses the hero with Id; false when they aren't on the roster. */
	bool SelectById(FName Id);

	/** Hero Index can be confirmed: playable, or Looter.Hero.Unlock (UHeroDefinition::CanBePicked). */
	bool CanConfirm(int32 Index) const;

	/**
	 * Confirms the chosen hero: UI.Equip and Ember.Ready, the beat, then OnConfirmed(Slot, Id). False, with UI.Denied and
	 * the reason on the panel, for a hero in the works (or while a confirm is already under way).
	 */
	bool Confirm();
	bool IsConfirming() const { return ConfirmLeft >= 0.f; }

	/** The start failed after all (the main menu's StartNewGame refused): the screen takes keys again and says so. */
	void CancelConfirm(const FString& Why);

	/** The panel's status line, as it reads (for the tests). */
	FString GetStatus() const;

	// --- The keyboard and a controller (HeroSelectNav.cpp; the main menu hands every key on while this shows) ---

	/** Like UMainMenuWidget::HandleNavKey: moves, A, B, 1 to 5 and the bumpers. True when it was this screen's. */
	bool HandleNavKey(const FKey& Key, bool bRepeat = false);
	void HandleNavKeyUp(const FKey& Key);
	void HandleNavStick(const FVector2D& Stick);
	/** A frame: held keys' repeats, the focus look's breath, the confirm's beat, the glow's pulse. */
	void AdvanceNav(float DeltaSeconds);
	/** The focus look shown or not, for the device last used. */
	void RefreshNavLook();
	UWidget* GetFocusedWidget() const;
	TArray<MenuNav::FPrompt> GetPrompts() const;

	ULooterButton* GetCard(int32 Index) const;
	ULooterButton* GetConfirmButton() const { return ConfirmButton; }
	ULooterButton* GetBackButton() const { return BackButton; }

	/** The pick was confirmed and its beat played out. */
	FOnHeroConfirmed OnConfirmed;
	/** B, Esc or Back: the main menu goes back to the session picker. */
	FSimpleDelegate OnBack;

protected:
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	/** A roster card's parts that change with the choice. */
	struct FCard
	{
		ULooterButton* Button = nullptr;
		UImage* Fill = nullptr;
		UImage* Line = nullptr;
		UWidget* FocusLook = nullptr;
		UTextBlock* Name = nullptr;
	};

	// HeroSelectWidget.cpp: layout, the stage, the panel
	void Gather();
	UWidget* MakeHeader();
	UWidget* MakeStage();
	UWidget* MakePanel();
	/** The stage and the panel for the chosen hero, and each card's look. */
	void ShowSelected();
	// HeroSelectRoster.cpp: the cards
	void RebuildRoster();
	FCard MakeCard(int32 Index, const UHeroDefinition& Hero);
	void RestyleCards();
	void SetStatus(const FString& Words, const FLinearColor& Tone);
	/** The confirm's beat, a frame at a time (from AdvanceNav). */
	void TickConfirm(float DeltaSeconds);

	// HeroSelectNav.cpp: keys, mouse, key hints
	void HandleButton(ULooterButton* Button);
	void HandleButtonHovered(ULooterButton* Button);
	void RebuildNavigation(const UWidget* Prefer = nullptr);
	void PressNav(const FKey& Key, MenuNav::EMove Move);
	void StepNav(MenuNav::EMove Move);
	bool RevealNav(const FKey& Key);
	void StopNavHold();
	void GoBack();
	/** The choice follows the focus along the roster (keys and the controller). */
	void FollowFocus();
	void RefreshFooter();

	/** The roster's heroes, left to right (UHeroDefinition::RosterIds; a missing definition is left out). */
	UPROPERTY(Transient) TArray<TObjectPtr<const UHeroDefinition>> Heroes;
	/** Their portraits (null while one isn't imported), kept loaded while the screen is. */
	UPROPERTY(Transient) TArray<TObjectPtr<UTexture2D>> Portraits;
	/** Whether each can be confirmed, read as the screen opens (loading a hero's mesh to tell isn't free). */
	TArray<bool> Pickable;

	UPROPERTY(Transient) TObjectPtr<UTextBlock> Eyebrow;
	UPROPERTY(Transient) TObjectPtr<UImage> StageGlow;
	UPROPERTY(Transient) TObjectPtr<UWidget> StagePicture;
	UPROPERTY(Transient) TObjectPtr<UImage> StageImage;
	UPROPERTY(Transient) TObjectPtr<UImage> StageWash;
	UPROPERTY(Transient) TObjectPtr<UWidget> StageStamp;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> RosterRow;

	UPROPERTY(Transient) TObjectPtr<UTextBlock> RoleText;
	UPROPERTY(Transient) TObjectPtr<UWidget> RoleWorks;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(Transient) TObjectPtr<UImage> NameRule;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StoryText;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> PowerList;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> BackButton;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> ConfirmButton;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> FooterPrompts;

	TArray<FCard> Cards;
	int32 SessionSlot = 0;
	int32 Selected = 0;
	/** Seconds left of the confirm's beat (0 once over, while the session starts); negative while none runs. */
	float ConfirmLeft = -1.f;
	/** OnConfirmed went out for this confirm (once only). */
	bool bConfirmSent = false;
	/** The glow's slow breath behind the stage. */
	float GlowClock = 0.f;
	/** 0 to 1: the stage's portrait settling in after the choice changed. */
	float StageIn = 1.f;
	/** The card under the mouse (it lights), or INDEX_NONE. */
	int32 HoveredCard = INDEX_NONE;

	FMenuNavigator Nav;
	MenuNav::FRepeat NavRepeat;
	FVector2D NavStick = FVector2D::ZeroVector;
	MenuNav::EMove NavStickMove = MenuNav::EMove::None;
	FString ShownPrompts;
};

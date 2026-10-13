#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "World/BossShrineRules.h"
#include "BossShrineWidget.generated.h"

class ABossShrine;
class APlayerController;
class UHorizontalBox;
class ULooterButton;
class USizeBox;
class UTextBlock;
struct FBossShrineView;

/**
 * A vigil stone's menu (ABossShrine; the user's "a pedestal with a menu"), one LooterUI panel over the dimmed world, opened by
 * a tap of Interact at the stone: its boss's name and level, when it was last laid to rest, how many times it was beaten,
 * what it drops (with the call back's lesser luck), and "Call it back". While the boss can't be called the button refuses
 * (the stone's refusal sound) and the panel says why: the countdown to its call back (mm:ss, running by the frame), the
 * boss out there now, or not yet; and the rule under it (five minutes, or at once after a Save & Quit).
 *
 * E, Enter or the face button rings the bell; Esc or the other face button closes it; the mouse clicks either. It opens and
 * closes itself (the stone asks it to: OpenFor), holding the keyboard and showing the mouse while it's up, and closes on its
 * own if the player walks off or falls. Every background it paints follows the UI transparency setting (MarkBackground).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UBossShrineWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Opens Shrine's menu for Player (closing any stone's menu already up). Null when it can't (no local player). */
	static UBossShrineWidget* OpenFor(APlayerController* Player, ABossShrine* Shrine);

	/** The stone's menu up now, if any. */
	static UBossShrineWidget* GetOpen();

	/** Closes it: the game has its keys and its mouse back. */
	void Close();

	/** Rings the bell (the button, E): the boss called back and the menu closed, or the refusal. Whether it came. */
	bool TryCall();

	ABossShrine* GetShrine() const;

	/** The state the panel shows now. */
	EBossShrineState GetShownState() const { return ShownState; }

	/** The player walks this far (cm) from the stone and the menu closes. */
	static constexpr float StayRadius = 450.f;

protected:
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	/** Builds the panel (the tree is made once; the words follow the stone's view each frame). */
	void BuildPanel();
	/** One row of the record: its label on the left, its value (returned) on the right. */
	UWidget* MakeInfoRow(const FText& Label, TObjectPtr<UTextBlock>& OutValue);
	/** Puts the stone's view on the panel; the buttons are made again only when the call's readiness changes. */
	void ShowView(const FBossShrineView& View);
	void MakeButtons(bool bCanCall);
	void HandleButton(ULooterButton* Button);
	/** The player can't keep the menu up any more (gone, fallen, walked off, the stone gone). */
	bool ShouldClose() const;

	TWeakObjectPtr<ABossShrine> Shrine;
	TWeakObjectPtr<APlayerController> Player;

	UPROPERTY(Transient) TObjectPtr<USizeBox> PanelBox;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LevelText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LastKillText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TimesText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LootText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CountdownText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> ButtonRow;

	EBossShrineState ShownState = EBossShrineState::Dormant;
	bool bButtonsMade = false;
	bool bButtonsReady = false;
	/** A refused press flashes the status this long (s). */
	float RefusedFlash = 0.f;
};

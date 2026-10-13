#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtrTemplates.h"

class ULooterButton;
class USlider;
class UWidget;

/**
 * One stop of a menu's keyboard and controller focus: a button (A presses it), a slider (left and right step it) or a
 * row of choices, one lit at a time like the settings' switches (left and right pick the one beside it, A the next round),
 * and what lights up while it has the focus: the button itself, or its row's focus look (LooterUI::AddFocusFrame).
 */
struct FMenuNavItem
{
	TWeakObjectPtr<ULooterButton> Button;
	TWeakObjectPtr<USlider> Slider;
	TArray<TWeakObjectPtr<ULooterButton>> Choices;
	/** What lights up (a ULooterButton, or a focus look). */
	TWeakObjectPtr<UWidget> Light;
	/** What a scrolling list brings into view for it (its row); none outside the list. */
	TWeakObjectPtr<UWidget> Row;
	/** The section it's in, for the bumpers; INDEX_NONE above the first (and the menu's own count below the last). */
	int32 Section = INDEX_NONE;

	static FMenuNavItem MakeButton(ULooterButton* InButton, int32 InSection = INDEX_NONE, UWidget* InRow = nullptr);
	static FMenuNavItem MakeSlider(USlider* InSlider, UWidget* InLight, UWidget* InRow, int32 InSection);
	static FMenuNavItem MakeChoices(const TArray<ULooterButton*>& InChoices, UWidget* InLight, UWidget* InRow, int32 InSection);

	bool IsButton() const { return Button.IsValid(); }
	bool IsSlider() const { return Slider.IsValid(); }
	bool IsChoices() const { return !Choices.IsEmpty(); }
	/** The widget it stands for: its button, its slider or its first choice. */
	UWidget* GetWidget() const;
	/** Shown and enabled: the focus passes over the rest (a collapsed Resume, Multiplayer's SOON). */
	bool IsAvailable() const;
	/** Which choice is lit now, or INDEX_NONE. */
	int32 GetChosen() const;
};

/** A row of stops, left to right. */
using FMenuNavRow = TArray<FMenuNavItem>;

/** What a key did to the focus. */
enum class EMenuNavResult : uint8
{
	/** Nothing: an edge, or nothing to do there. */
	None,
	/** The focus moved to another stop. */
	Moved,
	/** A slider stepped. */
	Stepped,
	/** Another choice was picked. */
	Chose,
	/** A button was pressed. */
	Pressed,
};

/**
 * A menu's focus for the keyboard and a controller: its stops in rows, top to bottom, where the focus is, moving it,
 * pressing, stepping and choosing, and the focus look on the stop that has it while the menus are driven by keys
 * (SetShown). Up and down move between rows (keeping to the column, round the ends with SetWrap), left and right along a
 * row, or step the slider or the choices the focus is on. A slider stepped by keys is held like a dragged one: its
 * controller capture begins at the first step and ends at EndSliderHold, so its preview and let-go save work as for the
 * mouse. Plain C++ over the menu's widgets (held weakly), so the main menu and the settings menu share it.
 */
class AI_LOOTER_SHOOTER_API FMenuNavigator
{
public:
	/**
	 * The menu's stops, top to bottom. The focus goes to Prefer if it's there and available, else stays on the widget it
	 * was on, else the first available stop.
	 */
	void SetRows(TArray<FMenuNavRow> InRows, const UWidget* Prefer = nullptr);
	/** Up from the top goes to the bottom, and down from the bottom to the top (short menus; a long list stops). */
	void SetWrap(bool bInWrap) { bWrap = bInWrap; }
	/** No stops: the focus look put out and any held slider let go. */
	void Reset();

	EMenuNavResult MoveVertical(int32 Step);
	/** Along the row, or a step of the slider or the choices the focus is on. */
	EMenuNavResult MoveHorizontal(int32 Step);
	/** A: press the button, or pick the next choice round. Nothing on a slider. */
	EMenuNavResult Activate();
	/**
	 * The bumpers: to the first stop of the next (Step 1) or previous section of NumSections, round the ends. From above
	 * the first section, the next is the first; from below the last, the previous is the last.
	 */
	EMenuNavResult JumpSection(int32 Step, int32 NumSections);
	/** Puts the focus on Widget's stop (the mouse pointed at it) without a sound; false when it isn't one. */
	bool FocusOn(const UWidget* Widget);

	const FMenuNavItem* GetFocused() const;
	UWidget* GetFocusedWidget() const;
	int32 GetRow() const { return Row; }
	int32 GetColumn() const { return Column; }
	int32 NumRows() const { return Rows.Num(); }

	/** The focus look shows (keys or a gamepad drive the menu) or not (the mouse). */
	void SetShown(bool bInShown);
	bool IsShown() const { return bShown; }
	/** The focus look's breath. */
	void Tick(float DeltaSeconds);

	/** Lets go of a slider held by keys (its controller capture ends): the key was let go, or the focus moved. */
	void EndSliderHold();
	USlider* GetHeldSlider() const { return HeldSlider.Get(); }

	/** The value a slider steps to from Value: the next whole step that way (an off-step value lands on the step beside it). */
	static float SteppedValue(float Value, float Min, float Max, float Step, int32 Direction);

private:
	bool IsRowAvailable(int32 RowIndex) const;
	/** The available column in RowIndex nearest Wanted, or INDEX_NONE. */
	int32 NearestColumn(int32 RowIndex, int32 Wanted) const;
	bool FindWidget(const UWidget* Widget, int32& OutRow, int32& OutColumn) const;
	void FocusFirst();
	void SetFocus(int32 NewRow, int32 NewColumn);
	EMenuNavResult StepSlider(USlider& Slider, int32 Direction);
	/** The lit choice's neighbour that way (bWrapRound: round the ends); false when there's none to pick. */
	bool PickChoice(const FMenuNavItem& Item, int32 Direction, bool bWrapRound);
	void Relight();

	TArray<FMenuNavRow> Rows;
	int32 Row = INDEX_NONE;
	int32 Column = 0;
	/** The column up and down keep to, set by moving along a row (a short row in between doesn't lose it). */
	int32 WantedColumn = 0;
	bool bWrap = false;
	bool bShown = false;
	TWeakObjectPtr<UWidget> LitWidget;
	float LitAge = 0.f;
	TWeakObjectPtr<USlider> HeldSlider;
};

// UMainMenuWidget for the keyboard and a controller: the focus over the main buttons, the session cards and the popups
// (round the ends), A to press, B to step back, and the key hints at the bottom for the device last used
// (LooterUIFocus.h, MenuNav.h, MenuNavigator.h). While the hero select shows, every key and its clock go to it.

#include "UI/Menus/MainMenuWidget.h"
#include "UI/Menus/HeroSelectWidget.h"
#include "UI/Menus/MainMenuHUD.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIFocus.h"
#include "Audio/LooterSound.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

using namespace MenuNav;

namespace
{
	/** The gap between the key hints at the bottom. */
	constexpr float PromptGap = 20.f;
}

ULooterButton* UMainMenuWidget::GetPickerBackButton() const
{
	return PickerBackButton;
}

// ---------------------------------------------------------------------------
// The stops for what's on screen
// ---------------------------------------------------------------------------

void UMainMenuWidget::RebuildNavigation(const UWidget* Prefer)
{
	TArray<FMenuNavRow> Rows;
	if (IsPopupOpen())
	{
		Rows = PopupNav;
	}
	else if (bPickerShown)
	{
		// Card by card, top to bottom: Continue (or New Game), then its Delete; Back last.
		for (int32 Index = 0; Index < PlayButtons.Num(); ++Index)
		{
			if (PlayButtons[Index])
			{
				Rows.Add(FMenuNavRow{ FMenuNavItem::MakeButton(PlayButtons[Index]) });
			}
			if (DeleteButtons.IsValidIndex(Index) && DeleteButtons[Index])
			{
				Rows.Add(FMenuNavRow{ FMenuNavItem::MakeButton(DeleteButtons[Index]) });
			}
		}
		Rows.Add(FMenuNavRow{ FMenuNavItem::MakeButton(PickerBackButton) });
	}
	else
	{
		for (ULooterButton* Button : MainNavButtons)
		{
			Rows.Add(FMenuNavRow{ FMenuNavItem::MakeButton(Button) });
		}
	}
	// A few stops each: round the ends, as the bumpers' long settings list doesn't.
	Nav.SetWrap(true);
	Nav.SetRows(MoveTemp(Rows), Prefer);
	RefreshFooter();
}

void UMainMenuWidget::OpenPopupNav(TArray<FMenuNavRow> Rows, const UWidget* Prefer)
{
	PopupReturn = Nav.GetFocusedWidget();
	PopupNav = MoveTemp(Rows);
	RebuildNavigation(Prefer);
}

void UMainMenuWidget::ClosePopupNav()
{
	if (PopupNav.IsEmpty())
	{
		return;
	}
	PopupNav.Reset();
	// Back on what opened it (the card's Delete).
	RebuildNavigation(PopupReturn.Get());
	PopupReturn.Reset();
}

bool UMainMenuWidget::StepBack()
{
	// One step at a time: the popup, the hero select, then the picker. On the main buttons nothing (the game is left with
	// its own button, never by a stray key), and nothing while a session's level opens.
	if (bLoading)
	{
		return false;
	}
	if (DeleteIndex != INDEX_NONE)
	{
		CloseDeleteConfirm();
		return true;
	}
	if (bHeroSelectShown)
	{
		CloseHeroSelect();
		return true;
	}
	if (bPickerShown)
	{
		ShowMainButtons();
		return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
// Keys and the stick
// ---------------------------------------------------------------------------

bool UMainMenuWidget::HandleNavKey(const FKey& Key, bool bRepeat)
{
	// The settings menu over this one takes its own keys; these are for when it's closed.
	if (const AMainMenuHUD* HUD = GetMenuHUD(); HUD && HUD->IsSettingsOpen())
	{
		return false;
	}
	// The hero select over the whole screen has its own stops (its B comes back here through CloseHeroSelect).
	if (bHeroSelectShown && HeroSelect)
	{
		return HeroSelect->HandleNavKey(Key, bRepeat) || bLoading;
	}
	if (IsBack(Key))
	{
		// B shows the controller's prompts; Esc leaves a mouse player's menu as it is.
		if (Key.IsGamepadKey() && LooterUI::NoteMenuKey(Key))
		{
			RefreshNavLook();
		}
		if (!bRepeat)
		{
			StopNavHold();
			if (StepBack())
			{
				LooterSound::Play2D(this, LooterSoundCue::Back);
			}
		}
		return true;
	}
	if (IsStickKey(Key))
	{
		return true;
	}
	const EMove Move = MoveOf(Key);
	const bool bAccept = IsAccept(Key);
	if (Move == EMove::None && !bAccept)
	{
		return false;
	}
	// While a session's level opens the menu takes no more; held keys repeat on the menu's own clock.
	if (bRepeat || bLoading)
	{
		return true;
	}
	// The first move or press after the mouse only shows where the focus is.
	if (!RevealNav(Key))
	{
		return true;
	}
	if (Move != EMove::None)
	{
		PressNav(Key, Move);
	}
	else
	{
		StopNavHold();
		// The button's own click (its sound and HandleButton); it may open the picker, a popup or the settings.
		Nav.Activate();
		RefreshFooter();
	}
	return true;
}

void UMainMenuWidget::HandleNavKeyUp(const FKey& Key)
{
	NavRepeat.Release(Key);
	if (HeroSelect)
	{
		HeroSelect->HandleNavKeyUp(Key);
	}
}

void UMainMenuWidget::HandleNavStick(const FVector2D& Stick)
{
	if (bHeroSelectShown && HeroSelect)
	{
		HeroSelect->HandleNavStick(Stick);
		return;
	}
	const EMove Next = StickMove(Stick, NavStickMove);
	if (Next == NavStickMove)
	{
		return;
	}
	const EMove Before = NavStickMove;
	NavStickMove = Next;
	if (Before != EMove::None)
	{
		HandleNavKeyUp(StickKey(Before));
	}
	const AMainMenuHUD* HUD = GetMenuHUD();
	if (Next != EMove::None && !bLoading && !(HUD && HUD->IsSettingsOpen()))
	{
		const FKey Key = StickKey(Next);
		if (RevealNav(Key))
		{
			PressNav(Key, Next);
		}
	}
}

bool UMainMenuWidget::RevealNav(const FKey& Key)
{
	const bool bWasShown = LooterUI::IsMenuFocusShown();
	if (LooterUI::NoteMenuKey(Key))
	{
		RefreshNavLook();
	}
	return bWasShown;
}

void UMainMenuWidget::PressNav(const FKey& Key, EMove Move)
{
	StepNav(Move);
	NavRepeat.Press(Key, Move, false);
}

void UMainMenuWidget::StepNav(EMove Move)
{
	const int32 Vertical = VerticalStep(Move);
	const EMenuNavResult Result = Vertical != 0 ? Nav.MoveVertical(Vertical) : Nav.MoveHorizontal(HorizontalStep(Move));
	if (Result == EMenuNavResult::Moved)
	{
		LooterSound::Play2D(this, LooterSoundCue::Hover);
		RefreshFooter();
	}
}

void UMainMenuWidget::AdvanceNav(float DeltaSeconds)
{
	// The settings menu over this one may have seen the device change.
	if (!bNavLookApplied || LooterUI::GetMenuDevice() != ShownDevice)
	{
		RefreshNavLook();
	}
	for (int32 Due = NavRepeat.Tick(DeltaSeconds); Due > 0 && NavRepeat.IsHeld(); --Due)
	{
		StepNav(NavRepeat.GetMove());
	}
	Nav.Tick(DeltaSeconds);
	// The hero select runs on this menu's clock (its held keys, its stage, its confirm's beat).
	if (bHeroSelectShown && HeroSelect)
	{
		HeroSelect->AdvanceNav(DeltaSeconds);
	}
}

void UMainMenuWidget::StopNavHold()
{
	NavRepeat.Stop();
	Nav.EndSliderHold();
}

void UMainMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AdvanceNav(InDeltaTime);
	// The arrival, the lit buttons, the ember's breath (MainMenuMotion.cpp), after the focus moved this frame.
	AdvanceLook(InDeltaTime);
}

FReply UMainMenuWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	HandleNavKeyUp(InKeyEvent.GetKey());
	return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}

FReply UMainMenuWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent)
{
	const FKey Key = InAnalogEvent.GetKey();
	if (Key == EKeys::Gamepad_LeftX || Key == EKeys::Gamepad_LeftY)
	{
		(Key == EKeys::Gamepad_LeftX ? NavStick.X : NavStick.Y) = InAnalogEvent.GetAnalogValue();
		HandleNavStick(NavStick);
		// Taken, so Slate makes no navigation of its own from the stick.
		return FReply::Handled();
	}
	// The right stick turns the hero on the hero select's stand.
	if (Key == EKeys::Gamepad_RightX && bHeroSelectShown && HeroSelect)
	{
		HeroSelect->HandleTurnStick(InAnalogEvent.GetAnalogValue());
		return FReply::Handled();
	}
	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
}

FNavigationReply UMainMenuWidget::NativeOnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent,
	const FNavigationReply& InDefaultReply)
{
	// The menu moves its own focus: Slate's must never carry the keyboard off it.
	return FNavigationReply::Stop();
}

void UMainMenuWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	StopNavHold();
	NavStick = FVector2D::ZeroVector;
	NavStickMove = EMove::None;
	Super::NativeOnFocusLost(InFocusEvent);
}

// ---------------------------------------------------------------------------
// The mouse
// ---------------------------------------------------------------------------

void UMainMenuWidget::NotePointer()
{
	if (LooterUI::NoteMenuPointer())
	{
		StopNavHold();
		RefreshNavLook();
	}
}

void UMainMenuWidget::HandleButtonHovered(ULooterButton* Button)
{
	// Only while the mouse is in charge (a layout change under a still pointer mustn't steal the keys' focus).
	if (!LooterUI::IsMenuFocusShown() && Nav.FocusOn(Button))
	{
		RefreshFooter();
	}
}

FReply UMainMenuWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	NotePointer();
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UMainMenuWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// A real move: Slate's own made-up moves have no distance.
	const FVector2D Delta = InMouseEvent.GetCursorDelta();
	if (Delta.SizeSquared() > 1.0)
	{
		NotePointer();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UMainMenuWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	NotePointer();
	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

FCursorReply UMainMenuWidget::NativeOnCursorQuery(const FGeometry& InGeometry, const FPointerEvent& InCursorEvent)
{
	// The pointer hides while keys or a controller drive the menu, and is back with the mouse's first move.
	if (Nav.IsShown())
	{
		return FCursorReply::Cursor(EMouseCursor::None);
	}
	return Super::NativeOnCursorQuery(InGeometry, InCursorEvent);
}

// ---------------------------------------------------------------------------
// The look and the key hints
// ---------------------------------------------------------------------------

UWidget* UMainMenuWidget::GetFocusedWidget() const
{
	if (bHeroSelectShown && HeroSelect)
	{
		return HeroSelect->GetFocusedWidget();
	}
	return Nav.GetFocusedWidget();
}

bool UMainMenuWidget::IsFocusLookShown() const
{
	if (bHeroSelectShown)
	{
		return LooterUI::IsMenuFocusShown();
	}
	const FMenuNavItem* Here = Nav.GetFocused();
	return Nav.IsShown() && Here && LooterUI::IsFocusLit(Here->Light.Get());
}

void UMainMenuWidget::RefreshNavLook()
{
	ShownDevice = LooterUI::GetMenuDevice();
	bNavLookApplied = true;
	Nav.SetShown(LooterUI::IsMenuFocusShown());
	if (bHeroSelectShown && HeroSelect)
	{
		HeroSelect->RefreshNavLook();
	}
	RefreshFooter();
}

TArray<FPrompt> UMainMenuWidget::GetPrompts() const
{
	TArray<FPrompt> Prompts;
	// A session's level opening: nothing more to press.
	if (bLoading)
	{
		return Prompts;
	}
	// The hero select has its own hints, along its own bottom.
	if (bHeroSelectShown && HeroSelect)
	{
		return HeroSelect->GetPrompts();
	}
	const bool bPad = LooterUI::IsMenuGamepad();
	Prompts.Add(MakePrompt(EPromptKey::Select, bPad, TEXT("Select")));
	if (IsPopupOpen())
	{
		Prompts.Add(MakePrompt(EPromptKey::Back, bPad, TEXT("Cancel")));
	}
	else if (bPickerShown)
	{
		Prompts.Add(MakePrompt(EPromptKey::Back, bPad, TEXT("Back")));
	}
	return Prompts;
}

void UMainMenuWidget::RefreshFooter()
{
	if (!FooterPrompts || !WidgetTree)
	{
		return;
	}
	const TArray<FPrompt> Prompts = GetPrompts();
	const FString Line = PromptsText(Prompts);
	if (Line == ShownPrompts)
	{
		return;
	}
	ShownPrompts = Line;
	// The inventory's key hints (a cap and its words; the first, the main one, lit).
	FooterPrompts->ClearChildren();
	for (int32 Index = 0; Index < Prompts.Num(); ++Index)
	{
		UHorizontalBoxSlot* HintSlot = FooterPrompts->AddChildToHorizontalBox(LoadoutParts::MakeKeyHint(WidgetTree, Prompts[Index].Key, Prompts[Index].Text, Index == 0));
		HintSlot->SetVerticalAlignment(VAlign_Center);
		HintSlot->SetPadding(FMargin(Index > 0 ? PromptGap : 0.f, 0.f, 0.f, 0.f));
	}
}

// UPauseMenuWidget for the keyboard, a controller and the mouse, the settings menu's way (MenuNav.h, MenuNavigator.h,
// LooterUIFocus.h): up and down round the ends (four options), A or Enter picks, B, Esc or Start resume; the first key
// after the mouse only brings the focus look up; the key hints bottom-right follow the device last used.

#include "UI/Menus/PauseMenuWidget.h"
#include "UI/Cards/LooterHollowKey.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIFocus.h"
#include "UI/Style/LooterUIStyle.h"
#include "Audio/LooterSound.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScaleBox.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"

using namespace MenuNav;

namespace
{
	/**
	 * The key hints (e.js keysHTML, common.css .hint/.cap): 34 between hints, 12 between a key and its word, the words 26 px
	 * bold spaced .08 em. The kit's hollow key is drawn 1.45 times its size, as big as the mockup's cap (40 tall).
	 */
	constexpr float PromptGap = 34.f;
	constexpr float PromptKeyGap = 12.f;
	constexpr int32 PromptWordSize = 26;
	constexpr int32 PromptWordSpacing = 80;
	constexpr float PromptKeyScale = 1.45f;
}

// ---------------------------------------------------------------------------
// The stops
// ---------------------------------------------------------------------------

void UPauseMenuWidget::RebuildNavigation(const UWidget* Prefer)
{
	// One stop an option, top to bottom. Each lights an empty box as its focus look: the option marked (gold, slid out,
	// the needle on it) is the focus, so the kit's outline round the tag's box isn't wanted.
	TArray<FMenuNavRow> Rows;
	for (const FPauseMenuOption& Option : Options)
	{
		if (ULooterButton* Button = Option.Button.Get())
		{
			FMenuNavItem Item = FMenuNavItem::MakeButton(Button);
			if (UWidget* Light = Option.FocusLight.Get())
			{
				Item.Light = Light;
			}
			Rows.Add(FMenuNavRow{ Item });
		}
	}
	// A short list: round the ends, as the main menu's.
	Nav.SetWrap(true);
	Nav.SetRows(MoveTemp(Rows), Prefer);
	RefreshPrompts();
}

UWidget* UPauseMenuWidget::GetFocusedWidget() const
{
	return Nav.GetFocusedWidget();
}

EPauseMenuItem UPauseMenuWidget::GetFocusedItem() const
{
	const FPauseMenuOption* Found = FindOption(Cast<ULooterButton>(Nav.GetFocusedWidget()));
	return Found ? Found->Item : EPauseMenuItem::Count;
}

bool UPauseMenuWidget::IsFocusLookShown() const
{
	const FMenuNavItem* Here = Nav.GetFocused();
	return Nav.IsShown() && Here && LooterUI::IsFocusLit(Here->Light.Get());
}

// ---------------------------------------------------------------------------
// Keys and the stick
// ---------------------------------------------------------------------------

bool UPauseMenuWidget::HandleNavKey(const FKey& Key, bool bRepeat)
{
	if (bLeaving)
	{
		return true;
	}
	if (IsBack(Key) || IsMenuButton(Key))
	{
		// B and Start show the controller's prompts; Esc leaves a mouse player's menu as it is.
		if (Key.IsGamepadKey() && LooterUI::NoteMenuKey(Key))
		{
			RefreshNavLook();
		}
		// The Esc or Start that opened it (or backed out of the settings into it), still held and repeating, mustn't resume.
		if (!bRepeat)
		{
			// Backing out of the list is going back to the game (the HUD sounds the close).
			Choose(EPauseMenuItem::Resume);
		}
		return true;
	}
	// The stick's own direction keys: the stick is read from its axes (HandleNavStick), never twice.
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
	// Held keys repeat on the menu's own clock (AdvanceNav), the same for every keyboard and pad.
	if (bRepeat)
	{
		return true;
	}
	// The first move or press after the mouse only shows where the focus is.
	if (!RevealNav(Key))
	{
		return true;
	}
	if (VerticalStep(Move) != 0)
	{
		PressNav(Key, Move);
	}
	else if (bAccept)
	{
		ActivateNav();
	}
	// Left and right: the list has one stop a row, so they're taken and do nothing.
	return true;
}

void UPauseMenuWidget::HandleNavKeyUp(const FKey& Key)
{
	NavRepeat.Release(Key);
}

void UPauseMenuWidget::HandleNavStick(const FVector2D& Stick)
{
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
	if (Next != EMove::None && VerticalStep(Next) != 0 && !bLeaving)
	{
		const FKey Key = StickKey(Next);
		if (RevealNav(Key))
		{
			PressNav(Key, Next);
		}
	}
}

bool UPauseMenuWidget::RevealNav(const FKey& Key)
{
	const bool bWasShown = LooterUI::IsMenuFocusShown();
	if (LooterUI::NoteMenuKey(Key))
	{
		RefreshNavLook();
	}
	return bWasShown;
}

void UPauseMenuWidget::PressNav(const FKey& Key, EMove Move)
{
	StepNav(Move);
	NavRepeat.Press(Key, Move, /*bAccelerate*/ false);
}

void UPauseMenuWidget::StepNav(EMove Move)
{
	if (Nav.MoveVertical(VerticalStep(Move)) == EMenuNavResult::Moved)
	{
		LooterSound::Play2D(this, LooterSoundCue::Hover);
		RefreshPrompts();
	}
}

void UPauseMenuWidget::ActivateNav()
{
	StopNavHold();
	// The button's own click: its sound, then HandleButton (which may take the list off screen).
	Nav.Activate();
}

void UPauseMenuWidget::AdvanceNav(float DeltaSeconds)
{
	// Another menu (the settings, a page) may have seen the device change while this one was away.
	if (!bNavLookApplied || LooterUI::GetMenuDevice() != ShownDevice)
	{
		RefreshNavLook();
	}
	for (int32 Due = NavRepeat.Tick(DeltaSeconds); Due > 0 && NavRepeat.IsHeld(); --Due)
	{
		StepNav(NavRepeat.GetMove());
	}
	Nav.Tick(DeltaSeconds);
}

void UPauseMenuWidget::StopNavHold()
{
	NavRepeat.Stop();
	Nav.EndSliderHold();
}

// ---------------------------------------------------------------------------
// The mouse
// ---------------------------------------------------------------------------

void UPauseMenuWidget::NotePointer()
{
	if (LooterUI::NoteMenuPointer())
	{
		StopNavHold();
		RefreshNavLook();
	}
}

void UPauseMenuWidget::HandleButtonHovered(ULooterButton* Button)
{
	// Only while the mouse is in charge: the focus follows the pointer, so keys carry on from the row it's on.
	if (!LooterUI::IsMenuFocusShown() && Nav.FocusOn(Button))
	{
		RefreshPrompts();
	}
}

FReply UPauseMenuWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	return HandleNavKey(InKeyEvent.GetKey(), InKeyEvent.IsRepeat()) ? FReply::Handled() : Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UPauseMenuWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	HandleNavKeyUp(InKeyEvent.GetKey());
	return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}

FReply UPauseMenuWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent)
{
	const FKey Key = InAnalogEvent.GetKey();
	if (Key == EKeys::Gamepad_LeftX || Key == EKeys::Gamepad_LeftY)
	{
		(Key == EKeys::Gamepad_LeftX ? NavStick.X : NavStick.Y) = InAnalogEvent.GetAnalogValue();
		HandleNavStick(NavStick);
		// Taken, so Slate makes no navigation of its own from the stick (it would carry the keyboard focus out of the menu).
		return FReply::Handled();
	}
	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
}

FReply UPauseMenuWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	NotePointer();
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UPauseMenuWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// A real move: Slate's own made-up moves have no distance.
	if (InMouseEvent.GetCursorDelta().SizeSquared() > 1.0)
	{
		NotePointer();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FCursorReply UPauseMenuWidget::NativeOnCursorQuery(const FGeometry& InGeometry, const FPointerEvent& InCursorEvent)
{
	// The pointer hides while keys or a controller drive the menu, and is back with the mouse's first move.
	if (Nav.IsShown())
	{
		return FCursorReply::Cursor(EMouseCursor::None);
	}
	return Super::NativeOnCursorQuery(InGeometry, InCursorEvent);
}

FNavigationReply UPauseMenuWidget::NativeOnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent,
	const FNavigationReply& InDefaultReply)
{
	// The menu moves its own focus: Slate's (Tab, an unhandled stick) must never carry the keyboard out of it, to the game.
	return FNavigationReply::Stop();
}

void UPauseMenuWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	// Whatever takes the keys now (the settings, a page) gets their let-go: nothing may keep repeating here.
	StopNavHold();
	NavStick = FVector2D::ZeroVector;
	NavStickMove = EMove::None;
	Super::NativeOnFocusLost(InFocusEvent);
}

// ---------------------------------------------------------------------------
// The look and the prompts
// ---------------------------------------------------------------------------

void UPauseMenuWidget::RefreshNavLook()
{
	ShownDevice = LooterUI::GetMenuDevice();
	bNavLookApplied = true;
	Nav.SetShown(LooterUI::IsMenuFocusShown());
	RefreshPrompts();
}

TArray<FPrompt> UPauseMenuWidget::GetPrompts() const
{
	const bool bPad = LooterUI::IsMenuGamepad();
	TArray<FPrompt> Prompts;
	Prompts.Add(MakePrompt(EPromptKey::Select, bPad, TEXT("Select")));
	Prompts.Add(MakePrompt(EPromptKey::Back, bPad, TEXT("Resume")));
	return Prompts;
}

void UPauseMenuWidget::RefreshPrompts()
{
	if (!PromptBar || !WidgetTree)
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
	// Bottom-right, floating: each a hollow gold key (the kit's, drawn the mockup's size) and its word, spaced.
	PromptBar->ClearChildren();
	for (int32 Index = 0; Index < Prompts.Num(); ++Index)
	{
		ULooterHollowKey* Key = WidgetTree->ConstructWidget<ULooterHollowKey>(ULooterHollowKey::StaticClass());
		Key->SetLabel(Prompts[Index].Key.ToUpper());
		Key->SetVisibility(ESlateVisibility::HitTestInvisible);
		UScaleBox* KeySize = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
		KeySize->SetStretch(EStretch::UserSpecified);
		KeySize->SetUserSpecifiedScale(PromptKeyScale);
		KeySize->SetContent(Key);
		KeySize->SetVisibility(ESlateVisibility::HitTestInvisible);
		UHorizontalBoxSlot* KeySlot = PromptBar->AddChildToHorizontalBox(KeySize);
		KeySlot->SetVerticalAlignment(VAlign_Center);
		KeySlot->SetPadding(FMargin(Index > 0 ? PromptGap : 0.f, 0.f, 0.f, 0.f));
		UTextBlock* Word = LooterUI::MakeFloatingText(WidgetTree, PromptWordSize, LooterUI::Color::SlateText(), PromptWordSpacing);
		Word->SetText(FText::FromString(Prompts[Index].Text.ToUpper()));
		UHorizontalBoxSlot* WordSlot = PromptBar->AddChildToHorizontalBox(Word);
		WordSlot->SetVerticalAlignment(VAlign_Center);
		WordSlot->SetPadding(FMargin(PromptKeyGap, 0.f, 0.f, 0.f));
	}
}

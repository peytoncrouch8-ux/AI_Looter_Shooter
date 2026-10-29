// USettingsMenuWidget for the keyboard and a controller, Borderlands' way: the focus moves down the page, steps sliders
// (held, faster and faster) and switches, presses buttons, turns the tabs on the bumpers and backs out on B; the focus
// look and the prompts along the bottom follow the device last used (LooterUIFocus.h, MenuNav.h, MenuNavigator.h).

#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/SettingsMenuHelp.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Cards/LooterHollowKey.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIFocus.h"
#include "UI/Style/LooterUIStyle.h"
#include "Audio/LooterSound.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"

using namespace MenuNav;
using namespace SettingsMenu;

namespace
{
	/** The gap between the prompts under the plate, between a key and its word, and the word's size and spacing. */
	constexpr float PromptGap = 30.f;
	constexpr float PromptKeyGap = 10.f;
	constexpr int32 PromptWordSize = 18;
	constexpr int32 PromptWordSpacing = 111;
	/** Room kept round a stop scrolled into view, so the rows beside it show too. */
	constexpr float ScrollPadding = 24.f;
	/** A slider's tick per step (the volume sliders tick at their own new level instead). */
	constexpr float StepTickVolume = 0.6f;
}

ULooterButton* USettingsMenuWidget::GetResumeButton() const
{
	return ResumeButton;
}

USlider* USettingsMenuWidget::GetMinimapSizeSlider() const
{
	return MinimapSlider;
}

// ---------------------------------------------------------------------------
// The stops (each page's are noted as it's built: NavSlider, NavChoices, NavButton in SettingsMenuTabs.cpp)
// ---------------------------------------------------------------------------

void USettingsMenuWidget::RebuildNavigation(const UWidget* Prefer)
{
	if (!ResumeButton)
	{
		return;
	}
	// Top to bottom: Resume (over the game), the page on show (on Controls, its key list after its sliders), then Save &
	// Quit or Back (the one the mode shows; the focus passes over what's hidden). Only the page on show: the others' rows
	// aren't on screen to move to.
	TArray<FMenuNavRow> Rows;
	Rows.Add(FMenuNavRow{ FMenuNavItem::MakeButton(ResumeButton) });
	if (PageNav.IsValidIndex(CurrentPage))
	{
		Rows.Append(PageNav[CurrentPage]);
	}
	if (CurrentPage == static_cast<int32>(ESettingsPage::Controls))
	{
		Rows.Append(KeyNav);
	}
	FMenuNavRow Footer;
	for (ULooterButton* Button : { SaveQuitButton.Get(), BackButton.Get() })
	{
		if (Button)
		{
			Footer.Add(FMenuNavItem::MakeButton(Button, NumPages));
		}
	}
	if (!Footer.IsEmpty())
	{
		Rows.Add(MoveTemp(Footer));
	}
	// A long page: the ends stop the focus rather than throw it to the far end.
	Nav.SetWrap(false);
	Nav.SetRows(MoveTemp(Rows), Prefer);
	RefreshPrompts();
}

UWidget* USettingsMenuWidget::FirstPageStop() const
{
	auto FirstOf = [](const TArray<FMenuNavRow>& Rows) -> UWidget*
	{
		for (const FMenuNavRow& Row : Rows)
		{
			for (const FMenuNavItem& Item : Row)
			{
				if (Item.IsAvailable())
				{
					return Item.GetWidget();
				}
			}
		}
		return nullptr;
	};
	UWidget* First = PageNav.IsValidIndex(CurrentPage) ? FirstOf(PageNav[CurrentPage]) : nullptr;
	if (!First && CurrentPage == static_cast<int32>(ESettingsPage::Controls))
	{
		First = FirstOf(KeyNav);
	}
	return First;
}

void USettingsMenuWidget::ResetNavigation()
{
	// Over the game the focus starts on Resume (A at once resumes); from the main menu on the page's first setting.
	RebuildNavigation(Mode == ESettingsMenuMode::Pause ? static_cast<UWidget*>(ResumeButton.Get()) : FirstPageStop());
}

// ---------------------------------------------------------------------------
// Keys and the stick
// ---------------------------------------------------------------------------

bool USettingsMenuWidget::HandleNavKey(const FKey& Key, bool bRepeat)
{
	if (IsBack(Key) || IsMenuButton(Key))
	{
		// B and Start show the controller's prompts; Esc leaves a mouse player's menu as it is (no focus look for backing out).
		if (Key.IsGamepadKey() && LooterUI::NoteMenuKey(Key))
		{
			RefreshNavLook();
		}
		// The Esc or Start that opened the menu, still held and repeating, mustn't close it again.
		if (bRepeat)
		{
			return true;
		}
		NavRepeat.Stop();
		// A field of view or zoom slider held (by the mouse, or a key or stick): back as it was, before anything closes.
		if (CancelPreviewDrag())
		{
			Nav.EndSliderHold();
			LooterSound::Play2D(this, LooterSoundCue::Back);
			RefreshPrompts();
			return true;
		}
		Nav.EndSliderHold();
		// The pause menu's close is the HUD's to sound; the main menu has no HUD to.
		if (Mode == ESettingsMenuMode::MainMenu)
		{
			LooterSound::Play2D(this, LooterSoundCue::Close);
		}
		OnClose.ExecuteIfBound();
		return true;
	}
	// The stick's own direction keys: the stick is read from its axes (HandleNavStick), never twice.
	if (IsStickKey(Key))
	{
		return true;
	}
	const EMove Move = MoveOf(Key);
	const int32 SectionStep = SectionStepOf(Key);
	const bool bAccept = IsAccept(Key);
	if (Move == EMove::None && SectionStep == 0 && !bAccept)
	{
		return false;
	}
	// Held keys repeat on the menu's own clock (AdvanceNav), the same for every keyboard and pad.
	if (bRepeat)
	{
		return true;
	}
	// The first move or press after the mouse only shows where the focus is (nothing moves or happens under a hidden focus).
	if (!RevealNav(Key) && SectionStep == 0)
	{
		return true;
	}
	if (Move != EMove::None)
	{
		PressNav(Key, Move);
	}
	else if (SectionStep != 0)
	{
		StepPage(SectionStep);
	}
	else
	{
		ActivateNav();
	}
	return true;
}

void USettingsMenuWidget::HandleNavKeyUp(const FKey& Key)
{
	// Let go: the repeats stop and a slider the key held is let go (saved, and its preview lingers a moment, as after a drag).
	if (NavRepeat.Release(Key))
	{
		Nav.EndSliderHold();
	}
}

void USettingsMenuWidget::HandleNavStick(const FVector2D& Stick)
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
	if (Next != EMove::None && ListeningIndex == INDEX_NONE)
	{
		const FKey Key = StickKey(Next);
		if (RevealNav(Key))
		{
			PressNav(Key, Next);
		}
	}
}

bool USettingsMenuWidget::RevealNav(const FKey& Key)
{
	const bool bWasShown = LooterUI::IsMenuFocusShown();
	if (LooterUI::NoteMenuKey(Key))
	{
		RefreshNavLook();
	}
	return bWasShown;
}

void USettingsMenuWidget::PressNav(const FKey& Key, EMove Move)
{
	StepNav(Move);
	// A slider held left or right speeds up the longer it's held; moving through the list keeps an even pace.
	const FMenuNavItem* Here = Nav.GetFocused();
	NavRepeat.Press(Key, Move, HorizontalStep(Move) != 0 && Here && Here->IsSlider());
}

void USettingsMenuWidget::StepNav(EMove Move)
{
	const int32 Vertical = VerticalStep(Move);
	if (Vertical != 0)
	{
		Nav.EndSliderHold();
		if (Nav.MoveVertical(Vertical) == EMenuNavResult::Moved)
		{
			LooterSound::Play2D(this, LooterSoundCue::Hover);
			ScrollToFocus();
			RefreshPrompts();
		}
		return;
	}
	switch (Nav.MoveHorizontal(HorizontalStep(Move)))
	{
	case EMenuNavResult::Moved:
		LooterSound::Play2D(this, LooterSoundCue::Hover);
		ScrollToFocus();
		RefreshPrompts();
		break;
	case EMenuNavResult::Stepped:
		// The volume sliders tick at their own new level (ChangeVolume); the others tick here, a soft click a step.
		if (!VolumeSliders.Contains(Nav.GetHeldSlider()))
		{
			LooterSound::Play2D(this, LooterSoundCue::Hover, StepTickVolume);
		}
		break;
	default:
		// A choice picked has sounded its own click; an edge stays quiet.
		break;
	}
}

void USettingsMenuWidget::ActivateNav()
{
	StopNavHold();
	// The button's own click (its sound and HandleButton); it may close the menu or start waiting for a key.
	Nav.Activate();
	RefreshPrompts();
}

void USettingsMenuWidget::AdvanceNav(float DeltaSeconds)
{
	// Another menu (the main menu under this one) may have seen the device change.
	if (!bNavLookApplied || LooterUI::GetMenuDevice() != ShownDevice)
	{
		RefreshNavLook();
	}
	for (int32 Due = NavRepeat.Tick(DeltaSeconds); Due > 0 && NavRepeat.IsHeld(); --Due)
	{
		StepNav(NavRepeat.GetMove());
	}
	Nav.Tick(DeltaSeconds);
	AdvanceHoverLook();
	// The description column follows the focus or the pointer (SettingsMenuTabs.cpp).
	AdvanceHelp();
}

void USettingsMenuWidget::StopNavHold()
{
	NavRepeat.Stop();
	Nav.EndSliderHold();
}

// ---------------------------------------------------------------------------
// The mouse
// ---------------------------------------------------------------------------

void USettingsMenuWidget::NotePointer()
{
	if (LooterUI::NoteMenuPointer())
	{
		StopNavHold();
		RefreshNavLook();
	}
}

void USettingsMenuWidget::HandleButtonHovered(ULooterButton* Button)
{
	// Only while the mouse is in charge: a list scrolled by keys slides buttons under a still pointer, which mustn't steal
	// the focus.
	if (!LooterUI::IsMenuFocusShown() && Nav.FocusOn(Button))
	{
		RefreshPrompts();
	}
}

FReply USettingsMenuWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// A real move: Slate's own made-up moves (the list scrolling under a still pointer) have no distance.
	const FVector2D Delta = InMouseEvent.GetCursorDelta();
	if (Delta.SizeSquared() > 1.0)
	{
		NotePointer();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FCursorReply USettingsMenuWidget::NativeOnCursorQuery(const FGeometry& InGeometry, const FPointerEvent& InCursorEvent)
{
	// The pointer hides while keys or a controller drive the menu, and is back with the mouse's first move.
	if (Nav.IsShown())
	{
		return FCursorReply::Cursor(EMouseCursor::None);
	}
	return Super::NativeOnCursorQuery(InGeometry, InCursorEvent);
}

FReply USettingsMenuWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	HandleNavKeyUp(InKeyEvent.GetKey());
	return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}

FReply USettingsMenuWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent)
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

FNavigationReply USettingsMenuWidget::NativeOnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent,
	const FNavigationReply& InDefaultReply)
{
	// The menu moves its own focus: Slate's (Tab, an unhandled stick) must never carry the keyboard out of it, to the
	// main menu under it or the game.
	return FNavigationReply::Stop();
}

void USettingsMenuWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	// Whatever takes the keys now gets their let-go: nothing may keep repeating here.
	StopNavHold();
	NavStick = FVector2D::ZeroVector;
	NavStickMove = EMove::None;
	Super::NativeOnFocusLost(InFocusEvent);
}

// ---------------------------------------------------------------------------
// The look
// ---------------------------------------------------------------------------

UWidget* USettingsMenuWidget::GetFocusedWidget() const
{
	return Nav.GetFocusedWidget();
}

bool USettingsMenuWidget::IsFocusLookShown() const
{
	const FMenuNavItem* Here = Nav.GetFocused();
	return Nav.IsShown() && Here && LooterUI::IsFocusLit(Here->Light.Get());
}

void USettingsMenuWidget::RefreshNavLook()
{
	ShownDevice = LooterUI::GetMenuDevice();
	bNavLookApplied = true;
	Nav.SetShown(LooterUI::IsMenuFocusShown());
	RefreshTabKeys();
	if (KeyHintText)
	{
		// The list is the keyboard's and the mouse's; a controller keeps its own buttons.
		KeyHintText->SetText(FText::FromString(LooterUI::IsMenuGamepad()
			? TEXT("These are the keyboard and mouse keys (a controller keeps its own). Press A on one, then the new key or mouse button.")
			: TEXT("Click a key (or press Enter on it) to change it, then press the new key or mouse button. Esc cancels.")));
	}
	RefreshPrompts();
}

TArray<FPrompt> USettingsMenuWidget::GetPrompts() const
{
	const bool bPad = LooterUI::IsMenuGamepad();
	TArray<FPrompt> Prompts;
	if (ListeningIndex != INDEX_NONE)
	{
		Prompts.Add(MakePrompt(EPromptKey::Back, bPad, TEXT("Cancel")));
		return Prompts;
	}
	// What the stop with the focus takes first, then the sections and the way out.
	if (const FMenuNavItem* Here = Nav.GetFocused())
	{
		if (Here->IsSlider())
		{
			Prompts.Add(MakePrompt(EPromptKey::Adjust, bPad, TEXT("Adjust")));
		}
		else if (Here->IsChoices())
		{
			Prompts.Add(MakePrompt(EPromptKey::Select, bPad, TEXT("Change")));
		}
		else
		{
			Prompts.Add(MakePrompt(EPromptKey::Select, bPad, KeyButtons.Contains(Here->Button.Get()) ? TEXT("Rebind") : TEXT("Select")));
		}
	}
	Prompts.Add(MakePrompt(EPromptKey::Sections, bPad, TEXT("Tabs")));
	Prompts.Add(MakePrompt(EPromptKey::Back, bPad, Mode == ESettingsMenuMode::Pause ? TEXT("Resume") : TEXT("Back")));
	return Prompts;
}

void USettingsMenuWidget::RefreshPrompts()
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
	// Under the plate, floating: each a hollow key (the user's pick, 2026-10-28) and its word, spaced (c1/c3: 18 px).
	PromptBar->ClearChildren();
	for (int32 Index = 0; Index < Prompts.Num(); ++Index)
	{
		ULooterHollowKey* Key = WidgetTree->ConstructWidget<ULooterHollowKey>(ULooterHollowKey::StaticClass());
		Key->SetLabel(Prompts[Index].Key.ToUpper());
		Key->SetVisibility(ESlateVisibility::HitTestInvisible);
		UHorizontalBoxSlot* KeySlot = PromptBar->AddChildToHorizontalBox(Key);
		KeySlot->SetVerticalAlignment(VAlign_Center);
		KeySlot->SetPadding(FMargin(Index > 0 ? PromptGap : 0.f, 0.f, 0.f, 0.f));
		UTextBlock* Word = LooterUI::MakeFloatingText(WidgetTree, PromptWordSize, LooterUI::Color::SlateText(), PromptWordSpacing);
		Word->SetText(FText::FromString(Prompts[Index].Text.ToUpper()));
		UHorizontalBoxSlot* WordSlot = PromptBar->AddChildToHorizontalBox(Word);
		WordSlot->SetVerticalAlignment(VAlign_Center);
		WordSlot->SetPadding(FMargin(PromptKeyGap, 0.f, 0.f, 0.f));
	}
}

void USettingsMenuWidget::ScrollToFocus()
{
	const FMenuNavItem* Here = Nav.GetFocused();
	if (!Body || !Here)
	{
		return;
	}
	if (UWidget* Row = Here->Row.Get())
	{
		Body->ScrollWidgetIntoView(Row, true, EDescendantScrollDestination::IntoView, ScrollPadding);
	}
	else if (Here->Button.Get() == ResumeButton)
	{
		// Resume sits over the list: the list goes back to its top under it.
		Body->ScrollToStart();
	}
}

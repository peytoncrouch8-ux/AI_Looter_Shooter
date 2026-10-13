// UMissionOfferWidget for the keyboard, a controller and the mouse: E takes the mission, Esc or B puts it back; left and
// right move the focus between the two buttons and Enter, Space or A presses the one focused; the prompts at the bottom
// name the keys of the device last used (LooterUIFocus.h, MenuNav.h, MenuNavigator.h).

#include "UI/World/MissionOfferWidget.h"
#include "Audio/LooterSound.h"
#include "Settings/KeyBindingSubsystem.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "InputCoreTypes.h"

using namespace MenuNav;

namespace
{
	/** The gap between the two prompts. */
	constexpr float PromptGap = 24.f;
}

// ---------------------------------------------------------------------------
// The focus and the prompts
// ---------------------------------------------------------------------------

void UMissionOfferWidget::RebuildNav()
{
	TArray<FMenuNavRow> Rows;
	Nav.SetWrap(true);
	if (Mode == EMissionCardMode::Choice && ChoiceButtons.Num() >= 2)
	{
		// The two choices side by side; the focus starts on the first, and nothing is chosen by a press alone.
		Rows.Add(FMenuNavRow{ FMenuNavItem::MakeButton(ChoiceButtons[0].Get()), FMenuNavItem::MakeButton(ChoiceButtons[1].Get()) });
		Nav.SetRows(MoveTemp(Rows), ChoiceButtons[0].Get());
		return;
	}
	if (Mode == EMissionCardMode::Read)
	{
		Rows.Add(FMenuNavRow{ FMenuNavItem::MakeButton(DeclineButton.Get()) });
		Nav.SetRows(MoveTemp(Rows), DeclineButton.Get());
		return;
	}
	// One row, Not now then Accept; the focus starts on Accept, the choice a giver hopes for.
	Rows.Add(FMenuNavRow{ FMenuNavItem::MakeButton(DeclineButton.Get()), FMenuNavItem::MakeButton(AcceptButton.Get()) });
	Nav.SetRows(MoveTemp(Rows), AcceptButton.Get());
}

FKey UMissionOfferWidget::GetInteractKey() const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	const FKey Interact = Bindings ? Bindings->GetKey(TEXT("Interact")) : EKeys::E;
	return Interact.IsValid() ? Interact : EKeys::E;
}

void UMissionOfferWidget::RefreshNavLook()
{
	ShownDevice = LooterUI::GetMenuDevice();
	Nav.SetShown(LooterUI::IsMenuFocusShown() && !bClosing);
	if (!PromptBar)
	{
		return;
	}
	PromptBar->ClearChildren();
	const bool bPadNow = LooterUI::IsMenuGamepad();
	if (Mode != EMissionCardMode::Offer)
	{
		// Choosing: the keys that move between the two, the hold that chooses (E, or a pad's A), and the way out. Reading:
		// only the way out.
		struct FPromptWords
		{
			FString Key;
			FString Words;
		};
		TArray<FPromptWords, TInlineAllocator<3>> Prompts;
		if (Mode == EMissionCardMode::Choice)
		{
			Prompts.Add({ bPadNow ? FString(TEXT("D-pad")) : FString(TEXT("A / D")), NSLOCTEXT("MissionOffer", "PromptPick", "Pick").ToString() });
			Prompts.Add({ bPadNow ? PromptKeyName(EPromptKey::Select, true) : GetInteractKey().GetDisplayName().ToString(),
				NSLOCTEXT("MissionOffer", "PromptHold", "Hold to choose").ToString() });
		}
		Prompts.Add({ PromptKeyName(EPromptKey::Back, bPadNow), Card.Decline.ToString() });
		for (int32 Index = 0; Index < Prompts.Num(); ++Index)
		{
			UHorizontalBoxSlot* PromptSlot = PromptBar->AddChildToHorizontalBox(LoadoutParts::MakeKeyHint(WidgetTree, Prompts[Index].Key,
				Prompts[Index].Words, Index == Prompts.Num() - 2 || Prompts.Num() == 1));
			PromptSlot->SetVerticalAlignment(VAlign_Center);
			PromptSlot->SetPadding(FMargin(Index > 0 ? PromptGap : 0.f, 0.f, 0.f, 0.f));
		}
		return;
	}
	// The keyboard's E (wherever Interact is bound) always accepts; a pad's A presses what has the focus, so its prompt
	// names that.
	const bool bPad = LooterUI::IsMenuGamepad();
	FString AcceptKey = PromptKeyName(EPromptKey::Select, true);
	FText AcceptWord = Card.Accept;
	if (bPad)
	{
		AcceptWord = Nav.GetFocusedWidget() == DeclineButton.Get() ? Card.Decline : Card.Accept;
	}
	else
	{
		const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
		const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
		const FKey Interact = Bindings ? Bindings->GetKey(TEXT("Interact")) : EKeys::E;
		AcceptKey = Interact.IsValid() ? Interact.GetDisplayName().ToString() : FString(TEXT("E"));
	}
	PromptBar->AddChildToHorizontalBox(LoadoutParts::MakeKeyHint(WidgetTree, AcceptKey, AcceptWord.ToString(), true))
		->SetVerticalAlignment(VAlign_Center);
	UHorizontalBoxSlot* BackSlot = PromptBar->AddChildToHorizontalBox(LoadoutParts::MakeKeyHint(WidgetTree,
		PromptKeyName(EPromptKey::Back, bPad), Card.Decline.ToString(), false));
	BackSlot->SetVerticalAlignment(VAlign_Center);
	BackSlot->SetPadding(FMargin(PromptGap, 0.f, 0.f, 0.f));
}

bool UMissionOfferWidget::RevealNav(const FKey& Key)
{
	const bool bWasShown = LooterUI::IsMenuFocusShown();
	if (LooterUI::NoteMenuKey(Key) || bWasShown != LooterUI::IsMenuFocusShown())
	{
		RefreshNavLook();
	}
	return bWasShown;
}

void UMissionOfferWidget::StepNav(EMove Move)
{
	const int32 Step = HorizontalStep(Move) != 0 ? HorizontalStep(Move) : VerticalStep(Move);
	if (Step != 0 && Nav.MoveHorizontal(Step) == EMenuNavResult::Moved)
	{
		LooterSound::Play2D(this, LooterSoundCue::Hover);
		RefreshNavLook();
	}
}

// ---------------------------------------------------------------------------
// Buttons and the mouse
// ---------------------------------------------------------------------------

void UMissionOfferWidget::HandleButton(ULooterButton* Button)
{
	if (!Button || bClosing)
	{
		return;
	}
	// A choice is held, never clicked: a click only says so (the press and its release drove the hold).
	const int32 ChoiceIndex = ChoiceButtons.IndexOfByKey(Button);
	if (ChoiceIndex != INDEX_NONE)
	{
		if (Chosen == INDEX_NONE && HoldProgress < 1.f)
		{
			SetStatus(NSLOCTEXT("MissionOffer", "HoldToChooseHint", "Hold it down to choose: this can't be undone."));
		}
		if (IsInViewport())
		{
			SetKeyboardFocus();
		}
		return;
	}
	if (Button == AcceptButton.Get())
	{
		if (!Accept() && IsInViewport())
		{
			// A click handed the keyboard to the game viewport (LooterButton): the card keeps it while it stays.
			SetKeyboardFocus();
		}
		return;
	}
	Decline();
}

void UMissionOfferWidget::HandleHovered(ULooterButton* Button)
{
	// Only while the mouse is in charge: the keys' focus isn't moved by a pointer resting where the card opened.
	if (!bClosing && !LooterUI::IsMenuFocusShown() && Nav.FocusOn(Button))
	{
		LooterSound::Play2D(this, LooterSoundCue::Hover);
	}
}

void UMissionOfferWidget::NotePointer()
{
	if (LooterUI::NoteMenuPointer())
	{
		RefreshNavLook();
	}
}

FReply UMissionOfferWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// A click on no button ends here rather than in the game viewport, which would take the keyboard from the card.
	NotePointer();
	return FReply::Handled();
}

FReply UMissionOfferWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// A real move: Slate's own made-up moves have no distance.
	if (InMouseEvent.GetCursorDelta().SizeSquared() > 1.0)
	{
		NotePointer();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

// ---------------------------------------------------------------------------
// Keys and the stick
// ---------------------------------------------------------------------------

FReply UMissionOfferWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	// On its way out (the stamp, the fade): nothing more to choose, and no key reaches the game under it.
	if (bClosing)
	{
		return FReply::Handled();
	}
	if (Mode == EMissionCardMode::Choice && HandleChoiceKeyDown(InKeyEvent))
	{
		return FReply::Handled();
	}
	if (Mode == EMissionCardMode::Read && !IsStickKey(Key) && (IsBack(Key) || IsAccept(Key) || Key == GetInteractKey()))
	{
		// Read: any of the usual keys puts it away (once it's had a moment, so the key that opened it doesn't).
		RevealNav(Key);
		if (!InKeyEvent.IsRepeat() && (IsBack(Key) || IsArmed()))
		{
			Decline();
		}
		return FReply::Handled();
	}
	if (IsBack(Key))
	{
		RevealNav(Key);
		if (!InKeyEvent.IsRepeat())
		{
			Decline();
		}
		return FReply::Handled();
	}
	if (IsStickKey(Key))
	{
		return FReply::Handled();
	}
	// E, or wherever Interact is bound, takes it wherever the focus is (the prompt says so). A key still held from the
	// talk or the pickup that opened the card only repeats, and takes nothing.
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	if (Key == EKeys::E || (Bindings && Bindings->GetKey(TEXT("Interact")) == Key))
	{
		RevealNav(Key);
		if (!InKeyEvent.IsRepeat())
		{
			Accept();
		}
		return FReply::Handled();
	}
	const EMove Move = MoveOf(Key);
	if (Move != EMove::None)
	{
		// The first move after the mouse only shows where the focus is.
		if (RevealNav(Key) && !InKeyEvent.IsRepeat())
		{
			StepNav(Move);
		}
		return FReply::Handled();
	}
	if (IsAccept(Key))
	{
		// Enter, Space or A press what has the focus: Accept, unless it was moved to Not now.
		RevealNav(Key);
		if (!InKeyEvent.IsRepeat())
		{
			Nav.Activate();
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UMissionOfferWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Letting a choice's key go before its ring is full drains it back.
	if (Mode == EMissionCardMode::Choice && HandleChoiceKeyUp(InKeyEvent))
	{
		return FReply::Handled();
	}
	return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}

FReply UMissionOfferWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent)
{
	const FKey Key = InAnalogEvent.GetKey();
	if (Key != EKeys::Gamepad_LeftX && Key != EKeys::Gamepad_LeftY)
	{
		return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
	}
	(Key == EKeys::Gamepad_LeftX ? NavStick.X : NavStick.Y) = InAnalogEvent.GetAnalogValue();
	const EMove Next = StickMove(NavStick, NavStickMove);
	if (Next != NavStickMove)
	{
		NavStickMove = Next;
		// A push of the stick moves once, as a press of the D-pad does (two buttons need no repeat).
		if (Next != EMove::None && !bClosing && RevealNav(StickKey(Next)))
		{
			StepNav(Next);
		}
	}
	// Taken, so Slate makes no navigation of its own from the stick.
	return FReply::Handled();
}

FNavigationReply UMissionOfferWidget::NativeOnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent,
	const FNavigationReply& InDefaultReply)
{
	// The card moves its own focus: Slate's must never carry the keyboard off it.
	return FNavigationReply::Stop();
}

void UMissionOfferWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	NavStick = FVector2D::ZeroVector;
	NavStickMove = EMove::None;
	// A key held on a choice never sees its release once the focus has gone: the hold lets go with it. A mouse hold
	// keeps going (a click on the button hands the keyboard to the viewport for a moment).
	if (HoldKey.IsValid())
	{
		EndHold();
	}
	Super::NativeOnFocusLost(InFocusEvent);
}

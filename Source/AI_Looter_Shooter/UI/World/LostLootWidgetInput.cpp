// ULostLootWidget for the keyboard, a controller and the mouse: W / S, the arrows, the D-pad and the stick choose a ticket
// (held, the stick keeps choosing), Page Up / Down or the bumpers a page at a time; E (wherever Interact is bound), Enter,
// Space or A claims; U or Y buys shelving; Esc or B backs out of the confirm, then closes. The mouse chooses by hovering
// while it's in charge, clicks the buttons and scrolls. The prompts name the keys of the device last used (LooterUIFocus.h,
// MenuNav.h).

#include "UI/World/LostLootWidget.h"
#include "Audio/LooterSoundCues.h"
#include "Settings/KeyBindingSubsystem.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "LooterLostLoot"

using namespace MenuNav;

namespace
{
	/** The gap between two prompts. */
	constexpr float PromptGap = 14.f;

	struct FPromptWords
	{
		FString Key;
		FText Does;
	};

	/** The prompts for a device, the main one first: the confirm's two, else the counter's four. */
	TArray<FPromptWords> MakePrompts(bool bPad, bool bConfirm, const FString& InteractKey)
	{
		const FString Claim = bPad ? PromptKeyName(EPromptKey::Select, true) : InteractKey;
		const FString Back = PromptKeyName(EPromptKey::Back, bPad);
		if (bConfirm)
		{
			return { { Claim, LOCTEXT("PromptBuy", "Buy") }, { Back, LOCTEXT("PromptNotNow", "Not now") } };
		}
		return {
			{ Claim, LOCTEXT("PromptClaim", "Claim") },
			{ bPad ? PromptKeyName(EPromptKey::Move, true) : FString(TEXT("W/S")), LOCTEXT("PromptChoose", "Choose") },
			{ bPad ? FString(TEXT("Y")) : FString(TEXT("U")), LOCTEXT("PromptShelving", "More shelving") },
			{ Back, LOCTEXT("PromptClose", "Close") },
		};
	}
}

// ---------------------------------------------------------------------------
// The prompts
// ---------------------------------------------------------------------------

TArray<FString> ULostLootWidget::PromptWords() const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	const FKey Interact = Bindings ? Bindings->GetKey(TEXT("Interact")) : EKeys::E;
	const FString InteractKey = Interact.IsValid() ? Interact.GetDisplayName().ToString() : FString(TEXT("E"));
	TArray<FString> Words;
	for (const FPromptWords& Prompt : MakePrompts(LooterUI::IsMenuGamepad(), bConfirmOpen, InteractKey))
	{
		Words.Add(FString::Printf(TEXT("%s: %s"), *Prompt.Key, *Prompt.Does.ToString()));
	}
	return Words;
}

void ULostLootWidget::RefreshPrompts()
{
	ShownDevice = LooterUI::GetMenuDevice();
	if (!PromptBar)
	{
		return;
	}
	PromptBar->ClearChildren();
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	const FKey Interact = Bindings ? Bindings->GetKey(TEXT("Interact")) : EKeys::E;
	const FString InteractKey = Interact.IsValid() ? Interact.GetDisplayName().ToString() : FString(TEXT("E"));
	const TArray<FPromptWords> Prompts = MakePrompts(LooterUI::IsMenuGamepad(), bConfirmOpen, InteractKey);
	for (int32 Index = 0; Index < Prompts.Num(); ++Index)
	{
		UHorizontalBoxSlot* HintSlot = PromptBar->AddChildToHorizontalBox(LoadoutParts::MakeKeyHint(WidgetTree, Prompts[Index].Key,
			Prompts[Index].Does.ToString(), Index == 0));
		HintSlot->SetVerticalAlignment(VAlign_Center);
		HintSlot->SetPadding(FMargin(Index > 0 ? PromptGap : 0.f, 0.f, 0.f, 0.f));
	}
}

void ULostLootWidget::NoteKey(const FKey& Key)
{
	if (LooterUI::NoteMenuKey(Key) || ShownDevice != LooterUI::GetMenuDevice())
	{
		RefreshPrompts();
	}
}

void ULostLootWidget::NotePointer()
{
	if (LooterUI::NoteMenuPointer())
	{
		RefreshPrompts();
	}
}

// ---------------------------------------------------------------------------
// Buttons and the mouse
// ---------------------------------------------------------------------------

void ULostLootWidget::HandleButton(ULooterButton* Button)
{
	if (!Button)
	{
		return;
	}
	if (Button->Action == LostLootActions::Close)
	{
		Close();
		return;
	}
	if (bConfirmOpen)
	{
		if (Button->Action == LostLootActions::Accept)
		{
			AcceptConfirm();
		}
		else if (Button->Action == LostLootActions::Back)
		{
			CloseConfirm();
			PlayCue(LooterSoundCue::Back);
		}
	}
	else if (!bStamping)
	{
		if (Button->Action == LostLootActions::Ticket)
		{
			// A click chooses the ticket; its card's Claim takes it.
			Select(Button->Index, Button->Index != Selected, false);
		}
		else if (Button->Action == LostLootActions::Claim)
		{
			Claim();
		}
		else if (Button->Action == LostLootActions::Upgrade)
		{
			BuyUpgrade(/*bConfirmed*/ false);
		}
	}
	// A click handed the keyboard to the game viewport (LooterButton): take it back so the keys keep reaching the screen.
	if (IsInViewport())
	{
		SetKeyboardFocus();
	}
}

void ULostLootWidget::HandleHovered(ULooterButton* Button)
{
	// Only while the mouse is in charge: a pointer resting where the screen opened never moves the keys' choice.
	if (Button && Button->Action == LostLootActions::Ticket && !bConfirmOpen && !bStamping && !LooterUI::IsMenuFocusShown())
	{
		Select(Button->Index, true, false);
	}
}

FReply ULostLootWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// A click on no button ends here rather than in the game viewport, which would take the keyboard from the screen.
	NotePointer();
	return FReply::Handled();
}

FReply ULostLootWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
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

bool ULostLootWidget::HandleKey(const FKey& Key, bool bRepeat)
{
	NoteKey(Key);
	if (IsBack(Key) || IsMenuButton(Key))
	{
		if (!bRepeat)
		{
			if (bConfirmOpen)
			{
				CloseConfirm();
				PlayCue(LooterSoundCue::Back);
			}
			else
			{
				Close();
			}
		}
		return true;
	}
	// The stick comes in through its axes (NativeOnAnalogValueChanged), never twice.
	if (IsStickKey(Key))
	{
		return true;
	}
	// CLAIMED plays out first; only leaving cuts it short.
	if (bStamping)
	{
		return true;
	}
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	const bool bInteract = Key == EKeys::E || (Bindings && Bindings->GetKey(TEXT("Interact")) == Key);
	// The Interact key that opened the screen may still be held and repeating: only a fresh press acts.
	const bool bAct = !bRepeat && (bInteract || IsAccept(Key));
	if (bConfirmOpen)
	{
		// A confirm takes only its own keys.
		if (bAct)
		{
			AcceptConfirm();
		}
		return true;
	}
	if (Key == EKeys::U || Key == EKeys::Gamepad_FaceButton_Top)
	{
		if (!bRepeat)
		{
			BuyUpgrade(/*bConfirmed*/ false);
		}
		return true;
	}
	const EMove Move = MoveOf(Key);
	if (Move != EMove::None)
	{
		// Up and down choose; left and right have nothing to do here, and are taken so they reach nothing under the screen.
		if (VerticalStep(Move) != 0)
		{
			Step(VerticalStep(Move));
		}
		return true;
	}
	const int32 Pages = SectionStepOf(Key);
	if (Pages != 0)
	{
		Step(Pages * LostLootLayout::PageStep);
		return true;
	}
	if (Key == EKeys::Home || Key == EKeys::End)
	{
		Step(Key == EKeys::Home ? -Rows.Num() : Rows.Num());
		return true;
	}
	if (bInteract || IsAccept(Key))
	{
		if (bAct)
		{
			Claim();
		}
		return true;
	}
	return false;
}

FReply ULostLootWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	return HandleKey(InKeyEvent.GetKey(), InKeyEvent.IsRepeat()) ? FReply::Handled() : Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply ULostLootWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent)
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
		StickRepeat.Stop();
		if (Next != EMove::None)
		{
			NoteKey(StickKey(Next));
			// A push moves once at once, then repeats while held, as a held D-pad does (a long list wants it).
			if (VerticalStep(Next) != 0 && !bConfirmOpen && !bStamping)
			{
				Step(VerticalStep(Next));
				StickRepeat.Press(StickKey(Next), Next, /*bAccelerate*/ false);
			}
		}
	}
	// Taken, so Slate makes no navigation of its own from the stick.
	return FReply::Handled();
}

FNavigationReply ULostLootWidget::NativeOnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent,
	const FNavigationReply& InDefaultReply)
{
	// The screen moves its own choice: Slate's must never carry the keyboard off it.
	return FNavigationReply::Stop();
}

void ULostLootWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	NavStick = FVector2D::ZeroVector;
	NavStickMove = EMove::None;
	StickRepeat.Stop();
	Super::NativeOnFocusLost(InFocusEvent);
}

#undef LOCTEXT_NAMESPACE

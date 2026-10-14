// UNoticeBoardWidget for the keyboard, a controller and the mouse: the cards that can be acted on are an FMenuNavigator's
// stops (W / S, the arrows, the D-pad or the stick choose; E, Enter, Space or A act; Esc or B close), a click chooses a
// card and a click on the chosen one acts on it, the mouse over a card lights it, and the prompts name the device's keys
// (LooterUIFocus.h, MenuNav.h, MenuNavigator.h).

#include "UI/World/NoticeBoardWidget.h"
#include "Audio/LooterSound.h"
#include "Settings/KeyBindingSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "NoticeBoard"

using namespace MenuNav;

namespace
{
	/** The gap between the prompts at the bottom. */
	constexpr float PromptGap = 16.f;
}

// ---------------------------------------------------------------------------
// Choosing
// ---------------------------------------------------------------------------

int32 UNoticeBoardWidget::CardOf(const UWidget* Widget) const
{
	return Widget ? Cards.IndexOfByPredicate([Widget](const FCard& Card) { return Card.Button == Widget; }) : INDEX_NONE;
}

void UNoticeBoardWidget::Select(int32 Index)
{
	if (!CanAct(Index) || Index == Selected)
	{
		return;
	}
	Selected = Index;
	// The keys go on from the card the mouse chose, and its focus look moves with it.
	if (Cards.IsValidIndex(Index) && Cards[Index].Button)
	{
		Nav.FocusOn(Cards[Index].Button);
	}
	Restyle();
}

void UNoticeBoardWidget::Step(int32 Direction)
{
	// Past the done and locked cards (they're no stops) to the next that can be acted on, round the ends.
	if (Direction == 0 || Nav.MoveVertical(Direction) != EMenuNavResult::Moved)
	{
		return;
	}
	const int32 Index = CardOf(Nav.GetFocusedWidget());
	if (Index != INDEX_NONE && Index != Selected)
	{
		Selected = Index;
		Restyle();
		LooterSound::Play2D(this, LooterSoundCue::Hover);
	}
}

bool UNoticeBoardWidget::ClickCard(int32 Index)
{
	if (!CanAct(Index))
	{
		return false;
	}
	if (Index != Selected)
	{
		// The first click chooses it, as W and S would; the next (or a double click's second) acts on it.
		Select(Index);
		return true;
	}
	Act(Index);
	return true;
}

void UNoticeBoardWidget::RebuildNav()
{
	TArray<FMenuNavRow> Rows;
	for (const FCard& Card : Cards)
	{
		if (Card.Button)
		{
			// The button holds the whole card and has no look of its own (it's bare): the frame laid over the card lights.
			FMenuNavItem Item = FMenuNavItem::MakeButton(Card.Button);
			Item.Light = Card.Focus;
			Rows.Add(FMenuNavRow{ Item });
		}
	}
	// A short board: round the ends, as it always went.
	Nav.SetWrap(true);
	Nav.SetRows(MoveTemp(Rows), Cards.IsValidIndex(Selected) ? Cards[Selected].Button : nullptr);
}

// ---------------------------------------------------------------------------
// The device: the focus look, the keycaps, the prompts
// ---------------------------------------------------------------------------

FString UNoticeBoardWidget::ActKeyName() const
{
	if (LooterUI::IsMenuGamepad())
	{
		return PromptKeyName(EPromptKey::Select, true);
	}
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	const FKey Interact = Bindings ? Bindings->GetKey(TEXT("Interact")) : FKey();
	return Interact.IsValid() ? Interact.GetDisplayName(/*bLongDisplayName*/ false).ToString() : FString(TEXT("E"));
}

bool UNoticeBoardWidget::IsActKey(const FKey& Key) const
{
	if (IsAccept(Key))
	{
		return true;
	}
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	return Bindings && Bindings->GetKey(TEXT("Interact")) == Key;
}

void UNoticeBoardWidget::RefreshNavLook()
{
	ShownDevice = LooterUI::GetMenuDevice();
	Nav.SetShown(LooterUI::IsMenuFocusShown());
	const FText Key = FText::FromString(ActKeyName());
	for (const FCard& Card : Cards)
	{
		if (Card.KeyText)
		{
			Card.KeyText->SetText(Key);
		}
	}
	// The mouse's light goes with the mouse: keys show the focus look instead.
	if (LooterUI::IsMenuFocusShown())
	{
		Hovered = INDEX_NONE;
	}
	RefreshPrompts();
	Restyle();
}

void UNoticeBoardWidget::RefreshPrompts()
{
	if (!PromptBar)
	{
		return;
	}
	PromptBar->ClearChildren();
	const bool bPad = LooterUI::IsMenuGamepad();
	struct FHint
	{
		FString Key;
		FText Does;
	};
	const FHint Hints[] = {
		{ bPad ? FString(TEXT("D-pad")) : FString(TEXT("W/S")), LOCTEXT("HintSelect", "Select") },
		{ ActKeyName(), LOCTEXT("HintAct", "Take, track or turn in") },
		{ PromptKeyName(EPromptKey::Back, bPad), LOCTEXT("HintClose", "Close") },
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Hints); ++Index)
	{
		UHorizontalBoxSlot* HintSlot = PromptBar->AddChildToHorizontalBox(LoadoutParts::MakeKeyHint(WidgetTree, Hints[Index].Key,
			Hints[Index].Does.ToString(), Index == 1));
		HintSlot->SetVerticalAlignment(VAlign_Center);
		HintSlot->SetPadding(FMargin(Index > 0 ? PromptGap : 0.f, 0.f, 0.f, 0.f));
	}
}

void UNoticeBoardWidget::NoteKey(const FKey& Key)
{
	if (LooterUI::NoteMenuKey(Key))
	{
		RefreshNavLook();
	}
}

void UNoticeBoardWidget::NotePointer()
{
	if (LooterUI::NoteMenuPointer())
	{
		RefreshNavLook();
	}
}

void UNoticeBoardWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// Another menu (the offer card over the board) may have seen the device change.
	if (LooterUI::GetMenuDevice() != ShownDevice)
	{
		RefreshNavLook();
	}
	Nav.Tick(InDeltaTime);
	// The card under the mouse lights, as the kit's buttons do; only while the mouse drives, so a pointer resting where the
	// board opened never vies with the keys' focus.
	int32 Under = INDEX_NONE;
	if (!LooterUI::IsMenuFocusShown())
	{
		Under = Cards.IndexOfByPredicate([](const FCard& Card) { return Card.Button && Card.Button->IsHovered(); });
	}
	if (Under != Hovered)
	{
		Hovered = Under;
		Restyle();
	}
}

// ---------------------------------------------------------------------------
// Buttons and the mouse
// ---------------------------------------------------------------------------

void UNoticeBoardWidget::HandleButton(ULooterButton* Button)
{
	if (!Button)
	{
		return;
	}
	if (Button == CloseButton)
	{
		Close();
		return;
	}
	// A click on a card, or the navigator's press of the chosen one (E and the rest): choose it, or act on it.
	if (!ClickCard(Button->Index))
	{
		LooterSound::Play2D(this, LooterSoundCue::Denied);
	}
	// A click handed the keyboard to the game viewport (LooterButton): the board takes it back, unless the offer card it just
	// opened is over it now, which keeps the keys until it closes (then ALooterHUD::CloseMissionOffer gives them back).
	const ALooterHUD* HUD = OwningHUD.Get();
	if (IsInViewport() && !(HUD && HUD->IsMissionOfferOpen()))
	{
		SetKeyboardFocus();
	}
}

FReply UNoticeBoardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// A click on no button ends here rather than in the game viewport, which would take the keyboard from the board.
	NotePointer();
	return FReply::Handled();
}

FReply UNoticeBoardWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
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

FReply UNoticeBoardWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (IsBack(Key))
	{
		NoteKey(Key);
		Close();
		return FReply::Handled();
	}
	if (IsStickKey(Key))
	{
		// The stick is read from its axes (NativeOnAnalogValueChanged): its own direction keys would move the choice twice.
		return FReply::Handled();
	}
	const int32 Direction = VerticalStep(MoveOf(Key));
	if (Direction != 0)
	{
		NoteKey(Key);
		Step(Direction);
		return FReply::Handled();
	}
	if (IsActKey(Key))
	{
		NoteKey(Key);
		// The Interact key that read the board may still be held and repeating: only a fresh press acts. The navigator
		// presses the chosen card's button, so a key and a click take the one way (HandleButton).
		if (!InKeyEvent.IsRepeat() && Nav.Activate() == EMenuNavResult::None)
		{
			LooterSound::Play2D(this, LooterSoundCue::Denied);
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UNoticeBoardWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent)
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
		// A push of the stick moves the choice once, as a press of the D-pad does.
		const int32 Direction = VerticalStep(Next);
		if (Direction != 0)
		{
			NoteKey(StickKey(Next));
			Step(Direction);
		}
	}
	// Taken, so Slate makes no navigation of its own from the stick.
	return FReply::Handled();
}

FNavigationReply UNoticeBoardWidget::NativeOnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent,
	const FNavigationReply& InDefaultReply)
{
	// The board moves its own choice: Slate's must never carry the keyboard off it.
	return FNavigationReply::Stop();
}

void UNoticeBoardWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	// A stick held as the focus went never sees its release.
	NavStick = FVector2D::ZeroVector;
	NavStickMove = EMove::None;
	Super::NativeOnFocusLost(InFocusEvent);
}

#undef LOCTEXT_NAMESPACE

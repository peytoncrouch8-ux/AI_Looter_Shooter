// UHeroSelectWidget for the keyboard, a controller and the mouse: the focus over the roster and the buttons, the choice
// following it, 1 to 5 and the bumpers, A to confirm, B to go back, the clicks, the cards lit under the mouse, the
// stage's own motion (the choice settling in, the glow's breath, the confirm's beat) and the key hints at the bottom.

#include "UI/Menus/HeroSelectWidget.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Embers/HeroDefinition.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIFocus.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "InputCoreTypes.h"

using namespace MenuNav;

namespace
{
	const FName ActionHero(TEXT("Hero"));
	const FName ActionBack(TEXT("Back"));
	const FName ActionConfirm(TEXT("ConfirmHero"));

	/** The gap between the key hints at the bottom. */
	constexpr float PromptGap = 20.f;
	/** How long the chosen portrait takes to settle in, and how far it slides from. */
	constexpr float StageInSeconds = 0.2f;
	constexpr float StageInSlide = 14.f;
	/** The glow's breath behind the stage: its period and how far it dims. */
	constexpr float GlowBreathSeconds = 4.2f;
	constexpr float GlowBreathDepth = 0.18f;

	/** 1 to 5 on the keyboard's top row or the number pad: the roster's place (0 to 4), or INDEX_NONE. */
	int32 RosterPlaceOf(const FKey& Key)
	{
		static const FKey Keys[][2] = {
			{ EKeys::One, EKeys::NumPadOne }, { EKeys::Two, EKeys::NumPadTwo }, { EKeys::Three, EKeys::NumPadThree },
			{ EKeys::Four, EKeys::NumPadFour }, { EKeys::Five, EKeys::NumPadFive } };
		for (int32 Place = 0; Place < UE_ARRAY_COUNT(Keys); ++Place)
		{
			if (Key == Keys[Place][0] || Key == Keys[Place][1])
			{
				return Place;
			}
		}
		return INDEX_NONE;
	}
}

// ---------------------------------------------------------------------------
// The stops
// ---------------------------------------------------------------------------

void UHeroSelectWidget::RebuildNavigation(const UWidget* Prefer)
{
	// The roster's cards in a row (their focus look is the frame over each), then Back and Play.
	TArray<FMenuNavRow> Rows;
	FMenuNavRow Roster;
	for (const FCard& Card : Cards)
	{
		FMenuNavItem Item = FMenuNavItem::MakeButton(Card.Button);
		Item.Light = Card.FocusLook;
		Roster.Add(Item);
	}
	if (!Roster.IsEmpty())
	{
		Rows.Add(MoveTemp(Roster));
	}
	if (BackButton && ConfirmButton)
	{
		Rows.Add(FMenuNavRow{ FMenuNavItem::MakeButton(BackButton), FMenuNavItem::MakeButton(ConfirmButton) });
	}
	Nav.SetWrap(true);
	Nav.SetRows(MoveTemp(Rows), Prefer);
	RefreshFooter();
}

void UHeroSelectWidget::FollowFocus()
{
	const int32 Index = Cards.IndexOfByPredicate([this](const FCard& Card) { return Card.Button == Nav.GetFocusedWidget(); });
	if (Index != INDEX_NONE && Index != Selected)
	{
		// The move already sounded.
		Select(Index, false);
	}
}

void UHeroSelectWidget::GoBack()
{
	StopNavHold();
	OnBack.ExecuteIfBound();
}

// ---------------------------------------------------------------------------
// Keys and the stick
// ---------------------------------------------------------------------------

bool UHeroSelectWidget::HandleNavKey(const FKey& Key, bool bRepeat)
{
	// The pick has taken: the session is starting, nothing more to press.
	if (IsConfirming())
	{
		return true;
	}
	if (IsBack(Key))
	{
		if (Key.IsGamepadKey() && LooterUI::NoteMenuKey(Key))
		{
			RefreshNavLook();
		}
		if (!bRepeat)
		{
			LooterSound::Play2D(this, LooterSoundCue::Back);
			GoBack();
		}
		return true;
	}
	if (IsStickKey(Key))
	{
		return true;
	}

	// 1 to 5 and the bumpers choose a hero straight away, the focus going with the choice.
	const int32 Place = RosterPlaceOf(Key);
	const int32 Turn = SectionStepOf(Key);
	if (Place != INDEX_NONE || Turn != 0)
	{
		if (!bRepeat && !Heroes.IsEmpty())
		{
			RevealNav(Key);
			StopNavHold();
			const int32 Index = Place != INDEX_NONE ? Place : (Selected + Turn + Heroes.Num()) % Heroes.Num();
			if (Heroes.IsValidIndex(Index))
			{
				Select(Index);
				Nav.FocusOn(GetCard(Index));
				RefreshFooter();
			}
		}
		return true;
	}

	const EMove Move = MoveOf(Key);
	const bool bAccept = IsAccept(Key);
	if (Move == EMove::None && !bAccept)
	{
		return false;
	}
	if (bRepeat)
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
		return true;
	}
	StopNavHold();
	// A on a card confirms that hero (Borderlands' way); on Back or Play, the button's own press.
	const UWidget* Focused = Nav.GetFocusedWidget();
	if (Cards.ContainsByPredicate([Focused](const FCard& Card) { return Card.Button == Focused; }))
	{
		FollowFocus();
		Confirm();
	}
	else
	{
		Nav.Activate();
	}
	RefreshFooter();
	return true;
}

void UHeroSelectWidget::HandleNavKeyUp(const FKey& Key)
{
	NavRepeat.Release(Key);
}

void UHeroSelectWidget::HandleNavStick(const FVector2D& Stick)
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
	if (Next != EMove::None && !IsConfirming())
	{
		const FKey Key = StickKey(Next);
		if (RevealNav(Key))
		{
			PressNav(Key, Next);
		}
	}
}

bool UHeroSelectWidget::RevealNav(const FKey& Key)
{
	const bool bWasShown = LooterUI::IsMenuFocusShown();
	if (LooterUI::NoteMenuKey(Key))
	{
		RefreshNavLook();
	}
	return bWasShown;
}

void UHeroSelectWidget::PressNav(const FKey& Key, EMove Move)
{
	StepNav(Move);
	NavRepeat.Press(Key, Move, false);
}

void UHeroSelectWidget::StepNav(EMove Move)
{
	const int32 Vertical = VerticalStep(Move);
	const EMenuNavResult Result = Vertical != 0 ? Nav.MoveVertical(Vertical) : Nav.MoveHorizontal(HorizontalStep(Move));
	if (Result == EMenuNavResult::Moved)
	{
		LooterSound::Play2D(this, LooterSoundCue::Hover);
		FollowFocus();
		RefreshFooter();
	}
}

void UHeroSelectWidget::StopNavHold()
{
	NavRepeat.Stop();
	Nav.EndSliderHold();
}

void UHeroSelectWidget::AdvanceNav(float DeltaSeconds)
{
	for (int32 Due = NavRepeat.Tick(DeltaSeconds); Due > 0 && NavRepeat.IsHeld(); --Due)
	{
		StepNav(NavRepeat.GetMove());
	}
	Nav.Tick(DeltaSeconds);

	// The card under the mouse lights (bare buttons draw no hover of their own).
	const int32 Hovered = Cards.IndexOfByPredicate([](const FCard& Card) { return Card.Button && Card.Button->IsHovered(); });
	if (Hovered != HoveredCard)
	{
		HoveredCard = Hovered;
		RestyleCards();
	}

	// The chosen portrait settles in from a little to the left; the glow behind it breathes slowly.
	if (StageIn < 1.f && StagePicture)
	{
		StageIn = FMath::Min(StageIn + DeltaSeconds / StageInSeconds, 1.f);
		const float Eased = 1.f - FMath::Square(1.f - StageIn);
		StagePicture->SetRenderOpacity(FMath::Lerp(0.35f, 1.f, Eased));
		StagePicture->SetRenderTranslation(FVector2D(-StageInSlide * (1.f - Eased), 0.f));
	}
	GlowClock = FMath::Fmod(GlowClock + DeltaSeconds, GlowBreathSeconds);
	if (StageGlow && !IsConfirming())
	{
		StageGlow->SetRenderOpacity(1.f - GlowBreathDepth * 0.5f * (1.f - FMath::Cos(GlowClock / GlowBreathSeconds * UE_TWO_PI)));
	}
	TickConfirm(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// The mouse
// ---------------------------------------------------------------------------

void UHeroSelectWidget::HandleButton(ULooterButton* Button)
{
	if (!Button || IsConfirming())
	{
		return;
	}
	if (Button->Action == ActionHero)
	{
		// A click chooses (Play confirms); the keys' A on a card confirms instead (HandleNavKey).
		if (Button->Index != Selected)
		{
			LooterSound::Play2D(this, LooterSoundCue::Click);
		}
		Select(Button->Index, false);
		Nav.FocusOn(Button);
	}
	else if (Button->Action == ActionConfirm)
	{
		Confirm();
	}
	else if (Button->Action == ActionBack)
	{
		// Its own click sounded UI.Back.
		GoBack();
	}
	RefreshFooter();
	// A click handed the keyboard to the game viewport (LooterButton): the menu holding this screen takes it back.
	if (UUserWidget* Host = GetTypedOuter<UUserWidget>())
	{
		Host->SetKeyboardFocus();
	}
}

void UHeroSelectWidget::HandleButtonHovered(ULooterButton* Button)
{
	// Only while the mouse is in charge (a layout change under a still pointer mustn't steal the keys' focus).
	if (!LooterUI::IsMenuFocusShown() && Nav.FocusOn(Button))
	{
		RefreshFooter();
	}
}

// ---------------------------------------------------------------------------
// The look and the key hints
// ---------------------------------------------------------------------------

UWidget* UHeroSelectWidget::GetFocusedWidget() const
{
	return Nav.GetFocusedWidget();
}

void UHeroSelectWidget::RefreshNavLook()
{
	Nav.SetShown(LooterUI::IsMenuFocusShown());
	RefreshFooter();
}

TArray<FPrompt> UHeroSelectWidget::GetPrompts() const
{
	TArray<FPrompt> Prompts;
	if (IsConfirming())
	{
		return Prompts;
	}
	const bool bPad = LooterUI::IsMenuGamepad();
	// On Back the main key steps back; anywhere else it plays the hero chosen.
	const bool bOnBack = Nav.GetFocusedWidget() == BackButton;
	Prompts.Add(MakePrompt(EPromptKey::Select, bPad, bOnBack ? TEXT("Select") : TEXT("Play")));
	Prompts.Add(MakePrompt(EPromptKey::Adjust, bPad, TEXT("Hero")));
	if (!bPad)
	{
		Prompts.Add(FPrompt{ TEXT("1 - 5"), TEXT("Hero") });
	}
	Prompts.Add(MakePrompt(EPromptKey::Back, bPad, TEXT("Back")));
	return Prompts;
}

void UHeroSelectWidget::RefreshFooter()
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
	FooterPrompts->ClearChildren();
	for (int32 Index = 0; Index < Prompts.Num(); ++Index)
	{
		UHorizontalBoxSlot* HintSlot = FooterPrompts->AddChildToHorizontalBox(LoadoutParts::MakeKeyHint(WidgetTree, Prompts[Index].Key, Prompts[Index].Text, Index == 0));
		HintSlot->SetVerticalAlignment(VAlign_Center);
		HintSlot->SetPadding(FMargin(Index > 0 ? PromptGap : 0.f, 0.f, 0.f, 0.f));
	}
}

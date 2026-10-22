// UBountyBoardWidget: a bounty board's screen: opening and closing, the panel, its keys and buttons, taking, collecting
// and giving up. The cards are BountyBoardWidgetCards.cpp's.

#include "UI/World/BountyBoardWidget.h"
#include "Audio/LooterSound.h"
#include "Bounties/BountyBoard.h"
#include "Bounties/BountyBoardSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Economy/EconomyRules.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Missions/MissionRewards.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "LooterBounty"

using namespace LooterUI;

namespace
{
	const FName ActionAct(TEXT("Act"));
	const FName ActionGiveUp(TEXT("GiveUp"));
	const FName ActionClose(TEXT("Close"));

	/** Three cards and their gaps. */
	constexpr float PanelWidth = 1240.f;

	/** The clock's words (when new postings go up, an empty card's wait) are looked at this often (s). */
	constexpr float ClockSeconds = 1.f;

	/** The one board's screen up at a time. */
	TWeakObjectPtr<UBountyBoardWidget> OpenScreen;
}

// ---------------------------------------------------------------------------
// Opening and closing
// ---------------------------------------------------------------------------

UBountyBoardWidget* UBountyBoardWidget::OpenFor(APlayerController* InPlayer, ABountyBoard* InBoard)
{
	if (!InPlayer || !InPlayer->IsLocalController() || !InBoard)
	{
		return nullptr;
	}
	if (UBountyBoardWidget* Already = OpenScreen.Get())
	{
		Already->Close();
	}
	UBountyBoardWidget* Screen = CreateWidget<UBountyBoardWidget>(InPlayer, UBountyBoardWidget::StaticClass());
	if (!Screen)
	{
		return nullptr;
	}
	Screen->Board = InBoard;
	Screen->Player = InPlayer;
	// Its cards follow the board (bound here: CreateWidget initializes it before it knows its board).
	if (UBountyBoardSubsystem* Bounties = Screen->GetBounties())
	{
		Screen->ChangedHandle = Bounties->OnChanged.AddUObject(Screen, &UBountyBoardWidget::HandleChanged);
	}
	// Nothing fires behind the screen.
	if (const APawn* Pawn = InPlayer->GetPawn())
	{
		if (UWeaponManagerComponent* Manager = Pawn->FindComponentByClass<UWeaponManagerComponent>())
		{
			Manager->StopFire();
		}
	}
	// Choose the bounty under way first (to collect or give up), else the first posted.
	if (const UBountyBoardSubsystem* Bounties = UBountyBoardSubsystem::Get(InBoard))
	{
		const FBountyBoardRecord& Record = Bounties->GetRecord();
		const int32 Active = Record.FindActiveSlot();
		Screen->Selected = Active != INDEX_NONE ? Active : 0;
		if (Active == INDEX_NONE)
		{
			for (int32 CardIndex = 0; CardIndex < Record.Slots.Num(); ++CardIndex)
			{
				if (Record.Slots[CardIndex].IsPosting())
				{
					Screen->Selected = CardIndex;
					break;
				}
			}
		}
	}
	// Over the gameplay HUD (its floating pieces dim under the backdrop), with the keyboard and the mouse.
	Screen->AddToViewport(26);
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(Screen->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InPlayer->SetInputMode(InputMode);
	InPlayer->SetShowMouseCursor(true);
	OpenScreen = Screen;
	return Screen;
}

UBountyBoardWidget* UBountyBoardWidget::GetOpen()
{
	return OpenScreen.Get();
}

ABountyBoard* UBountyBoardWidget::GetBoard() const
{
	return Board.Get();
}

UBountyBoardSubsystem* UBountyBoardWidget::GetBounties() const
{
	return UBountyBoardSubsystem::Get(Board.Get());
}

void UBountyBoardWidget::Close()
{
	if (OpenScreen.Get() == this)
	{
		OpenScreen.Reset();
	}
	const bool bWasShown = IsInViewport();
	RemoveFromParent();
	if (APlayerController* PC = Player.Get())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
	if (bWasShown)
	{
		LooterSound::Play2D(this, LooterSoundCue::Close);
	}
}

void UBountyBoardWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// It holds the keyboard (A/D choose, E acts, R gives up, Escape closes); its buttons never take it.
	SetIsFocusable(true);
}

void UBountyBoardWidget::NativeDestruct()
{
	if (UBountyBoardSubsystem* Bounties = GetBounties())
	{
		Bounties->OnChanged.Remove(ChangedHandle);
	}
	Super::NativeDestruct();
}

bool UBountyBoardWidget::ShouldClose() const
{
	const ABountyBoard* Here = Board.Get();
	const APlayerController* PC = Player.Get();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Here || !Pawn || !Here->IsOpen())
	{
		return true;
	}
	const UHealthComponent* Health = Pawn->FindComponentByClass<UHealthComponent>();
	if (Health && Health->IsDead())
	{
		return true;
	}
	return FVector::Dist2D(Pawn->GetActorLocation(), Here->GetInteractionLocation().Get(Here->GetActorLocation())) > StayRadius;
}

// ---------------------------------------------------------------------------
// The panel
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UBountyBoardWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		// The whole screen answers the mouse, so a click beside the panel lands here and the keyboard stays with it.
		Root->SetVisibility(ESlateVisibility::Visible);
		WidgetTree->RootWidget = Root;

		// The street stays in view behind the board, dimmed.
		UImage* Backdrop = MakeImage(WidgetTree, RectBrush(Color::Backdrop()));
		MarkBackground(Backdrop);
		FillOverlaySlot(Root->AddChildToOverlay(Backdrop));

		PanelBox = MakeSized(WidgetTree, nullptr, PanelWidth);
		UOverlaySlot* PanelSlot = Root->AddChildToOverlay(PanelBox);
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
		BuildPanel();
	}
	return Super::RebuildWidget();
}

void UBountyBoardWidget::BuildPanel()
{
	if (!PanelBox)
	{
		return;
	}
	const ABountyBoard* Here = Board.Get();
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	// The board's line on the left; how many it has paid and when new postings go up on the right.
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	SubtitleText = MakeText(WidgetTree, Here ? Here->Subtitle.ToString() : FString(), 12, Color::TextDim());
	UHorizontalBoxSlot* SubtitleSlot = Header->AddChildToHorizontalBox(SubtitleText);
	SubtitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	SubtitleSlot->SetVerticalAlignment(VAlign_Center);
	PaidText = MakeText(WidgetTree, FString(), 11, Color::TextDim(), true, 120);
	UHorizontalBoxSlot* PaidSlot = Header->AddChildToHorizontalBox(PaidText);
	PaidSlot->SetVerticalAlignment(VAlign_Center);
	PaidSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
	RotationText = MakeText(WidgetTree, FString(), 12, Color::CyanText(), true, 100);
	UHorizontalBoxSlot* RotationSlot = Header->AddChildToHorizontalBox(RotationText);
	RotationSlot->SetVerticalAlignment(VAlign_Center);
	RotationSlot->SetPadding(FMargin(20.f, 0.f, 0.f, 0.f));
	Content->AddChildToVerticalBox(Header);

	// The cards.
	CardRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Content->AddChildToVerticalBox(CardRow)->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));

	// What just happened, or why not.
	StatusText = MakeText(WidgetTree, FString(), 13, Color::Text());
	StatusText->SetAutoWrapText(true);
	StatusText->SetJustification(ETextJustify::Center);
	USizeBox* StatusBox = MakeSized(WidgetTree, StatusText, 0.f);
	StatusBox->SetMinDesiredHeight(26.f);
	Content->AddChildToVerticalBox(StatusBox)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));

	// The keys, and the buttons for the mouse.
	UHorizontalBox* Bottom = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	const auto AddHint = [this, Bottom](const TCHAR* Key, const FText& Words, bool bPrimary, float Gap)
	{
		UHorizontalBoxSlot* HintSlot = Bottom->AddChildToHorizontalBox(LoadoutParts::MakeKeyHint(WidgetTree, Key, Words.ToString(), bPrimary));
		HintSlot->SetVerticalAlignment(VAlign_Center);
		HintSlot->SetPadding(FMargin(Gap, 0.f, 0.f, 0.f));
	};
	AddHint(TEXT("A / D"), LOCTEXT("HintChoose", "Choose"), false, 0.f);
	AddHint(TEXT("E"), LOCTEXT("HintAct", "Take / Collect"), true, 16.f);
	AddHint(TEXT("R"), LOCTEXT("HintGiveUp", "Give it up"), false, 16.f);
	AddHint(TEXT("Esc"), LOCTEXT("HintClose", "Close"), false, 16.f);
	Bottom->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, 0.f))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Bottom->AddChildToHorizontalBox(ButtonRow)->SetVerticalAlignment(VAlign_Center);
	Content->AddChildToVerticalBox(Bottom)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));

	PanelBox->SetContent(MakePanel(WidgetTree, Here ? Here->Title.ToString() : LOCTEXT("BoardScreenTitle", "Bounties").ToString(), Content));
	MakeButtons();
	ShowCards(/*bForce*/ true);
}

void UBountyBoardWidget::MakeButtons()
{
	if (!ButtonRow)
	{
		return;
	}
	ButtonRow->ClearChildren();
	const FBountyCardView* Card = Shown.IsValidIndex(Selected) ? &Shown[Selected] : nullptr;
	const bool bCollect = Card && Card->bCanCollect;
	const bool bCanAct = Card && (Card->bCanTake || Card->bCanCollect);
	const auto AddButton = [this](FName Action, const FText& Words, EButtonKind Kind, float Width, float Gap)
	{
		ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
		Button->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), Action, 0, Words, 13, Kind);
		Button->OnButtonClicked.BindUObject(this, &UBountyBoardWidget::HandleButton);
		UHorizontalBoxSlot* ButtonSlot = ButtonRow->AddChildToHorizontalBox(MakeSized(WidgetTree, Button, Width, 36.f));
		ButtonSlot->SetVerticalAlignment(VAlign_Center);
		ButtonSlot->SetPadding(FMargin(Gap, 0.f, 0.f, 0.f));
	};
	AddButton(ActionAct, bCollect ? LOCTEXT("Collect", "Collect") : LOCTEXT("Take", "Take it"), bCanAct ? EButtonKind::Primary : EButtonKind::Mini,
		150.f, 0.f);
	if (Card && Card->bCanGiveUp)
	{
		AddButton(ActionGiveUp, LOCTEXT("GiveUp", "Give it up"), EButtonKind::Danger, 140.f, 10.f);
	}
	AddButton(ActionClose, LOCTEXT("Close", "Close"), EButtonKind::Normal, 110.f, 10.f);
}

void UBountyBoardWidget::SetStatus(const FText& Words, bool bBad)
{
	Status = Words;
	bStatusBad = bBad;
	if (StatusText)
	{
		StatusText->SetText(Words);
		StatusText->SetColorAndOpacity(FSlateColor(bBad ? Color::Worse() : Color::Text()));
	}
}

void UBountyBoardWidget::HandleChanged()
{
	ShowCards(/*bForce*/ false);
}

// ---------------------------------------------------------------------------
// Acting
// ---------------------------------------------------------------------------

void UBountyBoardWidget::Select(int32 CardIndex)
{
	const int32 Count = FMath::Max(BountyRules::SlotCount, 1);
	const int32 Wrapped = ((CardIndex % Count) + Count) % Count;
	if (Wrapped != Selected)
	{
		Selected = Wrapped;
		bGiveUpArmed = false;
		LooterSound::Play2D(this, LooterSoundCue::Click);
		ShowCards(/*bForce*/ false);
	}
}

bool UBountyBoardWidget::ActOnSelected()
{
	UBountyBoardSubsystem* Bounties = GetBounties();
	const FBountyCardView* Card = Shown.IsValidIndex(Selected) ? &Shown[Selected] : nullptr;
	if (!Bounties || !Card || Card->bEmpty)
	{
		SetStatus(LOCTEXT("NothingHere", "Nothing posted there yet."), true);
		return false;
	}
	bGiveUpArmed = false;
	if (Card->bCanCollect)
	{
		FMissionRewardsGiven Given;
		APlayerController* PC = Player.Get();
		if (Bounties->Collect(PC ? PC->GetPawn() : nullptr, &Given))
		{
			SetStatus(FText::Format(LOCTEXT("Paid", "Paid: +{0} XP · +{1} silver. Your strongbox waits where it fell."),
				FText::AsNumber(Given.Experience), FText::FromString(LooterEconomy::Format(Given.Silver))), false);
			ShowCards(/*bForce*/ true);
			return true;
		}
	}
	else if (Card->bCanTake)
	{
		// Copied first: taking it remakes the cards (Card points into them).
		const FText Mark = Card->Title;
		const FText Where = Card->Where;
		if (Bounties->Take(Selected))
		{
			SetStatus(FText::Format(LOCTEXT("Taken", "Taken. {0} is on your tracker. {1}."), Mark, Where), false);
			ShowCards(/*bForce*/ true);
			return true;
		}
	}
	else if (Card->State == EBountyState::Posted)
	{
		SetStatus(LOCTEXT("OneAtATime", "One bounty at a time: finish yours, or give it up."), true);
		LooterSound::Play2D(this, LooterSoundCue::Close);
		return false;
	}
	else
	{
		SetStatus(LOCTEXT("Hunting", "Under way. Come back with proof."), false);
	}
	return false;
}

bool UBountyBoardWidget::GiveUpSelected()
{
	UBountyBoardSubsystem* Bounties = GetBounties();
	const FBountyCardView* Card = Shown.IsValidIndex(Selected) ? &Shown[Selected] : nullptr;
	if (!Bounties || !Card || !Card->bCanGiveUp)
	{
		SetStatus(LOCTEXT("CantGiveUp", "Only a bounty you've taken, and not finished, can be given up."), true);
		return false;
	}
	// Twice, so a slip of the finger never throws a hunt away.
	if (!bGiveUpArmed)
	{
		bGiveUpArmed = true;
		SetStatus(LOCTEXT("GiveUpAsk", "Give it up? Its posse scatters and the paper goes back up. Press R again."), true);
		return false;
	}
	bGiveUpArmed = false;
	if (Bounties->GiveUp())
	{
		SetStatus(LOCTEXT("GaveUp", "Given up. The paper's back on the board."), false);
		ShowCards(/*bForce*/ true);
		return true;
	}
	return false;
}

void UBountyBoardWidget::HandleButton(ULooterButton* Button)
{
	if (!Button)
	{
		return;
	}
	if (Button->Action == ActionClose)
	{
		Close();
		return;
	}
	if (Button->Action == ActionAct)
	{
		ActOnSelected();
	}
	else if (Button->Action == ActionGiveUp)
	{
		GiveUpSelected();
	}
	else if (Button->Action == FName(TEXT("Card")))
	{
		// A click on a card chooses it; a click on the chosen one acts on it.
		if (Button->Index == Selected)
		{
			ActOnSelected();
		}
		else
		{
			Select(Button->Index);
		}
	}
	// A click handed the keyboard to the game viewport (LooterButton): take it back so the keys keep reaching the screen.
	if (IsInViewport())
	{
		SetKeyboardFocus();
	}
}

void UBountyBoardWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (ShouldClose())
	{
		Close();
		return;
	}
	// The clock's words move on (when new postings go up, an empty slot's wait).
	SinceClock += InDeltaTime;
	if (SinceClock >= ClockSeconds)
	{
		SinceClock = 0.f;
		ShowCards(/*bForce*/ false);
	}
}

FReply UBountyBoardWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		Close();
		return FReply::Handled();
	}
	if (Key == EKeys::A || Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftShoulder)
	{
		Select(Selected - 1);
		return FReply::Handled();
	}
	if (Key == EKeys::D || Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_RightShoulder)
	{
		Select(Selected + 1);
		return FReply::Handled();
	}
	// The Interact key that opened it may still be repeating: only a fresh press acts.
	if (!InKeyEvent.IsRepeat())
	{
		if (Key == EKeys::E || Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
		{
			ActOnSelected();
			return FReply::Handled();
		}
		if (Key == EKeys::R || Key == EKeys::Gamepad_FaceButton_Left)
		{
			GiveUpSelected();
			return FReply::Handled();
		}
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UBountyBoardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// A click on no button ends here rather than in the game viewport, which would take the keyboard from the screen.
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

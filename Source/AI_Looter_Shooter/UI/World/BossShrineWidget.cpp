// UBossShrineWidget: a vigil stone's menu: the panel, its words by the frame, its keys and buttons, opening and closing.

#include "UI/World/BossShrineWidget.h"
#include "Audio/LooterSound.h"
#include "Combat/HealthComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "World/BossShrine.h"
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

#define LOCTEXT_NAMESPACE "LooterBossShrine"

using namespace LooterUI;

namespace
{
	const FName ActionCall(TEXT("Call"));
	const FName ActionClose(TEXT("Close"));

	constexpr float PanelWidth = 600.f;
	constexpr float LabelWidth = 190.f;
	/** A refused press's flash (s). */
	constexpr float RefusedSeconds = 0.6f;

	/** The one stone's menu up at a time. */
	TWeakObjectPtr<UBossShrineWidget> OpenMenu;

	/** The status line's colour for a state. */
	FLinearColor StatusColor(EBossShrineState State)
	{
		switch (State)
		{
		case EBossShrineState::Ready:
			return Color::Better();
		case EBossShrineState::Waiting:
			return Color::Text();
		default:
			return Color::TextDim();
		}
	}
}

// ---------------------------------------------------------------------------
// Opening and closing
// ---------------------------------------------------------------------------

UBossShrineWidget* UBossShrineWidget::OpenFor(APlayerController* Player, ABossShrine* Shrine)
{
	if (!Player || !Player->IsLocalController() || !Shrine)
	{
		return nullptr;
	}
	if (UBossShrineWidget* Already = OpenMenu.Get())
	{
		Already->Close();
	}
	UBossShrineWidget* Menu = CreateWidget<UBossShrineWidget>(Player, UBossShrineWidget::StaticClass());
	if (!Menu)
	{
		return nullptr;
	}
	Menu->Shrine = Shrine;
	Menu->Player = Player;
	// Nothing fires behind the menu.
	if (const APawn* Pawn = Player->GetPawn())
	{
		if (UWeaponManagerComponent* Manager = Pawn->FindComponentByClass<UWeaponManagerComponent>())
		{
			Manager->StopFire();
		}
	}
	// Over the gameplay HUD (its floating pieces dim under the backdrop), with the keyboard and the mouse.
	Menu->AddToViewport(26);
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(Menu->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Player->SetInputMode(InputMode);
	Player->SetShowMouseCursor(true);
	Menu->ShowView(Shrine->GetView());
	OpenMenu = Menu;
	return Menu;
}

UBossShrineWidget* UBossShrineWidget::GetOpen()
{
	return OpenMenu.Get();
}

ABossShrine* UBossShrineWidget::GetShrine() const
{
	return Shrine.Get();
}

void UBossShrineWidget::Close()
{
	if (OpenMenu.Get() == this)
	{
		OpenMenu.Reset();
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

void UBossShrineWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// It holds the keyboard (E rings, Escape closes); its buttons never take it.
	SetIsFocusable(true);
}

bool UBossShrineWidget::ShouldClose() const
{
	const ABossShrine* Stone = Shrine.Get();
	const APlayerController* PC = Player.Get();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Stone || !Pawn)
	{
		return true;
	}
	const UHealthComponent* Health = Pawn->FindComponentByClass<UHealthComponent>();
	if (Health && Health->IsDead())
	{
		return true;
	}
	const FVector At = Stone->GetInteractionLocation().Get(Stone->GetActorLocation());
	return FVector::Dist2D(Pawn->GetActorLocation(), At) > StayRadius;
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UBossShrineWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		// The whole screen answers the mouse, so a click beside the panel lands here and the keyboard stays with it.
		Root->SetVisibility(ESlateVisibility::Visible);
		WidgetTree->RootWidget = Root;

		// The world stays in view behind the stone's panel, dimmed.
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

UWidget* UBossShrineWidget::MakeInfoRow(const FText& Label, TObjectPtr<UTextBlock>& OutValue)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeText(WidgetTree, Label.ToString(), 10,
		Color::TextDim(), true, 150), LabelWidth));
	LabelSlot->SetVerticalAlignment(VAlign_Center);
	OutValue = MakeText(WidgetTree, FString(), 13, Color::Text());
	OutValue->SetAutoWrapText(true);
	UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(OutValue);
	ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ValueSlot->SetVerticalAlignment(VAlign_Center);
	return MakeRow(WidgetTree, Row);
}

void UBossShrineWidget::BuildPanel()
{
	if (!PanelBox)
	{
		return;
	}
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	// The boss: its name large, its level beside it.
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	NameText = MakeText(WidgetTree, FString(), 22, Color::Title(), true, 60);
	NameText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
	UHorizontalBoxSlot* NameSlot = Header->AddChildToHorizontalBox(NameText);
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Bottom);
	LevelText = MakeText(WidgetTree, FString(), 14, Color::CyanText(), true, 120);
	UHorizontalBoxSlot* LevelSlot = Header->AddChildToHorizontalBox(LevelText);
	LevelSlot->SetVerticalAlignment(VAlign_Bottom);
	LevelSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 3.f));
	Content->AddChildToVerticalBox(Header);

	UTextBlock* Lore = MakeText(WidgetTree, LOCTEXT("Lore", "Keepers sit the night with their dead by candlelight. Light the candles "
		"and ring the bell, and the laid-to-rest walk again.").ToString(), 11, Color::TextDim());
	Lore->SetAutoWrapText(true);
	Content->AddChildToVerticalBox(Lore)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));

	// Its record.
	Content->AddChildToVerticalBox(MakeSection(WidgetTree, LOCTEXT("Record", "Record").ToString()))->SetPadding(FMargin(0.f, 14.f, 0.f, 6.f));
	Content->AddChildToVerticalBox(MakeInfoRow(LOCTEXT("LastKill", "Last laid to rest"), LastKillText));
	Content->AddChildToVerticalBox(MakeInfoRow(LOCTEXT("Times", "Times beaten"), TimesText))->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	Content->AddChildToVerticalBox(MakeInfoRow(LOCTEXT("Drops", "Drops"), LootText))->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	UTextBlock* Same = MakeText(WidgetTree, LOCTEXT("SameLoot", "Called back, it drops just what it did the first time.")
		.ToString(), 10, Color::TextDim());
	Same->SetAutoWrapText(true);
	Content->AddChildToVerticalBox(Same)->SetPadding(FMargin(LabelWidth + 12.f, 4.f, 0.f, 0.f));

	// The call: the countdown (while it waits), the status, the rule.
	CountdownText = MakeText(WidgetTree, FString(), 34, Color::Accent(), false, 80);
	CountdownText->SetJustification(ETextJustify::Center);
	Content->AddChildToVerticalBox(CountdownText)->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	StatusText = MakeText(WidgetTree, FString(), 13, Color::Text());
	StatusText->SetAutoWrapText(true);
	StatusText->SetJustification(ETextJustify::Center);
	USizeBox* StatusBox = MakeSized(WidgetTree, StatusText, 0.f);
	StatusBox->SetMinDesiredHeight(38.f);
	Content->AddChildToVerticalBox(StatusBox)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	UTextBlock* Rule = MakeText(WidgetTree, LOCTEXT("Rule", "The bell answers five minutes after its last rest, or at once after a "
		"Save & Quit.").ToString(), 10, Color::TextDim());
	Rule->SetAutoWrapText(true);
	Rule->SetJustification(ETextJustify::Center);
	Content->AddChildToVerticalBox(Rule)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));

	// The keys, and the buttons for the mouse.
	UHorizontalBox* Bottom = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* CallHint = Bottom->AddChildToHorizontalBox(LoadoutParts::MakeKeyHint(WidgetTree, TEXT("E"),
		LOCTEXT("HintCall", "Call it back").ToString(), true));
	CallHint->SetVerticalAlignment(VAlign_Center);
	UHorizontalBoxSlot* CloseHint = Bottom->AddChildToHorizontalBox(LoadoutParts::MakeKeyHint(WidgetTree, TEXT("Esc"),
		LOCTEXT("HintClose", "Close").ToString(), false));
	CloseHint->SetVerticalAlignment(VAlign_Center);
	CloseHint->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
	Bottom->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, 0.f))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Bottom->AddChildToHorizontalBox(ButtonRow)->SetVerticalAlignment(VAlign_Center);
	Content->AddChildToVerticalBox(Bottom)->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));

	PanelBox->SetContent(MakePanel(WidgetTree, LOCTEXT("Title", "Vigil Stone").ToString(), Content));
	bButtonsMade = false;
	if (const ABossShrine* Stone = Shrine.Get())
	{
		ShowView(Stone->GetView());
	}
}

void UBossShrineWidget::MakeButtons(bool bCanCall)
{
	if (!ButtonRow)
	{
		return;
	}
	ButtonRow->ClearChildren();
	// Ready: the orange call to action. Not ready: a quiet one that refuses with the stone's sound (the stone plays both
	// sounds, so the button makes none of its own).
	ULooterButton* Call = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Call->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), ActionCall, 0, LOCTEXT("Call", "Call it back"), 13,
		bCanCall ? EButtonKind::Primary : EButtonKind::Mini);
	Call->bPlaysSounds = false;
	Call->OnButtonClicked.BindUObject(this, &UBossShrineWidget::HandleButton);
	ButtonRow->AddChildToHorizontalBox(MakeSized(WidgetTree, Call, 170.f, 36.f))->SetVerticalAlignment(VAlign_Center);

	ULooterButton* Leave = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Leave->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), ActionClose, 0, LOCTEXT("Close", "Close"), 12,
		EButtonKind::Normal);
	Leave->OnButtonClicked.BindUObject(this, &UBossShrineWidget::HandleButton);
	UHorizontalBoxSlot* LeaveSlot = ButtonRow->AddChildToHorizontalBox(MakeSized(WidgetTree, Leave, 110.f, 36.f));
	LeaveSlot->SetVerticalAlignment(VAlign_Center);
	LeaveSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
	bButtonsMade = true;
	bButtonsReady = bCanCall;
}

void UBossShrineWidget::ShowView(const FBossShrineView& View)
{
	ShownState = View.State;
	if (!NameText)
	{
		return;
	}
	NameText->SetText(View.BossName);
	LevelText->SetText(View.Level > 0 ? FText::Format(LOCTEXT("Level", "Level {0}"), FText::AsNumber(View.Level)) : FText::GetEmpty());
	LastKillText->SetText(View.LastKill);
	TimesText->SetText(FText::AsNumber(View.TimesBeaten));
	LootText->SetText(View.LootHint.IsEmpty() ? LOCTEXT("NoHint", "Its own, as the first time") : View.LootHint);
	const bool bWaiting = View.State == EBossShrineState::Waiting;
	CountdownText->SetText(bWaiting ? BossShrineRules::Countdown(View.SecondsLeft) : FText::GetEmpty());
	CountdownText->SetVisibility(bWaiting ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	StatusText->SetText(View.Status);
	StatusText->SetColorAndOpacity(FSlateColor(RefusedFlash > 0.f ? Color::Worse() : StatusColor(View.State)));
	if (!bButtonsMade || bButtonsReady != View.CanCall())
	{
		MakeButtons(View.CanCall());
	}
}

// ---------------------------------------------------------------------------
// The call, keys and buttons
// ---------------------------------------------------------------------------

bool UBossShrineWidget::TryCall()
{
	ABossShrine* Stone = Shrine.Get();
	APlayerController* PC = Player.Get();
	if (!Stone)
	{
		return false;
	}
	if (Stone->CallBack(PC ? PC->GetPawn() : nullptr))
	{
		// The bell rings out; the panel steps aside so the player sees the candles go out and hears the answer.
		Close();
		return true;
	}
	RefusedFlash = RefusedSeconds;
	ShowView(Stone->GetView());
	return false;
}

void UBossShrineWidget::HandleButton(ULooterButton* Button)
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
	if (Button->Action == ActionCall)
	{
		TryCall();
	}
	// A click handed the keyboard to the game viewport (LooterButton): take it back so the keys keep reaching the menu.
	if (IsInViewport())
	{
		SetKeyboardFocus();
	}
}

void UBossShrineWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (ShouldClose())
	{
		Close();
		return;
	}
	RefusedFlash = FMath::Max(RefusedFlash - InDeltaTime, 0.f);
	// The countdown runs by the frame; the boss may come or go while it's up.
	if (const ABossShrine* Stone = Shrine.Get())
	{
		ShowView(Stone->GetView());
	}
}

FReply UBossShrineWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		Close();
		return FReply::Handled();
	}
	// The Interact key that opened it may still be repeating: only a fresh press rings.
	if (!InKeyEvent.IsRepeat()
		&& (Key == EKeys::E || Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom))
	{
		TryCall();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UBossShrineWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// A click on no button ends here rather than in the game viewport, which would take the keyboard from the menu.
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

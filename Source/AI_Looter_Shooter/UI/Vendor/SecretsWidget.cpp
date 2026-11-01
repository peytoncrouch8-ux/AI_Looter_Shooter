// USecretsWidget: Sexton's secrets screen: opening and closing, the panel, choosing, the keys and the mouse, buying, the
// purse. The rows and a buy's motion are SecretsWidgetRows.cpp's.

#include "UI/Vendor/SecretsWidget.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Combat/HealthComponent.h"
#include "Economy/EconomyRules.h"
#include "Economy/SecretBroker.h"
#include "Economy/SecretCatalog.h"
#include "Economy/SecretSubsystem.h"
#include "Economy/WalletSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Vendor/SilverUI.h"
#include "World/FindRules.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "LooterSecrets"

using namespace LooterUI;

namespace
{
	const FName SecretsActionRow(TEXT("SecretsRow"));
	const FName SecretsActionClose(TEXT("SecretsClose"));

	/** How fast the purse counts to the silver carried, how long its flash lasts (s), how much it pops. */
	constexpr float SecretsPurseSpeed = 9.f;
	constexpr float SecretsPurseFlashSeconds = 0.6f;
	constexpr float SecretsPursePop = 0.12f;

	/** The tink over the coins: a little quieter, so the counter's coins lead. */
	constexpr float SecretsGlintVolume = 0.7f;

	/** The one secrets screen up at a time. */
	TWeakObjectPtr<USecretsWidget> OpenSecrets;
}

// ---------------------------------------------------------------------------
// Opening and closing
// ---------------------------------------------------------------------------

USecretsWidget* USecretsWidget::OpenFor(APlayerController* InPlayer, USecretBrokerComponent* InBroker, bool bInAnywhere)
{
	if (!InPlayer || !InPlayer->IsLocalController() || !InBroker)
	{
		return nullptr;
	}
	if (USecretsWidget* Already = OpenSecrets.Get())
	{
		Already->Close();
	}
	USecretsWidget* Screen = CreateWidget<USecretsWidget>(InPlayer, USecretsWidget::StaticClass());
	if (!Screen)
	{
		return nullptr;
	}
	Screen->Broker = InBroker;
	Screen->Player = InPlayer;
	Screen->bAnywhere = bInAnywhere;
	Screen->PurseTarget = UWalletSubsystem::SilverOf(InBroker);
	Screen->ShownSilver = static_cast<float>(Screen->PurseTarget);
	if (const USecretSubsystem* Secrets = USecretSubsystem::Get(InBroker))
	{
		Screen->Rows = Secrets->GetRows();
	}
	// Nothing fires behind the screen.
	if (const APawn* Pawn = InPlayer->GetPawn())
	{
		if (UWeaponManagerComponent* Manager = Pawn->FindComponentByClass<UWeaponManagerComponent>())
		{
			Manager->StopFire();
		}
	}
	// Over the gameplay HUD (it dims under the backdrop), with the keyboard and the mouse.
	Screen->AddToViewport(26);
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(Screen->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InPlayer->SetInputMode(InputMode);
	InPlayer->SetShowMouseCursor(true);
	LooterSound::Play2D(Screen, LooterSoundCue::Open);
	OpenSecrets = Screen;
	return Screen;
}

USecretsWidget* USecretsWidget::GetOpen()
{
	return OpenSecrets.Get();
}

void USecretsWidget::Close()
{
	if (OpenSecrets.Get() == this)
	{
		OpenSecrets.Reset();
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

void USecretsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// It holds the keyboard (W/S choose, E buys, Escape closes); its rows never take it.
	SetIsFocusable(true);
}

USecretSubsystem* USecretsWidget::GetSecrets() const
{
	return USecretSubsystem::Get(Broker.Get());
}

bool USecretsWidget::ShouldClose() const
{
	const USecretBrokerComponent* Here = Broker.Get();
	const AActor* Seller = Here ? Here->GetOwner() : nullptr;
	const APlayerController* PC = Player.Get();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Seller || !Pawn || Seller->IsHidden())
	{
		return true;
	}
	const UHealthComponent* Health = Pawn->FindComponentByClass<UHealthComponent>();
	if (Health && Health->IsDead())
	{
		return true;
	}
	return !bAnywhere && FVector::Dist(Pawn->GetActorLocation(), Seller->GetActorLocation()) > StayRadius;
}

// ---------------------------------------------------------------------------
// The panel
// ---------------------------------------------------------------------------

TSharedRef<SWidget> USecretsWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		// The whole screen answers the mouse, so a click beside the panel lands here and the keyboard stays with it.
		Root->SetVisibility(ESlateVisibility::Visible);
		WidgetTree->RootWidget = Root;

		// The fog and the ferry stay in view behind the list, dimmed.
		UImage* Backdrop = MakeImage(WidgetTree, RectBrush(Color::Backdrop()));
		MarkBackground(Backdrop);
		FillOverlaySlot(Root->AddChildToOverlay(Backdrop));

		PanelBox = MakeSized(WidgetTree, nullptr, SecretsLayout::PanelWidth);
		UOverlaySlot* PanelSlot = Root->AddChildToOverlay(PanelBox);
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
		BuildPanel();
	}
	return Super::RebuildWidget();
}

void USecretsWidget::Refresh(FName KeepId)
{
	if (const USecretSubsystem* Secrets = GetSecrets())
	{
		Rows = Secrets->GetRows();
	}
	const int32 Kept = Rows.IndexOfByPredicate([KeepId](const FSecretRow& Row) { return Row.Id == KeepId; });
	Selected = Kept != INDEX_NONE ? Kept : FMath::Clamp(Selected, 0, FMath::Max(Rows.Num() - 1, 0));
	BuildPanel();
}

void USecretsWidget::BuildPanel()
{
	if (!PanelBox)
	{
		return;
	}
	const USecretSubsystem* Secrets = GetSecrets();
	const USecretCatalog* Catalog = Secrets ? Secrets->GetCatalog() : nullptr;
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	// His title and his line for the story on the left; the purse on the right.
	{
		UHorizontalBox* Head = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		const FText Title = Catalog && !Catalog->Title.IsEmpty() ? Catalog->Title : LOCTEXT("Title", "Secrets");
		Words->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, Title.ToString(), 18, Color::Title(), 160));
		const FText Greeting = Secrets ? Secrets->GetGreeting() : FText::GetEmpty();
		if (!Greeting.IsEmpty())
		{
			UTextBlock* Said = MakeText(WidgetTree, FString::Printf(TEXT("“%s”"), *Greeting.ToString()), 13, Color::TextDim());
			Said->SetAutoWrapText(true);
			Words->AddChildToVerticalBox(Said)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
		}
		UHorizontalBoxSlot* WordsSlot = Head->AddChildToHorizontalBox(Words);
		WordsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		WordsSlot->SetVerticalAlignment(VAlign_Center);
		WordsSlot->SetPadding(FMargin(0.f, 0.f, 24.f, 0.f));

		UVerticalBox* Purse = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Purse->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, LOCTEXT("YourSilver", "Your silver").ToString(), 9, Color::TextDim(), 250))
			->SetHorizontalAlignment(HAlign_Right);
		UTextBlock* Amount = nullptr;
		UWidget* Line = SilverUI::MakeAmount(WidgetTree, FMath::RoundToInt32(ShownSilver), 24, Color::Text(), false, &Amount);
		Line->SetRenderTransformPivot(FVector2D(1.f, 0.5f));
		Purse->AddChildToVerticalBox(Line)->SetHorizontalAlignment(HAlign_Right);
		PurseText = Amount;
		PurseLine = Line;
		const float Discount = Secrets ? Secrets->GetDiscount() : 0.f;
		if (Discount > 0.f)
		{
			// Paid on Delivery's reward, said where the prices are read.
			const FText Off = FMath::IsNearlyEqual(Discount, 0.25f) ? LOCTEXT("QuarterOff", "A quarter off")
				: FText::Format(LOCTEXT("PercentOff", "{0}% off"), FText::AsNumber(FMath::RoundToInt32(Discount * 100.f)));
			Purse->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, Off.ToString(), 10, Color::SoulGold(), 200))
				->SetHorizontalAlignment(HAlign_Right);
		}
		Head->AddChildToHorizontalBox(Purse)->SetVerticalAlignment(VAlign_Center);
		Content->AddChildToVerticalBox(Head);
	}

	// The list: for sale, then marked; it scrolls once long.
	{
		UVerticalBox* List = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		BuildRows(List);
		ListScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
		StyleScrollBox(ListScroll);
		ListScroll->AddChild(List);
		USizeBox* ListBox = MakeSized(WidgetTree, ListScroll, 0.f);
		ListBox->SetMaxDesiredHeight(SecretsLayout::ListHeight);
		Content->AddChildToVerticalBox(ListBox)->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));
	}

	// What the last buy did; it keeps its room when empty, so nothing under it jumps.
	StatusText = MakeText(WidgetTree, Status.ToString(), 13, StatusColor);
	StatusText->SetAutoWrapText(true);
	USizeBox* StatusBox = MakeSized(WidgetTree, StatusText, 0.f);
	StatusBox->SetMinDesiredHeight(22.f);
	Content->AddChildToVerticalBox(StatusBox)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));

	// The keys, and a button to close it for the mouse.
	{
		UHorizontalBox* Bottom = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		const auto AddHint = [this, Bottom](const TCHAR* Key, const FText& Does, bool bPrimary, float Gap)
		{
			UHorizontalBoxSlot* HintSlot = Bottom->AddChildToHorizontalBox(LoadoutParts::MakeKeyHint(WidgetTree, Key, Does.ToString(), bPrimary));
			HintSlot->SetVerticalAlignment(VAlign_Center);
			HintSlot->SetPadding(FMargin(Gap, 0.f, 0.f, 0.f));
		};
		AddHint(TEXT("E"), LOCTEXT("HintBuy", "Buy"), true, 0.f);
		AddHint(TEXT("W/S"), LOCTEXT("HintSelect", "Select"), false, 14.f);
		AddHint(TEXT("Esc"), LOCTEXT("HintClose", "Close"), false, 14.f);
		Bottom->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, 0.f))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ULooterButton* CloseButton = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
		CloseButton->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), SecretsActionClose, 0, LOCTEXT("Close", "Close"), 12,
			EButtonKind::Normal);
		CloseButton->OnButtonClicked.BindUObject(this, &USecretsWidget::HandleButton);
		Bottom->AddChildToHorizontalBox(MakeSized(WidgetTree, CloseButton, 120.f, 34.f))->SetVerticalAlignment(VAlign_Center);
		Content->AddChildToVerticalBox(Bottom)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	}

	const FText Tab = Catalog && !Catalog->Tab.IsEmpty() ? Catalog->Tab : LOCTEXT("Tab", "Secrets");
	PanelBox->SetContent(MakePanel(WidgetTree, Tab.ToString(), Content));
	Restyle();
	if (ListScroll && Parts.IsValidIndex(Selected) && Parts[Selected].Button)
	{
		ListScroll->ScrollWidgetIntoView(Parts[Selected].Button, /*AnimateScroll*/ false);
	}
}

void USecretsWidget::SetStatus(const FText& Words, const FLinearColor& TextColor)
{
	Status = Words;
	StatusColor = TextColor;
	if (StatusText)
	{
		StatusText->SetText(Words);
		StatusText->SetColorAndOpacity(FSlateColor(TextColor));
	}
}

// ---------------------------------------------------------------------------
// Choosing and buying
// ---------------------------------------------------------------------------

void USecretsWidget::Select(int32 RowIndex, bool bSound)
{
	if (Rows.IsEmpty())
	{
		return;
	}
	const int32 Wrapped = ((RowIndex % Rows.Num()) + Rows.Num()) % Rows.Num();
	if (Wrapped == Selected)
	{
		return;
	}
	Selected = Wrapped;
	if (bSound)
	{
		LooterSound::Play2D(this, LooterSoundCue::Hover);
	}
	Restyle();
	if (ListScroll && Parts.IsValidIndex(Selected) && Parts[Selected].Button)
	{
		ListScroll->ScrollWidgetIntoView(Parts[Selected].Button, /*AnimateScroll*/ true);
	}
}

bool USecretsWidget::BuySelected()
{
	USecretSubsystem* Secrets = GetSecrets();
	if (!Secrets || !Rows.IsValidIndex(Selected))
	{
		LooterSound::Play2D(this, LooterSoundCue::Denied);
		return false;
	}
	// Copied: buying remakes the rows.
	const FSecretRow Row = Rows[Selected];
	if (Row.State == ESecretState::Sold)
	{
		// Bought already: his word again, where it's marked.
		SetStatus(FText::Format(LOCTEXT("Already", "Yours already: “{0}”"), Row.Line), Color::SoulGold());
		LooterSound::Play2D(this, LooterSoundCue::Click);
		return false;
	}
	switch (Secrets->Buy(Row.Id))
	{
	case ESecretBuy::Bought:
		// Coins on the counter and a high tink: the find's brass catching the light, as its glint will out there.
		LooterSound::Play2D(this, LooterSoundCue::Economy::Buy);
		LooterSound::Play2D(this, FindCue::Glint, SecretsGlintVolume);
		FlashId = Row.Id;
		FlashAge = 0.f;
		SetStatus(SecretRules::MarkedWords(Row.Title, Row.Places), Color::SoulGold());
		Refresh(Row.Id);
		return true;
	case ESecretBuy::NotEnoughSilver:
		LooterSound::Play2D(this, LooterSoundCue::Economy::NotEnough);
		SetStatus(SecretRules::ShortWords(Row.Price, UWalletSubsystem::SilverOf(this)), Color::Worse());
		return false;
	case ESecretBuy::AlreadySold:
	case ESecretBuy::NotForSale:
	case ESecretBuy::NoTrade:
		break;
	}
	LooterSound::Play2D(this, LooterSoundCue::Denied);
	SetStatus(LOCTEXT("NotNow", "He won't sell that now."), Color::Worse());
	Refresh(NAME_None);
	return false;
}

void USecretsWidget::HandleButton(ULooterButton* Button)
{
	if (!Button)
	{
		return;
	}
	if (Button->Action == SecretsActionClose)
	{
		Close();
		return;
	}
	if (Button->Action == SecretsActionRow)
	{
		// A click on a row chooses it; a click on the chosen one buys it.
		if (Button->Index == Selected)
		{
			BuySelected();
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

void USecretsWidget::HandleHovered(ULooterButton* Button)
{
	if (Button && Button->Action == SecretsActionRow)
	{
		Select(Button->Index);
	}
}

// ---------------------------------------------------------------------------
// The clock: closing, the purse, a buy's motion
// ---------------------------------------------------------------------------

void USecretsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (ShouldClose())
	{
		Close();
		return;
	}
	UpdatePurse(InDeltaTime);
	UpdateMotion(InDeltaTime);
}

void USecretsWidget::UpdatePurse(float DeltaTime)
{
	const int32 Now = UWalletSubsystem::SilverOf(this);
	if (Now != PurseTarget)
	{
		bPurseGain = Now > PurseTarget;
		PurseTarget = Now;
		PurseFlash = 1.f;
	}
	const int32 Before = FMath::RoundToInt32(ShownSilver);
	ShownSilver = FMath::FInterpTo(ShownSilver, static_cast<float>(PurseTarget), DeltaTime, SecretsPurseSpeed);
	if (FMath::Abs(ShownSilver - static_cast<float>(PurseTarget)) < 0.5f)
	{
		ShownSilver = static_cast<float>(PurseTarget);
	}
	PurseFlash = FMath::Max(0.f, PurseFlash - DeltaTime / SecretsPurseFlashSeconds);
	if (PurseText)
	{
		const int32 Shown = FMath::RoundToInt32(ShownSilver);
		if (Shown != Before || PurseFlash > 0.f)
		{
			PurseText->SetText(FText::FromString(LooterEconomy::Format(Shown)));
			const FLinearColor Flash = bPurseGain ? Color::Better() : Color::Worse();
			PurseText->SetColorAndOpacity(FSlateColor(FMath::Lerp(Color::Text(), Flash, PurseFlash)));
		}
	}
	if (PurseLine)
	{
		PurseLine->SetRenderScale(FVector2D(1.f + SecretsPursePop * PurseFlash * PurseFlash));
	}
}

// ---------------------------------------------------------------------------
// Keys and the mouse
// ---------------------------------------------------------------------------

FReply USecretsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		Close();
		return FReply::Handled();
	}
	if (Key == EKeys::W || Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up)
	{
		Select(Selected - 1);
		return FReply::Handled();
	}
	if (Key == EKeys::S || Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down)
	{
		Select(Selected + 1);
		return FReply::Handled();
	}
	// The Interact key that opened it may still be repeating: only a fresh press buys.
	if (!InKeyEvent.IsRepeat()
		&& (Key == EKeys::E || Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom))
	{
		BuySelected();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply USecretsWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// A click on no button ends here rather than in the game viewport, which would take the keyboard from the screen.
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

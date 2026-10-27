// UVendorWidget: opening and closing, the panel, choosing among the rows and the purse's motion. The rows are
// VendorWidgetRows.cpp's; what acting on one does is VendorWidgetTrade.cpp's; the keys and the mouse VendorWidgetKeys.cpp's.

#include "UI/Vendor/VendorWidget.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Economy/EconomyRules.h"
#include "Economy/Vendor.h"
#include "Economy/WalletSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Player/PlayerThrowComponent.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Vendor/SilverUI.h"
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
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "LooterVendor"

using namespace LooterUI;

namespace
{
	/** How fast the purse counts to the silver carried, and how long its flash lasts (s). */
	constexpr float PurseCountSpeed = 9.f;
	constexpr float PurseFlashSeconds = 0.6f;
	constexpr float PursePop = 0.12f;
}

// ---------------------------------------------------------------------------
// Opening and closing
// ---------------------------------------------------------------------------

void UVendorWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// The screen holds the keyboard (Escape closes it); its rows never take it.
	SetIsFocusable(true);
}

void UVendorWidget::Open(ALooterHUD* InHUD, AVendor* InVendor)
{
	OwningHUD = InHUD;
	Vendor = InVendor;
	Confirm = EConfirm::None;
	ConfirmIndex = INDEX_NONE;
	Selected = INDEX_NONE;
	LastBuy = INDEX_NONE;
	LastSell = INDEX_NONE;
	StatusWords = FText::GetEmpty();
	StatusColor = Color::TextDim();

	// The gun of the day for the player as they are now (a level gained or a day gone by brings a new one), and the purse
	// as it stands, with no flash.
	if (UWalletSubsystem* Wallet = GetWallet())
	{
		Wallet->RefreshOffer(GetPlayerLevel());
		PurseTarget = Wallet->GetSilver();
	}
	else
	{
		PurseTarget = 0;
	}
	ShownSilver = static_cast<float>(PurseTarget);
	PurseFlash = 0.f;

	// Opened before it was first shown, the panel is built as the widget is.
	if (WidgetTree && WidgetTree->RootWidget)
	{
		CloseConfirm();
		BuildPanel();
	}
}

void UVendorWidget::Close()
{
	if (ALooterHUD* HUD = OwningHUD.Get())
	{
		HUD->CloseVendor();
	}
	else
	{
		RemoveFromParent();
	}
}

UWeaponManagerComponent* UVendorWidget::GetManager() const
{
	const APawn* Pawn = GetOwningPlayerPawn();
	return Pawn ? Pawn->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
}

UPlayerThrowComponent* UVendorWidget::GetThrow() const
{
	return UPlayerThrowComponent::Find(GetOwningPlayerPawn());
}

UWalletSubsystem* UVendorWidget::GetWallet() const
{
	return UWalletSubsystem::Get(this);
}

int32 UVendorWidget::GetPlayerLevel() const
{
	return UWalletSubsystem::PlayerLevelOf(this);
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UVendorWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		// The whole screen answers the mouse, so a click beside the panel lands on it and the keyboard stays with it.
		Root->SetVisibility(ESlateVisibility::Visible);
		WidgetTree->RootWidget = Root;

		// The world stays in view behind the counter, dimmed.
		UImage* Backdrop = MakeImage(WidgetTree, RectBrush(Color::Backdrop()));
		MarkBackground(Backdrop);
		FillOverlaySlot(Root->AddChildToOverlay(Backdrop));

		PanelBox = MakeSized(WidgetTree, nullptr, VendorLayout::PanelWidth);
		UOverlaySlot* PanelSlot = Root->AddChildToOverlay(PanelBox);
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);

		// Over everything: the confirm, when one asks.
		PopupLayer = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		PopupLayer->SetVisibility(ESlateVisibility::Collapsed);
		FillOverlaySlot(Root->AddChildToOverlay(PopupLayer));

		BuildPanel();
	}
	return Super::RebuildWidget();
}

void UVendorWidget::Refresh()
{
	// The same row stays chosen: the same kind and ammo, or the same place in the backpack (after a sale, the gun that took
	// the sold one's place), else the side's last row.
	const bool bHad = Entries.IsValidIndex(Selected);
	const FEntry Was = bHad ? Entries[Selected] : FEntry();
	BuildPanel();
	if (!bHad)
	{
		return;
	}
	int32 Again = Entries.IndexOfByPredicate([&Was](const FEntry& Each)
	{
		return Each.Kind == Was.Kind && Each.Ammo == Was.Ammo && Each.BackpackIndex == Was.BackpackIndex;
	});
	if (Again == INDEX_NONE)
	{
		for (int32 Index = Entries.Num() - 1; Index >= 0 && Again == INDEX_NONE; --Index)
		{
			Again = Entries[Index].bSell == Was.bSell && Entries[Index].Kind == EEntry::Sell ? Index : INDEX_NONE;
		}
	}
	if (Again == INDEX_NONE)
	{
		Again = Entries.IndexOfByPredicate([&Was](const FEntry& Each) { return Each.bSell == Was.bSell; });
	}
	Select(Again == INDEX_NONE ? 0 : Again);
}

void UVendorWidget::BuildPanel()
{
	if (!PanelBox)
	{
		return;
	}
	Entries.Reset();
	const AVendor* From = Vendor.Get();
	const UWeaponManagerComponent* Manager = GetManager();
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	// The shop's greeting on the left; the purse on the right.
	{
		UHorizontalBox* Head = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UTextBlock* Greeting = MakeText(WidgetTree, From ? From->Greeting.ToString() : FString(), 13, Color::TextDim());
		Greeting->SetAutoWrapText(true);
		UHorizontalBoxSlot* GreetingSlot = Head->AddChildToHorizontalBox(Greeting);
		GreetingSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		GreetingSlot->SetVerticalAlignment(VAlign_Center);
		GreetingSlot->SetPadding(FMargin(0.f, 0.f, 24.f, 0.f));

		UVerticalBox* Purse = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Purse->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, LOCTEXT("YourSilver", "Your silver").ToString(), 9, Color::TextDim(), 250))
			->SetHorizontalAlignment(HAlign_Right);
		UTextBlock* Amount = nullptr;
		UWidget* Line = SilverUI::MakeAmount(WidgetTree, FMath::RoundToInt32(ShownSilver), 24, Color::Text(), false, &Amount);
		Line->SetRenderTransformPivot(FVector2D(1.f, 0.5f));
		Purse->AddChildToVerticalBox(Line)->SetHorizontalAlignment(HAlign_Right);
		PurseText = Amount;
		PurseLine = Line;
		Head->AddChildToHorizontalBox(Purse)->SetVerticalAlignment(VAlign_Center);
		Content->AddChildToVerticalBox(Head);
	}

	// The two sides: what's for sale, and what the shop buys.
	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	{
		UVerticalBox* Buy = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Buy->AddChildToVerticalBox(MakeSection(WidgetTree, LOCTEXT("Buy", "Buy").ToString()));
		Buy->AddChildToVerticalBox(MakeRefillAmmoRow())->SetPadding(FMargin(0.f, VendorLayout::RowGap, 0.f, 0.f));
		for (const EAmmoType Type : VendorTrade::StockedAmmo())
		{
			Buy->AddChildToVerticalBox(MakeAmmoRow(Type))->SetPadding(FMargin(0.f, VendorLayout::RowGap, 0.f, 0.f));
		}
		Buy->AddChildToVerticalBox(MakeGrenadeRow())->SetPadding(FMargin(0.f, VendorLayout::RowGap, 0.f, 0.f));
		Buy->AddChildToVerticalBox(MakeSection(WidgetTree, LOCTEXT("GunOfTheDay", "Gun of the day").ToString()))
			->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
		Buy->AddChildToVerticalBox(MakeOfferRow())->SetPadding(FMargin(0.f, VendorLayout::RowGap, 0.f, 0.f));
		Columns->AddChildToHorizontalBox(MakeSized(WidgetTree, Buy, VendorLayout::ColumnWidth))->SetVerticalAlignment(VAlign_Top);
	}
	{
		UVerticalBox* Sell = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Sell->AddChildToVerticalBox(MakeSection(WidgetTree, LOCTEXT("Sell", "Sell").ToString()));
		const int32 Carried = Manager ? Manager->GetBackpack().Num() : 0;
		const int32 Room = Manager ? Manager->BackpackCapacity : 0;
		const FText Note = FText::Format(LOCTEXT("BackpackNote", "Backpack {0} / {1} · guns in your hands aren't for sale"),
			FText::AsNumber(Carried), FText::AsNumber(Room));
		Sell->AddChildToVerticalBox(MakeText(WidgetTree, Note.ToString(), 11, Color::TextDim()))->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));

		SellList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		for (int32 Index = 0; Index < Carried; ++Index)
		{
			SellList->AddChildToVerticalBox(MakeSellRow(Index))->SetPadding(FMargin(0.f, Index > 0 ? VendorLayout::RowGap : 0.f, 0.f, 0.f));
		}
		if (Carried == 0)
		{
			UTextBlock* Nothing = MakeText(WidgetTree, LOCTEXT("NothingToSell", "Nothing in your backpack to sell.").ToString(), 13, Color::TextDim());
			SellList->AddChildToVerticalBox(Nothing)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
		}
		UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
		StyleScrollBox(Scroll);
		Scroll->AddChild(SellList);
		USizeBox* List = MakeSized(WidgetTree, Scroll, 0.f);
		List->SetMaxDesiredHeight(VendorLayout::SellListHeight);
		Sell->AddChildToVerticalBox(List)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
		Sell->AddChildToVerticalBox(MakeSellJunkRow())->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
		UHorizontalBoxSlot* SellSlot = Columns->AddChildToHorizontalBox(MakeSized(WidgetTree, Sell, VendorLayout::ColumnWidth));
		SellSlot->SetVerticalAlignment(VAlign_Top);
		SellSlot->SetPadding(FMargin(VendorLayout::ColumnGap, 0.f, 0.f, 0.f));
	}
	Content->AddChildToVerticalBox(Columns)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));

	// What the last trade did; it keeps its room when empty, so nothing under it jumps.
	StatusText = MakeText(WidgetTree, StatusWords.ToString(), 13, StatusColor);
	StatusText->SetAutoWrapText(true);
	USizeBox* StatusBox = MakeSized(WidgetTree, StatusText, 0.f);
	StatusBox->SetMinDesiredHeight(22.f);
	Content->AddChildToVerticalBox(StatusBox)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));

	// What the keys do, and a button to close it for the mouse.
	{
		UHorizontalBox* Bottom = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		struct FKeyHint
		{
			const TCHAR* KeyName;
			FText Does;
		};
		const FKeyHint Hints[] = {
			{ TEXT("E"), LOCTEXT("HintAct", "Buy / sell") },
			{ TEXT("W/S"), LOCTEXT("HintSelect", "Select") },
			{ TEXT("A/D"), LOCTEXT("HintSide", "Buy · Sell") },
			{ TEXT("J"), LOCTEXT("HintJunk", "Mark junk") },
			{ TEXT("R"), LOCTEXT("HintSellJunk", "Sell all junk") },
			{ TEXT("Esc"), LOCTEXT("HintClose", "Close") },
		};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Hints); ++Index)
		{
			UHorizontalBoxSlot* HintSlot = Bottom->AddChildToHorizontalBox(LoadoutParts::MakeKeyHint(WidgetTree, Hints[Index].KeyName,
				Hints[Index].Does.ToString(), Index == 0));
			HintSlot->SetVerticalAlignment(VAlign_Center);
			HintSlot->SetPadding(FMargin(Index > 0 ? 14.f : 0.f, 0.f, 0.f, 0.f));
		}
		Bottom->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, 0.f))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ULooterButton* CloseButton = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
		CloseButton->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), VendorActions::Close, 0, LOCTEXT("Close", "Close"), 12,
			EButtonKind::Normal);
		CloseButton->OnButtonClicked.BindUObject(this, &UVendorWidget::HandleButton);
		Bottom->AddChildToHorizontalBox(MakeSized(WidgetTree, CloseButton, 120.f, 34.f))->SetVerticalAlignment(VAlign_Center);
		Content->AddChildToVerticalBox(Bottom)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	}

	const FText Title = From && !From->Title.IsEmpty() ? From->Title : LOCTEXT("Title", "General Store");
	UWidget* Panel = MakePanel(WidgetTree, Title.ToString(), Content);
	Page = Panel;
	PanelBox->SetContent(Panel);

	// The first row that can be bought, else the first row.
	Selected = INDEX_NONE;
	for (int32 Index = 0; Index < Entries.Num() && Selected == INDEX_NONE; ++Index)
	{
		Selected = Entries[Index].bActive ? Index : INDEX_NONE;
	}
	Selected = Selected == INDEX_NONE && Entries.Num() > 0 ? 0 : Selected;
	Restyle();
}

// ---------------------------------------------------------------------------
// Choosing
// ---------------------------------------------------------------------------

void UVendorWidget::Select(int32 Index, bool bSound)
{
	if (!Entries.IsValidIndex(Index))
	{
		return;
	}
	if (Index != Selected && bSound)
	{
		PlayCue(LooterSoundCue::Hover);
	}
	Selected = Index;
	(Entries[Index].bSell ? LastSell : LastBuy) = Index;
	Restyle();
}

void UVendorWidget::Step(int32 Direction)
{
	if (!Entries.IsValidIndex(Selected))
	{
		Select(0, true);
		return;
	}
	// Round the ends of its own side.
	const bool bSell = Entries[Selected].bSell;
	TArray<int32> Side;
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (Entries[Index].bSell == bSell)
		{
			Side.Add(Index);
		}
	}
	const int32 At = Side.IndexOfByKey(Selected);
	if (Side.Num() > 1 && At != INDEX_NONE)
	{
		Select(Side[(At + Direction + Side.Num()) % Side.Num()], true);
	}
}

void UVendorWidget::SwitchSide(bool bToSell)
{
	const int32 Remembered = bToSell ? LastSell : LastBuy;
	if (Entries.IsValidIndex(Remembered) && Entries[Remembered].bSell == bToSell)
	{
		Select(Remembered, true);
		return;
	}
	const int32 First = Entries.IndexOfByPredicate([bToSell](const FEntry& Each) { return Each.bSell == bToSell; });
	if (First != INDEX_NONE)
	{
		Select(First, true);
	}
}

void UVendorWidget::Restyle()
{
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		const FEntry& Entry = Entries[Index];
		const bool bChosen = Index == Selected;
		if (Entry.Fill)
		{
			Entry.Fill->SetColorAndOpacity(bChosen ? LoadoutParts::Colors::PickedFill()
				: Entry.bActive ? Color::Row() : LoadoutParts::Colors::EmptyFill());
		}
		if (Entry.Line)
		{
			Entry.Line->SetColorAndOpacity(bChosen ? Color::Accent() : Color::RowLine());
		}
	}
}

void UVendorWidget::SetStatus(const FText& Message, const FLinearColor& TextColor)
{
	StatusWords = Message;
	StatusColor = TextColor;
	if (StatusText)
	{
		StatusText->SetText(Message);
		StatusText->SetColorAndOpacity(FSlateColor(TextColor));
	}
}

// ---------------------------------------------------------------------------
// The purse
// ---------------------------------------------------------------------------

void UVendorWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdatePurse(InDeltaTime);
}

void UVendorWidget::UpdatePurse(float DeltaTime)
{
	const int32 Now = UWalletSubsystem::SilverOf(this);
	if (Now != PurseTarget)
	{
		bPurseGain = Now > PurseTarget;
		PurseTarget = Now;
		PurseFlash = 1.f;
	}
	const int32 Before = FMath::RoundToInt32(ShownSilver);
	ShownSilver = FMath::FInterpTo(ShownSilver, static_cast<float>(PurseTarget), DeltaTime, PurseCountSpeed);
	if (FMath::Abs(ShownSilver - static_cast<float>(PurseTarget)) < 0.5f)
	{
		ShownSilver = static_cast<float>(PurseTarget);
	}
	PurseFlash = FMath::Max(0.f, PurseFlash - DeltaTime / PurseFlashSeconds);
	if (PurseText)
	{
		const int32 Shown = FMath::RoundToInt32(ShownSilver);
		if (Shown != Before || PurseFlash > 0.f)
		{
			PurseText->SetText(FText::FromString(LooterEconomy::Format(Shown)));
			const FLinearColor Flash = bPurseGain ? Color::CyanText() : Color::Worse();
			PurseText->SetColorAndOpacity(FSlateColor(FMath::Lerp(Color::Text(), Flash, PurseFlash)));
		}
	}
	if (PurseLine)
	{
		PurseLine->SetRenderScale(FVector2D(1.f + PursePop * PurseFlash * PurseFlash));
	}
}

// Buttons, keys and the mouse: VendorWidgetKeys.cpp.

#undef LOCTEXT_NAMESPACE

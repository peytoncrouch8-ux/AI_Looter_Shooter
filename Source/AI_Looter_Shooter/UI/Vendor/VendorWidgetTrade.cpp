// UVendorWidget: acting on a row (buying, selling, the junk mark, selling all junk), the confirm before a big trade, and
// what each trade says and sounds like.

#include "UI/Vendor/VendorWidget.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Economy/EconomyRules.h"
#include "Economy/VendorTrade.h"
#include "Economy/WalletSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Player/PlayerThrowComponent.h"
#include "Player/PlayerThrowRules.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "LooterVendor"

using namespace LooterUI;

namespace
{
	/** The page behind an open confirm, faded as a whole so its rows don't read through the confirm's glass (the bench's). */
	constexpr float PageOpacityBehindConfirm = 0.2f;
}

void UVendorWidget::PlayCue(const TCHAR* Cue, float Volume) const
{
	LooterSound::Play2D(this, Cue, Volume);
}

FText UVendorWidget::GunName(const FWeaponInstanceData& Gun)
{
	return FText::FromString(LooterWeaponText::Name(Gun));
}

// ---------------------------------------------------------------------------
// Acting on a row
// ---------------------------------------------------------------------------

void UVendorWidget::Act(int32 Index)
{
	if (!Entries.IsValidIndex(Index))
	{
		PlayCue(LooterSoundCue::Denied);
		return;
	}
	const FEntry Entry = Entries[Index];
	switch (Entry.Kind)
	{
	case EEntry::Ammo:
		BuyAmmo(Entry.Ammo);
		break;
	case EEntry::Grenade:
		BuyGrenade();
		break;
	case EEntry::Offer:
		BuyOffer();
		break;
	case EEntry::Sell:
		SellGun(Entry.BackpackIndex);
		break;
	case EEntry::SellJunk:
		SellAllJunk();
		break;
	}
}

void UVendorWidget::BuyAmmo(EAmmoType Type)
{
	UWalletSubsystem* Wallet = GetWallet();
	UWeaponManagerComponent* Manager = GetManager();
	if (!Wallet || !Manager)
	{
		PlayCue(LooterSoundCue::Denied);
		return;
	}
	const FText Item = FText::FromString(LooterAmmo::GetInfo(Type).Name);
	const FVendorTradeResult Result = VendorTrade::BuyAmmo(*Wallet, *Manager, Type, GetPlayerLevel());
	Report(Result, FText::Format(LOCTEXT("BoughtAmmo", "Bought {0} {1} for {2} silver."), FText::AsNumber(Result.Count), Item,
		FText::AsNumber(Result.Silver)), Item);
}

void UVendorWidget::BuyGrenade()
{
	UWalletSubsystem* Wallet = GetWallet();
	UPlayerThrowComponent* Throw = GetThrow();
	if (!Wallet || !Throw)
	{
		PlayCue(LooterSoundCue::Denied);
		return;
	}
	const FText Item = LOCTEXT("GraveSaltItem", "grave salt");
	const FVendorTradeResult Result = VendorTrade::BuyGrenade(*Wallet, *Throw, GetPlayerLevel());
	Report(Result, FText::Format(LOCTEXT("BoughtSalt", "Bought a jar of grave salt for {0} silver."), FText::AsNumber(Result.Silver)), Item);
}

void UVendorWidget::BuyOffer()
{
	UWalletSubsystem* Wallet = GetWallet();
	UWeaponManagerComponent* Manager = GetManager();
	if (!Wallet || !Manager)
	{
		PlayCue(LooterSoundCue::Denied);
		return;
	}
	const FVendorOffer& Offer = Wallet->GetOffer();
	if (!Offer.bRolled || !Offer.Gun.Definition || Offer.bSold)
	{
		Report(VendorTrade::BuyOffer(*Wallet, *Manager), FText::GetEmpty(), FText::GetEmpty());
		return;
	}
	const int32 Price = LooterEconomy::GunBuyPrice(Offer.Gun);
	// What can't happen is said at once; what can asks first, since it's a big spend.
	if (!Wallet->CanAfford(Price) || !VendorTrade::HasRoomForGun(*Manager))
	{
		Report(VendorTrade::BuyOffer(*Wallet, *Manager), FText::GetEmpty(), GunName(Offer.Gun));
		return;
	}
	const FString Body = FText::Format(LOCTEXT("ConfirmBuyBody", "Buy the {0} {1} (level {2}) for {3} silver? You'll have {4} left."),
		UEnum::GetDisplayValueAsText(Offer.Gun.Rarity), GunName(Offer.Gun), FText::AsNumber(Offer.Gun.Level), FText::AsNumber(Price),
		FText::AsNumber(Wallet->GetSilver() - Price)).ToString();
	OpenConfirm(EConfirm::BuyOffer, INDEX_NONE, LOCTEXT("ConfirmBuyTitle", "Buy the gun of the day").ToString(), Body, LOCTEXT("Buy", "Buy"));
}

void UVendorWidget::SellGun(int32 BackpackIndex)
{
	UWalletSubsystem* Wallet = GetWallet();
	UWeaponManagerComponent* Manager = GetManager();
	if (!Wallet || !Manager || !Manager->GetBackpack().IsValidIndex(BackpackIndex))
	{
		PlayCue(LooterSoundCue::Denied);
		return;
	}
	const FWeaponInstanceData& Gun = Manager->GetBackpack()[BackpackIndex];
	FText Why;
	if (!LooterEconomy::CanSell(Gun, &Why))
	{
		PlayCue(LooterSoundCue::Denied);
		SetStatus(Why, Color::TextDim());
		return;
	}
	if (LooterEconomy::SaleNeedsConfirm(Gun) && Confirm == EConfirm::None)
	{
		const FString Body = FText::Format(LOCTEXT("ConfirmSellBody", "Sell the {0} {1} (level {2}) for {3} silver? It's gone for good."),
			UEnum::GetDisplayValueAsText(Gun.Rarity), GunName(Gun), FText::AsNumber(Gun.Level), FText::AsNumber(LooterEconomy::GunSellPrice(Gun))).ToString();
		OpenConfirm(EConfirm::Sell, BackpackIndex, LOCTEXT("ConfirmSellTitle", "Sell this gun").ToString(), Body, LOCTEXT("Sell", "Sell"));
		return;
	}
	const FText Item = GunName(Gun);
	const FVendorTradeResult Result = VendorTrade::SellGun(*Wallet, *Manager, BackpackIndex);
	Report(Result, FText::Format(LOCTEXT("SoldGun", "Sold the {0} for {1} silver."), Item, FText::AsNumber(Result.Silver)), Item, /*bSale*/ true);
}

void UVendorWidget::SellAllJunk()
{
	UWalletSubsystem* Wallet = GetWallet();
	UWeaponManagerComponent* Manager = GetManager();
	if (!Wallet || !Manager)
	{
		PlayCue(LooterSoundCue::Denied);
		return;
	}
	const FVendorJunkTally Tally = VendorTrade::TallyJunk(*Manager);
	if (Tally.Count == 0)
	{
		PlayCue(LooterSoundCue::Denied);
		SetStatus(LOCTEXT("NoJunk", "Nothing's marked junk: J marks the backpack gun you've chosen."), Color::TextDim());
		return;
	}
	if (Tally.bAnyRare && Confirm == EConfirm::None)
	{
		const FString Body = FText::Format(LOCTEXT("ConfirmJunkBody", "Sell all {0} junk guns for {1} silver? One or more is Rare or better, and they're gone for good."),
			FText::AsNumber(Tally.Count), FText::AsNumber(Tally.Silver)).ToString();
		OpenConfirm(EConfirm::SellJunk, INDEX_NONE, LOCTEXT("ConfirmJunkTitle", "Sell all junk").ToString(), Body, LOCTEXT("SellAll", "Sell all"));
		return;
	}
	const FVendorTradeResult Result = VendorTrade::SellAllJunk(*Wallet, *Manager);
	Report(Result, FText::Format(LOCTEXT("SoldJunk", "Sold {0} junk guns for {1} silver."), FText::AsNumber(Result.Count),
		FText::AsNumber(Result.Silver)), LOCTEXT("JunkItem", "junk"), /*bSale*/ true);
}

void UVendorWidget::ToggleJunk()
{
	UWeaponManagerComponent* Manager = GetManager();
	if (!Manager || !Entries.IsValidIndex(Selected) || Entries[Selected].Kind != EEntry::Sell)
	{
		PlayCue(LooterSoundCue::Denied);
		SetStatus(LOCTEXT("JunkHow", "Choose a backpack gun to mark it junk."), Color::TextDim());
		return;
	}
	const int32 BackpackIndex = Entries[Selected].BackpackIndex;
	if (!VendorTrade::ToggleJunk(*Manager, BackpackIndex))
	{
		PlayCue(LooterSoundCue::Denied);
		return;
	}
	const FWeaponInstanceData& Gun = Manager->GetBackpack()[BackpackIndex];
	PlayCue(LooterSoundCue::Tab);
	SetStatus(FText::Format(Gun.bJunk ? LOCTEXT("MarkedJunk", "The {0} is marked junk.") : LOCTEXT("UnmarkedJunk", "The {0} isn't junk any more."),
		GunName(Gun)), Color::TextDim());
	Refresh();
}

void UVendorWidget::Report(const FVendorTradeResult& Result, const FText& DoneWords, const FText& Item, bool bSale)
{
	const UWalletSubsystem* Wallet = GetWallet();
	switch (Result.Result)
	{
	case EVendorTrade::Done:
		// Bought or sold: the till's sound, and the words in the colour the purse flashes (cyan for silver coming in).
		PlayCue(bSale ? LooterSoundCue::Economy::Sell : LooterSoundCue::Economy::Buy);
		SetStatus(DoneWords, bSale ? Color::CyanText() : Color::Better());
		break;
	case EVendorTrade::NotEnoughSilver:
		PlayCue(LooterSoundCue::Economy::NotEnough);
		SetStatus(FText::Format(LOCTEXT("NotEnough", "Not enough silver: {0} needed, you carry {1}."), FText::AsNumber(Result.Silver),
			FText::AsNumber(Wallet ? Wallet->GetSilver() : 0)), Color::Worse());
		break;
	case EVendorTrade::Full:
		PlayCue(LooterSoundCue::Denied);
		SetStatus(FText::Format(LOCTEXT("FullWords", "You carry all the {0} you can."), Item), Color::TextDim());
		break;
	case EVendorTrade::NoRoom:
		PlayCue(LooterSoundCue::Denied);
		SetStatus(LOCTEXT("NoRoom", "No room for it: sell or drop a gun first."), Color::Worse());
		break;
	case EVendorTrade::NotForSale:
		PlayCue(LooterSoundCue::Denied);
		SetStatus(LOCTEXT("NotForSaleWords", "That gun isn't for sale."), Color::TextDim());
		break;
	case EVendorTrade::SoldOut:
		PlayCue(LooterSoundCue::Denied);
		SetStatus(LOCTEXT("SoldOut", "Sold out: a new gun comes with your next level, or tomorrow's trade."), Color::TextDim());
		break;
	case EVendorTrade::Nothing:
		PlayCue(LooterSoundCue::Denied);
		break;
	}
	Refresh();
}

// ---------------------------------------------------------------------------
// The confirm
// ---------------------------------------------------------------------------

void UVendorWidget::OpenConfirm(EConfirm Kind, int32 BackpackIndex, const FString& Title, const FString& Body, const FText& AcceptLabel)
{
	if (!PopupLayer)
	{
		return;
	}
	Confirm = Kind;
	ConfirmIndex = BackpackIndex;

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UTextBlock* BodyText = MakeText(WidgetTree, Body, 14, Color::Text());
	BodyText->SetAutoWrapText(true);
	Content->AddChildToVerticalBox(BodyText);

	// Back on the left; the trade on the right, in the call to action's orange.
	auto MakeButton = [this](FName Action, const FText& Text, EButtonKind Kind)
	{
		ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
		Button->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), Action, 0, Text, 12, Kind);
		Button->OnButtonClicked.BindUObject(this, &UVendorWidget::HandleButton);
		return Button;
	};
	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* BackSlot = Buttons->AddChildToHorizontalBox(MakeButton(VendorActions::Back, LOCTEXT("Keep", "Not now"), EButtonKind::Normal));
	BackSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	BackSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	Buttons->AddChildToHorizontalBox(MakeButton(VendorActions::Accept, AcceptLabel, EButtonKind::Primary))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Content->AddChildToVerticalBox(MakeSized(WidgetTree, Buttons, 0.f, 44.f))->SetPadding(FMargin(0.f, 22.f, 0.f, 0.f));

	// The screen behind dims further and takes every click, so nothing under it can be pressed while it asks.
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Backdrop->SetBrush(RectBrush(Color::Backdrop()));
	Backdrop->SetVisibility(ESlateVisibility::Visible);
	Backdrop->SetHorizontalAlignment(HAlign_Center);
	Backdrop->SetVerticalAlignment(VAlign_Center);
	MarkBackground(Backdrop);
	Backdrop->SetContent(MakeSized(WidgetTree, MakePanel(WidgetTree, Title, Content), VendorLayout::ConfirmWidth));

	PopupLayer->ClearChildren();
	FillOverlaySlot(PopupLayer->AddChildToOverlay(Backdrop));
	PopupLayer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (Page)
	{
		Page->SetRenderOpacity(PageOpacityBehindConfirm);
	}
	PlayCue(LooterSoundCue::Open, 0.6f);
}

void UVendorWidget::CloseConfirm()
{
	Confirm = EConfirm::None;
	ConfirmIndex = INDEX_NONE;
	if (PopupLayer)
	{
		PopupLayer->ClearChildren();
		PopupLayer->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Page)
	{
		Page->SetRenderOpacity(1.f);
	}
}

void UVendorWidget::AcceptConfirm()
{
	const EConfirm Kind = Confirm;
	const int32 BackpackIndex = ConfirmIndex;
	UWalletSubsystem* Wallet = GetWallet();
	UWeaponManagerComponent* Manager = GetManager();
	CloseConfirm();
	if (!Wallet || !Manager)
	{
		PlayCue(LooterSoundCue::Denied);
		return;
	}
	switch (Kind)
	{
	case EConfirm::Sell:
	{
		if (!Manager->GetBackpack().IsValidIndex(BackpackIndex))
		{
			PlayCue(LooterSoundCue::Denied);
			return;
		}
		const FText Item = GunName(Manager->GetBackpack()[BackpackIndex]);
		const FVendorTradeResult Result = VendorTrade::SellGun(*Wallet, *Manager, BackpackIndex);
		Report(Result, FText::Format(LOCTEXT("SoldGun", "Sold the {0} for {1} silver."), Item, FText::AsNumber(Result.Silver)), Item, /*bSale*/ true);
		break;
	}
	case EConfirm::SellJunk:
	{
		const FVendorTradeResult Result = VendorTrade::SellAllJunk(*Wallet, *Manager);
		Report(Result, FText::Format(LOCTEXT("SoldJunk", "Sold {0} junk guns for {1} silver."), FText::AsNumber(Result.Count),
			FText::AsNumber(Result.Silver)), LOCTEXT("JunkItem", "junk"), /*bSale*/ true);
		break;
	}
	case EConfirm::BuyOffer:
	{
		const FWeaponInstanceData Gun = Wallet->GetOffer().Gun;
		const int32 BackpackBefore = Manager->GetBackpack().Num();
		const FVendorTradeResult Result = VendorTrade::BuyOffer(*Wallet, *Manager);
		const bool bInBackpack = Manager->GetBackpack().Num() > BackpackBefore;
		Report(Result, FText::Format(bInBackpack ? LOCTEXT("BoughtGunPack", "The {0} is yours: it's in your backpack.")
			: LOCTEXT("BoughtGunSlot", "The {0} is yours: it's in your slots."), GunName(Gun)), GunName(Gun));
		break;
	}
	case EConfirm::None:
		break;
	}
}

#undef LOCTEXT_NAMESPACE

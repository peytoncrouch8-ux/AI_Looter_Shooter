// UVendorWidget: the rows: refill all ammo, a box of each ammo, a jar of grave salt, the gun of the day's card, the backpack's guns with what
// they'd fetch, and selling all junk.

#include "UI/Vendor/VendorWidget.h"
#include "Economy/EconomyRules.h"
#include "Economy/WalletSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Player/PlayerThrowComponent.h"
#include "Player/PlayerThrowRules.h"
#include "UI/HUD/HudGrenadeWidget.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "UI/Vendor/SilverUI.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "LooterVendor"

using namespace LooterUI;

namespace
{
	/** An icon a row can't use now: greyed (grey dims an Inked icon). */
	const FLinearColor DimIcon(0.5f, 0.5f, 0.5f, 0.7f);

	/** The icon's box on a supplies row, and the guns' pictures on the sell rows and the gun of the day's card. */
	constexpr float SupplySize = 32.f;
	const FVector2D SupplyIcon(SupplySize, SupplySize);
	const FVector2D SellPicture(78.f, 32.f);
	const FVector2D OfferPicture(124.f, 52.f);

	FString RarityWord(EWeaponRarity Rarity)
	{
		return UEnum::GetDisplayValueAsText(Rarity).ToString();
	}
}

int32 UVendorWidget::AddEntry(EEntry Kind, bool bSell)
{
	FEntry Entry;
	Entry.Kind = Kind;
	Entry.bSell = bSell;
	return Entries.Add(Entry);
}

UWidget* UVendorWidget::MakeRow(int32 EntryIndex, UWidget* Icon, UWidget* Name, const FString& Detail, const FLinearColor& DetailColor,
	UWidget* Right, float MinHeight)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (Icon)
	{
		UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(Icon);
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	}
	UVerticalBox* Info = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (Name)
	{
		Info->AddChildToVerticalBox(Name);
	}
	if (!Detail.IsEmpty())
	{
		UTextBlock* DetailText = MakeText(WidgetTree, Detail, 10, DetailColor, true, 100);
		DetailText->SetAutoWrapText(true);
		Info->AddChildToVerticalBox(DetailText)->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
	}
	UHorizontalBoxSlot* InfoSlot = Row->AddChildToHorizontalBox(Info);
	InfoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	InfoSlot->SetVerticalAlignment(VAlign_Center);
	if (Right)
	{
		UHorizontalBoxSlot* RightSlot = Row->AddChildToHorizontalBox(Right);
		RightSlot->SetVerticalAlignment(VAlign_Center);
		RightSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
	}

	FEntry& Entry = Entries[EntryIndex];
	UOverlay* Card = LoadoutParts::MakeCard(WidgetTree, Row, FMargin(12.f, 8.f, 14.f, 8.f), 1.5f, Entry.Fill, Entry.Line);
	USizeBox* Sized = MakeSized(WidgetTree, Card, 0.f);
	Sized->SetMinDesiredHeight(MinHeight);
	ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	// The screen sounds its own trades and hovers.
	Button->bPlaysSounds = false;
	Button->SetupContent(Sized, VendorActions::Row, EntryIndex, EButtonKind::Bare);
	Button->OnButtonClicked.BindUObject(this, &UVendorWidget::HandleButton);
	Button->OnButtonHovered.BindUObject(this, &UVendorWidget::HandleHovered);
	Entry.Button = Button;
	return Button;
}

UWidget* UVendorWidget::MakePrice(int32 Silver, bool bAffordable) const
{
	return SilverUI::MakeAmount(WidgetTree, Silver, 15, bAffordable ? Color::Text() : Color::Worse(), false);
}

UWidget* UVendorWidget::MakeWord(const FString& Word, const FLinearColor& WordColor) const
{
	return LoadoutParts::Label(WidgetTree, Word, 11, WordColor, 150);
}

// ---------------------------------------------------------------------------
// Supplies
// ---------------------------------------------------------------------------

UWidget* UVendorWidget::MakeRefillAmmoRow()
{
	const int32 Index = AddEntry(EEntry::RefillAmmo, false);
	const UWeaponManagerComponent* Manager = GetManager();
	const UWalletSubsystem* Wallet = GetWallet();
	const FVendorRefillTally Tally = Manager ? VendorTrade::TallyRefill(*Manager, GetPlayerLevel()) : FVendorRefillTally();
	const bool bFull = Tally.IsFull();
	const bool bAffordable = Wallet && Wallet->CanAfford(Tally.Silver);
	// Greyed when there's nothing to buy; a price too high still answers a click, with the reason (as the other rows do).
	Entries[Index].bActive = !bFull;

	const bool bUsable = !bFull && bAffordable;
	UWidget* Name = LoadoutParts::Label(WidgetTree, LOCTEXT("RefillAll", "Refill all ammo").ToString(), 13, bUsable ? Color::Accent() : Color::TextDim(), 80);
	FString Detail;
	if (bFull)
	{
		Detail = LOCTEXT("RefillFull", "Every kind is full: you carry all you can").ToString();
	}
	else if (!bAffordable)
	{
		Detail = FText::Format(LOCTEXT("RefillTooDear", "Not enough silver: {0} needed, you carry {1}"), FText::AsNumber(Tally.Silver),
			FText::AsNumber(Wallet ? Wallet->GetSilver() : 0)).ToString();
	}
	else
	{
		Detail = FText::Format(LOCTEXT("RefillDetail", "+{0} rounds across {1} kinds, to the most you carry"), FText::AsNumber(Tally.Rounds),
			FText::AsNumber(Tally.Types)).ToString();
	}
	UWidget* Right = bFull ? MakeWord(LOCTEXT("Full", "Full").ToString(), Color::TextDim()) : MakePrice(Tally.Silver, bAffordable);
	return MakeRow(Index, nullptr, Name, Detail, bFull || bAffordable ? Color::TextDim() : Color::Worse(), Right, VendorLayout::RowHeight);
}

UWidget* UVendorWidget::MakeAmmoRow(EAmmoType Type)
{
	const int32 Index = AddEntry(EEntry::Ammo, false);
	Entries[Index].Ammo = Type;
	const UWeaponManagerComponent* Manager = GetManager();
	const UWalletSubsystem* Wallet = GetWallet();
	FVendorAmmoOffer Offer;
	if (Manager)
	{
		Offer = VendorTrade::AmmoOffer(*Manager, Type, GetPlayerLevel());
	}
	const bool bFull = !Manager || Offer.IsFull();
	Entries[Index].bActive = !bFull;

	const LooterAmmo::FInfo& Info = LooterAmmo::GetInfo(Type);
	UWidget* Icon = MakeSized(WidgetTree, MakeImage(WidgetTree, InkedIconBrush(LoadoutParts::AmmoIconName(Type), LoadoutParts::AmmoIcon(Type),
		SupplyIcon, bFull ? DimIcon : FLinearColor::White)), SupplySize, SupplySize);
	UWidget* Name = LoadoutParts::Label(WidgetTree, Info.Name, 14, bFull ? Color::TextDim() : Color::Text(), 60);
	const FString Detail = bFull
		? FText::Format(LOCTEXT("AmmoFull", "Carrying all you can · {0} / {1}"), FText::AsNumber(Offer.Carried), FText::AsNumber(Offer.Max)).ToString()
		: FText::Format(LOCTEXT("AmmoBox", "+{0} rounds · carrying {1} / {2}"), FText::AsNumber(Offer.Rounds), FText::AsNumber(Offer.Carried),
			FText::AsNumber(Offer.Max)).ToString();
	UWidget* Right = bFull ? MakeWord(LOCTEXT("Full", "Full").ToString(), Color::TextDim())
		: MakePrice(Offer.Price, Wallet && Wallet->CanAfford(Offer.Price));
	return MakeRow(Index, Icon, Name, Detail, Color::TextDim(), Right, VendorLayout::RowHeight);
}

UWidget* UVendorWidget::MakeGrenadeRow()
{
	const int32 Index = AddEntry(EEntry::Grenade, false);
	const UPlayerThrowComponent* Throw = GetThrow();
	const UWalletSubsystem* Wallet = GetWallet();
	const int32 Carried = Throw ? Throw->GetGrenades() : 0;
	const bool bFull = !Throw || Carried >= FThrowRules::MaxGrenades;
	Entries[Index].bActive = !bFull;
	const int32 Price = LooterEconomy::GrenadePrice(GetPlayerLevel());

	UWidget* Icon = MakeSized(WidgetTree, MakeImage(WidgetTree, InkedIconBrush(TEXT("GraveSaltGrenade"), UHudGrenadeWidget::Icon(), SupplyIcon,
		bFull ? DimIcon : FLinearColor::White)), SupplySize, SupplySize);
	UWidget* Name = LoadoutParts::Label(WidgetTree, LOCTEXT("GraveSalt", "Grave salt").ToString(), 14, bFull ? Color::TextDim() : Color::Text(), 60);
	const FString Detail = FText::Format(bFull ? LOCTEXT("SaltFull", "Carrying all you can · {0} / {1}") : LOCTEXT("SaltJar", "+1 jar · carrying {0} / {1}"),
		FText::AsNumber(Carried), FText::AsNumber(FThrowRules::MaxGrenades)).ToString();
	UWidget* Right = bFull ? MakeWord(LOCTEXT("Full", "Full").ToString(), Color::TextDim()) : MakePrice(Price, Wallet && Wallet->CanAfford(Price));
	return MakeRow(Index, Icon, Name, Detail, Color::TextDim(), Right, VendorLayout::RowHeight);
}

// ---------------------------------------------------------------------------
// The gun of the day
// ---------------------------------------------------------------------------

UWidget* UVendorWidget::MakeOfferRow()
{
	const int32 Index = AddEntry(EEntry::Offer, false);
	const UWalletSubsystem* Wallet = GetWallet();
	const FVendorOffer* Offer = Wallet ? &Wallet->GetOffer() : nullptr;
	if (!Offer || !Offer->bRolled || !Offer->Gun.Definition)
	{
		Entries[Index].bActive = false;
		UWidget* Name = LoadoutParts::Label(WidgetTree, LOCTEXT("NoOffer", "Nothing in today").ToString(), 14, Color::TextDim(), 60);
		return MakeRow(Index, nullptr, Name, FString(), Color::TextDim(), nullptr, VendorLayout::OfferHeight);
	}
	const FWeaponInstanceData& Gun = Offer->Gun;
	const bool bSold = Offer->bSold;
	Entries[Index].bActive = !bSold;
	const int32 Price = LooterEconomy::GunBuyPrice(Gun);

	UWidget* Picture = LoadoutParts::MakeGunPicture(WidgetTree, Gun, OfferPicture, bSold ? DimIcon : FLinearColor::White);
	UWidget* Name = LoadoutParts::MakeGunNameLine(WidgetTree, Gun, 15, bSold ? Color::TextDim() : LooterWeaponText::Color(Gun), 40, 250.f, 11);
	FString Detail;
	if (bSold)
	{
		const int32 Minutes = FMath::Max(1, FMath::CeilToInt32(static_cast<float>(Offer->SecondsToNext(Wallet->GetClock()) / 60.0)));
		Detail = FText::Format(LOCTEXT("OfferSold", "Sold · a new one at level {0}, or in {1} min"), FText::AsNumber(Offer->RolledAtLevel + 1),
			FText::AsNumber(Minutes)).ToString();
	}
	else
	{
		const FWeaponStats& Stats = Gun.Stats;
		Detail = FText::Format(LOCTEXT("OfferDetail", "Level {0} · {1}\n{2} damage · {3} rpm · {4} rounds"), FText::AsNumber(Gun.Level),
			FText::FromString(RarityWord(Gun.Rarity)), FText::FromString(LooterWeaponText::DamageString(Stats)),
			FText::AsNumber(FMath::RoundToInt32(Stats.FireRate)), FText::AsNumber(Stats.MagazineSize)).ToString();
	}
	UWidget* Right = bSold ? MakeWord(LOCTEXT("Sold", "Sold").ToString(), Color::TextDim()) : MakePrice(Price, Wallet->CanAfford(Price));
	return MakeRow(Index, Picture, Name, Detail, Color::TextDim(), Right, VendorLayout::OfferHeight);
}

// ---------------------------------------------------------------------------
// Selling
// ---------------------------------------------------------------------------

UWidget* UVendorWidget::MakeSellRow(int32 BackpackIndex)
{
	const int32 Index = AddEntry(EEntry::Sell, true);
	Entries[Index].BackpackIndex = BackpackIndex;
	const UWeaponManagerComponent* Manager = GetManager();
	if (!Manager || !Manager->GetBackpack().IsValidIndex(BackpackIndex))
	{
		Entries[Index].bActive = false;
		return MakeRow(Index, nullptr, nullptr, FString(), Color::TextDim(), nullptr, VendorLayout::RowHeight);
	}
	const FWeaponInstanceData& Gun = Manager->GetBackpack()[BackpackIndex];
	const bool bSellable = LooterEconomy::CanSell(Gun);
	Entries[Index].bActive = bSellable;

	UWidget* Picture = LoadoutParts::MakeGunPicture(WidgetTree, Gun, SellPicture, bSellable ? FLinearColor::White : DimIcon);
	UWidget* Name = LoadoutParts::MakeGunNameLine(WidgetTree, Gun, 13, LooterWeaponText::Color(Gun), 40, 230.f, 10);
	const FString Detail = FText::Format(LOCTEXT("SellDetail", "Level {0} · {1}"), FText::AsNumber(Gun.Level),
		FText::FromString(RarityWord(Gun.Rarity))).ToString();

	// On the right: JUNK when it's marked, then what it fetches (or why it doesn't sell).
	UHorizontalBox* Right = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (Gun.bJunk)
	{
		UHorizontalBoxSlot* JunkSlot = Right->AddChildToHorizontalBox(MakeWord(LOCTEXT("Junk", "Junk").ToString(), Color::Worse()));
		JunkSlot->SetVerticalAlignment(VAlign_Center);
		JunkSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	}
	UWidget* Price = bSellable ? MakePrice(LooterEconomy::GunSellPrice(Gun), true)
		: MakeWord(LOCTEXT("NotForSale", "Not for sale").ToString(), Color::TextDim());
	Right->AddChildToHorizontalBox(Price)->SetVerticalAlignment(VAlign_Center);
	return MakeRow(Index, Picture, Name, Detail, Color::TextDim(), Right, VendorLayout::RowHeight);
}

UWidget* UVendorWidget::MakeSellJunkRow()
{
	const int32 Index = AddEntry(EEntry::SellJunk, true);
	const UWeaponManagerComponent* Manager = GetManager();
	const FVendorJunkTally Tally = Manager ? VendorTrade::TallyJunk(*Manager) : FVendorJunkTally();
	Entries[Index].bActive = Tally.Count > 0;
	const FString Title = FText::Format(LOCTEXT("SellAllJunk", "Sell all junk ({0})"), FText::AsNumber(Tally.Count)).ToString();
	UWidget* Name = LoadoutParts::Label(WidgetTree, Title, 13, Tally.Count > 0 ? Color::Accent() : Color::TextDim(), 80);
	const FString Detail = Tally.Count > 0
		? LOCTEXT("SellJunkDetail", "Every backpack gun marked junk, at once").ToString()
		: LOCTEXT("SellJunkHow", "Mark guns junk with J (or a right-click), here or in the inventory").ToString();
	UWidget* Right = Tally.Count > 0 ? MakePrice(Tally.Silver, true) : nullptr;
	return MakeRow(Index, nullptr, Name, Detail, Color::TextDim(), Right, VendorLayout::RowHeight);
}

#undef LOCTEXT_NAMESPACE

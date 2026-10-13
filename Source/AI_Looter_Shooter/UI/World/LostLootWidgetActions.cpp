// ULostLootWidget: claiming the chosen gun (CLAIMED stamped over its card), buying shelving (asking first), the confirm, and
// what each says and sounds like.

#include "UI/World/LostLootWidget.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Economy/WalletSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/LostLootRules.h"
#include "Loot/LostLootSubsystem.h"
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

#define LOCTEXT_NAMESPACE "LooterLostLoot"

using namespace LooterUI;

namespace
{
	/** The page behind an open confirm, faded as a whole so its tickets don't read through the confirm's glass (the vendor's). */
	constexpr float PageOpacityBehindConfirm = 0.2f;
}

void ULostLootWidget::PlayCue(const TCHAR* Cue, float Volume) const
{
	LooterSound::Play2D(this, Cue, Volume);
}

// ---------------------------------------------------------------------------
// Claiming
// ---------------------------------------------------------------------------

bool ULostLootWidget::Claim()
{
	if (bStamping || bConfirmOpen)
	{
		return false;
	}
	ULostLootSubsystem* LostLoot = GetLostLoot();
	UWeaponManagerComponent* Weapons = GetManager();
	const FLostLootEntry* Chosen = GetChosenEntry();
	if (!LostLoot || !Weapons || !Chosen)
	{
		PlayCue(LooterSoundCue::Denied);
		SetStatus(LOCTEXT("NothingChosen", "Choose a ticket to claim."), Color::TextDim());
		return false;
	}
	const FLostLootEntry Ticket = *Chosen;
	FLostLootStore& Store = LostLoot->GetMutableStore();
	const LostLootRules::ETake Result = LostLootRules::Take(Store, Store.FindTicket(Ticket.Ticket), *Weapons);
	const FText Name = FText::FromString(LooterWeaponText::Name(Ticket.Weapon));
	switch (Result)
	{
	case LostLootRules::ETake::InBackpack:
	case LostLootRules::ETake::InSlot:
		break;
	case LostLootRules::ETake::NoRoom:
		PlayCue(LooterSoundCue::LostLoot::NoRoom);
		SetStatus(LOCTEXT("NoRoom", "No room for it: your backpack and slots are full. Sell, drop or scrap a gun, then come back."), Color::Worse());
		return false;
	case LostLootRules::ETake::Nothing:
		PlayCue(LooterSoundCue::Denied);
		return false;
	}

	// The player's again: CLAIMED slams down on its card, and once it lifts the list is read again without it.
	Stamped = Ticket;
	bStamping = true;
	StampAge = 0.f;
	PlayCue(LooterSoundCue::LostLoot::Claim);
	SetStatus(FText::Format(Result == LostLootRules::ETake::InBackpack ? LOCTEXT("ClaimedPack", "Claimed: the {0} is in your backpack.")
		: LOCTEXT("ClaimedSlot", "Claimed: the {0} is in your slots."), Name), Color::Better());
	LostLoot->NotifyChanged();
	ApplyStamp();
	return true;
}

// ---------------------------------------------------------------------------
// More shelving
// ---------------------------------------------------------------------------

bool ULostLootWidget::BuyUpgrade(bool bConfirmed)
{
	if (bStamping)
	{
		return false;
	}
	ULostLootSubsystem* LostLoot = GetLostLoot();
	UWalletSubsystem* Wallet = GetWallet();
	if (!LostLoot || !Wallet)
	{
		PlayCue(LooterSoundCue::Denied);
		return false;
	}
	FLostLootStore& Store = LostLoot->GetMutableStore();
	if (Store.Upgrades >= LostLootRules::MaxUpgrades)
	{
		PlayCue(LooterSoundCue::Denied);
		SetStatus(FText::Format(LOCTEXT("AtMostStatus", "The shelves are at their most: room for {0} guns."), FText::AsNumber(Store.GetCapacity())),
			Color::TextDim());
		return false;
	}
	const int32 Level = GetPlayerLevel();
	const int32 Price = LostLootRules::UpgradePrice(Store.Upgrades, Level);
	const FText Name = LostLootRules::UpgradeName(Store.Upgrades);
	// What can't happen is said at once; what can asks first, since it's a big spend.
	if (!Wallet->CanAfford(Price))
	{
		PlayCue(LooterSoundCue::Economy::NotEnough);
		SetStatus(FText::Format(LOCTEXT("NotEnough", "Not enough silver for more shelving: {0} needed, you carry {1}."), FText::AsNumber(Price),
			FText::AsNumber(Wallet->GetSilver())), Color::Worse());
		return false;
	}
	if (!bConfirmed)
	{
		const FString Body = FText::Format(LOCTEXT("ConfirmBody", "{0}: room for {1} guns instead of {2}, for {3} silver? You'll have {4} left."),
			Name, FText::AsNumber(LostLootRules::Capacity(Store.Upgrades + 1)), FText::AsNumber(Store.GetCapacity()), FText::AsNumber(Price),
			FText::AsNumber(Wallet->GetSilver() - Price)).ToString();
		OpenConfirm(LOCTEXT("ConfirmTitle", "More shelving").ToString(), Body, LOCTEXT("Buy", "Buy"));
		return false;
	}
	const LostLootRules::FUpgradeResult Result = LostLootRules::BuyUpgrade(*Wallet, Store, Level);
	if (Result.Result != LostLootRules::EUpgrade::Done)
	{
		PlayCue(Result.Result == LostLootRules::EUpgrade::NotEnoughSilver ? LooterSoundCue::Economy::NotEnough : LooterSoundCue::Denied);
		return false;
	}
	// The till, then the hammer: the purse counts down by itself as the silver goes.
	PlayCue(LooterSoundCue::LostLoot::Upgrade);
	SetStatus(FText::Format(LOCTEXT("Bought", "{0}: the shelves hold {1} guns now."), Name, FText::AsNumber(Result.Capacity)), Color::Better());
	LostLoot->NotifyChanged();
	return true;
}

// ---------------------------------------------------------------------------
// The confirm
// ---------------------------------------------------------------------------

void ULostLootWidget::OpenConfirm(const FString& Title, const FString& Body, const FText& AcceptLabel)
{
	if (!PopupLayer)
	{
		return;
	}
	bConfirmOpen = true;
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UTextBlock* BodyText = MakeText(WidgetTree, Body, 14, Color::Text());
	BodyText->SetAutoWrapText(true);
	Content->AddChildToVerticalBox(BodyText);

	// Not now on the left; the purchase on the right, in the call to action's orange.
	auto MakeButton = [this](FName Action, const FText& Text, EButtonKind Kind)
	{
		ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
		Button->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), Action, 0, Text, 12, Kind);
		Button->OnButtonClicked.BindUObject(this, &ULostLootWidget::HandleButton);
		return Button;
	};
	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* BackSlot = Buttons->AddChildToHorizontalBox(MakeButton(LostLootActions::Back, LOCTEXT("NotNow", "Not now"), EButtonKind::Normal));
	BackSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	BackSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	Buttons->AddChildToHorizontalBox(MakeButton(LostLootActions::Accept, AcceptLabel, EButtonKind::Primary))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Content->AddChildToVerticalBox(MakeSized(WidgetTree, Buttons, 0.f, 44.f))->SetPadding(FMargin(0.f, 22.f, 0.f, 0.f));

	// The screen behind dims further and takes every click, so nothing under it can be pressed while it asks.
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Backdrop->SetBrush(RectBrush(Color::Backdrop()));
	Backdrop->SetVisibility(ESlateVisibility::Visible);
	Backdrop->SetHorizontalAlignment(HAlign_Center);
	Backdrop->SetVerticalAlignment(VAlign_Center);
	MarkBackground(Backdrop);
	Backdrop->SetContent(MakeSized(WidgetTree, MakePanel(WidgetTree, Title, Content), LostLootLayout::ConfirmWidth));

	PopupLayer->ClearChildren();
	FillOverlaySlot(PopupLayer->AddChildToOverlay(Backdrop));
	PopupLayer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (Page)
	{
		Page->SetRenderOpacity(PageOpacityBehindConfirm);
	}
	PlayCue(LooterSoundCue::Open, 0.6f);
	RefreshPrompts();
}

void ULostLootWidget::CloseConfirm()
{
	const bool bWasOpen = bConfirmOpen;
	bConfirmOpen = false;
	if (PopupLayer)
	{
		PopupLayer->ClearChildren();
		PopupLayer->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Page)
	{
		Page->SetRenderOpacity(1.f);
	}
	if (bWasOpen)
	{
		RefreshPrompts();
	}
	// Something came in while it asked: the list is read again now.
	if (bRefreshWanted && !bStamping)
	{
		Refresh();
	}
}

void ULostLootWidget::AcceptConfirm()
{
	CloseConfirm();
	BuyUpgrade(/*bConfirmed*/ true);
}

#undef LOCTEXT_NAMESPACE

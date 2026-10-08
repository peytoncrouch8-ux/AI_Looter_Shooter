// UBenchWidget: the confirm before anything is lost for good (scrapping a gun, throwing a part out), and doing it.

#include "UI/Bench/BenchWidget.h"
#include "Audio/LooterSound.h"
#include "Inventory/WeaponManagerComponent.h"
#include "UI/Bench/BenchRules.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Templates/UnrealTemplate.h"

#define LOCTEXT_NAMESPACE "LooterBench"

using namespace LooterUI;

namespace
{
	/**
	 * The page behind an open confirm, faded as a whole: its rows would otherwise read through the confirm's glass (the
	 * dimming backdrop is a background, which the UI transparency setting thins).
	 */
	constexpr float PageOpacityBehindConfirm = 0.2f;
}

ULooterButton* UBenchWidget::MakeButton(FName Action, const FText& Text, EButtonKind Kind)
{
	ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Button->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), Action, 0, Text, 12, Kind);
	Button->OnButtonClicked.BindUObject(this, &UBenchWidget::HandleButton);
	return Button;
}

void UBenchWidget::OpenConfirm(EConfirm Kind, const FString& Title, const FString& Body, const FText& AcceptLabel)
{
	if (!PopupLayer)
	{
		return;
	}
	Confirm = Kind;

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UTextBlock* BodyText = MakeText(WidgetTree, Body, 14, Color::Text());
	BodyText->SetAutoWrapText(true);
	Content->AddChildToVerticalBox(BodyText);

	// Back on the left; the loss, in the danger color, on the right.
	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* BackSlot = Buttons->AddChildToHorizontalBox(MakeButton(BenchActions::Back, LOCTEXT("Back", "Back"), EButtonKind::Normal));
	BackSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	BackSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	Buttons->AddChildToHorizontalBox(MakeButton(BenchActions::Accept, AcceptLabel, EButtonKind::Danger))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Content->AddChildToVerticalBox(MakeSized(WidgetTree, Buttons, 0.f, 44.f))->SetPadding(FMargin(0.f, 22.f, 0.f, 0.f));

	// The screen behind dims further and takes every click, so nothing under it can be pressed while it asks.
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Backdrop->SetBrush(RectBrush(Color::Backdrop()));
	Backdrop->SetVisibility(ESlateVisibility::Visible);
	Backdrop->SetHorizontalAlignment(HAlign_Center);
	Backdrop->SetVerticalAlignment(VAlign_Center);
	MarkBackground(Backdrop);
	Backdrop->SetContent(MakeSized(WidgetTree, MakePanel(WidgetTree, Title, Content), BenchLayout::ConfirmWidth));

	PopupLayer->ClearChildren();
	FillOverlaySlot(PopupLayer->AddChildToOverlay(Backdrop));
	PopupLayer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (Page)
	{
		Page->SetRenderOpacity(PageOpacityBehindConfirm);
	}
	RefreshPrompts();
}

void UBenchWidget::CloseConfirm()
{
	Confirm = EConfirm::None;
	if (PopupLayer)
	{
		PopupLayer->ClearChildren();
		PopupLayer->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Page)
	{
		Page->SetRenderOpacity(1.f);
	}
	RefreshPrompts();
}

void UBenchWidget::AcceptConfirm()
{
	switch (Confirm)
	{
	case EConfirm::Scrap:
		ScrapChosen();
		break;
	case EConfirm::Discard:
		DiscardChosen();
		break;
	case EConfirm::None:
		break;
	}
}

void UBenchWidget::ScrapChosen()
{
	const int32 Keep = ConfirmSlot;
	CloseConfirm();
	UWeaponManagerComponent* Inventory = Manager.Get();
	const FWeaponInstanceData* Item = ChosenItem();
	if (!Inventory || !Item)
	{
		return;
	}
	// Said after it's gone, so named now.
	const FText ScrappedName = FText::FromString(LooterWeaponText::Name(*Item));
	const FText KeptName = FText::FromString(BenchRules::SlotPartName(*Item, Keep));
	const FCarriedGun Scrapped = ChosenRef();
	bool bScrapped = false;
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		bScrapped = Inventory->ScrapGun(Scrapped, SlotNameAt(Keep));
	}
	bScrapping = false;
	if (bScrapped)
	{
		PlayCue(LooterSoundCue::BenchScrap);
		SetStatus(FText::Format(LOCTEXT("Scrapped", "Scrapped the {0} · the {1} is in the box"), ScrappedName, KeptName), Color::Better());
		// On to the gun that took its place in the list (the next one, or the last).
		Column = EColumn::Guns;
		CursorIndex = ChosenGun;
	}
	else
	{
		FText Why;
		Inventory->CanScrap(Scrapped, &Why);
		Deny(Why.IsEmpty() ? LOCTEXT("CantScrap", "That gun can't be scrapped now") : Why);
	}
	Refresh();
	RefreshStage(bScrapped);
}

void UBenchWidget::DiscardChosen()
{
	const int32 BoxIndex = ConfirmBoxIndex;
	CloseConfirm();
	UWeaponManagerComponent* Inventory = Manager.Get();
	if (!Inventory || !Inventory->GetPartsBox().IsValidIndex(BoxIndex))
	{
		return;
	}
	const FText ThrownName = FText::FromString(BenchRules::PartName(Inventory->GetPartsBox()[BoxIndex]));
	bool bThrownOut = false;
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		bThrownOut = Inventory->DiscardPart(BoxIndex);
	}
	if (bThrownOut)
	{
		// The scrap sound, quieter: a part tossed on the pile rather than a gun broken down.
		PlayCue(LooterSoundCue::BenchScrap, 0.7f);
		SetStatus(FText::Format(LOCTEXT("ThrownOut", "Threw out the {0}"), ThrownName), Color::TextDim());
	}
	Refresh();
}

#undef LOCTEXT_NAMESPACE

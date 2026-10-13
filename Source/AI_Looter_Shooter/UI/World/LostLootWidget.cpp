// ULostLootWidget: the panel, choosing among the tickets, the purse and the motion. Opening and closing are
// LostLootWidgetOpen.cpp's; the header and the tickets LostLootWidgetRows.cpp's; the card and the stamp
// LostLootWidgetCard.cpp's; claiming and buying LostLootWidgetActions.cpp's; the keys, the stick and the mouse
// LostLootWidgetInput.cpp's.

#include "UI/World/LostLootWidget.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Economy/EconomyRules.h"
#include "Economy/WalletSubsystem.h"
#include "Loot/LostLootRules.h"
#include "Loot/LostLootSubsystem.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Inventory/LoadoutParts.h"
#include "World/LostPropertyWindow.h"
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

#define LOCTEXT_NAMESPACE "LooterLostLoot"

using namespace LooterUI;

namespace
{
	/** How fast the purse counts to the silver carried, and how long its flash lasts (s); the vendor's. */
	constexpr float PurseCountSpeed = 9.f;
	constexpr float PurseFlashSeconds = 0.6f;
	constexpr float PursePop = 0.12f;
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

TSharedRef<SWidget> ULostLootWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		// The whole screen answers the mouse, so a click beside the panel lands here and the keyboard stays with it.
		Root->SetVisibility(ESlateVisibility::Visible);
		WidgetTree->RootWidget = Root;

		// The world stays in view behind the counter, dimmed.
		UImage* Backdrop = MakeImage(WidgetTree, RectBrush(Color::Backdrop()));
		MarkBackground(Backdrop);
		FillOverlaySlot(Root->AddChildToOverlay(Backdrop));

		PanelBox = MakeSized(WidgetTree, nullptr, LostLootLayout::PanelWidth);
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

void ULostLootWidget::BuildPanel()
{
	if (!PanelBox)
	{
		return;
	}
	Rows.Reset();
	Selected = INDEX_NONE;
	const ULostLootSubsystem* LostLoot = GetLostLoot();
	const FLostLootStore Empty;
	const FLostLootStore& Store = LostLoot ? LostLoot->GetStore() : Empty;
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Content->AddChildToVerticalBox(MakeHeader());

	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	{
		// The tickets, newest first: what was just lost is what the player came for.
		UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Left->AddChildToVerticalBox(MakeSection(WidgetTree, LOCTEXT("Tickets", "Claim tickets").ToString()));
		UVerticalBox* List = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		const int32 NextToGo = Store.FindNextToGo();
		for (int32 Index = Store.Entries.Num() - 1; Index >= 0; --Index)
		{
			const int32 RowIndex = Rows.Num();
			List->AddChildToVerticalBox(MakeTicketRow(RowIndex, Store.Entries[Index], Index == NextToGo, Store.IsFull()))
				->SetPadding(FMargin(0.f, RowIndex > 0 ? LostLootLayout::RowGap : 0.f, 0.f, 0.f));
		}
		if (Store.Entries.IsEmpty())
		{
			UTextBlock* Nothing = MakeText(WidgetTree, LOCTEXT("NothingHere", "Nothing on the shelves. Guns left lying for five minutes, "
				"or left on the ground when you leave an area or call it a day, are brought here.").ToString(), 13, Color::TextDim());
			Nothing->SetAutoWrapText(true);
			List->AddChildToVerticalBox(Nothing)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
		}
		ListScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
		StyleScrollBox(ListScroll);
		ListScroll->AddChild(List);
		USizeBox* ListBox = MakeSized(WidgetTree, ListScroll, 0.f);
		ListBox->SetMinDesiredHeight(LostLootLayout::ListHeight);
		ListBox->SetMaxDesiredHeight(LostLootLayout::ListHeight);
		Left->AddChildToVerticalBox(ListBox)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
		Left->AddChildToVerticalBox(MakeUpgradeRow())->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
		Columns->AddChildToHorizontalBox(MakeSized(WidgetTree, Left, LostLootLayout::ListWidth))->SetVerticalAlignment(VAlign_Top);
	}
	{
		// The chosen gun's card, CLAIMED stamped over it as it's claimed, and the claim under it.
		UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Right->AddChildToVerticalBox(MakeSection(WidgetTree, LOCTEXT("OnTheCounter", "On the counter").ToString()));
		UOverlay* CardArea = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		CardBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		CardScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
		StyleScrollBox(CardScroll);
		CardScroll->AddChild(CardBox);
		UImage* CardFill = nullptr;
		UImage* CardLine = nullptr;
		UOverlay* Card = LoadoutParts::MakeCard(WidgetTree, CardScroll, FMargin(16.f, 12.f, 10.f, 12.f), 2.f, CardFill, CardLine);
		if (CardFill && CardLine)
		{
			CardFill->SetColorAndOpacity(LoadoutParts::Colors::CardFill());
			CardLine->SetColorAndOpacity(LoadoutParts::Colors::CardLine());
		}
		FillOverlaySlot(CardArea->AddChildToOverlay(Card));
		StampWidget = MakeStamp();
		UOverlaySlot* StampSlot = CardArea->AddChildToOverlay(StampWidget);
		StampSlot->SetHorizontalAlignment(HAlign_Center);
		StampSlot->SetVerticalAlignment(VAlign_Center);
		USizeBox* CardSized = MakeSized(WidgetTree, CardArea, 0.f, LostLootLayout::CardHeight);
		Right->AddChildToVerticalBox(CardSized)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
		ClaimHolder = MakeSized(WidgetTree, nullptr, 0.f);
		Right->AddChildToVerticalBox(ClaimHolder)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
		UHorizontalBoxSlot* RightSlot = Columns->AddChildToHorizontalBox(MakeSized(WidgetTree, Right, LostLootLayout::CardWidth));
		RightSlot->SetVerticalAlignment(VAlign_Top);
		RightSlot->SetPadding(FMargin(LostLootLayout::ColumnGap, 0.f, 0.f, 0.f));
	}
	Content->AddChildToVerticalBox(Columns)->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));

	// What the last claim or purchase did; it keeps its room when empty, so nothing under it jumps.
	StatusText = MakeText(WidgetTree, StatusWords.ToString(), 13, StatusColor);
	StatusText->SetAutoWrapText(true);
	USizeBox* StatusBox = MakeSized(WidgetTree, StatusText, 0.f);
	StatusBox->SetMinDesiredHeight(22.f);
	Content->AddChildToVerticalBox(StatusBox)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));

	// The keys for the device in use, and a button to close it for the mouse.
	{
		UHorizontalBox* Bottom = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		PromptBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Bottom->AddChildToHorizontalBox(PromptBar)->SetVerticalAlignment(VAlign_Center);
		Bottom->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, 0.f))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ULooterButton* CloseButton = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
		CloseButton->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), LostLootActions::Close, 0, LOCTEXT("Close", "Close"), 12,
			EButtonKind::Normal);
		CloseButton->OnButtonClicked.BindUObject(this, &ULostLootWidget::HandleButton);
		Bottom->AddChildToHorizontalBox(MakeSized(WidgetTree, CloseButton, 120.f, 34.f))->SetVerticalAlignment(VAlign_Center);
		Content->AddChildToVerticalBox(Bottom)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	}

	const ALostPropertyWindow* At = Window.Get();
	const FText Title = At && !At->Title.IsEmpty() ? At->Title : LOCTEXT("Title", "Lost Property");
	UWidget* Panel = MakePanel(WidgetTree, Title.ToString(), Content);
	Page = Panel;
	PanelBox->SetContent(Panel);
	RefreshPrompts();
	// The newest ticket first; its card up.
	if (Rows.Num() > 0)
	{
		Select(0, false, false);
	}
	else
	{
		Restyle();
		RefreshCard();
	}
	ApplyStamp();
}

void ULostLootWidget::Refresh()
{
	bRefreshWanted = false;
	// The same ticket stays chosen; a claimed one's place goes to the ticket that took it, else the last.
	const int32 WasTicket = Rows.IsValidIndex(Selected) ? Rows[Selected].Ticket : INDEX_NONE;
	const int32 WasIndex = Selected;
	BuildPanel();
	if (Rows.IsEmpty())
	{
		return;
	}
	int32 Again = Rows.IndexOfByPredicate([WasTicket](const FRow& Row) { return Row.Ticket == WasTicket; });
	if (Again == INDEX_NONE)
	{
		Again = FMath::Clamp(WasIndex, 0, Rows.Num() - 1);
	}
	Select(Again, false, true);
}

// ---------------------------------------------------------------------------
// Choosing
// ---------------------------------------------------------------------------

void ULostLootWidget::Select(int32 Index, bool bSound, bool bScroll)
{
	if (!Rows.IsValidIndex(Index))
	{
		return;
	}
	if (Index != Selected && bSound)
	{
		PlayCue(LooterSoundCue::Hover);
	}
	Selected = Index;
	Restyle();
	RefreshCard();
	if (bScroll && ListScroll && Rows[Index].Button)
	{
		ListScroll->ScrollWidgetIntoView(Rows[Index].Button, /*AnimateScroll*/ true, EDescendantScrollDestination::IntoView);
	}
}

void ULostLootWidget::Step(int32 RowsToMove)
{
	if (Rows.IsEmpty())
	{
		return;
	}
	// A long list stops at its ends rather than jumping round them.
	const int32 From = Rows.IsValidIndex(Selected) ? Selected : 0;
	const int32 To = FMath::Clamp(From + RowsToMove, 0, Rows.Num() - 1);
	if (To != Selected)
	{
		Select(To, true, true);
	}
}

void ULostLootWidget::Restyle()
{
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		const FRow& Row = Rows[Index];
		const bool bChosen = Index == Selected;
		if (Row.Fill)
		{
			Row.Fill->SetColorAndOpacity(bChosen ? LoadoutParts::Colors::PickedFill() : Color::Row());
		}
		if (Row.Line)
		{
			Row.Line->SetColorAndOpacity(bChosen ? Color::Accent() : Color::RowLine());
		}
	}
}

void ULostLootWidget::SetStatus(const FText& Message, const FLinearColor& TextColor)
{
	StatusWords = Message;
	StatusColor = TextColor;
	if (StatusText)
	{
		StatusText->SetText(Message);
		StatusText->SetColorAndOpacity(FSlateColor(TextColor));
	}
}

void ULostLootWidget::HandleStoreChanged()
{
	// Never under the confirm or the stamp: the list is read again once they're gone.
	if (bConfirmOpen || bStamping || !PanelBox)
	{
		bRefreshWanted = true;
		return;
	}
	Refresh();
}

// ---------------------------------------------------------------------------
// The tick: closing when the player leaves, the purse, the stamp, the stick
// ---------------------------------------------------------------------------

void ULostLootWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (ShouldClose())
	{
		Close();
		return;
	}
	// Another menu may have seen the device change.
	if (GetMenuDevice() != ShownDevice)
	{
		RefreshPrompts();
	}
	Advance(InDeltaTime);
}

void ULostLootWidget::Advance(float DeltaSeconds)
{
	UpdatePurse(DeltaSeconds);
	if (bStamping)
	{
		StampAge += DeltaSeconds;
		if (StampAge >= StampSlamSeconds + StampHoldSeconds + StampOutSeconds)
		{
			// Lifted: the claimed ticket's gone from the list, and the card shows the next.
			bStamping = false;
			StampAge = 0.f;
			Refresh();
		}
		ApplyStamp();
	}
	// The stick held up or down keeps choosing, on the menus' own repeat (never under the confirm or the stamp).
	const int32 Repeats = StickRepeat.Tick(DeltaSeconds);
	for (int32 Repeat = 0; Repeat < Repeats && !bConfirmOpen && !bStamping; ++Repeat)
	{
		Step(MenuNav::VerticalStep(StickRepeat.GetMove()));
	}
}

void ULostLootWidget::UpdatePurse(float DeltaTime)
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

// ---------------------------------------------------------------------------
// What it shows, for the tests
// ---------------------------------------------------------------------------

ULostLootWidget::FView ULostLootWidget::GetView() const
{
	FView View;
	for (const FRow& Row : Rows)
	{
		View.Tickets.Add(Row.Ticket);
	}
	View.Chosen = Rows.IsValidIndex(Selected) ? Rows[Selected].Ticket : INDEX_NONE;
	if (const ULostLootSubsystem* LostLoot = GetLostLoot())
	{
		const FLostLootStore& Store = LostLoot->GetStore();
		const int32 Next = Store.FindNextToGo();
		View.NextToGo = Store.Entries.IsValidIndex(Next) ? Store.Entries[Next].Ticket : INDEX_NONE;
		View.bFull = Store.IsFull();
		View.Count = FString::Printf(TEXT("%d / %d"), Store.Num(), Store.GetCapacity());
		View.UpgradePrice = LostLootRules::UpgradePrice(Store.Upgrades, GetPlayerLevel());
	}
	View.bConfirmOpen = bConfirmOpen;
	View.bStamping = bStamping;
	View.Status = StatusWords.ToString();
	View.Prompts = PromptWords();
	return View;
}

#undef LOCTEXT_NAMESPACE

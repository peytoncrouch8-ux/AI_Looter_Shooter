// UMissionOfferWidget's reading mode: something read in the world (AReadableProp: the keepers' roll, a journal, the payroll
// book). Its words as a document, set off by the worn-brass edge a found letter has on the offer card, scrolled when long,
// and Close. What's said about it after plays as the card goes (the captions wait under it).

#include "UI/World/MissionOfferWidget.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/Actor.h"

#define LOCTEXT_NAMESPACE "MissionOffer"

using namespace LooterUI;

namespace
{
	const FName ActionClose(TEXT("Close"));
	/** A long page scrolls past this height rather than grow the card off the screen. */
	constexpr float PageMaxHeight = 430.f;
}

void UMissionOfferWidget::OpenReading(ALooterHUD* InHUD, AActor* InThing, bool bInOverScreen)
{
	Card = FMissionOfferCard::MakeReading(InThing, GetWorld());
	OpenAs(EMissionCardMode::Read, InHUD, nullptr, InThing, bInOverScreen);
}

void UMissionOfferWidget::BuildReadContent(UVerticalBox* Content)
{
	// The words, in the regular weight a paragraph reads easiest in, a touch bigger than a briefing: this is the thing itself.
	UTextBlock* Words = MakeText(WidgetTree, Card.Briefing.ToString(), 15, Color::Text());
	Words->SetFont(Font(15, false));
	Words->SetAutoWrapText(true);
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	StyleScrollBox(Scroll);
	Scroll->AddChild(Words);
	USizeBox* Page = MakeSized(WidgetTree, Scroll, 0.f, 0.f);
	Page->SetMaxDesiredHeight(PageMaxHeight);

	// A worn-brass edge down its side, like a page held up to the light.
	UHorizontalBox* Edged = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Edged->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::Curse())), 3.f));
	UHorizontalBoxSlot* PageSlot = Edged->AddChildToHorizontalBox(Page);
	PageSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	PageSlot->SetPadding(FMargin(16.f, 2.f, 0.f, 2.f));
	Content->AddChildToVerticalBox(Edged);

	// Close, the one thing to do.
	DeclineButton = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	DeclineButton->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), ActionClose, 0, Card.Decline, 14,
		EButtonKind::Normal);
	DeclineButton->bPlaysSounds = false;
	DeclineButton->OnButtonClicked.BindUObject(this, &UMissionOfferWidget::HandleButton);
	DeclineButton->OnButtonHovered.BindUObject(this, &UMissionOfferWidget::HandleHovered);
	UVerticalBoxSlot* CloseSlot = Content->AddChildToVerticalBox(MakeSized(WidgetTree, DeclineButton, 220.f, 42.f));
	CloseSlot->SetHorizontalAlignment(HAlign_Center);
	CloseSlot->SetPadding(FMargin(0.f, 20.f, 0.f, 0.f));
}

#undef LOCTEXT_NAMESPACE

// UMissionOfferWidget: the card (its ! and kind line, title, giver, story, objectives, rewards and the two choices), opening,
// accepting, declining. Its motion is MissionOfferWidgetMotion.cpp's, its words MissionOfferWidgetCard.cpp's, its keys,
// stick and mouse MissionOfferWidgetInput.cpp's.

#include "UI/World/MissionOfferWidget.h"
#include "Audio/LooterSound.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/MissionMarkIcons.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
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

#define LOCTEXT_NAMESPACE "MissionOffer"

using namespace LooterUI;

namespace
{
	const FName ActionAccept(TEXT("Accept"));
	const FName ActionDecline(TEXT("Decline"));

	constexpr float PanelWidth = 720.f;
	/** The ! in the card's corner, and the glow round it. */
	constexpr float MarkBox = 68.f;
	constexpr float MarkSize = 52.f;
	constexpr float RewardLabelWidth = 118.f;

	/** The objectives' marker: the diamond the Missions page draws by the step being played. */
	const FVectorIcon& DiamondIcon()
	{
		static const FVectorIcon Icon = []
		{
			FVectorIcon Diamond;
			Diamond.ViewBox = FVector2D(12.f, 12.f);
			Diamond.Fills.Add({ FVector2D(6.0, 1.0), FVector2D(11.0, 6.0), FVector2D(6.0, 11.0), FVector2D(1.0, 6.0) });
			return Diamond;
		}();
		return Icon;
	}
}

// ---------------------------------------------------------------------------
// Opening, accepting, declining
// ---------------------------------------------------------------------------

void UMissionOfferWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// The card holds the keyboard (E, Esc, the arrows); its buttons never take it.
	SetIsFocusable(true);
}

void UMissionOfferWidget::Open(ALooterHUD* InHUD, const UMissionDefinition* InMission, AActor* InGiver, bool bInOverScreen)
{
	OwningHUD = InHUD;
	Mission = InMission;
	Giver = InGiver;
	bOverScreen = bInOverScreen;
	bAccepted = false;
	bClosing = false;
	Age = 0.f;
	CloseAge = 0.f;
	NavStick = FVector2D::ZeroVector;
	NavStickMove = MenuNav::EMove::None;
	Card = InMission ? FMissionOfferCard::Make(*InMission, InGiver, GetWorld()) : FMissionOfferCard();

	// Opened before it was first shown, the card is built as the widget is.
	if (WidgetTree && WidgetTree->RootWidget)
	{
		BuildCard();
	}
	ApplyMotion();
	LooterSound::Play2D(this, LooterSoundCue::Giver::Offer);
}

bool UMissionOfferWidget::Accept()
{
	if (!IsArmed())
	{
		return false;
	}
	UMissionRunner* Runner = UMissionRunner::Get(this);
	const UMissionDefinition* Offered = Mission.Get();
	const FName MissionId = Offered ? Offered->GetMissionId() : NAME_None;
	if (!Runner || !Offered || !Runner->AcceptMission(MissionId))
	{
		LooterSound::Play2D(this, LooterSoundCue::Denied);
		const bool bTooSoon = Runner && Offered && !Runner->ArePrerequisitesMet(*Offered);
		SetStatus(bTooSoon ? LOCTEXT("TooSoon", "Not yet: finish what comes before it first.")
			: LOCTEXT("CantTake", "This can't be taken on here."));
		return false;
	}
	// What the player just took is what they follow: the tracker shows it, the minimap points the way.
	Runner->TrackMission(MissionId);
	bAccepted = true;
	bClosing = true;
	CloseAge = 0.f;
	Nav.SetShown(false);
	SetStatus(FText::GetEmpty());
	// The stamp comes down with its sound; the card goes once it has been seen.
	LooterSound::Play2D(this, LooterSoundCue::Giver::Accept);
	ApplyMotion();
	return true;
}

void UMissionOfferWidget::Decline()
{
	if (bClosing)
	{
		return;
	}
	bClosing = true;
	bAccepted = false;
	CloseAge = 0.f;
	Nav.SetShown(false);
	LooterSound::Play2D(this, LooterSoundCue::Giver::Decline);
}

void UMissionOfferWidget::FinishClose()
{
	if (ALooterHUD* HUD = OwningHUD.Get())
	{
		HUD->CloseMissionOffer();
	}
	else
	{
		RemoveFromParent();
	}
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UMissionOfferWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		// The whole screen answers the mouse, so a click beside the card lands on it and the keyboard stays with it.
		Root->SetVisibility(ESlateVisibility::Visible);
		WidgetTree->RootWidget = Root;

		// The world (or the board's screen) stays in view behind the card, dimmed.
		Backdrop = MakeImage(WidgetTree, RectBrush(Color::Backdrop()));
		MarkBackground(Backdrop);
		FillOverlaySlot(Root->AddChildToOverlay(Backdrop));

		PanelBox = MakeSized(WidgetTree, nullptr, PanelWidth);
		UOverlaySlot* PanelSlot = Root->AddChildToOverlay(PanelBox);
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);

		BuildCard();
		ApplyMotion();
	}
	return Super::RebuildWidget();
}

void UMissionOfferWidget::BuildCard()
{
	if (!PanelBox)
	{
		return;
	}
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Content->AddChildToVerticalBox(MakeHeader());

	// A cyan hairline under the header: who and what above, the job below.
	UImage* Rule = MakeImage(WidgetTree, RectBrush(Color::ScreenLine()));
	Content->AddChildToVerticalBox(MakeSized(WidgetTree, Rule, 0.f, 1.f))->SetPadding(FMargin(0.f, 14.f, 0.f, 14.f));

	if (UWidget* Briefing = MakeBriefing())
	{
		Content->AddChildToVerticalBox(Briefing)->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
	}
	if (UWidget* Objectives = MakeObjectives())
	{
		Content->AddChildToVerticalBox(Objectives)->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
	}
	Content->AddChildToVerticalBox(MakeRewards());

	// A word when Accept can't be done (what comes first isn't finished); it keeps no room while empty.
	StatusText = MakeText(WidgetTree, FString(), 12, Color::Worse());
	StatusText->SetAutoWrapText(true);
	StatusText->SetVisibility(ESlateVisibility::Collapsed);
	Content->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));

	Content->AddChildToVerticalBox(MakeButtons())->SetPadding(FMargin(0.f, 20.f, 0.f, 0.f));
	PromptBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* PromptSlot = Content->AddChildToVerticalBox(PromptBar);
	PromptSlot->SetHorizontalAlignment(HAlign_Center);
	PromptSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 2.f));

	// The panel, and the stamp over it once accepted.
	UOverlay* Body = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	FillOverlaySlot(Body->AddChildToOverlay(MakePanel(WidgetTree, LOCTEXT("PanelTitle", "New mission").ToString(), Content)));
	Stamp = MakeStamp();
	UOverlaySlot* StampSlot = Body->AddChildToOverlay(Stamp);
	StampSlot->SetHorizontalAlignment(HAlign_Center);
	StampSlot->SetVerticalAlignment(VAlign_Center);
	CardBody = Body;
	PanelBox->SetContent(Body);

	RebuildNav();
	RefreshNavLook();
}

UWidget* UMissionOfferWidget::MakeHeader()
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	// The giver's mark, as it floats over them in the world: gold for the story, white for a side job, in its glow.
	UOverlay* Mark = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	MarkHalo = MakeImage(WidgetTree, MissionMarkIcons::HaloBrush(Card.bMain, FVector2D(MarkBox), 0.55f));
	FillOverlaySlot(Mark->AddChildToOverlay(MarkHalo));
	MarkImage = MakeImage(WidgetTree, MissionMarkIcons::MarkBrush(MissionMarkIcons::EMark::Offer, Card.bMain, FVector2D(MarkSize)));
	MarkImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	UOverlaySlot* GlyphSlot = Mark->AddChildToOverlay(MarkImage);
	GlyphSlot->SetHorizontalAlignment(HAlign_Center);
	GlyphSlot->SetVerticalAlignment(VAlign_Center);
	UHorizontalBoxSlot* MarkSlot = Row->AddChildToHorizontalBox(MakeSized(WidgetTree, Mark, MarkBox, MarkBox));
	MarkSlot->SetVerticalAlignment(VAlign_Top);

	// The kind in the mark's colour over the title, and who offers it under them.
	UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Words->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, Card.Kind.ToString(), 11, MissionMarkIcons::Color(Card.bMain), 320));
	UTextBlock* Title = MakeText(WidgetTree, Card.Title.ToString(), 26, Color::Text(), true, 40);
	Title->SetAutoWrapText(true);
	Words->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
	if (!Card.From.IsEmpty())
	{
		Words->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, Card.From.ToString(), 10, Color::TextDim(), 160))
			->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	}
	UHorizontalBoxSlot* WordsSlot = Row->AddChildToHorizontalBox(Words);
	WordsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	WordsSlot->SetVerticalAlignment(VAlign_Center);
	WordsSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
	return Row;
}

UWidget* UMissionOfferWidget::MakeBriefing()
{
	if (Card.Briefing.IsEmpty())
	{
		return nullptr;
	}
	UTextBlock* Words = MakeText(WidgetTree, Card.Briefing.ToString(), 14, Color::Text());
	// A paragraph reads easier in the regular weight than the kit's bold.
	Words->SetFont(Font(14, false));
	Words->SetAutoWrapText(true);
	if (!Card.bDocument)
	{
		return Words;
	}
	// A found thing's own words, set off by a worn-brass edge like a page held up to the light.
	UHorizontalBox* Page = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Page->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::Curse())), 3.f));
	UHorizontalBoxSlot* WordsSlot = Page->AddChildToHorizontalBox(Words);
	WordsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	WordsSlot->SetPadding(FMargin(14.f, 2.f, 0.f, 2.f));
	return Page;
}

UWidget* UMissionOfferWidget::MakeObjectives()
{
	if (Card.Objectives.IsEmpty())
	{
		return nullptr;
	}
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Box->AddChildToVerticalBox(MakeSection(WidgetTree, LOCTEXT("Objectives", "Objectives").ToString()));
	const FSlateBrush Marker = IconBrush(TEXT("MissionOfferStep"), DiamondIcon(), 3.f, FVector2D(10.f, 10.f), Color::Accent());
	for (const FString& Objective : Card.Objectives)
	{
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UHorizontalBoxSlot* MarkerSlot = Line->AddChildToHorizontalBox(MakeImage(WidgetTree, Marker));
		MarkerSlot->SetVerticalAlignment(VAlign_Top);
		MarkerSlot->SetPadding(FMargin(2.f, 5.f, 12.f, 0.f));
		UTextBlock* Words = MakeText(WidgetTree, Objective, 13, Color::Text());
		Words->SetAutoWrapText(true);
		Line->AddChildToHorizontalBox(Words)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Box->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	}
	if (Card.MoreSteps > 0)
	{
		const FText More = Card.MoreSteps == 1 ? LOCTEXT("OneMore", "And one more step once it's under way")
			: FText::Format(LOCTEXT("MoreSteps", "And {0} more steps once it's under way"), Card.MoreSteps);
		Box->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, More.ToString(), 8, Color::TextDim(), 150))
			->SetPadding(FMargin(24.f, 8.f, 0.f, 0.f));
	}
	return Box;
}

UWidget* UMissionOfferWidget::MakeRewards()
{
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Box->AddChildToVerticalBox(MakeSection(WidgetTree, LOCTEXT("Rewards", "Rewards").ToString()));
	if (Card.Rewards.IsEmpty())
	{
		Box->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, LOCTEXT("NoRewards", "The work is its own reward").ToString(), 9,
			Color::TextDim(), 150))->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
		return Box;
	}
	// Each on its line: what it is, small, then how much in its own colour (a gun's rarity's, experience's cyan).
	for (const FMissionOfferReward& Reward : Card.Rewards)
	{
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UHorizontalBoxSlot* LabelSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree,
			LoadoutParts::Label(WidgetTree, Reward.Label.ToString(), 9, Color::TextDim(), 200), RewardLabelWidth));
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Value = MakeText(WidgetTree, Reward.Value.ToString(), 15, Reward.Color, false, 20);
		Value->SetAutoWrapText(true);
		UHorizontalBoxSlot* ValueSlot = Line->AddChildToHorizontalBox(Value);
		ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ValueSlot->SetVerticalAlignment(VAlign_Center);
		Box->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	}
	return Box;
}

UWidget* UMissionOfferWidget::MakeButtons()
{
	// Not now on the left; Accept, the call to action, on the right (as the station board's confirm).
	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	DeclineButton = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	DeclineButton->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), ActionDecline, 0, Card.Decline, 14,
		EButtonKind::Normal);
	AcceptButton = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	AcceptButton->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), ActionAccept, 0, Card.Accept, 15,
		EButtonKind::Primary);
	for (ULooterButton* Button : { DeclineButton.Get(), AcceptButton.Get() })
	{
		// The card sounds its own choices (the stamp, the paper set down) and its own pointing.
		Button->bPlaysSounds = false;
		Button->OnButtonClicked.BindUObject(this, &UMissionOfferWidget::HandleButton);
		Button->OnButtonHovered.BindUObject(this, &UMissionOfferWidget::HandleHovered);
	}
	UHorizontalBoxSlot* DeclineSlot = Buttons->AddChildToHorizontalBox(DeclineButton);
	DeclineSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	DeclineSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	Buttons->AddChildToHorizontalBox(AcceptButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	return MakeSized(WidgetTree, Buttons, 0.f, 46.f);
}

UWidget* UMissionOfferWidget::MakeStamp()
{
	// ACCEPTED, pressed over the card at a tilt in the mark's colour: on dark glass so it reads over the words under it.
	const FLinearColor Ink = MissionMarkIcons::Color(Card.bMain);
	const FLinearColor Glass = Color::ScreenBg();
	UBorder* Plate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Plate->SetBrush(RectBrush(FLinearColor(Glass.R, Glass.G, Glass.B, 0.82f), Ink, 3.f));
	MarkBackground(Plate);
	Plate->SetPadding(FMargin(26.f, 8.f, 26.f, 10.f));
	UTextBlock* Word = MakeFloatingText(WidgetTree, 34, Ink, 520, ETextJustify::Center);
	Word->SetText(LOCTEXT("Accepted", "ACCEPTED"));
	Plate->SetContent(Word);
	Plate->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	Plate->SetVisibility(ESlateVisibility::Collapsed);
	return Plate;
}

void UMissionOfferWidget::SetStatus(const FText& Message)
{
	if (StatusText)
	{
		StatusText->SetText(Message);
		StatusText->SetVisibility(Message.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

// Its motion (the slide, the fades, the ! popping, the stamp): MissionOfferWidgetMotion.cpp.

#undef LOCTEXT_NAMESPACE

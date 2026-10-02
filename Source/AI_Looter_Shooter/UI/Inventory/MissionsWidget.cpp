#include "UI/Inventory/MissionsWidget.h"
#include "Areas/AreaDefinition.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"

using namespace LooterUI;
using namespace LoadoutParts;

namespace
{
	const FName ActionMission(TEXT("Mission"));

	constexpr float ColumnHeight = 690.f;
	/** The details column: from beside the list to the page's right margin (no stand on this page). */
	constexpr float DetailsX = 540.f;
	constexpr float DetailsWidth = 1000.f;

	/** The list's sections, in order. Locked missions aren't listed: what's ahead stays a surprise. */
	const EMissionStatus Sections[] = { EMissionStatus::Active, EMissionStatus::Available, EMissionStatus::Completed };

	FString SectionName(EMissionStatus Status)
	{
		switch (Status)
		{
		case EMissionStatus::Active:    return TEXT("Active");
		case EMissionStatus::Available: return TEXT("Available");
		case EMissionStatus::Completed: return TEXT("Completed");
		case EMissionStatus::Locked:    break;
		}
		return FString();
	}

	/** A main mission's chip is orange, a side one's cyan, a tutorial's grey. */
	FLinearColor KindColor(EMissionKind Kind)
	{
		switch (Kind)
		{
		case EMissionKind::Main:     return Color::Accent();
		case EMissionKind::Side:     return Color::Title();
		case EMissionKind::Tutorial: break;
		}
		return Color::TextDim();
	}

	/** A done step's tick, and the marker of the step being played. */
	const FVectorIcon& DoneIcon()
	{
		static const FVectorIcon Icon = []
		{
			FVectorIcon Tick;
			Tick.ViewBox = FVector2D(12.f, 12.f);
			TArray<FVector2D>& Stroke = Tick.Strokes.AddDefaulted_GetRef();
			Stroke = { FVector2D(2.0, 6.5), FVector2D(5.0, 9.5), FVector2D(10.0, 2.5) };
			Tick.StrokeWidth = 1.8f;
			return Tick;
		}();
		return Icon;
	}

	const FVectorIcon& CurrentIcon()
	{
		static const FVectorIcon Icon = []
		{
			FVectorIcon Diamond;
			Diamond.ViewBox = FVector2D(12.f, 12.f);
			// Clockwise on screen, as the kit's fills are.
			TArray<FVector2D>& Shape = Diamond.Fills.AddDefaulted_GetRef();
			Shape = { FVector2D(6.0, 1.0), FVector2D(11.0, 6.0), FVector2D(6.0, 11.0), FVector2D(1.0, 6.0) };
			return Diamond;
		}();
		return Icon;
	}
}

// ---------------------------------------------------------------------------
// Opening and closing
// ---------------------------------------------------------------------------

void UMissionsWidget::Open(ALooterHUD* InHUD)
{
	OwningHUD = InHUD;
	SetIsFocusable(true);

	// Follow the level's missions, so progress made while the page is open (a hit, a step) shows. Bound once: the page
	// only notes a change, and redraws on its next tick, which only comes while it's on screen.
	UMissionRunner* Current = GetWorld() ? GetWorld()->GetSubsystem<UMissionRunner>() : nullptr;
	if (Current != Runner.Get())
	{
		if (UMissionRunner* Old = Runner.Get())
		{
			Old->OnChanged.Remove(ChangedHandle);
		}
		Runner = Current;
		ChangedHandle = Current ? Current->OnChanged.AddUObject(this, &UMissionsWidget::HandleRunnerChanged) : FDelegateHandle();
	}
	bDirty = false;

	Gather();
	RebuildList();
	Restyle();
	RefreshDetails();
	RefreshPrompts();
}

void UMissionsWidget::Close()
{
	if (ALooterHUD* HUD = OwningHUD.Get())
	{
		HUD->CloseInventory();
	}
}

void UMissionsWidget::ShowPage(EInventoryPage Page)
{
	if (ALooterHUD* HUD = OwningHUD.Get())
	{
		HUD->ShowInventoryPage(Page);
	}
}

void UMissionsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (bDirty)
	{
		bDirty = false;
		Gather();
		RebuildList();
		Restyle();
		RefreshDetails();
		RefreshPrompts();
	}
}

void UMissionsWidget::HandleRunnerChanged()
{
	bDirty = true;
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UMissionsWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// The world stays in view behind the page, dimmed, as behind the other pages.
		UImage* Backdrop = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Backdrop"));
		Backdrop->SetBrush(RectBrush(Colors::Dim()));
		MarkBackground(Backdrop);
		FillOverlaySlot(Root->AddChildToOverlay(Backdrop));

		UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
		Scale->SetStretch(EStretch::ScaleToFit);
		FillOverlaySlot(Root->AddChildToOverlay(Scale));
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Page"));
		Scale->SetContent(MakeSized(WidgetTree, Canvas, PageSize.X, PageSize.Y));

		auto Place = [Canvas](UWidget* Widget, const FVector2D& Position, const FVector2D& Size, const FVector2D& Alignment = FVector2D::ZeroVector)
		{
			UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Widget);
			CanvasSlot->SetPosition(Position);
			CanvasSlot->SetAlignment(Alignment);
			if (Size.IsZero())
			{
				CanvasSlot->SetAutoSize(true);
			}
			else
			{
				CanvasSlot->SetSize(Size);
			}
		};

		// Title tabs: the loadout, the bestiary and this page.
		TArray<ULooterButton*> Tabs;
		Place(MakePageTabs(WidgetTree, static_cast<int32>(EInventoryPage::Missions), Tabs), PageTabsPosition, FVector2D::ZeroVector, FVector2D(0.5f, 0.f));
		for (ULooterButton* Tab : Tabs)
		{
			Tab->OnButtonClicked.BindUObject(this, &UMissionsWidget::HandleTabClicked);
		}

		// Left: the mission log, by section.
		{
			UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			Header->AddChildToHorizontalBox(Label(WidgetTree, TEXT("Mission log"), 10, Color::Accent(), 300));
			ListCount = Label(WidgetTree, TEXT(""), 10, Color::TextDim(), 200);
			Header->AddChildToHorizontalBox(ListCount)->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
			Left->AddChildToVerticalBox(Header);
			ListBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
			StyleScrollBox(ListBox);
			UVerticalBoxSlot* ListSlot = Left->AddChildToVerticalBox(ListBox);
			ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ListSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
			Place(Left, FVector2D(LeftX, ColumnTop), FVector2D(LeftWidth, ColumnHeight));
		}

		// Right: the chosen mission's card, then its steps and rewards under it.
		{
			UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			DetailsHeader = Label(WidgetTree, TEXT(""), 10, Color::Accent(), 300);
			Right->AddChildToVerticalBox(DetailsHeader);
			DetailsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			UImage* DetailsFill = nullptr;
			UImage* DetailsLine = nullptr;
			UOverlay* DetailsCard = MakeCard(WidgetTree, DetailsBox, FMargin(18.f, 14.f), 1.7f, DetailsFill, DetailsLine);
			DetailsFill->SetColorAndOpacity(Colors::CardFill());
			DetailsLine->SetColorAndOpacity(Hex(90, 200, 255, 102));
			Right->AddChildToVerticalBox(DetailsCard)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));

			UScrollBox* StepsScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
			StyleScrollBox(StepsScroll);
			StepsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			StepsScroll->AddChild(StepsBox);
			UVerticalBoxSlot* StepsSlot = Right->AddChildToVerticalBox(StepsScroll);
			StepsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			StepsSlot->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
			Place(Right, FVector2D(DetailsX, ColumnTop), FVector2D(DetailsWidth, ColumnHeight));
		}

		// Bottom: what the keys do.
		PromptBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Place(PromptBar, FVector2D(800.f, 852.f), FVector2D::ZeroVector, FVector2D(0.5f, 0.f));

		DoneBrush = IconBrush(TEXT("MissionStepDone"), DoneIcon(), 3.f, FVector2D(12.f, 12.f), Color::Better());
		CurrentBrush = IconBrush(TEXT("MissionStepCurrent"), CurrentIcon(), 3.f, FVector2D(12.f, 12.f), Color::Accent());

		// Opened before it was first shown: the contents can go in now.
		RebuildList();
		Restyle();
		RefreshDetails();
		RefreshPrompts();
	}
	return Super::RebuildWidget();
}

// ---------------------------------------------------------------------------
// The list
// ---------------------------------------------------------------------------

void UMissionsWidget::Gather()
{
	// Read every time, so a mission started or finished since shows up; the choice stays on the same mission.
	const UMissionDefinition* Previous = GetSelected();
	Listed.Reset();
	Statuses.Reset();
	AreaNames.Reset();
	for (const UAreaDefinition* Area : UAreaDefinition::LoadAll())
	{
		AreaNames.Add(Area->GetAreaId(), Area->DisplayName.IsEmpty() ? Area->GetAreaId().ToString() : Area->DisplayName.ToString());
	}
	const UMissionRunner* MissionRunner = GetRunner();
	if (MissionRunner)
	{
		for (const EMissionStatus Section : Sections)
		{
			for (UMissionDefinition* Mission : MissionRunner->GetDefinitions())
			{
				if (Mission && MissionRunner->GetStatus(*Mission) == Section)
				{
					Listed.Add(Mission);
					Statuses.Add(Section);
				}
			}
		}
	}
	Selected = FMath::Max(Listed.IndexOfByKey(Previous), 0);
}

void UMissionsWidget::RebuildList()
{
	if (!ListBox)
	{
		return;
	}
	ListBox->ClearChildren();
	Cards.Reset();
	Cards.SetNum(Listed.Num());
	int32 Active = 0;
	for (const EMissionStatus Status : Statuses)
	{
		Active += Status == EMissionStatus::Active ? 1 : 0;
	}
	ListCount->SetText(FText::FromString(FString::Printf(TEXT("%d active"), Active).ToUpper()));

	for (int32 SectionIndex = 0; SectionIndex < UE_ARRAY_COUNT(Sections); ++SectionIndex)
	{
		const EMissionStatus Section = Sections[SectionIndex];
		int32 InSection = 0;
		for (const EMissionStatus Status : Statuses)
		{
			InSection += Status == Section ? 1 : 0;
		}

		UHorizontalBox* Head = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Head->AddChildToHorizontalBox(Label(WidgetTree, SectionName(Section), 8, Color::TextDim(), 220))->SetVerticalAlignment(VAlign_Center);
		UHorizontalBoxSlot* RuleSlot = Head->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Hex(90, 200, 255, 61))), 0.f, 1.f));
		RuleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		RuleSlot->SetVerticalAlignment(VAlign_Center);
		RuleSlot->SetPadding(FMargin(10.f, 0.f));
		Head->AddChildToHorizontalBox(Label(WidgetTree, FString::FromInt(InSection), 8, Color::TextDim(), 150))->SetVerticalAlignment(VAlign_Center);
		Cast<UScrollBoxSlot>(ListBox->AddChild(Head))->SetPadding(FMargin(0.f, SectionIndex > 0 ? 16.f : 4.f, 6.f, 2.f));

		if (InSection == 0)
		{
			const TCHAR* Empty = Section == EMissionStatus::Active ? TEXT("Nothing under way") : TEXT("Nothing here yet");
			Cast<UScrollBoxSlot>(ListBox->AddChild(Label(WidgetTree, Empty, 8, Hex(143, 179, 204, 140), 150)))->SetPadding(FMargin(2.f, 6.f, 6.f, 0.f));
			continue;
		}
		for (int32 Index = 0; Index < Listed.Num(); ++Index)
		{
			if (Statuses[Index] == Section)
			{
				Cards[Index] = MakeMissionCard(Index);
				Cast<UScrollBoxSlot>(ListBox->AddChild(Cards[Index].Button))->SetPadding(FMargin(0.f, 6.f, 6.f, 0.f));
			}
		}
	}
}

UMissionsWidget::FCard UMissionsWidget::MakeMissionCard(int32 Index)
{
	FCard Card;
	const UMissionDefinition& Mission = *Listed[Index];
	const EMissionStatus Status = Statuses[Index];
	const UMissionRunner* MissionRunner = GetRunner();
	const bool bTracked = MissionRunner && MissionRunner->GetTrackedMission() == Mission.GetMissionId();
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	// Its kind, as a small chip: main, side or tutorial.
	const FLinearColor Kind = KindColor(Mission.Kind);
	UBorder* Chip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Chip->SetBrush(RectBrush(Color::Plate(), Kind * FLinearColor(1.f, 1.f, 1.f, 0.7f), 1.f));
	MarkBackground(Chip);
	Chip->SetPadding(FMargin(6.f, 2.f));
	Chip->SetHorizontalAlignment(HAlign_Center);
	Chip->SetContent(Label(WidgetTree, UMissionDefinition::KindName(Mission.Kind).ToString(), 7, Kind, 150));
	// Wide enough for the longest kind, "TUTORIAL", in its spaced-out capitals.
	UHorizontalBoxSlot* ChipSlot = Row->AddChildToHorizontalBox(MakeSized(WidgetTree, Chip, 78.f, 0.f));
	ChipSlot->SetVerticalAlignment(VAlign_Center);

	// The title over where it stands.
	UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Words->AddChildToVerticalBox(FittedLabel(WidgetTree, Mission.Title.ToString(), 10,
		Status == EMissionStatus::Completed ? Color::TextDim() : Color::Text(), 50));
	const FString Standing = DescribeStanding(Mission, Status);
	Words->AddChildToVerticalBox(FittedLabel(WidgetTree, bTracked ? Standing + TEXT(" · Tracked") : Standing, 8,
		bTracked ? Color::Accent() : Color::TextDim(), 120))->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
	UHorizontalBoxSlot* WordsSlot = Row->AddChildToHorizontalBox(Words);
	WordsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	WordsSlot->SetVerticalAlignment(VAlign_Center);
	WordsSlot->SetPadding(FMargin(12.f, 0.f, 8.f, 0.f));

	// On the right: how far along a running one is, or a tick for a finished one.
	if (Status == EMissionStatus::Completed)
	{
		Row->AddChildToHorizontalBox(MakeImage(WidgetTree, DoneBrush))->SetVerticalAlignment(VAlign_Center);
	}
	else if (MissionRunner && MissionRunner->IsRunning(Mission.GetMissionId()))
	{
		const FString Steps = FString::Printf(TEXT("%d/%d"), MissionRunner->GetStep(Mission.GetMissionId()) + 1, Mission.Steps.Num());
		Row->AddChildToHorizontalBox(Label(WidgetTree, Steps, 12, Color::Text(), 40))->SetVerticalAlignment(VAlign_Center);
	}

	UOverlay* Box = MakeCard(WidgetTree, Row, FMargin(12.f, 7.f), 1.3f, Card.Fill, Card.Line);
	Card.Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Card.Button->SetupContent(MakeSized(WidgetTree, Box, 0.f, ListCardHeight), ActionMission, Index);
	Card.Button->OnButtonClicked.BindUObject(this, &UMissionsWidget::HandleCardClicked);
	Card.Button->OnButtonHovered.BindUObject(this, &UMissionsWidget::HandleCardHovered);
	return Card;
}

void UMissionsWidget::Select(int32 Index, bool bScrollIntoView)
{
	if (!Listed.IsValidIndex(Index) || Index == Selected)
	{
		return;
	}
	Selected = Index;
	Restyle();
	RefreshDetails();
	RefreshPrompts();
	if (bScrollIntoView && Cards.IsValidIndex(Index) && Cards[Index].Button)
	{
		ListBox->ScrollWidgetIntoView(Cards[Index].Button, false, EDescendantScrollDestination::IntoView, 8.f);
	}
}

void UMissionsWidget::Restyle()
{
	for (int32 Index = 0; Index < Cards.Num(); ++Index)
	{
		const FCard& Card = Cards[Index];
		if (Card.Button)
		{
			const bool bSelected = Index == Selected;
			Card.Fill->SetColorAndOpacity(bSelected ? Color::Tile() : Colors::CardFill());
			Card.Line->SetColorAndOpacity(bSelected ? Color::Accent() : Colors::CardLine());
		}
	}
}

void UMissionsWidget::HandleCardClicked(ULooterButton* Button)
{
	HandleCardHovered(Button);
	// Clicking handed keyboard focus to the game viewport (LooterButton); take it back for the page's keys.
	SetKeyboardFocus();
}

void UMissionsWidget::HandleCardHovered(ULooterButton* Button)
{
	if (Button)
	{
		Select(Button->Index, false);
	}
}

void UMissionsWidget::HandleTabClicked(ULooterButton* Button)
{
	if (Button && Button->Index != static_cast<int32>(EInventoryPage::Missions))
	{
		ShowPage(static_cast<EInventoryPage>(Button->Index));
	}
	else
	{
		SetKeyboardFocus();
	}
}

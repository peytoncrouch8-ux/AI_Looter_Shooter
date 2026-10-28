// ULoadoutWidget: the page's layout (2026-10-28, the user's ask: the backpack back on the right). Left the EQUIPPED slots,
// the WARD and the AMMO; right the BACKPACK; the middle the showcase over the gun card; the prompt bar at the bottom. On
// Inspect the columns go, the gun takes the left and its plain card, every stat, the backpack's place.

#include "UI/Inventory/LoadoutWidget.h"
#include "UI/Cards/GunCardWidget.h"
#include "UI/Cards/LooterCardBackdrop.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Inventory/LoadoutPaintLayer.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Materials/MaterialInstanceDynamic.h"

using namespace LooterUI;
using namespace LoadoutParts;

namespace
{
	/** The plain card's head, over a maker's sweep: the mark's size, and the gap under the head (or with no head, at the top). */
	constexpr float HeadMarkSize = 15.f;
	constexpr float NoHeadGap = 14.f;
	constexpr float BodyGap = 8.f;
	/** The middle card's centre on the page. */
	constexpr float PageMiddle = 800.f;
}

TSharedRef<SWidget> ULoadoutWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// The world stays in view behind the screen, dimmed.
		UImage* Backdrop = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Backdrop"));
		Backdrop->SetBrush(RectBrush(Colors::Dim()));
		MarkBackground(Backdrop);
		FillOverlaySlot(Root->AddChildToOverlay(Backdrop));

		// Laid out at 1600 x 900, scaled to fit the screen.
		UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
		Scale->SetStretch(EStretch::ScaleToFit);
		FillOverlaySlot(Root->AddChildToOverlay(Scale));
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Page"));
		Scale->SetContent(MakeSized(WidgetTree, Canvas, PageSize.X, PageSize.Y));
		Page = Canvas;

		// The title tabs: this page and the others.
		TArray<ULooterButton*> Tabs;
		PlaceOnPage(MakePageTabs(WidgetTree, static_cast<int32>(EInventoryPage::Loadout), Tabs), PageTabsPosition, FVector2D::ZeroVector, FVector2D(0.5f, 0.f));
		for (ULooterButton* Tab : Tabs)
		{
			Tab->OnButtonClicked.BindUObject(this, &ULoadoutWidget::HandleTabClicked);
		}

		// The showcase: the far half of its ring, the gun's picture, then the near half.
		BackLayer = WidgetTree->ConstructWidget<ULoadoutPaintLayer>(ULoadoutPaintLayer::StaticClass());
		BackLayer->SetPainter([this](const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) { PaintBack(Geometry, Elements, LayerId); });
		BackLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
		PlaceOnPage(BackLayer, FVector2D::ZeroVector, PageSize);

		StageImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Showcase"));
		StageImage->SetBrush(RectBrush(FLinearColor::Transparent));
		StageImage->SetCursor(EMouseCursor::GrabHand);
		StageSlot = PlaceOnPage(StageImage, LoadoutLayout::ShowcaseTopLeft, LoadoutLayout::ShowcaseSize);

		FrontLayer = WidgetTree->ConstructWidget<ULoadoutPaintLayer>(ULoadoutPaintLayer::StaticClass());
		FrontLayer->SetPainter([this](const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) { PaintFront(Geometry, Elements, LayerId); });
		FrontLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
		PlaceOnPage(FrontLayer, FVector2D::ZeroVector, PageSize);

		ShowcaseHint = Label(WidgetTree, TEXT(""), 8, Color::TextDim() * FLinearColor(1.f, 1.f, 1.f, 0.8f), 220);
		ShowcaseHint->SetVisibility(ESlateVisibility::HitTestInvisible);
		ShowcaseHintSlot = PlaceOnPage(ShowcaseHint, FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D(0.5f, 0.f));

		BuildLeftColumn();
		BuildRightColumn();
		BuildCards();

		// Bottom: what the keys do right now.
		PromptBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		PlaceOnPage(PromptBar, LoadoutLayout::PromptBarPosition, FVector2D::ZeroVector, FVector2D(0.5f, 0.f));

		DiscBrush = IconBrush(TEXT("LoadoutDisc"), DiscIcon(), 2.f, FVector2D(64.f, 64.f), FLinearColor::White);

		// Opened before it was first shown: the picture and contents can go in now.
		ApplyStageBrush();
		ApplyMode();
		Refresh();
	}
	return Super::RebuildWidget();
}

UCanvasPanelSlot* ULoadoutWidget::PlaceOnPage(UWidget* Widget, const FVector2D& Position, const FVector2D& Size, const FVector2D& Alignment)
{
	UCanvasPanelSlot* CanvasSlot = Page->AddChildToCanvas(Widget);
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
	return CanvasSlot;
}

UWidget* ULoadoutWidget::MakeSectionHeader(const TCHAR* Title, UTextBlock*& OutCount, UWidget* Extra)
{
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Line->AddChildToHorizontalBox(Label(WidgetTree, Title, 10, Color::Accent(), 300))->SetVerticalAlignment(VAlign_Center);
	OutCount = Label(WidgetTree, TEXT(""), 10, Color::Accent(), 300);
	UHorizontalBoxSlot* CountSlot = Line->AddChildToHorizontalBox(OutCount);
	CountSlot->SetVerticalAlignment(VAlign_Center);
	CountSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
	UHorizontalBoxSlot* GapSlot = Line->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass()));
	GapSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	if (Extra)
	{
		UHorizontalBoxSlot* ExtraSlot = Line->AddChildToHorizontalBox(Extra);
		ExtraSlot->SetVerticalAlignment(VAlign_Center);
		ExtraSlot->SetPadding(FMargin(12.f, 0.f, 8.f, 0.f));
	}
	// A fixed height: the columns' rows are laid out from it (RowCenterY).
	return MakeSized(WidgetTree, Line, 0.f, LoadoutLayout::HeaderHeight);
}

void ULoadoutWidget::BuildLeftColumn()
{
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UTextBlock* Count = nullptr;
	Column->AddChildToVerticalBox(MakeSectionHeader(TEXT("Equipped"), Count));
	EquippedCount = Count;
	SlotList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Column->AddChildToVerticalBox(SlotList)->SetPadding(FMargin(0.f, LoadoutLayout::HeaderGap, 0.f, 0.f));

	BuildWardSection(Column);

	// The ammo, on one quiet line: each kind's icon and the rounds carried, then the silver.
	UTextBlock* NoCount = nullptr;
	Column->AddChildToVerticalBox(MakeSectionHeader(TEXT("Ammo"), NoCount))->SetPadding(FMargin(0.f, LoadoutLayout::SectionGap, 0.f, 0.f));
	AmmoRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Column->AddChildToVerticalBox(AmmoRow)->SetPadding(FMargin(0.f, LoadoutLayout::HeaderGap, 0.f, 0.f));

	LeftColumn = Column;
	PlaceOnPage(Column, FVector2D(LoadoutLayout::ListX, LoadoutLayout::ColumnTop),
		FVector2D(LoadoutLayout::ListWidth, LoadoutLayout::ColumnBottom - LoadoutLayout::ColumnTop));
}

void ULoadoutWidget::BuildRightColumn()
{
	// The backpack's header carries its sort on its right end, which a click (or R) changes.
	SortText = Label(WidgetTree, TEXT(""), 8, Color::TextDim(), 200);
	SortButton = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	SortButton->bPlaysSounds = false;
	SortButton->SetupContent(SortText, ActionPrompt, static_cast<int32>(EAction::Sort));
	SortButton->SetCursor(EMouseCursor::Hand);
	SortButton->OnButtonClicked.BindUObject(this, &ULoadoutWidget::HandlePromptClicked);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UTextBlock* Count = nullptr;
	Column->AddChildToVerticalBox(MakeSectionHeader(TEXT("Backpack"), Count, SortButton));
	BackpackCount = Count;

	// Every place in the backpack, the guns first; it scrolls when a bigger backpack outgrows the column.
	ListBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	StyleScrollBox(ListBox);
	UVerticalBoxSlot* ListSlot = Column->AddChildToVerticalBox(ListBox);
	ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ListSlot->SetPadding(FMargin(0.f, LoadoutLayout::HeaderGap, 0.f, 0.f));

	RightColumn = Column;
	PlaceOnPage(Column, FVector2D(LoadoutLayout::BackpackX, LoadoutLayout::ColumnTop),
		FVector2D(LoadoutLayout::BackpackWidth, LoadoutLayout::ColumnBottom - LoadoutLayout::ColumnTop));
}

void ULoadoutWidget::BuildCards()
{
	// The middle: the gun card under the showcase. It takes no clicks (the showcase and the rows do).
	GunCard = WidgetTree->ConstructWidget<UGunCardWidget>(UGunCardWidget::StaticClass(), TEXT("GunCard"));
	GunCard->SetVisibility(ESlateVisibility::Collapsed);
	PlaceOnPage(GunCard, FVector2D(PageMiddle, LoadoutLayout::CardTop), FVector2D::ZeroVector, FVector2D(0.5f, 0.f));

	// The plain card, for what the gun card doesn't show: a ward, an empty slot, and Inspect's every stat. Its head wears
	// the gun's maker's sweep on Inspect: the mark, the maker's word and its quirk, as the gun card's does.
	{
		UHorizontalBox* Head = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		CardHeadMark = MakeImage(WidgetTree, RectBrush(FLinearColor::Transparent));
		Head->AddChildToHorizontalBox(MakeSized(WidgetTree, CardHeadMark, HeadMarkSize, HeadMarkSize))->SetVerticalAlignment(VAlign_Center);
		CardHeadWord = Label(WidgetTree, TEXT(""), 11, Color::Text(), 250);
		CardHeadWord->SetShadowColorAndOpacity(FLinearColor::Transparent);
		UHorizontalBoxSlot* WordSlot = Head->AddChildToHorizontalBox(CardHeadWord);
		WordSlot->SetVerticalAlignment(VAlign_Center);
		WordSlot->SetPadding(FMargin(6.f, 0.f, 0.f, 0.f));
		UHorizontalBoxSlot* GapSlot = Head->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass()));
		GapSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		CardHeadQuirk = Label(WidgetTree, TEXT(""), 11, Color::TextDim(), 160);
		Head->AddChildToHorizontalBox(CardHeadQuirk)->SetVerticalAlignment(VAlign_Center);
		CardHead = MakeSized(WidgetTree, Head, 0.f, LooterBackdrop::SweepHeight);
		CardHeadGap = MakeSized(WidgetTree, WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass()), 0.f, NoHeadGap);

		CardBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		CardScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
		StyleScrollBox(CardScroll);
		CardScroll->AddChild(CardBox);
		PlainCardLimit = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		PlainCardLimit->SetContent(CardScroll);

		UVerticalBox* Inner = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Inner->AddChildToVerticalBox(CardHead)->SetPadding(FMargin(10.f, 0.f, 18.f, 0.f));
		Inner->AddChildToVerticalBox(CardHeadGap);
		Inner->AddChildToVerticalBox(PlainCardLimit)->SetPadding(FMargin(18.f, BodyGap, 12.f, 16.f));

		ULooterCardBackdrop* Fill = nullptr;
		ULooterCardBackdrop* Lines = nullptr;
		UOverlay* Card = ULooterCardBackdrop::MakeLayered(WidgetTree, ELooterBackdropShape::Card, Inner, FMargin(0.f), Fill, Lines);
		PlainFill = Fill;
		PlainLines = Lines;
		PlainCardSize = MakeSized(WidgetTree, Card, LoadoutLayout::CardWidth, 0.f);
		PlainCard = PlainCardSize;
		PlainCard->SetVisibility(ESlateVisibility::Collapsed);
		PlainCardSlot = PlaceOnPage(PlainCard, FVector2D(PageMiddle, LoadoutLayout::CardTop), FVector2D::ZeroVector, FVector2D(0.5f, 0.f));
	}

	// The gun being dragged, following the mouse: a lit row with its picture and name, and what letting go here would do.
	{
		UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		GhostBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Content->AddChildToVerticalBox(GhostBox);
		GhostAction = Label(WidgetTree, TEXT(""), 8, Color::Accent(), 150);
		Content->AddChildToVerticalBox(GhostAction)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
		ULooterCardBackdrop* GhostFill = nullptr;
		ULooterCardBackdrop* GhostLines = nullptr;
		UOverlay* Card = ULooterCardBackdrop::MakeLayered(WidgetTree, ELooterBackdropShape::Row, Content, FMargin(18.f, 8.f, 14.f, 8.f), GhostFill, GhostLines);
		GhostFill->SetRowState(ELooterRowState::Lit);
		GhostLines->SetRowState(ELooterRowState::Lit);
		DragGhost = MakeSized(WidgetTree, Card, GhostWidth, 0.f);
		DragGhost->SetVisibility(ESlateVisibility::Collapsed);
		DragGhost->SetRenderOpacity(0.92f);
		GhostSlot = Page->AddChildToCanvas(DragGhost);
		GhostSlot->SetAutoSize(true);
	}
}

void ULoadoutWidget::ApplyStageBrush()
{
	if (StageImage && StageMaterial)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(StageMaterial);
		Brush.ImageSize = bInspecting ? LoadoutLayout::InspectShowcaseSize : LoadoutLayout::ShowcaseSize;
		StageImage->SetBrush(Brush);
	}
}

void ULoadoutWidget::ApplyMode()
{
	if (!StageSlot)
	{
		return;
	}
	// Inspect puts the columns away and gives the gun the left; its card takes the backpack's place.
	const FVector2D TopLeft = bInspecting ? LoadoutLayout::InspectShowcaseTopLeft : LoadoutLayout::ShowcaseTopLeft;
	const FVector2D Size = bInspecting ? LoadoutLayout::InspectShowcaseSize : LoadoutLayout::ShowcaseSize;
	StageSlot->SetPosition(TopLeft);
	StageSlot->SetSize(Size);
	ApplyStageBrush();
	const ESlateVisibility Lists = bInspecting ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible;
	if (LeftColumn)
	{
		LeftColumn->SetVisibility(Lists);
	}
	if (RightColumn)
	{
		RightColumn->SetVisibility(Lists);
	}
	if (PlainCardSlot && PlainCardSize && PlainCardLimit)
	{
		if (bInspecting)
		{
			PlainCardSlot->SetPosition(FVector2D(LoadoutLayout::InspectCardX, LoadoutLayout::ColumnTop));
			PlainCardSlot->SetAlignment(FVector2D::ZeroVector);
			PlainCardSize->SetWidthOverride(LoadoutLayout::InspectCardWidth);
			PlainCardLimit->SetMaxDesiredHeight(LoadoutLayout::ColumnBottom - LoadoutLayout::ColumnTop - LooterBackdrop::SweepHeight - BodyGap - 16.f);
		}
		else
		{
			PlainCardSlot->SetPosition(FVector2D(PageMiddle, LoadoutLayout::CardTop));
			PlainCardSlot->SetAlignment(FVector2D(0.5f, 0.f));
			PlainCardSize->SetWidthOverride(LoadoutLayout::CardWidth);
			PlainCardLimit->SetMaxDesiredHeight(LoadoutLayout::ColumnBottom - LoadoutLayout::CardTop - NoHeadGap - BodyGap - 16.f);
		}
	}
	if (ShowcaseHint && ShowcaseHintSlot)
	{
		ShowcaseHint->SetText(FText::FromString(FString(bInspecting ? TEXT("Drag to turn") : TEXT("Drag to turn · click to inspect")).ToUpper()));
		ShowcaseHintSlot->SetPosition(FVector2D(TopLeft.X + Size.X * 0.5f, TopLeft.Y + Size.Y + 4.f));
	}
}

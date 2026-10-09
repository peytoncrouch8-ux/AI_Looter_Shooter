// UMapWidget: opening, the level's map and pins, and the page's layout (the frame and the side column).

#include "UI/Inventory/MapWidget.h"
#include "Areas/AreaDefinition.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionSubsystem.h"
#include "Session/SessionSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Inventory/LoadoutPaintLayer.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Inventory/MapDiscoverySubsystem.h"
#include "UI/Inventory/MapIcons.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "World/MinimapSubsystem.h"
#include "World/PlayableArea.h"
#include "World/PlayableBoundary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
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
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/PackageName.h"

using namespace LooterUI;
using namespace LoadoutParts;

namespace
{
	const FName ActionTravel(TEXT("Travel"));

	/** The legend's rows, top to bottom (the player's own arrow comes first, apart). */
	const EMapPinKind LegendKinds[] = {
		EMapPinKind::Objective, EMapPinKind::TurnIn, EMapPinKind::Grave, EMapPinKind::Station, EMapPinKind::Bench,
		EMapPinKind::Chest, EMapPinKind::ChestLooted,
	};

	/** The legend's badge: a pin as the map draws it, small. */
	constexpr float LegendBadge = 22.f;

	/** Adds Widget to Canvas at Position (its top-left, or the point its Alignment names), sized or sizing itself. */
	UCanvasPanelSlot* PlaceOn(UCanvasPanel& Canvas, UWidget* Widget, const FVector2D& Position, const FVector2D& Size,
		const FVector2D& Alignment = FVector2D::ZeroVector)
	{
		UCanvasPanelSlot* CanvasSlot = Canvas.AddChildToCanvas(Widget);
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
}

// ---------------------------------------------------------------------------
// Opening and closing
// ---------------------------------------------------------------------------

void UMapWidget::Open(ALooterHUD* InHUD)
{
	OwningHUD = InHUD;
	SetIsFocusable(true);
	bPressing = false;
	bDragging = false;
	LeftStick = RightStick = FVector2D::ZeroVector;
	ZoomInAxis = ZoomOutAxis = 0.f;
	Hovered = INDEX_NONE;
	StatusFlash = 0.f;
	OpenTime = 0.f;

	ResolveLevel();
	RefreshPins();
	ResetView();
	// Opened on the same grave as last time, while it's still open; otherwise none chosen.
	if (!FindGravePin(SelectedGrave))
	{
		SelectedGrave = NAME_None;
	}
	PinRefreshLeft = 0.5f;
	CardRefreshLeft = 0.f;
	ShownStatus.Reset();
	RefreshLegend();
	RebuildGraveList();
	RefreshTravelCard();
	RefreshPrompts();
	LayoutMap(0.f);
}

void UMapWidget::Close()
{
	if (ALooterHUD* HUD = OwningHUD.Get())
	{
		HUD->CloseInventory();
	}
}

void UMapWidget::ResolveLevel()
{
	UWorld* World = GetWorld();
	const FString Map = USessionSubsystem::MapOf(World);
	LevelName = FPackageName::GetShortName(Map);
	Places = MapPlaces::For(LevelName);

	// The area's name ("Ransom's Rest"), else the level's own as the session names it.
	const UAreaRulesSubsystem* Rules = World ? World->GetSubsystem<UAreaRulesSubsystem>() : nullptr;
	const UAreaDefinition* Area = Rules ? Rules->GetArea() : nullptr;
	AreaName = Area && !Area->DisplayName.IsEmpty() ? Area->DisplayName : FText::FromString(USessionSubsystem::AreaName(Map));
	if (AreaTitle)
	{
		AreaTitle->SetText(AreaName.ToUpper());
	}

	// The minimap's bake is the map: asking for it starts one if none has (its square is known as it starts; the picture
	// follows a few frames later, and the page picks it up as it ticks).
	UMinimapSubsystem* Minimap = World ? World->GetSubsystem<UMinimapSubsystem>() : nullptr;
	UTexture2D* Picture = Minimap ? Minimap->GetMapTexture() : nullptr;
	MapBounds = Minimap ? Minimap->GetMapBounds() : FBox2D(ForceInit);
	MapTexture = Picture;
	MapBrush = FSlateBrush();
	MapBrush.SetResourceObject(Picture);
	if (Picture)
	{
		MapBrush.ImageSize = FVector2D(Picture->GetSizeX(), Picture->GetSizeY());
	}
	if (MapImage)
	{
		MapImage->SetBrush(MapBrush);
	}
}

void UMapWidget::ResetView()
{
	View = FMapView();
	View.Frame = MapLayout::MapSize;
	// The playable area frames the first view: the land past its closed edges is dimmed scenery.
	PlayableUV = FBox2D(ForceInit);
	if (const APlayableArea* Playable = MapBounds.bIsValid ? APlayableArea::Find(GetWorld()) : nullptr)
	{
		for (const FVector2D& Corner : Playable->GetBoundary().GetCorners())
		{
			PlayableUV += WorldToUV(FVector(Corner.X, Corner.Y, 0.0));
		}
	}
	if (PlayableUV.bIsValid)
	{
		View.Fit(PlayableUV, 0.04);
	}
	View.Clamp();
	bEasing = false;
}

void UMapWidget::RefreshPins()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		Pins.Reset();
		return;
	}
	FMapPinSources Sources;
	Sources.World = World;
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	Sources.Runner = Runner;
	Sources.Campaign = Runner ? &Runner->GetCampaign() : nullptr;
	Sources.Missions = World->GetSubsystem<UMissionSubsystem>();
	if (UMapDiscoverySubsystem* Discovery = World->GetSubsystem<UMapDiscoverySubsystem>())
	{
		Discovery->LookAround();
		Sources.FoundChests = &Discovery->GetFoundChests();
	}
	Pins = MapPins::Gather(Sources);
	if (!Pins.IsValidIndex(Hovered))
	{
		Hovered = INDEX_NONE;
	}

	// No bake to map by (a level without ground tagged, a test level): a square round the pins and the player instead.
	if (!MapBounds.bIsValid)
	{
		FBox2D Around(ForceInit);
		for (const FMapPin& Pin : Pins)
		{
			Around += FVector2D(Pin.Location.X, Pin.Location.Y);
		}
		if (const APawn* Player = GetOwningPlayerPawn())
		{
			Around += FVector2D(Player->GetActorLocation().X, Player->GetActorLocation().Y);
		}
		const FVector2D Middle = Around.bIsValid ? Around.GetCenter() : FVector2D::ZeroVector;
		const double Half = FMath::Max(Around.bIsValid ? Around.GetExtent().GetMax() * 1.25 : 0.0, 5000.0);
		MapBounds = FBox2D(Middle - FVector2D(Half), Middle + FVector2D(Half));
	}
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UMapWidget::RebuildWidget()
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
		UCanvasPanel* Page = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Page"));
		Scale->SetContent(MakeSized(WidgetTree, Page, PageSize.X, PageSize.Y));

		// Title tabs: the loadout, the bestiary, the missions and this page.
		TArray<ULooterButton*> Tabs;
		PlaceOn(*Page, MakePageTabs(WidgetTree, static_cast<int32>(EInventoryPage::Map), Tabs), PageTabsPosition, FVector2D::ZeroVector,
			FVector2D(0.5f, 0.f));
		for (ULooterButton* Tab : Tabs)
		{
			Tab->OnButtonClicked.BindUObject(this, &UMapWidget::HandleTabClicked);
		}

		BuildMapFrame(*Page);
		BuildSideColumn(*Page);

		// Bottom: what the keys do.
		PromptBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		PlaceOn(*Page, PromptBar, FVector2D(800.f, 852.f), FVector2D::ZeroVector, FVector2D(0.5f, 0.f));

		// Opened before it was first shown: the contents can go in now.
		if (AreaTitle)
		{
			AreaTitle->SetText(AreaName.ToUpper());
		}
		RefreshLegend();
		RebuildGraveList();
		RefreshTravelCard();
		RefreshPrompts();
		LayoutMap(0.f);
	}
	return Super::RebuildWidget();
}

void UMapWidget::BuildMapFrame(UCanvasPanel& Page)
{
	using namespace MapLayout;

	// The frame: the kit's dark glass in a chamfered card with a cyan line, the map clipped inside it.
	UOverlay* Inside = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	UImage* FrameFill = nullptr;
	UImage* FrameLine = nullptr;
	UOverlay* Frame = MakeCard(WidgetTree, Inside, FMargin(FrameInset), 2.f, FrameFill, FrameLine);
	FrameFill->SetColorAndOpacity(Color::ScreenBg());
	FrameLine->SetColorAndOpacity(Color::ScreenLine());
	PlaceOn(Page, Frame, FramePosition, FrameSize);

	MapCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MapCanvas"));
	MapCanvas->SetClipping(EWidgetClipping::ClipToBounds);
	FillOverlaySlot(Inside->AddChildToOverlay(MapCanvas));
	UCanvasPanel& Map = *MapCanvas;

	// Bottom to top: the picture, the grid and the route to the chosen grave, the places' names, the chosen grave's glow,
	// the pins, the player, the pointed-at pin's card, the scale's words and the bake's wait.
	MapImage = MakeImage(WidgetTree, MapBrush);
	MapImage->SetVisibility(ESlateVisibility::Hidden);
	PlaceOn(Map, MapImage, FVector2D::ZeroVector, MapSize);

	MapPaint = WidgetTree->ConstructWidget<ULoadoutPaintLayer>(ULoadoutPaintLayer::StaticClass());
	MapPaint->SetVisibility(ESlateVisibility::HitTestInvisible);
	MapPaint->SetPainter([this](const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId)
	{
		PaintMap(Geometry, Elements, LayerId);
	});
	UCanvasPanelSlot* PaintSlot = Map.AddChildToCanvas(MapPaint);
	PaintSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	PaintSlot->SetOffsets(FMargin(0.f));

	PlaceLabels.Reset();
	for (const FMapPlace& Place : Places)
	{
		UTextBlock* Name = MakeFloatingText(WidgetTree, Place.bMajor ? 11 : 8, Place.bMajor ? Color::Title() : Color::TextDim(),
			Place.bMajor ? 260 : 160, ETextJustify::Center);
		Name->SetText(FText::FromString(FString(Place.Name).ToUpper()));
		Name->SetVisibility(ESlateVisibility::Hidden);
		PlaceOn(Map, Name, FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D(0.5f, 0.5f));
		PlaceLabels.Add(Name);
	}

	SelectGlow = MakeImage(WidgetTree, GlowBrush(FVector2D(92.f), Color::Accent() * FLinearColor(1.f, 1.f, 1.f, 0.5f)));
	SelectGlow->SetVisibility(ESlateVisibility::Hidden);
	PlaceOn(Map, SelectGlow, FVector2D::ZeroVector, FVector2D(92.f), FVector2D(0.5f, 0.5f));
	SelectRing = MakeImage(WidgetTree, CircleBrush(FLinearColor::Transparent, Color::Accent(), 2.f));
	SelectRing->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	SelectRing->SetVisibility(ESlateVisibility::Hidden);
	PlaceOn(Map, SelectRing, FVector2D::ZeroVector, FVector2D(40.f), FVector2D(0.5f, 0.5f));

	PinSlots.Reset();
	for (int32 Index = 0; Index < MaxPins; ++Index)
	{
		FPinSlot& Pin = PinSlots.AddDefaulted_GetRef();
		Pin.Badge = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Pin.Badge->SetVisibility(ESlateVisibility::Hidden);
		PlaceOn(Map, Pin.Badge, FVector2D::ZeroVector, FVector2D(20.f), FVector2D(0.5f, 0.5f));
		Pin.Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Pin.Icon->SetVisibility(ESlateVisibility::Hidden);
		PlaceOn(Map, Pin.Icon, FVector2D::ZeroVector, FVector2D(14.f), FVector2D(0.5f, 0.5f));
	}

	PlayerGlow = MakeImage(WidgetTree, GlowBrush(FVector2D(56.f), Color::Title() * FLinearColor(1.f, 1.f, 1.f, 0.35f)));
	PlayerGlow->SetVisibility(ESlateVisibility::Hidden);
	PlaceOn(Map, PlayerGlow, FVector2D::ZeroVector, FVector2D(56.f), FVector2D(0.5f, 0.5f));
	PlayerArrow = MakeImage(WidgetTree, MarkerBrush(EMarker::Arrow, Color::Text()));
	PlayerArrow->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	PlayerArrow->SetVisibility(ESlateVisibility::Hidden);
	PlaceOn(Map, PlayerArrow, FVector2D::ZeroVector, FVector2D(26.f), FVector2D(0.5f, 0.5f));

	// The pointed-at pin's name on a small plate beside it.
	UVerticalBox* HoverWords = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	HoverName = Label(WidgetTree, TEXT(""), 10, Color::Text(), 120);
	HoverWords->AddChildToVerticalBox(HoverName);
	HoverDetail = MakeText(WidgetTree, TEXT(""), 8, Color::TextDim());
	HoverWords->AddChildToVerticalBox(HoverDetail)->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
	UBorder* Plate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Plate->SetBrush(RectBrush(Colors::InspectFill(), Colors::CardLine(), 1.f));
	MarkBackground(Plate);
	Plate->SetPadding(FMargin(10.f, 6.f));
	Plate->SetContent(HoverWords);
	Plate->SetVisibility(ESlateVisibility::Hidden);
	HoverCard = Plate;
	PlaceOn(Map, Plate, FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D(0.f, 0.5f));

	ScaleText = MakeFloatingText(WidgetTree, 8, Color::Text(), 160, ETextJustify::Left);
	PlaceOn(Map, ScaleText, FVector2D(22.f, MapSize.Y - 26.f), FVector2D::ZeroVector, FVector2D(0.f, 1.f));
	LoadingText = MakeFloatingText(WidgetTree, 11, Color::TextDim(), 300, ETextJustify::Center);
	LoadingText->SetText(FText::FromString(TEXT("CHARTING THE MAP...")));
	PlaceOn(Map, LoadingText, MapSize * 0.5, FVector2D::ZeroVector, FVector2D(0.5f, 0.5f));

	// The frame's own marks over the map: the area's name, the compass's N, the zoom, and the kit's orange brackets.
	AreaTitle = MakeFloatingText(WidgetTree, 15, Color::Title(), 350, ETextJustify::Left);
	PlaceOn(Page, AreaTitle, FramePosition + FVector2D(26.0, 20.0), FVector2D::ZeroVector);
	UTextBlock* NorthUp = MakeFloatingText(WidgetTree, 8, Color::TextDim(), 220, ETextJustify::Left);
	NorthUp->SetText(FText::FromString(TEXT("MAP  ·  NORTH UP")));
	PlaceOn(Page, NorthUp, FramePosition + FVector2D(27.0, 44.0), FVector2D::ZeroVector);

	const FVector2D Compass = FramePosition + FVector2D(FrameSize.X - 44.0, 26.0);
	PlaceOn(Page, MakeImage(WidgetTree, MarkerBrush(EMarker::Arrow, Color::Accent())), Compass, FVector2D(16.f), FVector2D(0.5f, 0.5f));
	UTextBlock* North = MakeFloatingText(WidgetTree, 12, Color::Accent(), 0, ETextJustify::Center);
	North->SetText(FText::FromString(TEXT("N")));
	PlaceOn(Page, North, Compass + FVector2D(0.0, 20.0), FVector2D::ZeroVector, FVector2D(0.5f, 0.5f));

	ZoomText = MakeFloatingText(WidgetTree, 8, Color::TextDim(), 200, ETextJustify::Right);
	PlaceOn(Page, ZoomText, FramePosition + FrameSize - FVector2D(24.0, 22.0), FVector2D::ZeroVector, FVector2D(1.f, 1.f));

	// The brackets hang just outside the frame's corners, the right ones mirrored, the bottom ones flipped.
	const FVector2D BracketSize(32.0, 12.0);
	const struct { FVector2D At; FVector2D Flip; } Corners[] = {
		{ FramePosition + FVector2D(-4.0, -4.0), FVector2D(1.0, 1.0) },
		{ FramePosition + FVector2D(FrameSize.X - BracketSize.X + 4.0, -4.0), FVector2D(-1.0, 1.0) },
		{ FramePosition + FVector2D(-4.0, FrameSize.Y - BracketSize.Y + 4.0), FVector2D(1.0, -1.0) },
		{ FramePosition + FrameSize - BracketSize + FVector2D(4.0), FVector2D(-1.0, -1.0) },
	};
	for (const auto& Corner : Corners)
	{
		UImage* Bracket = MakeImage(WidgetTree, ShapeBrush(EShape::Bracket, false, Color::Accent()));
		Bracket->SetRenderScale(Corner.Flip);
		PlaceOn(Page, Bracket, Corner.At, BracketSize);
	}
}

void UMapWidget::BuildSideColumn(UCanvasPanel& Page)
{
	using namespace MapLayout;
	UVerticalBox* Side = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	PlaceOn(Page, Side, FVector2D(SideX, FramePosition.Y), FVector2D(SideWidth, FrameSize.Y));

	// The legend: each kind of pin as the map draws it, and how many there are.
	Side->AddChildToVerticalBox(Label(WidgetTree, TEXT("Legend"), 10, Color::Accent(), 300));
	UVerticalBox* Rows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Rows->AddChildToVerticalBox(MakeYouLegendRow());
	LegendCounts.Init(nullptr, static_cast<int32>(EMapPinKind::Objective) + 1);
	for (const EMapPinKind Kind : LegendKinds)
	{
		UTextBlock* Count = nullptr;
		Rows->AddChildToVerticalBox(MakeLegendRow(Kind, Count))->SetPadding(FMargin(0.f, 5.f, 0.f, 0.f));
		LegendCounts[static_cast<int32>(Kind)] = Count;
	}
	UImage* LegendFill = nullptr;
	UImage* LegendLine = nullptr;
	UOverlay* Legend = MakeCard(WidgetTree, Rows, FMargin(16.f, 10.f), 1.7f, LegendFill, LegendLine);
	LegendFill->SetColorAndOpacity(Colors::CardFill());
	LegendLine->SetColorAndOpacity(Colors::CardLine());
	Side->AddChildToVerticalBox(Legend)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));

	// Fast travel: the chosen grave, how far, and whether the player can go.
	Side->AddChildToVerticalBox(Label(WidgetTree, TEXT("Fast travel"), 10, Color::Accent(), 300))->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
	UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	CardName = FittedLabel(WidgetTree, TEXT(""), 15, Color::Title(), 120);
	Card->AddChildToVerticalBox(CardName);
	CardDetail = MakeText(WidgetTree, TEXT(""), 9, Color::TextDim());
	CardDetail->SetAutoWrapText(true);
	Card->AddChildToVerticalBox(CardDetail)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	CardStatus = MakeText(WidgetTree, TEXT(""), 9, Color::Text());
	CardStatus->SetAutoWrapText(true);
	Card->AddChildToVerticalBox(CardStatus)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	TravelButton = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	TravelButton->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), ActionTravel, 0, FText::FromString(TEXT("Travel")), 11,
		EButtonKind::Primary);
	// Its sounds are the page's: the travel's whoosh, or a refusal.
	TravelButton->bPlaysSounds = false;
	TravelButton->OnButtonClicked.BindUObject(this, &UMapWidget::HandleTravelClicked);
	UVerticalBoxSlot* TravelSlot = Card->AddChildToVerticalBox(MakeSized(WidgetTree, TravelButton, 180.f, 34.f));
	TravelSlot->SetHorizontalAlignment(HAlign_Left);
	TravelSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	UImage* CardFill = nullptr;
	UImage* CardLine = nullptr;
	UOverlay* TravelCard = MakeCard(WidgetTree, Card, FMargin(18.f, 14.f), 1.7f, CardFill, CardLine);
	CardFill->SetColorAndOpacity(Colors::CardFill());
	CardLine->SetColorAndOpacity(Colors::CardLine());
	Side->AddChildToVerticalBox(TravelCard)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));

	// The open graves as a list: a grave far off the view is a click away.
	GraveListHeader = Label(WidgetTree, TEXT(""), 8, Color::TextDim(), 220);
	Side->AddChildToVerticalBox(GraveListHeader)->SetPadding(FMargin(2.f, 16.f, 0.f, 0.f));
	UScrollBox* GraveScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	StyleScrollBox(GraveScroll);
	GraveList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	GraveScroll->AddChild(GraveList);
	UVerticalBoxSlot* ListSlot = Side->AddChildToVerticalBox(GraveScroll);
	ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ListSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
}

UWidget* UMapWidget::MakeLegendRow(EMapPinKind Kind, UTextBlock*& OutCount)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	const FLinearColor Tint = MapIcons::Color(Kind);
	UOverlay* Badge = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	if (MapIcons::HasBadge(Kind))
	{
		FillOverlaySlot(Badge->AddChildToOverlay(MakeImage(WidgetTree, MapIcons::BadgeBrush(Tint, false))));
	}
	const float IconSize = MapIcons::HasBadge(Kind) ? LegendBadge * MapIcons::IconShare : LegendBadge * 0.8f;
	UOverlaySlot* IconSlot = Badge->AddChildToOverlay(MakeImage(WidgetTree, MapIcons::IconBrush(Kind, IconSize, Tint)));
	IconSlot->SetHorizontalAlignment(HAlign_Center);
	IconSlot->SetVerticalAlignment(VAlign_Center);
	Row->AddChildToHorizontalBox(MakeSized(WidgetTree, Badge, LegendBadge, LegendBadge))->SetVerticalAlignment(VAlign_Center);

	UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(Label(WidgetTree, MapPins::KindName(Kind).ToString(), 9, Color::Text(), 140));
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);
	NameSlot->SetPadding(FMargin(12.f, 0.f, 8.f, 0.f));
	OutCount = Label(WidgetTree, TEXT("0"), 10, Color::CyanText(), 60);
	Row->AddChildToHorizontalBox(OutCount)->SetVerticalAlignment(VAlign_Center);
	return Row;
}

UWidget* UMapWidget::MakeYouLegendRow()
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UOverlay* Badge = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	UOverlaySlot* ArrowSlot = Badge->AddChildToOverlay(MakeSized(WidgetTree, MakeImage(WidgetTree, MarkerBrush(EMarker::Arrow, Color::Text())), 18.f, 18.f));
	ArrowSlot->SetHorizontalAlignment(HAlign_Center);
	ArrowSlot->SetVerticalAlignment(VAlign_Center);
	Row->AddChildToHorizontalBox(MakeSized(WidgetTree, Badge, LegendBadge, LegendBadge))->SetVerticalAlignment(VAlign_Center);
	UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(Label(WidgetTree, TEXT("You"), 9, Color::Text(), 140));
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);
	NameSlot->SetPadding(FMargin(12.f, 0.f, 8.f, 0.f));
	return Row;
}

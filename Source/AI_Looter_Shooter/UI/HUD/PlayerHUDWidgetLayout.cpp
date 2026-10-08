#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/HUD/HudFrameRateWidget.h"
#include "UI/HUD/HudInteractPromptWidget.h"
#include "UI/HUD/HudLevelUpBannerWidget.h"
#include "UI/HUD/HudMagazineWidget.h"
#include "UI/HUD/HudMinimapWidget.h"
#include "UI/HUD/HudPickupFeedWidget.h"
#include "UI/HUD/HudPlayerFrameWidget.h"
#include "UI/HUD/HudScreenEdgeWidget.h"
#include "UI/HUD/HudWeaponSlotsWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
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
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

// UPlayerHUDWidget's layout as RebuildWidget builds it: the screen edges, the corner clusters, the crosshair and hit
// marker, the level-up banner, the loot card, the interaction prompt and the feeds. Running it frame by frame is in
// PlayerHUDWidget.cpp.

using namespace LooterUI;

namespace
{
	constexpr int32 NumCompareStats = 9;
	constexpr int32 PickupHoldSegmentCount = 16;
	/** Parallelogram slant of the bars, in degrees (mirrored left/right, like the reference). */
	constexpr float BarSlant = 16.f;
	/**
	 * How far left of the screen's middle the pickup feed's lines end: past half the widest crosshair (80 px) with room
	 * to spare, so a line never covers the crosshair or the hit marker.
	 */
	constexpr float PickupFeedGap = 76.f;

	// The weapon cluster, in 1080p pixels. It keeps its corner 48 px in from the right edge; its last line (the gun's
	// name) ends 32 px above the bottom, which puts the slots' circles at y 812, 888 and 964 (x 1775), the cartridge at
	// (1821, 800) with its base level with the last slot's bottom, and the fire mode's line at y 1000.
	constexpr float ClusterRight = 48.f;
	constexpr float ClusterBottom = 32.f;
	/** The cartridge stands this far right of the slots' column. */
	constexpr float CartridgeGap = 14.f;
	/** The two lines under them: the status and fire mode, then the gun's name, each this far below the last. */
	constexpr float LineGap = 4.f;
	constexpr float ModeLineHeight = 20.f;
	constexpr float NameLineHeight = 24.f;
	/** The status sits this far left of the fire mode, on its line. */
	constexpr float StatusGap = 12.f;
	/** A long name shrinks to fit this width, so it never reaches across the screen. */
	constexpr float NameMaxWidth = 300.f;
	/** The rarity gem after the name: a diamond, tip to tip, over its ink edge (1.6 px wider each side), and the gap before it
	 *  (6 px: the user asked for it 3 px further from the name, 2026-10-08). */
	constexpr float GemSize = 14.f;
	constexpr float GemEdgeSize = 18.5f;
	constexpr float GemGap = 6.f;

	/** Where the level-up banner's gem centres, down from the top's middle: the mockup puts the banner's top at 170 and its
	 *  180 px gem box at the top of it, which keeps the gem clear of the boss bar. */
	constexpr float BannerTop = 260.f;

	UCanvasPanelSlot* PlaceOnCanvas(UCanvasPanel* Canvas, UWidget* Widget, const FAnchors& Anchors, const FVector2D& Alignment, const FVector2D& Position)
	{
		UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Widget);
		CanvasSlot->SetAnchors(Anchors);
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetPosition(Position);
		CanvasSlot->SetAutoSize(true);
		return CanvasSlot;
	}

	/** A white rectangle, tinted per use (segments, pips). */
	UImage* TintableRect(UWidgetTree* Tree)
	{
		return MakeImage(Tree, RectBrush(FLinearColor::White));
	}

	/** Slim segmented bar, sheared into a parallelogram, on a faint dark backing. */
	UWidget* MakeSlantBar(UWidgetTree* Tree, int32 Count, float Width, float Height, float Shear, TArray<TObjectPtr<UImage>>& OutSegments,
		float Gap = 2.f)
	{
		UBorder* Backing = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Backing->SetBrush(RectBrush(FLinearColor(0.f, 0.02f, 0.04f, 0.45f)));
		MarkBackground(Backing);
		Backing->SetPadding(FMargin(2.f));
		UHorizontalBox* Bar = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		for (int32 Index = 0; Index < Count; ++Index)
		{
			UImage* Segment = TintableRect(Tree);
			Segment->SetColorAndOpacity(Color::SegmentOff());
			UHorizontalBoxSlot* SegmentSlot = Bar->AddChildToHorizontalBox(Segment);
			SegmentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			SegmentSlot->SetPadding(FMargin(0.f, 0.f, Index + 1 < Count ? Gap : 0.f, 0.f));
			OutSegments.Add(Segment);
		}
		Backing->SetContent(Bar);
		USizeBox* Size = MakeSized(Tree, Backing, Width, Height);
		Size->SetRenderShear(FVector2D(Shear, 0.f));
		return Size;
	}

	/** Four thin ticks around a gap (crosshair, and rotated 45 degrees for the hit marker). */
	UOverlay* MakeTicks(UWidgetTree* Tree, float Length, float Thickness, TArray<TObjectPtr<UImage>>* OutTicks = nullptr)
	{
		UOverlay* Overlay = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		struct FTick { EHorizontalAlignment H; EVerticalAlignment V; bool bVertical; };
		const FTick Ticks[] = {
			{ HAlign_Center, VAlign_Top, true }, { HAlign_Center, VAlign_Bottom, true },
			{ HAlign_Left, VAlign_Center, false }, { HAlign_Right, VAlign_Center, false } };
		for (const FTick& Tick : Ticks)
		{
			UImage* Image = MakeImage(Tree, RectBrush(FLinearColor::White, Color::Outline(), 1.f));
			Image->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.9f));
			UWidget* Box = MakeSized(Tree, Image, Tick.bVertical ? Thickness : Length, Tick.bVertical ? Length : Thickness);
			UOverlaySlot* TickSlot = Overlay->AddChildToOverlay(Box);
			TickSlot->SetHorizontalAlignment(Tick.H);
			TickSlot->SetVerticalAlignment(Tick.V);
			if (OutTicks)
			{
				OutTicks->Add(Image);
			}
		}
		return Overlay;
	}

	/** A diamond filling its box, white: the rarity gem and its ink edge, tinted per use. */
	const FVectorIcon& GemIcon()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Gem;
			Gem.ViewBox = FVector2D(24.f, 24.f);
			Gem.Fills.Add({ FVector2D(12.f, 0.5f), FVector2D(23.5f, 12.f), FVector2D(12.f, 23.5f), FVector2D(0.5f, 12.f) });
			return Gem;
		}();
		return Icon;
	}
}

TSharedRef<SWidget> UPlayerHUDWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Root;

		const FAnchors Center(0.5f, 0.5f);

		// Behind everything: the screen edges' red flash on a hit and pulse at low health, over the whole screen. It reads
		// the player's health itself.
		{
			UHudScreenEdgeWidget* ScreenEdges = WidgetTree->ConstructWidget<UHudScreenEdgeWidget>(UHudScreenEdgeWidget::StaticClass());
			UCanvasPanelSlot* EdgeSlot = Root->AddChildToCanvas(ScreenEdges);
			EdgeSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			EdgeSlot->SetOffsets(FMargin(0.f));
			EdgeSlot->SetAutoSize(false);
		}

		// Crosshair: four thin ticks with a center dot; the gap follows the weapon's spread, and each shot kicks it out.
		{
			// 4px ticks with a 1px dark edge leave a 2px white core that reads on any background.
			UOverlay* Ticks = MakeTicks(WidgetTree, 9.f, 4.f);
			UWidget* Dot = MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(FLinearColor(1.f, 1.f, 1.f, 0.8f))), 3.f, 3.f);
			UOverlaySlot* DotSlot = Ticks->AddChildToOverlay(Dot);
			DotSlot->SetHorizontalAlignment(HAlign_Center);
			DotSlot->SetVerticalAlignment(VAlign_Center);

			CrosshairBox = MakeSized(WidgetTree, Ticks, 20.f, 20.f);
			// The kick grows it about its middle.
			CrosshairBox->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			PlaceOnCanvas(Root, CrosshairBox, Center, FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
		}

		// Hit marker: the same ticks turned 45 degrees into an X.
		{
			USizeBox* MarkerBox = MakeSized(WidgetTree, MakeTicks(WidgetTree, 11.f, 4.f, &HitMarkerTicks), 34.f, 34.f);
			MarkerBox->SetRenderTransformAngle(45.f);
			MarkerBox->SetVisibility(ESlateVisibility::Hidden);
			HitMarker = MarkerBox;
			PlaceOnCanvas(Root, MarkerBox, Center, FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
		}

		// Message plate under the crosshair: the weapon manager's messages (level-ups have the banner).
		MessageText = MakeText(WidgetTree, TEXT(""), 15, Color::Accent(), true, 150);
		MessagePlate = MakePlate(WidgetTree, MessageText, FMargin(18.f, 8.f));
		MessagePlate->SetVisibility(ESlateVisibility::Hidden);
		PlaceOnCanvas(Root, MessagePlate, FAnchors(0.5f, 0.72f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);

		// Bottom-left: the player frame. Its own box holds its pieces measured from the screen's bottom-left corner. When
		// its experience bar reaches a new level, the banner shows it.
		PlayerFrame = WidgetTree->ConstructWidget<UHudPlayerFrameWidget>(UHudPlayerFrameWidget::StaticClass());
		PlayerFrame->OnLevelUp.BindUObject(this, &UPlayerHUDWidget::HandleLevelUp);
		PlaceOnCanvas(Root, PlayerFrame, FAnchors(0.f, 1.f), FVector2D(0.f, 1.f), FVector2D::ZeroVector);

		// Bottom-right: the weapon slots in a column with the cartridge standing on their right, its base level with the
		// last slot's; under them the status and fire mode, and under those the gun's name ending in its rarity gem.
		{
			UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			auto AddRight = [Box](UWidget* Child, float Top)
			{
				UVerticalBoxSlot* ChildSlot = Box->AddChildToVerticalBox(Child);
				ChildSlot->SetHorizontalAlignment(HAlign_Right);
				ChildSlot->SetPadding(FMargin(0.f, Top, 0.f, 0.f));
			};

			UHorizontalBox* Arms = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			WeaponSlots = WidgetTree->ConstructWidget<UHudWeaponSlotsWidget>(UHudWeaponSlotsWidget::StaticClass());
			Arms->AddChildToHorizontalBox(WeaponSlots)->SetVerticalAlignment(VAlign_Bottom);
			MagazineGauge = WidgetTree->ConstructWidget<UHudMagazineWidget>(UHudMagazineWidget::StaticClass());
			UHorizontalBoxSlot* CartridgeSlot = Arms->AddChildToHorizontalBox(MagazineGauge);
			CartridgeSlot->SetVerticalAlignment(VAlign_Bottom);
			CartridgeSlot->SetPadding(FMargin(CartridgeGap, 0.f, 0.f, 0.f));
			AddRight(Arms, 0.f);

			// The status (orange; "RELOADING", "[R] RELOAD", "NO AMMO") left of the fire mode, right-aligned.
			UHorizontalBox* ModeRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			StatusText = MakeFloatingText(WidgetTree, 12, Color::Accent(), 200, ETextJustify::Right);
			UHorizontalBoxSlot* StatusSlot = ModeRow->AddChildToHorizontalBox(StatusText);
			StatusSlot->SetVerticalAlignment(VAlign_Center);
			StatusSlot->SetPadding(FMargin(0.f, 0.f, StatusGap, 0.f));
			FireModeText = MakeFloatingText(WidgetTree, 13, Color::TextDim(), 150, ETextJustify::Right);
			ModeRow->AddChildToHorizontalBox(FireModeText)->SetVerticalAlignment(VAlign_Center);
			AddRight(MakeSized(WidgetTree, ModeRow, 0.f, ModeLineHeight), LineGap);

			// The gun's name in its rarity's color, shrinking to fit when it's long, then the rarity gem at the edge.
			UHorizontalBox* NameRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			WeaponName = MakeFloatingText(WidgetTree, 18, Color::Text(), 60, ETextJustify::Right);
			UScaleBox* NameFit = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
			NameFit->SetStretch(EStretch::ScaleToFit);
			NameFit->SetStretchDirection(EStretchDirection::DownOnly);
			NameFit->SetContent(WeaponName);
			USizeBox* NameBox = MakeSized(WidgetTree, NameFit, 0.f);
			NameBox->SetMaxDesiredWidth(NameMaxWidth);
			NameRow->AddChildToHorizontalBox(NameBox)->SetVerticalAlignment(VAlign_Center);
			UOverlay* Gem = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			UOverlaySlot* GemEdgeSlot = Gem->AddChildToOverlay(MakeImage(WidgetTree,
				IconBrush(TEXT("HudRarityGem"), GemIcon(), 2.f, FVector2D(GemEdgeSize, GemEdgeSize), Color::Ink())));
			GemEdgeSlot->SetHorizontalAlignment(HAlign_Center);
			GemEdgeSlot->SetVerticalAlignment(VAlign_Center);
			RarityGem = MakeImage(WidgetTree, IconBrush(TEXT("HudRarityGem"), GemIcon(), 2.f, FVector2D(GemSize, GemSize), FLinearColor::White));
			UOverlaySlot* GemSlot = Gem->AddChildToOverlay(RarityGem);
			GemSlot->SetHorizontalAlignment(HAlign_Center);
			GemSlot->SetVerticalAlignment(VAlign_Center);
			UHorizontalBoxSlot* GemRowSlot = NameRow->AddChildToHorizontalBox(Gem);
			GemRowSlot->SetVerticalAlignment(VAlign_Center);
			GemRowSlot->SetPadding(FMargin(GemGap, 0.f, 0.f, 0.f));
			AddRight(MakeSized(WidgetTree, NameRow, 0.f, NameLineHeight), LineGap);

			WeaponCluster = Box;
			PlaceOnCanvas(Root, Box, FAnchors(1.f, 1.f), FVector2D(1.f, 1.f), FVector2D(-ClusterRight, -ClusterBottom));
		}

		// Right-middle: comparison card for the loot you're looking at.
		{
			UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			Box->AddChildToVerticalBox(MakeSection(WidgetTree, TEXT("Loot")));
			PickupName = MakeText(WidgetTree, TEXT(""), 17, Color::Text(), false, 80);
			Box->AddChildToVerticalBox(PickupName)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
			// A named gun's line under its name (UpdatePickupCard collapses it for any other gun).
			PickupFlavor = MakeText(WidgetTree, TEXT(""), 12, LooterWeaponText::FlavorColor());
			PickupFlavor->SetFont(LooterWeaponText::FlavorFont(12));
			PickupFlavor->SetVisibility(ESlateVisibility::Collapsed);
			Box->AddChildToVerticalBox(PickupFlavor)->SetPadding(FMargin(0.f, 1.f, 0.f, 0.f));
			PickupLevel = MakeText(WidgetTree, TEXT(""), 11, Color::TextDim(), false, 120);
			Box->AddChildToVerticalBox(PickupLevel)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
			for (int32 Stat = 0; Stat < NumCompareStats; ++Stat)
			{
				UTextBlock* StatText = MakeText(WidgetTree, TEXT(""), 13, Color::Text());
				PickupStatTexts.Add(StatText);
				Box->AddChildToVerticalBox(StatText)->SetPadding(FMargin(0.f, 1.f));
			}
			PickupHint = MakeText(WidgetTree, TEXT(""), 13, Color::Accent(), false, 150);
			Box->AddChildToVerticalBox(PickupHint)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
			// Fills while the interact key is held to equip; hidden (keeping its space, so the card doesn't jump) otherwise.
			PickupHoldBar = MakeSlantBar(WidgetTree, PickupHoldSegmentCount, 200.f, 6.f, BarSlant, PickupHoldSegments);
			PickupHoldBar->SetVisibility(ESlateVisibility::Hidden);
			UVerticalBoxSlot* HoldSlot = Box->AddChildToVerticalBox(PickupHoldBar);
			HoldSlot->SetHorizontalAlignment(HAlign_Left);
			HoldSlot->SetPadding(FMargin(4.f, 6.f, 0.f, 0.f));

			USizeBox* CardSize = MakeSized(WidgetTree, Box, 0.f);
			CardSize->SetMinDesiredWidth(320.f);
			PickupCard = MakePlate(WidgetTree, CardSize, FMargin(16.f, 12.f));
			PickupCard->SetVisibility(ESlateVisibility::Collapsed);
			PlaceOnCanvas(Root, PickupCard, FAnchors(1.f, 0.5f), FVector2D(1.f, 0.5f), FVector2D(-32.f, 0.f));
		}

		// Under the crosshair: what the Interact key does to the thing looked at (loot has the card above instead).
		InteractPrompt = WidgetTree->ConstructWidget<UHudInteractPromptWidget>(UHudInteractPromptWidget::StaticClass());
		PlaceOnCanvas(Root, InteractPrompt, Center, FVector2D(0.5f, 0.f), FVector2D(0.f, UHudInteractPromptWidget::Gap));

		// Top-right: the minimap.
		UHudMinimapWidget* Minimap = WidgetTree->ConstructWidget<UHudMinimapWidget>(UHudMinimapWidget::StaticClass());
		PlaceOnCanvas(Root, Minimap, FAnchors(1.f, 0.f), FVector2D(1.f, 0.f), FVector2D(-UHudMinimapWidget::Margin, UHudMinimapWidget::Margin));

		// Top-left: the frame rate, the same distance in from the corner as the minimap.
		UHudFrameRateWidget* FrameRate = WidgetTree->ConstructWidget<UHudFrameRateWidget>(UHudFrameRateWidget::StaticClass());
		PlaceOnCanvas(Root, FrameRate, FAnchors(0.f, 0.f), FVector2D(0.f, 0.f), FVector2D(UHudMinimapWidget::Margin, UHudMinimapWidget::Margin));

		// Left of the crosshair: what was just picked up, where the eyes already are. The feed's lines end at its right
		// edge, the newest level with the crosshair and the older ones rising above it.
		UHudPickupFeedWidget* PickupFeed = WidgetTree->ConstructWidget<UHudPickupFeedWidget>(UHudPickupFeedWidget::StaticClass());
		UCanvasPanelSlot* FeedSlot = PlaceOnCanvas(Root, PickupFeed, Center, FVector2D(1.f, 1.f),
			FVector2D(-PickupFeedGap, UHudPickupFeedWidget::LineSpacing * 0.5f));
		FeedSlot->SetAutoSize(false);
		FeedSlot->SetSize(FVector2D(UHudPickupFeedWidget::Width, UHudPickupFeedWidget::Height));

		// Top-centre: the level-up banner, its gem centred 260 px down. It shows nothing until a level-up. (The bottom
		// centre, where the experience bar was, stays empty: the player frame has it.)
		LevelUpBanner = WidgetTree->ConstructWidget<UHudLevelUpBannerWidget>(UHudLevelUpBannerWidget::StaticClass());
		PlaceOnCanvas(Root, LevelUpBanner, FAnchors(0.5f, 0.f), FVector2D(0.5f, 0.5f), FVector2D(0.f, BannerTop));
	}
	return Super::RebuildWidget();
}

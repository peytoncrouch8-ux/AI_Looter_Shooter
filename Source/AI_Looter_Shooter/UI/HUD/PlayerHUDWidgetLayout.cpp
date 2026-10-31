#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/HUD/HudCrosshairRules.h"
#include "UI/HUD/HudCrosshairWidget.h"
#include "UI/HUD/HudDamageIndicatorWidget.h"
#include "UI/HUD/HudEmberSocketWidget.h"
#include "UI/HUD/HudFrameRateWidget.h"
#include "UI/HUD/HudFindBannerWidget.h"
#include "UI/HUD/HudFlaskWidget.h"
#include "UI/HUD/HudGraveMarkedWidget.h"
#include "UI/HUD/HudGrenadeWidget.h"
#include "UI/HUD/HudInteractPromptWidget.h"
#include "UI/HUD/HudLevelUpBannerWidget.h"
#include "UI/HUD/HudMagazineWidget.h"
#include "UI/HUD/HudMinimapWidget.h"
#include "UI/HUD/HudMissionCompleteWidget.h"
#include "UI/HUD/HudNotchTallyWidget.h"
#include "UI/HUD/HudPickupFeedWidget.h"
#include "UI/HUD/HudPlayerFrameWidget.h"
#include "UI/HUD/HudScreenEdgeWidget.h"
#include "UI/HUD/HudWeaponSlotsWidget.h"
#include "UI/Cards/GunCardWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

// UPlayerHUDWidget's layout as RebuildWidget builds it: the screen edges, the corner clusters, the crosshair and hit
// marker, the level-up banner, the in-hand card, the interaction prompt and the feeds. Running it frame by frame is in
// PlayerHUDWidget.cpp.

using namespace LooterUI;

namespace
{
	/**
	 * The "IN HAND" word over the in-hand card (the mockup's, wide.png): small, dim and widely spaced like the HUD's other
	 * labels, right-aligned a little in from the card's cut corner, with a small gap down to the card.
	 */
	constexpr int32 InHandWordSize = 12;
	constexpr int32 InHandWordSpacing = 250;
	constexpr float InHandWordInset = 8.f;
	constexpr float InHandWordGap = 6.f;
	/**
	 * How far left of the screen's middle the pickup feed's lines end: past the farthest a crosshair reaches
	 * (HudCrosshair::MaxReach, 50 px) with room to spare, so a line never covers the crosshair or the hit marker.
	 */
	constexpr float PickupFeedGap = 76.f;

	// The weapon cluster, in 1080p pixels. It keeps its corner 48 px in from the right edge; its last line (the gun's
	// name) ends 32 px above the bottom, which puts the slots' circles at y 812, 888 and 964 (x 1775), the cartridge at
	// (1821, 800) with its base level with the last slot's bottom, and the fire mode's line at y 1000.
	constexpr float ClusterRight = 48.f;
	constexpr float ClusterBottom = 32.f;
	/** The cartridge stands this far right of the slots' column. */
	constexpr float CartridgeGap = 14.f;
	/**
	 * The ember socket stands this far left of the slots' column: the slots' own gap (12) plus the 22 px a slot in hand
	 * reaches further left as it moves over and grows (UHudWeaponSlotsWidget's lift), so the last slot in hand never
	 * touches the socket.
	 */
	constexpr float EmberGap = 34.f;
	/** The grenade counter sits this far above the column's top (slot 1's circle and the cartridge's tip). */
	constexpr float GrenadeGap = 10.f;
	/** The two lines under them: the status and fire mode, then the gun's name, each this far below the last. */
	constexpr float LineGap = 4.f;
	constexpr float ModeLineHeight = 20.f;
	constexpr float NameLineHeight = 24.f;
	/**
	 * The Arms row (slots and cartridge) as the ember socket's row reproduces it: its width (a slot row, as
	 * HudWeaponSlotsWidget.cpp lays it out: ammo icon 20, gap 8, tab, gap 8, circle; then the cartridge's gap and the
	 * cartridge) and how far its bottom stands above the cluster's (the fire mode's and the name's lines with their gaps).
	 */
	constexpr float SlotRowWidth = 20.f + 8.f + UHudWeaponSlotsWidget::TabWidth + 8.f + UHudWeaponSlotsWidget::SlotDiameter;
	constexpr float ArmsWidth = SlotRowWidth + CartridgeGap + UHudMagazineWidget::Width;
	constexpr float ArmsLift = LineGap + ModeLineHeight + LineGap + NameLineHeight;
	/** The status sits this far left of the fire mode, on its line. */
	constexpr float StatusGap = 12.f;
	/** The rarity gem before the name: a diamond, tip to tip, over its ink edge (1.6 px wider each side), and the gap after
	 *  it (the user's call, 2026-10-08: the gem on the name's left, 3 px further from it than before). */
	constexpr float GemSize = 14.f;
	constexpr float GemEdgeSize = 18.5f;
	constexpr float GemGap = 6.f;
	/** The gun's notches stand this far before the gem (UHudNotchTallyWidget collapses itself, gap and all, at none). */
	constexpr float NotchGap = 10.f;

	/** Where the level-up banner's middle (its big level number) sits, down from the top's middle: its title then clears
	 *  the boss bar (UI round 3's Signal Glass level-up, 2026-10-29). */
	constexpr float BannerTop = 260.f;

	/** Where the "GRAVE MARKED" banner's lantern and words centre, down from the top's middle: above the level-up banner's gem
	 *  (it waits for that banner and the mission-complete one), under the boss bar's cluster, which ends 126 px down. */
	constexpr float GraveBannerTop = 175.f;

	/** The "FOUND" banner's box centre, down from the top's middle: its tally and title sit where the roost banner's lantern and
	 *  words do (it takes turns with it), under the boss bar's cluster. */
	/** The find banner's middle, as a share of the screen's height. */
	constexpr float FindBannerHeight = 0.78f;

	UCanvasPanelSlot* PlaceOnCanvas(UCanvasPanel* Canvas, UWidget* Widget, const FAnchors& Anchors, const FVector2D& Alignment, const FVector2D& Position)
	{
		UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Widget);
		CanvasSlot->SetAnchors(Anchors);
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetPosition(Position);
		CanvasSlot->SetAutoSize(true);
		return CanvasSlot;
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

		// Round the crosshair: where a hit came from, as red arcs on a ring. It reads the player's health itself.
		{
			UHudDamageIndicatorWidget* DamageArcs = WidgetTree->ConstructWidget<UHudDamageIndicatorWidget>(UHudDamageIndicatorWidget::StaticClass());
			UCanvasPanelSlot* ArcsSlot = Root->AddChildToCanvas(DamageArcs);
			ArcsSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			ArcsSlot->SetOffsets(FMargin(0.f));
			ArcsSlot->SetAutoSize(false);
		}

		// Crosshair: the gun kind's shape (HudCrosshairRules) round its centre dot; the gap follows the weapon's spread, and
		// each shot kicks it out. The dot is its own piece: through a third-person aim it stays as the aim point when the
		// lines have gone. Both paint in the same square, centred, so the dot sits where its shape puts it.
		{
			CrosshairLines = WidgetTree->ConstructWidget<UHudCrosshairWidget>(UHudCrosshairWidget::StaticClass());
			CrosshairLines->SetPart(EHudCrosshairPart::Lines);
			CrosshairBox = MakeSized(WidgetTree, CrosshairLines, HudCrosshair::CanvasSize, HudCrosshair::CanvasSize);
			// The kick and the switch grow it about its middle.
			CrosshairBox->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			PlaceOnCanvas(Root, CrosshairBox, Center, FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);

			// Full strength: its opacity (UpdateCrosshair) sets how soft it is.
			CrosshairDotShape = WidgetTree->ConstructWidget<UHudCrosshairWidget>(UHudCrosshairWidget::StaticClass());
			CrosshairDotShape->SetPart(EHudCrosshairPart::Dot);
			CrosshairDot = MakeSized(WidgetTree, CrosshairDotShape, HudCrosshair::CanvasSize, HudCrosshair::CanvasSize);
			CrosshairDot->SetRenderOpacity(0.8f);
			PlaceOnCanvas(Root, CrosshairDot, Center, FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
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

		// Under the frame's experience bar, its left end level with the bar's: the soul flask's row (collapsed with no flask),
		// its middle at UHudFlaskWidget::Middle on the 1080p screen. Healing sits with health; clear of the medallion, the
		// gem, the bar and its numbers.
		FlaskRow = WidgetTree->ConstructWidget<UHudFlaskWidget>(UHudFlaskWidget::StaticClass());
		PlaceOnCanvas(Root, FlaskRow, FAnchors(0.f, 1.f), FVector2D(0.f, 0.5f),
			FVector2D(UHudFlaskWidget::Left, -(UHudFlaskWidget::ScreenBottom - UHudFlaskWidget::Middle)));

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

			// At the very top, right-aligned: the grave-salt grenades carried (collapsed until the player has some), its
			// count over the cartridge and its tin over the slots' circles. No silver here (the user's call, 2026-10-27).
			GrenadeCounter = WidgetTree->ConstructWidget<UHudGrenadeWidget>(UHudGrenadeWidget::StaticClass());
			AddRight(GrenadeCounter, 0.f);

			UHorizontalBox* Arms = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			WeaponSlots = WidgetTree->ConstructWidget<UHudWeaponSlotsWidget>(UHudWeaponSlotsWidget::StaticClass());
			Arms->AddChildToHorizontalBox(WeaponSlots)->SetVerticalAlignment(VAlign_Bottom);
			MagazineGauge = WidgetTree->ConstructWidget<UHudMagazineWidget>(UHudMagazineWidget::StaticClass());
			UHorizontalBoxSlot* CartridgeSlot = Arms->AddChildToHorizontalBox(MagazineGauge);
			CartridgeSlot->SetVerticalAlignment(VAlign_Bottom);
			CartridgeSlot->SetPadding(FMargin(CartridgeGap, 0.f, 0.f, 0.f));
			AddRight(Arms, GrenadeGap);

			// The status (orange; "RELOADING", "[R] RELOAD", "NO AMMO") left of the fire mode, right-aligned.
			UHorizontalBox* ModeRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			StatusText = MakeFloatingText(WidgetTree, 12, Color::Accent(), 200, ETextJustify::Right);
			UHorizontalBoxSlot* StatusSlot = ModeRow->AddChildToHorizontalBox(StatusText);
			StatusSlot->SetVerticalAlignment(VAlign_Center);
			StatusSlot->SetPadding(FMargin(0.f, 0.f, StatusGap, 0.f));
			FireModeText = MakeFloatingText(WidgetTree, 13, Color::TextDim(), 150, ETextJustify::Right);
			ModeRow->AddChildToHorizontalBox(FireModeText)->SetVerticalAlignment(VAlign_Center);
			AddRight(MakeSized(WidgetTree, ModeRow, 0.f, ModeLineHeight), LineGap);

			// The gun's notches (a tally mark and the count, once it has any), the rarity gem, then the gun's name in its
			// rarity's color, shrinking to fit when it's long, at the edge. The notches grow the row leftward, never pushing
			// the name off the edge.
			UHorizontalBox* NameRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			NotchTally = WidgetTree->ConstructWidget<UHudNotchTallyWidget>(UHudNotchTallyWidget::StaticClass());
			UHorizontalBoxSlot* NotchSlot = NameRow->AddChildToHorizontalBox(NotchTally);
			NotchSlot->SetVerticalAlignment(VAlign_Center);
			NotchSlot->SetPadding(FMargin(0.f, 0.f, NotchGap, 0.f));
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
			GemRowSlot->SetPadding(FMargin(0.f, 0.f, GemGap, 0.f));
			// No scale box: one centred whatever it shrank (and fitted the 24 px line's height too, shrinking every name),
			// so the name stopped short of the right edge and the gem stood far from it (seen in the 1080p HUD shots).
			// FitWeaponName sets a long name smaller instead, measured when the gun changes.
			WeaponName = MakeFloatingText(WidgetTree, NameFontSize, Color::Text(), 60, ETextJustify::Right);
			NameRow->AddChildToHorizontalBox(WeaponName)->SetVerticalAlignment(VAlign_Center);
			AddRight(MakeSized(WidgetTree, NameRow, 0.f, NameLineHeight), LineGap);

			WeaponCluster = Box;
			PlaceOnCanvas(Root, Box, FAnchors(1.f, 1.f), FVector2D(1.f, 1.f), FVector2D(-ClusterRight, -ClusterBottom));
		}

		// The equipped ember power's socket (collapsed with none), apart from the weapon cluster, which hides with no gun in
		// hand: Q works unarmed, so the socket shows armed or not. It stands where the cluster's Arms row would put it, left
		// of the slots with its bottom level with the last slot's: a spacer as wide as the slots and the cartridge stands in
		// for them, and it sits above the cluster's two lines (fire mode, gun's name).
		{
			UHorizontalBox* EmberRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			EmberRow->SetVisibility(ESlateVisibility::HitTestInvisible);
			EmberSocket = WidgetTree->ConstructWidget<UHudEmberSocketWidget>(UHudEmberSocketWidget::StaticClass());
			UHorizontalBoxSlot* EmberSlot = EmberRow->AddChildToHorizontalBox(EmberSocket);
			EmberSlot->SetVerticalAlignment(VAlign_Bottom);
			EmberSlot->SetPadding(FMargin(0.f, 0.f, EmberGap, 0.f));
			EmberRow->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, ArmsWidth, 1.f))->SetVerticalAlignment(VAlign_Bottom);
			EmberArea = EmberRow;
			PlaceOnCanvas(Root, EmberRow, FAnchors(1.f, 1.f), FVector2D(1.f, 1.f), FVector2D(-ClusterRight, -(ClusterBottom + ArmsLift)));
		}

		// Right-middle, while loot is looked at with a gun in hand: "IN HAND" over that gun's card, to hold against the
		// looked-at gun's card over it (the user's call, 2026-10-28). UpdatePickupCard fills it and shows it.
		{
			UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			UTextBlock* InHandWord = MakeFloatingText(WidgetTree, InHandWordSize, Color::TextDim(), InHandWordSpacing, ETextJustify::Right);
			InHandWord->SetText(FText::FromString(TEXT("IN HAND")));
			UVerticalBoxSlot* WordSlot = Box->AddChildToVerticalBox(InHandWord);
			WordSlot->SetHorizontalAlignment(HAlign_Right);
			WordSlot->SetPadding(FMargin(0.f, 0.f, InHandWordInset, InHandWordGap));
			InHandCard = WidgetTree->ConstructWidget<UGunCardWidget>(UGunCardWidget::StaticClass(), TEXT("InHandCard"));
			Box->AddChildToVerticalBox(InHandCard)->SetHorizontalAlignment(HAlign_Right);
			PickupCard = Box;
			PickupCard->SetVisibility(ESlateVisibility::Collapsed);
			PlaceOnCanvas(Root, PickupCard, FAnchors(1.f, 0.5f), FVector2D(1.f, 0.5f), FVector2D(-32.f, 0.f));
		}

		// Under the crosshair: what the Interact key does to the thing looked at (loot has its card over the gun instead).
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
		PickupFeed = WidgetTree->ConstructWidget<UHudPickupFeedWidget>(UHudPickupFeedWidget::StaticClass());
		UCanvasPanelSlot* FeedSlot = PlaceOnCanvas(Root, PickupFeed, Center, FVector2D(1.f, 1.f),
			FVector2D(-PickupFeedGap, UHudPickupFeedWidget::LineSpacing * 0.5f));
		FeedSlot->SetAutoSize(false);
		FeedSlot->SetSize(FVector2D(UHudPickupFeedWidget::Width, UHudPickupFeedWidget::Height));

		// Top-centre: the level-up banner, its gem centred 260 px down. It shows nothing until a level-up. (The bottom
		// centre, where the experience bar was, stays empty: the player frame has it.)
		LevelUpBanner = WidgetTree->ConstructWidget<UHudLevelUpBannerWidget>(UHudLevelUpBannerWidget::StaticClass());
		PlaceOnCanvas(Root, LevelUpBanner, FAnchors(0.5f, 0.f), FVector2D(0.5f, 0.5f), FVector2D(0.f, BannerTop));

		// In the same place: the mission-complete banner as a mission is turned in, taking turns with the level-up banner.
		MissionBanner = WidgetTree->ConstructWidget<UHudMissionCompleteWidget>(UHudMissionCompleteWidget::StaticClass());
		MissionBanner->SetLevelUpBanner(LevelUpBanner);
		PlaceOnCanvas(Root, MissionBanner, FAnchors(0.5f, 0.f), FVector2D(0.5f, 0.5f), FVector2D(0.f, BannerTop));

		// A little above them: "GRAVE MARKED" when the player walks up to a respawn grave for the first time.
		GraveBanner = WidgetTree->ConstructWidget<UHudGraveMarkedWidget>(UHudGraveMarkedWidget::StaticClass());
		GraveBanner->SetOtherBanners(LevelUpBanner, MissionBanner);
		PlaceOnCanvas(Root, GraveBanner, FAnchors(0.5f, 0.f), FVector2D(0.5f, 0.5f), FVector2D(0.f, GraveBannerTop));

		// In the same place: "FOUND" when the player finds a thing off the path; it and the roost banner wait for each other's show.
		FindBanner = WidgetTree->ConstructWidget<UHudFindBannerWidget>(UHudFindBannerWidget::StaticClass());
		FindBanner->SetOtherBanners(LevelUpBanner, MissionBanner, GraveBanner);
		GraveBanner->SetOtherBanners(LevelUpBanner, MissionBanner, FindBanner);
		// Low in the middle, between the player frame and the weapon column: up high, the card over a looted gun covered it
		// (the 2026-10-29 play-check, a chest's guns in the crosshair as the banner came up).
		PlaceOnCanvas(Root, FindBanner, FAnchors(0.5f, FindBannerHeight), FVector2D(0.5f, 0.5f), FVector2D(0.f, 0.f));
	}
	return Super::RebuildWidget();
}

// The boss bar's build: the gem, the name, the bar's metalwork, fill and cuts, and the phase line under it.
#include "UI/HUD/HudBossBarWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

using namespace LooterUI;

namespace
{
	/**
	 * The readout's box, centred on the bar at the top of the screen (its top this far down, clear of the frame rate and
	 * the minimap in the corners): room either side of the bar for the gem on the left and the clamp on the right.
	 */
	constexpr float BossClusterTop = 30.f;
	constexpr float BossClusterSide = 60.f;
	constexpr float BossClusterWidth = UHudBossBarWidget::BarWidth + BossClusterSide * 2.f;
	constexpr float BossClusterHeight = 96.f;

	/** The bar, under the name: 640 x 30, leaning 16 degrees like every HUD bar (its top toward the right). */
	constexpr float BossBarTop = 36.f;
	constexpr float BossBarHeight = 30.f;
	constexpr float BossBarSlant = -16.f;
	/** The ink rim, the gunmetal bezel inside it, and the track inside that. */
	constexpr float BossRim = 2.f;
	constexpr float BossTrackInset = BossRim + 4.f;
	constexpr float BossTrackWidth = UHudBossBarWidget::BarWidth - BossTrackInset * 2.f;
	constexpr float BossTrackHeight = BossBarHeight - BossTrackInset * 2.f;
	/** The track: the health bar's dark glass, a thin dark line inside its edge. */
	constexpr float BossTrackOpacity = 0.88f;
	constexpr float BossTrackLineOpacity = 0.6f;

	/** The fill's bands from the bottom: the middle and dark ones over the light top (36% light, 28% dark). */
	constexpr float BossMidBandShare = 0.64f;
	constexpr float BossLowBandShare = 0.28f;
	/** The fill's lit top line and its light leading edge. */
	constexpr float BossTopLineHeight = 1.f;
	constexpr float BossEdgeWidth = 2.f;
	/**
	 * The hatch over the fill: 45-degree lines falling to the right, 1.4 px across and about 5 px apart square to them (the
	 * health bar's hatch; the spec in Docs/Handoffs/CloudIslandConcepts_2026-10-05.md), black at 13%. It is a tile one
	 * period wide that repeats along the fill: cut off, never squeezed.
	 * Slate repeats a tiled brush every TextureWidth pixels and draws it over the image's height, whatever the brush's
	 * ImageSize, so the tile's texture is made at 1 texel to the pixel (BossHatchPixelsPerUnit) and its period is a whole
	 * number of them: 7 along the bar is 4.95 apart square to the lines (5 x sqrt 2 is 7.07), and a line that runs one
	 * pixel across for each pixel down stays at exactly 45 degrees.
	 */
	constexpr float BossHatchPeriod = 7.f;
	constexpr float BossHatchWidth = 1.4f;
	constexpr float BossHatchOpacity = 0.13f;
	constexpr float BossHatchPixelsPerUnit = 1.f;
	/** A phase's cut across the track. */
	constexpr float BossCutWidth = 3.f;

	/** The orange "]" over the far end: 14 x 36, from 7 px inside the end and 5 px above the bar; its arms are 5 thick. */
	constexpr float BossClampWidth = 14.f;
	constexpr float BossClampHeight = 36.f;
	constexpr float BossClampThickness = 5.f;
	constexpr float BossClampLeft = UHudBossBarWidget::BarWidth - 7.f;
	constexpr float BossClampTop = -5.f;

	/**
	 * A clear border round the bar's pictures: the lean slants their edges, and only edges inside a picture come out
	 * smooth (a picture's own border would step along the slant).
	 */
	constexpr float BossPictureMargin = 1.5f;
	/** The pictures are drawn at twice their size, so they stay crisp. */
	constexpr float BossPixelsPerUnit = 2.f;

	/** The gem: 44 px tip to tip in a 52 px box, centred left of the bar about level with it; the level in ink, 18 px. */
	constexpr float BossGemBox = 52.f;
	constexpr float BossGemRadius = 22.f;
	constexpr float BossGemX = BossClusterSide - 28.f;
	constexpr float BossGemY = 48.f;
	constexpr int32 BossLevelSize = 18;
	/** The name in its rank's color over the bar's left end; the phase line centred under the bar. */
	constexpr float BossNameLeft = BossClusterSide + 4.f;
	constexpr float BossNameGap = 2.f;
	constexpr int32 BossNameSize = 25;
	constexpr int32 BossNameSpacing = 120;
	constexpr float BossPhaseGap = 4.f;
	constexpr int32 BossPhaseSize = 13;
	constexpr int32 BossPhaseSpacing = 270;

	TArray<FVector2D> BossRect(float Left, float Top, float Right, float Bottom)
	{
		return { FVector2D(Left, Top), FVector2D(Right, Top), FVector2D(Right, Bottom), FVector2D(Left, Bottom) };
	}

	/** A convex Polygon cut to the band MinX <= x <= MaxX. */
	TArray<FVector2D> BossClipToSpan(const TArray<FVector2D>& Polygon, float MinX, float MaxX)
	{
		// One side at a time (Sutherland-Hodgman): Side +1 keeps x >= Edge, -1 keeps x <= Edge.
		auto ClipSide = [](const TArray<FVector2D>& In, float Edge, float Side)
		{
			TArray<FVector2D> Out;
			for (int32 Index = 0; Index < In.Num(); ++Index)
			{
				const FVector2D& A = In[Index];
				const FVector2D& B = In[(Index + 1) % In.Num()];
				const bool bAIn = (A.X - Edge) * Side >= 0.0;
				const bool bBIn = (B.X - Edge) * Side >= 0.0;
				if (bAIn)
				{
					Out.Add(A);
				}
				if (bAIn != bBIn)
				{
					Out.Add(A + (B - A) * ((Edge - A.X) / (B.X - A.X)));
				}
			}
			return Out;
		};
		return ClipSide(ClipSide(Polygon, MinX, 1.f), MaxX, -1.f);
	}

	/**
	 * The bar's metal, in a clear border: the ink rim, and the gunmetal bezel lit along its top and shaded along its
	 * bottom. The track's place is cut out of the bezel, so when the UI transparency setting fades the track the world
	 * shows through the bar's inside, as through the HUD's other tracks.
	 */
	const FPaintedIcon& BossFramePicture()
	{
		static const FPaintedIcon Picture = []
		{
			const float M = BossPictureMargin;
			const float Right = M + UHudBossBarWidget::BarWidth;
			const float Bottom = M + BossBarHeight;
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(Right + M, Bottom + M);
			// Shapes listed together fill even-odd, so a rectangle inside another cuts it out.
			FPaintLayer Rim;
			Rim.Fills.Add(BossRect(M, M, Right, Bottom));
			Rim.Fills.Add(BossRect(M + BossRim, M + BossRim, Right - BossRim, Bottom - BossRim));
			Rim.Color = Color::Ink();
			Result.Layers.Add(Rim);
			FPaintLayer Bezel;
			Bezel.Fills.Add(BossRect(M + BossRim, M + BossRim, Right - BossRim, Bottom - BossRim));
			Bezel.Fills.Add(BossRect(M + BossTrackInset, M + BossTrackInset, Right - BossTrackInset, Bottom - BossTrackInset));
			Bezel.Color = Color::BarMetalHi();
			Bezel.GradientTo = Color::BarMetalLow();
			Bezel.GradientStart = FVector2D(0.f, M + BossRim);
			Bezel.GradientEnd = FVector2D(0.f, Bottom - BossRim);
			Result.Layers.Add(Bezel);
			FPaintLayer Lit;
			Lit.Fills.Add(BossRect(M + BossRim, M + BossRim, Right - BossRim, M + BossRim + 1.f));
			Lit.Color = FLinearColor::White;
			Lit.Opacity = 0.35f;
			Result.Layers.Add(Lit);
			FPaintLayer Shade;
			Shade.Fills.Add(BossRect(M + BossRim, Bottom - BossRim - 1.f, Right - BossRim, Bottom - BossRim));
			Shade.Color = Color::Ink();
			Shade.Opacity = 0.5f;
			Result.Layers.Add(Shade);
			return Result;
		}();
		return Picture;
	}

	/** One period of the fill's hatch, as tall as the track; tiled along the fill, its lines join up from tile to tile. Made at 1x. */
	const FPaintedIcon& BossHatchPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(BossHatchPeriod, BossTrackHeight);
			FPaintLayer Hatch;
			// A line's width along the bar; the lines that start a period or more back cross the tile lower down.
			const float Along = BossHatchWidth * UE_SQRT_2;
			const int32 Back = FMath::CeilToInt32(BossTrackHeight / BossHatchPeriod);
			for (int32 Copy = -Back; Copy <= 0; ++Copy)
			{
				const float Start = Copy * BossHatchPeriod;
				const TArray<FVector2D> Line = { FVector2D(Start, 0.f), FVector2D(Start + Along, 0.f),
					FVector2D(Start + Along + BossTrackHeight, BossTrackHeight), FVector2D(Start + BossTrackHeight, BossTrackHeight) };
				TArray<FVector2D> InTile = BossClipToSpan(Line, 0.f, BossHatchPeriod);
				if (InTile.Num() >= 3)
				{
					Hatch.Fills.Add(MoveTemp(InTile));
				}
			}
			Hatch.Color = Color::Ink();
			Hatch.Opacity = BossHatchOpacity;
			Result.Layers.Add(Hatch);
			return Result;
		}();
		return Picture;
	}

	/** The orange "]" clamp over the bar's far end, edged in ink, in a clear border. */
	const FPaintedIcon& BossClampPicture()
	{
		static const FPaintedIcon Picture = []
		{
			const float M = BossPictureMargin;
			const float Right = M + BossClampWidth;
			const float Bottom = M + BossClampHeight;
			const float Inner = Right - BossClampThickness;
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(Right + M, Bottom + M);
			const TArray<FVector2D> Shape = { FVector2D(M, M), FVector2D(Right, M), FVector2D(Right, Bottom), FVector2D(M, Bottom),
				FVector2D(M, Bottom - BossClampThickness), FVector2D(Inner, Bottom - BossClampThickness),
				FVector2D(Inner, M + BossClampThickness), FVector2D(M, M + BossClampThickness) };
			FPaintLayer Fill;
			Fill.Fills.Add(Shape);
			Fill.Color = Color::Accent();
			Result.Layers.Add(Fill);
			FPaintLayer Edge;
			TArray<FVector2D> Outline = Shape;
			Outline.Add(Shape[0]);
			Edge.Strokes.Add(MoveTemp(Outline));
			Edge.StrokeWidth = 2.f;
			Edge.Color = Color::Ink();
			Result.Layers.Add(Edge);
			return Result;
		}();
		return Picture;
	}

	TArray<FVector2D> BossGemDiamond(float Radius)
	{
		const float Center = BossGemBox * 0.5f;
		return { FVector2D(Center, Center - Radius), FVector2D(Center + Radius, Center), FVector2D(Center, Center + Radius),
			FVector2D(Center - Radius, Center) };
	}

	/** The gem's face, white so it takes the boss's rank's color as a tint. */
	const FPaintedIcon& BossGemFacePicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(BossGemBox);
			FPaintLayer Face;
			Face.Fills.Add(BossGemDiamond(BossGemRadius));
			Face.Color = FLinearColor::White;
			Result.Layers.Add(Face);
			return Result;
		}();
		return Picture;
	}

	/** Over the face: its ink edge and a light line inside its upper edges, the way the HUD's metal is lit. */
	const FPaintedIcon& BossGemEdgePicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(BossGemBox);
			FPaintLayer Edge;
			TArray<FVector2D> Outline = BossGemDiamond(BossGemRadius);
			Outline.Add(FVector2D(Outline[0])); // a copy: adding an element of the array itself trips TArray's check (the add may reallocate)
			Edge.Strokes.Add(MoveTemp(Outline));
			Edge.StrokeWidth = 2.8f;
			Edge.Color = Color::Ink();
			Result.Layers.Add(Edge);
			FPaintLayer Light;
			const TArray<FVector2D> Inside = BossGemDiamond(BossGemRadius - 7.5f);
			Light.Strokes.Add(TArray<FVector2D>{ Inside[3], Inside[0], Inside[1] });
			Light.StrokeWidth = 1.3f;
			Light.Color = FLinearColor::White;
			Light.Opacity = 0.7f;
			Result.Layers.Add(Light);
			return Result;
		}();
		return Picture;
	}

	/** A picture of the bar, as a brush its own size. */
	FSlateBrush BossPictureBrush(FName Name, const FPaintedIcon& Picture)
	{
		return PaintedIconBrush(Name, Picture, BossPixelsPerUnit, Picture.ViewBox);
	}

	/** Content in the track, from its left end, cut to length later by its box's width (ApplyFill). */
	USizeBox* AddBossLength(UWidgetTree* Tree, UOverlay* Track, UWidget* Content)
	{
		USizeBox* Length = MakeSized(Tree, Content, 0.f, BossTrackHeight);
		Length->SetWidthOverride(0.f);
		UOverlaySlot* LengthSlot = Track->AddChildToOverlay(Length);
		LengthSlot->SetHorizontalAlignment(HAlign_Left);
		LengthSlot->SetVerticalAlignment(VAlign_Fill);
		return Length;
	}

	/** Shows a widget, or hides it, only when that changes (the fill and chip change every frame while draining). */
	void BossShowIf(UWidget* Widget, bool bShow)
	{
		const ESlateVisibility Wanted = bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
		if (Widget && Widget->GetVisibility() != Wanted)
		{
			Widget->SetVisibility(Wanted);
		}
	}
}

TSharedRef<SWidget> UHudBossBarWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Root;

		UCanvasPanel* Box = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		UCanvasPanelSlot* BoxSlot = Root->AddChildToCanvas(Box);
		BoxSlot->SetAnchors(FAnchors(0.5f, 0.f));
		BoxSlot->SetAlignment(FVector2D(0.5f, 0.f));
		BoxSlot->SetPosition(FVector2D(0.f, BossClusterTop));
		BoxSlot->SetSize(FVector2D(BossClusterWidth, BossClusterHeight));
		auto AddToBox = [Box](UWidget* Widget, const FVector2D& Position, const FVector2D& Alignment, const FVector2D& Size)
		{
			UCanvasPanelSlot* WidgetSlot = Box->AddChildToCanvas(Widget);
			WidgetSlot->SetPosition(Position);
			WidgetSlot->SetAlignment(Alignment);
			WidgetSlot->SetAutoSize(Size.IsZero());
			if (!Size.IsZero())
			{
				WidgetSlot->SetSize(Size);
			}
		};

		// The bar leans as one piece, so the fill's end leans with the bar's ends. Back to front: the rim and bezel, the
		// track (a background), the chip, the fill, the phase cuts, then the clamp over the far end.
		UCanvasPanel* Bar = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		auto AddToBar = [Bar](UWidget* Part, float Left, float Top, const FVector2D& Size)
		{
			UCanvasPanelSlot* PartSlot = Bar->AddChildToCanvas(Part);
			PartSlot->SetPosition(FVector2D(Left, Top));
			PartSlot->SetSize(Size);
		};
		const float Margin = BossPictureMargin;
		const FPaintedIcon& Frame = BossFramePicture();
		AddToBar(MakeImage(WidgetTree, BossPictureBrush(TEXT("BossBarFrame"), Frame)), -Margin, -Margin, Frame.ViewBox);

		UOverlay* Track = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		AddToBar(Track, BossTrackInset, BossTrackInset, FVector2D(BossTrackWidth, BossTrackHeight));
		UImage* TrackGlass = MakeImage(WidgetTree, RectBrush(Color::Track().CopyWithNewOpacity(BossTrackOpacity),
			Color::Ink().CopyWithNewOpacity(BossTrackLineOpacity), 1.f));
		MarkBackground(TrackGlass);
		FillOverlaySlot(Track->AddChildToOverlay(TrackGlass));
		ChipBox = AddBossLength(WidgetTree, Track, MakeImage(WidgetTree, RectBrush(Color::HealthChip())));

		// The fill: flat bands (tinted by ApplyColors, so they can grey), the hatch tiled from the empty end, the lit top
		// line and the leading edge at the far end. Its box's width cuts it to length; nothing in it stretches.
		UOverlay* Lit = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		FillBands.Reset();
		for (const float Share : { 1.f, BossMidBandShare, BossLowBandShare })
		{
			UImage* Band = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
			UOverlaySlot* BandSlot = Lit->AddChildToOverlay(MakeSized(WidgetTree, Band, 0.f, BossTrackHeight * Share));
			BandSlot->SetHorizontalAlignment(HAlign_Fill);
			BandSlot->SetVerticalAlignment(VAlign_Bottom);
			FillBands.Add(Band);
		}
		// Not BossPictureBrush: the tile is made at 1x so it repeats every BossHatchPeriod pixels and fills the track's height 1:1.
		FSlateBrush HatchBrush = PaintedIconBrush(TEXT("BossBarHatch"), BossHatchPicture(), BossHatchPixelsPerUnit, BossHatchPicture().ViewBox);
		HatchBrush.Tiling = ESlateBrushTileType::Horizontal;
		FillOverlaySlot(Lit->AddChildToOverlay(MakeImage(WidgetTree, HatchBrush)));
		FillTopLine = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
		UOverlaySlot* TopLineSlot = Lit->AddChildToOverlay(MakeSized(WidgetTree, FillTopLine, 0.f, BossTopLineHeight));
		TopLineSlot->SetHorizontalAlignment(HAlign_Fill);
		TopLineSlot->SetVerticalAlignment(VAlign_Top);
		FillEdge = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
		UOverlaySlot* EdgeSlot = Lit->AddChildToOverlay(MakeSized(WidgetTree, FillEdge, BossEdgeWidth));
		EdgeSlot->SetHorizontalAlignment(HAlign_Right);
		EdgeSlot->SetVerticalAlignment(VAlign_Fill);
		FillBox = AddBossLength(WidgetTree, Track, Lit);

		TickLayer = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		FillOverlaySlot(Track->AddChildToOverlay(TickLayer));

		const FPaintedIcon& Clamp = BossClampPicture();
		AddToBar(MakeImage(WidgetTree, BossPictureBrush(TEXT("BossBarClamp"), Clamp)), BossClampLeft - Margin, BossClampTop - Margin,
			Clamp.ViewBox);

		USizeBox* BarSize = MakeSized(WidgetTree, Bar, UHudBossBarWidget::BarWidth, BossBarHeight);
		BarSize->SetRenderShear(FVector2D(BossBarSlant, 0.f));
		AddToBox(BarSize, FVector2D(BossClusterSide, BossBarTop), FVector2D::ZeroVector,
			FVector2D(UHudBossBarWidget::BarWidth, BossBarHeight));

		// The level in a gem of the boss's rank's color (ApplyColors tints its face), left of the bar.
		UOverlay* Gem = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		GemFace = MakeImage(WidgetTree, BossPictureBrush(TEXT("BossGemFace"), BossGemFacePicture()));
		FillOverlaySlot(Gem->AddChildToOverlay(GemFace));
		FillOverlaySlot(Gem->AddChildToOverlay(MakeImage(WidgetTree, BossPictureBrush(TEXT("BossGemEdge"), BossGemEdgePicture()))));
		// Ink on the gem's color, as the player's level gem; no outline, it's not floating over the world.
		LevelLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		LevelLabel->SetFont(Font(BossLevelSize));
		LevelLabel->SetColorAndOpacity(FSlateColor(Color::Ink()));
		LevelLabel->SetJustification(ETextJustify::Center);
		UOverlaySlot* LevelSlot = Gem->AddChildToOverlay(LevelLabel);
		LevelSlot->SetHorizontalAlignment(HAlign_Center);
		LevelSlot->SetVerticalAlignment(VAlign_Center);
		AddToBox(Gem, FVector2D(BossGemX, BossGemY), FVector2D(0.5f, 0.5f), FVector2D(BossGemBox));

		// The name over the bar's left end, its foot just above the bar whatever the type's height.
		NameLabel = MakeFloatingText(WidgetTree, BossNameSize, NameColor, BossNameSpacing, ETextJustify::Left);
		AddToBox(NameLabel, FVector2D(BossNameLeft, BossBarTop - BossNameGap), FVector2D(0.f, 1.f), FVector2D::ZeroVector);

		// Under the bar: the phase's name, or while the boss can't be hurt, what to do about it. It pops from its middle.
		PhaseLabel = MakeFloatingText(WidgetTree, BossPhaseSize, Color::TextDim(), BossPhaseSpacing, ETextJustify::Center);
		PhaseLabel->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		AddToBox(PhaseLabel, FVector2D(BossClusterWidth * 0.5f, BossBarTop + BossBarHeight + BossPhaseGap), FVector2D(0.5f, 0.f),
			FVector2D::ZeroVector);

		Box->SetRenderOpacity(Opacity);
		Box->SetVisibility(Opacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		Cluster = Box;

		ShownFillWidth = -1.f;
		ShownChipWidth = -1.f;
		BuildTicks();
		ApplyTexts();
		ApplyFill();
		ApplyColors();
	}
	return Super::RebuildWidget();
}

void UHudBossBarWidget::BuildTicks()
{
	if (!TickLayer || !WidgetTree)
	{
		return;
	}
	TickLayer->ClearChildren();
	TickImages.Reset();
	for (const float Share : TickShares)
	{
		UImage* Tick = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
		UCanvasPanelSlot* TickSlot = TickLayer->AddChildToCanvas(Tick);
		TickSlot->SetPosition(FVector2D(BossTrackWidth * Share - BossCutWidth * 0.5f, 0.f));
		TickSlot->SetSize(FVector2D(BossCutWidth, BossTrackHeight));
		TickImages.Add(Tick);
	}
	ShownLitTicks = INDEX_NONE;
}

void UHudBossBarWidget::ApplyFill()
{
	if (!FillBox || !ChipBox)
	{
		return;
	}
	// In half pixels, so a draining chip only repaints when its end visibly moves. An empty one hides (nothing to draw).
	// The intro runs both up from empty.
	const float Sweep = GetIntroSweep();
	const float FillWidth = FMath::RoundToFloat(BossTrackWidth * Fraction * Sweep * 2.f) * 0.5f;
	const float ChipWidth = FMath::RoundToFloat(BossTrackWidth * ChipFraction * Sweep * 2.f) * 0.5f;
	if (FillWidth != ShownFillWidth)
	{
		ShownFillWidth = FillWidth;
		FillBox->SetWidthOverride(FillWidth);
		BossShowIf(FillBox, FillWidth > 0.f);
	}
	if (ChipWidth != ShownChipWidth)
	{
		ShownChipWidth = ChipWidth;
		ChipBox->SetWidthOverride(ChipWidth);
		BossShowIf(ChipBox, ChipWidth > 0.f);
	}

	// A cut is dark across the red while the health left is above it, and lights up once that phase is reached; the cuts
	// are highest first.
	int32 LitTicks = 0;
	for (const float Share : TickShares)
	{
		LitTicks += Share < Fraction ? 1 : 0;
	}
	if (LitTicks == ShownLitTicks)
	{
		return;
	}
	ShownLitTicks = LitTicks;
	ColorTicks();
}

void UHudBossBarWidget::ColorTicks()
{
	// Dark while the health left is above it, lit once its phase is reached, orange while a new phase flashes.
	const FLinearColor Passed = FMath::Lerp(Color::Text(), Color::Accent(), FMath::Clamp(BarFlash, 0.f, 1.f));
	for (int32 Index = 0; Index < TickImages.Num() && Index < TickShares.Num(); ++Index)
	{
		if (UImage* Tick = TickImages[Index])
		{
			Tick->SetColorAndOpacity(TickShares[Index] < Fraction ? Color::Ink() : Passed);
		}
	}
}

// UHudGunSlotBannerWidget's pieces: the shade, the title and its gold bar, the slot (the HUD column's pictures, larger) between
// salt hairlines and gold arrowheads, the reward and what opened it. The show that moves them is HudGunSlotBannerWidget.cpp.

#include "UI/HUD/HudGunSlotBannerWidget.h"
#include "UI/HUD/HudWeaponSlotsWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

using namespace LooterUI;

namespace
{
	/** Every piece by its offset from the box's centre: the title under the boss bar's cluster, the slot under it. */
	constexpr float TitleDown = -112.f;
	constexpr float BarDown = -76.f;
	constexpr float SlotDown = 14.f;
	constexpr float RewardDown = 110.f;
	constexpr float CauseDown = 146.f;

	/** The level-up banner's type: the title 30 px spaced 0.53 em, the reward 24 px spaced 0.3 em with a 2 px outline. */
	constexpr int32 TitleSize = 30;
	constexpr int32 TitleSpacing = 533;
	constexpr int32 RewardSize = 24;
	constexpr int32 RewardSpacing = 300;
	constexpr int32 RewardOutline = 2;
	/** What opened it, quieter, under the reward. */
	constexpr int32 CauseSize = 15;
	constexpr int32 CauseSpacing = 300;
	/** The slot's number in its disc, outlined by hand like the level's number (the floating text's would swell it). */
	constexpr int32 NumberSize = 50;
	constexpr int32 NumberOutline = 3;

	/** The slot is the HUD's, half as big again. */
	constexpr float SlotScale = 1.5f;
	constexpr float SlotSize = UHudWeaponSlotsWidget::SlotDiameter * SlotScale;

	/** The salt hairlines either side of the slot, their ticks, and the gold arrowheads by the slot pointing out along them. */
	constexpr float LineLength = 170.f;
	constexpr float LineMargin = 2.f;
	constexpr float LineTick = 7.f;
	constexpr float LineArtHeight = 18.f;
	constexpr float ArrowWidth = 13.f;
	constexpr float ArrowHalfHeight = 7.5f;
	constexpr float ArrowArtWidth = 18.f;
	constexpr float ArrowArtHeight = 20.f;
	constexpr float SlotGap = 18.f;
	constexpr float ArrowToLine = 4.f;
	/** The gold bar under the title. */
	constexpr float BarWidth = 104.f;
	constexpr float BarHeight = 5.f;
	constexpr float BarEdge = 2.f;
	constexpr float PixelsPerUnit = 2.f;
	/** The soft dark shade behind it all, and the soul-gold glow behind the slot as it opens. */
	const FVector2D ShadeSize(860.f, 440.f);
	constexpr float ShadeOpacity = 0.42f;
	const FVector2D GlowSize(300.f, 300.f);
	constexpr float GlowOpacity = 0.65f;

	FSlateFontInfo OutlinedFont(int32 Size, int32 Spacing, int32 Outline)
	{
		FSlateFontInfo Result = FloatingFont(Size, Spacing);
		Result.OutlineSettings.OutlineSize = Outline;
		return Result;
	}

	FPaintLayer Strokes(TArray<TArray<FVector2D>> Lines, float StrokeWidth, const FLinearColor& LayerColor, float Opacity)
	{
		FPaintLayer Layer;
		Layer.Strokes = MoveTemp(Lines);
		Layer.StrokeWidth = StrokeWidth;
		Layer.Color = LayerColor;
		Layer.Opacity = Opacity;
		return Layer;
	}

	/** The hairline with a tick at each end: a salt line on a dark edge, as the level-up banner's (it floats over the world). */
	const FPaintedIcon& LinePicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(LineLength + LineMargin * 2.f, LineArtHeight);
			const float Mid = LineArtHeight * 0.5f;
			const float Left = LineMargin;
			const float Right = LineMargin + LineLength;
			const TArray<TArray<FVector2D>> Lines = {
				{ FVector2D(Left, Mid), FVector2D(Right, Mid) },
				{ FVector2D(Left, Mid - LineTick), FVector2D(Left, Mid + LineTick) },
				{ FVector2D(Right, Mid - LineTick), FVector2D(Right, Mid + LineTick) } };
			Result.Layers.Add(Strokes(Lines, 3.6f, Color::Ink(), 0.55f));
			Result.Layers.Add(Strokes(Lines, 1.4f, Color::SaltLine(), 0.9f));
			return Result;
		}();
		return Picture;
	}

	/** A small gold arrowhead on a dark edge; bRight points right. */
	const FPaintedIcon& ArrowPicture(bool bRight)
	{
		auto Make = [](bool bPointRight)
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(ArrowArtWidth, ArrowArtHeight);
			const float Mid = ArrowArtHeight * 0.5f;
			const float Tip = bPointRight ? ArrowArtWidth - 2.5f : 2.5f;
			const float Base = bPointRight ? Tip - ArrowWidth : Tip + ArrowWidth;
			const TArray<FVector2D> Shape = { FVector2D(Tip, Mid), FVector2D(Base, Mid - ArrowHalfHeight), FVector2D(Base, Mid + ArrowHalfHeight) };
			TArray<FVector2D> Edge = Shape;
			Edge.Add(Shape[0]);
			Result.Layers.Add(Strokes({ Edge }, 3.f, Color::Ink(), 0.6f));
			FPaintLayer Fill;
			Fill.Fills.Add(Shape);
			Fill.Color = Color::SoulGold();
			Result.Layers.Add(MoveTemp(Fill));
			return Result;
		};
		static const FPaintedIcon Left = Make(false);
		static const FPaintedIcon Right = Make(true);
		return bRight ? Right : Left;
	}

	/** The gold bar under the title, on a thin dark edge. */
	const FPaintedIcon& BarPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			const FVector2D Size(BarWidth + BarEdge * 2.f, BarHeight + BarEdge * 2.f);
			Result.ViewBox = Size;
			FPaintLayer Edge;
			Edge.Fills.Add({ FVector2D(0.f, 0.f), FVector2D(Size.X, 0.f), FVector2D(Size.X, Size.Y), FVector2D(0.f, Size.Y) });
			Edge.Color = Color::Ink();
			Edge.Opacity = 0.55f;
			Result.Layers.Add(MoveTemp(Edge));
			FPaintLayer Gold;
			Gold.Fills.Add({ FVector2D(BarEdge, BarEdge), FVector2D(Size.X - BarEdge, BarEdge), FVector2D(Size.X - BarEdge, Size.Y - BarEdge),
				FVector2D(BarEdge, Size.Y - BarEdge) });
			Gold.Color = Color::SoulGold();
			Result.Layers.Add(MoveTemp(Gold));
			return Result;
		}();
		return Picture;
	}

	/** A slot picture at the banner's size. */
	FSlateBrush SlotBrush(EHudSlotPicture Picture, float Size)
	{
		FSlateBrush Brush = UHudWeaponSlotsWidget::PictureBrush(Picture);
		Brush.ImageSize = FVector2D(Size, Size);
		return Brush;
	}
}

// ---------------------------------------------------------------------------
// Building it
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UHudGunSlotBannerWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
		PaintShow(/*bCovered*/ false);
	}
	return Super::RebuildWidget();
}

void UHudGunSlotBannerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// It never takes the mouse.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UHudGunSlotBannerWidget::BuildTree()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	const FVector2D Centre(Width * 0.5f, Height * 0.5f);
	// Each piece placed by the point Alignment names (the middle unless said), its own size.
	auto Place = [Canvas](UWidget* Child, const FVector2D& Position, const FVector2D& Alignment = FVector2D(0.5f, 0.5f))
	{
		UCanvasPanelSlot* ChildSlot = Canvas->AddChildToCanvas(Child);
		ChildSlot->SetAutoSize(true);
		ChildSlot->SetPosition(Position);
		ChildSlot->SetAlignment(Alignment);
		return Child;
	};
	auto Picture = [this](const TCHAR* Name, const FPaintedIcon& Icon)
	{
		return MakeImage(WidgetTree, PaintedIconBrush(Name, Icon, PixelsPerUnit, Icon.ViewBox));
	};
	// The slot's layers all centre on its middle, grown about it.
	const FVector2D SlotCentre = Centre + FVector2D(0.f, SlotDown);
	auto SlotLayer = [this, &Place, &SlotCentre](const FSlateBrush& Brush)
	{
		UImage* Layer = MakeImage(WidgetTree, Brush);
		Layer->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Place(Layer, SlotCentre);
		return Layer;
	};

	// The dark shade keeps the white type readable over bright sky: the banner's only background.
	UImage* Shade = MakeImage(WidgetTree, GlowBrush(ShadeSize, FLinearColor::Black.CopyWithNewOpacity(ShadeOpacity)));
	Place(Shade, Centre + FVector2D(0.f, SlotDown * 0.5f));
	MarkBackground(Shade);
	SlotGlow = Place(MakeImage(WidgetTree, GlowBrush(GlowSize, Color::SoulGold().CopyWithNewOpacity(GlowOpacity))), SlotCentre);

	// The hairlines grow out from their inner ends by the slot, the arrowheads between.
	const float ArrowIn = SlotSize * 0.5f + SlotGap;
	const float LineIn = ArrowIn + ArrowWidth + ArrowToLine;
	LeftLine = Place(Picture(TEXT("HudGunSlotLine"), LinePicture()), SlotCentre - FVector2D(LineIn - LineMargin, 0.f), FVector2D(1.f, 0.5f));
	LeftLine->SetRenderTransformPivot(FVector2D(1.f, 0.5f));
	RightLine = Place(Picture(TEXT("HudGunSlotLine"), LinePicture()), SlotCentre + FVector2D(LineIn - LineMargin, 0.f), FVector2D(0.f, 0.5f));
	RightLine->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
	const float ArrowOffset = ArrowIn - (ArrowArtWidth - ArrowWidth - 2.5f);
	LeftArrow = Place(Picture(TEXT("HudGunSlotArrowLeft"), ArrowPicture(false)), SlotCentre - FVector2D(ArrowOffset, 0.f), FVector2D(1.f, 0.5f));
	RightArrow = Place(Picture(TEXT("HudGunSlotArrowRight"), ArrowPicture(true)), SlotCentre + FVector2D(ArrowOffset, 0.f), FVector2D(0.f, 0.5f));

	// The slot, back to front as the HUD's column draws it: the dark disc (a background), the rim, the gunmetal ring, the cyan
	// hairline, then the padlock, the flashing ring and the number.
	UImage* Disc = SlotLayer(SlotBrush(EHudSlotPicture::Disc, SlotSize));
	Disc->SetColorAndOpacity(Color::ScreenBg().CopyWithNewOpacity(0.85f));
	MarkBackground(Disc);
	UImage* Rim = SlotLayer(SlotBrush(EHudSlotPicture::Rim, SlotSize));
	Rim->SetColorAndOpacity(Color::Ink());
	SlotMetal = SlotLayer(SlotBrush(EHudSlotPicture::Metal, SlotSize));
	SlotHairline = SlotLayer(SlotBrush(EHudSlotPicture::Hairline, SlotSize));
	FSlateBrush LockBrush = UHudWeaponSlotsWidget::PictureBrush(EHudSlotPicture::Lock);
	LockBrush.ImageSize = FVector2D(UHudWeaponSlotsWidget::LockWidth, UHudWeaponSlotsWidget::LockHeight) * SlotScale;
	SlotLock = SlotLayer(LockBrush);
	UImage* Ring = SlotLayer(SlotBrush(EHudSlotPicture::AccentRing, SlotSize));
	Ring->SetColorAndOpacity(Color::SoulGold());
	SlotRing = Ring;
	SlotNumber = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	SlotNumber->SetFont(OutlinedFont(NumberSize, 0, NumberOutline));
	SlotNumber->SetColorAndOpacity(FSlateColor(Color::SlateText()));
	SlotNumber->SetJustification(ETextJustify::Center);
	SlotNumber->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	// Chakra Petch's digits sit in the middle of their line, so centring the line centres the digit in the disc.
	Place(SlotNumber, SlotCentre);

	UTextBlock* Title = MakeFloatingText(WidgetTree, TitleSize, Color::SlateText(), TitleSpacing, ETextJustify::Center);
	Title->SetText(NSLOCTEXT("LooterHUD", "NewGunSlot", "NEW GUN SLOT"));
	TitleText = Place(Title, Centre + FVector2D(0.f, TitleDown));
	// The bar draws out from its middle, last of the lines.
	GoldBar = Place(Picture(TEXT("HudGunSlotBar"), BarPicture()), Centre + FVector2D(0.f, BarDown));

	RewardText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	RewardText->SetFont(OutlinedFont(RewardSize, RewardSpacing, RewardOutline));
	RewardText->SetColorAndOpacity(FSlateColor(Color::SlateText()));
	RewardText->SetJustification(ETextJustify::Center);
	Place(RewardText, Centre + FVector2D(0.f, RewardDown));
	CauseText = MakeFloatingText(WidgetTree, CauseSize, Color::SlateLabel(), CauseSpacing, ETextJustify::Center);
	Place(CauseText, Centre + FVector2D(0.f, CauseDown));

	USizeBox* Root = MakeSized(WidgetTree, Canvas, Width, Height);
	WidgetTree->RootWidget = Root;
	Banner = Root;
}

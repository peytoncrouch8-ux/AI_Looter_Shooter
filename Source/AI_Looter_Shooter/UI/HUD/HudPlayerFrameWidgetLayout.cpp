// UHudPlayerFrameWidget's widget tree: every picture, stretch and number in one canvas, back to front, placed by the
// spec's 1080p numbers. The pictures are drawn in HudPlayerFrameWidgetPictures.cpp, the bars' stretches painted in
// HudPlayerFrameWidgetStretches.cpp; health is in HudPlayerFrameWidget.cpp, experience in HudPlayerFrameWidgetXP.cpp.

#include "UI/HUD/HudPlayerFrameWidget.h"
#include "UI/HUD/HudPortraitWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

using namespace LooterUI;

namespace
{
	// Spots on the 1080p screen (the mockup's, scaled with the frame), and type sizes.

	/** The health number, centred on the track: the health in white, then "/ 100" smaller and dimmer. */
	constexpr float HealthNumberX = 376.f;
	constexpr float HealthNumberY = 953.5f;
	constexpr int32 HealthValueSize = 18;
	constexpr int32 HealthMaxSize = 12;
	constexpr float HealthMaxGap = 3.4f;
	/**
	 * Lifts the smaller type so the two sit on one baseline: their line boxes are bottom-aligned, and Chakra Petch's
	 * descent grows with its size.
	 */
	constexpr float HealthMaxLift = 2.f;
	/** The heal's "+30", near the bar's end (its middle at this height when it starts to rise). */
	constexpr float HealFloatX = 547.f;
	constexpr float HealFloatY = 946.5f;
	constexpr int32 HealFloatSize = 15;
	/** "1,240 / 2,000 XP" after the experience bar, from x 518 (its middle at this height), and "+160 XP" over it. */
	constexpr float XPNumbersX = 518.f;
	constexpr float XPNumbersY = 997.f;
	constexpr int32 XPValueSize = 13;
	constexpr int32 XPMaxSize = 10;
	constexpr float XPMaxGap = 2.5f;
	constexpr float XPMaxLift = 1.f;
	constexpr float XPFloatY = 978.f;
	constexpr int32 XPFloatSize = 12;
	/** The level in the gem, in ink. */
	constexpr int32 LevelSize = 15;

	/** Two texts on one line, the second smaller, sharing a baseline. */
	UHorizontalBox* MakeNumberPair(UWidgetTree* Tree, UTextBlock* Big, UTextBlock* Small, float Gap, float Lift)
	{
		UHorizontalBox* Pair = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Pair->AddChildToHorizontalBox(Big)->SetVerticalAlignment(VAlign_Bottom);
		UHorizontalBoxSlot* SmallSlot = Pair->AddChildToHorizontalBox(Small);
		SmallSlot->SetVerticalAlignment(VAlign_Bottom);
		SmallSlot->SetPadding(FMargin(Gap, 0.f, 0.f, Lift));
		return Pair;
	}
}

TSharedRef<SWidget> UHudPlayerFrameWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		Canvas->SetVisibility(ESlateVisibility::HitTestInvisible);
		// Each child at its own size, by the spot Alignment marks on it (its top-left by default).
		auto Place = [Canvas](UWidget* Child, const FVector2D& Position, const FVector2D& Alignment = FVector2D::ZeroVector)
		{
			UCanvasPanelSlot* ChildSlot = Canvas->AddChildToCanvas(Child);
			ChildSlot->SetAutoSize(true);
			ChildSlot->SetPosition(Position);
			ChildSlot->SetAlignment(Alignment);
			return ChildSlot;
		};
		auto AddPicture = [this, &Place](EHudFramePicture Picture)
		{
			UImage* Image = MakeImage(WidgetTree, PictureBrush(Picture));
			Place(Image, PicturePosition(Picture));
			return Image;
		};

		StretchBodies.Reset();
		StretchCaps.Reset();
		StretchBrushes.Reset();
		StretchShown.Reset();

		// The health bar, its left end tucked behind the medallion. Back to front: its metal, the track (a background), the
		// chip, the fill and its low-health beat, the heal's shine, then the quarter cuts and the clamp over them.
		AddPicture(EHudFramePicture::HealthBack);
		MarkBackground(AddPicture(EHudFramePicture::HealthTrack));
		AddStretch(Canvas, EHudFrameStretch::HealthChip, Color::HealthChip());
		AddStretch(Canvas, EHudFrameStretch::HealthFill, FLinearColor::White);
		AddStretch(Canvas, EHudFrameStretch::HealthBeat, Color::HealthEdge().CopyWithNewOpacity(0.f));
		ShineBrush = PictureBrush(EHudFramePicture::HealthShine);
		Shine = AddPicture(EHudFramePicture::HealthShine);
		Shine->SetVisibility(ESlateVisibility::Hidden);
		bShineShown = false;
		AddPicture(EHudFramePicture::HealthFront);

		HealthValue = MakeFloatingText(WidgetTree, HealthValueSize, FLinearColor::White);
		HealthMax = MakeFloatingText(WidgetTree, HealthMaxSize, Color::NumberDim());
		Place(MakeNumberPair(WidgetTree, HealthValue, HealthMax, HealthMaxGap, HealthMaxLift), InFrame(HealthNumberX, HealthNumberY),
			FVector2D(0.5f, 0.5f));
		HealFloat = MakeFloatingText(WidgetTree, HealFloatSize, Color::Heal());
		HealFloat->SetVisibility(ESlateVisibility::Hidden);
		Place(HealFloat, InFrame(HealFloatX, HealFloatY), FVector2D(0.f, 0.5f));

		// The experience bar under it. Back to front: the ink edge, the track and the empty sections (a background), the
		// stretch still to catch up to, the fill, the glow over what was just earned, and the fill as it was before it.
		AddPicture(EHudFramePicture::XPEdge);
		MarkBackground(AddPicture(EHudFramePicture::XPTrack));
		AddStretch(Canvas, EHudFrameStretch::XPGain, FLinearColor::White);
		AddStretch(Canvas, EHudFrameStretch::XPFill, FLinearColor::White);
		AddStretch(Canvas, EHudFrameStretch::XPGlow, FLinearColor::White);
		AddStretch(Canvas, EHudFrameStretch::XPBefore, FLinearColor::White);

		XPValue = MakeFloatingText(WidgetTree, XPValueSize, Color::Text());
		XPMax = MakeFloatingText(WidgetTree, XPMaxSize, Color::TextDim());
		Place(MakeNumberPair(WidgetTree, XPValue, XPMax, XPMaxGap, XPMaxLift), InFrame(XPNumbersX, XPNumbersY), FVector2D(0.f, 0.5f));
		XPFloat = MakeFloatingText(WidgetTree, XPFloatSize, Color::CyanText());
		XPFloat->SetVisibility(ESlateVisibility::Hidden);
		Place(XPFloat, InFrame(XPNumbersX, XPFloatY), FVector2D(0.f, 0.5f));

		// The medallion over the bars' ends: the horn and bezel, the portrait in the window, then the window's edges and
		// the clamps over the portrait's rim.
		AddPicture(EHudFramePicture::MedallionBack);
		Portrait = WidgetTree->ConstructWidget<UHudPortraitWidget>(UHudPortraitWidget::StaticClass());
		UCanvasPanelSlot* PortraitSlot = Canvas->AddChildToCanvas(Portrait);
		PortraitSlot->SetAutoSize(false);
		PortraitSlot->SetSize(FVector2D(UHudPortraitWidget::WindowSize));
		PortraitSlot->SetPosition(InFrame(MedallionX, MedallionY) - FVector2D(UHudPortraitWidget::WindowSize * 0.5f));
		AddPicture(EHudFramePicture::MedallionFront);

		// The level gem on the medallion's lower-right edge: the ring that spreads behind it on a level-up, the gem, its
		// flash and the level in ink (no outline: it's on the gem, not floating over the world).
		GemRing = AddPicture(EHudFramePicture::GemRing);
		GemRing->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		GemRing->SetVisibility(ESlateVisibility::Hidden);
		AddPicture(EHudFramePicture::Gem);
		GemFlash = AddPicture(EHudFramePicture::GemFlash);
		GemFlash->SetVisibility(ESlateVisibility::Hidden);
		LevelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		LevelText->SetFont(Font(LevelSize));
		LevelText->SetColorAndOpacity(FSlateColor(Color::Ink()));
		LevelText->SetJustification(ETextJustify::Center);
		// Chakra Petch's digits sit in the middle of their line, so centring the line centres them in the gem.
		Place(LevelText, InFrame(GemX, GemY), FVector2D(0.5f, 0.5f));

		USizeBox* Root = MakeSized(WidgetTree, Canvas, Width, Height);
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Root;
		Cluster = Canvas;

		// Everything shows afresh: the next health and progress are taken as they are.
		LastHealth = -1.f;
		LastFraction = -1.f;
		ShownPoints = INDEX_NONE;
		ShownMaxPoints = INDEX_NONE;
		bShownLow = false;
		bBeatShown = false;
		ShownLevel = INDEX_NONE;
		ShownProgress = -1.0;
		GainStart = -1.f;
		GainAlpha = 0.f;
	}
	return Super::RebuildWidget();
}

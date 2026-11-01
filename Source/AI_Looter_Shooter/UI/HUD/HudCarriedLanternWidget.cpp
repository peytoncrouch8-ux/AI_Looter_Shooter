// UHudCarriedLanternWidget: Abel's light on the HUD while it's carried home (the lantern, its label, its slanted bar).

#include "UI/HUD/HudCarriedLanternWidget.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

using namespace LooterUI;

namespace
{
	/** The cluster, bottom middle: its foot this far over the screen's bottom edge, clear of the hint line under it. */
	constexpr float LanternClusterBottom = 118.f;
	constexpr float LanternClusterWidth = UHudCarriedLanternWidget::BarWidth + 64.f;
	constexpr float LanternClusterHeight = 54.f;
	/** The lantern picture (24 x 32 view box, drawn 2x), left of the label and bar. */
	constexpr float LanternIconWidth = 30.f;
	constexpr float LanternIconHeight = 40.f;
	constexpr float LanternPixels = 2.f;
	/** The label over the bar, the bar under it: 220 x 10, leaning as every HUD bar does. */
	constexpr float LanternBarLeft = 46.f;
	constexpr float LanternLabelTop = 14.f;
	constexpr float LanternBarTop = 22.f;
	constexpr float LanternBarHeight = 10.f;
	constexpr float LanternBarSlant = -16.f;
	constexpr float LanternBarInset = 2.f;
	constexpr int32 LanternLabelSize = 14;
	constexpr int32 LanternLabelSpacing = 220;
	/** Fades (s), the bar's ease after the light (a second's share), the drain's pulse (Hz), the edge's glint (s). */
	constexpr float LanternFadeSeconds = 0.35f;
	constexpr float LanternEase = 6.f;
	constexpr float LanternPulseHz = 2.2f;
	constexpr float LanternGlintSeconds = 0.6f;

	TArray<FVector2D> LanternRect(float Left, float Top, float Right, float Bottom)
	{
		return { FVector2D(Left, Top), FVector2D(Right, Top), FVector2D(Right, Bottom), FVector2D(Left, Bottom) };
	}

	/** The keeper's lantern's ironwork: its bail, cap, the frame's posts round the glass, its foot; inked edges. */
	const FPaintedIcon& LanternFramePicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(24.f, 32.f);
			FPaintLayer Ink;
			Ink.Color = Color::Ink();
			Ink.StrokeWidth = 2.6f;
			// The bail's ring over the cap, the cap, the posts and the foot, outlined thick so it reads over any ground.
			Ink.Strokes.Add({ FVector2D(8.f, 6.f), FVector2D(8.f, 2.5f), FVector2D(12.f, 1.f), FVector2D(16.f, 2.5f), FVector2D(16.f, 6.f) });
			Ink.Fills.Add({ FVector2D(5.f, 9.f), FVector2D(9.f, 5.f), FVector2D(15.f, 5.f), FVector2D(19.f, 9.f) });
			Ink.Fills.Add(LanternRect(4.f, 9.f, 6.6f, 26.f));
			Ink.Fills.Add(LanternRect(17.4f, 9.f, 20.f, 26.f));
			Ink.Fills.Add({ FVector2D(3.f, 26.f), FVector2D(21.f, 26.f), FVector2D(19.f, 30.f), FVector2D(5.f, 30.f) });
			Result.Layers.Add(Ink);
			FPaintLayer Metal;
			Metal.Color = Color::MetalHi();
			Metal.GradientTo = Color::MetalLow();
			Metal.GradientStart = FVector2D(12.f, 4.f);
			Metal.GradientEnd = FVector2D(12.f, 30.f);
			Metal.StrokeWidth = 1.2f;
			Metal.Strokes.Add({ FVector2D(8.f, 6.f), FVector2D(8.f, 2.5f), FVector2D(12.f, 1.f), FVector2D(16.f, 2.5f), FVector2D(16.f, 6.f) });
			Metal.Fills.Add({ FVector2D(6.f, 8.6f), FVector2D(9.5f, 5.8f), FVector2D(14.5f, 5.8f), FVector2D(18.f, 8.6f) });
			Metal.Fills.Add(LanternRect(4.6f, 9.f, 6.f, 25.6f));
			Metal.Fills.Add(LanternRect(18.f, 9.f, 19.4f, 25.6f));
			Metal.Fills.Add({ FVector2D(3.8f, 26.4f), FVector2D(20.2f, 26.4f), FVector2D(18.6f, 29.4f), FVector2D(5.4f, 29.4f) });
			Result.Layers.Add(Metal);
			return Result;
		}();
		return Picture;
	}

	/** The glass between the posts, white to be tinted by his light. */
	const FPaintedIcon& LanternGlassPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(24.f, 32.f);
			FPaintLayer Glass;
			Glass.Color = FLinearColor::White;
			Glass.Fills.Add(LanternRect(6.6f, 9.4f, 17.4f, 25.8f));
			Result.Layers.Add(Glass);
			FPaintLayer Flame;
			Flame.Color = FLinearColor::White;
			Flame.Opacity = 1.f;
			Flame.Fills.Add({ FVector2D(12.f, 12.f), FVector2D(14.6f, 18.f), FVector2D(12.f, 22.5f), FVector2D(9.4f, 18.f) });
			Result.Layers.Add(Flame);
			return Result;
		}();
		return Picture;
	}

	/** The gold of his light at Light (dimmed toward the track's grey as it drains), pulsing while drained. */
	FLinearColor LightColor(float Light, bool bDraining, float Pulse)
	{
		const float Strength = FMath::Clamp(0.35f + 0.65f * Light, 0.f, 1.f) * (bDraining ? 0.75f + 0.25f * Pulse : 1.f);
		return FMath::Lerp(Color::TextDim(), Color::SoulGold(), Strength);
	}
}

TSharedRef<SWidget> UHudCarriedLanternWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Root;

		Cluster = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		UCanvasPanelSlot* ClusterSlot = Root->AddChildToCanvas(Cluster);
		ClusterSlot->SetAnchors(FAnchors(0.5f, 1.f));
		ClusterSlot->SetAlignment(FVector2D(0.5f, 1.f));
		ClusterSlot->SetPosition(FVector2D(0.f, -LanternClusterBottom));
		ClusterSlot->SetSize(FVector2D(LanternClusterWidth, LanternClusterHeight));
		auto Add = [this](UWidget* Widget, const FVector2D& Position, const FVector2D& Size)
		{
			UCanvasPanelSlot* WidgetSlot = Cluster->AddChildToCanvas(Widget);
			WidgetSlot->SetPosition(Position);
			WidgetSlot->SetAutoSize(Size.IsZero());
			if (!Size.IsZero())
			{
				WidgetSlot->SetSize(Size);
			}
		};

		// The lantern: a soft glow behind it, the glass tinted by his light, the ironwork over both.
		const FVector2D IconSize(LanternIconWidth, LanternIconHeight);
		GlassGlow = MakeImage(WidgetTree, GlowBrush(IconSize * 2.f, FLinearColor::White));
		Add(GlassGlow, FVector2D(-LanternIconWidth * 0.5f, -LanternIconHeight * 0.5f + 6.f), IconSize * 2.f);
		LanternGlass = MakeImage(WidgetTree, PaintedIconBrush(TEXT("CarriedLanternGlass"), LanternGlassPicture(), LanternPixels, IconSize));
		Add(LanternGlass, FVector2D(0.f, 6.f), IconSize);
		Add(MakeImage(WidgetTree, PaintedIconBrush(TEXT("CarriedLanternFrame"), LanternFramePicture(), LanternPixels, IconSize)),
			FVector2D(0.f, 6.f), IconSize);

		// "ABEL'S LIGHT" floating over the bar's left end.
		Label = MakeFloatingText(WidgetTree, LanternLabelSize, Color::SoulGold(), LanternLabelSpacing, ETextJustify::Left);
		Label->SetText(NSLOCTEXT("LooterLantern", "AbelsLight", "ABEL'S LIGHT"));
		UCanvasPanelSlot* LabelSlot = Cluster->AddChildToCanvas(Label);
		LabelSlot->SetPosition(FVector2D(LanternBarLeft + 2.f, LanternLabelTop));
		LabelSlot->SetAlignment(FVector2D(0.f, 1.f));
		LabelSlot->SetAutoSize(true);

		// The bar: an inked rim, the dark track (the one background), the gold fill cut to length, its bright leading edge.
		UOverlay* Bar = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UImage* Rim = MakeImage(WidgetTree, RectBrush(Color::Ink(), Color::BarMetalLow(), 1.f));
		FillOverlaySlot(Bar->AddChildToOverlay(Rim));
		UImage* Track = MakeImage(WidgetTree, RectBrush(Color::Track().CopyWithNewOpacity(0.88f)));
		MarkBackground(Track);
		FillOverlaySlot(Bar->AddChildToOverlay(Track), FMargin(LanternBarInset));
		UOverlay* Lit = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		Fill = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
		FillOverlaySlot(Lit->AddChildToOverlay(Fill));
		FillEdge = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
		UOverlaySlot* EdgeSlot = Lit->AddChildToOverlay(MakeSized(WidgetTree, FillEdge, 2.f));
		EdgeSlot->SetHorizontalAlignment(HAlign_Right);
		EdgeSlot->SetVerticalAlignment(VAlign_Fill);
		FillBox = MakeSized(WidgetTree, Lit, BarWidth - LanternBarInset * 2.f, LanternBarHeight - LanternBarInset * 2.f);
		UOverlaySlot* FillSlot = Bar->AddChildToOverlay(FillBox);
		FillSlot->SetHorizontalAlignment(HAlign_Left);
		FillSlot->SetVerticalAlignment(VAlign_Fill);
		FillSlot->SetPadding(FMargin(LanternBarInset));
		USizeBox* BarSize = MakeSized(WidgetTree, Bar, BarWidth, LanternBarHeight);
		BarSize->SetRenderShear(FVector2D(LanternBarSlant, 0.f));
		Add(BarSize, FVector2D(LanternBarLeft, LanternBarTop), FVector2D(BarWidth, LanternBarHeight));

		Cluster->SetRenderOpacity(Opacity);
		Cluster->SetVisibility(Opacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		LastShownWidth = -1.f;
		Apply(0.f);
	}
	return Super::RebuildWidget();
}

void UHudCarriedLanternWidget::SetShown(bool bInShown)
{
	bWanted = bInShown;
}

void UHudCarriedLanternWidget::SetLight(float InLight, bool bInDraining, float Clock)
{
	// Filling again past where it was: the edge glints (the light won back).
	if (InLight > Light + 0.002f && !bInDraining)
	{
		Glint = FMath::Max(Glint, 0.4f);
	}
	if (InLight >= 0.999f && Light < 0.999f)
	{
		Glint = 1.f;
	}
	Light = FMath::Clamp(InLight, 0.f, 1.f);
	bDraining = bInDraining;
	PulseClock = Clock;
}

bool UHudCarriedLanternWidget::IsMenuOpen() const
{
	const APlayerController* Player = GetOwningPlayer();
	const ALooterHUD* LooterHUD = Player ? Cast<ALooterHUD>(Player->GetHUD()) : nullptr;
	const UWorld* World = GetWorld();
	return (LooterHUD && LooterHUD->IsMenuOpen()) || (World && World->IsPaused());
}

void UHudCarriedLanternWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const float Target = bWanted && !IsMenuOpen() ? 1.f : 0.f;
	if (Opacity != Target && Cluster)
	{
		Opacity = FMath::FInterpConstantTo(Opacity, Target, InDeltaTime, 1.f / LanternFadeSeconds);
		Cluster->SetRenderOpacity(Opacity);
		Cluster->SetVisibility(Opacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (Opacity <= 0.f)
	{
		return;
	}
	Shown = FMath::FInterpTo(Shown, Light, InDeltaTime, LanternEase);
	Glint = FMath::Max(0.f, Glint - InDeltaTime / LanternGlintSeconds);
	Apply(PulseClock);
}

void UHudCarriedLanternWidget::Apply(float Clock)
{
	if (!FillBox || !Fill || !FillEdge || !LanternGlass || !GlassGlow || !Label)
	{
		return;
	}
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * UE_TWO_PI * LanternPulseHz);
	const FLinearColor Gold = LightColor(Shown, bDraining, Pulse);
	// In half pixels, so it only repaints when its end visibly moves.
	const float Width = FMath::RoundToFloat((BarWidth - LanternBarInset * 2.f) * FMath::Clamp(Shown, 0.f, 1.f) * 2.f) * 0.5f;
	if (Width != LastShownWidth)
	{
		LastShownWidth = Width;
		FillBox->SetWidthOverride(Width);
	}
	Fill->SetColorAndOpacity(Gold);
	FillEdge->SetColorAndOpacity(FMath::Lerp(Gold, FLinearColor::White, 0.55f + 0.45f * Glint));
	LanternGlass->SetColorAndOpacity(Gold);
	GlassGlow->SetColorAndOpacity(Gold.CopyWithNewOpacity(0.25f + 0.45f * Shown * (bDraining ? Pulse : 1.f)));
	// Drained, the label says so in the hose's cyan; free, it's his name for the light again.
	if (bDraining != bLabelDraining)
	{
		bLabelDraining = bDraining;
		Label->SetText(bDraining ? NSLOCTEXT("LooterLantern", "Draining", "ABEL'S LIGHT  -  DRAINING")
			: NSLOCTEXT("LooterLantern", "AbelsLight", "ABEL'S LIGHT"));
	}
	Label->SetColorAndOpacity(FSlateColor(bDraining ? FMath::Lerp(Color::CyanText(), Color::SoulGold(), Pulse * 0.3f) : Color::SoulGold()));
}

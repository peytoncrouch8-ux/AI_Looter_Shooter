#include "UI/HUD/HudScreenEdgeWidget.h"
#include "UI/HUD/HudPlayerFrameWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Combat/HealthComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

using namespace LooterUI;

namespace
{
	/** A strip's picture runs this far from its edge inward, and this far along the edge (it's stretched to the screen). */
	constexpr float EdgeRamp = 64.f;
	constexpr float EdgeAlong = 4.f;
	/** How far each strip reaches in: this share of the screen's width (the sides) or height (the top and bottom). */
	constexpr float EdgeDepth = 0.11f;
	/**
	 * The red's fall from the edge: a faint ramp over the whole strip under a stronger one over its outer part, so it
	 * drops quickly and then tails off, like a vignette. Where two strips meet the corners come out deeper.
	 */
	constexpr float EdgeWideAlpha = 0.4f;
	constexpr float EdgeNearAlpha = 0.6f;
	constexpr float EdgeNearShare = 0.4f;

	/** A hit's flash fades over this long (from EdgeHitPeak); the low-health pulse swings between these on the beat. */
	constexpr float EdgeHitSeconds = 0.55f;
	constexpr float EdgeHitPeak = 0.85f;
	constexpr float EdgeLowLow = 0.25f;
	constexpr float EdgeLowHigh = 0.6f;
	/** Scales those (the mockup's) to the strips, so a hit's peak tints the edges about a quarter red. */
	constexpr float EdgeStrength = 0.39f;
	/** Health falls by more than this (in points) for a hit. */
	constexpr float EdgeHitStep = 0.01f;

	/**
	 * A strip's picture: the red fading from its edge inward. bAcrossX: the fade runs along x (the sides), else along y;
	 * bEdgeAtEnd: the edge is at the picture's far end (the right side, the bottom).
	 */
	FPaintedIcon EdgeStrip(bool bAcrossX, bool bEdgeAtEnd)
	{
		FPaintedIcon Result;
		Result.ViewBox = bAcrossX ? FVector2D(EdgeRamp, EdgeAlong) : FVector2D(EdgeAlong, EdgeRamp);
		// A point Distance in from the edge on the fade's axis, and the band from the edge to Depth in.
		auto Inward = [bAcrossX, bEdgeAtEnd](float Distance)
		{
			const float At = bEdgeAtEnd ? EdgeRamp - Distance : Distance;
			return bAcrossX ? FVector2D(At, 0.f) : FVector2D(0.f, At);
		};
		auto Band = [bAcrossX, bEdgeAtEnd](float Depth)
		{
			const float From = bEdgeAtEnd ? EdgeRamp - Depth : 0.f;
			const float To = bEdgeAtEnd ? EdgeRamp : Depth;
			return bAcrossX
				? TArray<FVector2D>{ FVector2D(From, 0.f), FVector2D(To, 0.f), FVector2D(To, EdgeAlong), FVector2D(From, EdgeAlong) }
				: TArray<FVector2D>{ FVector2D(0.f, From), FVector2D(EdgeAlong, From), FVector2D(EdgeAlong, To), FVector2D(0.f, To) };
		};
		for (const TPair<float, float>& Ramp : { TPair<float, float>(EdgeRamp, EdgeWideAlpha), TPair<float, float>(EdgeRamp * EdgeNearShare, EdgeNearAlpha) })
		{
			FPaintLayer Layer;
			Layer.Fills.Add(Band(Ramp.Key));
			Layer.Color = Color::Hurt().CopyWithNewOpacity(Ramp.Value);
			Layer.GradientTo = Color::Hurt().CopyWithNewOpacity(0.f);
			Layer.GradientStart = Inward(0.f);
			Layer.GradientEnd = Inward(Ramp.Key);
			Result.Layers.Add(MoveTemp(Layer));
		}
		return Result;
	}

	FSlateBrush EdgeStripBrush(FName Name, const FPaintedIcon& Picture)
	{
		// Smooth ramps, stretched over hundreds of pixels: a texel a unit is plenty.
		return PaintedIconBrush(Name, Picture, 1.f, Picture.ViewBox);
	}
}

TSharedRef<SWidget> UHudScreenEdgeWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		static const FPaintedIcon Left = EdgeStrip(true, false);
		static const FPaintedIcon Right = EdgeStrip(true, true);
		static const FPaintedIcon Top = EdgeStrip(false, false);
		static const FPaintedIcon Bottom = EdgeStrip(false, true);

		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		// Each strip stretched between its anchors, whatever the screen's shape.
		auto AddStrip = [this, Canvas](FName Name, const FPaintedIcon& Picture, const FAnchors& Anchors)
		{
			UCanvasPanelSlot* StripSlot = Canvas->AddChildToCanvas(MakeImage(WidgetTree, EdgeStripBrush(Name, Picture)));
			StripSlot->SetAnchors(Anchors);
			StripSlot->SetOffsets(FMargin(0.f));
		};
		AddStrip(TEXT("HudEdgeLeft"), Left, FAnchors(0.f, 0.f, EdgeDepth, 1.f));
		AddStrip(TEXT("HudEdgeRight"), Right, FAnchors(1.f - EdgeDepth, 0.f, 1.f, 1.f));
		AddStrip(TEXT("HudEdgeTop"), Top, FAnchors(0.f, 0.f, 1.f, EdgeDepth));
		AddStrip(TEXT("HudEdgeBottom"), Bottom, FAnchors(0.f, 1.f - EdgeDepth, 1.f, 1.f));

		Canvas->SetVisibility(ESlateVisibility::Collapsed);
		WidgetTree->RootWidget = Canvas;
		Edges = Canvas;
		ShownOpacity = 0.f;
	}
	return Super::RebuildWidget();
}

void UHudScreenEdgeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// It covers the whole screen: it must never take a click.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UHudScreenEdgeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!Edges)
	{
		return;
	}

	const APlayerController* Player = GetOwningPlayer();
	APawn* Pawn = Player ? Player->GetPawn() : nullptr;
	if (Pawn != WatchedPawn.Get())
	{
		// A new pawn (a respawn, or none): its health as it is, no flash.
		WatchedPawn = Pawn;
		WatchedHealth = Pawn ? Pawn->FindComponentByClass<UHealthComponent>() : nullptr;
		LastHealth = -1.f;
	}

	bool bLow = false;
	if (const UHealthComponent* Health = WatchedHealth.Get())
	{
		const float Points = Health->GetHealth();
		if (LastHealth >= 0.f && Points < LastHealth - EdgeHitStep)
		{
			FlashTime = EdgeHitSeconds;
		}
		LastHealth = Points;
		bLow = !Health->IsDead() && Health->GetHealthPercent() <= UHudPlayerFrameWidget::LowFraction;
	}

	// A hit flashes and fades quickly; low health pulses on the frame's beat. The stronger of the two shows.
	float Strength = 0.f;
	if (FlashTime > 0.f)
	{
		Strength = EdgeHitPeak * (1.f - FMath::InterpEaseOut(0.f, 1.f, 1.f - FlashTime / EdgeHitSeconds, 2.f));
		FlashTime = FMath::Max(0.f, FlashTime - InDeltaTime);
	}
	if (bLow)
	{
		Strength = FMath::Max(Strength, FMath::Lerp(EdgeLowLow, EdgeLowHigh, UHudPlayerFrameWidget::LowBeat(GetWorld())));
	}

	const float Opacity = Strength * EdgeStrength;
	if (Opacity == ShownOpacity)
	{
		return;
	}
	ShownOpacity = Opacity;
	if (Opacity <= 0.001f)
	{
		Edges->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	Edges->SetRenderOpacity(Opacity);
	if (Edges->GetVisibility() != ESlateVisibility::HitTestInvisible)
	{
		Edges->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

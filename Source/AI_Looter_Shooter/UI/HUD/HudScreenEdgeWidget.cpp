#include "UI/HUD/HudScreenEdgeWidget.h"
#include "UI/HUD/HudPlayerFrameWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/HUD/HudWardHit.h"
#include "Combat/HealthComponent.h"
#include "Player/PlayerRattleSubsystem.h"
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
	// The ward's cyan is a light colour over a warm, bright world: at the red's strength it all but vanished (play-test
	// 2026-10-22), so its layer shows at twice that.
	constexpr float WardEdgeStrength = 0.8f;
	// A scream's frost (UPlayerRattleSubsystem): the pale salt line's colour, light over a bright world like the ward's cyan,
	// so it shows at about its strength.
	constexpr float FrostEdgeStrength = 0.75f;

	/**
	 * A strip's picture: Tint (the red, or the ward's cyan) fading from its edge inward. bAcrossX: the fade runs along x (the
	 * sides), else along y; bEdgeAtEnd: the edge is at the picture's far end (the right side, the bottom).
	 */
	FPaintedIcon EdgeStrip(bool bAcrossX, bool bEdgeAtEnd, const FLinearColor& Tint)
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
			Layer.Color = Tint.CopyWithNewOpacity(Ramp.Value);
			Layer.GradientTo = Tint.CopyWithNewOpacity(0.f);
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

	/** Fades Layer to Opacity, collapsing it when it's gone (it costs nothing so); Shown is what it shows now. */
	void ShowEdgeLayer(UWidget* Layer, float Opacity, float& Shown)
	{
		if (!Layer || Opacity == Shown)
		{
			return;
		}
		Shown = Opacity;
		if (Opacity <= 0.001f)
		{
			Layer->SetVisibility(ESlateVisibility::Collapsed);
			return;
		}
		Layer->SetRenderOpacity(Opacity);
		if (Layer->GetVisibility() != ESlateVisibility::HitTestInvisible)
		{
			Layer->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
}

TSharedRef<SWidget> UHudScreenEdgeWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		static const FPaintedIcon Left = EdgeStrip(true, false, Color::Hurt());
		static const FPaintedIcon Right = EdgeStrip(true, true, Color::Hurt());
		static const FPaintedIcon Top = EdgeStrip(false, false, Color::Hurt());
		static const FPaintedIcon Bottom = EdgeStrip(false, true, Color::Hurt());
		// The ward's cyan: the ward bar's own light half.
		static const FPaintedIcon WardLeft = EdgeStrip(true, false, Color::XPLight());
		static const FPaintedIcon WardRight = EdgeStrip(true, true, Color::XPLight());
		static const FPaintedIcon WardTop = EdgeStrip(false, false, Color::XPLight());
		static const FPaintedIcon WardBottom = EdgeStrip(false, true, Color::XPLight());
		// The rattle's frost: the grave-salt line's pale colour.
		static const FPaintedIcon FrostLeft = EdgeStrip(true, false, Color::SaltLine());
		static const FPaintedIcon FrostRight = EdgeStrip(true, true, Color::SaltLine());
		static const FPaintedIcon FrostTop = EdgeStrip(false, false, Color::SaltLine());
		static const FPaintedIcon FrostBottom = EdgeStrip(false, true, Color::SaltLine());

		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		// A set of four strips filling the screen, each strip stretched between its anchors, whatever the screen's shape.
		auto AddSet = [this, Root](const TCHAR* Prefix, const FPaintedIcon& L, const FPaintedIcon& R, const FPaintedIcon& T, const FPaintedIcon& B)
		{
			UCanvasPanel* Set = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
			UCanvasPanelSlot* SetSlot = Root->AddChildToCanvas(Set);
			SetSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			SetSlot->SetOffsets(FMargin(0.f));
			auto AddStrip = [this, Set](const FString& Name, const FPaintedIcon& Picture, const FAnchors& Anchors)
			{
				UCanvasPanelSlot* StripSlot = Set->AddChildToCanvas(MakeImage(WidgetTree, EdgeStripBrush(FName(*Name), Picture)));
				StripSlot->SetAnchors(Anchors);
				StripSlot->SetOffsets(FMargin(0.f));
			};
			AddStrip(FString(Prefix) + TEXT("Left"), L, FAnchors(0.f, 0.f, EdgeDepth, 1.f));
			AddStrip(FString(Prefix) + TEXT("Right"), R, FAnchors(1.f - EdgeDepth, 0.f, 1.f, 1.f));
			AddStrip(FString(Prefix) + TEXT("Top"), T, FAnchors(0.f, 0.f, 1.f, EdgeDepth));
			AddStrip(FString(Prefix) + TEXT("Bottom"), B, FAnchors(0.f, 1.f - EdgeDepth, 1.f, 1.f));
			Set->SetVisibility(ESlateVisibility::Collapsed);
			return Set;
		};
		// The frost under all, then the cyan, so a breaking hit's red lies over both.
		FrostEdges = AddSet(TEXT("HudFrostEdge"), FrostLeft, FrostRight, FrostTop, FrostBottom);
		WardEdges = AddSet(TEXT("HudWardEdge"), WardLeft, WardRight, WardTop, WardBottom);
		Edges = AddSet(TEXT("HudEdge"), Left, Right, Top, Bottom);

		WidgetTree->RootWidget = Root;
		ShownOpacity = 0.f;
		ShownWardOpacity = 0.f;
		ShownFrostOpacity = 0.f;
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
	if (!Edges || !WardEdges)
	{
		return;
	}

	const APlayerController* Player = GetOwningPlayer();
	const HudWardHit::FReaction Hit = Watch.Look(Player ? Player->GetPawn() : nullptr);
	const HudWardHit::FTuning Tuning = HudWardHit::FTuning::Live();
	if (Hit.bWound)
	{
		FlashTime = EdgeHitSeconds;
	}
	if (Hit.bWard)
	{
		WardFlashTime = Tuning.EdgeSeconds;
		WardFlashLength = Tuning.EdgeSeconds;
	}
	bool bLow = false;
	if (const UHealthComponent* Health = Watch.GetHealth())
	{
		bLow = !Health->IsDead() && Health->GetHealthPercent() <= UHudPlayerFrameWidget::LowFraction;
	}

	// A wound flashes and fades quickly; low health pulses on the frame's beat. The stronger of the two shows.
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
	// A ward hit: the same fall, lighter and quicker.
	float WardStrength = 0.f;
	if (WardFlashTime > 0.f)
	{
		WardStrength = Tuning.EdgePeak * (1.f - FMath::InterpEaseOut(0.f, 1.f, 1.f - WardFlashTime / WardFlashLength, 2.f));
		WardFlashTime = FMath::Max(0.f, WardFlashTime - InDeltaTime);
	}

	ShowEdgeLayer(Edges, Strength * EdgeStrength, ShownOpacity);
	ShowEdgeLayer(WardEdges, WardStrength * WardEdgeStrength, ShownWardOpacity);

	// A scream's frost creeping in at the edges and thawing (the rattle's own envelope).
	float Frost = 0.f;
	FVector Source;
	float Age = 0.f;
	const UPlayerRattleSubsystem* Rattles = UPlayerRattleSubsystem::Get(this);
	if (Rattles && !Rattles->GetRattle(Player ? Player->GetPawn() : nullptr, Frost, Source, Age))
	{
		Frost = 0.f;
	}
	ShowEdgeLayer(FrostEdges, Frost * FrostEdgeStrength, ShownFrostOpacity);
}

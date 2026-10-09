#include "UI/HUD/HudDamageIndicatorWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Combat/HealthComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

using namespace LooterUI;

namespace
{
	/** The arc's picture: a band of the ring 12 px thick over 30 degrees, a short point at its middle, in a 112 x 36 box. */
	constexpr float ArcWidth = 112.f;
	constexpr float ArcHeight = 36.f;
	constexpr float ArcHalfSpan = 15.f;
	constexpr float ArcThickness = 12.f;
	constexpr float TipHeight = 8.f;
	constexpr float TipHalfAngle = 2.4f;

	/** It comes in over this long, holds, then fades out by ShowSeconds; a weak hit shows at this share of full. */
	constexpr float FadeInSeconds = 0.06f;
	constexpr float HoldSeconds = 0.5f;
	constexpr float WeakOpacity = 0.6f;
	/** A hit for this share of the bar (or more) is the boldest arc. */
	constexpr float FullShare = 0.25f;

	/** A point on the ring Radius from its middle at Degrees (0 up, clockwise), in the arc picture's own box. */
	FVector2D RingPoint(float Degrees, float Radius)
	{
		const FVector2D Middle(ArcWidth * 0.5f, ArcHeight * 0.5f + UHudDamageIndicatorWidget::RingRadius);
		const float Radians = FMath::DegreesToRadians(Degrees);
		return Middle + FVector2D(FMath::Sin(Radians), -FMath::Cos(Radians)) * Radius;
	}

	/** The arc's outline: the outer edge with its point, then the inner edge back. */
	TArray<FVector2D> ArcOutline()
	{
		const float Outer = UHudDamageIndicatorWidget::RingRadius + ArcThickness * 0.5f;
		const float Inner = UHudDamageIndicatorWidget::RingRadius - ArcThickness * 0.5f;
		// Every 3 degrees along each edge, the point between -3 and 3 (ArcHalfSpan is a whole number of steps).
		constexpr int32 Steps = static_cast<int32>(ArcHalfSpan / 3.f);
		TArray<FVector2D> Points;
		for (int32 Step = -Steps; Step <= -1; ++Step)
		{
			Points.Add(RingPoint(Step * 3.f, Outer));
		}
		Points.Add(RingPoint(-TipHalfAngle, Outer));
		Points.Add(RingPoint(0.f, Outer + TipHeight));
		Points.Add(RingPoint(TipHalfAngle, Outer));
		for (int32 Step = 1; Step <= Steps; ++Step)
		{
			Points.Add(RingPoint(Step * 3.f, Outer));
		}
		for (int32 Step = Steps; Step >= -Steps; --Step)
		{
			Points.Add(RingPoint(Step * 3.f, Inner));
		}
		return Points;
	}

	/**
	 * The arc in the HUD's metalwork style: a red band, brightest at its point and dimming toward its ends, a light rim
	 * along its outer edge, all in an ink line so it reads over sky or snow.
	 */
	const FPaintedIcon& ArcPicture()
	{
		static const FPaintedIcon Picture = []()
		{
			FPaintedIcon Arc;
			Arc.ViewBox = FVector2D(ArcWidth, ArcHeight);
			const TArray<FVector2D> Outline = ArcOutline();

			FPaintLayer Ink;
			TArray<FVector2D> Closed = Outline;
			Closed.Add(FVector2D(Outline[0]));
			Ink.Strokes.Add(Closed);
			Ink.StrokeWidth = 3.f;
			Ink.Color = Color::Ink();
			Arc.Layers.Add(MoveTemp(Ink));

			FPaintLayer Fill;
			Fill.Fills.Add(Outline);
			Fill.Color = Color::Hurt();
			Fill.GradientTo = Color::Hurt().CopyWithNewOpacity(0.4f);
			Fill.bRadialGradient = true;
			Fill.GradientStart = RingPoint(0.f, UHudDamageIndicatorWidget::RingRadius);
			Fill.GradientEnd = RingPoint(ArcHalfSpan, UHudDamageIndicatorWidget::RingRadius);
			Arc.Layers.Add(MoveTemp(Fill));

			FPaintLayer Rim;
			TArray<FVector2D> Edge;
			for (float Degrees = -ArcHalfSpan + 2.f; Degrees <= ArcHalfSpan - 1.99f; Degrees += 2.f)
			{
				Edge.Add(RingPoint(Degrees, UHudDamageIndicatorWidget::RingRadius + ArcThickness * 0.5f - 1.5f));
			}
			Rim.Strokes.Add(Edge);
			Rim.StrokeWidth = 1.4f;
			Rim.Color = Color::HealthHi();
			Arc.Layers.Add(MoveTemp(Rim));
			return Arc;
		}();
		return Picture;
	}
}

float UHudDamageIndicatorWidget::ScreenAngle(const FVector& ViewLocation, float ViewYaw, const FVector& Source)
{
	const FVector Toward = (Source - ViewLocation).GetSafeNormal2D();
	if (Toward.IsNearlyZero())
	{
		return 0.f;
	}
	const FRotator Yaw(0.f, ViewYaw, 0.f);
	const FVector Forward = Yaw.Vector();
	const FVector Right = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);
	return FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(FVector::DotProduct(Toward, Right)),
		static_cast<float>(FVector::DotProduct(Toward, Forward))));
}

TSharedRef<SWidget> UHudDamageIndicatorWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		ArcImages.Reset();
		// Twice the picture's units: crisp at 1080p and above.
		const FSlateBrush Brush = PaintedIconBrush(TEXT("HudDamageArc"), ArcPicture(), 2.f, FVector2D(ArcWidth, ArcHeight));
		for (int32 Index = 0; Index < MaxArcs; ++Index)
		{
			UImage* Image = MakeImage(WidgetTree, Brush);
			Image->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			Image->SetVisibility(ESlateVisibility::Collapsed);
			// Centred on the crosshair; NativeTick swings each out to the ring.
			UCanvasPanelSlot* ArcSlot = Canvas->AddChildToCanvas(Image);
			ArcSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			ArcSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			ArcSlot->SetSize(FVector2D(ArcWidth, ArcHeight));
			ArcImages.Add(Image);
		}
		Canvas->SetVisibility(ESlateVisibility::Collapsed);
		WidgetTree->RootWidget = Canvas;
	}
	return Super::RebuildWidget();
}

void UHudDamageIndicatorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// It covers the whole screen: it must never take a click.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UHudDamageIndicatorWidget::NativeDestruct()
{
	if (UHealthComponent* Health = WatchedHealth.Get())
	{
		Health->OnDamaged.RemoveDynamic(this, &UHudDamageIndicatorWidget::HandleDamaged);
	}
	WatchedHealth.Reset();
	WatchedPawn.Reset();
	Super::NativeDestruct();
}

void UHudDamageIndicatorWidget::WatchPawn()
{
	const APlayerController* Player = GetOwningPlayer();
	APawn* Pawn = Player ? Player->GetPawn() : nullptr;
	if (Pawn == WatchedPawn.Get())
	{
		return;
	}
	if (UHealthComponent* Old = WatchedHealth.Get())
	{
		Old->OnDamaged.RemoveDynamic(this, &UHudDamageIndicatorWidget::HandleDamaged);
	}
	WatchedPawn = Pawn;
	WatchedHealth = Pawn ? Pawn->FindComponentByClass<UHealthComponent>() : nullptr;
	if (UHealthComponent* Health = WatchedHealth.Get())
	{
		Health->OnDamaged.AddUniqueDynamic(this, &UHudDamageIndicatorWidget::HandleDamaged);
	}
	// A new body starts with nothing shown.
	for (FArc& Arc : Arcs)
	{
		Arc.Age = ShowSeconds;
	}
}

void UHudDamageIndicatorWidget::HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser)
{
	const APawn* Pawn = WatchedPawn.Get();
	const UHealthComponent* Health = WatchedHealth.Get();
	// What dealt it (a creature, the shooter of a pellet), else whoever was behind it; nothing to point at, no arc.
	AActor* Source = DamageCauser ? DamageCauser : (InstigatedBy ? InstigatedBy->GetPawn() : nullptr);
	if (!Pawn || !Health || !Source || Source == Pawn || Damage <= 0.f)
	{
		return;
	}
	// The same attacker again: its arc brightens. Otherwise a free one, or the oldest.
	FArc* Chosen = nullptr;
	for (FArc& Arc : Arcs)
	{
		if (Arc.Age < ShowSeconds && Arc.Source.Get() == Source)
		{
			Chosen = &Arc;
			break;
		}
	}
	if (!Chosen)
	{
		Chosen = &Arcs[0];
		for (FArc& Arc : Arcs)
		{
			if (Arc.Age > Chosen->Age)
			{
				Chosen = &Arc;
			}
		}
	}
	const float Strength = FMath::Clamp(Damage / FMath::Max(Health->GetMaxHealth(), 1.f) / FullShare, 0.f, 1.f);
	const bool bFresh = Chosen->Source.Get() != Source || Chosen->Age >= ShowSeconds;
	Chosen->Source = Source;
	Chosen->Where = Source->GetActorLocation();
	Chosen->Strength = bFresh ? Strength : FMath::Max(Chosen->Strength, Strength);
	// A fresh arc fades in; one already up jumps back to full.
	Chosen->Age = bFresh ? 0.f : FadeInSeconds;
}

void UHudDamageIndicatorWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	WatchPawn();
	if (!Canvas)
	{
		return;
	}

	bool bAny = false;
	for (const FArc& Arc : Arcs)
	{
		bAny |= Arc.Age < ShowSeconds;
	}
	if (bAny != bShowing)
	{
		bShowing = bAny;
		Canvas->SetVisibility(bAny ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (!bAny)
	{
		return;
	}

	// Measured from the camera, so it points true in third person too, and turns with the view as the player looks round.
	const APlayerController* Player = GetOwningPlayer();
	const APlayerCameraManager* Camera = Player ? Player->PlayerCameraManager.Get() : nullptr;
	const APawn* Pawn = WatchedPawn.Get();
	const FVector ViewLocation = Camera ? Camera->GetCameraLocation() : (Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector);
	const float ViewYaw = static_cast<float>(Camera ? Camera->GetCameraRotation().Yaw : (Player ? Player->GetControlRotation().Yaw : 0.f));

	for (int32 Index = 0; Index < MaxArcs; ++Index)
	{
		FArc& Arc = Arcs[Index];
		UImage* Image = ArcImages.IsValidIndex(Index) ? ArcImages[Index].Get() : nullptr;
		if (!Image)
		{
			continue;
		}
		if (Arc.Age >= ShowSeconds)
		{
			if (Image->GetVisibility() != ESlateVisibility::Collapsed)
			{
				Image->SetVisibility(ESlateVisibility::Collapsed);
			}
			continue;
		}
		Arc.Age += InDeltaTime;
		if (const AActor* Source = Arc.Source.Get())
		{
			Arc.Where = Source->GetActorLocation();
		}
		const float Angle = ScreenAngle(ViewLocation, ViewYaw, Arc.Where);
		const float Radians = FMath::DegreesToRadians(Angle);
		const float In = FMath::Clamp(Arc.Age / FadeInSeconds, 0.f, 1.f);
		const float Out = 1.f - FMath::Clamp((Arc.Age - FadeInSeconds - HoldSeconds) / (ShowSeconds - FadeInSeconds - HoldSeconds), 0.f, 1.f);
		const float Opacity = In * Out * FMath::Lerp(WeakOpacity, 1.f, Arc.Strength);
		// Bolder for a harder hit, and a little pop outward as it comes in.
		const float Scale = FMath::Lerp(0.9f, 1.15f, Arc.Strength) * (1.f + 0.15f * (1.f - In));

		FWidgetTransform Transform;
		Transform.Translation = FVector2D(FMath::Sin(Radians), -FMath::Cos(Radians)) * RingRadius;
		Transform.Angle = Angle;
		Transform.Scale = FVector2D(Scale);
		Image->SetRenderTransform(Transform);
		Image->SetRenderOpacity(Opacity);
		if (Image->GetVisibility() != ESlateVisibility::HitTestInvisible)
		{
			Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
}

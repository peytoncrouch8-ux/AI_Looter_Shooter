#include "Player/UnderwaterView.h"
#include "Player/PlayerLocomotionComponent.h"
#include "UI/Style/LooterProgressRing.h"
#include "UI/Style/LooterUIStyle.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Widgets/SOverlay.h"

// UUnderwaterViewComponent's breath ring: the kit's progress ring (SLooterProgressRing) under the crosshair while the breath is
// short, its eight parts emptying as the breath goes and filling as it comes back, cyan, red when nearly out. No panel behind
// it (the HUD's floating style); it never takes the mouse.

namespace
{
	/** The ring's box, its line and parts (Slate units), and how far under the crosshair it sits. */
	constexpr float RingDiameter = 34.f;
	constexpr float RingThickness = 4.f;
	constexpr int32 RingSegments = 8;
	constexpr float RingGap = 3.f;
	constexpr float RingBelowCrosshair = 72.f;
	/** Seconds it takes to show or go, and how long it stays once the breath is full again in the air. */
	constexpr float ShowSeconds = 0.2f;
	constexpr float StayFullSeconds = 0.8f;
	/** Under this much breath the ring turns red: time to come up. */
	constexpr float LowBreath = 0.3f;
	/** Full enough to count as full (the breath fills on a float). */
	constexpr float FullBreath = 0.999f;
}

void UUnderwaterViewComponent::UpdateBreathRing(float DeltaTime)
{
	const UPlayerLocomotionComponent* Moves = Locomotion.Get();
	BreathShown = Moves ? Moves->GetBreath() : 1.f;
	const bool bUnderHead = Moves && Moves->IsHeadUnderwater();
	BreathFullFor = BreathShown >= FullBreath && !bUnderHead ? BreathFullFor + DeltaTime : 0.f;
	const bool bWanted = Moves && (bUnderHead || BreathFullFor < StayFullSeconds);
	BreathShow = FMath::FInterpConstantTo(BreathShow, bWanted ? 1.f : 0.f, DeltaTime, 1.f / ShowSeconds);
	if (BreathShow > 0.f && !BreathOverlay.IsValid())
	{
		ShowBreathRing(true);
	}
	else if (BreathShow <= 0.f && BreathOverlay.IsValid())
	{
		ShowBreathRing(false);
	}
	if (BreathOverlay.IsValid())
	{
		BreathOverlay->SetRenderOpacity(BreathShow);
	}
}

void UUnderwaterViewComponent::ShowBreathRing(bool bShow)
{
	if (!bShow)
	{
		UGameViewportClient* Shown = BreathViewport.Get();
		if (Shown && BreathOverlay.IsValid())
		{
			Shown->RemoveViewportWidgetContent(BreathOverlay.ToSharedRef());
		}
		BreathOverlay.Reset();
		BreathViewport.Reset();
		return;
	}
	if (BreathOverlay.IsValid())
	{
		return;
	}
	const UWorld* World = GetWorld();
	UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;
	if (!Viewport)
	{
		return;
	}
	const TWeakObjectPtr<UUnderwaterViewComponent> Self(this);
	// Centred on the screen and pushed down by twice its offset (centred in what's left under the padding).
	BreathOverlay = SNew(SOverlay)
		.Visibility(EVisibility::HitTestInvisible)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(FMargin(0.f, RingBelowCrosshair * 2.f, 0.f, 0.f))
		[
			SNew(SLooterProgressRing)
			.Diameter(RingDiameter)
			.Thickness(RingThickness)
			.Segments(RingSegments)
			.SegmentGap(RingGap)
			.Progress_Lambda([Self]() { return Self.IsValid() ? Self->BreathShown : 1.f; })
			.FillColor_Lambda([Self]()
			{
				const float Left = Self.IsValid() ? Self->BreathShown : 1.f;
				return Left < LowBreath ? LooterUI::Color::Danger() : LooterUI::Color::CyanText();
			})
		];
	Viewport->AddViewportWidgetContent(BreathOverlay.ToSharedRef(), BreathZOrder);
	BreathViewport = Viewport;
}

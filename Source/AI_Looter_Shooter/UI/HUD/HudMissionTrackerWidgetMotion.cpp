// UHudMissionTrackerWidget: where it stands and how it moves: placed under the minimap as the player has it sized, stepping
// aside under menus, scenes, the pause and the loot card, its fade, the objective's slide-in and the count's pop. Following
// the tracked mission is HudMissionTrackerWidget.cpp's.

#include "UI/HUD/HudMissionTrackerWidget.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Scenes/SceneSubsystem.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Layout/Geometry.h"

using namespace LooterUI;

namespace
{
	/** Seconds for the whole tracker to fade in or out. */
	constexpr float TrackerFadeSeconds = 0.35f;
	/** The next objective slides in from this far right (from the screen's edge, the tracker's side). */
	constexpr float SlideDistance = 14.f;
	/** A new count pops to this size, white, and settles over PopSeconds. */
	constexpr float PopScale = 1.35f;
	/** Room kept between the tracker and the loot card before it steps aside for the card. */
	constexpr float LootCardClearance = 6.f;

	/** Fast at first, settling gently, like the mockup's CSS ease-out. */
	float TrackerEaseOut(float Share)
	{
		const float Remaining = 1.f - FMath::Clamp(Share, 0.f, 1.f);
		return 1.f - Remaining * Remaining;
	}
}

void UHudMissionTrackerWidget::UpdatePlacement()
{
	// The player's minimap size and switch, live (the settings menu changes them while the game is paused).
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UGraphicsSettingsSubsystem* Settings = LocalPlayer ? LocalPlayer->GetSubsystem<UGraphicsSettingsSubsystem>() : nullptr;
	const float WantedTop = TopFor(Settings ? Settings->GetMinimapScale() : 1.f, !Settings || Settings->IsMinimapShown());
	if (BlockSlot && !FMath::IsNearlyEqual(WantedTop, PlacedTop, 0.25f))
	{
		PlacedTop = WantedTop;
		BlockSlot->SetPosition(FVector2D(-RightInset, PlacedTop));
	}
}

bool UHudMissionTrackerWidget::IsCovered() const
{
	const APlayerController* Player = GetOwningPlayer();
	const ALooterHUD* LooterHUD = Player ? Cast<ALooterHUD>(Player->GetHUD()) : nullptr;
	const UWorld* World = GetWorld();
	return CoveredBy(LooterHUD && LooterHUD->IsMenuOpen(), USceneSubsystem::HidesGameplayHUD(this), World && World->IsPaused(),
		IsMakingWay());
}

bool UHudMissionTrackerWidget::CoveredBy(bool bMenuOpen, bool bSceneHidesHUD, bool bPaused, bool bMakingWay)
{
	// Every menu covers it, the inventory too, even on a step done there (Main 2's Ledger, the tutorial's last): over the
	// pages it sat on their cards (the 2026-10-23 play-check). The check waits: time under a menu doesn't count, so the
	// player sees it ticked as the menu closes. A scene that holds the player (the skiff ride) puts the gameplay HUD away,
	// and the tracker with it; the pause menu pauses the game; the loot card has its corner while it's up.
	return bMenuOpen || bSceneHidesHUD || bPaused || bMakingWay;
}

bool UHudMissionTrackerWidget::IsMakingWay() const
{
	const APlayerController* Player = GetOwningPlayer();
	const ALooterHUD* LooterHUD = Player ? Cast<ALooterHUD>(Player->GetHUD()) : nullptr;
	const UPlayerHUDWidget* GameplayHUD = LooterHUD ? LooterHUD->GetHUDWidget() : nullptr;
	FVector2D CardMin;
	FVector2D CardMax;
	if (!Block || !GameplayHUD || !GameplayHUD->GetLootCardBounds(CardMin, CardMax))
	{
		return false;
	}
	// Both are on the same viewport, so their desktop rectangles compare. The block's is where it was last laid out (it
	// keeps it while faded out), so the tracker comes back once the card has gone, not as soon as it has stepped aside.
	const FGeometry& BlockGeometry = Block->GetCachedGeometry();
	const FVector2D BlockSize = BlockGeometry.GetAbsoluteSize();
	if (BlockSize.X <= 0.f || BlockSize.Y <= 0.f)
	{
		return false;
	}
	const FVector2D BlockMin = BlockGeometry.GetAbsolutePosition() - FVector2D(LootCardClearance);
	const FVector2D BlockMax = BlockGeometry.GetAbsolutePosition() + BlockSize + FVector2D(LootCardClearance);
	return BlockMin.X < CardMax.X && CardMin.X < BlockMax.X && BlockMin.Y < CardMax.Y && CardMin.Y < BlockMax.Y;
}

void UHudMissionTrackerWidget::Animate(float DeltaTime, bool bVisible)
{
	const float Target = bVisible ? 1.f : 0.f;
	if (BlockOpacity != Target)
	{
		BlockOpacity = FMath::Clamp(BlockOpacity + (bVisible ? 1.f : -1.f) * DeltaTime / TrackerFadeSeconds, 0.f, 1.f);
		Block->SetRenderOpacity(BlockOpacity);
		Block->SetVisibility(BlockOpacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (SlideAge < SlideSeconds)
	{
		SlideAge = FMath::Min(SlideAge + DeltaTime, SlideSeconds);
		ApplySlide(SlideAge / SlideSeconds);
	}
	if (PopAge < PopSeconds)
	{
		PopAge = FMath::Min(PopAge + DeltaTime, PopSeconds);
		ApplyPop(PopAge / PopSeconds);
	}
	AnimateChecks(DeltaTime);
	AnimateOptionals(DeltaTime);
}

void UHudMissionTrackerWidget::StartSlide()
{
	// The first frame already shows it at the start of its slide, so the new words never flash in place.
	SlideAge = 0.f;
	ApplySlide(0.f);
}

void UHudMissionTrackerWidget::StartPop()
{
	PopAge = 0.f;
	ApplyPop(0.f);
}

void UHudMissionTrackerWidget::ApplySlide(float DoneShare)
{
	if (Objective)
	{
		const float Eased = TrackerEaseOut(DoneShare);
		Objective->SetRenderTranslation(FVector2D(SlideDistance * (1.f - Eased), 0.f));
		Objective->SetRenderOpacity(Eased);
	}
}

void UHudMissionTrackerWidget::ApplyPop(float DoneShare)
{
	if (CountText)
	{
		const float Eased = TrackerEaseOut(DoneShare);
		CountText->SetRenderScale(FVector2D(FMath::Lerp(PopScale, 1.f, Eased)));
		CountText->SetColorAndOpacity(FSlateColor(FMath::Lerp(FLinearColor::White, Color::CyanText(), Eased)));
	}
}

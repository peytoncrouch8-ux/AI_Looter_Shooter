#include "Loot/LootFanfarePopLabel.h"
#include "UI/HUD/HudLevelUpBannerWidget.h"
#include "UI/World/WeaponLabelWidget.h"
#include "Weapons/WeaponBase.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The level-up banner's gem hangs this far down the screen (widget units; PlayerHUDWidget.h), its box centred on it. */
	constexpr float LevelUpGemDown = 260.f;
	/** The room kept between a pop and the level-up banner (widget units), and from the screen's bottom when it slides under. */
	constexpr float LevelUpClear = 14.f;
	constexpr float PopScreenBottomKeep = 150.f;
	/** How fast a pop slides clear of the banner and back (per second). */
	constexpr float PopSlideRate = 12.f;
	/** A pop's size before its first measure (widget units), and the most its pop scale grows it. */
	constexpr float PopGuessWidth = 420.f;
	constexpr float PopGuessHeight = 90.f;
	constexpr float PopMaxScale = 1.25f;

	/** The level-up banner when one is on screen (its root is collapsed while it waits for a level-up), else null. */
	const UHudLevelUpBannerWidget* ShowingLevelUp(UWorld& World)
	{
		static TWeakObjectPtr<UHudLevelUpBannerWidget> Found;
		if (!Found.IsValid() || Found->GetWorld() != &World)
		{
			TArray<UUserWidget*> Widgets;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(&World, Widgets, UHudLevelUpBannerWidget::StaticClass(), false);
			Found = Widgets.IsEmpty() ? nullptr : Cast<UHudLevelUpBannerWidget>(Widgets[0]);
		}
		const UWidget* Root = Found.IsValid() ? Found->GetRootWidget() : nullptr;
		return Root && Root->GetVisibility() != ESlateVisibility::Collapsed ? Found.Get() : nullptr;
	}
}

void LootFanfarePop::QuietOwnLabel(const AWeaponBase& Weapon, bool bQuiet)
{
	const UWidgetComponent* OwnLabel = Weapon.GetLabelComponent();
	if (UWeaponLabelWidget* Own = OwnLabel ? Cast<UWeaponLabelWidget>(OwnLabel->GetUserWidgetObject()) : nullptr)
	{
		const float Wanted = bQuiet ? 0.f : 1.f;
		if (Own->GetRenderOpacity() != Wanted)
		{
			Own->SetRenderOpacity(Wanted);
		}
	}
}

void LootFanfarePop::KeepClearOfLevelUp(UWorld& World, UWidgetComponent& Widget, UWeaponLabelWidget& Label, float DeltaTime)
{
	float Wanted = 0.f;
	APlayerController* PC = World.GetFirstPlayerController();
	FVector2D Anchor;
	if (PC && ShowingLevelUp(World)
		&& UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, Widget.GetComponentLocation(), Anchor, /*bPlayerViewportRelative*/ true))
	{
		const FVector2D View = UWidgetLayoutLibrary::GetViewportSize(PC) / FMath::Max(UWidgetLayoutLibrary::GetViewportScale(PC), 0.01f);
		const FVector2D Measured = Label.GetDesiredSize();
		const double Width = (Measured.X > 1.0 ? Measured.X : PopGuessWidth) * PopMaxScale;
		const double Height = (Measured.Y > 1.0 ? Measured.Y : PopGuessHeight) * PopMaxScale;
		// The pop hangs from its anchor (pivot: bottom middle); the banner's box is centred on the screen, its gem down.
		const double BannerLeft = View.X * 0.5 - UHudLevelUpBannerWidget::Width * 0.5;
		const double BannerRight = View.X * 0.5 + UHudLevelUpBannerWidget::Width * 0.5;
		const double BannerTop = LevelUpGemDown - UHudLevelUpBannerWidget::Height * 0.5;
		const double BannerBottom = LevelUpGemDown + UHudLevelUpBannerWidget::Height * 0.5;
		const double Top = Anchor.Y - Height;
		const bool bAcross = Anchor.X + Width * 0.5 > BannerLeft && Anchor.X - Width * 0.5 < BannerRight;
		if (bAcross && Anchor.Y > BannerTop && Top < BannerBottom)
		{
			const double Down = BannerBottom + LevelUpClear - Top;
			const double Up = BannerTop - LevelUpClear - Anchor.Y;
			Wanted = static_cast<float>(Anchor.Y + Down <= View.Y - PopScreenBottomKeep ? Down : Up);
		}
	}
	const FVector2D Now = Label.GetRenderTransform().Translation;
	const float Next = FMath::FInterpTo(static_cast<float>(Now.Y), Wanted, DeltaTime, PopSlideRate);
	Label.SetRenderTranslation(FVector2D(0.0, Next));
}

#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/HUD/HudFrameRateWidget.h"
#include "UI/HUD/HudInteractPromptWidget.h"
#include "UI/HUD/HudMagazineWidget.h"
#include "UI/HUD/HudMinimapWidget.h"
#include "UI/HUD/HudPickupFeedWidget.h"
#include "UI/HUD/HudVitalsWidget.h"
#include "UI/HUD/HudWeaponSlotsWidget.h"
#include "UI/HUD/HudXPBarWidget.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Combat/HealthComponent.h"
#include "Interaction/InteractionComponent.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

using namespace LooterUI;

namespace
{
	/** Opacity of a corner cluster when nothing is happening. */
	constexpr float IdleOpacity = 0.6f;
	/** Seconds a cluster stays fully visible after something happens in it. */
	constexpr float ActivityHold = 3.f;
	/**
	 * How far the gun is raised toward the sight (GetAimAlpha) when the crosshair is gone: it fades out over the first
	 * part of the raise, so it never shows beside the sight's own reticle.
	 */
	constexpr float CrosshairGoneAtAim = 0.6f;

	void SetTextIfChanged(UTextBlock* Text, const FString& Value)
	{
		if (!Text->GetText().ToString().Equals(Value))
		{
			Text->SetText(FText::FromString(Value));
		}
	}
}

// A little bigger than the slots' (the icons share one square view box sized for the tall sniper round), so the round
// stands about as tall as the cartridge's numbers.
const FVector2D UPlayerHUDWidget::AmmoClassBox(30.f, 30.f);

FString UPlayerHUDWidget::BoundKeyName(FName BindingId, const TCHAR* Fallback) const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	return Bindings ? Bindings->GetKey(BindingId).GetDisplayName().ToString().ToUpper() : FString(Fallback);
}

void UPlayerHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	PulseTime += InDeltaTime;

	const APlayerController* PC = GetOwningPlayer();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	UWeaponManagerComponent* Manager = Pawn ? Pawn->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	UHealthComponent* Health = Pawn ? Pawn->FindComponentByClass<UHealthComponent>() : nullptr;
	const UInteractionComponent* Interaction = Pawn ? Pawn->FindComponentByClass<UInteractionComponent>() : nullptr;

	BindToPawn(Manager);
	UpdateWeaponCluster(Manager, InDeltaTime);
	UpdateVitals(Health, InDeltaTime);
	UpdatePickupCard(Manager, Interaction, InDeltaTime);

	if (HitMarkerTime > 0.f)
	{
		HitMarkerTime -= InDeltaTime;
		const float Alpha = FMath::Clamp(HitMarkerTime / 0.18f, 0.f, 1.f);
		HitMarker->SetRenderOpacity(Alpha);
		// Pops in slightly large and settles.
		HitMarker->SetRenderScale(FVector2D(1.f + 0.3f * Alpha * Alpha));
		if (HitMarkerTime <= 0.f)
		{
			HitMarker->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (MessageTime > 0.f)
	{
		MessageTime -= InDeltaTime;
		MessagePlate->SetRenderOpacity(FMath::Clamp(MessageTime / 0.5f, 0.f, 1.f));
		if (MessageTime <= 0.f)
		{
			MessagePlate->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UPlayerHUDWidget::BindToPawn(UWeaponManagerComponent* Manager)
{
	if (BoundManager.Get() != Manager)
	{
		if (UWeaponManagerComponent* Old = BoundManager.Get())
		{
			Old->OnMessage.RemoveDynamic(this, &UPlayerHUDWidget::HandleMessage);
		}
		if (Manager)
		{
			Manager->OnMessage.AddUniqueDynamic(this, &UPlayerHUDWidget::HandleMessage);
		}
		BoundManager = Manager;
	}

	// Hit markers and reload progress come from whichever weapon is in hand.
	AWeaponBase* Active = Manager ? Manager->GetActiveWeapon() : nullptr;
	if (BoundWeapon.Get() != Active)
	{
		if (AWeaponBase* Old = BoundWeapon.Get())
		{
			Old->OnHit.RemoveDynamic(this, &UPlayerHUDWidget::HandleHit);
			Old->OnReloadStarted.RemoveDynamic(this, &UPlayerHUDWidget::HandleReloadStarted);
		}
		if (Active)
		{
			Active->OnHit.AddUniqueDynamic(this, &UPlayerHUDWidget::HandleHit);
			Active->OnReloadStarted.AddUniqueDynamic(this, &UPlayerHUDWidget::HandleReloadStarted);
		}
		BoundWeapon = Active;
		// A weapon switch is worth showing.
		WeaponActivity = ActivityHold;
		LastMagazine = INDEX_NONE;
		ReloadDuration = 0.f;
	}
}

void UPlayerHUDWidget::UpdateWeaponCluster(UWeaponManagerComponent* Manager, float DeltaTime)
{
	const AWeaponBase* Active = Manager ? Manager->GetActiveWeapon() : nullptr;
	WeaponCluster->SetVisibility(Active ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	UpdateCrosshair(Active, DeltaTime);
	if (!Active)
	{
		return;
	}

	const int32 Magazine = Active->GetCurrentMagazine();
	const int32 Reserve = Active->GetReserveAmmo();
	const int32 MagazineSize = FMath::Max(1, Active->GetStats().MagazineSize);
	// BindToPawn forgets the count when another gun comes into hand: its magazine shows at once instead of easing there.
	const bool bSwitchedWeapon = LastMagazine == INDEX_NONE;
	if (Magazine != LastMagazine || Reserve != LastReserve)
	{
		WeaponActivity = ActivityHold;
		LastMagazine = Magazine;
		LastReserve = Reserve;
	}
	const bool bReloading = Active->IsReloading();
	if (bReloading)
	{
		WeaponActivity = ActivityHold;
		ReloadElapsed += DeltaTime;
	}
	WeaponActivity = FMath::Max(0.f, WeaponActivity - DeltaTime);

	const float MagazineFraction = static_cast<float>(Magazine) / MagazineSize;
	const bool bLow = MagazineFraction <= UHudMagazineWidget::LowFraction;
	const float Pulse = 0.5f + 0.5f * FMath::Sin(PulseTime * 8.f);

	// The magazine: a cartridge that drains as the gun fires and, while reloading, fills with the reload's progress. It
	// colors itself orange when low and red when empty, its outline beating with the reload prompt.
	const float ReloadProgress = ReloadDuration > 0.f ? FMath::Clamp(ReloadElapsed / ReloadDuration, 0.f, 1.f) : 0.f;
	MagazineGauge->SetMagazine(Magazine, MagazineSize, bReloading, ReloadProgress, bSwitchedWeapon, Pulse, DeltaTime);
	SetTextIfChanged(ReserveText, FString::FromInt(Reserve));
	ReserveText->SetColorAndOpacity(FSlateColor(Reserve == 0 ? Color::Worse() : Color::TextDim()));

	// Status beside the magazine: reloading, a prompt when dry, or nothing.
	if (bReloading)
	{
		SetTextIfChanged(StatusText, TEXT("RELOADING"));
		StatusText->SetColorAndOpacity(FSlateColor(Color::Accent()));
	}
	else if (Magazine == 0 && Reserve > 0)
	{
		SetTextIfChanged(StatusText, FString::Printf(TEXT("[%s] RELOAD"), *BoundKeyName(TEXT("Reload"), TEXT("R"))));
		StatusText->SetColorAndOpacity(FSlateColor(FMath::Lerp(Color::Accent(), Color::Worse(), Pulse)));
	}
	else if (Magazine == 0)
	{
		SetTextIfChanged(StatusText, TEXT("NO AMMO"));
		StatusText->SetColorAndOpacity(FSlateColor(Color::Worse()));
	}
	else
	{
		SetTextIfChanged(StatusText, TEXT(""));
	}

	const FWeaponInstanceData& Instance = Active->GetInstance();
	SetTextIfChanged(WeaponName, LooterWeaponText::Name(Instance).ToUpper());
	WeaponName->SetColorAndOpacity(FSlateColor(LooterWeaponText::Color(Instance)));
	SetTextIfChanged(FireModeText, LooterWeaponText::FireModeName(Instance).ToUpper());
	// The ammo it takes, as the same Inked icon the slots and the inventory show.
	const TOptional<EAmmoType> AmmoType = Instance.Definition && LooterAmmo::IsValid(Instance.Definition->AmmoType)
		? TOptional<EAmmoType>(Instance.Definition->AmmoType) : TOptional<EAmmoType>();
	if (AmmoType != ShownAmmoType)
	{
		ShownAmmoType = AmmoType;
		if (AmmoType.IsSet())
		{
			AmmoClassIcon->SetBrush(InkedIconBrush(LoadoutParts::AmmoIconName(*AmmoType), LoadoutParts::AmmoIcon(*AmmoType), AmmoClassBox));
		}
		AmmoClassIcon->SetVisibility(AmmoType.IsSet() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	WeaponSlots->Update(Manager, DeltaTime);

	// Fade back when idle; stay up while there's something to act on.
	const bool bNeedsAttention = WeaponActivity > 0.f || bLow || Reserve == 0;
	const float Target = bNeedsAttention ? 1.f : IdleOpacity;
	WeaponCluster->SetRenderOpacity(FMath::FInterpTo(WeaponCluster->GetRenderOpacity(), Target, DeltaTime, 5.f));
}

void UPlayerHUDWidget::UpdateVitals(UHealthComponent* Health, float DeltaTime)
{
	Vitals->SetVisibility(Health ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (Health)
	{
		// The readout keeps its own damage chip, low-health beat and idle fade.
		Vitals->SetHealth(Health->GetHealth(), Health->GetMaxHealth(), DeltaTime);
	}
}

void UPlayerHUDWidget::UpdateCrosshair(const AWeaponBase* Active, float DeltaTime)
{
	// The gap tracks the weapon's accuracy: tight for rifles, wide for shotguns, tighter still when crouched.
	const float Spread = Active ? Active->GetEffectiveSpread() : 1.f;
	const float Wanted = FMath::Clamp(22.f + Spread * 7.f, 24.f, 80.f);
	if (!FMath::IsNearlyEqual(Wanted, CrosshairSize, 0.25f))
	{
		CrosshairSize = CrosshairSize <= 0.f ? Wanted : FMath::FInterpTo(CrosshairSize, Wanted, DeltaTime, 10.f);
		CrosshairBox->SetWidthOverride(CrosshairSize);
		CrosshairBox->SetHeightOverride(CrosshairSize);
	}

	// No aiming while the gun is down in the sprint pose, and no crosshair when the camera faces the character.
	const AActor* Holder = Active ? Active->GetOwner() : nullptr;
	const UPlayerLocomotionComponent* Locomotion = Holder ? Holder->FindComponentByClass<UPlayerLocomotionComponent>() : nullptr;
	const UPlayerViewComponent* View = Holder ? Holder->FindComponentByClass<UPlayerViewComponent>() : nullptr;
	const bool bFacingCamera = View && View->GetViewMode() == EPlayerViewMode::ThirdPersonFront;
	// Looking through the sight in first person, the sight's reticle is the aim point: the crosshair fades as the gun
	// comes up and returns as it's lowered, so only one shows. Third-person aiming only zooms (no sight is seen), so the
	// crosshair stays there.
	const float SightAim = View && View->IsFirstPerson() ? View->GetAimAlpha() : 0.f;
	const float SightFade = FMath::Clamp(1.f - SightAim / CrosshairGoneAtAim, 0.f, 1.f);
	const float Opacity = bFacingCamera ? 0.f : (1.f - (Locomotion ? Locomotion->GetSprintAlpha() : 0.f)) * SightFade;
	if (!FMath::IsNearlyEqual(CrosshairBox->GetRenderOpacity(), Opacity, 0.01f))
	{
		CrosshairBox->SetRenderOpacity(Opacity);
	}
}

void UPlayerHUDWidget::HandleHit(const FHitResult& Hit, float Damage, bool bCritical)
{
	// Only confirm hits on things that can actually be hurt, not walls.
	const AActor* HitActor = Hit.GetActor();
	if (!HitActor || !HitActor->FindComponentByClass<UHealthComponent>())
	{
		return;
	}

	HitMarkerTime = 0.18f;
	HitMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
	for (UImage* Tick : HitMarkerTicks)
	{
		Tick->SetColorAndOpacity(bCritical ? Color::Accent() : FLinearColor::White);
	}
}

void UPlayerHUDWidget::HandleReloadStarted(float Duration)
{
	ReloadDuration = Duration;
	ReloadElapsed = 0.f;
}

void UPlayerHUDWidget::HandleMessage(const FText& Message)
{
	MessageText->SetText(FText::FromString(Message.ToString().ToUpper()));
	MessagePlate->SetVisibility(ESlateVisibility::HitTestInvisible);
	MessagePlate->SetRenderOpacity(1.f);
	MessageTime = 2.5f;
}

#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/HUD/HudGrenadeWidget.h"
#include "UI/HUD/HudLevelUpBannerWidget.h"
#include "UI/HUD/HudPickupFeedWidget.h"
#include "UI/HUD/HudMagazineWidget.h"
#include "UI/HUD/HudMissionCompleteWidget.h"
#include "UI/HUD/HudPlayerFrameWidget.h"
#include "UI/HUD/HudWeaponSlotsWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundRules.h"
#include "Combat/HealthComponent.h"
#include "Interaction/InteractionComponent.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerMeleeComponent.h"
#include "Player/PlayerThrowComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Weapons/WeaponBase.h"
#include "World/BreakableKinds.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
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
	/** Each shot kicks the crosshair out to this size, settling back over CrosshairKickTime seconds (easing out). */
	constexpr float CrosshairKickScale = 1.45f;
	constexpr float CrosshairKickTime = 0.16f;

	/** What the status line beside the fire mode says. */
	enum class EHudWeaponStatus : uint8
	{
		None,
		Reloading,
		ReloadPrompt,
		NoAmmo
	};
}

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
	BindThrow(UPlayerThrowComponent::Find(Pawn));
	UpdateGrenades(InDeltaTime);
	UpdateWeaponCluster(Manager, InDeltaTime);
	UpdatePlayerFrame(Health, InDeltaTime);
	UpdatePickupCard(Manager, Interaction, InDeltaTime);

	if (HitMarkerTime > 0.f)
	{
		HitMarkerTime -= InDeltaTime;
		const float Alpha = FMath::Clamp(HitMarkerTime / (bKillMarker ? KillMarkerSeconds : HitMarkerSeconds), 0.f, 1.f);
		HitMarker->SetRenderOpacity(Alpha);
		// Pops in slightly large and settles; a kill's pops bigger, in red, and stays a moment longer.
		HitMarker->SetRenderScale(FVector2D(1.f + (bKillMarker ? KillMarkerPop : 0.3f) * Alpha * Alpha));
		if (HitMarkerTime <= 0.f)
		{
			HitMarker->SetVisibility(ESlateVisibility::Hidden);
			bKillMarker = false;
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

	// A fist's strike has no gun to report it, so its hit marker comes from the melee itself.
	UPlayerMeleeComponent* Melee = Manager ? UPlayerMeleeComponent::Find(Manager->GetOwner()) : nullptr;
	if (BoundMelee.Get() != Melee)
	{
		if (UPlayerMeleeComponent* Old = BoundMelee.Get())
		{
			Old->OnMeleeHit.RemoveDynamic(this, &UPlayerHUDWidget::HandleMeleeHit);
		}
		if (Melee)
		{
			Melee->OnMeleeHit.AddUniqueDynamic(this, &UPlayerHUDWidget::HandleMeleeHit);
		}
		BoundMelee = Melee;
	}

	// Shots, hit markers and reload progress come from whichever weapon is in hand.
	AWeaponBase* Active = Manager ? Manager->GetActiveWeapon() : nullptr;
	if (BoundWeapon.Get() != Active)
	{
		if (AWeaponBase* Old = BoundWeapon.Get())
		{
			Old->OnHit.RemoveDynamic(this, &UPlayerHUDWidget::HandleHit);
			Old->OnFired.RemoveDynamic(this, &UPlayerHUDWidget::HandleFired);
			Old->OnReloadStarted.RemoveDynamic(this, &UPlayerHUDWidget::HandleReloadStarted);
		}
		if (Active)
		{
			Active->OnHit.AddUniqueDynamic(this, &UPlayerHUDWidget::HandleHit);
			Active->OnFired.AddUniqueDynamic(this, &UPlayerHUDWidget::HandleFired);
			Active->OnReloadStarted.AddUniqueDynamic(this, &UPlayerHUDWidget::HandleReloadStarted);
		}
		BoundWeapon = Active;
		// A weapon switch is worth showing.
		WeaponActivity = ActivityHold;
		LastMagazine = INDEX_NONE;
		ReloadDuration = 0.f;
		bWeaponTextStale = true;
		ShownStatus = MAX_uint8;
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

	// The magazine: a cartridge that drains from the tip as the gun fires and, while reloading, fills from the base with
	// the reload's progress, with the rounds and the reserve by its base. It colors itself orange when low and red when
	// empty, its outline beating with the reload prompt.
	const float ReloadProgress = ReloadDuration > 0.f ? FMath::Clamp(ReloadElapsed / ReloadDuration, 0.f, 1.f) : 0.f;
	MagazineGauge->SetMagazine(Magazine, MagazineSize, Reserve, bReloading, ReloadProgress, bSwitchedWeapon, Pulse, DeltaTime);
	UpdateWeaponStatus(bReloading, Magazine, Reserve, Pulse);

	// The gun's name in its rarity's color, the gem after it and the fire mode only change with the gun in hand.
	if (bWeaponTextStale)
	{
		bWeaponTextStale = false;
		const FWeaponInstanceData& Instance = Active->GetInstance();
		const FLinearColor Rarity = LooterWeaponText::Color(Instance);
		FitWeaponName(LooterWeaponText::Name(Instance).ToUpper());
		WeaponName->SetColorAndOpacity(FSlateColor(Rarity));
		RarityGem->SetColorAndOpacity(Rarity);
		FireModeText->SetText(FText::FromString(LooterWeaponText::FireModeName(Instance).ToUpper()));
	}

	WeaponSlots->Update(Manager, DeltaTime);

	// Fade back when idle; stay up while there's something to act on.
	const bool bNeedsAttention = WeaponActivity > 0.f || bLow || Reserve == 0;
	const float Target = bNeedsAttention ? 1.f : IdleOpacity;
	WeaponCluster->SetRenderOpacity(FMath::FInterpTo(WeaponCluster->GetRenderOpacity(), Target, DeltaTime, 5.f));
}

void UPlayerHUDWidget::UpdateWeaponStatus(bool bReloading, int32 Magazine, int32 Reserve, float Pulse)
{
	const EHudWeaponStatus Status = bReloading ? EHudWeaponStatus::Reloading
		: Magazine > 0 ? EHudWeaponStatus::None
		: Reserve > 0 ? EHudWeaponStatus::ReloadPrompt
		: EHudWeaponStatus::NoAmmo;
	// The words only change with the state (the prompt's key is looked up then).
	if (static_cast<uint8>(Status) != ShownStatus)
	{
		ShownStatus = static_cast<uint8>(Status);
		switch (Status)
		{
		case EHudWeaponStatus::Reloading:
			StatusText->SetText(FText::FromString(TEXT("RELOADING")));
			StatusText->SetColorAndOpacity(FSlateColor(Color::Accent()));
			break;
		case EHudWeaponStatus::ReloadPrompt:
			StatusText->SetText(FText::FromString(FString::Printf(TEXT("[%s] RELOAD"), *BoundKeyName(TEXT("Reload"), TEXT("R")))));
			break;
		case EHudWeaponStatus::NoAmmo:
			StatusText->SetText(FText::FromString(TEXT("NO AMMO")));
			StatusText->SetColorAndOpacity(FSlateColor(Color::Worse()));
			break;
		default:
			StatusText->SetText(FText::GetEmpty());
			break;
		}
	}
	// Dry with rounds to reload from: the prompt beats toward red, with the cartridge's outline.
	if (Status == EHudWeaponStatus::ReloadPrompt)
	{
		StatusText->SetColorAndOpacity(FSlateColor(FMath::Lerp(Color::Accent(), Color::Worse(), Pulse)));
	}
}

void UPlayerHUDWidget::UpdatePlayerFrame(UHealthComponent* Health, float DeltaTime)
{
	PlayerFrame->SetVisibility(Health ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (Health)
	{
		// The frame keeps its own damage chip, low-health beat, portrait reactions and experience bar.
		PlayerFrame->SetHealth(Health->GetHealth(), Health->GetMaxHealth(), DeltaTime);
	}
	else
	{
		// No health to show (a pawn without any): the next pawn's health is shown as it is, not as a heal from this one's.
		PlayerFrame->ForgetHealth();
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

	// Each shot kicks it out and it settles back, so every shot is felt where the eyes are. The frame of the shot shows
	// the full kick.
	if (bCrosshairKicking)
	{
		const float Settled = FMath::Min(CrosshairKickAge / CrosshairKickTime, 1.f);
		const float Kick = 1.f - FMath::InterpEaseOut(0.f, 1.f, Settled, 2.f);
		CrosshairBox->SetRenderScale(FVector2D(1.f + (CrosshairKickScale - 1.f) * Kick));
		CrosshairKickAge += DeltaTime;
		bCrosshairKicking = Settled < 1.f;
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

void UPlayerHUDWidget::HandleMeleeHit(const FHitResult& Hit, float Damage, bool bCritical)
{
	// With a gun in hand the strike already came through the gun's OnHit: only a fist's is shown from here.
	if (!BoundWeapon.IsValid())
	{
		HandleHit(Hit, Damage, bCritical);
	}
}

void UPlayerHUDWidget::BindThrow(UPlayerThrowComponent* Throw)
{
	if (BoundThrow.Get() == Throw)
	{
		return;
	}
	if (UPlayerThrowComponent* Old = BoundThrow.Get())
	{
		Old->OnGrenadeHit.RemoveDynamic(this, &UPlayerHUDWidget::HandleGrenadeHit);
		Old->OnGrenadesChanged.Remove(GrenadesChangedHandle);
	}
	GrenadesChangedHandle.Reset();
	if (Throw)
	{
		// A burst's hits have no gun to report them: the hit marker comes from the throw itself.
		Throw->OnGrenadeHit.AddUniqueDynamic(this, &UPlayerHUDWidget::HandleGrenadeHit);
		GrenadesChangedHandle = Throw->OnGrenadesChanged.AddUObject(this, &UPlayerHUDWidget::HandleGrenadesChanged);
	}
	BoundThrow = Throw;
}

void UPlayerHUDWidget::UpdateGrenades(float DeltaTime)
{
	if (!GrenadeCounter)
	{
		return;
	}
	const UPlayerThrowComponent* Throw = BoundThrow.Get();
	GrenadeCounter->Update(Throw ? Throw->GetGrenades() : 0, UPlayerThrowComponent::GetMaxGrenades(), Throw && Throw->IsUnlocked(),
		BoundKeyName(UKeyBindingSubsystem::GrenadeBindingId(), TEXT("G")), DeltaTime);
}

void UPlayerHUDWidget::HandleGrenadesChanged(int32 Count, int32 Delta, EGrenadeChange Why)
{
	// Worth bringing the weapon column forward for, whatever it was.
	WeaponActivity = ActivityHold;
	if (!GrenadeCounter)
	{
		return;
	}
	switch (Why)
	{
	case EGrenadeChange::Thrown:
		GrenadeCounter->Flash(EHudGrenadeFlash::Thrown);
		break;
	case EGrenadeChange::Denied:
		GrenadeCounter->Flash(EHudGrenadeFlash::Denied);
		break;
	case EGrenadeChange::PickedUp:
	case EGrenadeChange::Given:
		if (Delta > 0)
		{
			GrenadeCounter->Flash(EHudGrenadeFlash::Gained);
		}
		if (PickupFeed && (Delta > 0 || Why == EGrenadeChange::PickedUp))
		{
			// Told where the eyes are, in the ammo pickups' words ("+36 AR Ammo", "AR Ammo full").
			PickupFeed->AddLine(Delta > 0 ? FString::Printf(TEXT("+%d Grave Salt"), Delta) : FString(TEXT("Grave Salt full")));
		}
		break;
	default:
		break;
	}
}

void UPlayerHUDWidget::HandleGrenadeHit(const FHitResult& Hit, float Damage, bool bCritical)
{
	// One burst lands on many bodies in the same frame: the marker shows them all (a kill's red wins), the sound once.
	const bool bSameBurst = GrenadeHitFrame == GFrameCounter;
	GrenadeHitFrame = GFrameCounter;
	bMuteHitSound = bSameBurst;
	HandleHit(Hit, Damage, bCritical);
	bMuteHitSound = false;
}

void UPlayerHUDWidget::HandleHit(const FHitResult& Hit, float Damage, bool bCritical)
{
	// Only confirm hits on things that can actually be hurt, not walls.
	const AActor* HitActor = Hit.GetActor();
	const UHealthComponent* TargetHealth = HitActor ? HitActor->FindComponentByClass<UHealthComponent>() : nullptr;
	if (!TargetHealth)
	{
		return;
	}
	// Heard as well as seen. The hit lands before the HUD hears of it, so a dead target is a kill, which has its own sound
	// (the creature's voice plays UI.Kill once per death), or a body already down, which confirms nothing more. A crate or
	// a barrel that breaks isn't a kill: it's a hit like any other (the white marker, the tick), not the red confirm.
	const bool bKill = TargetHealth->IsDead() && !HitActor->ActorHasTag(FName(LooterBreakables::Tag));
	if (!bKill && !bMuteHitSound)
	{
		LooterSound::Play2D(this, LooterSoundCue::HitMarker, 1.f, bCritical ? LooterSoundRules::CritPitch : 1.f);
	}

	// The kill's confirm: the marker in red, bigger and longer, and no later pellet of the same blast takes it back.
	if (bKill || !bKillMarker)
	{
		bKillMarker = bKill;
		HitMarkerTime = bKill ? KillMarkerSeconds : HitMarkerSeconds;
		const FLinearColor Tint = bKill ? Color::Hurt() : (bCritical ? Color::Accent() : FLinearColor::White);
		for (UImage* Tick : HitMarkerTicks)
		{
			Tick->SetColorAndOpacity(Tint);
		}
	}
	HitMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPlayerHUDWidget::HandleFired()
{
	CrosshairKickAge = 0.f;
	bCrosshairKicking = true;
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

void UPlayerHUDWidget::HandleLevelUp(int32 NewLevel)
{
	// A turn-in's experience levels up while its mission-complete banner shows: the level-up banner follows it.
	if (MissionBanner && MissionBanner->DeferLevelUp(NewLevel))
	{
		return;
	}
	if (LevelUpBanner)
	{
		LevelUpBanner->Show(NewLevel);
	}
}

void UPlayerHUDWidget::FitWeaponName(const FString& Name)
{
	const FText Text = FText::FromString(Name);
	WeaponName->SetText(Text);
	// Measured once per gun change: a name wider than NameMaxWidth is set smaller in proportion, in whole points.
	FSlateFontInfo Font = WeaponName->GetFont();
	Font.Size = NameFontSize;
	FSlateRenderer* Renderer = FSlateApplication::IsInitialized() ? FSlateApplication::Get().GetRenderer() : nullptr;
	if (Renderer)
	{
		const float Width = static_cast<float>(Renderer->GetFontMeasureService()->Measure(Name, Font).X);
		if (Width > NameMaxWidth)
		{
			Font.Size = FMath::Max(10, FMath::FloorToInt(NameFontSize * NameMaxWidth / Width));
		}
	}
	WeaponName->SetFont(Font);
}

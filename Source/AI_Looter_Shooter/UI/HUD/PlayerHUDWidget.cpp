#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/HUD/HudGraveMarkedWidget.h"
#include "UI/HUD/HudGrenadeWidget.h"
#include "UI/HUD/HudLevelUpBannerWidget.h"
#include "UI/HUD/HudPickupFeedWidget.h"
#include "UI/HUD/HudMagazineWidget.h"
#include "UI/HUD/HudMissionCompleteWidget.h"
#include "UI/HUD/HudNotchTallyWidget.h"
#include "UI/HUD/HudPlayerFrameWidget.h"
#include "UI/HUD/HudWeaponSlotsWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Audio/LooterSound.h"
#include "Combat/GrenadeKinds.h"
#include "Combat/HealthComponent.h"
#include "Embers/EmberComponent.h"
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
	BindEmbers(UEmberComponent::Find(Pawn));
	UpdateEmbers(InDeltaTime);
	UpdateWeaponCluster(Manager, InDeltaTime);
	UpdatePlayerFrame(Health, InDeltaTime);
	UpdatePickupCard(Manager, Interaction, InDeltaTime);

	// The wards of the creatures last hit, as they are now (a burn may have broken one between the player's hits).
	WardMemory.Refresh();
	if (HitMarkerTime > 0.f)
	{
		HitMarkerTime -= InDeltaTime;
		const float Alpha = FMath::Clamp(HitMarkerTime / FMath::Max(HitMarkerShowFor, 0.01f), 0.f, 1.f);
		HitMarker->SetRenderOpacity(Alpha);
		// Pops in slightly large and settles, at its hit's size; a kill's and a ward break's pop bigger and stay a moment longer.
		HitMarker->SetRenderScale(FVector2D(HitMarkerSize * (1.f + HitMarkerPop * Alpha * Alpha)));
		if (HitMarkerTime <= 0.f)
		{
			HitMarker->SetVisibility(ESlateVisibility::Hidden);
			ShownMark = EHudHitMark::Body;
			HitMarkerSize = 1.f;
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
			Old->OnHit.RemoveDynamic(this, &UPlayerHUDWidget::HandleGunHit);
			Old->OnFired.RemoveDynamic(this, &UPlayerHUDWidget::HandleFired);
			Old->OnReloadStarted.RemoveDynamic(this, &UPlayerHUDWidget::HandleReloadStarted);
		}
		if (Active)
		{
			Active->OnHit.AddUniqueDynamic(this, &UPlayerHUDWidget::HandleGunHit);
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

	// The gun's notches before its name: a kill with it pops the count and brings the column forward; its name is set
	// again, as a milestone can give it a nickname.
	if (NotchTally && NotchTally->Update(Active->GetInstance().Kills, bSwitchedWeapon, DeltaTime))
	{
		WeaponActivity = ActivityHold;
		bWeaponTextStale = true;
	}

	// The gun's name in its rarity's color, the gem before it and the fire mode only change with the gun in hand.
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

// The crosshair (UpdateCrosshair, when it shows, the shot's kick): PlayerHUDWidgetCrosshair.cpp.

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
		if (PickupFeed && Delta > 0)
		{
			// Told where the eyes are, in the ammo pickups' words ("+36 AR Ammo"); a full count says nothing.
			const UPlayerThrowComponent* Throw = BoundThrow.Get();
			PickupFeed->AddLine(FString::Printf(TEXT("+%d %s"), Delta, *GrenadeKinds::Name(Throw ? Throw->GetKind() : EGrenadeKind::Salt).ToString()));
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

void UPlayerHUDWidget::ShowGraveMarked(const FText& GraveName)
{
	if (GraveBanner)
	{
		GraveBanner->Announce(GraveName);
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

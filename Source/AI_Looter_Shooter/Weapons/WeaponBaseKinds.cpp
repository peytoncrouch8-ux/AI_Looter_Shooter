#include "Weapons/WeaponBase.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSoundCues.h"
#include "Player/PawnInputBinding.h"
#include "Player/PlayerViewComponent.h"
#include "Settings/KeyBindingSubsystem.h"
#include "UI/HUD/HudScopeWidget.h"
#include "Weapons/GunKindsTuning.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponModelComponent.h"
#include "Weapons/WeaponRocket.h"
#include "Weapons/WeaponRocketBlast.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedPlayerInput.h"
#include "InputAction.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// AWeaponBase's ways for the four frontier-tech kinds (Phase 12.3; GunKindsTuning has the numbers and the rules): the
// SMG helix's glow, the sniper's bolt and scope, the LMG's spin and heat, the launcher's rocket. Every number is read
// when it's used, so a change in play shows at once.

namespace
{
	const FName BoltCycleCue(LooterSoundCue::Sniper::BoltCycle);
	const FName SpinUpCue(LooterSoundCue::LMG::SpinUp);
	/** An LMG this cold or colder is heard spinning up as the trigger goes down. */
	constexpr float SpinHeardBelow = 0.35f;
	/** How quickly the scope's sway follows the breath (an exponential rate a second): held, it settles in a moment. */
	constexpr float SwayEase = 5.f;
	/** Aimed at least this far for the breath to be held (the scope's up). */
	constexpr float BreathAimed = 0.9f;
	/** The scope picture fades in over this part of the sights' rise; past HideGunAt the gun and the hands are hidden behind it. */
	constexpr float ScopeFrom = 0.75f;
	constexpr float ScopeFull = 0.95f;
	constexpr float HideGunAt = 0.5f;

	EWeaponKind KindOfInstance(const FWeaponInstanceData& Instance)
	{
		return Instance.Definition ? Instance.Definition->Kind : EWeaponKind::None;
	}
}

// ---------------------------------------------------------------------------
// The numbers the rest of the gun asks
// ---------------------------------------------------------------------------

float AWeaponBase::GetShotInterval() const
{
	const float Base = Instance.Stats.GetSecondsBetweenShots();
	if (KindOfInstance(Instance) != EWeaponKind::LMG)
	{
		return Base;
	}
	// Spinning up: the rate climbs from its start share to full.
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : SpinTime;
	return Base / GunKinds::SpinRateShare(GetSpinAt(Now), GunKinds::LMGSpinStart());
}

float AWeaponBase::GetSpinAt(double Now) const
{
	return GunKinds::SpinAfter(Spin, static_cast<float>(Now - SpinTime), bSpinFiring, GunKinds::LMGSpinUp(), GunKinds::LMGSpinDown());
}

float AWeaponBase::GetSpin() const
{
	const UWorld* World = GetWorld();
	return World ? GetSpinAt(World->GetTimeSeconds()) : Spin;
}

float AWeaponBase::GetBarrelHeat() const
{
	const UWorld* World = GetWorld();
	if (BarrelHeat <= 0.f || !World)
	{
		return BarrelHeat;
	}
	return GunKinds::HeatCooled(BarrelHeat, static_cast<float>(World->GetTimeSeconds() - BarrelHeatTime), GunKinds::LMGHeatDelay(),
		GunKinds::LMGHeatCool());
}

float AWeaponBase::GetBoltCycle() const
{
	const UWorld* World = GetWorld();
	if (!bBoltCycling || !World)
	{
		return -1.f;
	}
	return GunKinds::BoltCycleProgress(static_cast<float>(World->GetTimeSeconds() - BoltShotTime), BoltTiming);
}

float AWeaponBase::GetBoltOpen() const
{
	if (GetReloadPart() != EWeaponReloadPart::Bolt)
	{
		return 0.f;
	}
	const float Reload = GetReloadProgress();
	if (Reload >= 0.f)
	{
		return LooterReload::BoltOpenInReload(Reload, ReloadPlan);
	}
	return LooterReload::BoltCycleOpen(GetBoltCycle());
}

float AWeaponBase::GetBoltHand() const
{
	if (GetReloadPart() != EWeaponReloadPart::Bolt)
	{
		return 0.f;
	}
	const float Reload = GetReloadProgress();
	return Reload >= 0.f ? LooterReload::BoltHandInReload(Reload, ReloadPlan) : LooterReload::BoltHandInCycle(GetBoltCycle());
}

float AWeaponBase::GetBreathLeft() const
{
	return Breath.Left(GunKinds::BreathRules());
}

float AWeaponBase::GetHolderAimAlpha() const
{
	const AActor* Holder = GetOwner();
	const UPlayerViewComponent* View = Holder ? Holder->FindComponentByClass<UPlayerViewComponent>() : nullptr;
	return View ? View->GetAimAlpha() : 0.f;
}

bool AWeaponBase::IsSprintHeld() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* Player = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!Player || !Player->IsLocalController())
	{
		return false;
	}
	// The sprint key as the player bound it (rebindable, keyboard or pad): aimed, it holds the breath instead of sprinting.
	const UKeyBindingSubsystem* Bindings = FPawnInputBinding::GetBindings(Player);
	const UInputAction* Sprint = Bindings ? Bindings->GetSprintAction() : nullptr;
	const UEnhancedPlayerInput* Input = Cast<UEnhancedPlayerInput>(Player->PlayerInput);
	return Sprint && Input && Input->GetActionValue(Sprint).Get<bool>();
}

// ---------------------------------------------------------------------------
// The trigger and the shot
// ---------------------------------------------------------------------------

void AWeaponBase::StartKindTrigger()
{
	const UWorld* World = GetWorld();
	if (KindOfInstance(Instance) != EWeaponKind::LMG || !World || bSpinFiring)
	{
		return;
	}
	// The barrels start to turn: from cold it's heard winding up before the rate is full.
	const double Now = World->GetTimeSeconds();
	Spin = GetSpinAt(Now);
	SpinTime = Now;
	bSpinFiring = true;
	if (Spin < SpinHeardBelow)
	{
		PlayCue(SpinUpCue);
	}
}

void AWeaponBase::StopKindTrigger()
{
	const UWorld* World = GetWorld();
	if (!bSpinFiring || !World)
	{
		bSpinFiring = false;
		return;
	}
	const double Now = World->GetTimeSeconds();
	Spin = GetSpinAt(Now);
	SpinTime = Now;
	bSpinFiring = false;
}

void AWeaponBase::NoteKindShot(bool bMisfire)
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	switch (KindOfInstance(Instance))
	{
	case EWeaponKind::Sniper:
		// The bolt is worked after every shot (a dud's too, to clear it): it waits out the kick, then cycles in the time
		// the gun has before it can fire again, its case flying as it's drawn back.
		if (bUsingModel)
		{
			BoltShotTime = Now;
			BoltTiming = GunKinds::BoltTiming(GetShotInterval() / GetOwnerTimeScale(), GunKinds::SniperBoltDelay(),
				GunKinds::SniperBoltShare(), GunKinds::SniperBoltMaxSeconds());
			bBoltCycling = true;
			bBoltHeard = false;
			bBoltCaseOut = false;
		}
		break;
	case EWeaponKind::LMG:
		if (!bMisfire)
		{
			BarrelHeat = GunKinds::HeatAfterShot(GetBarrelHeat(), GunKinds::LMGHeatPerShot());
			BarrelHeatTime = Now;
			RefreshGauges();
		}
		break;
	case EWeaponKind::Launcher:
		// The mouth is empty once its last rocket's gone.
		UpdateReloadPart();
		break;
	default:
		break;
	}
	RefreshTick();
}

void AWeaponBase::FireRocket(const FVector& ViewLocation, const FVector& Direction)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	AWeaponRocket::Launch(World, ViewLocation, Direction, GetVisibleMuzzleLocation(), this);
	// The back-blast out of the tube's exhaust, as the player sees the gun.
	FVector Exhaust;
	FVector Way;
	if (bUsingModel && Model->GetBackblast(Exhaust, Way))
	{
		const FTransform& Gun = Model->GetComponentTransform();
		WeaponRocketBlast::SpawnBackblast(*World, GetVisibleLocation(Gun.TransformPosition(Exhaust)), Gun.TransformVectorNoScale(Way));
	}
}

// ---------------------------------------------------------------------------
// Each tick
// ---------------------------------------------------------------------------

bool AWeaponBase::WantsKindTick() const
{
	if (bIsPickup || !Instance.Definition)
	{
		return false;
	}
	const APawn* Holder = Cast<APawn>(GetOwner());
	const bool bInPlayerHand = Holder && Holder->IsPlayerControlled() && !IsHidden();
	switch (Instance.Definition->Kind)
	{
	case EWeaponKind::Sniper:
		// The scope's sway needs every frame in a player's hand (it moves their aim); the bolt while it's being worked.
		return bBoltCycling || bCyclePoseApplied || bInPlayerHand || bScopeHidden || !SwayApplied.IsNearlyZero(0.001f);
	case EWeaponKind::LMG:
		return GetBarrelHeat() > 0.f;
	case EWeaponKind::SMG:
	{
		// Only the last rounds' pulse moves on its own.
		const float Fill = GunKinds::AmmoGlowFill(CurrentMagazine, Instance.Stats.MagazineSize);
		return bInPlayerHand && Fill > 0.f && Fill <= GunKinds::SMGGlowLowShare();
	}
	default:
		return false;
	}
}

void AWeaponBase::UpdateKinds(float DeltaSeconds)
{
	switch (KindOfInstance(Instance))
	{
	case EWeaponKind::Sniper:
		UpdateBoltCycle();
		UpdateScopeSway(DeltaSeconds);
		break;
	case EWeaponKind::SMG:
	case EWeaponKind::LMG:
		RefreshGauges();
		break;
	default:
		break;
	}
}

void AWeaponBase::RefreshGauges()
{
	if (!bUsingModel)
	{
		return;
	}
	const UWorld* World = GetWorld();
	if (Model->HasAmmoGauge())
	{
		const float Fill = GunKinds::AmmoGlowFill(CurrentMagazine, Instance.Stats.MagazineSize);
		const float Time = World ? static_cast<float>(World->GetTimeSeconds()) : 0.f;
		Model->SetAmmoGauge(Fill, GunKinds::AmmoGlowStrength(Fill, GunKinds::SMGGlow(), GunKinds::SMGGlowLowShare(), GunKinds::SMGGlowPulse(), Time));
	}
	if (Model->HasHeatGauge())
	{
		const float Heat = GetBarrelHeat();
		Model->SetHeatGauge(Heat, Heat * GunKinds::LMGHeatGlow());
	}
}

void AWeaponBase::UpdateBoltCycle()
{
	const UWorld* World = GetWorld();
	if (!bBoltCycling || !World || !bUsingModel)
	{
		return;
	}
	const float SinceShot = static_cast<float>(World->GetTimeSeconds() - BoltShotTime);
	const float Progress = GunKinds::BoltCycleProgress(SinceShot, BoltTiming);
	if (Progress < 0.f)
	{
		if (SinceShot >= BoltTiming.Start + BoltTiming.Seconds)
		{
			// Home: whatever a long frame skipped, the case is out and the bolt is down.
			bBoltCycling = false;
			Model->SetBoltOpen(0.f);
			ApplyCyclePose(FVector::ZeroVector, FRotator::ZeroRotator);
			if (!bBoltCaseOut)
			{
				bBoltCaseOut = true;
				EjectCasing();
			}
		}
		return;
	}
	if (!bBoltHeard)
	{
		bBoltHeard = true;
		PlayCue(BoltCycleCue);
	}
	Model->SetBoltOpen(LooterReload::BoltCycleOpen(Progress));
	if (!bBoltCaseOut && Progress >= LooterReload::BoltCaseOutAt)
	{
		bBoltCaseOut = true;
		EjectCasing();
	}
	FVector Offset;
	FRotator Rotation;
	LooterReload::BoltCycleViewPose(Progress, Offset, Rotation);
	ApplyCyclePose(Offset, Rotation);
}

void AWeaponBase::ApplyCyclePose(const FVector& Offset, const FRotator& Rotation)
{
	if (!bUsingModel)
	{
		return;
	}
	// Only the first-person gun turns with the bolt's work; a strike's own pose has the model while it swings.
	const bool bPose = !bIsPickup && !bMeleeSwinging && IsDrawnFirstPerson() && !(Offset.IsNearlyZero(0.01) && Rotation.IsNearlyZero(0.01f));
	if (!bPose)
	{
		if (bCyclePoseApplied && !bMeleeSwinging && !bIsPickup)
		{
			Model->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
		}
		bCyclePoseApplied = false;
		return;
	}
	// Turned about the trigger hand's grip, so the gun rolls in the hand that works the bolt.
	const FVector Pivot = Model->GetGrip();
	const FQuat Turn = Rotation.Quaternion();
	Model->SetRelativeLocationAndRotation(Offset + Pivot - Turn.RotateVector(Pivot), Turn);
	bCyclePoseApplied = true;
}

void AWeaponBase::UpdateScopeSway(float DeltaSeconds)
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* Player = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	// Hidden by its own scope (UpdateScope) it's still in hand; hidden any other way (put away) it isn't.
	if (!Player || !Player->IsLocalController() || bIsPickup || (IsHidden() && !bScopeHidden))
	{
		ClearScopeSway();
		return;
	}
	// Through the scope the aim drifts in a slow figure of eight; held breath steadies it for a few seconds, then the
	// shooter is winded and it drifts wider a while. The sway moves the real aim (the view and the shot), eased in with
	// the scope, so what the crosshair shows is where the round goes.
	const float Aim = GetHolderAimAlpha();
	const float Breathing = Breath.Step(Aim >= BreathAimed && IsSprintHeld(), DeltaSeconds, GunKinds::BreathRules());
	SwayScale = FMath::FInterpTo(SwayScale, Breathing, DeltaSeconds, SwayEase);
	SwayTime += DeltaSeconds;
	const FRotator Target = GunKinds::SwayAt(SwayTime, GunKinds::SniperSway(), GunKinds::SniperSwayRate()) * (SwayScale * Aim);
	const FRotator Delta = Target - SwayApplied;
	if (!Delta.IsNearlyZero(0.0001f))
	{
		Player->SetControlRotation(Player->GetControlRotation() + Delta);
	}
	SwayApplied = Target;
	UpdateScope();
}

void AWeaponBase::ClearScopeSway()
{
	if (!SwayApplied.IsNearlyZero(0.0001f))
	{
		const APawn* Pawn = Cast<APawn>(GetOwner());
		if (APlayerController* Player = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr)
		{
			Player->SetControlRotation(Player->GetControlRotation() - SwayApplied);
		}
	}
	SwayApplied = FRotator::ZeroRotator;
	SwayScale = 1.f;
	Breath = GunKinds::FScopeBreath();
	HideScope();
}

void AWeaponBase::UpdateScope()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* Player = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	// Only a scoped gun in a local player's first-person hands: in third person the gun shows as ever.
	const bool bFirstPerson = bUsingModel && Model->GetFirstPersonPrimitiveType() == EFirstPersonPrimitiveType::FirstPerson;
	const float Aim = GetHolderAimAlpha();
	const float Strength = Player && Player->IsLocalController() && bFirstPerson && !bIsPickup && Instance.Stats.Zoom > 1.5f
		? FMath::SmoothStep(ScopeFrom, ScopeFull, Aim) : 0.f;
	if (Strength <= 0.f && !ScopeWidget.IsValid())
	{
		HideScope();
		return;
	}
	UHudScopeWidget* Widget = ScopeWidget.Get();
	if (!Widget && Player)
	{
		Widget = CreateWidget<UHudScopeWidget>(Player, UHudScopeWidget::StaticClass());
		if (Widget)
		{
			Widget->AddToViewport(UHudScopeWidget::ViewportZOrder);
			ScopeWidget = Widget;
		}
	}
	if (Widget)
	{
		Widget->ShowScope(Strength, GetBreathLeft(), Breath.bHolding, Breath.Winded > 0.f);
	}
	// Behind the picture the eyepiece (22 cm from the eye) would fill the lens: the gun and the hands step out of the way.
	const bool bHide = Strength >= HideGunAt;
	if (bHide != bScopeHidden)
	{
		bScopeHidden = bHide;
		SetActorHiddenInGame(bHide);
	}
}

void AWeaponBase::HideScope()
{
	if (UHudScopeWidget* Widget = ScopeWidget.Get())
	{
		Widget->ShowScope(0.f, 1.f, false, false);
	}
	if (bScopeHidden)
	{
		bScopeHidden = false;
		SetActorHiddenInGame(false);
	}
}

#include "Weapons/WeaponBase.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Weapons/WeaponCurseEffects.h"
#include "Weapons/WeaponCurses.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/GunfeelKickTuning.h"
#include "Weapons/GunshotSound.h"
#include "Weapons/PartEffectComponent.h"
#include "Weapons/WeaponSounds.h"
#include "Weapons/WeaponTrigger.h"
#include "Weapons/VfxRules.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerViewComponent.h"
#include "UI/World/WeaponLabelWidget.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/WeaponModelComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraTypes.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Loot/LootTossComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

namespace
{
	/** How big a misfire's spit of sparks and smoke is beside a shot's flash in the open (FWeaponFX::SpawnFlash). */
	constexpr float MisfirePuffScale = 0.25f;

	/** The gun's holder is down (dead, until a respawn resets their health): a dead hand fires nothing. */
	bool IsHolderDead(const AActor* Holder)
	{
		const UHealthComponent* Health = Holder ? Holder->FindComponentByClass<UHealthComponent>() : nullptr;
		return Health && Health->IsDead();
	}
}

// ---------------------------------------------------------------------------
// Firing
// ---------------------------------------------------------------------------

void AWeaponBase::StartFire()
{
	const UWorld* World = GetWorld();
	StartFireAt(World ? World->GetTimeSeconds() : 0.0);
}

void AWeaponBase::StartFireAt(double Now)
{
	// From death to respawn the trigger does nothing (a press held or sent while dying once landed shots 2 s after death).
	if (IsHolderDead(GetOwner()))
	{
		bWantsToFire = false;
		return;
	}
	bWantsToFire = true;

	// A pump gun mid-reload finishes the shell in hand, then fires (InterruptReload; others ignore the press).
	if (bReloading)
	{
		InterruptReload();
	}

	// Mid-strike the trigger is only remembered: auto fire takes up again as the swing ends (EndMeleeSwing).
	if (bReloading || bMeleeSwinging || !Instance.Definition)
	{
		return;
	}
	// An LMG starts to spin (heard from cold).
	StartKindTrigger();

	// Respect the fire rate however the trigger is worked. A press while the gun is still cycling is remembered only if it
	// comes just before the gun is ready, and only once (WeaponTrigger::Decide): clicks made earlier are lost, so spamming
	// never stacks shots that go off after the clicking has stopped.
	FTimerManager& Timers = GetWorldTimerManager();
	// A part effect's say over the trigger (Phase 12.3, WeaponTrigger::PlanFor): a Burst part's three rounds and pause, an
	// Overcharge part's wind-up at half the rate; any other gun's plan is its definition's.
	const WeaponTrigger::FTriggerPlan Trigger = WeaponTrigger::PlanFor(Instance);
	// In the world's seconds: a shooter living faster than the world (Sundown) cycles the gun in fewer of them. An LMG
	// still spinning up fires slower (GetShotInterval).
	const double Interval = GetShotInterval() * Trigger.PullIntervalScale / GetOwnerTimeScale();
	const double SecondsUntilReady = Interval - (Now - LastFireTime);
	const WeaponTrigger::EPress Press = WeaponTrigger::Decide(Trigger.FireMode, SecondsUntilReady, Timers.IsTimerActive(FireTimer));
	// An Overcharge pull winds up from the press itself: one made before the gun is ready is lost, never a charge that
	// starts late on its own.
	if (Press == WeaponTrigger::EPress::Ignore || (Trigger.ChargeSeconds > 0.f && Press != WeaponTrigger::EPress::FireNow))
	{
		return;
	}

	if (Trigger.FireMode == EWeaponFireMode::Burst)
	{
		BurstShotsRemaining = Trigger.BurstCount;
	}

	if (Press == WeaponTrigger::EPress::FireNow && Trigger.ChargeSeconds > 0.f)
	{
		// Held, it fires charged (x2) when the wind-up is done; let go sooner, it fires what it has (StopFire).
		UPartEffectComponent::For(*this)->BeginCharge();
		Timers.SetTimer(FireTimer, this, &AWeaponBase::HandleFiring, Trigger.ChargeSeconds / GetOwnerTimeScale(), false);
	}
	else if (Press == WeaponTrigger::EPress::FireNow)
	{
		HandleFiring();
	}
	else
	{
		Timers.SetTimer(FireTimer, this, &AWeaponBase::HandleFiring, static_cast<float>(SecondsUntilReady), false);
	}
}

bool AWeaponBase::HasShotPending() const
{
	const UWorld* World = GetWorld();
	return World && World->GetTimerManager().IsTimerActive(FireTimer);
}

void AWeaponBase::StopFire()
{
	GunshotSound::PlayBurstEnd(*this);
	bWantsToFire = false;
	StopKindTrigger();

	// Bursts finish on their own and semi-auto keeps its one remembered press (made just before the gun was ready), so only
	// auto stops immediately.
	// Without a world (its holder ending play as the world is collected) there's no timer left to clear: asking for the
	// timer manager then crashed the editor between test batches.
	UWorld* World = GetWorld();
	if (!World || !Instance.Definition)
	{
		return;
	}
	// An Overcharge gun let go mid wind-up fires at the charge it reached (UPartEffectComponent::TakeChargeScale).
	UPartEffectComponent* Parts = UPartEffectComponent::Find(this);
	if (Parts && Parts->IsCharging() && World->GetTimerManager().IsTimerActive(FireTimer))
	{
		World->GetTimerManager().ClearTimer(FireTimer);
		HandleFiring();
		return;
	}
	if (WeaponTrigger::PlanFor(Instance).FireMode == EWeaponFireMode::FullAuto)
	{
		World->GetTimerManager().ClearTimer(FireTimer);
	}
}

void AWeaponBase::HandleFiring()
{
	if (bReloading || bMeleeSwinging || !Instance.Definition)
	{
		return;
	}
	// Died with the trigger held, a burst under way or a shot waiting on the cycle: it all stops here.
	if (IsHolderDead(GetOwner()))
	{
		BurstShotsRemaining = 0;
		// An Overcharge wind-up is dropped, not let go (StopFire would fire what it reached).
		if (UPartEffectComponent* Parts = UPartEffectComponent::Find(this))
		{
			Parts->CancelCharge();
		}
		GetWorldTimerManager().ClearTimer(FireTimer);
		StopFire();
		return;
	}

	if (CurrentMagazine <= 0)
	{
		PlayCue(WeaponSounds::DryFire(Instance.Definition->Kind), Instance.Definition->DryFireSound);
		BurstShotsRemaining = 0;
		Reload();
		return;
	}

	// A part effect's plan for this pull (WeaponTrigger::PlanFor: a Burst part's rounds and their gap; StartFireAt winds an
	// Overcharge's charge, which the bullets take as the shot goes: UPartEffectSubsystem).
	const WeaponTrigger::FTriggerPlan Trigger = WeaponTrigger::PlanFor(Instance);
	// A cursed iron's drawback acts on the trigger: Greedy spends two rounds a shot (or the last one alone), and Unlucky
	// now and then drops the hammer on a dud. Either way the shot takes its time and its rounds.
	const WeaponCurseEffects::FShotPlan Plan = WeaponCurseEffects::PlanShot(Instance, CurrentMagazine, ShotRolls);
	if (Plan.bMisfire)
	{
		Misfire();
	}
	else
	{
		FireShot();
	}
	// The wind-up is spent with the shot (a bullet took its charge; a dud or a rocket lets it go).
	if (UPartEffectComponent* Parts = UPartEffectComponent::Find(this))
	{
		Parts->CancelCharge();
	}
	CurrentMagazine = FMath::Max(CurrentMagazine - Plan.Rounds, 0);
	// A six-gun keeps its empties (a dud's too) until a reload tips them out.
	SpentInCylinder = FMath::Min(SpentInCylinder + Plan.Rounds, Instance.Stats.MagazineSize);
	LastFireTime = GetWorld()->GetTimeSeconds();
	// The frontier-tech kinds' after-shot: the sniper's bolt cycles (a dud's too), the LMG heats, the launcher's mouth empties.
	NoteKindShot(Plan.bMisfire);
	// A revolver's cylinder turns the next round under the hammer (a dud's too), as the gun settles from the kick.
	if (bUsingModel && Model->GetCylinderChambers() > 0)
	{
		Model->TurnCylinder(Plan.Rounds);
		RefreshTick();
	}

	BroadcastAmmo();
	if (!Plan.bMisfire)
	{
		// A dud doesn't kick, and the crosshair doesn't bloom for it.
		OnFired.Broadcast();
		// The click of the magazine's last rounds (the last one is the reload's own, so none then).
		GunshotSound::PlayLowAmmo(*this, CurrentMagazine, Instance.Stats.MagazineSize);
	}

	if (CurrentMagazine <= 0)
	{
		BurstShotsRemaining = 0;
		Reload();
		return;
	}

	bool bContinue = false;
	switch (Trigger.FireMode)
	{
	case EWeaponFireMode::FullAuto:
		bContinue = bWantsToFire;
		break;
	case EWeaponFireMode::Burst:
		bContinue = --BurstShotsRemaining > 0;
		break;
	default:
		break;
	}

	if (bContinue)
	{
		const float Gap = Trigger.FireMode == EWeaponFireMode::Burst ? Trigger.BurstIntervalScale : 1.f;
		GetWorldTimerManager().SetTimer(FireTimer, this, &AWeaponBase::HandleFiring, GetShotInterval() * Gap / GetOwnerTimeScale(), false);
	}
}

void AWeaponBase::FireShot()
{
	FVector ViewLocation;
	FRotator ViewRotation;
	GetAimViewPoint(ViewLocation, ViewRotation);
	const FVector AimDirection = ViewRotation.Vector();
	UE_LOG(LogLooter, VeryVerbose, TEXT("%s shot from %s along %s"), *GetName(), *ViewLocation.ToCompactString(), *AimDirection.ToCompactString());

	// Every pellet is a real bullet that flies from the aim point along the aim (the tracer leaves from the muzzle). A
	// launcher's shot is a rocket instead (AWeaponRocket), off the same aim, its cone the gun's spread as a pellet's.
	const UWeaponDefinition* Definition = Instance.Definition;
	if (Definition->Kind == EWeaponKind::Launcher)
	{
		const float SpreadRadians = FMath::DegreesToRadians(GetEffectiveSpread());
		FireRocket(ViewLocation, SpreadRadians > 0.f ? FMath::VRandCone(AimDirection, SpreadRadians) : AimDirection);
	}
	else if (UBulletSubsystem* Bullets = GetWorld()->GetSubsystem<UBulletSubsystem>())
	{
		FBulletShot Shot;
		Shot.Start = ViewLocation;
		Shot.VisualStart = GetVisibleMuzzleLocation();
		Shot.Speed = Definition->BulletSpeed;
		Shot.Range = Instance.Stats.Range * FWeaponStats::MaxRangeFactor;
		Shot.FalloffStats = Instance.Stats;
		Shot.Damage = Instance.Stats.Damage;
		// A cursed iron's critical hits (Unlucky's triple, Cold's extra) when larger than the kind's own: the revolver's
		// 1.75x, the sniper's 2x, the game's 1.5x for the rest.
		Shot.CritMultiplier = GunfeelKick::CritMultiplier(Definition->Kind, WeaponCurses::CritMultiplier(Instance));
		Shot.HitImpulse = Definition->HitImpulse;
		Shot.Channel = TraceChannel;
		Shot.TracerColor = Definition->TracerColor;
		Shot.TracerWidth = Definition->TracerWidth;
		Shot.TracerLength = Definition->TracerLength;
		// The kind's tracer (the VFX pass): an SMG's needles, a sniper's bright line, an LMG's every third round.
		const FVfxTracerLook Tracer = VfxRules::TracerLook(Definition->Kind, CurrentMagazine);
		Shot.TracerColor *= Tracer.Glow;
		Shot.TracerWidth *= Tracer.Width;
		Shot.TracerLength *= Tracer.Length;
		Shot.ImpactFX = Definition->ImpactFX.Get();
		Shot.bDrawDebug = bDrawDebugTraces;
		Shot.Weapon = this;
		Shot.Shooter = GetOwner();
		Shot.Instigator = GetInstigatorController();

		const float SpreadRadians = FMath::DegreesToRadians(GetEffectiveSpread());
		for (int32 Pellet = 0; Pellet < Instance.Stats.PelletsPerShot; ++Pellet)
		{
			Shot.Direction = SpreadRadians > 0.f ? FMath::VRandCone(AimDirection, SpreadRadians) : AimDirection;
			Bullets->Fire(Shot);
		}
	}
	// The next shot's cone is wider (a rifle's bloom).
	AddShotBloom();

	if (Definition->MuzzleFlashFX)
	{
		if (bUsingModel)
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(Definition->MuzzleFlashFX, Model, NAME_None,
				Model->GetMuzzle(), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
		}
		else
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(Definition->MuzzleFlashFX, GetActiveMesh(), Definition->MuzzleSocket,
				FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
		}
	}
	else
	{
		PlayMuzzleFlash();
	}
	GunshotSound::PlayShot(*this);
	// What a shot leaves: its brass, and heat in the barrel that smokes once firing stops.
	EjectAfterShot();
	NoteShotHeat();
}

void AWeaponBase::Misfire()
{
	// No bullet and no flash: the primer pops weakly, spitting a few sparks and a wisp of smoke from the muzzle as the
	// player sees it.
	if (UBulletSubsystem* Bullets = GetWorld()->GetSubsystem<UBulletSubsystem>())
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		GetAimViewPoint(ViewLocation, ViewRotation);
		Bullets->GetEffects().Initialize(GetWorld());
		Bullets->GetEffects().SpawnFlash(GetVisibleMuzzleLocation(), ViewRotation.Vector(), MisfirePuffScale);
	}
	PlayCue(LooterSoundCue::Misfire);
	UE_LOG(LogLooter, Verbose, TEXT("%s misfired"), *GetName());
}

void AWeaponBase::PlayCue(FName Cue, USoundBase* Override) const
{
	// A definition that sets a sound of its own keeps it.
	if (Override)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Override, GetMuzzleLocation());
		return;
	}
	// On the gun, so it follows it: the player's own shots stay with the view as it turns.
	LooterSound::PlayAttached(Cue, GetRootComponent());
}

void AWeaponBase::PlayAimIn() const
{
	// Each kind's own raise (Weapon.<Kind>.AimIn), the shared one when the bank has none or the identity switch is off.
	PlayCue(GunshotSound::AimInCue(this, GunfeelKick::KindOf(this)));
}

void AWeaponBase::PlayAimOut() const
{
	// Letting go of the sights with them up: a short lowering (none with the identity switch off, as before).
	const FName Cue = GunshotSound::AimOutCue(this);
	if (!Cue.IsNone())
	{
		PlayCue(Cue);
	}
}

void AWeaponBase::NotifyBulletHit(const FHitResult& Hit, float Damage, bool bCritical)
{
	OnHit.Broadcast(Hit, Damage, bCritical);
}

FVector AWeaponBase::GetVisibleMuzzleLocation() const
{
	return GetVisibleLocation(GetMuzzleLocation());
}

FVector AWeaponBase::GetVisibleLocation(const FVector& WorldLocation) const
{
	const UCameraComponent* Camera = UPlayerViewComponent::FindFirstPersonCamera(GetOwner());
	if (!Camera || !IsDrawnFirstPerson())
	{
		return WorldLocation;
	}
	// First-person guns are drawn with their own field of view and pulled in toward the camera: find where the point
	// actually shows, the way the renderer does (the muzzle's flash and tracers, the port's brass).
	FMinimalViewInfo View;
	View.Location = Camera->GetComponentLocation();
	View.Rotation = Camera->GetComponentRotation();
	View.FOV = Camera->FieldOfView;
	View.FirstPersonFOV = Camera->bEnableFirstPersonFieldOfView ? Camera->FirstPersonFieldOfView : Camera->FieldOfView;
	View.FirstPersonScale = Camera->bEnableFirstPersonScale ? Camera->FirstPersonScale : 1.f;
	View.bUseFirstPersonParameters = Camera->bEnableFirstPersonFieldOfView || Camera->bEnableFirstPersonScale;
	return View.bUseFirstPersonParameters ? View.TransformWorldToFirstPerson(WorldLocation, false) : WorldLocation;
}

float AWeaponBase::GetEffectiveSpread() const
{
	const AActor* Holder = GetOwner();
	const UPlayerLocomotionComponent* Locomotion = Holder ? Holder->FindComponentByClass<UPlayerLocomotionComponent>() : nullptr;
	const UPlayerViewComponent* View = Holder ? Holder->FindComponentByClass<UPlayerViewComponent>() : nullptr;
	// The sniper is a scope gun: its spread is its hip spread until the scope is up, then all but none (no halving mercy).
	const bool bSniper = Instance.Definition && Instance.Definition->Kind == EWeaponKind::Sniper;
	const float Aimed = !View ? 1.f : (bSniper ? GunKinds::AimSpreadScale(EWeaponKind::Sniper, View->GetAimAlpha(),
		UPlayerViewComponent::AimSpreadMultiplier, GunKinds::SniperAimedSpread()) : View->GetAimSpreadMultiplier());
	return Instance.Stats.Spread * (Locomotion ? Locomotion->GetSpreadMultiplier() : 1.f) * Aimed * (1.f + GetSpreadBloom());
}

float AWeaponBase::GetOwnerTimeScale() const
{
	const AActor* Carrier = GetOwner();
	return Carrier && Carrier->CustomTimeDilation > UE_KINDA_SMALL_NUMBER ? Carrier->CustomTimeDilation : 1.f;
}

float AWeaponBase::GetReadyAlpha() const
{
	const UWorld* World = GetWorld();
	return World && ReadySeconds > 0.f ? FMath::Clamp(static_cast<float>(World->GetTimeSeconds() - DrawnTime) / ReadySeconds, 0.f, 1.f) : 1.f;
}

FVector AWeaponBase::GetAimPoint() const
{
	return bUsingModel ? Model->GetRelativeTransform().TransformPosition(Model->GetAimPoint()) : FVector(30.f, 0.f, 8.f);
}

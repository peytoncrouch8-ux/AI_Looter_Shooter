#include "Weapons/WeaponBase.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Weapons/WeaponCurseEffects.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponSounds.h"
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

// ---------------------------------------------------------------------------
// Ammo
// ---------------------------------------------------------------------------

float AWeaponBase::GetReloadProgress() const
{
	if (!bReloading)
	{
		return -1.f;
	}
	const FTimerManager& Timers = GetWorldTimerManager();
	const float Rate = Timers.GetTimerRate(ReloadTimer);
	return Rate > 0.f ? FMath::Clamp(Timers.GetTimerElapsed(ReloadTimer) / Rate, 0.f, 1.f) : 0.f;
}

float AWeaponBase::GetReloadBlend() const
{
	const float Progress = GetReloadProgress();
	if (Progress < 0.f)
	{
		return 0.f;
	}
	// Ease in over the first ~12% of the reload and out over the last ~12%.
	return FMath::SmoothStep(0.f, 1.f, FMath::Clamp(FMath::Min(Progress, 1.f - Progress) * 8.f, 0.f, 1.f));
}

bool AWeaponBase::CanReload() const
{
	return !bReloading && GetReserveAmmo() > 0 && CurrentMagazine < Instance.Stats.MagazineSize;
}

void AWeaponBase::Reload()
{
	// Mid-strike, a reload asked for waits for the swing to end (EndMeleeSwing starts it).
	if (bMeleeSwinging)
	{
		bReloadAfterMelee = bReloadAfterMelee || CanReload();
		return;
	}
	if (!CanReload())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(FireTimer);
	BurstShotsRemaining = 0;
	// An LMG spins down through it; a bolt still cycling is taken over by the reload's own (its case flies as it opens).
	StopKindTrigger();
	bBoltCycling = false;
	ApplyCyclePose(FVector::ZeroVector, FRotator::ZeroRotator);
	bReloading = true;
	ReloadSoundProgress = 0.f;
	RefreshTick();

	// A definition's own reload sound plays whole, in place of the steps' cues (PlayReloadSteps).
	if (Instance.Definition && Instance.Definition->ReloadSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Instance.Definition->ReloadSound, GetActorLocation());
	}

	const float Duration = FMath::Max(Instance.Stats.ReloadTime, 0.01f);
	OnReloadStarted.Broadcast(Duration);
	// The timer runs on the world's clock: a reloader living faster than the world (Sundown) finishes in fewer of its seconds.
	GetWorldTimerManager().SetTimer(ReloadTimer, this, &AWeaponBase::FinishReload, Duration / GetOwnerTimeScale(), false);
}

void AWeaponBase::PlayReloadSteps(float Progress)
{
	if (Progress <= ReloadSoundProgress)
	{
		return;
	}
	// A definition's own reload sound plays whole, in place of the steps' cues (the steps still happen).
	const bool bOwnSound = Instance.Definition && Instance.Definition->ReloadSound;
	// Each step as the motion reaches it (LooterReload choreographs both), so the clicks land on the hands at any reload time.
	for (const FReloadStepAt& Step : LooterReload::Steps(GetReloadPart()))
	{
		if (Step.Progress > ReloadSoundProgress && Step.Progress <= Progress)
		{
			if (!bOwnSound)
			{
				PlayCue(WeaponSounds::ReloadStep(Step.Step));
			}
			// The rod's punch: the empties tumble out of the cylinder.
			if (Step.Step == EReloadStep::Eject)
			{
				DumpEmpties();
			}
			// The bolt drawn back: the last shot's case, if its own cycle never got to throw it.
			if (Step.Step == EReloadStep::BoltOpen && !bBoltCaseOut)
			{
				bBoltCaseOut = true;
				EjectCasing();
			}
		}
	}
	ReloadSoundProgress = Progress;
}

void AWeaponBase::FinishReload()
{
	// The last steps, if the reload ended between two frames.
	PlayReloadSteps(1.f);

	// Reloads draw from the holder's shared pool for this ammo class.
	UWeaponManagerComponent* Inventory = GetHolderInventory();
	const int32 Needed = Instance.Stats.MagazineSize - CurrentMagazine;
	CurrentMagazine += Inventory ? Inventory->TakeAmmo(GetAmmoType(), Needed) : 0;
	bReloading = false;
	RefreshTick();
	UpdateReloadPart();
	// A Hungry iron feeds on its holder for the rounds it took.
	WeaponCurseEffects::PayReload(*this);

	OnReloadFinished.Broadcast();
	BroadcastAmmo();

	// Holding the trigger through a reload resumes auto fire.
	if (bWantsToFire && Instance.Definition && Instance.Definition->FireMode == EWeaponFireMode::FullAuto)
	{
		StartFire();
	}
}

void AWeaponBase::CancelReload()
{
	GetWorldTimerManager().ClearTimer(ReloadTimer);
	bReloading = false;
	RefreshTick();
	UpdateReloadPart();
}

void AWeaponBase::UpdateReloadPart()
{
	const EWeaponReloadPart Part = GetReloadPart();
	if (Part == EWeaponReloadPart::None)
	{
		return;
	}
	const float Progress = GetReloadProgress();
	if (Part == EWeaponReloadPart::Rocket)
	{
		// The launcher: the loaded rocket's nose shows while one's in; through a reload the next one is brought up in front
		// of the mouth and slid in, and once it seats the nose shows again.
		bool bShown = false;
		bool bSeated = false;
		const float RocketOut = Progress >= 0.f ? LooterReload::RocketTravel(Progress, bShown, bSeated) : 0.f;
		Model->SetRocketLoad(RocketOut, bShown, Progress >= 0.f ? bSeated : CurrentMagazine > 0);
		return;
	}
	bool bVisible = true;
	float Travel = 0.f;
	if (Progress >= 0.f)
	{
		switch (Part)
		{
		case EWeaponReloadPart::Magazine:
		case EWeaponReloadPart::Helix:
		case EWeaponReloadPart::Box:
		case EWeaponReloadPart::Bolt:
			Travel = LooterReload::WellTravel(Part, Progress, bVisible);
			break;
		case EWeaponReloadPart::Cylinder:
			Travel = LooterReload::CylinderSwing(Progress);
			break;
		case EWeaponReloadPart::Pump:
		default:
			Travel = LooterReload::PumpTravel(Progress);
			break;
		}
	}
	Model->SetReloadTravel(Travel, bVisible);
	// The sniper's bolt: opened and run home by the reload; between reloads its own cycle moves it (UpdateBoltCycle).
	if (Part == EWeaponReloadPart::Bolt && (Progress >= 0.f || !bBoltCycling))
	{
		Model->SetBoltOpen(Progress >= 0.f ? LooterReload::BoltOpenInReload(Progress) : 0.f);
	}
}

void AWeaponBase::BroadcastAmmo()
{
	OnAmmoChanged.Broadcast(CurrentMagazine, GetReserveAmmo());
	// The SMG's helix shows the rounds left.
	RefreshGauges();
}

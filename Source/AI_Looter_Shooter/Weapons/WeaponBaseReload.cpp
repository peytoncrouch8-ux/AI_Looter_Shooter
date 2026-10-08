#include "Weapons/WeaponBase.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Weapons/WeaponCurseEffects.h"
#include "Weapons/WeaponDefinition.h"
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
	/** The sound of a reload's step. */
	FName ReloadStepCue(EReloadStep Step)
	{
		switch (Step)
		{
		case EReloadStep::MagOut:
			return LooterSoundCue::RifleMagOut;
		case EReloadStep::MagIn:
			return LooterSoundCue::RifleMagIn;
		case EReloadStep::Bolt:
			return LooterSoundCue::RifleBolt;
		case EReloadStep::ShellIn:
			return LooterSoundCue::ShotgunShellIn;
		case EReloadStep::Pump:
			return LooterSoundCue::ShotgunPump;
		default:
			return NAME_None;
		}
	}
}

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
	if (!CanReload())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(FireTimer);
	BurstShotsRemaining = 0;
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
	GetWorldTimerManager().SetTimer(ReloadTimer, this, &AWeaponBase::FinishReload, Duration, false);
}

void AWeaponBase::PlayReloadSteps(float Progress)
{
	if (Progress <= ReloadSoundProgress || (Instance.Definition && Instance.Definition->ReloadSound))
	{
		return;
	}
	// Each step as the motion reaches it (LooterReload choreographs both), so the clicks land on the hands at any reload time.
	for (const FReloadStepAt& Step : LooterReload::Steps(GetReloadPart()))
	{
		if (Step.Progress > ReloadSoundProgress && Step.Progress <= Progress)
		{
			PlayCue(ReloadStepCue(Step.Step));
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
	bool bVisible = true;
	float Travel = 0.f;
	if (Progress >= 0.f)
	{
		Travel = Part == EWeaponReloadPart::Magazine ? LooterReload::MagazineTravel(Progress, bVisible) : LooterReload::PumpTravel(Progress);
	}
	Model->SetReloadTravel(Travel, bVisible);
}

void AWeaponBase::BroadcastAmmo()
{
	OnAmmoChanged.Broadcast(CurrentMagazine, GetReserveAmmo());
}

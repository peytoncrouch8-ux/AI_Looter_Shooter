#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

// AWeaponBase's side of a melee strike (UPlayerMeleeComponent): while the stock swings the gun neither fires nor reloads,
// its visible model takes the swing's pose, and afterwards what the swing held back follows: a reload it cut short starts
// over (Borderlands' way), or auto fire takes up again under a held trigger.

bool AWeaponBase::BeginMeleeSwing()
{
	const bool bCutReload = bReloading;
	// A shot queued by a spammed trigger, auto fire or the rest of a burst: none goes off mid-swing.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireTimer);
	}
	BurstShotsRemaining = 0;
	if (bCutReload)
	{
		CancelReload();
	}
	bMeleeSwinging = true;
	bReloadAfterMelee = bCutReload;
	return bCutReload;
}

void AWeaponBase::EndMeleeSwing(bool bResume)
{
	if (!bMeleeSwinging)
	{
		return;
	}
	bMeleeSwinging = false;
	SetMeleePose(FVector::ZeroVector, FRotator::ZeroRotator);
	const bool bReload = bReloadAfterMelee;
	bReloadAfterMelee = false;
	if (!bResume || bIsPickup)
	{
		return;
	}
	// The reload starts over from the top: the strike was the price of it.
	if (bReload && CanReload())
	{
		Reload();
		return;
	}
	// A trigger held through the swing takes auto fire up again, as after a reload.
	if (bWantsToFire && Instance.Definition && Instance.Definition->FireMode == EWeaponFireMode::FullAuto)
	{
		StartFire();
	}
}

void AWeaponBase::SetMeleePose(const FVector& Offset, const FRotator& Rotation)
{
	USceneComponent* Mesh = GetActiveMesh();
	if (!Mesh)
	{
		return;
	}
	// Only the first-person gun swings on its own; a third-person body has no strike to carry it, so the gun stays in its
	// hands rather than twisting there. Loot is never touched (its model sits off its middle to spin).
	const bool bPose = !bIsPickup && IsDrawnFirstPerson() && !(Offset.IsNearlyZero(0.01) && Rotation.IsNearlyZero(0.01f));
	if (!bPose)
	{
		if (bMeleePoseApplied)
		{
			Mesh->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
			bMeleePoseApplied = false;
		}
		return;
	}
	// Turned about the middle of the hands (between grip and foregrip), so the stock behind them swings out across the
	// view while the hands stay about where they were.
	const FVector Pivot = (GetGripPoint() + GetForegripPoint()) * 0.5f;
	const FQuat Turn = Rotation.Quaternion();
	Mesh->SetRelativeLocationAndRotation(Offset + Pivot - Turn.RotateVector(Pivot), Turn);
	bMeleePoseApplied = true;
}

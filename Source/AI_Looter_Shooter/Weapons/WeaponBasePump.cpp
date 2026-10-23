#include "Weapons/WeaponBase.h"
#include "Weapons/GunfeelKickTuning.h"
#include "Weapons/GunshotSound.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/ReloadTuning.h"

// AWeaponBase's pump racked between shots (the gun feel pass; the user's 2026-10-22 yes): after each Ranchhand shot the
// fore-end is snapped back, throwing the hull as it bottoms (the rack's sound and view beat there too), and shoved home
// before the gun can fire again; the first-person gun rocks with it (ApplyCyclePose, as the sniper's bolt). The rack is
// timed by the shot interval (ReloadTuning::RackTiming) and counted down by the gun's own ticks (PendingEjectSeconds),
// so a test that ticks the gun by hand sees it too.

float AWeaponBase::GetHullDelay() const
{
	const ReloadTuning::FRackTiming Rack = ReloadTuning::RackTiming(GetShotInterval() / FMath::Max(GetOwnerTimeScale(), KINDA_SMALL_NUMBER));
	return Rack.Start + Rack.Seconds * LooterReload::PumpCycleBackAt;
}

float AWeaponBase::GetPumpCycle() const
{
	if (PendingEjectSeconds < 0.f || PumpRackSeconds <= 0.f)
	{
		return -1.f;
	}
	// The countdown runs from the shot to the rack's end.
	const float SinceShot = PumpRackStart + PumpRackSeconds - PendingEjectSeconds;
	const float Cycle = (SinceShot - PumpRackStart) / PumpRackSeconds;
	return Cycle >= 0.f && Cycle < 1.f ? Cycle : -1.f;
}

void AWeaponBase::StartPumpCycle()
{
	// A shot fired before the last rack threw its hull (a fast-firing roll): that hull goes first.
	if (PendingEjectSeconds >= 0.f && bHullInChamber)
	{
		EjectCasing();
	}
	const ReloadTuning::FRackTiming Rack = ReloadTuning::RackTiming(GetShotInterval() / FMath::Max(GetOwnerTimeScale(), KINDA_SMALL_NUMBER));
	PumpRackStart = Rack.Start;
	PumpRackSeconds = FMath::Max(Rack.Seconds, KINDA_SMALL_NUMBER);
	PendingEjectSeconds = Rack.End();
	bHullInChamber = true;
	bRackBeatDone = false;
	RefreshTick();
}

void AWeaponBase::EndPumpCycle(bool bThrowHull)
{
	if (PendingEjectSeconds >= 0.f)
	{
		PendingEjectSeconds = -1.f;
		ApplyCyclePose(FVector::ZeroVector, FRotator::ZeroRotator);
	}
	if (bThrowHull && bHullInChamber)
	{
		bHullInChamber = false;
		EjectCasing();
	}
}

void AWeaponBase::UpdateBrass(float DeltaSeconds)
{
	if (PendingEjectSeconds < 0.f)
	{
		return;
	}
	PendingEjectSeconds -= DeltaSeconds;
	const bool bDone = PendingEjectSeconds < 0.f;
	const float Cycle = GetPumpCycle();
	// Fully back: the hull flies, and the rack is heard and felt on that beat (the sound pass and the kick pass time to it).
	if (!bRackBeatDone && (bDone || Cycle >= LooterReload::PumpCycleBackAt))
	{
		bRackBeatDone = true;
		if (bHullInChamber)
		{
			bHullInChamber = false;
			EjectCasing();
		}
		GunshotSound::PlayRack(*this);
		GunfeelKick::RackBeat(*this);
	}
	if (bDone)
	{
		// Home (whatever a long frame skipped): the fore-end and the first-person gun back in place.
		PendingEjectSeconds = -1.f;
		ApplyCyclePose(FVector::ZeroVector, FRotator::ZeroRotator);
		UpdateReloadPart();
		return;
	}
	FVector Offset;
	FRotator Rotation;
	LooterReload::PumpCycleViewPose(Cycle, Offset, Rotation);
	ApplyCyclePose(Offset, Rotation);
}

#include "Weapons/WeaponBase.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/HealthComponent.h"
#include "Weapons/ReloadTuning.h"
#include "Weapons/WeaponCurseEffects.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponSounds.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/WeaponModelComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

// AWeaponBase's magazine and reload, as planned when it starts (ReloadTuning, LooterReload::StepsFor): tactical or from
// empty, the rounds in as the magazine seats, ready a beat before the motion ends; a pump gun's shells one at a time, a
// round each, cut short by a trigger press after the shell in hand. The reload's clock is its timer (a test world ticks
// the timer manager by hand), carried across a re-plan or into the last beat by ReloadClockOffset.

// ---------------------------------------------------------------------------
// Where the reload is
// ---------------------------------------------------------------------------

float AWeaponBase::GetReloadRawProgress() const
{
	if ((!bReloading && !bReloadTail) || ReloadMotionSeconds <= 0.f)
	{
		return -1.f;
	}
	const FTimerManager& Timers = GetWorldTimerManager();
	const bool bTimer = Timers.IsTimerActive(ReloadTimer) || Timers.IsTimerPending(ReloadTimer);
	// The last beat after ready ends early once the gun fires again (the shot's own kick takes over).
	if (bReloadTail && (!bTimer || LastFireTime >= ReloadReadyTime))
	{
		return -1.f;
	}
	const float Elapsed = bTimer ? FMath::Max(Timers.GetTimerElapsed(ReloadTimer), 0.f) : 0.f;
	return FMath::Clamp((ReloadClockOffset + Elapsed) / ReloadMotionSeconds, 0.f, 1.f);
}

float AWeaponBase::GetReloadProgress() const
{
	const float Raw = GetReloadRawProgress();
	return Raw < 0.f ? -1.f : LooterReload::ChoreographyProgress(GetReloadPart(), ReloadPlan, Raw);
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
	// A dead holder reloads nothing until the respawn gives their health back.
	const UHealthComponent* Health = GetOwner() ? GetOwner()->FindComponentByClass<UHealthComponent>() : nullptr;
	return !bReloading && GetReserveAmmo() > 0 && CurrentMagazine < Instance.Stats.MagazineSize && !(Health && Health->IsDead());
}

// ---------------------------------------------------------------------------
// The reload
// ---------------------------------------------------------------------------

void AWeaponBase::PlanReloadTimes()
{
	const UWeaponDefinition* Definition = Instance.Definition;
	const EWeaponKind Kind = Definition ? Definition->Kind : EWeaponKind::None;
	// A pump gun's seconds scale with its rolled reload time over its definition's, so its parts and maker still count.
	const float GunScale = ReloadTuning::GunScale(Instance.Stats.ReloadTime, Definition ? Definition->BaseStats.ReloadTime : 0.f);
	const float Motion = ReloadTuning::MotionSeconds(GetReloadPart(), ReloadPlan, FMath::Max(Instance.Stats.ReloadTime, 0.01f), GunScale,
		ReloadTuning::TimeScale(Kind, ReloadPlan.bEmpty), ReloadTuning::ShellTiming());
	// On the world's clock: a reloader living faster than the world (Sundown) finishes in fewer of its seconds.
	const float TimeScale = FMath::Max(GetOwnerTimeScale(), KINDA_SMALL_NUMBER);
	ReloadMotionSeconds = Motion / TimeScale;
	ReloadReadySeconds = ReloadTuning::ReadySeconds(Motion, ReloadTuning::ReadyEarly()) / TimeScale;
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
	// An LMG spins down through it.
	StopKindTrigger();
	const EWeaponReloadPart Part = GetReloadPart();
	ReloadPlan = ReloadTuning::PlanFor(Part, CurrentMagazine, Instance.Stats.MagazineSize, GetReserveAmmo());
	// A bolt still cycling is taken over by the reload's own (its case flies as it opens); a tactical reload never opens
	// it, so the case goes now.
	if (bBoltCycling && !bBoltCaseOut && !ReloadPlan.bEmpty)
	{
		bBoltCaseOut = true;
		EjectCasing();
	}
	bBoltCycling = false;
	// A pump still racking after the shot stops: from empty its hull waits in the chamber for the reload's pump, else it's out now.
	EndPumpCycle(!ReloadPlan.bEmpty);
	ApplyCyclePose(FVector::ZeroVector, FRotator::ZeroRotator);

	LooterReload::StepsFor(Part, ReloadPlan, ReloadSteps);
	bReloading = true;
	bReloadTail = false;
	bReloadCommitted = false;
	bFireAfterReload = false;
	ReloadSoundProgress = 0.f;
	ReloadClockOffset = 0.f;
	PlanReloadTimes();
	RefreshTick();

	// A definition's own reload sound plays whole, in place of the steps' cues (PlayReloadSteps); else glove and sling first.
	if (Instance.Definition && Instance.Definition->ReloadSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Instance.Definition->ReloadSound, GetActorLocation());
	}
	else
	{
		PlayCue(WeaponSounds::ReloadStart());
	}

	// The HUD's bar counts the reloader's own seconds to ready, as the timer counts the world's.
	OnReloadStarted.Broadcast(ReloadReadySeconds * GetOwnerTimeScale());
	GetWorldTimerManager().SetTimer(ReloadTimer, this, &AWeaponBase::FinishReload, FMath::Max(ReloadReadySeconds, 0.01f), false);
	LogReload(TEXT("start"));
}

bool AWeaponBase::InterruptReload()
{
	if (!bReloading || bMeleeSwinging || GetReloadPart() != EWeaponReloadPart::Pump || ReloadPlan.Shells <= 0
		|| !ReloadTuning::ShotgunInterrupt())
	{
		return false;
	}
	bFireAfterReload = true;
	// The shell in hand goes home (and the pump from empty), the rest stay on the belt. The beats are the same seconds in
	// the shorter plan, so the motion carries on from where it is.
	const float Raw = FMath::Max(GetReloadRawProgress(), 0.f);
	const ReloadTuning::FShellTiming Timing = ReloadTuning::ShellTiming();
	const int32 Kept = ReloadTuning::ShellsKept(ReloadPlan, Raw * ReloadTuning::ShellReloadSeconds(ReloadPlan, Timing), Timing);
	if (Kept < ReloadPlan.Shells)
	{
		const float Elapsed = Raw * ReloadMotionSeconds;
		ReloadPlan.Shells = Kept;
		LooterReload::StepsFor(EWeaponReloadPart::Pump, ReloadPlan, ReloadSteps);
		PlanReloadTimes();
		ReloadClockOffset = Elapsed;
		ReloadSoundProgress = ReloadMotionSeconds > 0.f ? FMath::Clamp(Elapsed / ReloadMotionSeconds, 0.f, 1.f) : 0.f;
		GetWorldTimerManager().SetTimer(ReloadTimer, this, &AWeaponBase::FinishReload, FMath::Max(ReloadReadySeconds - Elapsed, 0.01f), false);
	}
	LogReload(TEXT("interrupted"));
	return true;
}

void AWeaponBase::PlayReloadSteps(float Progress)
{
	if (Progress <= ReloadSoundProgress)
	{
		return;
	}
	// A definition's own reload sound plays whole, in place of the steps' cues (the steps still happen).
	const bool bOwnSound = Instance.Definition && Instance.Definition->ReloadSound;
	const EWeaponKind Kind = Instance.Definition ? Instance.Definition->Kind : EWeaponKind::None;
	const EWeaponReloadPart Part = GetReloadPart();
	const bool bCommitAtSeat = ReloadTuning::CommitAtSeat();
	const float From = ReloadSoundProgress;
	ReloadSoundProgress = Progress;
	// Each step as the motion reaches it (LooterReload choreographs both), so the clicks land on the hands at any reload time.
	for (int32 Index = 0; Index < ReloadSteps.Num(); ++Index)
	{
		const FReloadStepAt Step = ReloadSteps[Index];
		if (Step.Progress <= From || Step.Progress > Progress)
		{
			continue;
		}
		if (!bOwnSound)
		{
			PlayCue(WeaponSounds::ReloadStep(Kind, Step.Step));
		}
		switch (Step.Step)
		{
		case EReloadStep::Eject:
			// The rod's punch: the empties tumble out of the cylinder.
			DumpEmpties();
			break;
		case EReloadStep::BoltOpen:
			// The bolt drawn back: the last shot's case, if its own cycle never got to throw it.
			if (!bBoltCaseOut)
			{
				bBoltCaseOut = true;
				EjectCasing();
			}
			break;
		case EReloadStep::Pump:
			// From empty: the pump throws the hull the last shot left and chambers the first shell.
			if (bHullInChamber)
			{
				bHullInChamber = false;
				EjectCasing();
			}
			break;
		default:
			break;
		}
		// The rounds go in where they seat: a pump gun's one a shell, any other's all at once as its magazine goes home.
		if (Part == EWeaponReloadPart::Pump && Step.Step == EReloadStep::ShellIn)
		{
			SeatShell();
		}
		else if (bCommitAtSeat && LooterReload::CommitsAt(Part, Step.Step))
		{
			CommitReloadRounds();
		}
	}
}

void AWeaponBase::CommitReloadRounds()
{
	if (bReloadCommitted)
	{
		return;
	}
	bReloadCommitted = true;
	// Reloads draw from the holder's shared pool for this ammo class.
	UWeaponManagerComponent* Inventory = GetHolderInventory();
	const int32 Needed = Instance.Stats.MagazineSize - CurrentMagazine;
	CurrentMagazine += Inventory && Needed > 0 ? Inventory->TakeAmmo(GetAmmoType(), Needed) : 0;
	// A Hungry iron feeds on its holder for the rounds it took.
	WeaponCurseEffects::PayReload(*this);
	BroadcastAmmo();
	LogReload(TEXT("rounds in"));
}

void AWeaponBase::SeatShell()
{
	UWeaponManagerComponent* Inventory = GetHolderInventory();
	if (!Inventory || CurrentMagazine >= Instance.Stats.MagazineSize)
	{
		return;
	}
	const int32 Taken = Inventory->TakeAmmo(GetAmmoType(), 1);
	if (Taken <= 0)
	{
		return;
	}
	CurrentMagazine += Taken;
	// A Hungry iron's toll once a reload, as its first shell goes in.
	if (!bReloadCommitted)
	{
		bReloadCommitted = true;
		WeaponCurseEffects::PayReload(*this);
	}
	BroadcastAmmo();
}

void AWeaponBase::FinishReload()
{
	// The last steps, if the reload ended between two frames (or before them: the gun is ready a beat early).
	PlayReloadSteps(1.f);
	// Rounds not in yet (Looter.Gunfeel.Reload.CommitAtSeat 0, or a reload with no seat): now. A pump gun's are its shells.
	if (!bReloadCommitted && GetReloadPart() != EWeaponReloadPart::Pump)
	{
		CommitReloadRounds();
	}
	bReloading = false;
	// The motion's last beat runs on for the view and the hands, until it ends or the gun fires again.
	const float Tail = ReloadMotionSeconds - ReloadReadySeconds;
	const UWorld* World = GetWorld();
	ReloadReadyTime = World ? World->GetTimeSeconds() : 0.0;
	bReloadTail = Tail > KINDA_SMALL_NUMBER;
	if (bReloadTail)
	{
		ReloadClockOffset = ReloadReadySeconds;
		GetWorldTimerManager().SetTimer(ReloadTimer, this, &AWeaponBase::EndReloadTail, Tail, false);
	}
	RefreshTick();
	UpdateReloadPart();
	LogReload(TEXT("ready"));

	OnReloadFinished.Broadcast();
	BroadcastAmmo();

	if (bFireAfterReload)
	{
		// The press that cut a shell reload short fires now, held or not (a tap let go mid-shell still fires once).
		bFireAfterReload = false;
		const bool bHeld = bWantsToFire;
		StartFire();
		if (!bHeld)
		{
			StopFire();
		}
	}
	else if (bWantsToFire && Instance.Definition && Instance.Definition->FireMode == EWeaponFireMode::FullAuto)
	{
		// Holding the trigger through a reload resumes auto fire.
		StartFire();
	}
}

void AWeaponBase::EndReloadTail()
{
	bReloadTail = false;
}

void AWeaponBase::CancelReload()
{
	if (bReloading)
	{
		LogReload(TEXT("cut short"));
	}
	GetWorldTimerManager().ClearTimer(ReloadTimer);
	// Rounds already seated stay in (the magazine home, the shells pushed); the rest wait for the next reload.
	bReloading = false;
	bReloadTail = false;
	bFireAfterReload = false;
	// A hull the cut reload's pump would have thrown counts as racked out while the gun was away (none flies later).
	bHullInChamber = false;
	RefreshTick();
	UpdateReloadPart();
}

void AWeaponBase::LogReload(const TCHAR* What) const
{
	if (!ReloadTuning::Debug())
	{
		return;
	}
	const EWeaponKind Kind = Instance.Definition ? Instance.Definition->Kind : EWeaponKind::None;
	UE_LOG(LogLooter, Display, TEXT("Reload %s: %s (%s) %s, %d shells, motion %.2f s, ready %.2f s, %d/%d in, at %.2f"), What, *GetName(),
		*UEnum::GetValueAsString(Kind), ReloadPlan.bEmpty ? TEXT("empty") : TEXT("tactical"), ReloadPlan.Shells, ReloadMotionSeconds,
		ReloadReadySeconds, CurrentMagazine, Instance.Stats.MagazineSize, GetReloadProgress());
}

void AWeaponBase::UpdateReloadPart()
{
	const EWeaponReloadPart Part = GetReloadPart();
	if (Part == EWeaponReloadPart::None)
	{
		return;
	}
	// The gun's parts follow the reload while it's on; its last beat after ready is the view's and the hands', the parts home by then.
	const float Progress = bReloading ? GetReloadProgress() : -1.f;
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
	switch (Part)
	{
	case EWeaponReloadPart::Magazine:
	case EWeaponReloadPart::Helix:
	case EWeaponReloadPart::Box:
	case EWeaponReloadPart::Bolt:
		Travel = Progress >= 0.f ? LooterReload::WellTravel(Part, Progress, bVisible) : 0.f;
		break;
	case EWeaponReloadPart::Cylinder:
		Travel = Progress >= 0.f ? LooterReload::CylinderSwing(Progress) : 0.f;
		break;
	case EWeaponReloadPart::Pump:
	default:
		// The pump through a reload's pump, or racked between shots (WeaponBasePump.cpp).
		Travel = Progress >= 0.f ? LooterReload::PumpTravel(Progress, ReloadPlan) : LooterReload::PumpCycleTravel(GetPumpCycle());
		break;
	}
	Model->SetReloadTravel(Travel, bVisible);
	// The sniper's bolt: opened and run home by a reload from empty; between reloads its own cycle moves it (UpdateBoltCycle).
	if (Part == EWeaponReloadPart::Bolt && (Progress >= 0.f || !bBoltCycling))
	{
		Model->SetBoltOpen(Progress >= 0.f ? LooterReload::BoltOpenInReload(Progress, ReloadPlan) : 0.f);
	}
}

void AWeaponBase::BroadcastAmmo()
{
	OnAmmoChanged.Broadcast(CurrentMagazine, GetReserveAmmo());
	// The SMG's helix shows the rounds left.
	RefreshGauges();
}

// UWeaponManagerComponent: the equip slots opening with the player's progress (the user, 2026-10-31: two from the start,
// the third at level 3, the fourth once Ransom's Rest's main missions are all finished; GunSlotRules has the rule). Only a
// local player's slots are gated: anyone else (a test's holder, a creature) has every slot. The slots fill from the first,
// so "open" is a count: a gun never goes into a shut slot (HasFreeSlot), and a gun an older save put in a slot that is shut
// now stays there, in use, until it leaves (the user's "never lose a gun"; RestoreInventory). Here too the watch that tells
// the HUD the moment a slot opens.

#include "Inventory/WeaponManagerComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The local player behind a weapon manager's pawn: its controller's, else (unpossessed a moment: a death) the world's first. */
	ULocalPlayer* FindSlotPlayer(const UWeaponManagerComponent& Manager)
	{
		const APawn* Pawn = Cast<APawn>(Manager.GetOwner());
		const APlayerController* Controller = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
		if (ULocalPlayer* Player = Controller ? Controller->GetLocalPlayer() : nullptr)
		{
			return Player;
		}
		const UWorld* World = Manager.GetWorld();
		return World ? World->GetFirstLocalPlayerFromController() : nullptr;
	}
}

// ---------------------------------------------------------------------------
// The slots open now
// ---------------------------------------------------------------------------

TOptional<FGunSlotProgress> UWeaponManagerComponent::GetSlotProgress() const
{
	if (SlotProgressForTesting.IsSet())
	{
		return SlotProgressForTesting;
	}
	// Only the player's slots open with progress (bHeldByPlayer stays true past a death's unpossess).
	if (!bHeldByPlayer)
	{
		return TOptional<FGunSlotProgress>();
	}
	const UPlayerProgressionSubsystem* Progression = SlotProgression.Get();
	if (!Progression)
	{
		const ULocalPlayer* Player = FindSlotPlayer(*this);
		Progression = Player ? Player->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	}
	if (!Progression)
	{
		return TOptional<FGunSlotProgress>();
	}
	FGunSlotProgress Progress;
	Progress.Level = Progression->GetLevel();
	// The story from the level's missions (their data, every main mission of the area) against the session's campaign.
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	Progress.bStoryDone = Runner && GunSlotRules::IsStoryDone(Runner->GetDefinitions(), Runner->GetCampaign(), GunSlotRules::StoryArea());
	return Progress;
}

int32 UWeaponManagerComponent::GetUnlockedSlots() const
{
	const TOptional<FGunSlotProgress> Progress = GetSlotProgress();
	return Progress.IsSet() ? FMath::Clamp(GunSlotRules::OpenSlots(Progress.GetValue()), 1, MaxWeapons) : MaxWeapons;
}

bool UWeaponManagerComponent::HasFreeSlot() const
{
	// A load puts back what the save had in every slot it used (an older save's third gun below level 3 included).
	return Weapons.Num() < (bRestoringSlots ? MaxWeapons : GetUnlockedSlots());
}

EGunSlotLock UWeaponManagerComponent::GetSlotLock(int32 SlotIndex) const
{
	// A slot that holds a gun is in use, whatever the rule says now; no slot past the last isn't anything.
	if (SlotIndex < 0 || SlotIndex >= MaxWeapons || Weapons.IsValidIndex(SlotIndex))
	{
		return EGunSlotLock::Open;
	}
	const TOptional<FGunSlotProgress> Progress = GetSlotProgress();
	return Progress.IsSet() ? GunSlotRules::LockOf(SlotIndex, Progress.GetValue()) : EGunSlotLock::Open;
}

void UWeaponManagerComponent::HandleSlotKey(int32 SlotIndex)
{
	const EGunSlotLock Lock = GetSlotLock(SlotIndex);
	if (Lock == EGunSlotLock::Open)
	{
		// A slot's gun in hand (an empty open slot, or the one in hand, does nothing, as before).
		EquipSlot(SlotIndex);
		return;
	}
	// A shut slot's key says how it opens, so the key never just does nothing.
	OnMessage.Broadcast(GunSlotRules::LockedKeyMessage(SlotIndex, Lock));
	LooterSound::Play2D(this, LooterSoundCue::Denied);
}

// ---------------------------------------------------------------------------
// The moment a slot opens
// ---------------------------------------------------------------------------

void UWeaponManagerComponent::WatchSlotProgress(AController* Controller)
{
	const APlayerController* Player = Cast<APlayerController>(Controller);
	ULocalPlayer* LocalPlayer = Player ? Player->GetLocalPlayer() : nullptr;
	UPlayerProgressionSubsystem* Progression = LocalPlayer ? LocalPlayer->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	if (Progression && SlotProgression.Get() != Progression)
	{
		if (UPlayerProgressionSubsystem* Old = SlotProgression.Get())
		{
			Old->OnXPChanged.Remove(SlotXPHandle);
		}
		SlotXPHandle = Progression->OnXPChanged.AddUObject(this, &UWeaponManagerComponent::HandleSlotXPChanged);
		SlotProgression = Progression;
		SlotLevelSeen = Progression->GetLevel();
	}
	// The runner is the world's (made with it): the missions finished here.
	UMissionRunner* Runner = UMissionRunner::Get(this);
	if (Runner && SlotRunner.Get() != Runner)
	{
		if (UMissionRunner* Old = SlotRunner.Get())
		{
			Old->OnMissionFinished.Remove(SlotMissionHandle);
		}
		SlotMissionHandle = Runner->OnMissionFinished.AddUObject(this, &UWeaponManagerComponent::HandleSlotMissionFinished);
		SlotRunner = Runner;
	}
}

void UWeaponManagerComponent::StopWatchingSlotProgress()
{
	if (UPlayerProgressionSubsystem* Progression = SlotProgression.Get())
	{
		Progression->OnXPChanged.Remove(SlotXPHandle);
	}
	if (UMissionRunner* Runner = SlotRunner.Get())
	{
		Runner->OnMissionFinished.Remove(SlotMissionHandle);
	}
	SlotXPHandle.Reset();
	SlotMissionHandle.Reset();
	SlotProgression.Reset();
	SlotRunner.Reset();
}

void UWeaponManagerComponent::HandleSlotXPChanged(int64 Gained, EXPSource Source)
{
	const UPlayerProgressionSubsystem* Progression = SlotProgression.Get();
	const TOptional<FGunSlotProgress> Now = GetSlotProgress();
	if (!Progression || !Now.IsSet())
	{
		return;
	}
	// Before: the level last seen with the story as it stands, so a level only ever opens the level's slot. A session's
	// progress coming in (a load) opens nothing with a flourish: the slots simply are what the save's level makes them.
	FGunSlotProgress Before = Now.GetValue();
	Before.Level = SlotLevelSeen;
	SlotLevelSeen = Progression->GetLevel();
	if (Source != EXPSource::Loaded)
	{
		AnnounceSlotsOpened(Before, Now.GetValue());
	}
}

void UWeaponManagerComponent::HandleSlotMissionFinished(const UMissionDefinition& Mission, bool bRewarded)
{
	const TOptional<FGunSlotProgress> Now = GetSlotProgress();
	// Only the first finish counts (one played again gives nothing); and only a main mission of the story area can open one.
	if (!bRewarded || !Now.IsSet() || Mission.Kind != EMissionKind::Main || Mission.Area != GunSlotRules::StoryArea())
	{
		return;
	}
	const UMissionRunner* Runner = SlotRunner.Get();
	if (!Runner)
	{
		return;
	}
	// Before: the story as it stood with this mission still to finish.
	FGunSlotProgress Before = Now.GetValue();
	Before.bStoryDone = GunSlotRules::IsStoryDone(Runner->GetDefinitions(), Runner->GetCampaign(), GunSlotRules::StoryArea(), Mission.GetMissionId());
	AnnounceSlotsOpened(Before, Now.GetValue());
}

void UWeaponManagerComponent::AnnounceSlotsOpened(const FGunSlotProgress& Before, const FGunSlotProgress& Now)
{
	const int32 From = FMath::Min(GunSlotRules::OpenSlots(Before), MaxWeapons);
	const int32 To = FMath::Min(GunSlotRules::OpenSlots(Now), MaxWeapons);
	for (int32 SlotIndex = From; SlotIndex < To; ++SlotIndex)
	{
		const EGunSlotLock Was = GunSlotRules::LockOf(SlotIndex, Before);
		UE_LOG(LogLooter, Log, TEXT("Gun slots: slot %d opened (%s); %d of %d open."), SlotIndex + 1,
			Was == EGunSlotLock::Level ? TEXT("the level") : TEXT("the story"), To, MaxWeapons);
		OnSlotUnlocked.Broadcast(SlotIndex, Was);
	}
}

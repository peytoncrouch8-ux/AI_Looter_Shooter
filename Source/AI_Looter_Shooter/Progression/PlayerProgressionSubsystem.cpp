#include "Progression/PlayerProgressionSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/CreatureBase.h"
#include "Progression/LooterProgressSave.h"
#include "Progression/ProgressionSettings.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const TCHAR* ProgressSaveSlot = TEXT("PlayerProgress");

	/** Seconds between earning experience and writing it, at most: losing that much to a crash is fine. */
	constexpr float SaveDelay = 5.f;
}

void UPlayerProgressionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SaveData = Cast<ULooterProgressSave>(UGameplayStatics::LoadGameFromSlot(ProgressSaveSlot, 0));
	if (!SaveData)
	{
		// A new game: level 1. Nothing is written until the player earns something.
		SaveData = NewObject<ULooterProgressSave>(this);
		SaveData->Version = ULooterProgressSave::CurrentVersion;
	}
	if (SaveData->Version < ULooterProgressSave::CurrentVersion)
	{
		// Nothing to upgrade yet; later versions convert older saves here.
		SaveData->Version = ULooterProgressSave::CurrentVersion;
		bUnsaved = true;
	}
	GetCurve().Clamp(SaveData->Level, SaveData->XP);
	UE_LOG(LogLooter, Log, TEXT("Player progress: level %d, %lld / %lld XP"), GetLevel(), GetXP(), GetXPToNextLevel());
}

void UPlayerProgressionSubsystem::Deinitialize()
{
	if (bUnsaved)
	{
		SaveProgress();
	}
	if (PendingSave.IsValid())
	{
		FTSTicker::RemoveTicker(PendingSave);
		PendingSave.Reset();
	}
	Super::Deinitialize();
}

FXPCurve UPlayerProgressionSubsystem::GetCurve()
{
	return GetDefault<UProgressionSettings>()->GetCurve();
}

int32 UPlayerProgressionSubsystem::GetLevel() const
{
	return SaveData ? SaveData->Level : 1;
}

int64 UPlayerProgressionSubsystem::GetXP() const
{
	return SaveData ? SaveData->XP : 0;
}

int64 UPlayerProgressionSubsystem::GetXPToNextLevel() const
{
	return GetCurve().XPToNextLevel(GetLevel());
}

float UPlayerProgressionSubsystem::GetLevelProgress() const
{
	return GetCurve().LevelProgress(GetLevel(), GetXP());
}

bool UPlayerProgressionSubsystem::IsMaxLevel() const
{
	return GetCurve().IsMaxLevel(GetLevel());
}

int32 UPlayerProgressionSubsystem::AddXP(int64 Amount, EXPSource Source)
{
	const FXPCurve Curve = GetCurve();
	if (!SaveData || Amount <= 0 || Curve.IsMaxLevel(SaveData->Level))
	{
		return 0;
	}

	const int32 OldLevel = SaveData->Level;
	const int32 LevelsGained = Curve.ApplyXP(SaveData->Level, SaveData->XP, Amount);
	UE_LOG(LogLooter, Verbose, TEXT("+%lld XP (%s): level %d, %lld / %lld"), Amount, *UEnum::GetValueAsString(Source),
		SaveData->Level, SaveData->XP, Curve.XPToNextLevel(SaveData->Level));

	for (int32 NewLevel = OldLevel + 1; NewLevel <= SaveData->Level; ++NewLevel)
	{
		UE_LOG(LogLooter, Log, TEXT("Level up: %d"), NewLevel);
		OnLevelUp.Broadcast(NewLevel);
	}
	OnXPChanged.Broadcast(Amount, Source);

	// A new level is saved at once (after the level-up events, so whatever they grant goes into the same write).
	if (LevelsGained > 0)
	{
		SaveProgress();
	}
	else
	{
		ScheduleSave();
	}
	return LevelsGained;
}

void UPlayerProgressionSubsystem::SetLevel(int32 Level)
{
	if (!SaveData)
	{
		return;
	}
	SaveData->Level = Level;
	SaveData->XP = 0;
	GetCurve().Clamp(SaveData->Level, SaveData->XP);
	UE_LOG(LogLooter, Log, TEXT("Player level set to %d"), SaveData->Level);
	OnXPChanged.Broadcast(0, EXPSource::Debug);
	SaveProgress();
}

void UPlayerProgressionSubsystem::ResetProgress()
{
	if (SaveData)
	{
		SaveData->bTutorialDone = false;
	}
	SetLevel(1);
}

bool UPlayerProgressionSubsystem::IsTutorialDone() const
{
	return SaveData && SaveData->bTutorialDone;
}

void UPlayerProgressionSubsystem::SetTutorialDone(bool bDone)
{
	if (SaveData && SaveData->bTutorialDone != bDone)
	{
		SaveData->bTutorialDone = bDone;
		SaveProgress();
	}
}

int64 UPlayerProgressionSubsystem::KillXP(const AActor* Victim)
{
	const ACreatureBase* Creature = Cast<ACreatureBase>(Victim);
	return Creature ? FMath::Max(Creature->XPReward, 0) : 0;
}

void UPlayerProgressionSubsystem::AwardKill(const AController* Killer, const AActor* Victim)
{
	// Only a local player has progress to add to (creatures killing each other, or a kill with no instigator, give none).
	const APlayerController* Player = Cast<APlayerController>(Killer);
	const ULocalPlayer* LocalPlayer = Player ? Player->GetLocalPlayer() : nullptr;
	UPlayerProgressionSubsystem* Progression = LocalPlayer ? LocalPlayer->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	const int64 XP = KillXP(Victim);
	if (Progression && XP > 0)
	{
		Progression->AddXP(XP, EXPSource::Kill);
	}
}

void UPlayerProgressionSubsystem::SaveProgress()
{
	if (PendingSave.IsValid())
	{
		FTSTicker::RemoveTicker(PendingSave);
		PendingSave.Reset();
	}
	if (SaveData)
	{
		UGameplayStatics::SaveGameToSlot(SaveData, ProgressSaveSlot, 0);
	}
	bUnsaved = false;
}

void UPlayerProgressionSubsystem::ScheduleSave()
{
	bUnsaved = true;
	if (!PendingSave.IsValid())
	{
		PendingSave = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UPlayerProgressionSubsystem::HandleSaveDue), SaveDelay);
	}
}

bool UPlayerProgressionSubsystem::HandleSaveDue(float DeltaTime)
{
	// The ticker drops this one-shot when it returns false; forget the handle first so SaveProgress doesn't remove it.
	PendingSave.Reset();
	SaveProgress();
	return false;
}

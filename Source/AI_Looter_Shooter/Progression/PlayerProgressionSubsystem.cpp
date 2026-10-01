#include "Progression/PlayerProgressionSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/CreatureBase.h"
#include "Progression/ProgressionSettings.h"
#include "Session/SessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

void UPlayerProgressionSubsystem::SetProgress(const FPlayerProgressData& InProgress)
{
	Progress = InProgress;
	GetCurve().Clamp(Progress.Level, Progress.XP);
	UE_LOG(LogLooter, Log, TEXT("Player progress: level %d, %lld / %lld XP"), GetLevel(), GetXP(), GetXPToNextLevel());
	OnXPChanged.Broadcast(0, EXPSource::Loaded);
}

FXPCurve UPlayerProgressionSubsystem::GetCurve()
{
	return GetDefault<UProgressionSettings>()->GetCurve();
}

int32 UPlayerProgressionSubsystem::GetLevel() const
{
	return Progress.Level;
}

int64 UPlayerProgressionSubsystem::GetXP() const
{
	return Progress.XP;
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
	if (Amount <= 0 || Curve.IsMaxLevel(Progress.Level))
	{
		return 0;
	}

	const int32 OldLevel = Progress.Level;
	const int32 LevelsGained = Curve.ApplyXP(Progress.Level, Progress.XP, Amount);
	UE_LOG(LogLooter, Verbose, TEXT("+%lld XP (%s): level %d, %lld / %lld"), Amount, *UEnum::GetValueAsString(Source),
		Progress.Level, Progress.XP, Curve.XPToNextLevel(Progress.Level));

	for (int32 NewLevel = OldLevel + 1; NewLevel <= Progress.Level; ++NewLevel)
	{
		UE_LOG(LogLooter, Log, TEXT("Level up: %d"), NewLevel);
		OnLevelUp.Broadcast(NewLevel);
	}
	OnXPChanged.Broadcast(Amount, Source);
	RequestSave();
	return LevelsGained;
}

void UPlayerProgressionSubsystem::SetLevel(int32 Level)
{
	Progress.Level = Level;
	Progress.XP = 0;
	GetCurve().Clamp(Progress.Level, Progress.XP);
	UE_LOG(LogLooter, Log, TEXT("Player level set to %d"), Progress.Level);
	OnXPChanged.Broadcast(0, EXPSource::Debug);
	RequestSave();
}

void UPlayerProgressionSubsystem::ResetProgress()
{
	Progress.bTutorialDone = false;
	Progress.Defeated.Reset();
	Progress.Encountered.Reset();
	SetLevel(1);
}

bool UPlayerProgressionSubsystem::IsTutorialDone() const
{
	return Progress.bTutorialDone;
}

void UPlayerProgressionSubsystem::SetTutorialDone(bool bDone)
{
	if (Progress.bTutorialDone != bDone)
	{
		Progress.bTutorialDone = bDone;
		RequestSave();
	}
}

int64 UPlayerProgressionSubsystem::KillXP(const AActor* Victim)
{
	const ACreatureBase* Creature = Cast<ACreatureBase>(Victim);
	return Creature ? FMath::Max(Creature->XPReward, 0) : 0;
}

void UPlayerProgressionSubsystem::AwardKill(const AController* Killer, const AActor* Victim)
{
	// Only a local player has progress to add to (creatures killing each other, or a kill with no instigator, count for
	// nothing). Anything can be defeated (a target dummy too), but only creatures give experience.
	const APlayerController* Player = Cast<APlayerController>(Killer);
	const ULocalPlayer* LocalPlayer = Player ? Player->GetLocalPlayer() : nullptr;
	UPlayerProgressionSubsystem* Progression = LocalPlayer ? LocalPlayer->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	if (!Progression)
	{
		return;
	}
	Progression->RecordDefeat(Victim);
	const int64 XP = KillXP(Victim);
	if (XP > 0)
	{
		Progression->AddXP(XP, EXPSource::Kill);
	}
}

void UPlayerProgressionSubsystem::RecordDefeat(const AActor* Victim)
{
	if (!Victim)
	{
		return;
	}
	const FString Kind = Victim->GetClass()->GetPathName();
	++Progress.Defeated.FindOrAdd(Kind);
	Progress.Encountered.Add(Kind);
	RequestSave();
}

void UPlayerProgressionSubsystem::RecordEncounter(const AController* Player, const AActor* Actor)
{
	const APlayerController* PlayerController = Cast<APlayerController>(Player);
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	UPlayerProgressionSubsystem* Progression = LocalPlayer ? LocalPlayer->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	if (!Progression || !Actor)
	{
		return;
	}
	// Most calls are for kinds already met (every hit lands here): only a first meeting asks for a save.
	bool bAlreadyMet = false;
	Progression->Progress.Encountered.Add(Actor->GetClass()->GetPathName(), &bAlreadyMet);
	if (!bAlreadyMet)
	{
		UE_LOG(LogLooter, Log, TEXT("Bestiary: met %s for the first time"), *Actor->GetClass()->GetName());
		Progression->RequestSave();
	}
}

void UPlayerProgressionSubsystem::ForgetBestiary()
{
	Progress.Encountered.Reset();
	Progress.Defeated.Reset();
	RequestSave();
}

bool UPlayerProgressionSubsystem::HasEncountered(const UClass* ActorType) const
{
	if (!ActorType)
	{
		return false;
	}
	// Met per exact class, so meeting a Blueprint child of a creature opens its parent's page too.
	for (const FString& Kind : Progress.Encountered)
	{
		const UClass* Met = FSoftClassPath(Kind).TryLoadClass<AActor>();
		if (Met && Met->IsChildOf(ActorType))
		{
			return true;
		}
	}
	return false;
}

int32 UPlayerProgressionSubsystem::GetDefeated(const UClass* ActorType) const
{
	if (!ActorType)
	{
		return 0;
	}
	// Kills are kept per exact class, so a Blueprint child of a creature counts toward its parent's entry too.
	int32 Count = 0;
	for (const TPair<FString, int32>& Pair : Progress.Defeated)
	{
		const UClass* Killed = FSoftClassPath(Pair.Key).TryLoadClass<AActor>();
		Count += Killed && Killed->IsChildOf(ActorType) ? Pair.Value : 0;
	}
	return Count;
}

void UPlayerProgressionSubsystem::RequestSave() const
{
	// Nothing happens without a session (the main menu, or a level played straight from the editor).
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	const UGameInstance* GameInstance = LocalPlayer ? LocalPlayer->GetGameInstance() : nullptr;
	if (USessionSubsystem* Sessions = GameInstance ? GameInstance->GetSubsystem<USessionSubsystem>() : nullptr)
	{
		Sessions->SaveSoon();
	}
}

#include "Progression/PlayerProgressionSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Progression/ProgressionSettings.h"
#include "Session/SessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/**
	 * Whether something of class Met (as kept, by exact class) counts as one of a bestiary page's kind: that class itself, or
	 * a Blueprint made from it. A C++ class made from it is a kind of its own (the Gravemother, a spider).
	 */
	bool CountsForPage(const UClass* Met, const UClass* PageKind)
	{
		return Met && PageKind && Met->IsChildOf(PageKind) && (Met == PageKind || !Met->HasAnyClassFlags(CLASS_Native));
	}
}

void UPlayerProgressionSubsystem::PlayerControllerChanged(APlayerController* NewPlayerController)
{
	Super::PlayerControllerChanged(NewPlayerController);
	if (IsValid(NewPlayerController))
	{
		// As a level starts the controller possesses the character before anything begins play, so the character starts
		// full at its level's health and the session's saved health fits inside it.
		NewPlayerController->OnPossessedPawnChanged.AddUniqueDynamic(this, &UPlayerProgressionSubsystem::HandlePossessedPawnChanged);
		ApplyLevelRewards(NewPlayerController->GetPawn());
	}
}

void UPlayerProgressionSubsystem::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	ApplyLevelRewards(NewPawn);
}

void UPlayerProgressionSubsystem::SetProgress(const FPlayerProgressData& InProgress)
{
	Progress = InProgress;
	GetCurve().Clamp(Progress.Level, Progress.XP);
	UE_LOG(LogLooter, Log, TEXT("Player progress: level %d, %lld / %lld XP"), GetLevel(), GetXP(), GetXPToNextLevel());
	// Usually there's no character yet (a level is starting); it gets its health as it's possessed.
	ApplyLevelRewards(GetPlayerPawn());
	OnXPChanged.Broadcast(0, EXPSource::Loaded);
}

FXPCurve UPlayerProgressionSubsystem::GetCurve()
{
	return GetDefault<UProgressionSettings>()->GetCurve();
}

FLevelRules UPlayerProgressionSubsystem::GetLevelRules()
{
	return GetDefault<UProgressionSettings>()->GetLevelRules();
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

	if (LevelsGained > 0)
	{
		// The new level's rewards first, so whoever hears of the level-up finds them in place.
		ApplyLevelRewards(GetPlayerPawn());
	}
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
	ApplyLevelRewards(GetPlayerPawn());
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

int64 UPlayerProgressionSubsystem::KillXP(const AActor* Victim, int32 PlayerLevel)
{
	// The area the victim died in decides first: Skyreach is practice, outside the story and no road to levels.
	if (!UAreaRulesSubsystem::GivesKillExperienceAt(Victim))
	{
		return 0;
	}
	return KillXPIn(nullptr, Victim, PlayerLevel);
}

int64 UPlayerProgressionSubsystem::KillXPIn(const UAreaDefinition* Area, const AActor* Victim, int32 PlayerLevel)
{
	// Its XPReward has its rank's multiplier in it already: the kill adds only its level's growth, and the falloff when
	// it's below the player.
	const ACreatureBase* Creature = Cast<ACreatureBase>(Victim);
	if (!Creature || !UAreaRulesSubsystem::GivesKillExperienceIn(Area))
	{
		return 0;
	}
	return GetLevelRules().KillXP(Creature->XPReward, Creature->Level, PlayerLevel);
}

void UPlayerProgressionSubsystem::ApplyLevelHealth(UHealthComponent& Health, int32 Level)
{
	// The character's own max health is its archetype's (its Blueprint's template), which nothing changes in play.
	const UHealthComponent* Authored = Cast<UHealthComponent>(Health.GetArchetype());
	const float AuthoredMaxHealth = Authored ? Authored->MaxHealth : Health.MaxHealth;
	Health.SetMaxHealth(AuthoredMaxHealth * GetLevelRules().PlayerHealthScale(Level));
}

APawn* UPlayerProgressionSubsystem::GetPlayerPawn() const
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	const APlayerController* Controller = LocalPlayer ? LocalPlayer->GetPlayerController(nullptr) : nullptr;
	return IsValid(Controller) ? Controller->GetPawn() : nullptr;
}

void UPlayerProgressionSubsystem::ApplyLevelRewards(APawn* Pawn) const
{
	if (UHealthComponent* Health = IsValid(Pawn) ? Pawn->FindComponentByClass<UHealthComponent>() : nullptr)
	{
		ApplyLevelHealth(*Health, GetLevel());
	}
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
	const int64 XP = KillXP(Victim, Progression->GetLevel());
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
	// Met per exact class, so meeting a Blueprint child of a creature opens its parent's page too. A C++ child is a kind of
	// its own with a page of its own (the Gravemother, a spider): meeting her doesn't open the brown spider's.
	for (const FString& Kind : Progress.Encountered)
	{
		const UClass* Met = FSoftClassPath(Kind).TryLoadClass<AActor>();
		if (CountsForPage(Met, ActorType))
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
	// Kills are kept per exact class, so a Blueprint child of a creature counts toward its parent's entry too (a C++ child,
	// the Gravemother, toward her own).
	int32 Count = 0;
	for (const TPair<FString, int32>& Pair : Progress.Defeated)
	{
		const UClass* Killed = FSoftClassPath(Pair.Key).TryLoadClass<AActor>();
		Count += CountsForPage(Killed, ActorType) ? Pair.Value : 0;
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

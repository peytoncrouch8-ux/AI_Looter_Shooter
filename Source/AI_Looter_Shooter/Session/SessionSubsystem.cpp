#include "Session/SessionSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Progression/LooterProgressSave.h"
#include "Progression/PlayerProgressData.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Session/SessionSave.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameMapsSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"

namespace
{
	/** The main menu's game mode, by its alias in DefaultEngine.ini (GameModeClassAliases), when LocalMapOptions has none. */
	const TCHAR* MenuOptions = TEXT("game=Menu");

	/** Where GameUserSettings.ini remembers that the old progress save was carried into the sessions. */
	const TCHAR* ConfigSection = TEXT("/Script/AI_Looter_Shooter.SessionSubsystem");
	const TCHAR* ImportedKey = TEXT("bImportedLegacyProgress");

	/**
	 * The progress saved before sessions: where this run keeps its saves, or else the editor's. A standalone game run by
	 * the installed engine keeps its saves under AppData, apart from Play-In-Editor's in the project's Saved folder.
	 */
	ULooterProgressSave* LoadLegacyProgress()
	{
		if (UGameplayStatics::DoesSaveGameExist(ULooterProgressSave::SlotName, 0))
		{
			return Cast<ULooterProgressSave>(UGameplayStatics::LoadGameFromSlot(ULooterProgressSave::SlotName, 0));
		}
		const FString EditorSave = FPaths::Combine(FPaths::ProjectDir(), TEXT("Saved"), TEXT("SaveGames"), FString(ULooterProgressSave::SlotName) + TEXT(".sav"));
		TArray<uint8> Bytes;
		if (FFileHelper::LoadFileToArray(Bytes, *EditorSave, FILEREAD_Silent))
		{
			return Cast<ULooterProgressSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
		}
		return nullptr;
	}
}

USessionSubsystem* USessionSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<USessionSubsystem>() : nullptr;
}

void USessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Saved as each level starts to tear down, while its actors are all still there: quitting the game, travelling to
	// another level, or stopping Play-In-Editor.
	TearDownHandle = FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &USessionSubsystem::HandleWorldBeginTearDown);
	ImportLegacyProgress();
}

void USessionSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldBeginTearDown.Remove(TearDownHandle);
	StopTimers();
	Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

FString USessionSubsystem::SlotName(int32 Index)
{
	return FString::Printf(TEXT("Session%d"), Index + 1);
}

ULooterSessionSave* USessionSubsystem::LoadSlot(int32 Index) const
{
	if (Index < 0 || Index >= MaxSessions || !UGameplayStatics::DoesSaveGameExist(SlotName(Index), 0))
	{
		return nullptr;
	}
	ULooterSessionSave* Save = Cast<ULooterSessionSave>(UGameplayStatics::LoadGameFromSlot(SlotName(Index), 0));
	if (!Save)
	{
		UE_LOG(LogLooter, Warning, TEXT("Session %d's save couldn't be read."), Index + 1);
	}
	return Save;
}

FSessionSummary USessionSubsystem::GetSummary(int32 Index) const
{
	FSessionSummary Summary;
	Summary.Index = Index;
	// The session being played is newer in memory than on disk (the menu never has one, but tests and commands might).
	const ULooterSessionSave* Save = Index == ActiveIndex && Current ? Current.Get() : LoadSlot(Index);
	if (!Save)
	{
		return Summary;
	}
	Summary.bExists = true;
	Summary.Level = Save->Progress.Level;
	Summary.Weapons = Save->Inventory.Equipped.Num() + Save->Inventory.Backpack.Num();
	Summary.PlayedSeconds = Save->PlayedSeconds;
	Summary.Saved = Save->Saved;
	Summary.Place = PlaceName(Save->Map.IsEmpty() ? NewGameMap() : Save->Map);
	Summary.bTutorialDone = Save->Progress.bTutorialDone;
	return Summary;
}

bool USessionSubsystem::PlaySession(int32 Index)
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (Index < 0 || Index >= MaxSessions || !World)
	{
		return false;
	}
	// It continues in the level it was saved in, when that still exists.
	const ULooterSessionSave* Save = LoadSlot(Index);
	FString Map = Save ? Save->Map : FString();
	if (Map.IsEmpty() || !FPackageName::DoesPackageExist(Map))
	{
		Map = NewGameMap();
	}
	UE_LOG(LogLooter, Log, TEXT("Session %d: %s in %s"), Index + 1, Save ? TEXT("continuing") : TEXT("new game"), *Map);
	UGameplayStatics::OpenLevel(World, FName(*Map), /*bAbsolute*/ true, FString::Printf(TEXT("Session=%d"), Index + 1));
	return true;
}

bool USessionSubsystem::DeleteSession(int32 Index)
{
	if (Index < 0 || Index >= MaxSessions || Index == ActiveIndex || !UGameplayStatics::DoesSaveGameExist(SlotName(Index), 0))
	{
		return false;
	}
	const bool bDeleted = UGameplayStatics::DeleteGameInSlot(SlotName(Index), 0);
	UE_LOG(LogLooter, Log, TEXT("Session %d %s"), Index + 1, bDeleted ? TEXT("deleted") : TEXT("could not be deleted"));
	return bDeleted;
}

void USessionSubsystem::OpenMainMenu()
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (!World)
	{
		return;
	}
	// The same as the game's start: the default map with LocalMapOptions (the menu's game mode).
	FString Options = GetDefault<UGameMapsSettings>()->LocalMapOptions;
	Options.RemoveFromStart(TEXT("?"));
	if (Options.IsEmpty())
	{
		Options = MenuOptions;
	}
	UGameplayStatics::OpenLevel(World, FName(*NewGameMap()), /*bAbsolute*/ true, Options);
}

FString USessionSubsystem::NewGameMap()
{
	// "/Game/Maps/Lvl_TutorialIsland.Lvl_TutorialIsland" is the object; OpenLevel and the saves use the package.
	const FString DefaultMap = UGameMapsSettings::GetGameDefaultMap();
	return FPackageName::ObjectPathToPackageName(DefaultMap);
}

// ---------------------------------------------------------------------------
// The session being played
// ---------------------------------------------------------------------------

void USessionSubsystem::BeginPlayWorld(UWorld* World, const FString& Options)
{
	StopTimers();
	PlayWorld = World;
	bWorldRestored = false;
	PlayClock = World ? World->GetTimeSeconds() : 0.0;

	const FString Option = UGameplayStatics::ParseOption(Options, TEXT("Session"));
	const int32 Number = Option.IsEmpty() ? 0 : FCString::Atoi(*Option);
	ActiveIndex = Number >= 1 && Number <= MaxSessions ? Number - 1 : INDEX_NONE;
	Current = nullptr;
	if (ActiveIndex != INDEX_NONE)
	{
		Current = LoadSlot(ActiveIndex);
		if (!Current)
		{
			// A new game in this slot. Nothing is written until it is first saved.
			Current = NewObject<ULooterSessionSave>(this);
			Current->Version = ULooterSessionSave::CurrentVersion;
			Current->Created = FDateTime::Now();
		}
		AutosaveTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &USessionSubsystem::HandleAutosave), AutosaveInterval);
		UE_LOG(LogLooter, Log, TEXT("Playing session %d (%s)"), ActiveIndex + 1, Current->bHasWorld ? TEXT("saved") : TEXT("new game"));
	}
	else
	{
		UE_LOG(LogLooter, Log, TEXT("Playing without a session: nothing is saved."));
	}
	SetPlayerProgress(Current ? Current->Progress : FPlayerProgressData());
}

void USessionSubsystem::RestorePlayWorld(UWorld* World)
{
	if (World != PlayWorld.Get())
	{
		return;
	}
	if (Current)
	{
		RestoreWorld(World, *Current);
	}
	bWorldRestored = true;
}

void USessionSubsystem::BeginMenuWorld(UWorld* World)
{
	StopTimers();
	ActiveIndex = INDEX_NONE;
	Current = nullptr;
	PlayWorld = nullptr;
	bWorldRestored = false;
	// The menu is no one's game: whatever was played before is put away.
	SetPlayerProgress(FPlayerProgressData());
}

bool USessionSubsystem::SaveNow()
{
	UWorld* World = PlayWorld.Get();
	if (!IsPlayingSession() || !Current || !World || !bWorldRestored)
	{
		return false;
	}
	if (PendingSave.IsValid())
	{
		FTSTicker::RemoveTicker(PendingSave);
		PendingSave.Reset();
	}

	const double Now = World->GetTimeSeconds();
	Current->PlayedSeconds += FMath::Max(Now - PlayClock, 0.0);
	PlayClock = Now;
	Current->Version = ULooterSessionSave::CurrentVersion;
	Current->Saved = FDateTime::Now();
	Current->Map = UWorld::RemovePIEPrefix(World->GetOutermost()->GetName());
	GetPlayerProgress(Current->Progress);
	CaptureWorld(World, *Current);

	const bool bSaved = UGameplayStatics::SaveGameToSlot(Current, SlotName(ActiveIndex), 0);
	UE_LOG(LogLooter, Log, TEXT("Session %d %s: level %d, %d guns, %d loot on the ground, %.0f s played"), ActiveIndex + 1,
		bSaved ? TEXT("saved") : TEXT("FAILED to save"), Current->Progress.Level,
		Current->Inventory.Equipped.Num() + Current->Inventory.Backpack.Num(), Current->LootWeapons.Num() + Current->AmmoPickups.Num(),
		Current->PlayedSeconds);
	return bSaved;
}

void USessionSubsystem::SaveSoon()
{
	if (IsPlayingSession() && !PendingSave.IsValid())
	{
		PendingSave = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &USessionSubsystem::HandleSaveDue), SaveSoonDelay);
	}
}

void USessionSubsystem::SaveAndQuitToMenu()
{
	SaveNow();
	// Played no more: the level's teardown on the way to the menu has nothing left to save.
	StopTimers();
	ActiveIndex = INDEX_NONE;
	Current = nullptr;
	OpenMainMenu();
}

void USessionSubsystem::HandleWorldBeginTearDown(UWorld* World)
{
	if (World && World == PlayWorld.Get())
	{
		SaveNow();
		StopTimers();
		PlayWorld = nullptr;
		bWorldRestored = false;
	}
}

bool USessionSubsystem::HandleAutosave(float DeltaTime)
{
	SaveNow();
	return true;
}

bool USessionSubsystem::HandleSaveDue(float DeltaTime)
{
	// The ticker drops this one-shot when it returns false; forget the handle first so SaveNow doesn't remove it.
	PendingSave.Reset();
	SaveNow();
	return false;
}

void USessionSubsystem::StopTimers()
{
	if (AutosaveTicker.IsValid())
	{
		FTSTicker::RemoveTicker(AutosaveTicker);
		AutosaveTicker.Reset();
	}
	if (PendingSave.IsValid())
	{
		FTSTicker::RemoveTicker(PendingSave);
		PendingSave.Reset();
	}
}

// ---------------------------------------------------------------------------
// Progress
// ---------------------------------------------------------------------------

void USessionSubsystem::SetPlayerProgress(const FPlayerProgressData& Progress) const
{
	const ULocalPlayer* Player = GetGameInstance()->GetFirstGamePlayer();
	if (UPlayerProgressionSubsystem* Progression = Player ? Player->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr)
	{
		Progression->SetProgress(Progress);
	}
}

void USessionSubsystem::GetPlayerProgress(FPlayerProgressData& OutProgress) const
{
	const ULocalPlayer* Player = GetGameInstance()->GetFirstGamePlayer();
	if (const UPlayerProgressionSubsystem* Progression = Player ? Player->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr)
	{
		OutProgress = Progression->GetProgress();
	}
}

void USessionSubsystem::ImportLegacyProgress()
{
	// Before sessions, the player's progress was one save. It becomes session 1 the first time sessions start, unless
	// something is there already. Once only (GameUserSettings.ini remembers), so deleting session 1 later doesn't bring
	// it back; the old file itself stays as it was.
	bool bImported = false;
	GConfig->GetBool(ConfigSection, ImportedKey, bImported, GGameUserSettingsIni);
	if (bImported)
	{
		return;
	}
	if (!UGameplayStatics::DoesSaveGameExist(SlotName(0), 0))
	{
		if (const ULooterProgressSave* Legacy = LoadLegacyProgress())
		{
			ULooterSessionSave* Save = NewObject<ULooterSessionSave>(this);
			Save->Version = ULooterSessionSave::CurrentVersion;
			Save->Created = FDateTime::Now();
			Save->Saved = Save->Created;
			Save->Map = NewGameMap();
			Save->Progress = Legacy->ToProgress();
			if (!UGameplayStatics::SaveGameToSlot(Save, SlotName(0), 0))
			{
				UE_LOG(LogLooter, Warning, TEXT("Couldn't carry the old progress save into session 1; trying again next start."));
				return;
			}
			UE_LOG(LogLooter, Log, TEXT("The old progress save (level %d) is now session 1."), Save->Progress.Level);
		}
	}
	GConfig->SetBool(ConfigSection, ImportedKey, true, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

// ---------------------------------------------------------------------------
// Words for people
// ---------------------------------------------------------------------------

FString USessionSubsystem::FormatPlayTime(double Seconds)
{
	const int64 Total = FMath::Max<int64>(FMath::FloorToInt64(Seconds), 0);
	if (Total < 60)
	{
		return FString::Printf(TEXT("%lld s"), Total);
	}
	const int64 Minutes = Total / 60;
	if (Minutes < 60)
	{
		return FString::Printf(TEXT("%lld min"), Minutes);
	}
	return FString::Printf(TEXT("%lld h %02lld min"), Minutes / 60, Minutes % 60);
}

FString USessionSubsystem::FormatSavedTime(const FDateTime& Saved, const FDateTime& Now)
{
	const FString Clock = FString::Printf(TEXT("%02d:%02d"), Saved.GetHour(), Saved.GetMinute());
	if (Saved.GetDate() == Now.GetDate())
	{
		return TEXT("Today ") + Clock;
	}
	if (Saved.GetDate() == (Now - FTimespan::FromDays(1.0)).GetDate())
	{
		return TEXT("Yesterday ") + Clock;
	}
	static const TCHAR* Months[] = { TEXT("Jan"), TEXT("Feb"), TEXT("Mar"), TEXT("Apr"), TEXT("May"), TEXT("Jun"), TEXT("Jul"),
		TEXT("Aug"), TEXT("Sep"), TEXT("Oct"), TEXT("Nov"), TEXT("Dec") };
	return FString::Printf(TEXT("%s %d, %d"), Months[FMath::Clamp(Saved.GetMonth(), 1, 12) - 1], Saved.GetDay(), Saved.GetYear());
}

FString USessionSubsystem::PlaceName(const FString& Map)
{
	// "/Game/Maps/Lvl_TutorialIsland" -> "TutorialIsland" -> "Tutorial Island".
	FString Name = FPackageName::GetShortName(Map);
	Name.RemoveFromStart(TEXT("Lvl_"));
	return FName::NameToDisplayString(Name, false);
}

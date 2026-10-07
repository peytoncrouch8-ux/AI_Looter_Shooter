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
	// Saved as each level starts to tear down, while its actors are all still there: quitting the game, or stopping
	// Play-In-Editor. A trip has saved already, and the gate keeps the level it leaves from saving over it.
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
	TArray<uint8> Bytes;
	ULooterSessionSave* Save = UGameplayStatics::LoadDataFromSlot(Bytes, SlotName(Index), 0) ? ReadSave(Bytes) : nullptr;
	if (!Save)
	{
		UE_LOG(LogLooter, Warning, TEXT("Session %d's save couldn't be read."), Index + 1);
	}
	return Save;
}

ULooterSessionSave* USessionSubsystem::ReadSave(const TArray<uint8>& Bytes)
{
	ULooterSessionSave* Save = Cast<ULooterSessionSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!Save)
	{
		return nullptr;
	}
	// Every read goes through here, so an older save is brought up to date before anything can save it again.
	const int32 ReadVersion = Save->Upgrade();
	if (ReadVersion < ULooterSessionSave::CurrentVersion)
	{
		UE_LOG(LogLooter, Log, TEXT("Session save read as version %d and brought up to version %d: %d guns, %d maps' worlds"), ReadVersion,
			ULooterSessionSave::CurrentVersion, Save->CountGuns(), Save->Worlds.Num());
	}
	else if (ReadVersion > ULooterSessionSave::CurrentVersion)
	{
		UE_LOG(LogLooter, Warning, TEXT("Session save is version %d, newer than this game's %d: read as it is."), ReadVersion,
			ULooterSessionSave::CurrentVersion);
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
	Summary.Weapons = Save->CountGuns();
	Summary.PlayedSeconds = Save->PlayedSeconds;
	Summary.Saved = Save->Saved;
	// Where Continue goes, by the area's name.
	Summary.Place = AreaName(ContinueMap(Save->Map));
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
	const FString Map = ContinueMap(Save ? Save->Map : FString());
	if (Save && !Save->Map.IsEmpty() && !Save->Map.Equals(Map, ESearchCase::IgnoreCase))
	{
		UE_LOG(LogLooter, Warning, TEXT("Session %d was saved in %s, which isn't in the game: it continues in %s, at the level's start."),
			Index + 1, *Save->Map, *Map);
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

FString USessionSubsystem::ContinueMap(const FString& SavedMap)
{
	if (!SavedMap.IsEmpty() && FPackageName::IsValidLongPackageName(SavedMap) && FPackageName::DoesPackageExist(SavedMap))
	{
		return SavedMap;
	}
	return NewGameMap();
}

FString USessionSubsystem::MapOf(const UWorld* World)
{
	// Play-In-Editor plays a copy named "/Game/Maps/UEDPIE_0_Lvl_X": sessions name the level itself.
	return World ? UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()) : FString();
}

// ---------------------------------------------------------------------------
// The session being played
// ---------------------------------------------------------------------------

ULooterSessionSave* USessionSubsystem::NewSave()
{
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>(this);
	Save->Version = ULooterSessionSave::CurrentVersion;
	Save->Created = FDateTime::Now();
	return Save;
}

void USessionSubsystem::BeginPlayWorld(UWorld* World, const FString& Options)
{
	StopTimers();
	// A trip ends as its destination begins, and whatever held saves in the level it left is over with it.
	const bool bFromTrip = SaveGate.IsTravelling();
	SaveGate.Reset();
	bSaveWanted = false;
	PlayWorld = World;
	bWorldRestored = false;
	PlayClock = World ? World->GetTimeSeconds() : 0.0;

	const FString Option = UGameplayStatics::ParseOption(Options, TEXT("Session"));
	const int32 Number = Option.IsEmpty() ? 0 : FCString::Atoi(*Option);
	ActiveIndex = Number >= 1 && Number <= MaxSessions ? Number - 1 : INDEX_NONE;
	if (ActiveIndex != INDEX_NONE)
	{
		// After a trip this reads the trip's own save, which points here.
		Current = LoadSlot(ActiveIndex);
		const bool bLoaded = Current != nullptr;
		if (!bLoaded)
		{
			// A new game in this slot. Nothing is written until it is first saved.
			Current = NewSave();
		}
		AutosaveTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &USessionSubsystem::HandleAutosave), AutosaveInterval);
		UE_LOG(LogLooter, Log, TEXT("Playing session %d (%s)"), ActiveIndex + 1, bLoaded ? TEXT("saved") : TEXT("new game"));
	}
	else if (bFromTrip && Current)
	{
		UE_LOG(LogLooter, Log, TEXT("Playing without a session: nothing is saved, but the player and the worlds came along on the trip."));
	}
	else
	{
		Current = nullptr;
		UE_LOG(LogLooter, Log, TEXT("Playing without a session: nothing is saved."));
	}

	// After a trip the player arrives at its landing, unless the session has a spot for them on this level; by train only
	// from a train's trip.
	ArrivalLanding = Current ? Current->GetArrivalOn(MapOf(World)) : NAME_None;
	bArrivalByTrain = bArrivalByTrain && bFromTrip && !ArrivalLanding.IsNone();
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
		// Whatever trip brought the player here is over: from now on the session keeps where they stand.
		Current->ArrivalTag = NAME_None;
	}
	if (!ArrivalLanding.IsNone())
	{
		PlaceAtLanding(World, ArrivalLanding);
		ArrivalLanding = NAME_None;
	}
	bArrivalByTrain = false;
	bWorldRestored = true;
}

void USessionSubsystem::BeginMenuWorld(UWorld* World)
{
	StopTimers();
	SaveGate.Reset();
	bSaveWanted = false;
	ArrivalLanding = NAME_None;
	bArrivalByTrain = false;
	ActiveIndex = INDEX_NONE;
	Current = nullptr;
	PlayWorld = nullptr;
	bWorldRestored = false;
	// The menu is no one's game: whatever was played before is put away.
	SetPlayerProgress(FPlayerProgressData());
}

bool USessionSubsystem::SaveNow()
{
	return SaveFor(ESessionSaveReason::Asked);
}

bool USessionSubsystem::SaveFor(ESessionSaveReason Reason)
{
	UWorld* World = PlayWorld.Get();
	if (!IsPlayingSession() || !Current || !World || !bWorldRestored)
	{
		return false;
	}
	if (!SaveGate.Allows(Reason))
	{
		// Held for a ride or a fade: it saves once the holds end. During a trip nothing waits, as the trip's own save has
		// everything and the level it leaves is going.
		bSaveWanted = bSaveWanted || !SaveGate.IsTravelling();
		return false;
	}
	if (PendingSave.IsValid())
	{
		FTSTicker::RemoveTicker(PendingSave);
		PendingSave.Reset();
	}
	bSaveWanted = false;
	CaptureSession(*World);
	return WriteSession();
}

void USessionSubsystem::CaptureSession(UWorld& World)
{
	const double Now = World.GetTimeSeconds();
	Current->PlayedSeconds += FMath::Max(Now - PlayClock, 0.0);
	PlayClock = Now;
	Current->Version = ULooterSessionSave::CurrentVersion;
	GetPlayerProgress(Current->Progress);
	CaptureWorld(&World, *Current);
}

bool USessionSubsystem::WriteSession()
{
	Current->Saved = FDateTime::Now();
	const bool bSaved = UGameplayStatics::SaveGameToSlot(Current, SlotName(ActiveIndex), 0);
	const FSavedMapWorld* Here = Current->FindWorld(Current->Map);
	UE_LOG(LogLooter, Log, TEXT("Session %d %s: level %d, %d guns, in %s with %d loot on the ground, %.0f s played"), ActiveIndex + 1,
		bSaved ? TEXT("saved") : TEXT("FAILED to save"), Current->Progress.Level, Current->CountGuns(), *FPackageName::GetShortName(Current->Map),
		Here ? Here->LootWeapons.Num() + Here->AmmoPickups.Num() : 0, Current->PlayedSeconds);
	return bSaved;
}

void USessionSubsystem::SaveSoon()
{
	// During a trip nothing new is wanted: its own save has everything.
	if (!IsPlayingSession() || SaveGate.IsTravelling())
	{
		return;
	}
	if (!SaveGate.Allows(ESessionSaveReason::Soon))
	{
		bSaveWanted = true;
		return;
	}
	if (!PendingSave.IsValid())
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

void USessionSubsystem::HoldSaves(FName Reason)
{
	SaveGate.Hold(Reason);
}

void USessionSubsystem::ReleaseSaves(FName Reason)
{
	if (SaveGate.Release(Reason) && bSaveWanted)
	{
		// What waited saves now, a few seconds on like any save-soon.
		bSaveWanted = false;
		SaveSoon();
	}
}

FCampaignRecord* USessionSubsystem::GetCampaign()
{
	// Played without a session, the story is kept in memory for this play (and its trips) only, like everything else.
	if (!Current && PlayWorld.IsValid())
	{
		Current = NewSave();
	}
	return Current ? &Current->Campaign : nullptr;
}

void USessionSubsystem::HandleWorldBeginTearDown(UWorld* World)
{
	if (World && World == PlayWorld.Get())
	{
		// Refused while a trip is under way: the level being left would file itself as the one the session continues in.
		SaveFor(ESessionSaveReason::LevelEnd);
		StopTimers();
		PlayWorld = nullptr;
		bWorldRestored = false;
	}
}

bool USessionSubsystem::HandleAutosave(float DeltaTime)
{
	SaveFor(ESessionSaveReason::Autosave);
	return true;
}

bool USessionSubsystem::HandleSaveDue(float DeltaTime)
{
	// The ticker drops this one-shot when it returns false; forget the handle first so SaveFor doesn't remove it.
	PendingSave.Reset();
	SaveFor(ESessionSaveReason::Soon);
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
			ULooterSessionSave* Save = NewSave();
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

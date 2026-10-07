// Developer console commands for sessions, travel and areas (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Session/SessionSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaDefinition.h"
#include "Session/SessionSave.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponDefinition.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

namespace
{
	/** The sessions of the world the command was typed in, or of the running play session (from the editor). */
	USessionSubsystem* FindSessions(UWorld* World)
	{
		UWorld* GameWorld = World && World->IsGameWorld() ? World : nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (!GameWorld && Context.World() && Context.World()->IsGameWorld())
			{
				GameWorld = Context.World();
			}
		}
		const UGameInstance* GameInstance = GameWorld ? GameWorld->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<USessionSubsystem>() : nullptr;
	}

	/** "2" -> slot 1; INDEX_NONE when it isn't a session's number. */
	int32 ParseSessionNumber(const FString& Word)
	{
		const int32 Number = Word.IsNumeric() ? FCString::Atoi(*Word) : 0;
		return Number >= 1 && Number <= USessionSubsystem::MaxSessions ? Number - 1 : INDEX_NONE;
	}

	void PlaySession(const TArray<FString>& Args, UWorld* World)
	{
		USessionSubsystem* Sessions = FindSessions(World);
		const int32 SlotIndex = Args.Num() == 1 ? ParseSessionNumber(Args[0]) : INDEX_NONE;
		if (!Sessions || SlotIndex == INDEX_NONE)
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage (in a game): Looter.Session.Play <1-%d>"), USessionSubsystem::MaxSessions);
			return;
		}
		Sessions->PlaySession(SlotIndex);
	}

	void SaveSession(const TArray<FString>& Args, UWorld* World)
	{
		USessionSubsystem* Sessions = FindSessions(World);
		if (!Sessions || !Sessions->SaveNow())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Session.Save: no session is being played (start one from the main menu or Looter.Session.Play), or a trip is under way."));
		}
	}

	void OpenMenu(const TArray<FString>& Args, UWorld* World)
	{
		if (USessionSubsystem* Sessions = FindSessions(World))
		{
			Sessions->OpenMainMenu();
		}
	}

	void ListSessions(const TArray<FString>& Args, UWorld* World)
	{
		const USessionSubsystem* Sessions = FindSessions(World);
		if (!Sessions)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Session.List: start the game first."));
			return;
		}
		for (int32 Index = 0; Index < USessionSubsystem::MaxSessions; ++Index)
		{
			const FSessionSummary Summary = Sessions->GetSummary(Index);
			if (!Summary.bExists)
			{
				UE_LOG(LogLooter, Log, TEXT("Session %d: empty"), Index + 1);
				continue;
			}
			UE_LOG(LogLooter, Log, TEXT("Session %d: level %d, %d guns, %s played, saved %s, in %s%s"), Index + 1, Summary.Level, Summary.Weapons,
				*USessionSubsystem::FormatPlayTime(Summary.PlayedSeconds), *USessionSubsystem::FormatSavedTime(Summary.Saved, FDateTime::Now()),
				*Summary.Place, Index == Sessions->GetActiveSession() ? TEXT(" (being played)") : TEXT(""));
		}
	}

	// --- Travel and areas ---

	/** A level by package path ("/Game/Maps/Lvl_Skyreach") or by its file's name ("Lvl_Skyreach"); empty when none is. */
	FString FindLevel(const FString& Words)
	{
		if (Words.StartsWith(TEXT("/")))
		{
			const FString AskedPackage = FPackageName::ObjectPathToPackageName(Words);
			return FPackageName::IsValidLongPackageName(AskedPackage) && FPackageName::DoesPackageExist(AskedPackage) ? AskedPackage : FString();
		}
		IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		TArray<FAssetData> LevelAssets;
		Registry.GetAssetsByClass(UWorld::StaticClass()->GetClassPathName(), LevelAssets);
		for (const FAssetData& LevelAsset : LevelAssets)
		{
			if (LevelAsset.AssetName.ToString().Equals(Words, ESearchCase::IgnoreCase))
			{
				return LevelAsset.PackageName.ToString();
			}
		}
		return FString();
	}

	void Travel(const TArray<FString>& Args, UWorld* World)
	{
		USessionSubsystem* Sessions = FindSessions(World);
		if (!Sessions || Args.Num() < 1 || Args.Num() > 2)
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage (in a game): Looter.Travel <area or level> [landing], e.g. Looter.Travel Skyreach, Looter.Travel RansomsRest, Looter.Travel Lvl_Skyreach Landing_Jetty"));
			return;
		}
		const FName Landing = Args.Num() == 2 ? FName(*Args[1]) : NAME_None;
		if (const UAreaDefinition* Area = UAreaDefinition::FindByName(Args[0]))
		{
			Sessions->TravelToArea(*Area, Landing);
			return;
		}
		const FString LevelPackage = FindLevel(Args[0]);
		if (LevelPackage.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Travel: no area or level is called %s (Looter.Area.List lists the areas)."), *Args[0]);
			return;
		}
		Sessions->TravelToMap(LevelPackage, Landing);
	}

	void ListAreas(const TArray<FString>& Args, UWorld* World)
	{
		const TArray<UAreaDefinition*> Areas = UAreaDefinition::LoadAll();
		if (Areas.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("No area assets (%s): run Tools/Unreal/create_area_assets.py in the editor. Looter.Travel still takes a level."),
				UAreaDefinition::AssetFolder);
			return;
		}
		for (const UAreaDefinition* Area : Areas)
		{
			const FString MapPackage = Area->GetMapPackage();
			const FString Landings = FString::JoinBy(Area->Landings, TEXT(", "), [](FName Tag) { return Tag.ToString(); });
			UE_LOG(LogLooter, Log, TEXT("Area %s \"%s\": %s%s, landings [%s]%s%s"), *Area->GetAreaId().ToString(), *Area->DisplayName.ToString(),
				MapPackage.IsEmpty() ? TEXT("no map") : *MapPackage, Area->HasMap() ? TEXT("") : TEXT(" (not in the game yet)"), *Landings,
				Area->bPractice ? TEXT(", practice") : TEXT(""),
				*(Area->OpeningMission.IsNone() ? FString() : FString::Printf(TEXT(", opens with %s"), *Area->OpeningMission.ToString())));
		}
	}

	// --- Checking a save's upgrade on a copy ---

	/** Where Looter.Session.CheckSave puts its copies: in Saved, beside the save folder and never in it. */
	FString CheckFolder()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SessionCheck")));
	}

	/** A gun in a few words, to compare guns before and after: kind, rarity, level, seed, parts, magazine, named gun. */
	FString DescribeGun(const FWeaponInstanceData& Gun)
	{
		const FString Named = Gun.Named ? FString::Printf(TEXT(" named %s"), *Gun.Named->GetNamedId().ToString()) : FString();
		return FString::Printf(TEXT("%s %s L%d seed %d [%s] mag %d%s"), Gun.Definition ? *Gun.Definition->GetName() : TEXT("(kind missing)"),
			*UEnum::GetValueAsString(Gun.Rarity), Gun.Level, Gun.Seed, *FString::JoinBy(Gun.Parts, TEXT(","), [](FName Part) { return Part.ToString(); }),
			Gun.SavedMagazine, *Named);
	}

	/** What the player carries and has earned, in a few lines, to compare before and after. */
	TArray<FString> DescribePlayer(const ULooterSessionSave& Save)
	{
		TArray<FString> Lines;
		Lines.Add(FString::Printf(TEXT("level %d, %lld XP, tutorial %s, %d kinds met, %d kinds defeated"), Save.Progress.Level, Save.Progress.XP,
			Save.Progress.bTutorialDone ? TEXT("done") : TEXT("not done"), Save.Progress.Encountered.Num(), Save.Progress.Defeated.Num()));
		Lines.Add(FString::Printf(TEXT("health %.1f, spot %s, inventory %s, in hand slot %d"), Save.Health,
			Save.bHasPlayerSpot ? *Save.PlayerLocation.ToString() : TEXT("none"), Save.bHasInventory ? TEXT("kept") : TEXT("none"), Save.Inventory.ActiveSlot));
		for (const FWeaponInstanceData& Gun : Save.Inventory.Equipped)
		{
			Lines.Add(TEXT("equipped: ") + DescribeGun(Gun));
		}
		for (const FWeaponInstanceData& Gun : Save.Inventory.Backpack)
		{
			Lines.Add(TEXT("backpack: ") + DescribeGun(Gun));
		}
		Lines.Add(TEXT("ammo: ") + FString::JoinBy(Save.Inventory.Ammo, TEXT(" "), [](int32 Rounds) { return FString::FromInt(Rounds); }));
		return Lines;
	}

	/** Each map's world in a line, and version 1's one world when it's still there. */
	TArray<FString> DescribeWorlds(const ULooterSessionSave& Save)
	{
		TArray<FString> Lines;
		if (Save.bHasWorld || Save.LootWeapons.Num() > 0 || Save.AmmoPickups.Num() > 0 || Save.Racks.Num() > 0)
		{
			Lines.Add(FString::Printf(TEXT("version 1 world: %d guns and %d ammo on the ground, %d racks, tutorial step %d"),
				Save.LootWeapons.Num(), Save.AmmoPickups.Num(), Save.Racks.Num(), Save.TutorialStep));
		}
		for (const TPair<FString, FSavedMapWorld>& Pair : Save.Worlds)
		{
			Lines.Add(FString::Printf(TEXT("world of %s: %d guns and %d ammo on the ground, %d racks, tutorial step %d"), *Pair.Key,
				Pair.Value.LootWeapons.Num(), Pair.Value.AmmoPickups.Num(), Pair.Value.Racks.Num(), Pair.Value.TutorialStep));
		}
		return Lines;
	}

	void LogLines(const TCHAR* Heading, const TArray<FString>& Lines)
	{
		for (const FString& Line : Lines)
		{
			UE_LOG(LogLooter, Log, TEXT("SESSIONCHECK %s %s"), Heading, *Line);
		}
	}

	/**
	 * Looter.Session.CheckSave [1-3 | file.sav]: reads a session's save (never writing it), copies it to Saved/SessionCheck,
	 * reads the copy as written and as the game reads it (upgraded), and checks nothing was lost: the player's guns, ammo
	 * and progress the same, and version 1's world filed whole under its map. Then the upgraded save goes through a
	 * write and a read as version 2 (kept beside the copy as <name>.v2.sav). Ends with "SESSIONCHECK PASS" or "FAIL".
	 */
	void CheckSave(const TArray<FString>& Args, UWorld* World)
	{
		TArray<uint8> OriginalBytes;
		FString SaveName;
		const int32 SlotIndex = Args.Num() == 0 ? 0 : ParseSessionNumber(Args[0]);
		if (SlotIndex != INDEX_NONE)
		{
			SaveName = USessionSubsystem::SlotName(SlotIndex);
			if (!UGameplayStatics::DoesSaveGameExist(SaveName, 0) || !UGameplayStatics::LoadDataFromSlot(OriginalBytes, SaveName, 0))
			{
				UE_LOG(LogLooter, Warning, TEXT("SESSIONCHECK FAIL: session %d has no save to check."), SlotIndex + 1);
				return;
			}
		}
		else
		{
			SaveName = FPaths::GetBaseFilename(Args[0]);
			if (!FFileHelper::LoadFileToArray(OriginalBytes, *Args[0]))
			{
				UE_LOG(LogLooter, Warning, TEXT("SESSIONCHECK FAIL: couldn't read %s (Looter.Session.CheckSave [1-3 | file.sav])."), *Args[0]);
				return;
			}
		}

		// The copy, outside the save folder; everything below reads the copy.
		const FString CopyPath = FPaths::Combine(CheckFolder(), SaveName + TEXT(".sav"));
		IFileManager::Get().MakeDirectory(*CheckFolder(), true);
		TArray<uint8> CopyBytes;
		if (!FFileHelper::SaveArrayToFile(OriginalBytes, *CopyPath) || !FFileHelper::LoadFileToArray(CopyBytes, *CopyPath) || !(CopyBytes == OriginalBytes))
		{
			UE_LOG(LogLooter, Error, TEXT("SESSIONCHECK FAIL: couldn't make a copy at %s."), *CopyPath);
			return;
		}
		UE_LOG(LogLooter, Log, TEXT("SESSIONCHECK copied %s (%d bytes) to %s"), *SaveName, CopyBytes.Num(), *CopyPath);

		const ULooterSessionSave* AsWritten = Cast<ULooterSessionSave>(UGameplayStatics::LoadGameFromMemory(CopyBytes));
		ULooterSessionSave* AsRead = USessionSubsystem::ReadSave(CopyBytes);
		if (!AsWritten || !AsRead)
		{
			UE_LOG(LogLooter, Error, TEXT("SESSIONCHECK FAIL: %s isn't a session save."), *SaveName);
			return;
		}
		UE_LOG(LogLooter, Log, TEXT("SESSIONCHECK version %d read as version %d; map %s; %.0f s played; saved %s"), AsWritten->Version, AsRead->Version,
			*AsRead->Map, AsRead->PlayedSeconds, *AsRead->Saved.ToString());
		LogLines(TEXT("before:"), DescribePlayer(*AsWritten));
		LogLines(TEXT("before:"), DescribeWorlds(*AsWritten));
		LogLines(TEXT("after:"), DescribePlayer(*AsRead));
		LogLines(TEXT("after:"), DescribeWorlds(*AsRead));
		UE_LOG(LogLooter, Log, TEXT("SESSIONCHECK after: arrival %s; campaign: %d missions done, %d areas open, first cast-off %s"),
			*AsRead->ArrivalTag.ToString(), AsRead->Campaign.CompletedMissions.Num(), AsRead->Campaign.OpenedAreas.Num(),
			AsRead->Campaign.bFirstCastOff ? TEXT("made") : TEXT("not yet"));

		TArray<FString> Problems;
		if (AsRead->Version != ULooterSessionSave::CurrentVersion)
		{
			Problems.Add(FString::Printf(TEXT("read as version %d, not %d"), AsRead->Version, ULooterSessionSave::CurrentVersion));
		}
		if (!(DescribePlayer(*AsWritten) == DescribePlayer(*AsRead)) || AsWritten->Map != AsRead->Map || AsWritten->PlayedSeconds != AsRead->PlayedSeconds)
		{
			Problems.Add(TEXT("the player (guns, ammo, progress, health or spot), the map or the time played changed"));
		}
		if (AsWritten->Version < 2 && AsWritten->bHasWorld)
		{
			const FSavedMapWorld* Filed = AsRead->FindWorld(AsWritten->Map.IsEmpty() ? FString(ULooterSessionSave::Version1Map) : AsWritten->Map);
			if (!Filed || Filed->LootWeapons.Num() != AsWritten->LootWeapons.Num() || Filed->AmmoPickups.Num() != AsWritten->AmmoPickups.Num()
				|| Filed->Racks.Num() != AsWritten->Racks.Num() || Filed->TutorialStep != AsWritten->TutorialStep || AsRead->Worlds.Num() != 1)
			{
				Problems.Add(TEXT("version 1's world wasn't filed whole under its map"));
			}
		}
		if (AsRead->bHasWorld || AsRead->LootWeapons.Num() > 0 || AsRead->AmmoPickups.Num() > 0 || AsRead->Racks.Num() > 0)
		{
			Problems.Add(TEXT("version 1's world fields weren't emptied"));
		}

		// The upgraded save written and read back, as the game will next save and load it.
		TArray<uint8> UpgradedBytes;
		const ULooterSessionSave* Again = UGameplayStatics::SaveGameToMemory(AsRead, UpgradedBytes) ? USessionSubsystem::ReadSave(UpgradedBytes) : nullptr;
		const FString UpgradedPath = FPaths::Combine(CheckFolder(), SaveName + TEXT(".v2.sav"));
		FFileHelper::SaveArrayToFile(UpgradedBytes, *UpgradedPath);
		if (!Again || Again->Version != ULooterSessionSave::CurrentVersion || !(DescribePlayer(*Again) == DescribePlayer(*AsRead))
			|| !(DescribeWorlds(*Again) == DescribeWorlds(*AsRead)) || Again->Map != AsRead->Map)
		{
			Problems.Add(TEXT("the upgraded save doesn't come back the same from a write and a read"));
		}

		if (Problems.IsEmpty())
		{
			UE_LOG(LogLooter, Log, TEXT("SESSIONCHECK PASS: %s reads as version %d with nothing lost (copies in %s; the original was only read)."),
				*SaveName, ULooterSessionSave::CurrentVersion, *CheckFolder());
		}
		else
		{
			UE_LOG(LogLooter, Error, TEXT("SESSIONCHECK FAIL: %s: %s (copies in %s; the original was only read)."), *SaveName,
				*FString::Join(Problems, TEXT("; ")), *CheckFolder());
		}
	}

	/**
	 * Looter.Session.Copy <from> <to>: copies a session's save, as it is on disk, into an empty slot (it never overwrites
	 * one). Playing the copy tries an older save in the game while the original stays as it was.
	 */
	void CopySession(const TArray<FString>& Args, UWorld* World)
	{
		const int32 From = Args.Num() == 2 ? ParseSessionNumber(Args[0]) : INDEX_NONE;
		const int32 To = Args.Num() == 2 ? ParseSessionNumber(Args[1]) : INDEX_NONE;
		if (From == INDEX_NONE || To == INDEX_NONE || From == To)
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage: Looter.Session.Copy <from 1-%d> <to an empty slot 1-%d>"), USessionSubsystem::MaxSessions, USessionSubsystem::MaxSessions);
			return;
		}
		const USessionSubsystem* Sessions = FindSessions(World);
		if (UGameplayStatics::DoesSaveGameExist(USessionSubsystem::SlotName(To), 0) || (Sessions && Sessions->GetActiveSession() == To))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Session.Copy: session %d isn't empty, so nothing was copied (it never overwrites a session)."), To + 1);
			return;
		}
		TArray<uint8> SaveBytes;
		if (!UGameplayStatics::LoadDataFromSlot(SaveBytes, USessionSubsystem::SlotName(From), 0))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Session.Copy: session %d has no save."), From + 1);
			return;
		}
		const bool bCopied = UGameplayStatics::SaveDataToSlot(SaveBytes, USessionSubsystem::SlotName(To), 0);
		UE_LOG(LogLooter, Log, TEXT("Looter.Session.Copy: session %d %s session %d (%d bytes, as on disk)."), From + 1,
			bCopied ? TEXT("copied into") : TEXT("could NOT be copied into"), To + 1, SaveBytes.Num());
	}

	FAutoConsoleCommandWithWorldAndArgs PlaySessionCommand(
		TEXT("Looter.Session.Play"),
		TEXT("Plays a session as the main menu would (a new game when it's empty): Looter.Session.Play <1-3>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PlaySession));

	FAutoConsoleCommandWithWorldAndArgs SaveSessionCommand(
		TEXT("Looter.Session.Save"),
		TEXT("Saves the session being played now."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SaveSession));

	FAutoConsoleCommandWithWorldAndArgs OpenMenuCommand(
		TEXT("Looter.Session.Menu"),
		TEXT("Opens the main menu (the session being played saves as its level ends)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&OpenMenu));

	FAutoConsoleCommandWithWorldAndArgs ListSessionsCommand(
		TEXT("Looter.Session.List"),
		TEXT("Lists the three sessions: level, guns, play time, when saved, where."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListSessions));

	FAutoConsoleCommandWithWorldAndArgs CheckSaveCommand(
		TEXT("Looter.Session.CheckSave"),
		TEXT("Checks a session save's upgrade on a copy in Saved/SessionCheck (the save itself is only read): Looter.Session.CheckSave [1-3 | file.sav]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CheckSave));

	FAutoConsoleCommandWithWorldAndArgs CopySessionCommand(
		TEXT("Looter.Session.Copy"),
		TEXT("Copies a session's save into an empty slot, never over one: Looter.Session.Copy <from> <to>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&CopySession));

	FAutoConsoleCommandWithWorldAndArgs TravelCommand(
		TEXT("Looter.Travel"),
		TEXT("Travels to an area or a level, saving the session: Looter.Travel <area or level> [landing], e.g. Looter.Travel Skyreach, Looter.Travel Lvl_Skyreach"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Travel));

	FAutoConsoleCommandWithWorldAndArgs ListAreasCommand(
		TEXT("Looter.Area.List"),
		TEXT("Lists the areas (UAreaDefinition assets): id, name, level, landings, practice, opening mission."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ListAreas));
}

#endif

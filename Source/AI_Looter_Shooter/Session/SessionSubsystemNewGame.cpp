// USessionSubsystem: a new game, which starts the story at its first arrival (the cold open, then Ellis's grave on
// Ransom's Rest). Skyreach is no longer the start (the user's call, 2026-10-13): it comes later, from the Lily.

#include "Session/SessionSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Areas/AreaDefinition.h"
#include "Areas/StationBoard.h"
#include "Scenes/TransitionScreenSubsystem.h"
#include "Session/SessionSave.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/WeaponDefinition.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameMapsSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"

namespace
{
	/** The assault rifle's kind: its parts build the white bullpup, the game's only non-legendary rifle. */
	const TCHAR* BullpupPath = TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle");

	const UAreaDefinition* FindFirstArea()
	{
		return UAreaDefinition::FindByName(StationBoard::FirstAreaId().ToString());
	}
}

UWeaponDefinition* USessionSubsystem::LoadBullpup()
{
	return LoadObject<UWeaponDefinition>(nullptr, BullpupPath);
}

FString USessionSubsystem::NewGameMap()
{
	// "/Game/Maps/Lvl_TutorialIsland.Lvl_TutorialIsland" is the object; OpenLevel and the saves use the package. Still the
	// menu's backdrop and the fallback, though a new game starts on the story's first level.
	const FString DefaultMap = UGameMapsSettings::GetGameDefaultMap();
	return FPackageName::ObjectPathToPackageName(DefaultMap);
}

bool USessionSubsystem::CanStartStory()
{
	const UAreaDefinition* First = FindFirstArea();
	return First && First->HasMap();
}

bool USessionSubsystem::ApplyNewGame(ULooterSessionSave& Save, const UAreaDefinition& FirstArea, UWeaponDefinition* Bullpup)
{
	// The story has begun: the first cast-off is how the record says so (the cold open is due from it, and the boards
	// list the story's areas). Skyreach's old tutorial counts as done, so a visit there later never sends the player
	// through it, and its jetty never offers the first trip again. The save's names stay as they were for old sessions.
	Save.Progress.bTutorialDone = true;
	StationBoard::RecordFirstCastOff(Save.Campaign);

	// The Reaches bury their dead with their irons: a Common Bullpup in the coffin, in hand, with its starting magazines.
	bool bArmed = false;
	if (Bullpup)
	{
		const FWeaponInstanceData Gun = UWeaponRollLibrary::RollWeaponWithRarity(Bullpup, EWeaponRarity::Common, 1);
		Save.Inventory.Equipped = { Gun };
		Save.Inventory.ActiveSlot = 0;
		Save.Inventory.Backpack.Reset();
		Save.Inventory.Ammo.Init(0, LooterAmmo::NumTypes);
		if (LooterAmmo::IsValid(Bullpup->AmmoType))
		{
			Save.Inventory.Ammo[static_cast<int32>(Bullpup->AmmoType)] = Gun.Stats.MagazineSize * Bullpup->StartingReserveMagazines;
		}
		Save.bHasInventory = true;
		bArmed = true;
	}

	// Straight to the story's first arrival: the session continues in the first area's level, at the family plot's grave.
	Save.PrepareTrip(FirstArea.GetMapPackage(), StationBoard::FirstArrivalLanding());
	return bArmed;
}

bool USessionSubsystem::StartNewGame(int32 Index)
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (Index < 0 || Index >= MaxSessions || !World)
	{
		return false;
	}
	if (UGameplayStatics::DoesSaveGameExist(SlotName(Index), 0))
	{
		UE_LOG(LogLooter, Warning, TEXT("Session %d isn't empty: a new game only starts in an empty slot."), Index + 1);
		return false;
	}
	const UAreaDefinition* First = FindFirstArea();
	if (!First || !First->HasMap())
	{
		UE_LOG(LogLooter, Warning, TEXT("New game: the story's first level isn't in the game yet, so session %d starts nowhere."), Index + 1);
		return false;
	}

	ULooterSessionSave* Save = NewSave();
	Save->Saved = Save->Created;
	if (!ApplyNewGame(*Save, *First, LoadBullpup()))
	{
		UE_LOG(LogLooter, Warning, TEXT("New game: no Bullpup (%s), so session %d starts unarmed."), BullpupPath, Index + 1);
	}
	// Written first, so the level reads it as it opens, as it reads a trip's.
	if (!UGameplayStatics::SaveGameToSlot(Save, SlotName(Index), 0))
	{
		UE_LOG(LogLooter, Error, TEXT("New game: session %d couldn't be saved, so it doesn't start."), Index + 1);
		return false;
	}

	// Behind the white: the cold open takes it on the story's first level (UAreaTravelSubsystem), or, with scenes off, the
	// title rises through it there.
	if (UTransitionScreenSubsystem* White = GetGameInstance()->GetSubsystem<UTransitionScreenSubsystem>())
	{
		White->HoldWhite();
	}
	const FString Map = First->GetMapPackage();
	UE_LOG(LogLooter, Log, TEXT("Session %d: new game, the story begins in %s at %s"), Index + 1, *Map,
		*StationBoard::FirstArrivalLanding().ToString());
	UGameplayStatics::OpenLevel(World, FName(*Map), /*bAbsolute*/ true, FString::Printf(TEXT("Session=%d"), Index + 1));
	return true;
}

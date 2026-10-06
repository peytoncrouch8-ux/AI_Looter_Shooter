// USessionSubsystem: "Skip the tutorial", a new game that starts on the story's first arrival.

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
#include "Kismet/GameplayStatics.h"

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

bool USessionSubsystem::CanSkipTutorial()
{
	const UAreaDefinition* First = FindFirstArea();
	return First && First->HasMap();
}

bool USessionSubsystem::ApplyTutorialSkip(ULooterSessionSave& Save, const UAreaDefinition& FirstArea, UWeaponDefinition* Bullpup)
{
	// It counts as the first cast-off, and the tutorial as done: Skyreach is a practice island from here on, and a visit
	// there never sends the player through the tutorial again.
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

bool USessionSubsystem::PlaySessionSkippingTutorial(int32 Index)
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (Index < 0 || Index >= MaxSessions || !World)
	{
		return false;
	}
	if (UGameplayStatics::DoesSaveGameExist(SlotName(Index), 0))
	{
		UE_LOG(LogLooter, Warning, TEXT("Session %d isn't empty: only a new game skips the tutorial."), Index + 1);
		return false;
	}
	const UAreaDefinition* First = FindFirstArea();
	if (!First || !First->HasMap())
	{
		UE_LOG(LogLooter, Warning, TEXT("Skip the tutorial: the story's first level isn't in the game yet, so session %d starts nowhere."), Index + 1);
		return false;
	}

	ULooterSessionSave* Save = NewSave();
	Save->Saved = Save->Created;
	if (!ApplyTutorialSkip(*Save, *First, LoadBullpup()))
	{
		UE_LOG(LogLooter, Warning, TEXT("Skip the tutorial: no Bullpup (%s), so session %d starts unarmed."), BullpupPath, Index + 1);
	}
	// Written first, so the level reads it as it opens, as it reads a trip's.
	if (!UGameplayStatics::SaveGameToSlot(Save, SlotName(Index), 0))
	{
		UE_LOG(LogLooter, Error, TEXT("Skip the tutorial: session %d couldn't be saved, so it doesn't start."), Index + 1);
		return false;
	}

	// Behind the white, as the first cast-off arrives: the title rises through it on the story's first level.
	if (UTransitionScreenSubsystem* White = GetGameInstance()->GetSubsystem<UTransitionScreenSubsystem>())
	{
		White->HoldWhite();
	}
	const FString Map = First->GetMapPackage();
	UE_LOG(LogLooter, Log, TEXT("Session %d: new game, tutorial skipped, in %s at %s"), Index + 1, *Map,
		*StationBoard::FirstArrivalLanding().ToString());
	UGameplayStatics::OpenLevel(World, FName(*Map), /*bAbsolute*/ true, FString::Printf(TEXT("Session=%d"), Index + 1));
	return true;
}

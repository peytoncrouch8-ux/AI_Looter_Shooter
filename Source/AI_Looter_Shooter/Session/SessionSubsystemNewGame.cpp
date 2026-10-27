// USessionSubsystem: a new game, which starts the story at its first arrival (the cold open, then Ellis's grave on
// Ransom's Rest). Skyreach is no longer the start (the user's call, 2026-10-13): it comes later, from the Lily.

#include "Session/SessionSubsystem.h"
#include "Story/NewGameIntroSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Areas/AreaDefinition.h"
#include "Areas/StationBoard.h"
#include "Embers/HeroDefinition.h"
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
	// The fallback a session lands on when its level is gone (a new game starts on the story's first level). It was the
	// default map until the menu moved to Teropa's own scene (2026-10-27), so it's named here now.
	return TEXT("/Game/Maps/Lvl_TutorialIsland");
}

FString USessionSubsystem::MenuMap()
{
	// The default map is the menu's scene (Lvl_MenuTeropa since 2026-10-27); OpenLevel takes the package.
	return FPackageName::ObjectPathToPackageName(UGameMapsSettings::GetGameDefaultMap());
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

bool USessionSubsystem::ApplyNewGameHero(ULooterSessionSave& Save, FName HeroId)
{
	// Only a hero the select screen would let through: one in the works plays only by the developers' unlock.
	const UHeroDefinition* Hero = UHeroDefinition::Load(HeroId);
	if (!Hero || !Hero->CanBePicked())
	{
		UE_LOG(LogLooter, Warning, TEXT("New game: %s %s."), *HeroId.ToString(),
			Hero ? TEXT("is in the works (Looter.Hero.Unlock 1 lets one be picked)") : TEXT("isn't a hero (no DA_Hero_ asset)"));
		return false;
	}
	Save.HeroId = Hero->Id;
	return true;
}

bool USessionSubsystem::StartNewGame(int32 Index, FName HeroId)
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
	// The hero first: the session keeps them for good, so a refused one starts nothing.
	if (!ApplyNewGameHero(*Save, HeroId))
	{
		return false;
	}
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

	const FString Map = First->GetMapPackage();
	UE_LOG(LogLooter, Log, TEXT("Session %d: new game as %s, the story begins in %s at %s"), Index + 1, *Save->HeroId.ToString(), *Map,
		*StationBoard::FirstArrivalLanding().ToString());
	// Behind the white: the cold open takes it on the story's first level (UAreaTravelSubsystem), or, with scenes off, the
	// title rises through it there.
	const TWeakObjectPtr<USessionSubsystem> WeakThis = this;
	TFunction<void()> Open = [WeakThis, Map, Index]()
	{
		USessionSubsystem* Sessions = WeakThis.Get();
		UWorld* Now = Sessions ? Sessions->GetGameInstance()->GetWorld() : nullptr;
		if (!Now)
		{
			return;
		}
		if (UTransitionScreenSubsystem* White = Sessions->GetGameInstance()->GetSubsystem<UTransitionScreenSubsystem>())
		{
			White->HoldWhite();
		}
		UGameplayStatics::OpenLevel(Now, FName(*Map), /*bAbsolute*/ true, FString::Printf(TEXT("Session=%d"), Index + 1));
	};
	// The new game's intro first, when the menu is Teropa's scene (Story/NewGameIntroSubsystem): the level opens at its white.
	UNewGameIntroSubsystem* Intro = GetGameInstance()->GetSubsystem<UNewGameIntroSubsystem>();
	if (!Intro || !Intro->PlayPlanet(*World, Index, Open))
	{
		Open();
	}
	return true;
}

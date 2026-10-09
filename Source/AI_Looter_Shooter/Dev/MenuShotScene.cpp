// The scene of Looter.MenuShots: the loadout it gives the player and how each state of the inventory is made (not in
// shipping builds). MenuShotDevCommands.cpp runs the steps and takes the pictures; MenuShotScene.h says what a step is.

#include "Dev/MenuShotScene.h"
#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Tutorial/TutorialDirector.h"
#include "UI/HUD/LooterHUD.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponCurses.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponParts.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Widgets/SWindow.h"

namespace
{
	using MenuShotScene::FStep;

	/** After a request the screenshot is written at the end of the frame; this long before the next step changes the picture. */
	constexpr float DefaultGapSeconds = 0.6f;
	/** After the last picture, before the game quits: the file is written from another thread. */
	constexpr float LastGapSeconds = 1.5f;
	/** Where the mouse is parked, from the window's top-left (pixels): left of every list, so it hovers nothing. */
	const FVector2D ParkedMouse(6.0, 420.0);

	/** Everything the steps act on, found afresh each time. */
	struct FMenuShotPlayer
	{
		APlayerController* Controller = nullptr;
		APawn* Pawn = nullptr;
		UWeaponManagerComponent* Weapons = nullptr;
		ALooterHUD* Hud = nullptr;

		bool IsReady() const { return Weapons && Hud; }
	};

	FMenuShotPlayer MenuShotFindPlayer(UWorld& World)
	{
		FMenuShotPlayer Player;
		Player.Controller = World.GetFirstPlayerController();
		Player.Pawn = Player.Controller ? Player.Controller->GetPawn() : nullptr;
		Player.Weapons = Player.Pawn ? Player.Pawn->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
		Player.Hud = ALooterHUD::FindFor(Player.Controller);
		return Player;
	}

	/** An asset of Class called ExactName, or else the first whose name contains NameContains. */
	UObject* MenuShotFindAsset(UClass* Class, const TCHAR* ExactName, const TCHAR* NameContains)
	{
		TArray<FAssetData> Assets;
		IAssetRegistry::GetChecked().GetAssetsByClass(Class->GetClassPathName(), Assets, true);
		const FAssetData* Found = Assets.FindByPredicate([ExactName](const FAssetData& Asset)
			{
				return Asset.AssetName.ToString().Equals(ExactName, ESearchCase::IgnoreCase);
			});
		if (!Found && NameContains)
		{
			Found = Assets.FindByPredicate([NameContains](const FAssetData& Asset)
				{
					return Asset.AssetName.ToString().Contains(NameContains);
				});
		}
		return Found ? Found->GetAsset() : nullptr;
	}

	/**
	 * A gun as a found one is: fixed seed (so every run makes the same guns, names and pictures), its parts saved by key,
	 * its notches and curse, and its stats worked out from all of it.
	 */
	FWeaponInstanceData MenuShotGun(UWeaponDefinition& Definition, EWeaponRarity Rarity, int32 Level, int32 Seed, int32 Kills = 0,
		FName Curse = NAME_None)
	{
		FWeaponInstanceData Gun = UWeaponRollLibrary::RollWeaponWithRarity(&Definition, Rarity, Level);
		Gun.Seed = Seed;
		Gun.Parts = WeaponParts::PartKeys(WeaponParts::Pick(Definition, Seed, Rarity));
		Gun.Kills = Kills;
		Gun.Curse = Curse;
		Gun.bCurseLifted = false;
		Gun.Stats = UWeaponRollLibrary::ComputeInstanceStats(Gun);
		Gun.SavedMagazine = -1;
		return Gun;
	}

	/** Hungry (+30% damage, reloads cost health), or the first curse there is. */
	FName MenuShotCurse()
	{
		const TConstArrayView<FWeaponCurse> Curses = WeaponCurses::All();
		const FWeaponCurse* Hungry = Curses.FindByPredicate([](const FWeaponCurse& Curse) { return Curse.Key == FName(TEXT("Hungry")); });
		return Hungry ? Hungry->Key : (Curses.IsEmpty() ? NAME_None : Curses[0].Key);
	}

	/** A key pressed and let go, as the keyboard sends it: Slate routes it to the focused screen. */
	void MenuShotPress(const FKey& Key)
	{
		if (!FSlateApplication::IsInitialized())
		{
			return;
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.MenuShots: key %s"), *Key.ToString());
		FSlateApplication& Slate = FSlateApplication::Get();
		const FModifierKeysState NoModifiers;
		const uint32 User = static_cast<uint32>(FMath::Max(Slate.GetUserIndexForKeyboard(), 0));
		Slate.ProcessKeyDownEvent(FKeyEvent(Key, NoModifiers, User, false, 0u, 0u));
		Slate.ProcessKeyUpEvent(FKeyEvent(Key, NoModifiers, User, false, 0u, 0u));
	}

	void MenuShotShowPage(UWorld& World, EInventoryPage Page)
	{
		const FMenuShotPlayer Player = MenuShotFindPlayer(World);
		if (Player.Hud)
		{
			Player.Hud->ShowInventoryPage(Page);
		}
	}

	/** The mouse to a spot over no list or button, so the screens' hover follows only the keys. */
	void MenuShotParkMouse()
	{
		const TSharedPtr<SWindow> Window = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWindow() : nullptr;
		if (Window.IsValid() && FSlateApplication::IsInitialized())
		{
			const FVector2D WindowPosition = Window->GetPositionInScreen();
			FSlateApplication::Get().SetCursorPos(WindowPosition + ParkedMouse);
		}
	}

	// --- The steps' beginnings, moments and endings ---

	void MenuShotBeginOpen(UWorld& World)
	{
		MenuShotParkMouse();
		MenuShotShowPage(World, EInventoryPage::Loadout);
	}

	bool MenuShotInventoryOpen(UWorld& World)
	{
		const FMenuShotPlayer Player = MenuShotFindPlayer(World);
		return Player.Hud && Player.Hud->IsInventoryOpen();
	}

	/** The backpack's first gun (best for the slot in hand: the cursed iron). */
	void MenuShotBeginBackpack(UWorld& World)
	{
		MenuShotPress(EKeys::D);
	}

	/** The next backpack gun (an upgrade over the rifle in hand). */
	void MenuShotBeginNext(UWorld& World)
	{
		MenuShotPress(EKeys::S);
	}

	void MenuShotBeginInspect(UWorld& World)
	{
		MenuShotPress(EKeys::X);
	}

	/** Back from Inspect, and the upgrade equipped: caught while its slot still flashes. */
	void MenuShotBeginEquip(UWorld& World)
	{
		MenuShotPress(EKeys::X);
		MenuShotPress(EKeys::E);
	}

	/** The second slot (the shotgun) chosen as the swap target: the backpack's shotguns first, compared with it. */
	void MenuShotBeginSwapTarget(UWorld& World)
	{
		MenuShotPress(EKeys::A);
		MenuShotPress(EKeys::S);
		MenuShotPress(EKeys::E);
	}

	void MenuShotBeginSort(UWorld& World)
	{
		MenuShotPress(EKeys::R);
	}

	void MenuShotBeginBestiary(UWorld& World)
	{
		MenuShotShowPage(World, EInventoryPage::Bestiary);
	}

	void MenuShotBeginMissions(UWorld& World)
	{
		MenuShotShowPage(World, EInventoryPage::Missions);
	}

	void MenuShotEndClose(UWorld& World)
	{
		const FMenuShotPlayer Player = MenuShotFindPlayer(World);
		if (Player.Hud)
		{
			Player.Hud->ShowInventoryPage(EInventoryPage::Loadout);
			Player.Hud->CloseInventory();
		}
	}

	// The first picture waits out the showcase's swing and the gun's textures streaming in; each later one the screen's
	// card sliding in (0.16 s) and the showcase's swing (0.45 s). The equip is caught at 0.2 s, its row's flash still bright.
	const FStep MenuShotSteps[] =
	{
		// Name             Seconds  Begin                        Ready                     After                Gap
		{ TEXT("loadout"),      1.8f, &MenuShotBeginOpen,         &MenuShotInventoryOpen,   nullptr,             DefaultGapSeconds },
		{ TEXT("backpack"),     1.0f, &MenuShotBeginBackpack,     nullptr,                  nullptr,             DefaultGapSeconds },
		{ TEXT("upgrade"),      1.0f, &MenuShotBeginNext,         nullptr,                  nullptr,             DefaultGapSeconds },
		{ TEXT("inspect"),      1.2f, &MenuShotBeginInspect,      nullptr,                  nullptr,             DefaultGapSeconds },
		{ TEXT("equipped"),     0.2f, &MenuShotBeginEquip,        nullptr,                  nullptr,             1.0f },
		{ TEXT("swap_target"),  1.0f, &MenuShotBeginSwapTarget,   nullptr,                  nullptr,             DefaultGapSeconds },
		{ TEXT("sorted"),       0.8f, &MenuShotBeginSort,         nullptr,                  nullptr,             DefaultGapSeconds },
		{ TEXT("bestiary"),     1.5f, &MenuShotBeginBestiary,     nullptr,                  nullptr,             DefaultGapSeconds },
		{ TEXT("missions"),     1.2f, &MenuShotBeginMissions,     nullptr,                  &MenuShotEndClose,   LastGapSeconds },
	};
}

namespace MenuShotScene
{
	TConstArrayView<FStep> Steps()
	{
		return MakeArrayView(MenuShotSteps);
	}

	UWorld* FindGameWorld()
	{
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.World() && Context.World()->IsGameWorld())
				{
					return Context.World();
				}
			}
		}
		return nullptr;
	}

	bool IsPlayerReady(UWorld& World)
	{
		return MenuShotFindPlayer(World).IsReady();
	}

	void Prepare(UWorld& World)
	{
		const FMenuShotPlayer Player = MenuShotFindPlayer(World);
		if (!Player.IsReady())
		{
			return;
		}
		UWeaponDefinition* Rifle = Cast<UWeaponDefinition>(MenuShotFindAsset(UWeaponDefinition::StaticClass(), TEXT("DA_AssaultRifle"), TEXT("Rifle")));
		UWeaponDefinition* Shotgun = Cast<UWeaponDefinition>(MenuShotFindAsset(UWeaponDefinition::StaticClass(), TEXT("DA_PumpShotgun"), TEXT("Shotgun")));
		if (!Rifle || !Shotgun)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.MenuShots: no rifle (DA_AssaultRifle) or shotgun (DA_PumpShotgun) definition found."));
			return;
		}
		UWeaponManagerComponent& Weapons = *Player.Weapons;
		Player.Hud->CloseInventory();
		Weapons.ClearInventory();

		// Equipped: the rifle in hand, a shotgun Named by its notches (its nickname in quotes), a Blooded rifle.
		Weapons.GiveWeapon(MenuShotGun(*Rifle, EWeaponRarity::Rare, 12, 4711));
		Weapons.GiveWeapon(MenuShotGun(*Shotgun, EWeaponRarity::Epic, 11, 1138, 312));
		Weapons.GiveWeapon(MenuShotGun(*Rifle, EWeaponRarity::Uncommon, 10, 2024, 63));

		// The backpack: the cursed iron first (the old list kept backpack order), then rifles and shotguns of every rarity.
		Weapons.AddToBackpack(MenuShotGun(*Rifle, EWeaponRarity::Legendary, 12, 9001, 17, MenuShotCurse()));
		Weapons.AddToBackpack(MenuShotGun(*Rifle, EWeaponRarity::Epic, 13, 3141));
		Weapons.AddToBackpack(MenuShotGun(*Rifle, EWeaponRarity::Common, 9, 1001));
		Weapons.AddToBackpack(MenuShotGun(*Rifle, EWeaponRarity::Rare, 11, 8080));
		Weapons.AddToBackpack(MenuShotGun(*Shotgun, EWeaponRarity::Rare, 12, 5150));
		Weapons.AddToBackpack(MenuShotGun(*Shotgun, EWeaponRarity::Uncommon, 8, 777));
		Weapons.AddToBackpack(MenuShotGun(*Shotgun, EWeaponRarity::Common, 10, 4242));
		if (UNamedWeaponDefinition* Heirloom = Cast<UNamedWeaponDefinition>(MenuShotFindAsset(UNamedWeaponDefinition::StaticClass(),
			TEXT("DA_Named_Heirloom"), UNamedWeaponDefinition::AssetPrefix)))
		{
			Weapons.AddToBackpack(Heirloom->MakeInstance(12));
		}
		Weapons.EquipSlot(0);

		// Ammo of every kind, one kind with none (it shows faded).
		const int32 Rounds[] = { 180, 40, 60, 0, 12 };
		const TConstArrayView<EAmmoType> Types = LooterAmmo::AllTypes();
		for (int32 Index = 0; Index < Types.Num() && Index < UE_ARRAY_COUNT(Rounds); ++Index)
		{
			Weapons.AddAmmo(Types[Index], Rounds[Index]);
		}

		// The tutorial rests on the notice board step (the guns are in hand already, so "Find a gun in town" is behind it),
		// which never asks for the inventory, so its tracker stays under the screen.
		TActorIterator<ATutorialDirector> It(&World);
		if (ATutorialDirector* Director = It ? *It : nullptr)
		{
			if (Director->GetCurrentStep() == INDEX_NONE)
			{
				Director->Restart();
			}
			Director->ResumeAtStep(ATutorialDirector::BoardStep);
		}
		MenuShotParkMouse();
		UE_LOG(LogLooter, Display, TEXT("Looter.MenuShots: %d guns equipped, %d in the backpack."), Weapons.GetWeapons().Num(), Weapons.GetBackpack().Num());
	}
}

#endif

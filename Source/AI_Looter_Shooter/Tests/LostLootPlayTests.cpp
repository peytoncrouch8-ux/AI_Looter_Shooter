#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Economy/WalletSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/LostLootRules.h"
#include "Loot/LostLootStore.h"
#include "Loot/LostLootSubsystem.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Lost Property in test levels (the rules and shelves alone are LostLootTests.cpp; the screen is LostLootScreenTests.cpp):
// a gun's five minutes on the ground and what keeps it there, every ground gun handed in on leaving, the guns a save kept
// lying handed in at the load (never put back, never twice), an old save's pile coming down to the shelves' size, and
// claiming and shelving at the window.

namespace
{
	UWeaponDefinition* LoadRifle()
	{
		return LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	}

	/** A rolled gun told apart by Seed (its look and stats are its roll's; the seed only names it here). */
	FWeaponInstanceData MakeGun(UWeaponDefinition* Rifle, int32 Seed)
	{
		FWeaponInstanceData Gun = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Rare, 4);
		Gun.Seed = Seed;
		return Gun;
	}

	/** A gun lying loose at Where, as a kill or a drop leaves one. */
	AWeaponBase* DropGun(UWorld* World, UWeaponDefinition* Rifle, int32 Seed, const FVector& Where)
	{
		AWeaponBase* Gun = UWeaponRollLibrary::SpawnWeapon(World, MakeGun(Rifle, Seed), FTransform(Where));
		if (Gun)
		{
			Gun->Toss(FVector::ZeroVector);
		}
		return Gun;
	}

	/** The seeds of the guns lying loose in World, in no order. */
	TArray<int32> GroundSeeds(UWorld* World)
	{
		TArray<int32> Seeds;
		for (TActorIterator<AWeaponBase> It(World); It; ++It)
		{
			if (It->IsPickup() && !It->IsActorBeingDestroyed())
			{
				Seeds.Add(It->GetInstance().Seed);
			}
		}
		Seeds.Sort();
		return Seeds;
	}

	/** The seeds on the shelves, oldest first. */
	TArray<int32> ShelfSeeds(const FLostLootStore& Store)
	{
		TArray<int32> Seeds;
		for (const FLostLootEntry& Entry : Store.Entries)
		{
			Seeds.Add(Entry.Weapon.Seed);
		}
		return Seeds;
	}

	/** Through the save format and back, read as the game reads a session; null when it failed. */
	ULooterSessionSave* WriteAndRead(ULooterSessionSave* Save)
	{
		TArray<uint8> Bytes;
		return UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	}

	ULooterSessionSave* NewSave()
	{
		ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
		Save->Version = ULooterSessionSave::CurrentVersion;
		return Save;
	}

	/** Files the world kept for From under To's map too (the test's second level stands in for the same map, played again). */
	void FileAs(ULooterSessionSave& Save, UWorld* From, UWorld* To)
	{
		if (const FSavedMapWorld* Kept = Save.FindWorld(USessionSubsystem::MapOf(From)))
		{
			// Copied first: adding the second map's entry may move the first's.
			const FSavedMapWorld Copy = *Kept;
			Save.FindOrAddWorld(USessionSubsystem::MapOf(To)) = Copy;
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLostLootGroundLifeTest, "Looter.LostLoot.GroundLife",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLostLootGroundLifeTest::RunTest(const FString& Parameters)
{
	// Five minutes of play on the ground, then Lost Property: not from under the player's feet (for a minute more), not
	// while a scene hides it, and a gun picked up and dropped again starts its five minutes over.
	FTestWorldWrapper Level;
	UWeaponDefinition* Rifle = LoadRifle();
	if (!TestNotNull(TEXT("Rifle definition"), Rifle) || !TestTrue(TEXT("Test level made"), Level.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = Level.GetTestWorld();
	ULostLootSubsystem* LostLoot = ULostLootSubsystem::Get(World);
	if (!TestNotNull(TEXT("Lost Property in the level"), LostLoot))
	{
		return false;
	}
	AWeaponBase* Near = DropGun(World, Rifle, 1, FVector(0.0, 0.0, 50.0));
	AWeaponBase* Far = DropGun(World, Rifle, 2, FVector(3000.0, 0.0, 50.0));
	AWeaponBase* Hidden = DropGun(World, Rifle, 3, FVector(-3000.0, 0.0, 50.0));
	if (!TestTrue(TEXT("Three guns lying there"), Near && Far && Hidden && GroundSeeds(World).Num() == 3))
	{
		return false;
	}
	Hidden->SetActorHiddenInGame(true);

	// A second short of five minutes: all still there.
	LostLoot->AdvanceAt(LostLootRules::GroundLifeSeconds - 1.f, TOptional<FVector>());
	TestEqual(TEXT("A second short: all three lie there"), GroundSeeds(World).Num(), 3);
	TestNearlyEqual(TEXT("...with a second left"), LostLoot->GetLifeLeft(*Far), 1.f, 0.01f);
	TestNearlyEqual(TEXT("...but the hidden one hasn't aged"), LostLoot->GetLifeLeft(*Hidden), LostLootRules::GroundLifeSeconds, 0.01f);

	// Time up: the player stands by the near one, so only the far one goes.
	const FVector PlayerAt(150.0, 0.0, 50.0);
	LostLoot->AdvanceAt(2.f, PlayerAt);
	TestTrue(TEXT("Time up: the far one gone to Lost Property"), GroundSeeds(World) == TArray<int32>({ 1, 3 }));
	TestTrue(TEXT("...on the shelves, left lying, found here"), ShelfSeeds(LostLoot->GetStore()) == TArray<int32>{ 2 }
		&& LostLoot->GetStore().Entries[0].Reason == ELostLootReason::LeftLying
		&& LostLoot->GetStore().Entries[0].Map == USessionSubsystem::MapOf(World));
	TestTrue(TEXT("...the first one ever: explained on the HUD once the player can see it, never a count"),
		LostLoot->IsExplainWaiting() && LostLoot->GetUnannounced() == 0);

	// The near one waits by the player, but not for good.
	LostLoot->AdvanceAt(LostLootRules::NearGraceSeconds * 0.5f, PlayerAt);
	TestTrue(TEXT("By the player, it waits"), GroundSeeds(World).Contains(1));
	LostLoot->AdvanceAt(LostLootRules::NearGraceSeconds, PlayerAt);
	TestFalse(TEXT("...for a minute at most"), GroundSeeds(World).Contains(1));
	TestTrue(TEXT("The hidden one still lies there"), GroundSeeds(World) == TArray<int32>{ 3 });

	// Picked up, it's the player's: dropped again, it has five whole minutes.
	APawn* Holder = World->SpawnActor<APawn>();
	UWeaponManagerComponent* Inventory = Holder ? NewObject<UWeaponManagerComponent>(Holder) : nullptr;
	if (!TestNotNull(TEXT("A holder"), Inventory))
	{
		return false;
	}
	Inventory->RegisterComponent();
	Inventory->MaxWeapons = 3;
	AWeaponBase* Kept = DropGun(World, Rifle, 4, FVector(0.0, 500.0, 50.0));
	LostLoot->AdvanceAt(200.f, TOptional<FVector>());
	TestNearlyEqual(TEXT("A new gun, a hundred seconds left"), LostLoot->GetLifeLeft(*Kept), 100.f, 0.01f);
	TestTrue(TEXT("Picked up"), Inventory->TryPickup(Kept) && !Kept->IsPickup());
	LostLoot->AdvanceAt(200.f, TOptional<FVector>());
	TestTrue(TEXT("Carried, it never goes"), Inventory->GetWeapons().Contains(Kept) && ShelfSeeds(LostLoot->GetStore()).Num() == 2);
	TestTrue(TEXT("Dropped again"), Inventory->DropActiveWeapon() == Kept && Kept->IsPickup());
	LostLoot->AdvanceAt(1.f, TOptional<FVector>());
	TestNearlyEqual(TEXT("...five whole minutes again"), LostLoot->GetLifeLeft(*Kept), LostLootRules::GroundLifeSeconds - 1.f, 0.01f);

	// Shown by its scene, the hidden one starts aging.
	Hidden->SetActorHiddenInGame(false);
	LostLoot->AdvanceAt(LostLootRules::GroundLifeSeconds, TOptional<FVector>());
	TArray<int32> Shelved = ShelfSeeds(LostLoot->GetStore());
	TestTrue(TEXT("Shown, its five minutes run, and it goes with the dropped one, after the two before them"), GroundSeeds(World).IsEmpty()
		&& Shelved.Num() == 4 && Shelved[0] == 2 && Shelved[1] == 1);
	Shelved.Sort();
	TestTrue(TEXT("...each gun once"), Shelved == TArray<int32>({ 1, 2, 3, 4 }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLostLootLeavingTest, "Looter.LostLoot.Leaving",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLostLootLeavingTest::RunTest(const FString& Parameters)
{
	// Leaving the area hands in every gun on the ground at once, the longest lain first, before the world is kept: the map's
	// save keeps none, the shelves have them, and the next level (the trip's arrival) has them too, told of on its HUD.
	FTestWorldWrapper PlayedLevel;
	FTestWorldWrapper ArrivalLevel;
	UWeaponDefinition* Rifle = LoadRifle();
	if (!TestNotNull(TEXT("Rifle definition"), Rifle) || !TestTrue(TEXT("Test levels made"),
		PlayedLevel.CreateTestWorld(EWorldType::EditorPreview) && ArrivalLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* Played = PlayedLevel.GetTestWorld();
	UWorld* Arrival = ArrivalLevel.GetTestWorld();
	ULostLootSubsystem* Leaving = ULostLootSubsystem::Get(Played);
	ULostLootSubsystem* Arriving = ULostLootSubsystem::Get(Arrival);
	if (!TestTrue(TEXT("Lost Property in both"), Leaving && Arriving))
	{
		return false;
	}
	AWeaponBase* Fresh = DropGun(Played, Rifle, 30, FVector(0.0, 0.0, 50.0));
	AWeaponBase* Older = DropGun(Played, Rifle, 20, FVector(300.0, 0.0, 50.0));
	AWeaponBase* Oldest = DropGun(Played, Rifle, 10, FVector(600.0, 0.0, 50.0));
	if (!TestTrue(TEXT("Three guns lying there"), Fresh && Older && Oldest))
	{
		return false;
	}
	Leaving->SetLifeLeft(*Older, 120.f);
	Leaving->SetLifeLeft(*Oldest, 15.f);

	TestEqual(TEXT("Leaving: all three handed in"), Leaving->SendAllGroundWeapons(ELostLootReason::LeftArea), 3);
	TestTrue(TEXT("...none left lying"), GroundSeeds(Played).IsEmpty());
	TestTrue(TEXT("...on the shelves the longest lain first"), ShelfSeeds(Leaving->GetStore()) == TArray<int32>({ 10, 20, 30 }));
	TestTrue(TEXT("...left behind on leaving"), Leaving->GetStore().Num() > 0 && Leaving->GetStore().Entries.Last().Reason == ELostLootReason::LeftArea);
	TestEqual(TEXT("...three to tell of on arrival"), Leaving->GetUnannounced(), 3);

	// The trip's save: the world left keeps no gun, the shelves keep all three.
	ULooterSessionSave* Save = NewSave();
	USessionSubsystem::CaptureWorld(Played, *Save);
	const FSavedMapWorld* Left = Save->FindWorld(USessionSubsystem::MapOf(Played));
	TestTrue(TEXT("The map's save keeps no gun on the ground"), Left && Left->LootWeapons.IsEmpty());
	ULooterSessionSave* OnArrival = WriteAndRead(Save);
	if (!TestNotNull(TEXT("Through the save file"), OnArrival) || !TestEqual(TEXT("...the shelves with it"), OnArrival->LostLoot.Num(), 3))
	{
		return false;
	}

	// Arrived: the same shelves, the same tickets, nothing on this ground, the count waiting for the HUD.
	USessionSubsystem::RestoreWorld(Arrival, *OnArrival);
	TestTrue(TEXT("Arrived: the same three"), ShelfSeeds(Arriving->GetStore()) == TArray<int32>({ 10, 20, 30 }));
	TestTrue(TEXT("...the same tickets"), Arriving->GetStore().Num() == 3 && Leaving->GetStore().Num() == 3
		&& Arriving->GetStore().Entries[0].Ticket == Leaving->GetStore().Entries[0].Ticket
		&& Arriving->GetStore().NextTicket == Leaving->GetStore().NextTicket);
	TestTrue(TEXT("...nothing lying here"), GroundSeeds(Arrival).IsEmpty());
	TestEqual(TEXT("...told of here"), Arriving->GetUnannounced(), 3);

	// Saved again from here: still three, never six.
	ULooterSessionSave* Again = NewSave();
	USessionSubsystem::CaptureWorld(Arrival, *Again);
	TestEqual(TEXT("Saved again: three on the shelves, not six"), Again->LostLoot.Num(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLostLootQuitAndLoadTest, "Looter.LostLoot.QuitAndLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLostLootQuitAndLoadTest::RunTest(const FString& Parameters)
{
	// Save & Quit (or the game closed) keeps the ground's guns with their time left; the load hands them in, the longest
	// lain first, and never puts one back on the ground. Loaded twice from the same save (no save between: a crash), they
	// go on the shelves once; saved again, they're on the shelves and nowhere else.
	FTestWorldWrapper PlayedLevel;
	FTestWorldWrapper LoadedLevel;
	FTestWorldWrapper CrashLevel;
	UWeaponDefinition* Rifle = LoadRifle();
	if (!TestNotNull(TEXT("Rifle definition"), Rifle) || !TestTrue(TEXT("Test levels made"), PlayedLevel.CreateTestWorld(EWorldType::EditorPreview)
		&& LoadedLevel.CreateTestWorld(EWorldType::EditorPreview) && CrashLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* Played = PlayedLevel.GetTestWorld();
	UWorld* Loaded = LoadedLevel.GetTestWorld();
	UWorld* Crashed = CrashLevel.GetTestWorld();
	ULostLootSubsystem* PlayedLoot = ULostLootSubsystem::Get(Played);
	ULostLootSubsystem* LoadedLoot = ULostLootSubsystem::Get(Loaded);
	ULostLootSubsystem* CrashedLoot = ULostLootSubsystem::Get(Crashed);
	if (!TestTrue(TEXT("Lost Property in every level"), PlayedLoot && LoadedLoot && CrashedLoot))
	{
		return false;
	}
	AWeaponBase* Whole = DropGun(Played, Rifle, 3, FVector(0.0, 0.0, 50.0));
	AWeaponBase* Middle = DropGun(Played, Rifle, 2, FVector(300.0, 0.0, 50.0));
	AWeaponBase* Old = DropGun(Played, Rifle, 1, FVector(600.0, 0.0, 50.0));
	if (!TestTrue(TEXT("Three guns lying there"), Whole && Middle && Old))
	{
		return false;
	}
	PlayedLoot->SetLifeLeft(*Middle, 120.f);
	PlayedLoot->SetLifeLeft(*Old, 30.f);

	ULooterSessionSave* Save = NewSave();
	USessionSubsystem::CaptureWorld(Played, *Save);
	ULooterSessionSave* Back = WriteAndRead(Save);
	const FSavedMapWorld* Kept = Back ? Back->FindWorld(USessionSubsystem::MapOf(Played)) : nullptr;
	if (!TestTrue(TEXT("Saved: three on the ground"), Kept && Kept->LootWeapons.Num() == 3))
	{
		return false;
	}
	TArray<float> Lives;
	for (const FSavedLootWeapon& Gun : Kept->LootWeapons)
	{
		Lives.Add(Gun.LifeLeft);
	}
	Lives.Sort();
	TestTrue(TEXT("...each with its time left"), Lives.Num() == 3 && FMath::IsNearlyEqual(Lives[0], 30.f) && FMath::IsNearlyEqual(Lives[1], 120.f)
		&& FMath::IsNearlyEqual(Lives[2], LostLootRules::GroundLifeSeconds));

	// Continue: the level starts again (with a gun of its own lying there, which the save's world replaces) and the session
	// hands in what the save kept lying.
	TestNotNull(TEXT("The level's own gun"), DropGun(Loaded, Rifle, 99, FVector(-500.0, 0.0, 50.0)));
	FileAs(*Back, Played, Loaded);
	USessionSubsystem::RestoreWorld(Loaded, *Back);
	TestTrue(TEXT("Loaded: nothing lying on the ground"), GroundSeeds(Loaded).IsEmpty());
	TestTrue(TEXT("...the three on the shelves, the longest lain first"), ShelfSeeds(LoadedLoot->GetStore()) == TArray<int32>({ 1, 2, 3 }));
	TestTrue(TEXT("...left at day's end"), LoadedLoot->GetStore().Num() == 3 && LoadedLoot->GetStore().Entries[0].Reason == ELostLootReason::LeftAtQuit);
	TestEqual(TEXT("...three to tell of"), LoadedLoot->GetUnannounced(), 3);

	// The same save loaded again (it crashed before saving): three, never six.
	FileAs(*Back, Played, Crashed);
	USessionSubsystem::RestoreWorld(Crashed, *Back);
	TestTrue(TEXT("Loaded again from the same save: the same three"), ShelfSeeds(CrashedLoot->GetStore()) == TArray<int32>({ 1, 2, 3 })
		&& GroundSeeds(Crashed).IsEmpty());

	// Saved from the loaded level: on the shelves, and the ground's list empty, so the next load hands in nothing more.
	ULooterSessionSave* Next = NewSave();
	USessionSubsystem::CaptureWorld(Loaded, *Next);
	const FSavedMapWorld* NextHere = Next->FindWorld(USessionSubsystem::MapOf(Loaded));
	TestTrue(TEXT("Saved again: three on the shelves, none on the ground"), Next->LostLoot.Num() == 3 && NextHere && NextHere->LootWeapons.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLostLootOldSavesTest, "Looter.LostLoot.OldSaves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLostLootOldSavesTest::RunTest(const FString& Parameters)
{
	// A save from before Lost Property kept every gun on the ground (one session had 112): its load hands them in in the
	// order the save kept them, so the shelves keep the newest thirty, the oldest going to make room, and the HUD says so.
	FTestWorldWrapper Level;
	UWeaponDefinition* Rifle = LoadRifle();
	if (!TestNotNull(TEXT("Rifle definition"), Rifle) || !TestTrue(TEXT("Test level made"), Level.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = Level.GetTestWorld();
	ULostLootSubsystem* LostLoot = ULostLootSubsystem::Get(World);
	if (!TestNotNull(TEXT("Lost Property in the level"), LostLoot))
	{
		return false;
	}
	ULooterSessionSave* Old = NewSave();
	FSavedMapWorld& Ground = Old->FindOrAddWorld(USessionSubsystem::MapOf(World));
	constexpr int32 Lying = 40;
	for (int32 Seed = 1; Seed <= Lying; ++Seed)
	{
		FSavedLootWeapon& Gun = Ground.LootWeapons.AddDefaulted_GetRef();
		Gun.Weapon = MakeGun(Rifle, Seed);
		Gun.Transform = FTransform(FVector(Seed * 100.0, 0.0, 50.0));
		// Before Lost Property no time left was kept.
		Gun.LifeLeft = 0.f;
	}
	ULooterSessionSave* Back = WriteAndRead(Old);
	if (!TestNotNull(TEXT("Through the save file"), Back))
	{
		return false;
	}
	TestEqual(TEXT("An old save's shelves read as empty"), Back->LostLoot.Num(), 0);
	USessionSubsystem::RestoreWorld(World, *Back);

	const FLostLootStore& Store = LostLoot->GetStore();
	TArray<int32> Newest;
	for (int32 Seed = Lying - LostLootRules::BaseCapacity + 1; Seed <= Lying; ++Seed)
	{
		Newest.Add(Seed);
	}
	TestTrue(TEXT("Thirty on the shelves: the newest the save kept"), ShelfSeeds(Store) == Newest);
	TestTrue(TEXT("...found lying, from the old save"), Store.Num() > 0 && Store.Entries[0].Reason == ELostLootReason::OldSave);
	TestEqual(TEXT("...the ten oldest gone to make room"), Store.GivenAway, Lying - LostLootRules::BaseCapacity);
	TestTrue(TEXT("...none put back on the ground"), GroundSeeds(World).IsEmpty());
	TestTrue(TEXT("...told of: forty sent, ten made room"), Store.Unannounced == Lying && Store.UnannouncedGone == Lying - LostLootRules::BaseCapacity);
	TestTrue(TEXT("...in the HUD's words"), LostLootRules::SentLine(Store.Unannounced, Store.UnannouncedGone).ToString().StartsWith(TEXT("40 guns sent")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLostLootClaimTest, "Looter.LostLoot.Claim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLostLootClaimTest::RunTest(const FString& Parameters)
{
	// At the window: a gun claimed into the backpack, else a free slot, else not at all (and kept on the shelf); shelving
	// bought through the wallet, all or nothing, five times at most.
	FTestWorldWrapper Level;
	UWeaponDefinition* Rifle = LoadRifle();
	if (!TestNotNull(TEXT("Rifle definition"), Rifle) || !TestTrue(TEXT("Test level made"), Level.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = Level.GetTestWorld();
	ULostLootSubsystem* LostLoot = ULostLootSubsystem::Get(World);
	UWalletSubsystem* Wallet = UWalletSubsystem::Get(World);
	APawn* Holder = World->SpawnActor<APawn>();
	UWeaponManagerComponent* Inventory = Holder ? NewObject<UWeaponManagerComponent>(Holder) : nullptr;
	if (!TestTrue(TEXT("Lost Property, a wallet and a holder"), LostLoot && Wallet && Inventory))
	{
		return false;
	}
	Inventory->RegisterComponent();
	Inventory->MaxWeapons = 2;
	Inventory->BackpackCapacity = 1;
	FLostLootStore& Store = LostLoot->GetMutableStore();
	for (int32 Seed = 1; Seed <= 4; ++Seed)
	{
		Store.Add(MakeGun(Rifle, Seed), ELostLootReason::LeftLying, USessionSubsystem::MapOf(World), 0.0);
	}

	using LostLootRules::ETake;
	FWeaponInstanceData Claimed;
	TestTrue(TEXT("The first into the backpack"), LostLootRules::Take(Store, 0, *Inventory, &Claimed) == ETake::InBackpack && Claimed.Seed == 1
		&& Inventory->GetBackpack().Num() == 1 && Store.Num() == 3);
	TestTrue(TEXT("Backpack full: the next into a free slot"), LostLootRules::Take(Store, 0, *Inventory) == ETake::InSlot
		&& Inventory->GetWeapons().Num() == 1 && Store.Num() == 2);
	TestTrue(TEXT("...and the next"), LostLootRules::Take(Store, 0, *Inventory) == ETake::InSlot && Inventory->GetWeapons().Num() == 2);
	TestTrue(TEXT("No room: refused, and it stays on the shelf"), LostLootRules::Take(Store, 0, *Inventory) == ETake::NoRoom
		&& Store.Num() == 1 && Store.Entries[0].Weapon.Seed == 4 && Inventory->GetBackpack().Num() == 1 && Inventory->GetWeapons().Num() == 2);
	TestTrue(TEXT("Nothing at a place past the end"), LostLootRules::Take(Store, 5, *Inventory) == ETake::Nothing);

	// Shelving: refused without the silver, then bought five times at their prices, then no more.
	Wallet->SetSilver(100);
	TestTrue(TEXT("Not enough silver: nothing bought, nothing paid"), LostLootRules::BuyUpgrade(*Wallet, Store, 1).Result == LostLootRules::EUpgrade::NotEnoughSilver
		&& Wallet->GetSilver() == 100 && Store.Upgrades == 0);
	Wallet->SetSilver(6000);
	const LostLootRules::FUpgradeResult First = LostLootRules::BuyUpgrade(*Wallet, Store, 1);
	TestTrue(TEXT("The first: 300 paid, room for 40"), First.Result == LostLootRules::EUpgrade::Done && First.Silver == 300 && First.Capacity == 40
		&& Wallet->GetSilver() == 5700);
	for (int32 More = 0; More < 4; ++More)
	{
		LostLootRules::BuyUpgrade(*Wallet, Store, 1);
	}
	TestTrue(TEXT("All five: room for 80, 5,900 paid in all"), Store.GetCapacity() == 80 && Wallet->GetSilver() == 6000 - 5900);
	TestTrue(TEXT("A sixth: none left, nothing paid"), LostLootRules::BuyUpgrade(*Wallet, Store, 1).Result == LostLootRules::EUpgrade::AtMost
		&& Wallet->GetSilver() == 100);

	// A named gun waits on the shelves as itself, and a turn-in can take that very gun back off them.
	UNamedWeaponDefinition* Heirloom = NewObject<UNamedWeaponDefinition>(GetTransientPackage());
	FWeaponInstanceData Pa = MakeGun(Rifle, 77);
	Pa.Named = Heirloom;
	Store.Add(Pa, ELostLootReason::LeftLying, FString(), 0.0);
	FWeaponInstanceData Back;
	TestTrue(TEXT("Heirloom on the shelves"), LostLoot->HoldsNamed(Heirloom));
	TestTrue(TEXT("...taken back off them as itself"), LostLoot->TakeNamed(Heirloom, Back) && Back.Seed == 77 && !LostLoot->HoldsNamed(Heirloom));
	return true;
}

#endif

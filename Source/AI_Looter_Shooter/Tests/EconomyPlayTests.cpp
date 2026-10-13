#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureRank.h"
#include "Economy/EconomyRules.h"
#include "Economy/SilverDrops.h"
#include "Economy/SilverPickup.h"
#include "Economy/Vendor.h"
#include "Economy/VendorOffer.h"
#include "Economy/VendorTrade.h"
#include "Economy/WalletSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootTable.h"
#include "Missions/MissionRewards.h"
#include "Player/PlayerThrowComponent.h"
#include "Player/PlayerThrowRules.h"
#include "Player/PlayerThrowSave.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponDefinition.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RandomStream.h"
#include "Misc/ScopeExit.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// The economy in test levels (its rules alone are EconomyTests.cpp): the wallet, silver on the ground coming to the player,
// the kills' silver, the vendors' trades (ammo, grave salt, the gun of the day, selling and the junk mark), a vendor shut
// by its story, and the session keeping the purse, the gun of the day and the silver lying around.

namespace
{
	UWeaponDefinition* LoadRifle()
	{
		return LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	}

	/** A holder as the game has one (health, weapons, the throw), begun as play begins it, with no gun. */
	struct FHolder
	{
		APawn* Pawn = nullptr;
		UWeaponManagerComponent* Weapons = nullptr;
		UPlayerThrowComponent* Throw = nullptr;
	};

	FHolder SpawnHolder(UWorld* World)
	{
		FHolder Holder;
		Holder.Pawn = World->SpawnActor<APawn>();
		if (!Holder.Pawn)
		{
			return Holder;
		}
		UHealthComponent* Health = NewObject<UHealthComponent>(Holder.Pawn, TEXT("Health"));
		Health->bShowDamageNumbers = false;
		Holder.Pawn->AddInstanceComponent(Health);
		Health->RegisterComponent();
		Holder.Weapons = NewObject<UWeaponManagerComponent>(Holder.Pawn, TEXT("Weapons"));
		Holder.Pawn->AddInstanceComponent(Holder.Weapons);
		Holder.Weapons->RegisterComponent();
		Holder.Weapons->MaxWeapons = 3;
		Holder.Throw = NewObject<UPlayerThrowComponent>(Holder.Pawn, TEXT("Throw"));
		Holder.Pawn->AddInstanceComponent(Holder.Throw);
		Holder.Throw->RegisterComponent();
		FEditorScriptExecutionGuard RunEvents;
		Holder.Pawn->DispatchBeginPlay();
		return Holder;
	}

	/** Through the save format and back, read as the game reads a session; null when it failed. */
	ULooterSessionSave* WriteAndRead(ULooterSessionSave* Save)
	{
		TArray<uint8> Bytes;
		return UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	}

	TArray<int32> SilverOnGround(UWorld* World)
	{
		TArray<int32> Amounts;
		for (TActorIterator<ASilverPickup> It(World); It; ++It)
		{
			if (!It->IsActorBeingDestroyed())
			{
				Amounts.Add(It->GetAmount());
			}
		}
		Amounts.Sort();
		return Amounts;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomyWalletTest, "Looter.Economy.Wallet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEconomyWalletTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWalletSubsystem* Wallet = UWalletSubsystem::Get(WorldWrapper.GetTestWorld());
	if (!TestNotNull(TEXT("A test level has a wallet"), Wallet))
	{
		return false;
	}
	int32 LastTotal = -1;
	int32 LastDelta = 0;
	ESilverChange LastWhy = ESilverChange::Restored;
	const FDelegateHandle Listening = Wallet->OnSilverChanged.AddLambda([&](int32 Total, int32 Delta, ESilverChange Why)
	{
		LastTotal = Total;
		LastDelta = Delta;
		LastWhy = Why;
	});
	ON_SCOPE_EXIT
	{
		Wallet->OnSilverChanged.Remove(Listening);
	};

	TestEqual(TEXT("A new game's purse is empty"), Wallet->GetSilver(), 0);
	TestEqual(TEXT("A find goes in"), Wallet->Add(12, ESilverChange::Found), 12);
	TestTrue(TEXT("...and is told as found"), LastTotal == 12 && LastDelta == 12 && LastWhy == ESilverChange::Found);
	TestFalse(TEXT("Can't pay more than is carried"), Wallet->Spend(13));
	TestEqual(TEXT("...nothing taken"), Wallet->GetSilver(), 12);
	TestTrue(TEXT("Paid"), Wallet->Spend(5) && Wallet->GetSilver() == 7);
	TestTrue(TEXT("...told as spent"), LastDelta == -5 && LastWhy == ESilverChange::Spent);
	TestTrue(TEXT("Nothing costs nothing"), Wallet->Spend(0) && Wallet->GetSilver() == 7);
	TestEqual(TEXT("A negative find adds nothing"), Wallet->Add(-4), 0);
	Wallet->SetSilver(LooterEconomy::MaxSilver - 3);
	TestTrue(TEXT("A load sets it, told as restored"), LastWhy == ESilverChange::Restored);
	TestEqual(TEXT("Held to the most carried"), Wallet->Add(10), 3);
	TestEqual(TEXT("...the most"), Wallet->GetSilver(), LooterEconomy::MaxSilver);
	TestEqual(TEXT("The world's silver, asked of anything in it"), UWalletSubsystem::SilverOf(WorldWrapper.GetTestWorld()), LooterEconomy::MaxSilver);
	TestEqual(TEXT("A test level's player level is 1"), UWalletSubsystem::PlayerLevelOf(WorldWrapper.GetTestWorld()), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomyPickupTest, "Looter.Economy.Pickup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEconomyPickupTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UWalletSubsystem* Wallet = UWalletSubsystem::Get(World);
	APawn* Player = World->SpawnActor<APawn>();
	ASilverPickup* Near = ASilverPickup::SpawnSilver(World, 12, FVector(380.0, 0.0, 0.0));
	ASilverPickup* Far = ASilverPickup::SpawnSilver(World, 30, FVector(900.0, 0.0, 0.0));
	if (!TestTrue(TEXT("A wallet, a player and two piles"), Wallet && Player && Near && Far))
	{
		return false;
	}
	TestNull(TEXT("No silver, no pile"), ASilverPickup::SpawnSilver(World, 0, FVector::ZeroVector));

	// Freshly dropped it waits a moment, so it's seen to fall, before it comes.
	TestFalse(TEXT("Not at once"), Near->Advance(0.1f, Player) || Near->IsPulled());
	// Nobody to come to: it lies there (it ages meanwhile).
	TestFalse(TEXT("No player, no pull"), Near->Advance(ASilverPickup::CollectDelay, nullptr) || Near->IsPulled());
	// Then a player near enough pulls it in, faster and faster, and it's theirs when it reaches them.
	const double StartDistance = Near->GetActorLocation().Size();
	Near->Advance(0.02f, Player);
	TestTrue(TEXT("Within reach it starts toward the player"), Near->IsPulled());
	TestTrue(TEXT("...and moves toward them"), Near->GetActorLocation().Size() < StartDistance);
	TestEqual(TEXT("...not theirs yet"), Wallet->GetSilver(), 0);
	bool bCollected = false;
	float Time = 0.f;
	while (!bCollected && Time < 2.f)
	{
		bCollected = Near->Advance(1.f / 60.f, Player);
		Time += 1.f / 60.f;
	}
	TestTrue(FString::Printf(TEXT("It reaches the player in under a second (%.2f s)"), Time), bCollected && Time < 1.f);
	TestEqual(TEXT("Its silver is in the wallet"), Wallet->GetSilver(), 12);
	TestTrue(TEXT("...and it's gone"), !IsValid(Near) || Near->IsActorBeingDestroyed());

	// Out of reach it never comes, however long the player waits.
	for (int32 Step = 0; Step < 120; ++Step)
	{
		Far->Advance(1.f / 30.f, Player);
	}
	TestFalse(TEXT("A pile far off stays where it is"), Far->IsPulled());
	TestEqual(TEXT("...and nothing more in the wallet"), Wallet->GetSilver(), 12);

	// A kill's silver: a boss always drops some, popped out of the body; a Basic beast only now and then.
	AActor* Body = World->SpawnActor<AActor>();
	FRandomStream Random(5);
	ASilverPickup* BossSilver = Body ? SilverDrops::DropForKill(*Body, ECreatureRank::Boss, true, 1, Random) : nullptr;
	if (TestNotNull(TEXT("A boss's silver"), BossSilver))
	{
		TestTrue(FString::Printf(TEXT("...100 to 140 at level 1 (%d)"), BossSilver->GetAmount()), BossSilver->GetAmount() >= 100 && BossSilver->GetAmount() <= 140);
	}
	int32 Drops = 0;
	constexpr int32 Kills = 300;
	for (int32 Kill = 0; Kill < Kills && Body; ++Kill)
	{
		if (ASilverPickup* Pile = SilverDrops::DropForKill(*Body, ECreatureRank::Basic, false, 1, Random))
		{
			++Drops;
			Pile->Destroy();
		}
	}
	TestTrue(FString::Printf(TEXT("A Basic beast drops silver about one kill in five (%d of %d)"), Drops, Kills), Drops > Kills / 10 && Drops < Kills * 3 / 10);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomyBuyTest, "Looter.Economy.Trade.Buy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEconomyBuyTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UWalletSubsystem* Wallet = UWalletSubsystem::Get(World);
	const FHolder Holder = SpawnHolder(World);
	if (!TestTrue(TEXT("A wallet and a holder"), Wallet && Holder.Weapons && Holder.Throw))
	{
		return false;
	}
	UWeaponManagerComponent& Weapons = *Holder.Weapons;
	const EAmmoType Rifle = EAmmoType::AssaultRifle;

	// The stock: what the game's guns use.
	const TArray<EAmmoType> Stocked = VendorTrade::StockedAmmo();
	TestTrue(TEXT("Rifle rounds and shells are stocked"), Stocked.Contains(EAmmoType::AssaultRifle) && Stocked.Contains(EAmmoType::Shotgun));

	// No silver: nothing bought, nothing changed.
	const int32 RoundsBefore = Weapons.GetAmmo(Rifle);
	FVendorTradeResult Result = VendorTrade::BuyAmmo(*Wallet, Weapons, Rifle, 1);
	TestTrue(TEXT("Without the price: not enough silver"), Result.Result == EVendorTrade::NotEnoughSilver && Result.Silver == 15);
	TestEqual(TEXT("...no rounds"), Weapons.GetAmmo(Rifle), RoundsBefore);

	// Paid: a box goes in.
	Wallet->Add(100);
	Result = VendorTrade::BuyAmmo(*Wallet, Weapons, Rifle, 1);
	TestTrue(TEXT("A box bought"), Result.IsDone() && Result.Count == LooterEconomy::AmmoBoxRounds(Rifle) && Result.Silver == 15);
	TestEqual(TEXT("...its rounds carried"), Weapons.GetAmmo(Rifle), RoundsBefore + LooterEconomy::AmmoBoxRounds(Rifle));
	TestEqual(TEXT("...its price paid"), Wallet->GetSilver(), 85);

	// Nearly full: what fits, for its share. Full: none.
	Weapons.AddAmmo(Rifle, Weapons.GetMaxAmmo(Rifle));
	Weapons.TakeAmmo(Rifle, 12);
	const FVendorAmmoOffer Part = VendorTrade::AmmoOffer(Weapons, Rifle, 1);
	TestTrue(FString::Printf(TEXT("Twelve short: twelve rounds offered for 5 (%d for %d)"), Part.Rounds, Part.Price), Part.Rounds == 12 && Part.Price == 5);
	Result = VendorTrade::BuyAmmo(*Wallet, Weapons, Rifle, 1);
	TestTrue(TEXT("...bought"), Result.IsDone() && Result.Count == 12 && Wallet->GetSilver() == 80);
	Result = VendorTrade::BuyAmmo(*Wallet, Weapons, Rifle, 1);
	TestTrue(TEXT("Full: nothing to buy"), Result.Result == EVendorTrade::Full);
	TestEqual(TEXT("...nothing paid"), Wallet->GetSilver(), 80);

	// Grave salt: a jar at a time, up to the three carried.
	FThrowablesSave Two;
	Two.Grenades = 2;
	Two.bUnlocked = true;
	Holder.Throw->RestoreThrowables(Two);
	Result = VendorTrade::BuyGrenade(*Wallet, *Holder.Throw, 1);
	TestTrue(TEXT("A jar bought"), Result.IsDone() && Holder.Throw->GetGrenades() == FThrowRules::MaxGrenades && Wallet->GetSilver() == 55);
	Result = VendorTrade::BuyGrenade(*Wallet, *Holder.Throw, 1);
	TestTrue(TEXT("At three: full, nothing paid"), Result.Result == EVendorTrade::Full && Wallet->GetSilver() == 55);
	FThrowablesSave None;
	None.bUnlocked = false;
	Holder.Throw->RestoreThrowables(None);
	Wallet->SetSilver(10);
	Result = VendorTrade::BuyGrenade(*Wallet, *Holder.Throw, 1);
	TestTrue(TEXT("Short of the price: not enough"), Result.Result == EVendorTrade::NotEnoughSilver && Holder.Throw->GetGrenades() == 0);
	Wallet->SetSilver(25);
	TestTrue(TEXT("Bought with none carried: the counter comes on"), VendorTrade::BuyGrenade(*Wallet, *Holder.Throw, 1).IsDone()
		&& Holder.Throw->IsUnlocked() && Holder.Throw->GetGrenades() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomySellTest, "Looter.Economy.Trade.Sell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEconomySellTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	UWeaponDefinition* Rifle = LoadRifle();
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)) || !TestNotNull(TEXT("Rifle loads"), Rifle))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UWalletSubsystem* Wallet = UWalletSubsystem::Get(World);
	const FHolder Holder = SpawnHolder(World);
	if (!TestTrue(TEXT("A wallet and a holder"), Wallet && Holder.Weapons))
	{
		return false;
	}
	UWeaponManagerComponent& Weapons = *Holder.Weapons;

	// One in hand (never for sale), and in the backpack a Common, a Rare, a named gun and another Common.
	Weapons.GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Common, 1));
	Weapons.AddToBackpack(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Common, 1));
	Weapons.AddToBackpack(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Rare, 1));
	FWeaponInstanceData Heirloom = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Epic, 1);
	Heirloom.Named = NewObject<UNamedWeaponDefinition>();
	Weapons.AddToBackpack(Heirloom);
	Weapons.AddToBackpack(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Common, 1));
	if (!TestEqual(TEXT("Four in the backpack"), Weapons.GetBackpack().Num(), 4))
	{
		return false;
	}
	const int32 Equipped = Weapons.GetWeapons().Num();

	// A Common sold: its price in, the gun gone, the others moved up.
	FVendorTradeResult Result = VendorTrade::SellGun(*Wallet, Weapons, 0);
	TestTrue(TEXT("A Common sold for 8 at level 1"), Result.IsDone() && Result.Silver == 8 && Wallet->GetSilver() == 8);
	TestTrue(TEXT("...gone from the backpack"), Weapons.GetBackpack().Num() == 3 && Weapons.GetBackpack()[0].Rarity == EWeaponRarity::Rare);
	TestEqual(TEXT("...the gun in hand untouched"), Weapons.GetWeapons().Num(), Equipped);
	// The named gun stays.
	Result = VendorTrade::SellGun(*Wallet, Weapons, 1);
	TestTrue(TEXT("A named gun isn't for sale"), Result.Result == EVendorTrade::NotForSale && Weapons.GetBackpack().Num() == 3 && Wallet->GetSilver() == 8);
	TestTrue(TEXT("Nothing there, nothing sold"), VendorTrade::SellGun(*Wallet, Weapons, 7).Result == EVendorTrade::Nothing);

	// The junk mark: on the Rare and the Common; never on the named gun. Selling all junk sells just those.
	TestTrue(TEXT("The Rare marked junk"), VendorTrade::ToggleJunk(Weapons, 0) && Weapons.GetBackpack()[0].bJunk);
	TestFalse(TEXT("The named gun can't be junk"), VendorTrade::ToggleJunk(Weapons, 1));
	TestTrue(TEXT("The Common marked junk"), VendorTrade::ToggleJunk(Weapons, 2) && Weapons.GetBackpack()[2].bJunk);
	TestTrue(TEXT("...unmarked again, and marked"), VendorTrade::ToggleJunk(Weapons, 2) && !Weapons.GetBackpack()[2].bJunk
		&& VendorTrade::ToggleJunk(Weapons, 2));
	const FVendorJunkTally Tally = VendorTrade::TallyJunk(Weapons);
	TestTrue(FString::Printf(TEXT("Two junk guns for 48, one Rare among them (%d for %d)"), Tally.Count, Tally.Silver),
		Tally.Count == 2 && Tally.Silver == 40 + 8 && Tally.bAnyRare);
	Result = VendorTrade::SellAllJunk(*Wallet, Weapons);
	TestTrue(TEXT("All junk sold"), Result.IsDone() && Result.Count == 2 && Result.Silver == 48 && Wallet->GetSilver() == 56);
	TestTrue(TEXT("...the named gun left"), Weapons.GetBackpack().Num() == 1 && Weapons.GetBackpack()[0].Named != nullptr);
	TestTrue(TEXT("No junk left: nothing to sell"), VendorTrade::SellAllJunk(*Wallet, Weapons).Result == EVendorTrade::Nothing);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomyOfferTest, "Looter.Economy.Offer.Vendor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEconomyOfferTest::RunTest(const FString& Parameters)
{
	// The day's clock alone: a level gained or an hour of play brings a new gun; a clock that went back (a new level's,
	// played without a session) counts as no time gone.
	FVendorOffer Day;
	TestTrue(TEXT("None rolled: due"), Day.IsDue(1, 0.0));
	Day.bRolled = true;
	Day.Gun.Definition = LoadRifle();
	Day.RolledAtLevel = 3;
	Day.RolledAt = 100.0;
	TestFalse(TEXT("The same level, a minute on: not due"), Day.IsDue(3, 160.0));
	TestTrue(TEXT("A level gained: due"), Day.IsDue(4, 160.0));
	TestTrue(TEXT("A day's trade gone by: due"), Day.IsDue(3, 100.0 + LooterEconomy::OfferDaySeconds));
	TestFalse(TEXT("A clock that went back: not due"), Day.IsDue(3, 5.0));
	TestTrue(TEXT("Seconds to the next one"), FMath::IsNearlyEqual(Day.SecondsToNext(160.0), LooterEconomy::OfferDaySeconds - 60.0));

	const ULootTable* Table = ULootLibrary::GetDefaultLootTable();
	if (!Table || Table->Entries.IsEmpty())
	{
		AddWarning(TEXT("The default loot table (DA_LootTable_Default) lists no guns in this checkout: the gun of the day isn't rolled."));
		return true;
	}
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UWalletSubsystem* Wallet = UWalletSubsystem::Get(World);
	const FHolder Holder = SpawnHolder(World);
	if (!TestTrue(TEXT("A wallet and a holder"), Wallet && Holder.Weapons))
	{
		return false;
	}
	FRandomStream Random(31);
	const FVendorOffer& Offer = Wallet->RefreshOffer(3, &Random);
	if (!TestTrue(TEXT("A gun of the day"), Offer.bRolled && Offer.Gun.Definition != nullptr))
	{
		return false;
	}
	TestTrue(TEXT("...Uncommon or better"), static_cast<uint8>(Offer.Gun.Rarity) >= static_cast<uint8>(EWeaponRarity::Uncommon));
	TestEqual(TEXT("...at the player's level"), Offer.Gun.Level, 3);
	TestTrue(TEXT("...never cursed, never named"), Offer.Gun.Curse.IsNone() && Offer.Gun.Named == nullptr);
	const int32 Seed = Offer.Gun.Seed;
	TestEqual(TEXT("Opened again at the same level: the same gun"), Wallet->RefreshOffer(3, &Random).Gun.Seed, Seed);
	TestEqual(TEXT("A level gained: a new one, for that level"), Wallet->RefreshOffer(4, &Random).RolledAtLevel, 4);

	// Bought: too dear first, then paid, into the backpack; then sold out.
	const int32 Price = LooterEconomy::GunBuyPrice(Wallet->GetOffer().Gun);
	FVendorTradeResult Result = VendorTrade::BuyOffer(*Wallet, *Holder.Weapons);
	TestTrue(TEXT("Without the price: not enough silver"), Result.Result == EVendorTrade::NotEnoughSilver && Result.Silver == Price);
	Wallet->Add(Price + 5);
	Result = VendorTrade::BuyOffer(*Wallet, *Holder.Weapons);
	TestTrue(TEXT("Bought"), Result.IsDone() && Wallet->GetSilver() == 5);
	TestTrue(TEXT("...into the backpack"), Holder.Weapons->GetBackpack().Num() == 1
		&& Holder.Weapons->GetBackpack()[0].Seed == Wallet->GetOffer().Gun.Seed);
	TestTrue(TEXT("...sold out"), Wallet->GetOffer().bSold && VendorTrade::BuyOffer(*Wallet, *Holder.Weapons).Result == EVendorTrade::SoldOut);

	// Nowhere to put it: no sale, nothing paid.
	Wallet->RefreshOffer(5, &Random);
	Holder.Weapons->BackpackCapacity = Holder.Weapons->GetBackpack().Num();
	Holder.Weapons->MaxWeapons = Holder.Weapons->GetWeapons().Num();
	Wallet->Add(LooterEconomy::GunBuyPrice(Wallet->GetOffer().Gun));
	const int32 Before = Wallet->GetSilver();
	TestTrue(TEXT("No room: no sale"), VendorTrade::BuyOffer(*Wallet, *Holder.Weapons).Result == EVendorTrade::NoRoom && Wallet->GetSilver() == Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomyVendorTest, "Looter.Economy.Vendor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEconomyVendorTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	AVendor* Store = World->SpawnActor<AVendor>(FVector(200.0, 0.0, 150.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("A vendor"), Store))
	{
		return false;
	}
	TestTrue(TEXT("Tagged a vendor"), Store->ActorHasTag(AVendor::VendorTag));
	TestTrue(TEXT("With no story to wait on, it trades"), Store->IsOpen());
	TestTrue(TEXT("...used where it stands"), Store->GetInteractionLocation().IsSet()
		&& Store->GetInteractionLocation().GetValue().Equals(FVector(200.0, 0.0, 150.0)));
	// Pruitt's waits on Main 3, which nobody has finished here: it's shut.
	Store->OpenWhen.AfterMissions = { TEXT("Main3") };
	TestFalse(TEXT("Waiting on its story, it's shut"), Store->IsOpen());
	TestFalse(TEXT("...and won't open its screen"), Store->Use(nullptr));

	// A mission's purse goes into the level's wallet as it's turned in.
	UWalletSubsystem* Wallet = UWalletSubsystem::Get(World);
	if (TestNotNull(TEXT("A wallet"), Wallet))
	{
		TestEqual(TEXT("Cold Welcome pays 40"), MissionRewards::GiveSilver(World, TEXT("Main3")), 40);
		TestEqual(TEXT("Sexton's pays none"), MissionRewards::GiveSilver(World, TEXT("Main2")), 0);
		TestEqual(TEXT("...the purse holds 40"), Wallet->GetSilver(), 40);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomySaveTest, "Looter.Economy.Save",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEconomySaveTest::RunTest(const FString& Parameters)
{
	// A save from before money reads as none carried, none rolled and none lying around.
	const ULooterSessionSave* Old = NewObject<ULooterSessionSave>();
	TestTrue(TEXT("Before money: none"), Old->Silver == 0 && !Old->VendorOffer.bRolled && FSavedMapWorld().SilverPickups.IsEmpty());

	FTestWorldWrapper WorldWrapper;
	UWeaponDefinition* Rifle = LoadRifle();
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)) || !TestNotNull(TEXT("Rifle loads"), Rifle))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UWalletSubsystem* Wallet = UWalletSubsystem::Get(World);
	if (!TestNotNull(TEXT("A wallet"), Wallet))
	{
		return false;
	}
	// Played: 250 silver, a sold gun of the day, and 9 silver lying where a crate broke.
	Wallet->SetSilver(250);
	FVendorOffer Offer;
	Offer.bRolled = true;
	Offer.Gun = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Rare, 5);
	Offer.bSold = true;
	Offer.RolledAtLevel = 5;
	Offer.RolledAt = 1234.0;
	Wallet->SetOffer(Offer);
	TestNotNull(TEXT("Silver on the ground"), ASilverPickup::SpawnSilver(World, 9, FVector(100.0, 50.0, 20.0)));

	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	USessionSubsystem::CaptureWorld(World, *Save);
	const ULooterSessionSave* Loaded = WriteAndRead(Save);
	if (!TestNotNull(TEXT("Written and read"), Loaded))
	{
		return false;
	}
	TestEqual(TEXT("The purse kept"), Loaded->Silver, 250);
	TestTrue(TEXT("The gun of the day kept: its kind, rarity, level, seed, sold"), Loaded->VendorOffer.bRolled && Loaded->VendorOffer.bSold
		&& Loaded->VendorOffer.Gun.Definition == Rifle && Loaded->VendorOffer.Gun.Rarity == EWeaponRarity::Rare
		&& Loaded->VendorOffer.Gun.Level == 5 && Loaded->VendorOffer.Gun.Seed == Offer.Gun.Seed
		&& Loaded->VendorOffer.RolledAtLevel == 5 && FMath::IsNearlyEqual(Loaded->VendorOffer.RolledAt, 1234.0));
	const FSavedMapWorld* Here = Loaded->FindWorld(USessionSubsystem::MapOf(World));
	TestTrue(TEXT("The silver lying around kept"), Here && Here->SilverPickups.Num() == 1 && Here->SilverPickups[0].Amount == 9);

	// Back in the level, changed since: the session puts the purse, the day's gun and the silver on the ground back as kept.
	Wallet->SetSilver(3);
	Wallet->SetOffer(FVendorOffer());
	for (TActorIterator<ASilverPickup> It(World); It; ++It)
	{
		It->Destroy();
	}
	ASilverPickup::SpawnSilver(World, 4, FVector(-300.0, 0.0, 20.0));
	USessionSubsystem::RestoreWorld(World, *Loaded);
	TestEqual(TEXT("The purse is back"), Wallet->GetSilver(), 250);
	TestTrue(TEXT("The gun of the day is back, still sold"), Wallet->GetOffer().bRolled && Wallet->GetOffer().bSold
		&& Wallet->GetOffer().Gun.Seed == Offer.Gun.Seed);
	TestTrue(TEXT("The silver on the ground is the save's, and only that"), SilverOnGround(World) == TArray<int32>{ 9 });

	// A junk mark rides with a gun through the save.
	FWeaponInstanceData Junk = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Common, 2);
	Junk.bJunk = true;
	ULooterSessionSave* WithJunk = NewObject<ULooterSessionSave>();
	WithJunk->Version = ULooterSessionSave::CurrentVersion;
	WithJunk->bHasInventory = true;
	WithJunk->Inventory.Backpack = { Junk };
	const ULooterSessionSave* JunkRead = WriteAndRead(WithJunk);
	TestTrue(TEXT("A junk mark kept with the gun"), JunkRead && JunkRead->Inventory.Backpack.Num() == 1 && JunkRead->Inventory.Backpack[0].bJunk);
	return true;
}

#endif

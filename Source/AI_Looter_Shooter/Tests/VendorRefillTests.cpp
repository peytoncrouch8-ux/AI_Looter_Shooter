#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Economy/EconomyRules.h"
#include "Economy/VendorTrade.h"
#include "Economy/WalletSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/AmmoTypes.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/ScopeExit.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// "Refill all ammo" at the general store (VendorTrade::TallyRefill / RefillAllAmmo): the price is exactly what buying box by
// box would cost, it's paid once, and it fills every stocked type. Its rules need no world; the trade runs in a test level.

namespace
{
	/** What clicking Buy on the ammo row until it's full would cost for Rounds short: the existing box-by-box rule, walked. */
	int32 BoxByBox(EAmmoType Type, int32 Rounds, int32 Level)
	{
		int32 Silver = 0;
		while (Rounds > 0)
		{
			const int32 Bought = FMath::Min(Rounds, LooterEconomy::AmmoBoxRounds(Type));
			Silver += LooterEconomy::AmmoPrice(Type, Bought, Level);
			Rounds -= Bought;
		}
		return Silver;
	}

	/** A pawn with the weapon manager the vendor reads, begun as play begins it. */
	UWeaponManagerComponent* SpawnManager(UWorld* World)
	{
		APawn* Pawn = World->SpawnActor<APawn>();
		if (!Pawn)
		{
			return nullptr;
		}
		UHealthComponent* Health = NewObject<UHealthComponent>(Pawn, TEXT("Health"));
		Health->bShowDamageNumbers = false;
		Pawn->AddInstanceComponent(Health);
		Health->RegisterComponent();
		UWeaponManagerComponent* Weapons = NewObject<UWeaponManagerComponent>(Pawn, TEXT("Weapons"));
		Pawn->AddInstanceComponent(Weapons);
		Weapons->RegisterComponent();
		Weapons->MaxWeapons = 3;
		FEditorScriptExecutionGuard RunEvents;
		Pawn->DispatchBeginPlay();
		return Weapons;
	}

	/** Every stocked type emptied. */
	void EmptyAmmo(UWeaponManagerComponent& Weapons)
	{
		for (const EAmmoType Type : VendorTrade::StockedAmmo())
		{
			Weapons.TakeAmmo(Type, Weapons.GetAmmo(Type));
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVendorRefillPriceTest, "Looter.Economy.Trade.RefillPrice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FVendorRefillPriceTest::RunTest(const FString& Parameters)
{
	const EAmmoType Rifle = EAmmoType::AssaultRifle;
	const int32 Box = LooterEconomy::AmmoBoxRounds(Rifle);
	TestEqual(TEXT("Nothing short costs nothing"), VendorTrade::RefillPrice(Rifle, 0, 1), 0);
	TestEqual(TEXT("One whole box is the box price"), VendorTrade::RefillPrice(Rifle, Box, 1), LooterEconomy::AmmoBoxPrice(Rifle, 1));
	TestEqual(TEXT("Two boxes are two prices"), VendorTrade::RefillPrice(Rifle, Box * 2, 1), 2 * LooterEconomy::AmmoBoxPrice(Rifle, 1));
	TestEqual(TEXT("Two boxes and a part: the part at its share (12 of 36 is 5 at level 1)"),
		VendorTrade::RefillPrice(Rifle, Box * 2 + 12, 1), 2 * LooterEconomy::AmmoBoxPrice(Rifle, 1) + 5);
	TestEqual(TEXT("A few rounds never come free"), VendorTrade::RefillPrice(Rifle, 1, 1), 1);

	// For every stocked type, every shortfall and a few levels: exactly the box-by-box sum, never cheaper, never dearer.
	int32 Checked = 0;
	for (const EAmmoType Type : VendorTrade::StockedAmmo())
	{
		const int32 Most = LooterAmmo::GetInfo(Type).MaxCarried;
		for (const int32 Level : { 1, 6, 30 })
		{
			for (int32 Short = 0; Short <= Most; ++Short)
			{
				const int32 Want = BoxByBox(Type, Short, Level);
				if (VendorTrade::RefillPrice(Type, Short, Level) != Want)
				{
					AddError(FString::Printf(TEXT("%s, %d short at level %d: %d, box by box %d"), LooterAmmo::GetInfo(Type).Name, Short, Level,
						VendorTrade::RefillPrice(Type, Short, Level), Want));
					return false;
				}
				++Checked;
			}
		}
	}
	TestTrue(TEXT("Some prices were checked"), Checked > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVendorRefillAllTest, "Looter.Economy.Trade.RefillAll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FVendorRefillAllTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UWalletSubsystem* Wallet = UWalletSubsystem::Get(World);
	UWeaponManagerComponent* Weapons = SpawnManager(World);
	if (!TestTrue(TEXT("A wallet and a holder"), Wallet && Weapons))
	{
		return false;
	}
	const TArray<EAmmoType> Stocked = VendorTrade::StockedAmmo();
	constexpr int32 Level = 1;

	// Everything carried is full: nothing to buy, nothing paid.
	for (const EAmmoType Type : Stocked)
	{
		Weapons->AddAmmo(Type, Weapons->GetMaxAmmo(Type));
	}
	Wallet->SetSilver(500);
	TestTrue(TEXT("Full: the tally is empty"), VendorTrade::TallyRefill(*Weapons, Level).IsFull());
	TestTrue(TEXT("Full: refill says so and takes nothing"), VendorTrade::RefillAllAmmo(*Wallet, *Weapons, Level).Result == EVendorTrade::Full
		&& Wallet->GetSilver() == 500);

	// One type twelve short: the part box's share, 5 silver for the rifle's twelve.
	Weapons->TakeAmmo(EAmmoType::AssaultRifle, 12);
	const FVendorRefillTally Small = VendorTrade::TallyRefill(*Weapons, Level);
	TestTrue(FString::Printf(TEXT("Twelve rifle rounds short: 1 kind, 12 rounds, 5 silver (%d, %d, %d)"), Small.Types, Small.Rounds, Small.Silver),
		Small.Types == 1 && Small.Rounds == 12 && Small.Silver == 5);

	// Everything empty: the tally is what the box-by-box clicks add up to.
	EmptyAmmo(*Weapons);
	const FVendorRefillTally All = VendorTrade::TallyRefill(*Weapons, Level);
	int32 WantSilver = 0;
	int32 WantRounds = 0;
	for (const EAmmoType Type : Stocked)
	{
		WantRounds += Weapons->GetMaxAmmo(Type);
		WantSilver += BoxByBox(Type, Weapons->GetMaxAmmo(Type), Level);
	}
	TestTrue(FString::Printf(TEXT("Empty: every kind, every round, the box-by-box sum (%d kinds, %d rounds, %d silver; want %d, %d, %d)"),
		All.Types, All.Rounds, All.Silver, Stocked.Num(), WantRounds, WantSilver),
		All.Types == Stocked.Num() && All.Rounds == WantRounds && All.Silver == WantSilver);

	// ...and truly what clicking Buy on every row until full costs (a second pass of the real trades, rich enough).
	{
		Wallet->SetSilver(LooterEconomy::MaxSilver);
		int32 Paid = 0;
		for (const EAmmoType Type : Stocked)
		{
			for (int32 Click = 0; Click < 200; ++Click)
			{
				const FVendorTradeResult One = VendorTrade::BuyAmmo(*Wallet, *Weapons, Type, Level);
				if (!One.IsDone())
				{
					break;
				}
				Paid += One.Silver;
			}
		}
		TestEqual(TEXT("Buying every box one click at a time costs the tally"), Paid, All.Silver);
		TestEqual(TEXT("...and the clicks' purse change is the same"), LooterEconomy::MaxSilver - Wallet->GetSilver(), All.Silver);
		EmptyAmmo(*Weapons);
	}

	// Too poor: not enough silver, the price said, nothing bought, nothing paid.
	Wallet->SetSilver(All.Silver - 1);
	FVendorTradeResult Result = VendorTrade::RefillAllAmmo(*Wallet, *Weapons, Level);
	TestTrue(TEXT("One silver short: not enough, with the total"), Result.Result == EVendorTrade::NotEnoughSilver && Result.Silver == All.Silver);
	TestEqual(TEXT("...no silver taken"), Wallet->GetSilver(), All.Silver - 1);
	bool bAnyRounds = false;
	for (const EAmmoType Type : Stocked)
	{
		bAnyRounds |= Weapons->GetAmmo(Type) > 0;
	}
	TestFalse(TEXT("...no rounds given"), bAnyRounds);

	// Paid: one spend, everything full, the change left.
	int32 Spends = 0;
	int32 SpentTotal = 0;
	const FDelegateHandle Listening = Wallet->OnSilverChanged.AddLambda([&](int32, int32 Delta, ESilverChange Why)
	{
		if (Why == ESilverChange::Spent)
		{
			++Spends;
			SpentTotal += -Delta;
		}
	});
	ON_SCOPE_EXIT
	{
		Wallet->OnSilverChanged.Remove(Listening);
	};
	Wallet->SetSilver(All.Silver + 7);
	Result = VendorTrade::RefillAllAmmo(*Wallet, *Weapons, Level);
	TestTrue(TEXT("Refilled"), Result.IsDone() && Result.Silver == All.Silver && Result.Count == All.Rounds);
	TestTrue(FString::Printf(TEXT("...paid once, the whole price (%d spends, %d)"), Spends, SpentTotal), Spends == 1 && SpentTotal == All.Silver);
	TestEqual(TEXT("...the change left"), Wallet->GetSilver(), 7);
	bool bAllFull = true;
	for (const EAmmoType Type : Stocked)
	{
		bAllFull &= Weapons->GetAmmo(Type) == Weapons->GetMaxAmmo(Type);
	}
	TestTrue(TEXT("...every stocked kind at its most"), bAllFull);

	// Again: full, nothing taken.
	Result = VendorTrade::RefillAllAmmo(*Wallet, *Weapons, Level);
	TestTrue(TEXT("Full again: nothing to buy, nothing paid"), Result.Result == EVendorTrade::Full && Wallet->GetSilver() == 7 && Spends == 1);

	// The price grows with the player's level as one box does.
	EmptyAmmo(*Weapons);
	TestTrue(TEXT("At a higher level it costs more"), VendorTrade::TallyRefill(*Weapons, 10).Silver > All.Silver);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

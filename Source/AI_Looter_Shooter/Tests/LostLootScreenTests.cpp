#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Economy/WalletSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/LostLootRules.h"
#include "Loot/LostLootStore.h"
#include "Loot/LostLootSubsystem.h"
#include "Session/SessionSubsystem.h"
#include "UI/Style/LooterUIFocus.h"
#include "UI/World/LostLootWidget.h"
#include "Weapons/WeaponDefinition.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "Misc/ScopeExit.h"
#include "Tests/AutomationCommon.h"

// Lost Property's screen over a real weapon manager and the level's shelves, driven by its keys: the tickets newest first,
// NEXT TO GO, choosing, claiming with the stamp (into the backpack, then "no room"), shelving bought through the confirm,
// and the prompts. (The rules and the store are LostLootTests.cpp; the ground and the saves LostLootPlayTests.cpp.)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLostLootScreenTest, "Looter.LostLoot.Screen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLostLootScreenTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	if (!TestNotNull(TEXT("Rifle loads"), Rifle) || !TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ULostLootSubsystem* LostLoot = ULostLootSubsystem::Get(World);
	UWalletSubsystem* Wallet = UWalletSubsystem::Get(World);
	APawn* Holder = World->SpawnActor<APawn>();
	UWeaponManagerComponent* Inventory = Holder ? NewObject<UWeaponManagerComponent>(Holder) : nullptr;
	if (!TestTrue(TEXT("Lost Property, a wallet and a holder"), LostLoot && Wallet && Inventory))
	{
		return false;
	}
	// The prompts read the keyboard's keys; whatever the menus were driven by before comes back after.
	const LooterUI::EMenuDevice WasDevice = LooterUI::GetMenuDevice();
	LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Keyboard);
	ON_SCOPE_EXIT
	{
		LooterUI::SetMenuDevice(WasDevice);
	};

	// One slot, already holding a gun; room for two in the backpack; 1,000 silver; three guns on the shelves.
	Inventory->RegisterComponent();
	Inventory->MaxWeapons = 1;
	Inventory->BackpackCapacity = 2;
	Inventory->GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Common, 3));
	Wallet->SetSilver(1000);
	TArray<FLostLootArrival> Lost;
	for (int32 Life = 10; Life <= 30; Life += 10)
	{
		FLostLootArrival& Arrival = Lost.AddDefaulted_GetRef();
		Arrival.Weapon = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Uncommon, 3);
		Arrival.LifeLeft = static_cast<float>(Life);
	}
	LostLoot->Receive(MoveTemp(Lost), ELostLootReason::LeftLying, USessionSubsystem::MapOf(World), /*bAnnounce*/ false);

	ULostLootWidget* Screen = CreateWidget<ULostLootWidget>(World, ULostLootWidget::StaticClass());
	if (!TestNotNull(TEXT("The screen"), Screen))
	{
		return false;
	}
	Screen->Open(nullptr, Inventory);
	Screen->TakeWidget();
	ULostLootWidget::FView View = Screen->GetView();
	TestTrue(TEXT("Newest first"), View.Tickets == TArray<int32>({ 3, 2, 1 }));
	TestEqual(TEXT("...the newest chosen"), View.Chosen, 3);
	TestEqual(TEXT("The count"), View.Count, FString(TEXT("3 / 30")));
	TestTrue(TEXT("The oldest is next to go, the shelves not full"), View.NextToGo == 1 && !View.bFull);
	TestTrue(TEXT("E claims"), !View.Prompts.IsEmpty() && View.Prompts[0] == TEXT("E: Claim"));
	TestEqual(TEXT("The first shelving's price"), View.UpgradePrice, 300);

	// W / S choose, stopping at the ends.
	Screen->HandleKey(EKeys::S);
	TestEqual(TEXT("S: the next ticket"), Screen->GetView().Chosen, 2);
	Screen->HandleKey(EKeys::W);
	Screen->HandleKey(EKeys::W);
	TestEqual(TEXT("W: back up, and no further"), Screen->GetView().Chosen, 3);

	// E claims it: CLAIMED stamps down, and once it lifts the ticket's gone from the list.
	Screen->HandleKey(EKeys::E);
	View = Screen->GetView();
	TestTrue(TEXT("E: claimed into the backpack, the stamp down"), View.bStamping && Inventory->GetBackpack().Num() == 1
		&& LostLoot->GetStore().FindTicket(3) == INDEX_NONE);
	Screen->HandleKey(EKeys::S);
	TestEqual(TEXT("...the keys wait for the stamp"), Screen->GetView().Chosen, 3);
	Screen->Advance(1.f);
	View = Screen->GetView();
	TestTrue(TEXT("...lifted: two tickets left, the next chosen"), !View.bStamping && View.Tickets == TArray<int32>({ 2, 1 }) && View.Chosen == 2);
	TestEqual(TEXT("...the count"), View.Count, FString(TEXT("2 / 30")));

	// The next fills the backpack; with the one slot full too, the last has no room and stays.
	Screen->HandleKey(EKeys::Enter);
	Screen->Advance(1.f);
	TestTrue(TEXT("Enter claims too: the backpack full"), Inventory->GetBackpack().Num() == 2 && Screen->GetView().Tickets == TArray<int32>{ 1 });
	Screen->HandleKey(EKeys::E);
	View = Screen->GetView();
	TestTrue(TEXT("No room: no stamp, the ticket stays, the reason said"), !View.bStamping && View.Tickets == TArray<int32>{ 1 }
		&& View.Status.Contains(TEXT("No room")));

	// U asks before it buys shelving; Esc backs out of the confirm without closing anything.
	Screen->HandleKey(EKeys::U);
	TestTrue(TEXT("U: the confirm asks"), Screen->GetView().bConfirmOpen && Wallet->GetSilver() == 1000);
	TestTrue(TEXT("...its prompts"), !Screen->GetView().Prompts.IsEmpty() && Screen->GetView().Prompts[0] == TEXT("E: Buy"));
	Screen->HandleKey(EKeys::Escape);
	TestTrue(TEXT("Esc: the confirm gone, nothing bought"), !Screen->GetView().bConfirmOpen && LostLoot->GetCapacity() == 30);
	Screen->HandleKey(EKeys::U);
	Screen->HandleKey(EKeys::E);
	View = Screen->GetView();
	TestTrue(TEXT("U then E: bought for 300, room for 40"), !View.bConfirmOpen && LostLoot->GetCapacity() == 40 && Wallet->GetSilver() == 700
		&& View.Count == TEXT("1 / 40"));
	TestEqual(TEXT("...the next shelving dearer"), View.UpgradePrice, 600);

	// Not enough silver: refused at once, no confirm.
	Wallet->SetSilver(10);
	Screen->HandleKey(EKeys::U);
	View = Screen->GetView();
	TestTrue(TEXT("Too poor: no confirm, nothing bought, the reason said"), !View.bConfirmOpen && LostLoot->GetCapacity() == 40
		&& View.Status.Contains(TEXT("Not enough silver")));
	return true;
}

#endif

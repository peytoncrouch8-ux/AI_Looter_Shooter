#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/Inventory/LoadoutRules.h"
#include "UI/Inventory/LoadoutWidget.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponParts.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "Tests/AutomationCommon.h"

namespace
{
	/** A gun of a kind with the numbers the rules read, and nothing else. */
	FWeaponInstanceData LoadoutTestGun(UWeaponDefinition* Kind, EWeaponRarity Rarity, int32 Level, float Damage, float FireRate)
	{
		FWeaponInstanceData Gun;
		Gun.Definition = Kind;
		Gun.Rarity = Rarity;
		Gun.Level = Level;
		Gun.Stats.Damage = Damage;
		Gun.Stats.FireRate = FireRate;
		return Gun;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoadoutSortTest, "Looter.Inventory.Sort",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLoadoutSortTest::RunTest(const FString& Parameters)
{
	// The backpack list's four orders. Ties keep backpack order, so guns that are alike never trade places.
	using LoadoutRules::ESort;
	UWeaponDefinition* Rifle = NewObject<UWeaponDefinition>();
	UWeaponDefinition* Shotgun = NewObject<UWeaponDefinition>();
	const TArray<FWeaponInstanceData> Backpack = {
		LoadoutTestGun(Rifle, EWeaponRarity::Common, 5, 20.f, 600.f),      // 0: 200 damage a second
		LoadoutTestGun(Shotgun, EWeaponRarity::Epic, 3, 10.f, 80.f),       // 1: 13
		LoadoutTestGun(Rifle, EWeaponRarity::Rare, 7, 30.f, 600.f),        // 2: 300
		LoadoutTestGun(Rifle, EWeaponRarity::Rare, 9, 25.f, 600.f),        // 3: 250
		LoadoutTestGun(Shotgun, EWeaponRarity::Legendary, 2, 12.f, 80.f),  // 4: 16
		LoadoutTestGun(Rifle, EWeaponRarity::Rare, 7, 30.f, 600.f),        // 5: the same as 2
	};
	const FWeaponInstanceData SlotRifle = LoadoutTestGun(Rifle, EWeaponRarity::Rare, 8, 26.f, 600.f);
	const FWeaponInstanceData SlotShotgun = LoadoutTestGun(Shotgun, EWeaponRarity::Rare, 8, 11.f, 80.f);
	TArray<int32> Order;

	TestEqual(TEXT("Best for a rifle slot: four rifles lead"), LoadoutRules::SortBackpack(Backpack, &SlotRifle, ESort::Match, Order), 4);
	TestTrue(TEXT("...the most damage a second first, then the other kinds by rarity"), Order == TArray<int32>{ 2, 5, 3, 0, 4, 1 });
	TestEqual(TEXT("Best for a shotgun slot: two shotguns lead"), LoadoutRules::SortBackpack(Backpack, &SlotShotgun, ESort::Match, Order), 2);
	TestTrue(TEXT("...then the rifles by rarity and level"), Order == TArray<int32>{ 4, 1, 3, 2, 5, 0 });
	TestEqual(TEXT("By rarity: nothing leads as a kind"), LoadoutRules::SortBackpack(Backpack, &SlotRifle, ESort::Rarity, Order), 0);
	TestTrue(TEXT("...rarest first, then the higher level"), Order == TArray<int32>{ 4, 1, 3, 2, 5, 0 });
	LoadoutRules::SortBackpack(Backpack, &SlotRifle, ESort::Level, Order);
	TestTrue(TEXT("By level: highest first, then the rarer"), Order == TArray<int32>{ 3, 2, 5, 0, 1, 4 });
	LoadoutRules::SortBackpack(Backpack, &SlotRifle, ESort::Newest, Order);
	TestTrue(TEXT("Newest: the last found first"), Order == TArray<int32>{ 5, 4, 3, 2, 1, 0 });
	TestEqual(TEXT("An empty backpack"), LoadoutRules::SortBackpack({}, &SlotRifle, ESort::Match, Order), 0);
	TestTrue(TEXT("...lists nothing"), Order.IsEmpty());

	// R steps through them and comes round.
	TestTrue(TEXT("Best for slot, then rarity"), LoadoutRules::NextSort(ESort::Match) == ESort::Rarity);
	TestTrue(TEXT("...level, newest"), LoadoutRules::NextSort(ESort::Rarity) == ESort::Level && LoadoutRules::NextSort(ESort::Level) == ESort::Newest);
	TestTrue(TEXT("...and round again"), LoadoutRules::NextSort(ESort::Newest) == ESort::Match);
	TestEqual(TEXT("Its name on the list"), FString(LoadoutRules::SortName(ESort::Match)), FString(TEXT("Best for slot")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoadoutCompareTest, "Looter.Inventory.Compare",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLoadoutCompareTest::RunTest(const FString& Parameters)
{
	// The card's arrows: up (green) for better and down (red) for worse, whichever way the number moved, and none for a
	// change too small to show.
	using LoadoutRules::EChange;
	using LoadoutRules::EStat;
	FWeaponStats Old;
	Old.Damage = 20.f;
	Old.FireRate = 600.f;
	Old.MagazineSize = 30;
	Old.ReloadTime = 2.f;
	Old.Spread = 1.f;
	Old.Range = 4000.f;
	Old.Recoil = 1.f;
	Old.Handling = 1.f;
	Old.Zoom = 1.f;
	FWeaponStats New = Old;
	New.Damage = 26.f;
	New.FireRate = 600.2f;
	New.ReloadTime = 1.75f;
	New.Spread = 1.5f;
	New.Range = 3000.f;
	New.Recoil = 0.8f;
	New.Handling = 1.2f;
	New.Zoom = 4.f;

	TestTrue(TEXT("More damage: better"), LoadoutRules::CompareStat(EStat::Damage, New, Old) == EChange::Better);
	TestEqual(TEXT("...by +6"), LoadoutRules::DeltaText(EStat::Damage, New, Old), FString(TEXT("+6")));
	TestTrue(TEXT("A faster reload: better, though the number fell"), LoadoutRules::CompareStat(EStat::Reload, New, Old) == EChange::Better);
	TestEqual(TEXT("...by -0.25s"), LoadoutRules::DeltaText(EStat::Reload, New, Old), FString(TEXT("-0.25s")));
	TestTrue(TEXT("A wider spread: worse, though the number rose"), LoadoutRules::CompareStat(EStat::Accuracy, New, Old) == EChange::Worse);
	TestEqual(TEXT("...by +0.5°"), LoadoutRules::DeltaText(EStat::Accuracy, New, Old), FString(TEXT("+0.5°")));
	TestTrue(TEXT("Less recoil: better"), LoadoutRules::CompareStat(EStat::Recoil, New, Old) == EChange::Better);
	TestEqual(TEXT("...by -20%"), LoadoutRules::DeltaText(EStat::Recoil, New, Old), FString(TEXT("-20%")));
	TestTrue(TEXT("Shorter range: worse"), LoadoutRules::CompareStat(EStat::Range, New, Old) == EChange::Worse);
	TestEqual(TEXT("...by -10 m"), LoadoutRules::DeltaText(EStat::Range, New, Old), FString(TEXT("-10 m")));
	TestTrue(TEXT("Quicker handling: better"), LoadoutRules::CompareStat(EStat::Handling, New, Old) == EChange::Better);
	TestTrue(TEXT("More zoom: better"), LoadoutRules::CompareStat(EStat::Zoom, New, Old) == EChange::Better);
	TestEqual(TEXT("...by +3x"), LoadoutRules::DeltaText(EStat::Zoom, New, Old), FString(TEXT("+3x")));
	TestTrue(TEXT("The same magazine: no arrow"), LoadoutRules::CompareStat(EStat::Magazine, New, Old) == EChange::Same);
	TestTrue(TEXT("...nor its change written"), LoadoutRules::DeltaText(EStat::Magazine, New, Old).IsEmpty());
	TestTrue(TEXT("A change the card's rounding hides: no arrow"), LoadoutRules::CompareStat(EStat::FireRate, New, Old) == EChange::Same);

	// A shotgun's damage is the whole shot, so it lines up with a rifle's; its card writes it per pellet.
	FWeaponStats Shotgun = Old;
	Shotgun.Damage = 7.f;
	Shotgun.PelletsPerShot = 9;
	TestEqual(TEXT("A shotgun's whole shot"), LoadoutRules::StatValue(EStat::Damage, Shotgun), 63.f);
	TestEqual(TEXT("...written per pellet"), LoadoutRules::StatText(EStat::Damage, Shotgun), FString(TEXT("7 x9")));
	TestTrue(TEXT("Less is better for reload, spread and recoil only"), !LoadoutRules::HigherIsBetter(EStat::Reload)
		&& !LoadoutRules::HigherIsBetter(EStat::Accuracy) && !LoadoutRules::HigherIsBetter(EStat::Recoil)
		&& LoadoutRules::HigherIsBetter(EStat::Damage) && LoadoutRules::HigherIsBetter(EStat::Handling));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoadoutFirstGlanceTest, "Looter.Inventory.FirstGlance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLoadoutFirstGlanceTest::RunTest(const FString& Parameters)
{
	// At first glance a card shows damage and the four stats that decide how a gun fights; the rest wait behind Inspect,
	// which lists every stat in the same order, the first glance's first.
	using LoadoutRules::EStat;
	const TConstArrayView<EStat> First = LoadoutRules::FirstGlanceStats();
	const TConstArrayView<EStat> All = LoadoutRules::AllStats();
	TestTrue(TEXT("Damage, fire rate, magazine, reload, accuracy"),
		TArray<EStat>(First.GetData(), First.Num()) == TArray<EStat>{ EStat::Damage, EStat::FireRate, EStat::Magazine, EStat::Reload, EStat::Accuracy });
	TestEqual(TEXT("Inspect shows all nine"), All.Num(), 9);
	bool bFirstLead = All.Num() >= First.Num();
	for (int32 Index = 0; bFirstLead && Index < First.Num(); ++Index)
	{
		bFirstLead = All[Index] == First[Index];
	}
	TestTrue(TEXT("...the first glance's first"), bFirstLead);
	for (const EStat Later : { EStat::Range, EStat::Recoil, EStat::Handling, EStat::Zoom })
	{
		TestFalse(FString::Printf(TEXT("%s waits for Inspect"), LoadoutRules::StatName(Later)), First.Contains(Later));
		TestTrue(FString::Printf(TEXT("%s is on Inspect"), LoadoutRules::StatName(Later)), All.Contains(Later));
	}

	// The parts' bonuses: the most helpful first (less reload time and recoil help), tiny ones left out; the first glance
	// takes the best two that help.
	FWeaponPartTotals Totals;
	Totals.Damage = 12.f;
	Totals.Accuracy = 24.f;
	Totals.FireRate = -10.f;
	Totals.Reload = -30.f;
	Totals.Recoil = 15.f;
	Totals.Handling = 0.3f;
	const TArray<LoadoutRules::FBonus> Bonuses = LoadoutRules::Bonuses(Totals);
	TArray<EStat> Stats;
	for (const LoadoutRules::FBonus& Bonus : Bonuses)
	{
		Stats.Add(Bonus.Stat);
	}
	TestTrue(TEXT("Most helpful first, most harmful last, tiny ones out"),
		Stats == TArray<EStat>{ EStat::Reload, EStat::Accuracy, EStat::Damage, EStat::FireRate, EStat::Recoil });
	const TArray<LoadoutRules::FBonus> Top = LoadoutRules::TopBonuses(Totals);
	TestTrue(TEXT("The first glance: the best two"), Top.Num() == 2 && Top[0].Stat == EStat::Reload && Top[1].Stat == EStat::Accuracy);
	TestEqual(TEXT("Written as the parts give it"), LoadoutRules::BonusText(Top[0]), FString(TEXT("-30% RELOAD TIME")));
	TestEqual(TEXT("...a gain with its plus"), LoadoutRules::BonusText(Top[1]), FString(TEXT("+24% ACCURACY")));
	FWeaponPartTotals Poor;
	Poor.Damage = -10.f;
	Poor.Recoil = 20.f;
	TestTrue(TEXT("Parts that only cost: no line at first glance"), LoadoutRules::TopBonuses(Poor).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoadoutIdentityTest, "Looter.Inventory.NewMarks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLoadoutIdentityTest::RunTest(const FString& Parameters)
{
	// A gun is NEW once, when it's found: what happens to it after doesn't make it new again; another gun is another.
	UWeaponDefinition* Rifle = NewObject<UWeaponDefinition>();
	FWeaponInstanceData Gun = LoadoutTestGun(Rifle, EWeaponRarity::Rare, 7, 30.f, 600.f);
	Gun.Seed = 4711;
	const uint32 Identity = LoadoutRules::GunIdentity(Gun);
	FWeaponInstanceData Used = Gun;
	Used.Kills = 120;
	Used.Parts = { FName(TEXT("Marksman")) };
	Used.SavedMagazine = 3;
	Used.bCurseLifted = true;
	TestTrue(TEXT("Used, notched and refitted: the same gun"), LoadoutRules::GunIdentity(Used) == Identity);
	FWeaponInstanceData Other = Gun;
	Other.Seed = 4712;
	TestTrue(TEXT("Another seed: another gun"), LoadoutRules::GunIdentity(Other) != Identity);
	Other = Gun;
	Other.Rarity = EWeaponRarity::Epic;
	TestTrue(TEXT("Another rarity: another gun"), LoadoutRules::GunIdentity(Other) != Identity);
	Other = Gun;
	Other.Level = 8;
	TestTrue(TEXT("Another level: another gun"), LoadoutRules::GunIdentity(Other) != Identity);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoadoutScreenTest, "Looter.Inventory.Screen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLoadoutScreenTest::RunTest(const FString& Parameters)
{
	// The loadout screen over a real weapon manager, driven by its keys: the cursor, the swap target, Inspect, the sort,
	// equipping, stowing and dropping, and the prompts saying what E does. (A test's editor world has no showcase.)
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	UWeaponDefinition* Shotgun = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_PumpShotgun.DA_PumpShotgun"));
	if (!TestNotNull(TEXT("Rifle loads"), Rifle) || !TestNotNull(TEXT("Shotgun loads"), Shotgun))
	{
		return false;
	}
	APawn* Holder = World->SpawnActor<APawn>();
	UWeaponManagerComponent* Inventory = NewObject<UWeaponManagerComponent>(Holder);
	Inventory->RegisterComponent();
	Inventory->MaxWeapons = 3;
	Inventory->BackpackCapacity = 12;

	// Two equipped (a rifle in hand, a shotgun); in the backpack a better rifle, a shotgun and a worse rifle.
	Inventory->GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Rare, 5));
	Inventory->GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Shotgun, EWeaponRarity::Common, 5));
	Inventory->EquipSlot(0);
	if (!TestEqual(TEXT("Two guns equipped"), Inventory->GetWeapons().Num(), 2))
	{
		return false;
	}
	const FWeaponStats SlotRifle = Inventory->GetWeapons()[0]->GetInstance().Stats;
	FWeaponInstanceData Better = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Epic, 5);
	Better.Seed = 101;
	Better.Stats = SlotRifle;
	Better.Stats.Damage = SlotRifle.Damage * 2.f;
	FWeaponInstanceData Pump = UWeaponRollLibrary::RollWeaponWithRarity(Shotgun, EWeaponRarity::Rare, 5);
	FWeaponInstanceData Worse = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Common, 5);
	Worse.Seed = 202;
	Worse.Stats = SlotRifle;
	Worse.Stats.Damage = SlotRifle.Damage * 0.5f;
	Inventory->AddToBackpack(Better);
	Inventory->AddToBackpack(Pump);
	Inventory->AddToBackpack(Worse);

	ULoadoutWidget* Screen = CreateWidget<ULoadoutWidget>(World, ULoadoutWidget::StaticClass());
	if (!TestNotNull(TEXT("The screen"), Screen))
	{
		return false;
	}
	Screen->Open(nullptr, Inventory);
	Screen->TakeWidget();
	ULoadoutWidget::FView View = Screen->GetView();
	TestTrue(TEXT("Opens on the gun in hand, its slot the swap target"), View.bOnSlot && View.Cursor == 0 && View.TargetSlot == 0 && !View.bInspecting);
	TestTrue(TEXT("Best for the rifle slot: the better rifle, the worse, then the shotgun"), View.ListOrder == TArray<int32>{ 0, 2, 1 });
	TestTrue(TEXT("E on a slot swaps from the backpack"), !View.Prompts.IsEmpty() && View.Prompts[0] == TEXT("E: Swap from backpack"));

	// W / S walk the slots; walking over them doesn't change the swap target.
	Screen->HandleKey(EKeys::S);
	View = Screen->GetView();
	TestTrue(TEXT("S: the second slot"), View.bOnSlot && View.Cursor == 1 && View.TargetSlot == 0);
	Screen->HandleKey(EKeys::W);
	Screen->HandleKey(EKeys::D);
	View = Screen->GetView();
	TestTrue(TEXT("D: the backpack's first gun"), !View.bOnSlot && View.Cursor == 0);
	TestTrue(TEXT("...E swaps it into slot 1"), !View.Prompts.IsEmpty() && View.Prompts[0] == TEXT("E: Swap into slot 1"));

	// Inspect opens and Esc backs out of it before it closes anything.
	Screen->HandleKey(EKeys::X);
	View = Screen->GetView();
	TestTrue(TEXT("X: Inspect"), View.bInspecting && !View.Prompts.IsEmpty() && View.Prompts[0] == TEXT("X: Back"));
	Screen->HandleKey(EKeys::Escape);
	TestFalse(TEXT("Esc: back from Inspect"), Screen->GetView().bInspecting);

	// R re-sorts with the cursor kept on the same gun.
	Screen->HandleKey(EKeys::R);
	View = Screen->GetView();
	TestTrue(TEXT("R: by rarity"), View.Sort == LoadoutRules::ESort::Rarity);
	TestTrue(TEXT("...the cursor still on the better rifle"), View.ListOrder.IsValidIndex(View.Cursor) && View.ListOrder[View.Cursor] == 0);
	for (int32 Press = 0; Press < 3; ++Press)
	{
		Screen->HandleKey(EKeys::R);
	}
	TestTrue(TEXT("...and round to best for slot"), Screen->GetView().Sort == LoadoutRules::ESort::Match);

	// E equips the better rifle in slot 1; the cursor follows it there.
	Screen->HandleKey(EKeys::D);
	View = Screen->GetView();
	const int32 BetterRow = View.ListOrder.IndexOfByKey(0);
	while (Screen->GetView().Cursor < BetterRow)
	{
		Screen->HandleKey(EKeys::S);
	}
	Screen->HandleKey(EKeys::E);
	View = Screen->GetView();
	TestTrue(TEXT("E: the better rifle in slot 1"), Inventory->GetWeapons().IsValidIndex(0) && Inventory->GetWeapons()[0]->GetInstance().Seed == Better.Seed);
	TestTrue(TEXT("...the cursor on it"), View.bOnSlot && View.Cursor == 0);
	TestEqual(TEXT("...the old rifle in the backpack"), Inventory->GetBackpack().Num(), 3);

	// E on the shotgun's slot makes it the target: the backpack's shotguns lead.
	Screen->HandleKey(EKeys::S);
	Screen->HandleKey(EKeys::E);
	View = Screen->GetView();
	TestTrue(TEXT("E on slot 2: it's the target, the cursor in the backpack"), View.TargetSlot == 1 && !View.bOnSlot && View.Cursor == 0);
	const TArray<FWeaponInstanceData>& Pack = Inventory->GetBackpack();
	TestTrue(TEXT("...a shotgun first"), View.ListOrder.IsValidIndex(0) && Pack.IsValidIndex(View.ListOrder[0]) && Pack[View.ListOrder[0]].Definition == Shotgun);

	// Q drops the gun under the cursor; C stows a slot's gun.
	Screen->HandleKey(EKeys::Q);
	TestEqual(TEXT("Q: one fewer in the backpack"), Inventory->GetBackpack().Num(), 2);
	Screen->HandleKey(EKeys::A);
	View = Screen->GetView();
	TestTrue(TEXT("A: back to the target slot"), View.bOnSlot && View.Cursor == 1);
	Screen->HandleKey(EKeys::C);
	TestTrue(TEXT("C: the shotgun stowed"), Inventory->GetWeapons().Num() == 1 && Inventory->GetBackpack().Num() == 3);
	return true;
}

#endif

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Inventory/GunSlotRules.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Missions/MissionDefinition.h"
#include "UI/HUD/HudGunSlotBannerWidget.h"
#include "UI/HUD/HudLevelUpBannerWidget.h"
#include "UI/HUD/HudMissionCompleteWidget.h"
#include "UI/HUD/HudWeaponSlotsWidget.h"
#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/Inventory/LoadoutWidget.h"
#include "Weapons/WeaponDefinition.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// The equip slots opening with progress, on screen (Looter.UI.GunSlots.*): the HUD's column of four at 1080p (shut ones dim
// with their lock, the opening flash), the loadout's shut rows ("Unlocks at level 3"), and "NEW GUN SLOT" taking its turn.

namespace
{
	FGunSlotProgress SlotTestProgress(int32 Level, bool bStoryDone)
	{
		FGunSlotProgress Progress;
		Progress.Level = Level;
		Progress.bStoryDone = bStoryDone;
		return Progress;
	}

	/** A holder with every slot (MaxWeapons 4) whose slots open as a player's at Progress. */
	UWeaponManagerComponent* SlotTestHolder(UWorld* World, const FGunSlotProgress& Progress)
	{
		APawn* Holder = World->SpawnActor<APawn>();
		UWeaponManagerComponent* Inventory = Holder ? NewObject<UWeaponManagerComponent>(Holder) : nullptr;
		if (Inventory)
		{
			Inventory->RegisterComponent();
			Inventory->MaxWeapons = GunSlotRules::NumSlots;
			Inventory->BackpackCapacity = 12;
			Inventory->SetSlotProgressForTesting(Progress);
		}
		return Inventory;
	}

	UWeaponDefinition* SlotTestRifle()
	{
		return LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunSlotColumnTest, "Looter.UI.GunSlots.Column",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunSlotColumnTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	UWeaponDefinition* Rifle = SlotTestRifle();
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)) || !TestNotNull(TEXT("Rifle loads"), Rifle))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UWeaponManagerComponent* Inventory = SlotTestHolder(World, SlotTestProgress(1, false));
	UHudWeaponSlotsWidget* Column = CreateWidget<UHudWeaponSlotsWidget>(World, UHudWeaponSlotsWidget::StaticClass());
	if (!TestNotNull(TEXT("A holder"), Inventory) || !TestNotNull(TEXT("The column"), Column))
	{
		return false;
	}
	Inventory->GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Rare, 1));
	Inventory->GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Common, 1));
	const TSharedRef<SWidget> ColumnSlate = Column->TakeWidget();
	Column->Update(Inventory, 0.f);
	TestTrue(TEXT("Four slots shown"), Column->IsSlotShown(0) && Column->IsSlotShown(1) && Column->IsSlotShown(2) && Column->IsSlotShown(3));
	TestTrue(TEXT("...the two guns' open, slot 3 shut by the level, slot 4 by the story"), Column->GetShownLock(0) == EGunSlotLock::Open
		&& Column->GetShownLock(1) == EGunSlotLock::Open && Column->GetShownLock(2) == EGunSlotLock::Level && Column->GetShownLock(3) == EGunSlotLock::Story);
	ColumnSlate->SlatePrepass(1.f);
	TestTrue(FString::Printf(TEXT("The column is four rows tall (%.0f px)"), Column->GetDesiredSize().Y),
		FMath::IsNearlyEqual(static_cast<float>(Column->GetDesiredSize().Y), UHudWeaponSlotsWidget::ColumnHeight(4), 1.f));

	// The moment slot 3 opens, it flashes open.
	Inventory->SetSlotProgressForTesting(SlotTestProgress(3, false));
	Column->Update(Inventory, 0.1f);
	TestTrue(TEXT("Level 3: slot 3 open and flashing"), Column->GetShownLock(2) == EGunSlotLock::Open && Column->IsUnlockFlashing(2));
	Column->Update(Inventory, UHudWeaponSlotsWidget::UnlockSeconds + 0.1f);
	TestFalse(TEXT("...for a moment"), Column->IsUnlockFlashing(2));

	// In the HUD at 1080p: the weapon cluster with its four slots stays in the screen's lower part, under the right edge's
	// middle (the in-hand card) and the minimap's corner, its top 704 px down with the grenade row collapsed.
	UPlayerHUDWidget* Hud = CreateWidget<UPlayerHUDWidget>(World, UPlayerHUDWidget::StaticClass());
	if (!TestNotNull(TEXT("The HUD"), Hud))
	{
		return false;
	}
	const TSharedRef<SWidget> HudSlate = Hud->TakeWidget();
	HudSlate->SlatePrepass(1.f);
	const UHudWeaponSlotsWidget* HudSlots = Hud->GetWeaponSlots();
	const UPanelWidget* Arms = HudSlots ? HudSlots->GetParent() : nullptr;
	const UWidget* Cluster = Arms ? Arms->GetParent() : nullptr;
	const UCanvasPanelSlot* ClusterSlot = Cluster ? Cast<UCanvasPanelSlot>(Cluster->Slot) : nullptr;
	if (!TestTrue(TEXT("The cluster on the HUD's canvas"), HudSlots && ClusterSlot))
	{
		return false;
	}
	constexpr float ScreenHeight = 1080.f;
	const float SlotsHeight = static_cast<float>(HudSlots->GetDesiredSize().Y);
	const float ClusterTop = ScreenHeight + static_cast<float>(ClusterSlot->GetPosition().Y) - static_cast<float>(Cluster->GetDesiredSize().Y);
	TestTrue(FString::Printf(TEXT("The HUD's column is four slots tall (%.0f px)"), SlotsHeight),
		FMath::IsNearlyEqual(SlotsHeight, UHudWeaponSlotsWidget::ColumnHeight(4), 1.f));
	TestTrue(FString::Printf(TEXT("The cluster's top (%.0f px down) stays in the lower part of a 1080p screen"), ClusterTop),
		ClusterTop >= ScreenHeight * 0.6f && ClusterTop < ScreenHeight);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunSlotLoadoutTest, "Looter.UI.GunSlots.Loadout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunSlotLoadoutTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	UWeaponDefinition* Rifle = SlotTestRifle();
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)) || !TestNotNull(TEXT("Rifle loads"), Rifle))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UWeaponManagerComponent* Inventory = SlotTestHolder(World, SlotTestProgress(1, false));
	ULoadoutWidget* Screen = CreateWidget<ULoadoutWidget>(World, ULoadoutWidget::StaticClass());
	if (!TestNotNull(TEXT("A holder"), Inventory) || !TestNotNull(TEXT("The screen"), Screen))
	{
		return false;
	}
	Inventory->GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Rare, 1));
	Inventory->GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Common, 1));
	FWeaponInstanceData Spare = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Uncommon, 1);
	Spare.Seed = 77;
	Inventory->AddToBackpack(Spare);
	Inventory->EquipSlot(0);

	Screen->Open(nullptr, Inventory);
	Screen->TakeWidget();
	ULoadoutWidget::FView View = Screen->GetView();
	TestTrue(TEXT("EQUIPPED lists four rows, the last two shut"), View.LockedSlots == TArray<bool>({ false, false, true, true }));
	TestEqual(TEXT("...and counts 2 / 4"), View.EquippedCount, FString(TEXT("2 / 4")));

	// Down to slot 3: its card says how it opens, and nothing goes in.
	Screen->HandleKey(EKeys::S);
	Screen->HandleKey(EKeys::S);
	View = Screen->GetView();
	TestTrue(TEXT("The cursor on slot 3"), View.bOnSlot && View.Cursor == 2);
	TestEqual(TEXT("...its card: locked"), View.PlainCardTitle, FString(TEXT("Slot 3 is locked")));
	TestFalse(TEXT("...no \"choose from backpack\" for it"), View.Prompts.Contains(TEXT("E: Choose from backpack")));
	Screen->HandleKey(EKeys::E);
	View = Screen->GetView();
	TestTrue(TEXT("E there does nothing: still on slot 3, still two guns"), View.bOnSlot && View.Cursor == 2 && Inventory->GetWeapons().Num() == 2);

	// Level 3: slot 3 opens on the screen and takes the backpack's gun.
	Inventory->SetSlotProgressForTesting(SlotTestProgress(3, false));
	Inventory->OnInventoryChanged.Broadcast();
	View = Screen->GetView();
	TestTrue(TEXT("Level 3: only slot 4 shut"), View.LockedSlots == TArray<bool>({ false, false, false, true }));
	Screen->HandleKey(EKeys::E);
	Screen->HandleKey(EKeys::E);
	TestEqual(TEXT("E, E: the backpack's gun into slot 3"), Inventory->GetWeapons().Num(), 3);
	TestEqual(TEXT("...3 / 4"), Screen->GetView().EquippedCount, FString(TEXT("3 / 4")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunSlotBannerTest, "Looter.UI.GunSlots.Banner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunSlotBannerTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UHudGunSlotBannerWidget* Banner = CreateWidget<UHudGunSlotBannerWidget>(World, UHudGunSlotBannerWidget::StaticClass());
	UHudLevelUpBannerWidget* LevelUp = CreateWidget<UHudLevelUpBannerWidget>(World, UHudLevelUpBannerWidget::StaticClass());
	UHudMissionCompleteWidget* Mission = CreateWidget<UHudMissionCompleteWidget>(World, UHudMissionCompleteWidget::StaticClass());
	if (!TestTrue(TEXT("The banners"), Banner && LevelUp && Mission))
	{
		return false;
	}
	const TSharedRef<SWidget> BannerSlate = Banner->TakeWidget();
	const TSharedRef<SWidget> LevelUpSlate = LevelUp->TakeWidget();
	const TSharedRef<SWidget> MissionSlate = Mission->TakeWidget();
	TestTrue(TEXT("Clear of the mission tracker (under 768 wide)"), UHudGunSlotBannerWidget::Width < 768.f);

	// A level's slot follows its LEVEL UP: while that shows, it waits.
	Banner->SetOtherBanners(LevelUp, Mission, nullptr, nullptr);
	LevelUp->Show(3);
	Banner->Announce(2, EGunSlotLock::Level);
	Banner->Advance(1.f);
	TestTrue(TEXT("Waits while LEVEL UP shows"), Banner->IsBusy() && !Banner->IsShowing() && Banner->GetQueued() == 1);

	// Alone, a level's slot waits a while for its LEVEL UP (it comes as the experience bar gets there), then shows.
	UHudGunSlotBannerWidget* Alone = CreateWidget<UHudGunSlotBannerWidget>(World, UHudGunSlotBannerWidget::StaticClass());
	const TSharedRef<SWidget> AloneSlate = Alone->TakeWidget();
	Alone->SetOtherBanners(nullptr, Mission, nullptr, nullptr);
	Alone->Announce(2, EGunSlotLock::Level);
	Alone->Advance(1.f);
	TestFalse(TEXT("A level's slot waits for its LEVEL UP"), Alone->IsShowing());
	Alone->Advance(UHudGunSlotBannerWidget::LevelUpWaitSeconds);
	TestTrue(TEXT("...not forever: NEW GUN SLOT, slot 3"), Alone->IsShowing() && Alone->GetShownSlot() == 2);
	TestEqual(TEXT("...\"CARRY THREE GUNS\""), Alone->GetShownReward(), FString(TEXT("CARRY THREE GUNS")));
	TestFalse(TEXT("...shut at first"), Alone->IsOpenedInShow());
	Alone->Advance(0.5f);
	TestTrue(TEXT("...then the padlock pops off"), Alone->IsOpenedInShow());

	// The mission-complete banner waits while it shows (a turn-in right after the slot opened).
	UMissionDefinition* Turned = NewObject<UMissionDefinition>(GetTransientPackage());
	Turned->Id = TEXT("SlotTestTurnIn");
	Turned->Kind = EMissionKind::Main;
	Turned->Title = FText::FromString(TEXT("A Mission"));
	Mission->SetGunSlotBanner(Alone);
	Mission->Announce(*Turned, FMissionRewardsGiven());
	Mission->Advance(0.1f);
	TestFalse(TEXT("MISSION COMPLETE waits for NEW GUN SLOT"), Mission->IsShowing());
	Alone->Advance(UHudGunSlotBannerWidget::ShowSeconds);
	TestFalse(TEXT("NEW GUN SLOT done"), Alone->IsBusy());
	Mission->Advance(0.1f);
	TestTrue(TEXT("...then MISSION COMPLETE shows"), Mission->IsShowing());

	// The story's slot waits for that banner, then a short calm.
	Alone->Announce(3, EGunSlotLock::Story);
	Alone->Advance(0.1f);
	TestFalse(TEXT("The story's slot waits for MISSION COMPLETE"), Alone->IsShowing());
	Mission->Advance(UHudMissionCompleteWidget::ShowSeconds * 2.f);
	Alone->Advance(UHudGunSlotBannerWidget::CalmSeconds * 0.5f);
	TestFalse(TEXT("...and a calm after it"), Alone->IsShowing());
	Alone->Advance(UHudGunSlotBannerWidget::CalmSeconds);
	TestTrue(TEXT("...then slot 4: \"CARRY FOUR GUNS\""), Alone->IsShowing() && Alone->GetShownSlot() == 3
		&& Alone->GetShownReward() == TEXT("CARRY FOUR GUNS"));
	return true;
}

#endif

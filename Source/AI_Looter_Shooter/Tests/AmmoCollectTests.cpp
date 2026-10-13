#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Inventory/WeaponManagerComponent.h"
#include "Loot/AmmoPickup.h"
#include "Loot/GrenadePickup.h"
#include "Player/PlayerThrowComponent.h"
#include "Player/PlayerThrowRules.h"
#include "UI/HUD/HudPickupFeedWidget.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// Drops are always collected (Looter.Loot.AmmoCollect, .GrenadeCollect): with the pool full, or short of room, the pickup
// is spent anyway and what doesn't fit is lost, so a field of drops needn't be cleared by hand. The player is told of
// only what they actually got: the feed's "+N" line and the pickup sound, and no "full" message.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootAmmoCollectTest, "Looter.Loot.AmmoCollect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootAmmoCollectTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UWeaponManagerComponent* Inventory = NewObject<UWeaponManagerComponent>();
	const EAmmoType Type = EAmmoType::Pistol;
	const int32 Max = Inventory->GetMaxAmmo(Type);
	if (!TestTrue(TEXT("A pool big enough to test with"), Max > 60))
	{
		return false;
	}

	// Room for all of it: it goes in whole, and the pickup is gone.
	AAmmoPickup* Whole = AAmmoPickup::SpawnAmmo(World, Type, 12, FVector::ZeroVector);
	if (!TestNotNull(TEXT("A pickup of 12"), Whole))
	{
		return false;
	}
	TWeakObjectPtr<AAmmoPickup> WholeGone(Whole);
	TestEqual(TEXT("All twelve added"), Whole->CollectInto(*Inventory), 12);
	TestEqual(TEXT("...in the pool"), Inventory->GetAmmo(Type), 12);
	TestFalse(TEXT("...and the pickup is gone from the world"), WholeGone.IsValid());

	// Short of room: the part that fits goes in (a partial add), the rest is lost, and the pickup still goes.
	TestEqual(TEXT("Filled to five under the limit"), Inventory->AddAmmo(Type, Max - 5 - 12), Max - 5 - 12);
	AAmmoPickup* Partial = AAmmoPickup::SpawnAmmo(World, Type, 36, FVector(100.0, 0.0, 0.0));
	if (TestNotNull(TEXT("A pickup of 36"), Partial))
	{
		TWeakObjectPtr<AAmmoPickup> PartialGone(Partial);
		TestEqual(TEXT("Only the five that fit are added"), Partial->CollectInto(*Inventory), 5);
		TestEqual(TEXT("...the pool is full"), Inventory->GetAmmo(Type), Max);
		TestFalse(TEXT("...and the pickup is gone, the rest lost with it"), PartialGone.IsValid());
	}

	// Full: nothing is added, and the pickup is collected all the same.
	AAmmoPickup* Surplus = AAmmoPickup::SpawnAmmo(World, Type, 36, FVector(200.0, 0.0, 0.0));
	AAmmoPickup* OtherClass = AAmmoPickup::SpawnAmmo(World, EAmmoType::AssaultRifle, 30, FVector(300.0, 0.0, 0.0));
	if (TestNotNull(TEXT("A pickup the full pool cannot take"), Surplus) && TestNotNull(TEXT("A pickup of another class"), OtherClass))
	{
		TWeakObjectPtr<AAmmoPickup> SurplusGone(Surplus);
		TestEqual(TEXT("Nothing added to a full pool"), Surplus->CollectInto(*Inventory), 0);
		TestEqual(TEXT("...the pool stays at its limit"), Inventory->GetAmmo(Type), Max);
		TestFalse(TEXT("...and the pickup is collected anyway"), SurplusGone.IsValid());

		// Each class has its own pool: a full one doesn't turn another class's ammo away.
		TWeakObjectPtr<AAmmoPickup> OtherGone(OtherClass);
		TestEqual(TEXT("Another class still takes its rounds"), OtherClass->CollectInto(*Inventory), 30);
		TestEqual(TEXT("...into its own pool"), Inventory->GetAmmo(EAmmoType::AssaultRifle), 30);
		TestFalse(TEXT("...and its pickup is gone"), OtherGone.IsValid());
	}

	// The feed tells only of rounds added.
	TestEqual(TEXT("Five added: the feed says so"), UHudPickupFeedWidget::AmmoLine(Type, 5), FString::Printf(TEXT("+5 %s"), LooterAmmo::GetInfo(Type).Name));
	TestTrue(TEXT("None added: the feed says nothing (no 'full' line)"), UHudPickupFeedWidget::AmmoLine(Type, 0).IsEmpty());
	TestTrue(TEXT("...nor for a bad count or class"), UHudPickupFeedWidget::AmmoLine(Type, -3).IsEmpty() && UHudPickupFeedWidget::AmmoLine(EAmmoType::Count, 5).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootGrenadeCollectTest, "Looter.Loot.GrenadeCollect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootGrenadeCollectTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UPlayerThrowComponent* Throw = NewObject<UPlayerThrowComponent>();

	// Every change of the count the HUD would hear of: the counter's flash and the feed's "+N Grave Salt" line.
	int32 Changes = 0;
	int32 LastDelta = 0;
	EGrenadeChange LastWhy = EGrenadeChange::Restored;
	const FDelegateHandle Listening = Throw->OnGrenadesChanged.AddLambda([&Changes, &LastDelta, &LastWhy](int32, int32 Delta, EGrenadeChange Why)
	{
		++Changes;
		LastDelta = Delta;
		LastWhy = Why;
	});

	// Room for it: it goes in, said once as a pickup, and the tin is gone.
	Throw->AddGrenades(1, EGrenadeChange::Given);
	Changes = 0;
	AGrenadePickup* Roomy = AGrenadePickup::SpawnGrenades(World, 1, FVector::ZeroVector);
	if (TestNotNull(TEXT("A tin"), Roomy))
	{
		TWeakObjectPtr<AGrenadePickup> RoomyGone(Roomy);
		TestEqual(TEXT("One grenade added"), Roomy->CollectInto(*Throw), 1);
		TestEqual(TEXT("...two carried"), Throw->GetGrenades(), 2);
		TestTrue(TEXT("...told once, as a pickup of one"), Changes == 1 && LastDelta == 1 && LastWhy == EGrenadeChange::PickedUp);
		TestFalse(TEXT("...and the tin is gone"), RoomyGone.IsValid());
	}

	// Short of room: what fits goes in, the rest is lost.
	AGrenadePickup* Pair = AGrenadePickup::SpawnGrenades(World, 2, FVector(100.0, 0.0, 0.0));
	if (TestNotNull(TEXT("A pair of tins"), Pair))
	{
		Changes = 0;
		TWeakObjectPtr<AGrenadePickup> PairGone(Pair);
		TestEqual(TEXT("Only the one that fits is added"), Pair->CollectInto(*Throw), 1);
		TestEqual(TEXT("...three carried"), Throw->GetGrenades(), FThrowRules::MaxGrenades);
		TestTrue(TEXT("...told once, of one"), Changes == 1 && LastDelta == 1);
		TestFalse(TEXT("...and the pickup is gone"), PairGone.IsValid());
	}

	// Full: collected all the same, quietly. Nothing is added, and the count says nothing (no "full" line, no flash).
	AGrenadePickup* Spare = AGrenadePickup::SpawnGrenades(World, 1, FVector(200.0, 0.0, 0.0));
	if (TestNotNull(TEXT("A tin for a full hand"), Spare))
	{
		Changes = 0;
		TWeakObjectPtr<AGrenadePickup> SpareGone(Spare);
		TestEqual(TEXT("Nothing added at the most carried"), Spare->CollectInto(*Throw), 0);
		TestEqual(TEXT("...still three"), Throw->GetGrenades(), FThrowRules::MaxGrenades);
		TestEqual(TEXT("...and the HUD hears nothing"), Changes, 0);
		TestFalse(TEXT("...but the tin is collected"), SpareGone.IsValid());
	}

	Throw->OnGrenadesChanged.Remove(Listening);
	return true;
}

#endif

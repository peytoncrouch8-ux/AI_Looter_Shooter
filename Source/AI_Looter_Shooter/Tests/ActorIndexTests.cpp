#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/StoneSentinelCreature.h"
#include "Economy/SilverPickup.h"
#include "Loot/AmmoPickup.h"
#include "Loot/GrenadePickup.h"
#include "UI/World/LootLabelSubsystem.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "World/ActorIndexSubsystem.h"
#include "World/PlayableArea.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "HAL/IConsoleManager.h"
#include "Tests/AutomationCommon.h"

// The actor index (World/ActorIndexSubsystem; the audit of 2026-11-03: six TActorIterator scans a frame in normal play): its
// lists hold what a full look through the world finds, an actor that spawns joins at once, a destroyed one leaves at the
// next look, reading makes no look, the timer runs at the console variable's interval, and the loot labels sleep with
// nothing showing. The readers (the minimap, the labels, the interaction component, the enemy shots) are covered by their
// own tests: Looter.UI.WorldCard.* offers a gun spawned a moment before, through the index.

namespace
{
	/** How many entries of List are Actor (more than one would be a duplicate; a destroyed actor's null entry never counts). */
	template <typename ListType>
	int32 Count(const ListType& List, const AActor* Actor)
	{
		int32 Found = 0;
		for (const auto& Entry : List)
		{
			Found += Entry.Get() == Actor ? 1 : 0;
		}
		return Found;
	}

	int32 CountCreature(const TArray<FIndexedCreature>& List, const ACreatureBase* Creature)
	{
		int32 Found = 0;
		for (const FIndexedCreature& Entry : List)
		{
			Found += Entry.Creature.Get() == Creature ? 1 : 0;
		}
		return Found;
	}

	bool IsSentinel(const TArray<FIndexedCreature>& List, const ACreatureBase* Creature)
	{
		const FIndexedCreature* Entry = List.FindByPredicate([Creature](const FIndexedCreature& Each) { return Each.Creature.Get() == Creature; });
		return Entry && Entry->bSentinel;
	}

	FActorSpawnParameters AlwaysSpawn()
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return Params;
	}

	/** A console variable set as a developer's console would (outranking the code's own), put back as it was. */
	struct FIntervalScope
	{
		IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.ActorIndex.Interval"));
		float Before = Variable ? Variable->GetFloat() : 0.f;

		void Set(float Seconds) const
		{
			if (Variable)
			{
				Variable->Set(Seconds, ECVF_SetByConsole);
			}
		}

		~FIntervalScope()
		{
			Set(Before);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActorIndexListsTest, "Looter.World.ActorIndex.Lists",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FActorIndexListsTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UActorIndexSubsystem* Index = UActorIndexSubsystem::Get(World);
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	if (!TestNotNull(TEXT("The preview world keeps the actor index"), Index) || !TestNotNull(TEXT("The rifle loads"), Rifle))
	{
		return false;
	}

	// The world as it stands (a preview world has a few actors of its own): every later count is against this.
	Index->RefreshNow();
	const int32 Guns = Index->GetGuns().Num();
	const int32 Ammo = Index->GetAmmoPickups().Num();
	const int32 Grenades = Index->GetGrenadePickups().Num();
	const int32 Silver = Index->GetSilverPickups().Num();
	const int32 Creatures = Index->GetCreatures().Num();
	const int32 Characters = Index->GetCharacters().Num();
	const int32 Volumes = Index->GetVolumes().Num();
	const int32 Areas = Index->GetPlayableAreas().Num();

	AWeaponBase* Gun = UWeaponRollLibrary::SpawnWeapon(World, UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Common, 4), FTransform(FVector(0.0, 300.0, 30.0)));
	ASpiderCreature* Spider = World->SpawnActor<ASpiderCreature>(FVector(1500.0, 0.0, 0.0), FRotator::ZeroRotator, AlwaysSpawn());
	AStoneSentinelCreature* Sentinel = World->SpawnActor<AStoneSentinelCreature>(FVector(-1500.0, 0.0, 0.0), FRotator::ZeroRotator, AlwaysSpawn());
	ACharacter* Player = World->SpawnActor<ACharacter>(FVector(0.0, 0.0, 100.0), FRotator::ZeroRotator, AlwaysSpawn());
	AAmmoPickup* AmmoPickup = AAmmoPickup::SpawnAmmo(World, EAmmoType::AssaultRifle, 30, FVector(100.0, 0.0, 20.0));
	AGrenadePickup* GrenadePickup = AGrenadePickup::SpawnGrenades(World, 1, FVector(200.0, 0.0, 20.0));
	ASilverPickup* SilverPickup = ASilverPickup::SpawnSilver(World, 12, FVector(300.0, 0.0, 20.0));
	APostProcessVolume* Volume = World->SpawnActor<APostProcessVolume>();
	APlayableArea* Area = World->SpawnActor<APlayableArea>();
	if (!TestTrue(TEXT("Everything spawned"), Gun && Spider && Sentinel && Player && AmmoPickup && GrenadePickup && SilverPickup && Volume && Area))
	{
		return false;
	}
	Gun->OnDropped();

	// No look since: the spawn hook alone has them listed, each once, in the list of its kind.
	TestEqual(TEXT("The gun joined at once"), Count(Index->GetGuns(), Gun), 1);
	TestEqual(TEXT("...as the only new gun"), Index->GetGuns().Num(), Guns + 1);
	TestEqual(TEXT("The ammo joined at once"), Count(Index->GetAmmoPickups(), AmmoPickup), 1);
	TestEqual(TEXT("The grenades joined at once"), Count(Index->GetGrenadePickups(), GrenadePickup), 1);
	TestEqual(TEXT("The silver joined at once"), Count(Index->GetSilverPickups(), SilverPickup), 1);
	TestEqual(TEXT("The spider joined at once"), CountCreature(Index->GetCreatures(), Spider), 1);
	TestEqual(TEXT("The sentinel joined at once"), CountCreature(Index->GetCreatures(), Sentinel), 1);
	TestEqual(TEXT("...the two the only new creatures"), Index->GetCreatures().Num(), Creatures + 2);
	TestEqual(TEXT("The player's stand-in joined the characters"), Count(Index->GetCharacters(), Player), 1);
	TestEqual(TEXT("...a creature never does"), Index->GetCharacters().Num(), Characters + 1);
	TestEqual(TEXT("The volume joined at once"), Count(Index->GetVolumes(), Volume), 1);
	TestEqual(TEXT("The playable area joined at once"), Count(Index->GetPlayableAreas(), Area), 1);
	TestTrue(TEXT("The sentinel is known as one (no Cast per frame on the map)"), IsSentinel(Index->GetCreatures(), Sentinel));
	TestFalse(TEXT("...the spider is not"), IsSentinel(Index->GetCreatures(), Spider));
	// Whether a gun lies as loot is for the reader to ask: the list holds the carried ones too.
	TestTrue(TEXT("The dropped gun lies as loot"), Gun->IsPickup());

	// A full look finds the same world: nothing doubled, nothing lost.
	Index->RefreshNow();
	TestEqual(TEXT("A look keeps the gun once"), Count(Index->GetGuns(), Gun), 1);
	TestEqual(TEXT("...and the gun count"), Index->GetGuns().Num(), Guns + 1);
	TestEqual(TEXT("...the ammo"), Count(Index->GetAmmoPickups(), AmmoPickup), 1);
	TestEqual(TEXT("...the ammo count"), Index->GetAmmoPickups().Num(), Ammo + 1);
	TestEqual(TEXT("...the grenades"), Index->GetGrenadePickups().Num(), Grenades + 1);
	TestEqual(TEXT("...the silver"), Index->GetSilverPickups().Num(), Silver + 1);
	TestEqual(TEXT("...the creatures"), Index->GetCreatures().Num(), Creatures + 2);
	TestEqual(TEXT("...the characters"), Index->GetCharacters().Num(), Characters + 1);
	TestEqual(TEXT("...the volumes"), Index->GetVolumes().Num(), Volumes + 1);
	TestEqual(TEXT("...the playable areas"), Index->GetPlayableAreas().Num(), Areas + 1);
	TestTrue(TEXT("...and the sentinel flag"), IsSentinel(Index->GetCreatures(), Sentinel) && !IsSentinel(Index->GetCreatures(), Spider));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActorIndexGoneTest, "Looter.World.ActorIndex.Gone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FActorIndexGoneTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UActorIndexSubsystem* Index = UActorIndexSubsystem::Get(World);
	if (!TestNotNull(TEXT("The preview world keeps the actor index"), Index))
	{
		return false;
	}

	Index->RefreshNow();
	AAmmoPickup* Kept = AAmmoPickup::SpawnAmmo(World, EAmmoType::AssaultRifle, 30, FVector(100.0, 0.0, 20.0));
	AAmmoPickup* Gone = AAmmoPickup::SpawnAmmo(World, EAmmoType::AssaultRifle, 30, FVector(200.0, 0.0, 20.0));
	ASpiderCreature* Spider = World->SpawnActor<ASpiderCreature>(FVector(1500.0, 0.0, 0.0), FRotator::ZeroRotator, AlwaysSpawn());
	if (!TestTrue(TEXT("Two pickups and a spider spawned"), Kept && Gone && Spider))
	{
		return false;
	}
	const int32 AmmoBefore = Index->GetAmmoPickups().Num();
	const int32 CreaturesBefore = Index->GetCreatures().Num();
	TestEqual(TEXT("Both pickups listed"), Count(Index->GetAmmoPickups(), Kept) + Count(Index->GetAmmoPickups(), Gone), 2);

	Gone->Destroy();
	Spider->Destroy();
	// Weak pointers: the destroyed ones are never dereferenced; their entries stay until the next look.
	TestEqual(TEXT("Before the look the ammo list is as long (a null entry)"), Index->GetAmmoPickups().Num(), AmmoBefore);
	TestEqual(TEXT("Before the look the creature list is as long"), Index->GetCreatures().Num(), CreaturesBefore);

	Index->RefreshNow();
	TestEqual(TEXT("After the look the destroyed pickup is gone from the list"), Index->GetAmmoPickups().Num(), AmmoBefore - 1);
	TestEqual(TEXT("...and the other is still there once"), Count(Index->GetAmmoPickups(), Kept), 1);
	TestEqual(TEXT("...the destroyed spider is gone too"), Index->GetCreatures().Num(), CreaturesBefore - 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActorIndexReadsTest, "Looter.World.ActorIndex.Reads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FActorIndexReadsTest::RunTest(const FString& Parameters)
{
	// The rule the index exists for: reading it, however often, never walks the world.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UActorIndexSubsystem* Index = UActorIndexSubsystem::Get(WorldWrapper.GetTestWorld());
	if (!TestNotNull(TEXT("The preview world keeps the actor index"), Index))
	{
		return false;
	}
	TestEqual(TEXT("A world nobody has read has not been looked through"), Index->GetRefreshCount(), 0);

	// A world where play never began (a test's) is looked through by the first read, once.
	Index->GetGuns();
	TestEqual(TEXT("The first read looks once"), Index->GetRefreshCount(), 1);
	for (int32 Read = 0; Read < 100; ++Read)
	{
		Index->GetGuns();
		Index->GetAmmoPickups();
		Index->GetGrenadePickups();
		Index->GetSilverPickups();
		Index->GetShieldPickups();
		Index->GetCreatures();
		Index->GetCharacters();
		Index->GetVolumes();
		Index->GetPlayableAreas();
	}
	TestEqual(TEXT("600 more reads make no look"), Index->GetRefreshCount(), 1);

	// Spawning is no look either: the hook lists the actor.
	WorldWrapper.GetTestWorld()->SpawnActor<ACharacter>(FVector(0.0, 0.0, 100.0), FRotator::ZeroRotator, AlwaysSpawn());
	TestEqual(TEXT("A spawn makes no look"), Index->GetRefreshCount(), 1);

	Index->RefreshNow();
	TestEqual(TEXT("A look is counted"), Index->GetRefreshCount(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActorIndexRateTest, "Looter.World.ActorIndex.Rate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FActorIndexRateTest::RunTest(const FString& Parameters)
{
	// The plain rule: the interval is held between its least and most.
	TestEqual(TEXT("The default interval is a tenth of a second"), UActorIndexSubsystem::DefaultInterval, 0.1f);
	TestEqual(TEXT("A tuned interval in range stands"), UActorIndexSubsystem::ClampInterval(0.25f), 0.25f);
	TestEqual(TEXT("Zero is raised to the least (no look every frame)"), UActorIndexSubsystem::ClampInterval(0.f), UActorIndexSubsystem::MinInterval);
	TestEqual(TEXT("A huge interval is cut to the most"), UActorIndexSubsystem::ClampInterval(500.f), UActorIndexSubsystem::MaxInterval);

	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UActorIndexSubsystem* Index = UActorIndexSubsystem::Get(WorldWrapper.GetTestWorld());
	FIntervalScope Interval;
	if (!TestNotNull(TEXT("The preview world keeps the actor index"), Index) || !TestNotNull(TEXT("Looter.ActorIndex.Interval exists"), Interval.Variable))
	{
		return false;
	}

	// The timer runs at the console variable's interval: 0.1 s by default...
	TestEqual(TEXT("No timer before play begins"), Index->GetTimerRate(), -1.f);
	Interval.Set(UActorIndexSubsystem::DefaultInterval);
	Index->ArmTimer();
	TestEqual(TEXT("Armed, it looks every tenth of a second"), Index->GetTimerRate(), UActorIndexSubsystem::DefaultInterval);

	// ...and a tuned value takes hold at the next look, without a restart.
	Interval.Set(0.25f);
	TestEqual(TEXT("A tuned interval waits for the next look"), Index->GetTimerRate(), UActorIndexSubsystem::DefaultInterval);
	Index->RefreshNow();
	TestEqual(TEXT("...and then the timer runs at it"), Index->GetTimerRate(), 0.25f);

	Interval.Set(0.f);
	Index->RefreshNow();
	TestEqual(TEXT("An interval of zero runs at the least"), Index->GetTimerRate(), UActorIndexSubsystem::MinInterval);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActorIndexGateTest, "Looter.World.ActorIndex.Gate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FActorIndexGateTest::RunTest(const FString& Parameters)
{
	// The loot labels' sleep rule (ULootLabelSubsystem::IsTickable): awake while a label shows or labels' shifts and folds are
	// held to be put back; asleep otherwise, which is nearly all of the game.
	TestFalse(TEXT("No label showing, nothing held: asleep"), ULootLabelSubsystem::NeedsTick(false, false));
	TestTrue(TEXT("A label showing: awake"), ULootLabelSubsystem::NeedsTick(true, false));
	TestTrue(TEXT("The last label just hid with its shift still held: one more tick to put it back"), ULootLabelSubsystem::NeedsTick(false, true));
	TestTrue(TEXT("Showing and holding: awake"), ULootLabelSubsystem::NeedsTick(true, true));
	return true;
}

#endif

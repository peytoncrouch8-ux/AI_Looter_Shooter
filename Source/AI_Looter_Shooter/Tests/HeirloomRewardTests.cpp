#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/LootTossComponent.h"
#include "Loot/LostLootStore.h"
#include "Loot/LostLootSubsystem.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Story/DoorHandoff.h"
#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
#include "Tests/LanternLeansTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponBase.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// The mission rewards that are guns, as they come into the world (MissionRewards::DropInFront), and Heirloom, Main 7's
// gun handed out through Delia's door, given exactly once whatever order the door, a save and the turn-in come in (the
// user's report: it never appeared; it waited unseen at the crack, and the next tap of Interact took it unseen).

using namespace LanternLeansTestWorld;

namespace
{
	const FVector DoorAt(300.0, 0.0, 150.0);
	const FVector HandoffAt(290.0, 45.0, 100.0);
	/** Tilly's window, far from the farm. */
	const FVector TillyAt(-3000.0, 0.0, 90.0);

	void AdvanceBeat(ADoorHandoff& Handoff, float Seconds)
	{
		for (float Done = 0.f; Done < Seconds - KINDA_SMALL_NUMBER; Done += 0.1f)
		{
			Handoff.Advance(FMath::Min(0.1f, Seconds - Done));
		}
	}

	/** Through the save format and back, read as the game reads a session; null when it failed. */
	ULooterSessionSave* WriteAndRead(ULooterSessionSave* Save)
	{
		TArray<uint8> Bytes;
		return UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	}

	/** The gun's toss went straight up: no way on toward the giver. */
	bool TossedStraightUp(const AWeaponBase* Gun)
	{
		const ULootTossComponent* Toss = Gun ? Gun->FindComponentByClass<ULootTossComponent>() : nullptr;
		return Toss && FMath::IsNearlyZero(Toss->Velocity.X, 0.01) && FMath::IsNearlyZero(Toss->Velocity.Y, 0.01) && Toss->Velocity.Z > 0.0;
	}

	/**
	 * Main 7 in a test level: the player's stand-in with every slot full and a backpack (a gun taken goes there, as Heirloom
	 * does in play), Delia's door and the hand-off at it, Main 6 and Main 7 as the scripts make them.
	 */
	struct FMainSevenLevel
	{
		/** Declared first, so it outlives the level: the runner plays against it. */
		FCampaignRecord Campaign;
		FTestWorldWrapper Level;
		UWorld* World = nullptr;
		UMissionRunner* Runner = nullptr;
		AActor* Player = nullptr;
		UWeaponManagerComponent* Inventory = nullptr;
		ASpeakerPoint* Door = nullptr;
		ADoorHandoff* Handoff = nullptr;

		bool Make(const FCampaignRecord& Start)
		{
			Campaign = Start;
			if (!Level.CreateTestWorld(EWorldType::EditorPreview))
			{
				return false;
			}
			World = Level.GetTestWorld();
			Runner = World->GetSubsystem<UMissionRunner>();
			Player = MissionTestWorld::SpawnMarker(World, FVector(0.0, 0.0, 90.0));
			Door = PlaceDeliasDoor(World, DoorAt, 180.0);
			Handoff = PlaceHandoff(World, HandoffAt, 180.0, nullptr);
			if (!Runner || !Player || !Door || !Handoff)
			{
				return false;
			}
			Inventory = NewObject<UWeaponManagerComponent>(Player, TEXT("Weapons"));
			Inventory->RegisterComponent();
			Inventory->MaxWeapons = 0;
			UPackage* Scratch = CreatePackage(nullptr);
			Runner->BeginForTesting({ MakeMainSix(Scratch), MakeMainSeven(Scratch) }, Campaign, Player, Valley);
			Handoff->DispatchBeginPlay();
			return true;
		}

		/** Main 6 done, Main 7 started, and Delia talked to at her door: her step done, the beat begun. */
		void TalkToDelia()
		{
			Runner->Update(0.f);
			Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.MainSixDone")));
			Door->SpeakerPoint->Talk(Player);
		}

		/** At Tilly's window: Main 7 turned in. */
		void TurnIn()
		{
			Player->SetActorLocation(TillyAt);
			Runner->NotifyEvent(FMissionEvent::Named(FMissionEvent::Talk, nullptr, TillysWindow));
		}

		int32 InBackpack(const UNamedWeaponDefinition* Heirloom) const
		{
			return Inventory->GetBackpack().FilterByPredicate([Heirloom](const FWeaponInstanceData& Gun) { return Gun.Named == Heirloom; }).Num();
		}

		/** The Heirlooms in the level: lying as loot (the stand-in has no slots, so none is ever in a hand). */
		TArray<AWeaponBase*> Lying(const UNamedWeaponDefinition* Heirloom) const
		{
			TArray<AWeaponBase*> Found;
			for (TActorIterator<AWeaponBase> It(World); It; ++It)
			{
				if (!It->IsActorBeingDestroyed() && It->GetInstance().Named == Heirloom)
				{
					Found.Add(*It);
				}
			}
			return Found;
		}

		/** On Lost Property's shelves: a gun left on the ground at a save is handed in there as the session is loaded. */
		int32 InLostProperty(const UNamedWeaponDefinition* Heirloom) const
		{
			const ULostLootSubsystem* LostLoot = ULostLootSubsystem::Get(World);
			return LostLoot ? LostLoot->GetStore().Entries.FilterByPredicate([Heirloom](const FLostLootEntry& Entry) { return Entry.Weapon.Named == Heirloom; }).Num()
				: 0;
		}

		/** Every Heirloom the session has here: lying in the level, in the backpack, or in Lost Property. */
		int32 Heirlooms(const UNamedWeaponDefinition* Heirloom) const
		{
			return Lying(Heirloom).Num() + InBackpack(Heirloom) + InLostProperty(Heirloom);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionRewardInFrontTest, "Looter.Missions.RewardDrop.InFront",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionRewardInFrontTest::RunTest(const FString& Parameters)
{
	// A reward gun comes up in front of the player, toward the giver they face, and is tossed up to land as loot, beam and
	// all. Givers stand behind things (Tilly's shop window, Delia's screen door, Amos's fence): it stays on the player's
	// side of them, where it used to come up 1.5 m ahead, inside the shop, never seen.
	UNamedWeaponDefinition* Heirloom = UNamedWeaponDefinition::FindByName(HeirloomId.ToString());
	if (!Heirloom || !Heirloom->Weapon)
	{
		AddWarning(TEXT("DA_Named_Heirloom isn't made (Tools/Unreal/create_named_weapons.py): skipped."));
		return true;
	}
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	// Facing +X, the giver somewhere ahead.
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector(0.0, 0.0, 90.0));
	if (!TestNotNull(TEXT("The player's stand-in"), Player))
	{
		return false;
	}
	const FWeaponInstanceData Rolled = UWeaponRollLibrary::RollWeaponWithRarity(Heirloom->Weapon, EWeaponRarity::Rare, 3);

	// Nothing in the way: 1.5 m ahead over the player's middle, tossed up and on toward the giver.
	AWeaponBase* Open = MissionRewards::DropInFront(World, Rolled, *Player);
	const ULootTossComponent* OpenToss = Open ? Open->FindComponentByClass<ULootTossComponent>() : nullptr;
	TestTrue(TEXT("Open ground: 1.5 m ahead, over the player's middle"), Open && Open->GetActorLocation().Equals(FVector(150.0, 0.0, 150.0), 1.0));
	TestTrue(TEXT("...as loot with its beam up, tossed up and on"), Open && OpenToss && Open->IsPickup() && Open->GetLootBeam()->IsVisible()
		&& OpenToss->Velocity.X > 0.0 && OpenToss->Velocity.Z > 0.0);
	if (Open)
	{
		Open->Destroy();
	}

	// A shop window a metre ahead, its near face at 90 cm: the gun comes up this side of it, and straight up.
	AStaticMeshActor* Window = LocomotionTestWorld::SpawnBlock(World, FVector(100.0, 0.0, 150.0), FVector(20.0, 600.0, 400.0));
	if (!TestNotNull(TEXT("A window across the way"), Window))
	{
		return false;
	}
	AWeaponBase* Near = MissionRewards::DropInFront(World, Rolled, *Player);
	TestTrue(TEXT("A window a metre ahead: the gun comes up this side of it"), Near && Near->GetActorLocation().X > -1.0
		&& Near->GetActorLocation().X < 90.0 - 15.0);
	TestTrue(TEXT("...tossed straight up, never on through it"), TossedStraightUp(Near));
	if (Near)
	{
		Near->Destroy();
	}
	Window->Destroy();

	// A fence 2.5 m ahead (near face at 250 cm), where the toss would have carried it on to: up 1.5 m ahead, straight up.
	AStaticMeshActor* Fence = LocomotionTestWorld::SpawnBlock(World, FVector(260.0, 0.0, 150.0), FVector(20.0, 600.0, 400.0));
	if (!TestNotNull(TEXT("A fence across the way"), Fence))
	{
		return false;
	}
	AWeaponBase* Short = MissionRewards::DropInFront(World, Rolled, *Player);
	TestTrue(TEXT("A fence 2.5 m ahead: up short of it, straight up"), Short && Short->GetActorLocation().X < 250.0 - 15.0 && TossedStraightUp(Short));
	if (Short)
	{
		Short->Destroy();
	}

	// The mission's reward gun and its named gun come the same way.
	FMissionRewards Rewards;
	Rewards.bGun = true;
	Rewards.GunKind = Heirloom->Weapon;
	Rewards.GunRarityFloor = EWeaponRarity::Rare;
	const AWeaponBase* RewardGun = MissionRewards::DropGun(World, Rewards, 3, Player);
	TestTrue(TEXT("The reward gun: this side of the fence, Rare or better"), RewardGun && RewardGun->GetActorLocation().X < 235.0
		&& TossedStraightUp(RewardGun) && RewardGun->GetRarity() >= EWeaponRarity::Rare);
	FMissionRewards Named;
	Named.NamedGun = HeirloomId;
	const AWeaponBase* NamedGun = MissionRewards::DropNamedGun(World, Named, 3, Player);
	TestTrue(TEXT("The named gun: the same"), NamedGun && NamedGun->GetActorLocation().X < 235.0 && TossedStraightUp(NamedGun)
		&& NamedGun->GetInstance().Named == Heirloom);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeirloomOnceTest, "Looter.Story.LanternLeans.HeirloomOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHeirloomOnceTest::RunTest(const FString& Parameters)
{
	// Main 7 turned in gives the player exactly one Heirloom, however far Delia's beat got first (still talking, the gun out
	// on her porch, or taken into the backpack), and whether or not the session was saved and loaded before the turn-in.
	// Nothing is in the world while she talks: nothing to take unseen.
	UNamedWeaponDefinition* Heirloom = UNamedWeaponDefinition::FindByName(HeirloomId.ToString());
	if (!Heirloom)
	{
		AddWarning(TEXT("DA_Named_Heirloom isn't made (Tools/Unreal/create_named_weapons.py): skipped."));
		return true;
	}
	enum class EUntil : uint8 { Words, Out, Taken };
	for (const EUntil Until : { EUntil::Words, EUntil::Out, EUntil::Taken })
	{
		for (const bool bSaved : { false, true })
		{
			const TCHAR* Got = Until == EUntil::Words ? TEXT("While she talks") : Until == EUntil::Out ? TEXT("Out on her porch")
				: TEXT("Taken into the backpack");
			const FString Case = FString::Printf(TEXT("%s, %s"), Got, bSaved ? TEXT("saved and loaded, turned in") : TEXT("turned in"));

			FMainSevenLevel First;
			if (!First.Make(FCampaignRecord()))
			{
				AddError(Case + TEXT(": the test level isn't whole."));
				return false;
			}
			First.TalkToDelia();
			TestTrue(Case + TEXT(": her step done, Main 7 waits for Tilly"), First.Runner->IsReadyToTurnIn(MainSeven)
				&& First.Handoff->GetState() == EDoorHandoffState::Handing);
			TestEqual(Case + TEXT(": nothing in the world while she talks"), First.Heirlooms(Heirloom), 0);
			if (Until == EUntil::Words)
			{
				AdvanceBeat(*First.Handoff, First.Handoff->OpenAfter + 1.f);
				TestEqual(Case + TEXT(": the door open a crack, still nothing out"), First.Heirlooms(Heirloom), 0);
			}
			else
			{
				AdvanceBeat(*First.Handoff, First.Handoff->GunOutAfter + 0.1f);
				TestTrue(Case + TEXT(": out on the porch, one"), First.Handoff->IsGunOut() && First.Heirlooms(Heirloom) == 1);
			}
			if (Until == EUntil::Taken)
			{
				AWeaponBase* Gun = First.Handoff->GetGun();
				TestTrue(Case + TEXT(": taken with a tap"), Gun && First.Inventory->TryPickup(Gun) && First.InBackpack(Heirloom) == 1);
				First.Runner->NotifyEvent(FMissionEvent::Interaction(Gun, /*bHeld*/ false));
			}

			// Saved here and loaded into a fresh level: the campaign, what's carried and what lies on the ground, as a session
			// keeps them.
			FMainSevenLevel Second;
			FMainSevenLevel* Played = &First;
			if (bSaved)
			{
				ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
				Save->Version = ULooterSessionSave::CurrentVersion;
				USessionSubsystem::CaptureWorld(First.World, *Save);
				First.Inventory->SaveInventory(Save->Inventory);
				Save->bHasInventory = true;
				Save->Campaign = First.Campaign;
				ULooterSessionSave* Read = WriteAndRead(Save);
				if (!TestNotNull(Case + TEXT(": saved and read back"), Read) || !Second.Make(Read->Campaign))
				{
					return false;
				}
				Second.Inventory->RestoreInventory(Read->Inventory);
				// The first level's ground, filed under the second's own name (the game's level is the same map both times).
				if (const FSavedMapWorld* Left = Read->FindWorld(USessionSubsystem::MapOf(First.World)))
				{
					const FSavedMapWorld Copy = *Left;
					Read->Worlds.Add(USessionSubsystem::MapOf(Second.World), Copy);
				}
				USessionSubsystem::RestoreWorld(Second.World, *Read);
				Second.Runner->Update(0.f);
				TestTrue(Case + TEXT(": loaded, Main 7 back waiting for Tilly, her door done"), Second.Runner->IsReadyToTurnIn(MainSeven)
					&& Second.Handoff->GetState() == EDoorHandoffState::Done);
				TestEqual(Case + TEXT(": what the session kept"), Second.Heirlooms(Heirloom), Until == EUntil::Words ? 0 : 1);
				if (Until == EUntil::Out)
				{
					// Left on her porch at the save: handed in to Lost Property as the session loaded, not back on the porch.
					TestTrue(Case + TEXT(": handed in to Lost Property, not lying on her porch"), Second.InLostProperty(Heirloom) == 1
						&& Second.Lying(Heirloom).IsEmpty());
				}
				Played = &Second;
			}

			Played->TurnIn();
			TestTrue(Case + TEXT(": Main 7 finished"), Played->Campaign.HasCompleted(MainSeven) && !Played->Runner->IsRunning(MainSeven));
			// Whatever was left of her beat plays out: it never hands a second.
			AdvanceBeat(*Played->Handoff, Played->Handoff->GunOutAfter + Played->Handoff->CloseAfterTaken + Played->Handoff->SwingSeconds + 0.5f);
			TestEqual(Case + TEXT(": exactly one Heirloom"), Played->Heirlooms(Heirloom), 1);
			if (Until == EUntil::Taken)
			{
				TestTrue(Case + TEXT(": the backpack's, none dropped"), Played->InBackpack(Heirloom) == 1 && Played->Lying(Heirloom).IsEmpty());
			}
			else
			{
				const TArray<AWeaponBase*> Given = Played->Lying(Heirloom);
				TestTrue(Case + TEXT(": in front of the player at Tilly's window, as loot, none left at the farm"), Given.Num() == 1
					&& Given[0]->IsPickup() && FVector::Dist2D(Given[0]->GetActorLocation(), TillyAt) < 300.0);
				// One handed in to Lost Property is the very gun brought out (MissionRewards::EnsureNamedGun), never a second.
				TestEqual(Case + TEXT(": none left in Lost Property"), Played->InLostProperty(Heirloom), 0);
				TestTrue(Case + TEXT(": her door shut again, with nothing out"), !Played->Handoff->IsGunOut()
					&& (Played->Handoff->GetState() == EDoorHandoffState::Done || Played->Handoff->GetState() == EDoorHandoffState::Closing));
			}
		}
	}
	return true;
}

#endif

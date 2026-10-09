#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/InteractionComponent.h"
#include "Loot/AmmoPickup.h"
#include "Loot/Chest.h"
#include "Loot/LootTable.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Tests/InteractionTestWorld.h"
#include "Weapons/WeaponBase.h"
#include "World/BreakableProp.h"
#include "World/RespawnMarker.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"

// The lootable world's containers (Docs/Polish/BorderlandsComparison.md, item 10): AChest's coffins pried open and graves
// dug up with a hold, mailboxes and footlockers opened with a tap; what each gives; staying open with the session; and
// where Tools/Unreal/build_area_loot.py put them in the two levels.

namespace
{
	/** The new kinds, with the design's numbers: a hold or a tap, the words, guns, their chance, ammo pickups, the mote. */
	struct FContainerDesign
	{
		EChestKind Kind;
		const TCHAR* Name;
		bool bHold;
		const TCHAR* Words;
		int32 Guns;
		float GunChance;
		int32 Ammo;
		float Mote;
	};
	const FContainerDesign Designs[] = {
		{ EChestKind::Coffin, TEXT("Coffin"), true, TEXT("Pry the coffin open"), 1, 0.15f, 1, 0.2f },
		{ EChestKind::Grave, TEXT("Grave"), true, TEXT("Dig up the grave"), 1, 0.12f, 1, 0.3f },
		{ EChestKind::Mailbox, TEXT("Mailbox"), false, TEXT("Check the mailbox"), 1, 0.08f, 1, 0.f },
		{ EChestKind::Footlocker, TEXT("Footlocker"), false, TEXT("Open the footlocker"), 1, 0.35f, 2, 0.f },
	};

	AChest* SpawnChest(UWorld* World, EChestKind ChestKind, const FVector& Where, double Yaw = 180.0, FName Id = NAME_None)
	{
		const FTransform At(FRotator(0.0, Yaw, 0.0), Where);
		AChest* Chest = World->SpawnActorDeferred<AChest>(AChest::StaticClass(), At);
		if (!Chest)
		{
			return nullptr;
		}
		Chest->Kind = ChestKind;
		Chest->ChestId = Id;
		Chest->FinishSpawning(At);
		Chest->DispatchBeginPlay();
		return Chest;
	}

	void Run(AChest* Chest, float Seconds)
	{
		constexpr float Frame = 1.f / 60.f;
		for (float Done = 0.f; Done < Seconds; Done += Frame)
		{
			Chest->Advance(Frame);
		}
	}

	float OpenSeconds(const AChest* Chest)
	{
		return Chest->GetPreSeconds() + Chest->GetLidSecondsFor() + 0.1f;
	}

	int32 CountAmmo(UWorld* World)
	{
		int32 Ammo = 0;
		for (TActorIterator<AAmmoPickup> It(World); It; ++It)
		{
			Ammo += It->IsActorBeingDestroyed() ? 0 : 1;
		}
		return Ammo;
	}

	int32 CountGuns(UWorld* World)
	{
		int32 Guns = 0;
		for (TActorIterator<AWeaponBase> It(World); It; ++It)
		{
			Guns += It->IsPickup() && !It->IsActorBeingDestroyed() ? 1 : 0;
		}
		return Guns;
	}

	ULooterSessionSave* WriteAndRead(ULooterSessionSave* Save)
	{
		TArray<uint8> Bytes;
		return UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootContainersKindsTest, "Looter.Loot.Containers.Kinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootContainersKindsTest::RunTest(const FString& Parameters)
{
	// Each new kind as designed: a coffin pried and a grave dug with a hold, a mailbox and a footlocker opened with a tap;
	// their guns at their chance (rarely, at a low luck), their ammo pickups of a chest's 36 rounds, a grave's and a
	// coffin's soul-mote now and then; only the treasure (the caches, the strongboxes) on the map. A chest's own override
	// replaces its kind's numbers.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	double Along = 0.0;
	for (const FContainerDesign& Design : Designs)
	{
		const FChestKindInfo Info = FChestKindInfo::Get(Design.Kind);
		TestTrue(FString::Printf(TEXT("%s: %s"), Design.Name, Design.bHold ? TEXT("held to open") : TEXT("a tap")),
			(Info.HoldSeconds > 0.f) == Design.bHold);
		TestEqual(FString::Printf(TEXT("%s: its words"), Design.Name), AChest::DefaultPrompt(Design.Kind).ToString(), FString(Design.Words));
		TestTrue(FString::Printf(TEXT("%s: %d gun(s) at %.0f%%, low luck"), Design.Name, Design.Guns, Design.GunChance * 100.f),
			Info.Guns == Design.Guns && FMath::IsNearlyEqual(Info.GunChance, Design.GunChance) && Info.Luck <= 0.25f);
		TestTrue(FString::Printf(TEXT("%s: %d ammo pickup(s), a soul-mote at %.0f%%"), Design.Name, Design.Ammo, Design.Mote * 100.f),
			Info.AmmoPickups == Design.Ammo && FMath::IsNearlyEqual(Info.MoteChance, Design.Mote));
		TestFalse(FString::Printf(TEXT("%s: not a pin on the map"), Design.Name), Info.bOnMap);
		TestTrue(FString::Printf(TEXT("%s: a name for lists"), Design.Name), AChest::KindName(Design.Kind).ToString() == Design.Name);

		AChest* Chest = SpawnChest(World, Design.Kind, FVector(300.0, Along, 0.0));
		Along += 400.0;
		if (!TestNotNull(FString::Printf(TEXT("A %s"), Design.Name), Chest))
		{
			continue;
		}
		const ULootTable* Table = Chest->MakeLootTable();
		TestTrue(FString::Printf(TEXT("%s: its loot table's guns at its chance"), Design.Name),
			FMath::IsNearlyEqual(Table->WeaponDropChance, Design.GunChance) && Table->MinWeaponDrops == Design.Guns);
		TestTrue(FString::Printf(TEXT("%s: its ammo at a chest's 36 rounds"), Design.Name), Table->MinAmmoDrops == Design.Ammo
			&& Table->AmmoAmountMin == LooterLoot::ChestAmmoAmount && Table->AmmoAmountMax == LooterLoot::ChestAmmoAmount);
	}
	TestTrue(TEXT("The caches and strongboxes are the map's chests"), FChestKindInfo::Get(EChestKind::SupplyCrate).bOnMap
		&& FChestKindInfo::Get(EChestKind::Strongbox).bOnMap);
	TestEqual(TEXT("Every kind is listed"), FChestKindInfo::All().Num(), 6);

	AChest* Saloon = SpawnChest(World, EChestKind::Strongbox, FVector(300.0, Along, 0.0));
	if (TestNotNull(TEXT("A strongbox"), Saloon))
	{
		Saloon->LootOverride.bOverride = true;
		Saloon->LootOverride.Guns = 1;
		Saloon->LootOverride.GunChance = 1.f;
		Saloon->LootOverride.Luck = 0.5f;
		Saloon->LootOverride.AmmoPickups = 2;
		const ULootTable* Table = Saloon->MakeLootTable();
		TestTrue(TEXT("Its own numbers replace its kind's (one gun at Luck 0.5, not two at 1.0)"), Table->MinWeaponDrops == 1
			&& Table->MaxWeaponDrops == 1 && FMath::IsNearlyEqual(Table->Luck, 0.5f) && Table->MinAmmoDrops == 2);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootContainersHoldTest, "Looter.Loot.Containers.Hold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootContainersHoldTest::RunTest(const FString& Parameters)
{
	// A coffin in front of the player: a hold ("Pry the coffin open"); half a hold does nothing, the whole one pries it.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);
	AChest* Coffin = SpawnChest(World, EChestKind::Coffin, FVector(150.0, 0.0, 0.0), 180.0, TEXT("Coffin_Test"));
	if (!TestTrue(TEXT("The player and a coffin"), Player && Interaction && Coffin))
	{
		return false;
	}
	Interaction->UpdateInteraction(0.f);
	if (!TestTrue(TEXT("The coffin is what Interact would use"), Interaction->GetFocusedActor() == Coffin))
	{
		return false;
	}
	const FInteractionOptions& Options = Interaction->GetFocusedOptions();
	TestTrue(TEXT("...held, \"Pry the coffin open\""), Options.bHold && !Options.bTap
		&& FMath::IsNearlyEqual(Options.HoldSeconds, Coffin->GetHoldSeconds()) && Options.HoldPrompt.ToString() == TEXT("Pry the coffin open"));
	Interaction->PressInteract();
	Interaction->UpdateInteraction(Coffin->GetHoldSeconds() * 0.5f);
	TestTrue(TEXT("Half a hold: still shut"), Coffin->GetState() == EChestState::Closed);
	Interaction->UpdateInteraction(Coffin->GetHoldSeconds() * 0.5f + 0.01f);
	TestTrue(TEXT("Held: it's being pried"), Coffin->GetState() == EChestState::Unlocking && Coffin->IsUsedUp());
	Interaction->ReleaseInteract();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootContainersOpeningTest, "Looter.Loot.Containers.Opening",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootContainersOpeningTest::RunTest(const FString& Parameters)
{
	// Each opens as it should. A coffin's lid jumps on its nails, then is shoved back off the box; a grave's mound sinks
	// while the open grave heaves up out of the ground, its spade digging, then its coffin's lid goes onto the heap; a
	// mailbox's door drops forward and down; a footlocker's lid swings up. Each gives its ammo once, its guns at most once,
	// and is still again after.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	double Along = 0.0;
	for (const FContainerDesign& Design : Designs)
	{
		AChest* Chest = SpawnChest(World, Design.Kind, FVector(300.0, Along, 0.0));
		Along += 500.0;
		if (!TestNotNull(FString::Printf(TEXT("A %s"), Design.Name), Chest))
		{
			continue;
		}
		const FChestKindInfo Info = Chest->GetKindInfo();
		const FVector LidShut = Chest->Lid->GetRelativeLocation();
		const int32 AmmoBefore = CountAmmo(World);
		const int32 GunsBefore = CountGuns(World);
		if (Design.Kind == EChestKind::Grave)
		{
			TestTrue(TEXT("Grave: shut, its open grave under the ground and its spade standing by"),
				FMath::IsNearlyEqual(Chest->Body->GetRelativeLocation().Z, -static_cast<double>(Info.BodySink))
				&& Chest->Shovel->GetRelativeLocation().Equals(Info.ShovelBefore.GetLocation(), 0.1));
			TestTrue(TEXT("Grave: nothing on it collides"), Chest->Body->GetCollisionEnabled() == ECollisionEnabled::NoCollision
				&& Chest->Lid->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
			TestTrue(TEXT("Grave: the prompt points over its mound, above the ground"), Chest->GetInteractionLocation().IsSet()
				&& Chest->GetInteractionLocation().GetValue().Z > Chest->GetActorLocation().Z + 10.0);
		}
		if (!TestTrue(FString::Printf(TEXT("%s: opened"), Design.Name), Chest->Open(nullptr)))
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s: ticking while it moves"), Design.Name), Chest->IsActorTickEnabled());
		if (Info.PreMotion == EChestPreMotion::Pry)
		{
			Run(Chest, Chest->GetPreSeconds() * 0.25f);
			TestTrue(FString::Printf(TEXT("%s: pried, the lid jumps on its nails (%.1f cm)"), Design.Name,
				Chest->Lid->GetRelativeLocation().Z - LidShut.Z), Chest->GetState() == EChestState::Unlocking
				&& Chest->Lid->GetRelativeLocation().Z > LidShut.Z + 1.0);
			Run(Chest, Chest->GetPreSeconds() * 0.75f + 0.02f);
		}
		if (Info.PreMotion == EChestPreMotion::Dig)
		{
			// Two fifths in: in the second of the spade's three bites.
			Run(Chest, Chest->GetPreSeconds() * 0.4f);
			TestTrue(FString::Printf(TEXT("%s: being dug, the mound sinking, the grave coming up"), Design.Name),
				Chest->GetState() == EChestState::Unlocking && Chest->Cover->GetRelativeLocation().Z < -1.0
				&& Chest->Body->GetRelativeLocation().Z > -Info.BodySink * 0.6);
			TestFalse(FString::Printf(TEXT("%s: the spade biting, not where it stood"), Design.Name),
				Chest->Shovel->GetRelativeTransform().Equals(Info.ShovelBefore, 0.5));
			Run(Chest, Chest->GetPreSeconds() * 0.6f + 0.02f);
			TestTrue(FString::Printf(TEXT("%s: dug, the grave at the surface, its mound gone"), Design.Name),
				FMath::IsNearlyZero(Chest->Body->GetRelativeLocation().Z, 0.5) && !Chest->Cover->IsVisible());
		}
		TestTrue(FString::Printf(TEXT("%s: the lid moving now"), Design.Name), Chest->GetState() == EChestState::Opening);
		Run(Chest, Chest->GetLidSecondsFor() + 0.05f);
		TestTrue(FString::Printf(TEXT("%s: open, its loot out, still again"), Design.Name), Chest->GetState() == EChestState::Open
			&& Chest->HasGivenLoot() && !Chest->IsActorTickEnabled());
		if (Info.LidMotion == EChestLidMotion::Slide)
		{
			TestTrue(FString::Printf(TEXT("%s: the lid shoved off to where it rests"), Design.Name),
				Chest->Lid->GetRelativeLocation().Equals(LidShut + Info.SlideOffset, 0.5)
				&& Chest->Lid->GetRelativeRotation().Quaternion().Equals(Info.SlideTurn.Quaternion(), 0.01));
		}
		else
		{
			const FVector Up = Chest->Lid->GetRelativeRotation().RotateVector(FVector::UpVector);
			const FVector Front = Chest->Lid->GetRelativeRotation().RotateVector(FVector::ForwardVector);
			TestTrue(FString::Printf(TEXT("%s: the lid at its angle (%.0f degrees)"), Design.Name, Info.OpenAngle),
				FMath::IsNearlyEqual(Chest->GetLidAngle(), Info.OpenAngle, 0.01f));
			if (Info.OpenAngle < 0.f)
			{
				TestTrue(FString::Printf(TEXT("%s: the door dropped forward and down"), Design.Name), Up.X > 0.9 && Front.Z < -0.9);
			}
			else
			{
				TestTrue(FString::Printf(TEXT("%s: the lid swung up and back"), Design.Name), Front.Z > 0.9);
			}
		}
		TestEqual(FString::Printf(TEXT("%s: its ammo pickups came out"), Design.Name), CountAmmo(World) - AmmoBefore, Design.Ammo);
		TestTrue(FString::Printf(TEXT("%s: at most its guns"), Design.Name), CountGuns(World) - GunsBefore <= Design.Guns);
		Run(Chest, 1.f);
		TestEqual(FString::Printf(TEXT("%s: nothing more"), Design.Name), CountAmmo(World) - AmmoBefore, Design.Ammo);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootContainersSaveTest, "Looter.Loot.Containers.Save",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootContainersSaveTest::RunTest(const FString& Parameters)
{
	// A grave dug stays dug: kept by its id as any chest, and put back at the surface, its mound gone, its spade in the
	// heap, its lid off, giving nothing; quietly (no dirt thrown).
	FTestWorldWrapper PlayedLevel;
	FTestWorldWrapper AgainLevel;
	if (!TestTrue(TEXT("Test levels made"), PlayedLevel.CreateTestWorld(EWorldType::EditorPreview) && AgainLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	const FName GraveId(TEXT("Grave_Test_BootHill_1"));
	UWorld* Played = PlayedLevel.GetTestWorld();
	UWorld* Again = AgainLevel.GetTestWorld();
	AChest* Grave = SpawnChest(Played, EChestKind::Grave, FVector(300.0, 0.0, 0.0), 0.0, GraveId);
	if (!TestNotNull(TEXT("A grave"), Grave))
	{
		return false;
	}
	Grave->Open(nullptr);
	Run(Grave, OpenSeconds(Grave));
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	USessionSubsystem::CaptureWorld(Played, *Save);
	ULooterSessionSave* Back = WriteAndRead(Save);
	const FSavedMapWorld* Read = Back ? Back->FindWorld(USessionSubsystem::MapOf(Played)) : nullptr;
	if (!TestTrue(TEXT("Kept by its id, through the save file"), Read && Read->OpenedChests.Contains(GraveId)))
	{
		return false;
	}
	AChest* GraveAgain = SpawnChest(Again, EChestKind::Grave, FVector(300.0, 0.0, 0.0), 0.0, GraveId);
	if (!TestNotNull(TEXT("The grave again, shut"), GraveAgain))
	{
		return false;
	}
	const FSavedMapWorld PlayedWorld = *Read;
	Back->FindOrAddWorld(USessionSubsystem::MapOf(Again)) = PlayedWorld;
	USessionSubsystem::RestoreWorld(Again, *Back);
	const FChestKindInfo Info = GraveAgain->GetKindInfo();
	TestTrue(TEXT("Dug again: open, its loot given"), GraveAgain->GetState() == EChestState::Open && GraveAgain->HasGivenLoot());
	TestTrue(TEXT("...the grave at the surface, its mound gone"), FMath::IsNearlyZero(GraveAgain->Body->GetRelativeLocation().Z, 0.5)
		&& !GraveAgain->Cover->IsVisible());
	TestTrue(TEXT("...its spade in the heap"), GraveAgain->Shovel->GetRelativeLocation().Equals(Info.ShovelAfter.GetLocation(), 0.5));
	TestTrue(TEXT("...its lid off"), GraveAgain->Lid->GetRelativeLocation().Equals(Info.Hinge + Info.SlideOffset, 0.5)
		|| GraveAgain->GetLidShare() >= 1.f);
	TestTrue(TEXT("...giving nothing"), GraveAgain->GetDroppedLoot().IsEmpty());
	GraveAgain->CloseAgain();
	TestTrue(TEXT("Shut again (the console): the grave under the ground, its spade standing by"),
		FMath::IsNearlyEqual(GraveAgain->Body->GetRelativeLocation().Z, -static_cast<double>(Info.BodySink))
		&& GraveAgain->Shovel->GetRelativeLocation().Equals(Info.ShovelBefore.GetLocation(), 0.5));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootContainersLevelsTest, "Looter.Loot.Containers.Levels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootContainersLevelsTest::RunTest(const FString& Parameters)
{
	// Where build_area_loot.py put the lootable world, read from the saved levels: every breakable and chest has its own
	// id (the session keeps them by it); no grave near the family plot's, the story's (Ellis's, Abel's, the ancestors');
	// nothing standing on a respawn grave's spot. A level without them yet is noted, not failed.
	for (const TCHAR* MapPath : { TEXT("/Game/Maps/Lvl_RansomsRest"), TEXT("/Game/Maps/Lvl_TutorialIsland") })
	{
		if (!FPackageName::DoesPackageExist(MapPath))
		{
			AddInfo(FString::Printf(TEXT("%s isn't built: skipped."), MapPath));
			continue;
		}
		const FString Name = FPackageName::GetShortName(MapPath);
		const UWorld* Map = LoadObject<UWorld>(nullptr, *(FString(MapPath) + TEXT(".") + Name));
		const ULevel* Level = Map ? Map->PersistentLevel.Get() : nullptr;
		if (!TestNotNull(FString::Printf(TEXT("%s loads"), *Name), Level))
		{
			continue;
		}
		TSet<FName> Ids;
		int32 Breakables = 0;
		int32 Containers = 0;
		TArray<const AChest*> Graves;
		TArray<const ARespawnMarker*> Markers;
		for (const AActor* Actor : Level->Actors)
		{
			if (const ABreakableProp* Prop = Cast<ABreakableProp>(Actor))
			{
				++Breakables;
				bool bSeen = false;
				Ids.Add(Prop->GetSaveKey(), &bSeen);
				TestFalse(FString::Printf(TEXT("%s: %s's id is its own"), *Name, *Prop->GetSaveKey().ToString()), bSeen);
			}
			else if (const AChest* Chest = Cast<AChest>(Actor))
			{
				Containers += Chest->Kind >= EChestKind::Coffin ? 1 : 0;
				bool bSeen = false;
				Ids.Add(Chest->GetSaveKey(), &bSeen);
				TestFalse(FString::Printf(TEXT("%s: %s's id is its own"), *Name, *Chest->GetSaveKey().ToString()), bSeen);
				if (Chest->Kind == EChestKind::Grave)
				{
					Graves.Add(Chest);
				}
			}
			else if (const ARespawnMarker* Marker = Cast<ARespawnMarker>(Actor))
			{
				Markers.Add(Marker);
			}
		}
		if (Breakables + Containers == 0)
		{
			AddInfo(FString::Printf(TEXT("%s: no lootable world placed yet (run Tools/Unreal/build_area_loot.py)."), *Name));
			continue;
		}
		AddInfo(FString::Printf(TEXT("%s: %d breakables, %d containers (%d graves)."), *Name, Breakables, Containers, Graves.Num()));
		for (const ARespawnMarker* Marker : Markers)
		{
			const FVector At = Marker->GetActorLocation();
			const bool bFamily = Marker->GetMarkerId() == FName(TEXT("FamilyPlot"));
			for (const AChest* Grave : Graves)
			{
				const double Apart = FVector::Dist2D(Grave->GetActorLocation(), At);
				TestTrue(FString::Printf(TEXT("%s: %s %.0f m from %s's respawn grave"), *Name, *Grave->GetSaveKey().ToString(), Apart / 100.0,
					*Marker->GetMarkerId().ToString()), Apart > (bFamily ? 2000.0 : 300.0));
			}
		}
	}
	return true;
}

#endif

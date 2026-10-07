#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/InteractionComponent.h"
#include "Loot/AmmoPickup.h"
#include "Loot/Chest.h"
#include "Loot/LootOdds.h"
#include "Loot/LootTable.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Tests/InteractionTestWorld.h"
#include "Weapons/WeaponBase.h"
#include "World/PlayableArea.h"
#include "World/Windmill.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Rendering/PositionVertexBuffer.h"
#include "StaticMeshResources.h"
#include "Tests/AutomationCommon.h"
#if WITH_EDITOR
#include "StaticMeshCompiler.h"
#endif

// Step 26, the Ranger caches (Docs/Areas/RansomsRest.md, "Loot and chests"): Ruth Calder's three Supply Crates (one gun at
// Luck 0.5 each) and the gang's Strongbox in the sheriff's office (two guns at Luck 1.0), AChest opened once with a tap
// of Interact, kept open by the session, and placed by Tools/Unreal/build_area_caches.py.

namespace
{
	// The ids the build script gives them (build_area_caches.py), which the session keeps them by.
	const FName WindmillId(TEXT("RangerCache_Windmill"));
	const FName SinkRimId(TEXT("RangerCache_SinkRim"));
	const FName BluffPathId(TEXT("RangerCache_BluffPath"));
	const FName StrongboxId(TEXT("GangStrongbox"));

	/** The sheriff's office's floor spot for the Strongbox (FalseFronts.py's walk-in office). */
	const FName StrongboxSocket(TEXT("Strongbox"));

	/** Half the windmill's legs' square at the ground (Windmill.py's BASE_HALF) and half a Supply Crate's depth (cm). */
	constexpr double WindmillLegsHalf = 130.0;
	constexpr double CrateHalfDepth = 26.0;

	/** The design's numbers per kind: guns, luck, and the chance of at least one legendary per chest. */
	struct FChestDesign
	{
		EChestKind Kind;
		const TCHAR* Name;
		int32 Guns;
		float Luck;
		double Legendary;
	};
	const FChestDesign Designs[] = {
		{ EChestKind::SupplyCrate, TEXT("Supply Crate"), 1, 0.5f, 0.037 },
		{ EChestKind::Strongbox, TEXT("Strongbox"), 2, 1.f, 0.155 },
	};

	/** A chest of a kind at Where facing Yaw (180: back toward a stand-in at the origin), as the build script sets it up, begun. */
	AChest* SpawnChest(UWorld* World, EChestKind ChestKind, const FVector& Where, double Yaw = 180.0, FName Id = NAME_None)
	{
		const FTransform At(FRotator(0.0, Yaw, 0.0), Where);
		AChest* Chest = World->SpawnActorDeferred<AChest>(AChest::StaticClass(), At);
		if (!Chest)
		{
			return nullptr;
		}
		// Its kind before it's built, so it takes that kind's models and sockets.
		Chest->Kind = ChestKind;
		Chest->ChestId = Id;
		Chest->FinishSpawning(At);
		Chest->DispatchBeginPlay();
		return Chest;
	}

	/** Moves its opening on for Seconds, a frame at a time (a test level never ticks). */
	void Run(AChest* Chest, float Seconds)
	{
		constexpr float Frame = 1.f / 60.f;
		for (float Done = 0.f; Done < Seconds; Done += Frame)
		{
			Chest->Advance(Frame);
		}
	}

	/** Long enough for any chest to open fully. */
	float OpenSeconds(const AChest* Chest)
	{
		return Chest->WheelSeconds + Chest->LidSeconds + 0.1f;
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

	TArray<AAmmoPickup*> FindAmmo(UWorld* World)
	{
		TArray<AAmmoPickup*> Ammo;
		for (TActorIterator<AAmmoPickup> It(World); It; ++It)
		{
			Ammo.Add(*It);
		}
		return Ammo;
	}

	/** How far its lid stands open, read from the lid itself: the pitch of its front (+X) in the body's frame (degrees). */
	double LidPitch(const AChest* Chest)
	{
		const FVector Front = Chest->Lid->GetRelativeRotation().Vector();
		return FMath::RadiansToDegrees(FMath::Atan2(Front.Z, Front.X));
	}

	/** Through the save format and back, read as the game reads a session; null when it failed. */
	ULooterSessionSave* WriteAndRead(ULooterSessionSave* Save)
	{
		TArray<uint8> Bytes;
		return UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	}

	const ULevel* LoadRansomsRest(FAutomationTestBase& Test)
	{
		if (!FPackageName::DoesPackageExist(TEXT("/Game/Maps/Lvl_RansomsRest")))
		{
			Test.AddInfo(TEXT("Lvl_RansomsRest isn't built: skipped."));
			return nullptr;
		}
		const UWorld* Map = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/Lvl_RansomsRest.Lvl_RansomsRest"));
		return Test.TestNotNull(TEXT("Lvl_RansomsRest loads"), Map) ? Map->PersistentLevel.Get() : nullptr;
	}

	/**
	 * The terrain's height nearest Point's under and over it (cm), from the triangles its Ground tiles draw on Medium, which
	 * are their collision too (a terrain mesh's fallback keeps every triangle). A level loaded only to look at has no
	 * physics to trace, and its actors only their relative transforms. Unset when no tile lies under it, or a tile's
	 * triangles can't be read.
	 */
	TOptional<double> TerrainAt(const ULevel& Level, const FVector& Point)
	{
		const FName GroundTag(TEXT("Ground"));
		TOptional<double> Nearest;
		for (const AActor* Actor : Level.Actors)
		{
			const AStaticMeshActor* Tile = Cast<AStaticMeshActor>(Actor);
			const UStaticMeshComponent* Mesh = Tile && Tile->ActorHasTag(GroundTag) ? Tile->GetStaticMeshComponent() : nullptr;
			UStaticMesh* Asset = Mesh ? Mesh->GetStaticMesh() : nullptr;
			if (!Asset)
			{
				continue;
			}
			const FTransform ToWorld = Mesh->GetRelativeTransform();
			const FBox Box = Asset->GetBoundingBox().TransformBy(ToWorld);
			if (Point.X < Box.Min.X || Point.X > Box.Max.X || Point.Y < Box.Min.Y || Point.Y > Box.Max.Y)
			{
				continue;
			}
#if WITH_EDITOR
			UStaticMesh* const Compiling[] = { Asset };
			FStaticMeshCompilingManager::Get().FinishCompilation(Compiling);
#endif
			const FStaticMeshRenderData* Render = Asset->GetRenderData();
			if (!Render || Render->LODResources.Num() == 0)
			{
				continue;
			}
			const FStaticMeshLODResources& Drawn = Render->LODResources[0];
			const FIndexArrayView Indices = Drawn.IndexBuffer.GetArrayView();
			const FPositionVertexBuffer& Positions = Drawn.VertexBuffers.PositionVertexBuffer;
			if (Indices.Num() < 3 || Positions.GetNumVertices() == 0)
			{
				continue;
			}
			// Straight down through the point, in the tile's own frame (placed level, so still straight down there).
			const FVector Start = ToWorld.InverseTransformPosition(Point + FVector(0.0, 0.0, 500.0));
			const FVector End = ToWorld.InverseTransformPosition(Point - FVector(0.0, 0.0, 500.0));
			for (int32 Index = 0; Index + 2 < Indices.Num(); Index += 3)
			{
				const FVector A(Positions.VertexPosition(Indices[Index]));
				const FVector B(Positions.VertexPosition(Indices[Index + 1]));
				const FVector C(Positions.VertexPosition(Indices[Index + 2]));
				if (Start.X < FMath::Min3(A.X, B.X, C.X) || Start.X > FMath::Max3(A.X, B.X, C.X)
					|| Start.Y < FMath::Min3(A.Y, B.Y, C.Y) || Start.Y > FMath::Max3(A.Y, B.Y, C.Y))
				{
					continue;
				}
				FVector Hit;
				FVector Normal;
				if (FMath::SegmentTriangleIntersection(Start, End, A, B, C, Hit, Normal))
				{
					const double Height = ToWorld.TransformPosition(Hit).Z;
					if (!Nearest.IsSet() || FMath::Abs(Height - Point.Z) < FMath::Abs(Nearest.GetValue() - Point.Z))
					{
						Nearest = Height;
					}
				}
			}
		}
		return Nearest;
	}

	/** The SOCKET_Strongbox of a placed building (the sheriff's office), in the level: unset while no building has one. */
	TOptional<FTransform> StrongboxSpot(const ULevel& Level)
	{
		for (const AActor* Actor : Level.Actors)
		{
			const AStaticMeshActor* Building = Cast<AStaticMeshActor>(Actor);
			const UStaticMeshComponent* Mesh = Building ? Building->GetStaticMeshComponent() : nullptr;
			const UStaticMeshSocket* Socket = Mesh && Mesh->GetStaticMesh() ? Mesh->GetStaticMesh()->FindSocket(StrongboxSocket) : nullptr;
			if (Socket)
			{
				return FTransform(Socket->RelativeRotation, Socket->RelativeLocation, Socket->RelativeScale) * Mesh->GetRelativeTransform();
			}
		}
		return TOptional<FTransform>();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRangerCachesOpeningTest, "Looter.Loot.RangerCaches.Opening",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRangerCachesOpeningTest::RunTest(const FString& Parameters)
{
	// Opened, the Strongbox's wheel spins first; then the lid swings up about its hinge over about a second, easing, to its
	// kind's angle (Chests.py's OPEN_ANGLE: the crate's 112 degrees, the Strongbox's 102), and stops there. It ticks only
	// while it moves.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	TestEqual(TEXT("The crate's lid opens 112 degrees"), FChestKindInfo::Get(EChestKind::SupplyCrate).OpenAngle, 112.f);
	TestEqual(TEXT("The Strongbox's 102"), FChestKindInfo::Get(EChestKind::Strongbox).OpenAngle, 102.f);
	double Along = 0.0;
	for (const FChestDesign& Design : Designs)
	{
		AChest* Chest = SpawnChest(World, Design.Kind, FVector(300.0, Along, 0.0));
		Along += 300.0;
		if (!TestNotNull(FString::Printf(TEXT("A %s"), Design.Name), Chest))
		{
			continue;
		}
		const FChestKindInfo Info = Chest->GetKindInfo();
		TestTrue(FString::Printf(TEXT("%s: shut and still, not ticking"), Design.Name), Chest->GetState() == EChestState::Closed
			&& Chest->GetLidAngle() == 0.f && !Chest->IsActorTickEnabled());
		if (!TestTrue(FString::Printf(TEXT("%s: opened"), Design.Name), Chest->Open(nullptr)))
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s: ticking while it moves"), Design.Name), Chest->IsActorTickEnabled());
		if (Info.WheelTurns > 0.f)
		{
			Run(Chest, Chest->WheelSeconds * 0.5f);
			TestTrue(FString::Printf(TEXT("%s: the wheel turns first (%.0f degrees), the lid still shut"), Design.Name, Chest->GetWheelAngle()),
				Chest->GetState() == EChestState::Unlocking && Chest->GetWheelAngle() > 0.f && Chest->GetLidAngle() == 0.f);
			Run(Chest, Chest->WheelSeconds * 0.5f + 0.02f);
			TestTrue(FString::Printf(TEXT("%s: unlocked, the wheel turned %.1f times"), Design.Name, Info.WheelTurns),
				Chest->GetState() == EChestState::Opening && FMath::IsNearlyEqual(Chest->GetWheelAngle(), Info.WheelTurns * 360.f, 1.f));
		}
		Run(Chest, Chest->LidSeconds * 0.5f);
		TestTrue(FString::Printf(TEXT("%s: halfway through its swing, the lid is partway up (%.0f degrees)"), Design.Name, Chest->GetLidAngle()),
			Chest->GetState() == EChestState::Opening && Chest->GetLidAngle() > Info.OpenAngle * 0.3f && Chest->GetLidAngle() < Info.OpenAngle * 0.7f);
		Run(Chest, Chest->LidSeconds * 0.5f + 0.05f);
		TestTrue(FString::Printf(TEXT("%s: open at its angle (%.2f of %.0f degrees)"), Design.Name, Chest->GetLidAngle(), Info.OpenAngle),
			Chest->GetState() == EChestState::Open && FMath::IsNearlyEqual(Chest->GetLidAngle(), Info.OpenAngle, 0.01f));
		TestTrue(FString::Printf(TEXT("%s: the lid itself pitched up about its hinge to it (%.1f degrees)"), Design.Name, LidPitch(Chest)),
			FMath::IsNearlyEqual(LidPitch(Chest), static_cast<double>(Info.OpenAngle), 0.5));
		TestFalse(FString::Printf(TEXT("%s: still again, no longer ticking"), Design.Name), Chest->IsActorTickEnabled());
		Run(Chest, 1.f);
		TestTrue(FString::Printf(TEXT("%s: and it stays there"), Design.Name), FMath::IsNearlyEqual(Chest->GetLidAngle(), Info.OpenAngle, 0.01f));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRangerCachesLootTest, "Looter.Loot.RangerCaches.Loot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRangerCachesLootTest::RunTest(const FString& Parameters)
{
	// What each kind gives, from the default loot table: a Supply Crate one gun at Luck 0.5 (a legendary 3.7% of the time),
	// the Strongbox two at Luck 1.0 (15.5%), each with its ammo pickups of a chest's fixed 36 rounds; and opened for real,
	// that many guns and pickups come out of it, at the level the area gives the player (1 in a test level).
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	double Along = 0.0;
	for (const FChestDesign& Design : Designs)
	{
		const FChestKindInfo Info = FChestKindInfo::Get(Design.Kind);
		TestTrue(FString::Printf(TEXT("%s: %d gun(s) at Luck %.1f"), Design.Name, Design.Guns, Design.Luck),
			Info.Guns == Design.Guns && FMath::IsNearlyEqual(Info.Luck, Design.Luck));
		AChest* Chest = SpawnChest(World, Design.Kind, FVector(300.0, Along, 0.0));
		Along += 400.0;
		if (!TestNotNull(FString::Printf(TEXT("A %s"), Design.Name), Chest))
		{
			continue;
		}
		const ULootTable* Table = Chest->MakeLootTable();
		TestTrue(FString::Printf(TEXT("%s: every opening gives exactly %d gun(s), at its luck"), Design.Name, Design.Guns),
			Table->WeaponDropChance == 1.f && Table->MinWeaponDrops == Design.Guns && Table->MaxWeaponDrops == Design.Guns
			&& FMath::IsNearlyEqual(Table->Luck, Design.Luck));
		TestTrue(FString::Printf(TEXT("%s: %d ammo pickup(s) of a chest's 36 rounds"), Design.Name, Info.AmmoPickups),
			Table->MinAmmoDrops == Info.AmmoPickups && Table->MaxAmmoDrops == Info.AmmoPickups
			&& Table->AmmoAmountMin == LooterLoot::ChestAmmoAmount && Table->AmmoAmountMax == LooterLoot::ChestAmmoAmount);
		if (Table->Entries.IsEmpty())
		{
			AddWarning(TEXT("The default loot table (DA_LootTable_Default) lists no guns in this checkout: the odds and the drop aren't checked."));
			continue;
		}
		const LootOdds::FExpected Expected = LootOdds::Expected(Table);
		TestTrue(FString::Printf(TEXT("%s: a legendary %.2f%% of the time (design %.1f%%, within a tenth of it)"), Design.Name,
			Expected.LegendaryPerKill * 100.0, Design.Legendary * 100.0), FMath::Abs(Expected.LegendaryPerKill - Design.Legendary) <= Design.Legendary * 0.1);

		// Opened for real.
		const int32 GunsBefore = CountGuns(World);
		const int32 AmmoBefore = FindAmmo(World).Num();
		Chest->Open(nullptr);
		Run(Chest, (Info.WheelTurns > 0.f ? Chest->WheelSeconds : 0.f) + Chest->LidSeconds * Chest->LootShare * 0.5f);
		TestEqual(FString::Printf(TEXT("%s: nothing out while the lid is low"), Design.Name), CountGuns(World), GunsBefore);
		Run(Chest, OpenSeconds(Chest));
		TestEqual(FString::Printf(TEXT("%s: its guns came out of it"), Design.Name), CountGuns(World) - GunsBefore, Design.Guns);
		const TArray<AAmmoPickup*> Ammo = FindAmmo(World);
		TestEqual(FString::Printf(TEXT("%s: its ammo pickups too"), Design.Name), Ammo.Num() - AmmoBefore, Info.AmmoPickups);
		TestFalse(FString::Printf(TEXT("%s: each holding 36 rounds"), Design.Name), Ammo.ContainsByPredicate([](const AAmmoPickup* Pickup)
		{
			return Pickup->GetAmount() != LooterLoot::ChestAmmoAmount;
		}));
		TestEqual(FString::Printf(TEXT("%s: it knows what it gave"), Design.Name), Chest->GetDroppedLoot().Num(), Design.Guns + Info.AmmoPickups);
		TestEqual(FString::Printf(TEXT("%s: at the area's level for the player (none here: 1)"), Design.Name), Chest->GetLootLevel(), 1);
		for (const AActor* Loot : Chest->GetDroppedLoot())
		{
			if (const AWeaponBase* Gun = Cast<AWeaponBase>(Loot))
			{
				TestEqual(FString::Printf(TEXT("%s: %s at that level"), Design.Name, *Gun->GetName()), Gun->GetInstance().Level, Chest->GetLootLevel());
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRangerCachesOnceTest, "Looter.Loot.RangerCaches.OnceOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRangerCachesOnceTest::RunTest(const FString& Parameters)
{
	// A crate in front of the player: the Interact key's prompt ("Open the crate"), a tap opens it; once open it's used up,
	// never focused again, and nothing (a tap, the console's force) gives its loot a second time.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);
	AChest* Crate = SpawnChest(World, EChestKind::SupplyCrate, FVector(150.0, 0.0, 0.0), 180.0, WindmillId);
	if (!TestTrue(TEXT("The player and a crate"), Player && Interaction && Crate))
	{
		return false;
	}
	Interaction->UpdateInteraction(0.f);
	if (!TestTrue(TEXT("The crate in front of the player is what Interact would use"), Interaction->GetFocusedActor() == Crate))
	{
		return false;
	}
	const FInteractionOptions& Options = Interaction->GetFocusedOptions();
	TestTrue(TEXT("...with a tap"), Options.bTap && !Options.bHold);
	TestEqual(TEXT("...\"Open the crate\""), Options.TapPrompt.ToString(), FString(TEXT("Open the crate")));
	TestEqual(TEXT("The Strongbox's words"), AChest::DefaultPrompt(EChestKind::Strongbox).ToString(), FString(TEXT("Open the strongbox")));

	Interaction->PressInteract();
	Interaction->ReleaseInteract();
	TestTrue(TEXT("A tap opens it"), Crate->GetState() != EChestState::Closed);
	TestTrue(TEXT("...used up from then on"), Crate->IsUsedUp());
	Run(Crate, OpenSeconds(Crate));
	const int32 Guns = CountGuns(World);
	const int32 Ammo = FindAmmo(World).Num();
	TestTrue(TEXT("Open, its loot out"), Crate->GetState() == EChestState::Open && Crate->HasGivenLoot());

	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Open, it's never what Interact would use again"), Interaction->GetFocusedActor() != Crate);
	TestFalse(TEXT("...can't be opened by a tap"), Crate->Interact(*Interaction, false));
	TestFalse(TEXT("...nor by the console's force"), Crate->Open(nullptr, /*bForce*/ true));
	Run(Crate, OpenSeconds(Crate));
	TestTrue(TEXT("Nothing more came out of it"), CountGuns(World) == Guns && FindAmmo(World).Num() == Ammo);
	TestFalse(TEXT("Still open, not ticking"), Crate->IsActorTickEnabled());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRangerCachesSaveTest, "Looter.Loot.RangerCaches.Save",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRangerCachesSaveTest::RunTest(const FString& Parameters)
{
	// An opened chest stays open: the session keeps it by its id with the map's world, through the save file, and the level
	// played again has it open and empty, giving nothing (its loot comes back from the save, as any loot lying around).
	// One saved mid-swing, before its loot was out, stays closed and full; one never opened stays shut.
	FTestWorldWrapper PlayedLevel;
	FTestWorldWrapper AgainLevel;
	if (!TestTrue(TEXT("Test levels made"), PlayedLevel.CreateTestWorld(EWorldType::EditorPreview) && AgainLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* Played = PlayedLevel.GetTestWorld();
	UWorld* Again = AgainLevel.GetTestWorld();
	AChest* Opened = SpawnChest(Played, EChestKind::SupplyCrate, FVector(300.0, 0.0, 0.0), 180.0, WindmillId);
	AChest* Untouched = SpawnChest(Played, EChestKind::SupplyCrate, FVector(300.0, 400.0, 0.0), 180.0, SinkRimId);
	AChest* MidSwing = SpawnChest(Played, EChestKind::Strongbox, FVector(300.0, 800.0, 0.0), 180.0, StrongboxId);
	if (!TestTrue(TEXT("The level's chests"), Opened && Untouched && MidSwing))
	{
		return false;
	}
	Opened->Open(nullptr);
	Run(Opened, OpenSeconds(Opened));
	MidSwing->Open(nullptr);
	Run(MidSwing, MidSwing->WheelSeconds * 0.5f);
	const int32 Given = Opened->GetDroppedLoot().Num();

	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	USessionSubsystem::CaptureWorld(Played, *Save);
	const FString PlayedMap = USessionSubsystem::MapOf(Played);
	const FSavedMapWorld* Kept = Save->FindWorld(PlayedMap);
	TestTrue(TEXT("The opened one is kept by its id, and only it (not the one mid-swing)"), Kept && Kept->OpenedChests.Num() == 1
		&& Kept->OpenedChests[0] == WindmillId);
	ULooterSessionSave* Back = WriteAndRead(Save);
	const FSavedMapWorld* Read = Back ? Back->FindWorld(PlayedMap) : nullptr;
	if (!TestTrue(TEXT("Through the save file"), Read && Read->OpenedChests.Contains(WindmillId)))
	{
		return false;
	}
	TestEqual(TEXT("What it gave is kept as loot lying around"), Read->LootWeapons.Num() + Read->AmmoPickups.Num(), Given);

	// The level played again: its chests as built, all shut; the session puts back what was opened. (The second test level
	// stands in for the same map, so its world is filed under that map's name.)
	AChest* OpenedAgain = SpawnChest(Again, EChestKind::SupplyCrate, FVector(300.0, 0.0, 0.0), 180.0, WindmillId);
	AChest* UntouchedAgain = SpawnChest(Again, EChestKind::SupplyCrate, FVector(300.0, 400.0, 0.0), 180.0, SinkRimId);
	AChest* MidSwingAgain = SpawnChest(Again, EChestKind::Strongbox, FVector(300.0, 800.0, 0.0), 180.0, StrongboxId);
	if (!TestTrue(TEXT("The level's chests again, all shut"), OpenedAgain && UntouchedAgain && MidSwingAgain
		&& OpenedAgain->GetState() == EChestState::Closed))
	{
		return false;
	}
	// Copied first: adding the second map's entry may move the first's.
	const FSavedMapWorld PlayedWorld = *Read;
	Back->FindOrAddWorld(USessionSubsystem::MapOf(Again)) = PlayedWorld;
	USessionSubsystem::RestoreWorld(Again, *Back);
	const float Angle = OpenedAgain->GetKindInfo().OpenAngle;
	TestTrue(TEXT("Open again at once: the lid up, still, its loot given"), OpenedAgain->GetState() == EChestState::Open
		&& OpenedAgain->HasGivenLoot() && FMath::IsNearlyEqual(OpenedAgain->GetLidAngle(), Angle, 0.01f) && !OpenedAgain->IsActorTickEnabled());
	TestTrue(TEXT("...the lid itself up"), FMath::IsNearlyEqual(LidPitch(OpenedAgain), static_cast<double>(Angle), 0.5));
	TestEqual(TEXT("...nothing came out of it: the loot here is the save's"), OpenedAgain->GetDroppedLoot().Num(), 0);
	TestEqual(TEXT("...all of it back"), CountGuns(Again) + FindAmmo(Again).Num(), Given);
	TestFalse(TEXT("...and it gives nothing more"), OpenedAgain->Open(nullptr, /*bForce*/ true));
	TestTrue(TEXT("The one never opened is shut and full"), UntouchedAgain->GetState() == EChestState::Closed && UntouchedAgain->CanOpen());
	TestTrue(TEXT("The one saved mid-swing is shut and full: nothing was lost"), MidSwingAgain->GetState() == EChestState::Closed
		&& !MidSwingAgain->HasGivenLoot());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRangerCachesPlacedTest, "Looter.Loot.RangerCaches.Placed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRangerCachesPlacedTest::RunTest(const FString& Parameters)
{
	// Ransom's Rest as built (build_area_caches.py): Ruth's three Supply Crates (under the windmill, on the Sink's rim, on
	// the bluff path) and the gang's Strongbox at the sheriff's office's SOCKET_Strongbox, each inside the playable area,
	// standing on the ground with its pivot (the crates on the terrain, the Strongbox on the office's floor), solid for the
	// minimap and the scatter.
	const ULevel* Level = LoadRansomsRest(*this);
	if (!Level)
	{
		return true;
	}
	const APlayableArea* Area = nullptr;
	const AWindmill* Windmill = nullptr;
	TArray<const AChest*> Crates;
	TArray<const AChest*> Strongboxes;
	for (const AActor* Actor : Level->Actors)
	{
		Area = Area ? Area : Cast<APlayableArea>(Actor);
		Windmill = Windmill ? Windmill : Cast<AWindmill>(Actor);
		if (const AChest* Chest = Cast<AChest>(Actor))
		{
			(Chest->Kind == EChestKind::Strongbox ? Strongboxes : Crates).Add(Chest);
		}
	}
	if (Crates.IsEmpty() && Strongboxes.IsEmpty())
	{
		AddWarning(TEXT("The Ranger caches aren't placed yet: build the C++, import Art/Models/Loot/Chests.py (Tools/models.ps1), then ")
			TEXT("run Tools/Unreal/build_area.py RansomsRest gameplay."));
		return true;
	}
	if (!TestNotNull(TEXT("It has its playable area (Tools/Unreal/build_area_bounds.py)"), Area))
	{
		return false;
	}
	// A level loaded only to look at has no world transforms: its actors' relative ones are.
	const auto Where = [](const AActor* Actor) { return Actor->GetRootComponent()->GetRelativeLocation(); };
	const auto CheckChest = [this, Area, &Where](const AChest* Chest, const FString& What)
	{
		const FVector At = Where(Chest);
		TestTrue(FString::Printf(TEXT("%s at (%.0f, %.0f) is inside the playable area"), *What, At.X, At.Y), Area->Contains(At));
		TestTrue(FString::Printf(TEXT("%s is a chest and solid (tagged %s and Obstacle)"), *What, *AChest::ChestTag.ToString()),
			Chest->ActorHasTag(AChest::ChestTag) && Chest->ActorHasTag(TEXT("Obstacle")));
		TestTrue(FString::Printf(TEXT("%s has its model and lid"), *What), Chest->Body->GetStaticMesh() && Chest->Lid->GetStaticMesh());
		TestTrue(FString::Printf(TEXT("%s opens from the start"), *What), Chest->OpenWhen.IsEmpty());
	};

	TestEqual(TEXT("Ruth's three Supply Crates"), Crates.Num(), 3);
	for (const FName Id : { WindmillId, SinkRimId, BluffPathId })
	{
		const AChest* const* Found = Crates.FindByPredicate([Id](const AChest* Chest) { return Chest->ChestId == Id; });
		if (!TestNotNull(FString::Printf(TEXT("The cache %s"), *Id.ToString()), Found))
		{
			continue;
		}
		const AChest* Crate = *Found;
		const FString What = Id.ToString();
		CheckChest(Crate, What);
		const FVector At = Where(Crate);
		const TOptional<double> Ground = TerrainAt(*Level, At);
		if (!Ground.IsSet())
		{
			AddWarning(FString::Printf(TEXT("%s: the terrain's triangles under it couldn't be read, so its footing isn't checked."), *What));
		}
		else
		{
			TestTrue(FString::Printf(TEXT("%s stands on the terrain (its pivot %.1f cm off it)"), *What, At.Z - Ground.GetValue()),
				FMath::Abs(At.Z - Ground.GetValue()) <= 5.0);
		}
		TestTrue(FString::Printf(TEXT("%s stands level (pitch %.1f, roll %.1f)"), *What, Crate->GetRootComponent()->GetRelativeRotation().Pitch,
			Crate->GetRootComponent()->GetRelativeRotation().Roll), FMath::IsNearlyZero(Crate->GetRootComponent()->GetRelativeRotation().Pitch, 0.1)
			&& FMath::IsNearlyZero(Crate->GetRootComponent()->GetRelativeRotation().Roll, 0.1));
		if (Id == WindmillId && Windmill)
		{
			TestTrue(FString::Printf(TEXT("%s is at the windmill's foot (%.0f cm from it)"), *What, FVector::Dist2D(At, Where(Windmill))),
				FVector::Dist2D(At, Where(Windmill)) <= 300.0);
			// The tower's collision is one hull round its whole lattice, a square 2.6 m across at the ground (Windmill.py): a
			// crate standing in it, or with its back in it, couldn't be reached. Its middle stands half its depth past the
			// square at least.
			const FVector InTower = Windmill->GetRootComponent()->GetRelativeTransform().InverseTransformPosition(At);
			TestTrue(FString::Printf(TEXT("%s stands outside the windmill's legs (%.0f, %.0f in the tower's frame)"), *What, InTower.X, InTower.Y),
				FMath::Max(FMath::Abs(InTower.X), FMath::Abs(InTower.Y)) >= WindmillLegsHalf + CrateHalfDepth);
		}
	}

	// The Strongbox, once the sheriff's office has its walk-in front office (SOCKET_Strongbox on its floor).
	const TOptional<FTransform> Office = StrongboxSpot(*Level);
	if (!Office.IsSet())
	{
		TestEqual(TEXT("No Strongbox without the office's floor to stand on"), Strongboxes.Num(), 0);
		AddWarning(TEXT("The sheriff's office has no SOCKET_Strongbox yet (FalseFronts.py's walk-in office): the gang's Strongbox isn't placed."));
		return true;
	}
	if (TestEqual(TEXT("The gang's Strongbox"), Strongboxes.Num(), 1))
	{
		const AChest* Strongbox = Strongboxes[0];
		CheckChest(Strongbox, TEXT("The Strongbox"));
		TestEqual(TEXT("...its id"), Strongbox->ChestId, StrongboxId);
		const FVector At = Where(Strongbox);
		TestTrue(FString::Printf(TEXT("...on the office's floor at SOCKET_Strongbox (%.1f cm from it)"), FVector::Dist(At, Office->GetLocation())),
			FVector::Dist(At, Office->GetLocation()) <= 2.0);
		TestTrue(TEXT("...facing as the socket does (the door)"), FMath::IsNearlyEqual(FRotator::NormalizeAxis(
			Strongbox->GetRootComponent()->GetRelativeRotation().Yaw - Office->Rotator().Yaw), 0.0, 1.0));
	}
	return true;
}

#endif

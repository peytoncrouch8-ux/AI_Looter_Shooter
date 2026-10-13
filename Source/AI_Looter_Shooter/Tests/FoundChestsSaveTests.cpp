#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Loot/Chest.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "UI/Inventory/MapDiscoverySubsystem.h"
#include "UI/Inventory/MapPins.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"

namespace
{
	const FName FoundId(TEXT("Cache_Found"));
	const FName HiddenId(TEXT("Cache_Hidden"));
	const FName LootedId(TEXT("Cache_Looted"));
	const FName MailboxId(TEXT("Mailbox_Near"));

	ULooterSessionSave* WriteAndRead(ULooterSessionSave* Save)
	{
		TArray<uint8> Bytes;
		return UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	}

	AChest* PlaceChest(UWorld* World, EChestKind Kind, FName Id, const FVector& Where)
	{
		const FTransform At(Where);
		AChest* Chest = World->SpawnActorDeferred<AChest>(AChest::StaticClass(), At);
		if (Chest)
		{
			Chest->Kind = Kind;
			Chest->ChestId = Id;
			Chest->FinishSpawning(At);
		}
		return Chest;
	}

	/** The level's chests as the build script lays them: three Supply Crates (the map shows them) and a mailbox (it never does). */
	struct FCaches
	{
		AChest* Found = nullptr;
		AChest* Hidden = nullptr;
		AChest* Looted = nullptr;
		AChest* Mailbox = nullptr;

		bool IsComplete() const { return Found && Hidden && Looted && Mailbox; }
	};

	FCaches PlaceCaches(UWorld* World)
	{
		FCaches Caches;
		Caches.Found = PlaceChest(World, EChestKind::SupplyCrate, FoundId, FVector(2000.0, 2000.0, 0.0));
		Caches.Hidden = PlaceChest(World, EChestKind::SupplyCrate, HiddenId, FVector(-2000.0, 2000.0, 0.0));
		Caches.Looted = PlaceChest(World, EChestKind::SupplyCrate, LootedId, FVector(2000.0, -2000.0, 0.0));
		Caches.Mailbox = PlaceChest(World, EChestKind::Mailbox, MailboxId, FVector(-2000.0, -2000.0, 0.0));
		return Caches;
	}

	/** The map page's pins of a level with what its discovery has found. */
	TArray<FMapPin> PinsOf(UWorld* World, const UMapDiscoverySubsystem& Discovery)
	{
		FMapPinSources Sources;
		Sources.World = World;
		Sources.FoundChests = &Discovery.GetFoundChests();
		return MapPins::Gather(Sources);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapFoundChestsSavedTest, "Looter.Map.FoundChests.Saved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMapFoundChestsSavedTest::RunTest(const FString& Parameters)
{
	// The chests the player has found but not opened are kept per map in the session save (the user's yes, 2026-10-13), so
	// their pins on the map page survive a Save & Quit and Continue. The opened ones are the session's OpenedChests already
	// (pinned as looted); a chest the map never shows is never kept; a save from before reads as nothing found.
	FTestWorldWrapper PlayedLevel;
	FTestWorldWrapper AgainLevel;
	FTestWorldWrapper OldLevel;
	if (!TestTrue(TEXT("Test levels made"), PlayedLevel.CreateTestWorld(EWorldType::EditorPreview) && AgainLevel.CreateTestWorld(EWorldType::EditorPreview)
		&& OldLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* Played = PlayedLevel.GetTestWorld();
	UWorld* Again = AgainLevel.GetTestWorld();
	UWorld* Old = OldLevel.GetTestWorld();
	const FCaches PlayedCaches = PlaceCaches(Played);
	UMapDiscoverySubsystem* PlayedDiscovery = Played->GetSubsystem<UMapDiscoverySubsystem>();
	if (!TestTrue(TEXT("The level's chests"), PlayedCaches.IsComplete()) || !TestNotNull(TEXT("The map's discovery"), PlayedDiscovery))
	{
		return false;
	}

	// Played: one crate found, one found and then opened, a mailbox seen, one crate never come near.
	PlayedCaches.Looted->RestoreOpened();
	PlayedDiscovery->MarkChestFound(FoundId);
	PlayedDiscovery->MarkChestFound(LootedId);
	PlayedDiscovery->MarkChestFound(MailboxId);
	TestTrue(TEXT("The finds so far: three"), PlayedDiscovery->GetFoundChests().Num() == 3);
	const TArray<FName> Unopened = PlayedDiscovery->GetUnopenedFound();
	TestTrue(TEXT("Of them the session keeps one: the crate found and not opened"), Unopened.Num() == 1 && Unopened[0] == FoundId);

	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	USessionSubsystem::CaptureWorld(Played, *Save);
	const FString PlayedMap = USessionSubsystem::MapOf(Played);
	const FSavedMapWorld* Kept = Save->FindWorld(PlayedMap);
	TestTrue(TEXT("The save has the found crate, and only it"), Kept && Kept->FoundChests.Num() == 1 && Kept->FoundChests[0] == FoundId);
	TestTrue(TEXT("...the opened one is the save's opened chest instead, not a found one"), Kept && Kept->OpenedChests.Num() == 1
		&& Kept->OpenedChests[0] == LootedId);
	ULooterSessionSave* Back = WriteAndRead(Save);
	const FSavedMapWorld* Read = Back ? Back->FindWorld(PlayedMap) : nullptr;
	if (!TestTrue(TEXT("Through the save file"), Read && Read->FoundChests.Num() == 1 && Read->FoundChests[0] == FoundId))
	{
		return false;
	}

	// Continue: the level starts again with nothing found and every chest shut, and no pin but the map's own.
	const FCaches AgainCaches = PlaceCaches(Again);
	UMapDiscoverySubsystem* AgainDiscovery = Again->GetSubsystem<UMapDiscoverySubsystem>();
	if (!TestTrue(TEXT("The level's chests again"), AgainCaches.IsComplete()) || !TestNotNull(TEXT("A new discovery"), AgainDiscovery))
	{
		return false;
	}
	TestTrue(TEXT("A new visit has found nothing"), AgainDiscovery->GetFoundChests().IsEmpty());
	const TArray<FMapPin> FreshPins = PinsOf(Again, *AgainDiscovery);
	TestEqual(TEXT("...so the map has no chest pin"), MapPins::Count(FreshPins, EMapPinKind::Chest), 0);

	// The second test level stands in for the same map, so its world is filed under that map's name (copied first: adding the
	// second map's entry may move the first's).
	const FSavedMapWorld PlayedWorld = *Read;
	Back->FindOrAddWorld(USessionSubsystem::MapOf(Again)) = PlayedWorld;
	USessionSubsystem::RestoreWorld(Again, *Back);
	TestTrue(TEXT("Put back: the found crate is found again, and nothing else is"), AgainDiscovery->GetFoundChests().Num() == 1
		&& AgainDiscovery->GetFoundChests().Contains(FoundId));
	const TArray<FMapPin> AgainPins = PinsOf(Again, *AgainDiscovery);
	const FMapPin* FoundPin = AgainPins.FindByPredicate([](const FMapPin& Pin) { return Pin.Kind == EMapPinKind::Chest; });
	TestTrue(TEXT("...its pin is on the map page, not yet opened"), MapPins::Count(AgainPins, EMapPinKind::Chest) == 1 && FoundPin && FoundPin->Id == FoundId
		&& FoundPin->Location.Equals(AgainCaches.Found->GetActorLocation()));
	TestEqual(TEXT("...the opened crate still there, as looted"), MapPins::Count(AgainPins, EMapPinKind::ChestLooted), 1);
	TestTrue(TEXT("...the one never found stays off, and so does the mailbox"), AgainCaches.Hidden->GetState() == EChestState::Closed
		&& !AgainDiscovery->GetFoundChests().Contains(HiddenId) && !AgainDiscovery->GetFoundChests().Contains(MailboxId));

	// Saved again from the level played again (a second Save & Quit): still kept; then opened, it moves to the opened chests.
	ULooterSessionSave* Next = NewObject<ULooterSessionSave>();
	Next->Version = ULooterSessionSave::CurrentVersion;
	USessionSubsystem::CaptureWorld(Again, *Next);
	const FSavedMapWorld* NextKept = Next->FindWorld(USessionSubsystem::MapOf(Again));
	TestTrue(TEXT("A second Save & Quit keeps it"), NextKept && NextKept->FoundChests.Num() == 1 && NextKept->FoundChests[0] == FoundId);
	AgainCaches.Found->RestoreOpened();
	USessionSubsystem::CaptureWorld(Again, *Next);
	NextKept = Next->FindWorld(USessionSubsystem::MapOf(Again));
	TestTrue(TEXT("Opened, it's no longer a found chest but an opened one"), NextKept && NextKept->FoundChests.IsEmpty()
		&& NextKept->OpenedChests.Contains(FoundId) && NextKept->OpenedChests.Contains(LootedId));

	// A save from before: its map world has no finds, so a level put back from it has found nothing; what is found by
	// itself is never lost to a restore (the session adds to it).
	TestTrue(TEXT("Before the finds were kept: none"), FSavedMapWorld().FoundChests.IsEmpty());
	const FCaches OldCaches = PlaceCaches(Old);
	UMapDiscoverySubsystem* OldDiscovery = Old->GetSubsystem<UMapDiscoverySubsystem>();
	if (!TestTrue(TEXT("A level for an older save"), OldCaches.IsComplete()) || !TestNotNull(TEXT("Its discovery"), OldDiscovery))
	{
		return false;
	}
	ULooterSessionSave* OldSave = NewObject<ULooterSessionSave>();
	OldSave->Version = ULooterSessionSave::CurrentVersion;
	OldSave->FindOrAddWorld(USessionSubsystem::MapOf(Old));
	USessionSubsystem::RestoreWorld(Old, *OldSave);
	TestTrue(TEXT("An older save puts back nothing found"), OldDiscovery->GetFoundChests().IsEmpty());
	const TArray<FMapPin> OldPins = PinsOf(Old, *OldDiscovery);
	TestEqual(TEXT("...no chest pin"), MapPins::Count(OldPins, EMapPinKind::Chest), 0);
	OldDiscovery->MarkChestFound(HiddenId);
	TArray<FName> Restored;
	Restored.Add(FoundId);
	Restored.Add(FName(NAME_None));
	OldDiscovery->RestoreFound(Restored);
	TestTrue(TEXT("Restoring adds to what is found: both crates, and no unnamed one"), OldDiscovery->GetFoundChests().Num() == 2
		&& OldDiscovery->GetFoundChests().Contains(HiddenId) && OldDiscovery->GetFoundChests().Contains(FoundId));
	return true;
}

#endif

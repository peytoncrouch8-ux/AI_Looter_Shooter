#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureBase.h"
#include "Creatures/SpiderCreature.h"
#include "Loot/Chest.h"
#include "Session/CampaignRecord.h"
#include "UI/Inventory/MapPins.h"
#include "UI/Inventory/MapView.h"
#include "UI/Inventory/MapWidget.h"
#include "World/GraveTravelRules.h"
#include "World/GraveTravelSubsystem.h"
#include "World/GunsmithBench.h"
#include "World/RespawnMarker.h"
#include "World/TrainStation.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "InputCoreTypes.h"
#include "Tests/AutomationCommon.h"

namespace
{
	/** A respawn grave in a test level, named and opened (or not) before its play begins. */
	ARespawnMarker* PlaceMapGrave(UWorld* World, FName Id, const FVector& Where, double Yaw, bool bOpenFromStart, FName OpenedBy = NAME_None)
	{
		const FTransform At(FRotator(0.0, Yaw, 0.0), Where);
		ARespawnMarker* Grave = World->SpawnActorDeferred<ARespawnMarker>(ARespawnMarker::StaticClass(), At);
		if (Grave)
		{
			Grave->MarkerId = Id;
			Grave->bStartActive = bOpenFromStart;
			Grave->ActiveAfterMission = OpenedBy;
			Grave->FinishSpawning(At);
		}
		return Grave;
	}

	/** A Supply Crate in a test level with its id, as the build script sets one up. */
	AChest* PlaceMapChest(UWorld* World, FName Id, const FVector& Where)
	{
		const FTransform At(Where);
		AChest* Chest = World->SpawnActorDeferred<AChest>(AChest::StaticClass(), At);
		if (Chest)
		{
			Chest->Kind = EChestKind::SupplyCrate;
			Chest->ChestId = Id;
			Chest->FinishSpawning(At);
		}
		return Chest;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapViewTest, "Looter.UI.Map.View",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMapViewTest::RunTest(const FString& Parameters)
{
	// The map page's view as plain math: zoom 1 fits the whole map, a zoom keeps the point under the pointer, the map never
	// slides off where it could fill the frame, a box fits the frame, the grid's step follows the zoom, and the D-pad's pick.
	FMapView View;
	View.Frame = FVector2D(1000.0, 700.0);
	View.Clamp();
	TestTrue(TEXT("Zoom 1 shows the whole map, centred"), View.Zoom == 1.0 && View.Center.Equals(FVector2D(0.5, 0.5)));
	TestEqual(TEXT("...its square fits the frame's shorter side"), View.FitSide(), 700.0);
	TestTrue(TEXT("The map's middle in the frame's middle"), View.UVToFrame(FVector2D(0.5, 0.5)).Equals(FVector2D(500.0, 350.0), 1e-6));
	const FVector2D Somewhere(0.3, 0.7);
	TestTrue(TEXT("Frame and map points convert both ways"), View.FrameToUV(View.UVToFrame(Somewhere)).Equals(Somewhere, 1e-9));
	View.Pan(FVector2D(300.0, -200.0));
	TestTrue(TEXT("Zoom 1 can't pan: the map stays centred"), View.Center.Equals(FVector2D(0.5, 0.5), 1e-9));

	// Zoomed about a point, the map point under it stays under it.
	const FVector2D Pointer(600.0, 300.0);
	const FVector2D Under = View.FrameToUV(Pointer);
	View.ZoomAt(Pointer, 2.0);
	TestEqual(TEXT("The wheel zooms by its factor"), View.Zoom, 2.0);
	TestTrue(TEXT("...about the pointer"), View.FrameToUV(Pointer).Equals(Under, 1e-6));

	// Dragged far east, the map stops with its west edge on the frame's left.
	View.Pan(FVector2D(100000.0, 0.0));
	TestTrue(TEXT("The map's edge stops at the frame's"), FMath::IsNearlyEqual(View.UVToFrame(FVector2D(0.0, 0.5)).X, 0.0, 1e-6));
	View.ZoomAt(View.Frame * 0.5, 100.0);
	TestEqual(TEXT("Zoom stops at its most"), View.Zoom, FMapView::MaxZoom);
	View.ZoomAt(View.Frame * 0.5, 0.0001);
	TestTrue(TEXT("...and at the whole map, centred again"), View.Zoom == FMapView::MinZoom && View.Center.Equals(FVector2D(0.5, 0.5), 1e-9));

	// A box of the map fills the frame along its longer side (in the frame's proportions), centred.
	View.Fit(FBox2D(FVector2D(0.25, 0.25), FVector2D(0.75, 0.5)), 0.0);
	TestTrue(TEXT("A box is fitted: its middle in the frame's middle"), View.UVToFrame(FVector2D(0.5, 0.375)).Equals(View.Frame * 0.5, 1e-6));
	TestTrue(TEXT("...its width across the frame"), FMath::IsNearlyEqual(View.UVToFrame(FVector2D(0.25, 0.375)).X, 0.0, 1e-6)
		&& FMath::IsNearlyEqual(View.UVToFrame(FVector2D(0.75, 0.375)).X, View.Frame.X, 1e-6));

	// Zoomed in, the grid's lines come closer in the world; never closer than asked on the page.
	View = FMapView();
	View.Frame = FVector2D(1000.0, 700.0);
	const double MapCm = 40000.0;
	const double Far = View.GridStep(MapCm, 90.0);
	View.Zoom = 4.0;
	const double Near = View.GridStep(MapCm, 90.0);
	TestTrue(TEXT("The grid is finer zoomed in"), Near < Far);
	TestTrue(TEXT("...its lines at least 90 page units apart"), Near * View.Side() / MapCm >= 90.0);

	// The D-pad's pick: ahead within 60 degrees, the nearer and straighter first, the chosen one skipped.
	const TArray<FVector2D> Points = { FVector2D(100.0, 0.0), FVector2D(0.0, 100.0), FVector2D(-100.0, 0.0), FVector2D(200.0, 10.0) };
	TestEqual(TEXT("Right: the nearest straight ahead"), FMapView::PickInDirection(Points, FVector2D::ZeroVector, FVector2D(1.0, 0.0)), 0);
	TestEqual(TEXT("Down: the one below"), FMapView::PickInDirection(Points, FVector2D::ZeroVector, FVector2D(0.0, 1.0)), 1);
	TestEqual(TEXT("Up: nothing that way"), FMapView::PickInDirection(Points, FVector2D::ZeroVector, FVector2D(0.0, -1.0)), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("Right again, the chosen one skipped: the farther one"), FMapView::PickInDirection(Points, FVector2D::ZeroVector, FVector2D(1.0, 0.0), 0), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapTravelRulesTest, "Looter.UI.Map.TravelRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMapTravelRulesTest::RunTest(const FString& Parameters)
{
	// When fast travel may go: never without a living player, in a scene, a boss's fight or while anything hunts the player,
	// nor while a trip is under way; then the grave itself (open, and not where the player stands). Each block has words.
	using namespace GraveTravelRules;
	FGraveTravelSenses Clear;
	Clear.bHasPlayer = true;
	TestTrue(TEXT("Nobody to send"), CheckNow(FGraveTravelSenses()) == EGraveTravelBlock::NoPlayer);
	TestTrue(TEXT("Calm: it may go"), CheckNow(Clear) == EGraveTravelBlock::None);

	auto With = [&Clear](TFunctionRef<void(FGraveTravelSenses&)> Change)
	{
		FGraveTravelSenses Senses = Clear;
		Change(Senses);
		return CheckNow(Senses);
	};
	TestTrue(TEXT("Down: not now"), With([](FGraveTravelSenses& S) { S.bDying = true; }) == EGraveTravelBlock::Dying);
	TestTrue(TEXT("In a scene: not now"), With([](FGraveTravelSenses& S) { S.bInScene = true; }) == EGraveTravelBlock::Scene);
	TestTrue(TEXT("In a boss fight: not now"), With([](FGraveTravelSenses& S) { S.bBossFight = true; }) == EGraveTravelBlock::BossFight);
	TestTrue(TEXT("Hunted: not now"), With([](FGraveTravelSenses& S) { S.Hunters = 2; }) == EGraveTravelBlock::Hunted);
	TestTrue(TEXT("On the way already: not again"), With([](FGraveTravelSenses& S) { S.bTravelling = true; }) == EGraveTravelBlock::Travelling);
	TestTrue(TEXT("Down outranks hunted"), With([](FGraveTravelSenses& S) { S.bDying = true; S.Hunters = 1; }) == EGraveTravelBlock::Dying);
	TestTrue(TEXT("A scene outranks the fight it shows"), With([](FGraveTravelSenses& S) { S.bInScene = true; S.bBossFight = true; S.Hunters = 3; })
		== EGraveTravelBlock::Scene);
	TestTrue(TEXT("A boss fight is named before its hunters"), With([](FGraveTravelSenses& S) { S.bBossFight = true; S.Hunters = 3; })
		== EGraveTravelBlock::BossFight);

	FGraveTravelSenses Hunted = Clear;
	Hunted.Hunters = 1;
	TestTrue(TEXT("A closed grave: no"), CheckGrave(Clear, false, 5000.0) == EGraveTravelBlock::GraveClosed);
	TestTrue(TEXT("Standing at it: no"), CheckGrave(Clear, true, AlreadyThereDistance * 0.5) == EGraveTravelBlock::AlreadyThere);
	TestTrue(TEXT("An open grave away: yes"), CheckGrave(Clear, true, 5000.0) == EGraveTravelBlock::None);
	TestTrue(TEXT("The fight comes before the grave"), CheckGrave(Hunted, false, 0.0) == EGraveTravelBlock::Hunted);

	TestTrue(TEXT("No words when it can go"), Reason(EGraveTravelBlock::None).IsEmpty());
	TSet<FString> Words;
	const EGraveTravelBlock Blocks[] = { EGraveTravelBlock::NoPlayer, EGraveTravelBlock::Dying, EGraveTravelBlock::Scene, EGraveTravelBlock::BossFight,
		EGraveTravelBlock::Hunted, EGraveTravelBlock::Travelling, EGraveTravelBlock::GraveClosed, EGraveTravelBlock::AlreadyThere };
	for (const EGraveTravelBlock Block : Blocks)
	{
		Words.Add(Reason(Block).ToString());
	}
	TestTrue(TEXT("Every block says why, each its own way"), Words.Num() == static_cast<int32>(UE_ARRAY_COUNT(Blocks)) && !Words.Contains(FString()));
	TestTrue(TEXT("The fight's says hunted"), Reason(EGraveTravelBlock::Hunted).ToString().Contains(TEXT("hunting")));

	TestTrue(TEXT("The player stands on the grave's spot"), ArrivalLocation(FVector(100.0, 200.0, 300.0), 88.f).Equals(FVector(100.0, 200.0, 388.0)));
	TestTrue(TEXT("The fade is short: under two seconds in all"), FadeOutSeconds > 0.f && FadeInSeconds > 0.f
		&& FadeOutSeconds + HoldSeconds + FadeInSeconds < 2.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapPinsTest, "Looter.UI.Map.Pins",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMapPinsTest::RunTest(const FString& Parameters)
{
	// The map's pins from a level: the open graves only (from the start, or by the story's record), the station and the
	// bench, chests once found or opened; drawn in order, the story's pins on top; graves named by their id in words.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ARespawnMarker* BootHill = PlaceMapGrave(World, TEXT("BootHill"), FVector(3000.0, 0.0, 0.0), 0.0, true);
	ARespawnMarker* ChapelYard = PlaceMapGrave(World, TEXT("ChapelYard"), FVector(6000.0, -2000.0, 0.0), 0.0, false, TEXT("TestMain4"));
	ARespawnMarker* FamilyPlot = PlaceMapGrave(World, TEXT("FamilyPlot"), FVector(-3000.0, -9000.0, 0.0), 0.0, false, TEXT("TestMain1"));
	AGunsmithBench* Bench = World->SpawnActor<AGunsmithBench>(FVector(1000.0, 0.0, 0.0), FRotator::ZeroRotator);
	ATrainStation* Station = World->SpawnActor<ATrainStation>(FVector(1500.0, 11000.0, 0.0), FRotator::ZeroRotator);
	AChest* Found = PlaceMapChest(World, TEXT("Cache_Found"), FVector(2000.0, 2000.0, 0.0));
	AChest* Hidden = PlaceMapChest(World, TEXT("Cache_Hidden"), FVector(-2000.0, 2000.0, 0.0));
	AChest* Looted = PlaceMapChest(World, TEXT("Cache_Looted"), FVector(2000.0, -2000.0, 0.0));
	if (!TestTrue(TEXT("Everything placed"), BootHill && ChapelYard && FamilyPlot && Bench && Station && Found && Hidden && Looted))
	{
		return false;
	}
	Looted->RestoreOpened();
	ChapelYard->DisplayName = FText::FromString(TEXT("The chapel yard"));

	FCampaignRecord Campaign;
	Campaign.ActivateRespawn(TEXT("ChapelYard"));
	const TSet<FName> FoundChests = { TEXT("Cache_Found") };
	FMapPinSources Sources;
	Sources.World = World;
	Sources.Campaign = &Campaign;
	Sources.FoundChests = &FoundChests;
	const TArray<FMapPin> Pins = MapPins::Gather(Sources);

	auto Find = [&Pins](EMapPinKind Kind, FName Id) { return Pins.FindByPredicate([Kind, Id](const FMapPin& Pin) { return Pin.Kind == Kind && Pin.Id == Id; }); };
	TestEqual(TEXT("Two graves open"), MapPins::Count(Pins, EMapPinKind::Grave), 2);
	TestTrue(TEXT("...boot hill, open from the start"), Find(EMapPinKind::Grave, TEXT("BootHill")) != nullptr);
	TestTrue(TEXT("...the chapel yard, opened in the record"), Find(EMapPinKind::Grave, TEXT("ChapelYard")) != nullptr);
	TestNull(TEXT("...not the family plot, still closed"), Find(EMapPinKind::Grave, TEXT("FamilyPlot")));
	const FMapPin* BootHillPin = Find(EMapPinKind::Grave, TEXT("BootHill"));
	TestTrue(TEXT("A grave without a name is its id in words"), BootHillPin && BootHillPin->Name.ToString() == TEXT("Boot Hill"));
	const FMapPin* ChapelPin = Find(EMapPinKind::Grave, TEXT("ChapelYard"));
	TestTrue(TEXT("...a named one by its name"), ChapelPin && ChapelPin->Name.ToString() == TEXT("The chapel yard"));
	TestTrue(TEXT("...and it knows its grave"), ChapelPin && ChapelPin->Grave.Get() == ChapelYard);
	TestEqual(TEXT("The bench"), MapPins::Count(Pins, EMapPinKind::Bench), 1);
	TestEqual(TEXT("The station"), MapPins::Count(Pins, EMapPinKind::Station), 1);
	TestTrue(TEXT("The chest found"), Find(EMapPinKind::Chest, TEXT("Cache_Found")) != nullptr);
	TestTrue(TEXT("The chest opened, dim"), Find(EMapPinKind::ChestLooted, TEXT("Cache_Looted")) != nullptr);
	TestTrue(TEXT("Not the chest nobody has come across"), !Find(EMapPinKind::Chest, TEXT("Cache_Hidden")) && !Find(EMapPinKind::ChestLooted, TEXT("Cache_Hidden")));
	bool bInOrder = true;
	for (int32 Index = 1; Index < Pins.Num(); ++Index)
	{
		bInOrder &= static_cast<uint8>(Pins[Index - 1].Kind) <= static_cast<uint8>(Pins[Index].Kind);
	}
	TestTrue(TEXT("Pins in drawing order: chests under, graves over"), bInOrder);

	// Without the story's record, only the graves open from the start; without the finds, only the opened chests.
	FMapPinSources Bare;
	Bare.World = World;
	const TArray<FMapPin> BarePins = MapPins::Gather(Bare);
	TestEqual(TEXT("No story: only the grave open from the start"), MapPins::Count(BarePins, EMapPinKind::Grave), 1);
	TestEqual(TEXT("No finds: no closed chest"), MapPins::Count(BarePins, EMapPinKind::Chest), 0);
	TestEqual(TEXT("...the opened one still there"), MapPins::Count(BarePins, EMapPinKind::ChestLooted), 1);
	TestFalse(TEXT("Every kind has a legend name"), MapPins::KindName(EMapPinKind::Grave).IsEmpty() || MapPins::KindName(EMapPinKind::TurnIn).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapTravelTest, "Looter.UI.Map.Travel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMapTravelTest::RunTest(const FString& Parameters)
{
	// Fast travel in a test level: the rules read from the world (a grave closed, one where the player stands, a spider
	// hunting the player), then a travel run through its fade: the player ends standing on the grave, facing its arrow.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UGraveTravelSubsystem* Travel = World->GetSubsystem<UGraveTravelSubsystem>();
	ACharacter* Player = World->SpawnActor<ACharacter>(FVector(0.0, 0.0, 100.0), FRotator::ZeroRotator);
	ARespawnMarker* Far = PlaceMapGrave(World, TEXT("BootHill"), FVector(5000.0, 0.0, 0.0), 90.0, true);
	ARespawnMarker* Here = PlaceMapGrave(World, TEXT("Here"), FVector(300.0, 0.0, 0.0), 0.0, true);
	ARespawnMarker* Closed = PlaceMapGrave(World, TEXT("Closed"), FVector(0.0, 6000.0, 0.0), 0.0, false, TEXT("TestNeverDone"));
	if (!TestTrue(TEXT("The travel, the player and the graves"), Travel && Player && Far && Here && Closed))
	{
		return false;
	}

	TestTrue(TEXT("Calm: the far grave can be travelled to"), Travel->CheckGrave(Player, *Far) == EGraveTravelBlock::None);
	TestTrue(TEXT("Not the one the player stands at"), Travel->CheckGrave(Player, *Here) == EGraveTravelBlock::AlreadyThere);
	TestTrue(TEXT("Not a closed one"), Travel->CheckGrave(Player, *Closed) == EGraveTravelBlock::GraveClosed);
	TestTrue(TEXT("Nobody to send without a player"), Travel->CheckNow(nullptr) == EGraveTravelBlock::NoPlayer);

	// A spider comes for the player: no travel until it's off them.
	ASpiderCreature* Spider = World->SpawnActor<ASpiderCreature>(FVector(1500.0, 0.0, 0.0), FRotator::ZeroRotator);
	if (TestNotNull(TEXT("A spider"), Spider))
	{
		Spider->DevPutInState(ECreatureState::Chase, Player);
		TestEqual(TEXT("...hunting the player"), Travel->Sense(Player).Hunters, 1);
		TestTrue(TEXT("...so no travel"), Travel->CheckGrave(Player, *Far) == EGraveTravelBlock::Hunted);
		Spider->DevPutInState(ECreatureState::Idle);
		TestTrue(TEXT("Off the player: travel again"), Travel->CheckGrave(Player, *Far) == EGraveTravelBlock::None);
	}

	// Refused: nothing starts. Allowed: the fade runs, a second travel waits, and in the black the player is moved.
	EGraveTravelBlock Block = EGraveTravelBlock::None;
	TestFalse(TEXT("A refused travel doesn't start"), Travel->TravelTo(Player, *Here, &Block));
	TestTrue(TEXT("...and says why"), Block == EGraveTravelBlock::AlreadyThere && !Travel->IsTravelling());
	TestTrue(TEXT("The far grave: on the way"), Travel->TravelTo(Player, *Far, &Block) && Travel->IsTravelling());
	TestTrue(TEXT("...no second travel meanwhile"), Travel->CheckNow(Player) == EGraveTravelBlock::Travelling);
	// The black held a frame at a time, as the subsystem's tick moves it on (a test level never ticks).
	Travel->Advance(GraveTravelRules::FadeOutSeconds * 0.5f);
	TestTrue(TEXT("...still in the fade halfway through it"), Travel->IsTravelling());
	Travel->Advance(GraveTravelRules::FadeOutSeconds + GraveTravelRules::HoldSeconds);
	const FVector Expected = GraveTravelRules::ArrivalLocation(Far->GetActorLocation(), Player->GetDefaultHalfHeight());
	TestTrue(TEXT("Arrived: standing on the grave"), Player->GetActorLocation().Equals(Expected, 1.0));
	TestTrue(TEXT("...facing its arrow"), FMath::IsNearlyEqual(FRotator::NormalizeAxis(Player->GetActorRotation().Yaw), 90.0, 0.5));
	TestFalse(TEXT("...and the travel is over"), Travel->IsTravelling());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapPageTest, "Looter.UI.Map.Page",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMapPageTest::RunTest(const FString& Parameters)
{
	// The map page over a test level, driven by its keys: its grave pins, G cycling them, the D-pad picking the one that
	// way, Esc letting go before it closes, zoom and pan keys, and Travel refused with nobody to send.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	// North (up on the map) and east (to the right); a closed one stays off the page.
	ARespawnMarker* North = PlaceMapGrave(World, TEXT("NorthGrave"), FVector(2000.0, 0.0, 0.0), 0.0, true);
	ARespawnMarker* East = PlaceMapGrave(World, TEXT("EastGrave"), FVector(0.0, 3000.0, 0.0), 0.0, true);
	ARespawnMarker* Shut = PlaceMapGrave(World, TEXT("ShutGrave"), FVector(-2000.0, 0.0, 0.0), 0.0, false, TEXT("TestNeverDone"));
	UMapWidget* Page = CreateWidget<UMapWidget>(World, UMapWidget::StaticClass());
	if (!TestTrue(TEXT("The graves and the page"), North && East && Shut && Page))
	{
		return false;
	}
	Page->Open(nullptr);
	Page->TakeWidget();
	UMapWidget::FView View = Page->GetView();
	TestEqual(TEXT("Two grave pins"), MapPins::Count(View.Pins, EMapPinKind::Grave), 2);
	TestTrue(TEXT("None chosen as it opens"), View.SelectedGrave.IsNone());
	TestTrue(TEXT("No player in a test level: nobody to send"), View.Block == EGraveTravelBlock::NoPlayer);

	// G cycles the open graves and comes round again.
	Page->HandleKey(EKeys::G);
	const FName First = Page->GetView().SelectedGrave;
	Page->HandleKey(EKeys::G);
	const FName Second = Page->GetView().SelectedGrave;
	Page->HandleKey(EKeys::G);
	TestTrue(TEXT("G: one grave, then the other, then the first again"), !First.IsNone() && !Second.IsNone() && First != Second
		&& Page->GetView().SelectedGrave == First);

	// Esc lets go of the chosen grave before it would close anything.
	TestTrue(TEXT("Esc is the page's"), Page->HandleKey(EKeys::Escape));
	TestTrue(TEXT("...letting go of the grave"), Page->GetView().SelectedGrave.IsNone());

	// The D-pad: right from the middle is the east grave, up from it the north one.
	Page->HandleKey(EKeys::Gamepad_DPad_Right);
	TestTrue(TEXT("D-pad right: the east grave"), Page->GetView().SelectedGrave == FName(TEXT("EastGrave")));
	Page->HandleKey(EKeys::Gamepad_DPad_Up);
	TestTrue(TEXT("D-pad up: the north grave"), Page->GetView().SelectedGrave == FName(TEXT("NorthGrave")));

	// Travel with nobody to send: refused, the grave still chosen.
	Page->HandleKey(EKeys::E);
	View = Page->GetView();
	TestTrue(TEXT("E with nobody to send: refused, still chosen"), View.SelectedGrave == FName(TEXT("NorthGrave")) && View.Block == EGraveTravelBlock::NoPlayer);

	// Zoom in, then W looks north.
	const double ZoomBefore = View.Map.Zoom;
	Page->HandleKey(EKeys::PageUp);
	View = Page->GetView();
	TestTrue(TEXT("Page Up zooms in"), View.Map.Zoom > ZoomBefore);
	const double NorthBefore = View.Map.Center.Y;
	Page->HandleKey(EKeys::W);
	TestTrue(TEXT("W looks north"), Page->GetView().Map.Center.Y < NorthBefore);
	return true;
}

#endif

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Areas/AreaTravelSubsystem.h"
#include "Areas/StationBoard.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Scenes/SceneSubsystem.h"
#include "Scenes/TrainShots.h"
#include "Session/CampaignRecord.h"
#include "Tests/LanternLeansTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "World/MinimapSubsystem.h"
#include "World/SkiffJetty.h"
#include "World/Train.h"
#include "World/TrainStation.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// The train at Ransom's Rest's depot (Docs/Areas/RansomsRest.md: Stations and the train; Track and train collision; step 9's
// "a short shot, then a fade"): put together from Train.py's parts, cold and shut until Main 7, its wheels rolling as it
// moves, its two shots, and which trips go by it.

using namespace LanternLeansTestWorld;

namespace
{
	constexpr float SceneFrame = 1.f / 30.f;

	/** Where the layout parks the hearse car (layout.json's hearseCar): the train's origin, facing out along +Y. */
	const FVector Parked(-100.0, 11560.0, 30.0);
	const FRotator Facing(0.0, 90.0, 0.0);

	/** The train as the build script places it, begun. */
	ATrain* SpawnTrain(UWorld* World)
	{
		ATrain* Train = World->SpawnActor<ATrain>(Parked, Facing);
		if (Train)
		{
			Train->DispatchBeginPlay();
		}
		return Train;
	}

	/** Ticks the level's scenes for Seconds in frames, counting them on Clock. */
	void TickScenes(USceneSubsystem& Scenes, float Seconds, float& Clock)
	{
		for (float Done = 0.f; Done < Seconds - KINDA_SMALL_NUMBER; Done += SceneFrame)
		{
			const float Delta = FMath::Min(SceneFrame, Seconds - Done);
			Clock += Delta;
			Scenes.Tick(Delta);
		}
	}

	FVector InstanceLocation(const UInstancedStaticMeshComponent* Gear, int32 Index)
	{
		FTransform At;
		return Gear && Gear->GetInstanceTransform(Index, At, /*bWorldSpace*/ false) ? At.GetLocation() : FVector::ZeroVector;
	}

	FQuat InstanceTurn(const UInstancedStaticMeshComponent* Gear, int32 Index)
	{
		FTransform At;
		return Gear && Gear->GetInstanceTransform(Index, At, /*bWorldSpace*/ false) ? At.GetRotation() : FQuat::Identity;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrainAssemblyTest, "Looter.Train.Assembly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTrainAssemblyTest::RunTest(const FString& Parameters)
{
	// A wheel rolls on the rail without slipping: as the axle goes on 1 cm, the point of its tread that was lowest stays put
	// along the rail (it only starts to lift, by the square of the distance over twice the radius), for both treads.
	for (const float Radius : { 42.f, 52.f })
	{
		const FVector Bottom(0.0, 0.0, -Radius);
		const FVector Rolled = ATrain::WheelSpin(1.f, Radius).RotateVector(Bottom) + FVector(1.0, 0.0, 0.0);
		TestTrue(*FString::Printf(TEXT("A %.0f cm wheel rolls without slipping"), Radius), FMath::Abs(Rolled.X) < 0.001
			&& FMath::IsNearlyEqual(Rolled.Z, static_cast<double>(-Radius), 0.02));
		TestTrue(*FString::Printf(TEXT("...once round in its own girth (%.0f cm)"), Radius),
			ATrain::WheelSpin(2.f * UE_PI * Radius, Radius).RotateVector(Bottom).Equals(Bottom, 0.05));
	}

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ATrain* Train = SpawnTrain(World);
	if (!TestNotNull(TEXT("The train"), Train))
	{
		return false;
	}
	TestTrue(TEXT("Tagged as the train, and as an obstacle for the minimap"), Train->ActorHasTag(ATrain::TrainTag)
		&& Train->ActorHasTag(MinimapTags::Obstacle));
	TestFalse(TEXT("It never ticks"), Train->PrimaryActorTick.bCanEverTick);
	TestTrue(TEXT("The level's train is found"), ATrain::FindIn(World) == Train);

	const bool bModels = Train->HearseCarMesh && Train->PassengerCarMesh && Train->LocomotiveMesh;
	if (!bModels)
	{
		AddWarning(TEXT("The train's models aren't imported (Art/Models/Vehicles/Train.py): only the fallback spacing is checked."));
		TestEqual(TEXT("The passenger car's place without models"), Train->GetCarOffset(1), 1128.f);
		return true;
	}
	TestEqual(TEXT("The hearse car at the origin"), Train->GetCarOffset(0), 0.f);
	// Coupled where their couplers meet, as layout.json parks them (the passenger car 11.3 m on, the locomotive 21.8 m).
	TestNearlyEqual(TEXT("The passenger car coupled ahead"), Train->GetCarOffset(1), 1130.f, 10.f);
	TestNearlyEqual(TEXT("The locomotive coupled ahead of it"), Train->GetCarOffset(2), 2180.f, 10.f);
	for (int32 Car = 1; Car < ATrain::NumCars; ++Car)
	{
		const UStaticMeshComponent* Behind = Train->GetCar(Car - 1);
		const UStaticMeshComponent* Ahead = Train->GetCar(Car);
		if (Behind->DoesSocketExist(TEXT("Coupler_Front")) && Ahead->DoesSocketExist(TEXT("Coupler_Back")))
		{
			TestTrue(*FString::Printf(TEXT("Car %d's coupler plane meets car %d's: no gap to squeeze through"), Car - 1, Car),
				Behind->GetSocketLocation(TEXT("Coupler_Front")).Equals(Ahead->GetSocketLocation(TEXT("Coupler_Back")), 1.0));
		}
	}
	for (int32 Car = 0; Car < ATrain::NumCars; ++Car)
	{
		TestTrue(*FString::Printf(TEXT("Car %d is solid"), Car), Train->GetCar(Car)->GetCollisionEnabled() != ECollisionEnabled::NoCollision);
	}
	TestTrue(TEXT("The wheels, rods, door and steam stop nothing"), Train->DriverWheels->GetCollisionEnabled() == ECollisionEnabled::NoCollision
		&& Train->Rods->GetCollisionEnabled() == ECollisionEnabled::NoCollision && Train->HearseDoor->GetCollisionEnabled() == ECollisionEnabled::NoCollision
		&& Train->Smoke->GetCollisionEnabled() == ECollisionEnabled::NoCollision);

	// The running gear: Locomotive B's leading wheels, three drivers and trailing wheels; four on each car's two bogies.
	TestEqual(TEXT("Thirteen axles"), Train->GetNumAxles(), 13);
	TestEqual(TEXT("...three of them the locomotive's drivers"), Train->GetNumDriverAxles(), 3);
	TestEqual(TEXT("A wheel set on each"), Train->DriverWheels->GetInstanceCount() + Train->CarriageWheels->GetInstanceCount(), 13);
	TestEqual(TEXT("Two coupling rods"), Train->Rods->GetInstanceCount(), 2);

	// The platform side: its -Y, where the hearse car's door opens onto the boards 40 cm up.
	const FTransform Arrival = Train->GetArrivalSpot();
	const FVector OnPlatform = Train->GetActorTransform().InverseTransformPosition(Arrival.GetLocation());
	TestTrue(TEXT("Trips arrive on the platform at the hearse car's door"), OnPlatform.Y < -165.0 && OnPlatform.Y > -300.0
		&& FMath::Abs(OnPlatform.X) < 300.0 && FMath::IsNearlyEqual(OnPlatform.Z, 40.0, 5.0));
	TestTrue(TEXT("...facing out across the boards"), FVector::DotProduct(Arrival.GetRotation().GetForwardVector(), -Train->GetActorRightVector()) > 0.99);
	const FTransform Camera = Train->GetShotCameraTransform();
	const FVector CameraLocal = Train->GetActorTransform().InverseTransformPosition(Camera.GetLocation());
	TestTrue(TEXT("The shots' camera stands on the platform"), CameraLocal.Y < -165.0 && CameraLocal.Z > 100.0);
	TestTrue(TEXT("...looking out along the train"), FVector::DotProduct(Camera.GetRotation().GetForwardVector(), Train->GetForward()) > 0.95);

	// Moving: the whole train along its track, every wheel turned by the distance over its tread, the rods on the cranks.
	const FQuat DriverRest = InstanceTurn(Train->DriverWheels, 0);
	const FQuat CarriageRest = InstanceTurn(Train->CarriageWheels, 0);
	const FVector DriverMiddle = InstanceLocation(Train->DriverWheels, 0);
	const FVector RodRest = InstanceLocation(Train->Rods, 0);
	Train->SetTravel(500.f);
	TestTrue(TEXT("5 m out along its track (+Y here)"), Train->GetActorLocation().Equals(Parked + FVector(0.0, 500.0, 0.0), 0.1)
		&& FMath::IsNearlyEqual(Train->GetTravel(), 500.f));
	TestTrue(TEXT("...a driver turned by 5 m over its 52 cm tread"), InstanceTurn(Train->DriverWheels, 0).Equals(ATrain::WheelSpin(500.f, 52.f) * DriverRest, 1e-3));
	TestTrue(TEXT("...a carriage wheel by 5 m over its 42 cm"), InstanceTurn(Train->CarriageWheels, 0).Equals(ATrain::WheelSpin(500.f, 42.f) * CarriageRest, 1e-3));
	TestTrue(TEXT("...on the same axle"), InstanceLocation(Train->DriverWheels, 0).Equals(DriverMiddle, 0.01));
	const FVector RodMoved = InstanceLocation(Train->Rods, 0);
	TestTrue(TEXT("...the rod gone round with the crank, at the crank's reach"), !RodMoved.Equals(RodRest, 1.0)
		&& FMath::IsNearlyEqual(RodMoved.Y, RodRest.Y, 0.01));
	Train->SetTravel(0.f);
	TestTrue(TEXT("Put back: where it was parked, its wheels as they stood"), Train->GetActorLocation().Equals(Parked, 0.01)
		&& InstanceTurn(Train->DriverWheels, 0).Equals(DriverRest, 1e-4) && InstanceLocation(Train->Rods, 0).Equals(RodRest, 0.01));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrainStoryTest, "Looter.Train.Story",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTrainStoryTest::RunTest(const FString& Parameters)
{
	// Cold and shut at the platform from the start; from Main 7 on (Main 6 done) steam up, lamps lit, the hearse car's door
	// open (Docs/Story.md: "The train stands at the platform from the start, cold and shut; from this mission it has steam up
	// and the hearse car's door is open").
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector(0.0, 0.0, 90.0));
	if (!Runner || !Player)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	Runner->BeginForTesting({ MakeMainSix(Scratch), MakeMainSeven(Scratch) }, Campaign, Player, Valley);
	ATrain* Train = SpawnTrain(World);
	if (!TestNotNull(TEXT("The train"), Train))
	{
		return false;
	}
	TestTrue(TEXT("Warm from Main 7 on: after Main 6"), Train->WarmWhen.AfterMissions == TArray<FName>({ MainSix }));
	Runner->Update(0.f);

	const UStaticMeshComponent* Hearse = Train->HearseCar;
	const int32 Glass = Hearse->GetStaticMesh() ? Hearse->GetMaterialIndex(Train->GlassSlot) : INDEX_NONE;
	TestTrue(TEXT("During Main 6: cold and shut"), !Train->IsWarm() && !Train->IsDoorOpen() && !Train->Smoke->IsVisible());
	if (Glass != INDEX_NONE && Train->ColdGlass)
	{
		TestTrue(TEXT("...the hearse car's lamps dark"), Hearse->GetMaterial(Glass) == Train->ColdGlass);
	}
	else
	{
		AddWarning(TEXT("The hearse car (or the trim's glass) isn't imported: its lamps aren't checked."));
	}

	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.MainSixDone")));
	TestTrue(TEXT("Main 6 done, Main 7 begun"), Campaign.HasCompleted(MainSix) && Runner->IsRunning(MainSeven));
	TestTrue(TEXT("...steam up, and Tilly's car's door open"), Train->IsWarm() && Train->IsDoorOpen()
		&& Train->Smoke->IsVisible() == (Train->SmokeMesh != nullptr));
	if (Glass != INDEX_NONE)
	{
		TestTrue(TEXT("...its lamps lit"), Hearse->GetMaterial(Glass) == Hearse->GetStaticMesh()->GetMaterial(Glass));
	}

	// The console holds a look whatever the story says.
	Train->HoldLook(false);
	Train->RefreshStory();
	TestTrue(TEXT("Held cold, it stays cold through the story's changes"), !Train->IsWarm() && !Train->IsDoorOpen());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrainShotsTest, "Looter.Train.Shots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTrainShotsTest::RunTest(const FString& Parameters)
{
	// The departure as plain math: still until the brakes come off, then pulling away faster and faster, about 10 m out as
	// the screen goes black about 4 s in.
	TestTrue(TEXT("The departure lasts about 4 s"), TrainShots::DepartSeconds >= 3.5f && TrainShots::DepartSeconds <= 5.f);
	TestEqual(TEXT("Still until the brakes come off"), TrainShots::DepartDistanceAt(TrainShots::PullOutTime), 0.f);
	bool bGathers = true;
	float LastStep = 0.f;
	for (float Seconds = TrainShots::PullOutTime; Seconds < TrainShots::DepartSeconds; Seconds += 0.1f)
	{
		const float Step = TrainShots::DepartDistanceAt(Seconds + 0.1f) - TrainShots::DepartDistanceAt(Seconds);
		bGathers = bGathers && Step > LastStep - 0.01f;
		LastStep = Step;
	}
	TestTrue(TEXT("...then gathering speed"), bGathers);
	const float Out = TrainShots::DepartDistanceAt(TrainShots::DepartSeconds);
	TestTrue(TEXT("...8-15 m out by the black"), Out > 800.f && Out < 1500.f);
	TestEqual(TEXT("No black before it fades"), TrainShots::DepartBlackAt(TrainShots::FadeOutStart), 0.f);
	TestEqual(TEXT("Black at the end"), TrainShots::DepartBlackAt(TrainShots::DepartSeconds), 1.f);

	// The arrival, the same backwards: out of the black, rolling in from 10-25 m off and braking to a stand at the platform.
	const float From = TrainShots::ArriveDistanceAt(0.f);
	TestTrue(TEXT("It rolls in from 10-25 m out"), From > 1000.f && From < 2500.f);
	bool bBrakes = true;
	LastStep = TNumericLimits<float>::Max();
	for (float Seconds = 0.f; Seconds < TrainShots::RollInSeconds; Seconds += 0.1f)
	{
		const float Step = TrainShots::ArriveDistanceAt(Seconds) - TrainShots::ArriveDistanceAt(Seconds + 0.1f);
		bBrakes = bBrakes && Step >= 0.f && Step < LastStep + 0.01f;
		LastStep = Step;
	}
	TestTrue(TEXT("...braking all the way"), bBrakes);
	TestEqual(TEXT("...to a stand where it parks"), TrainShots::ArriveDistanceAt(TrainShots::RollInSeconds), 0.f);
	TestEqual(TEXT("...and there it stays"), TrainShots::ArriveDistanceAt(TrainShots::ArriveSeconds), 0.f);
	TestEqual(TEXT("Out of the black at once"), TrainShots::ArriveBlackAt(0.f), 1.f);
	TestEqual(TEXT("...clear once it has faded in"), TrainShots::ArriveBlackAt(TrainShots::FadeInSeconds), 0.f);
	TestTrue(TEXT("The player has the view back once it has stopped"), TrainShots::HandBackAt > TrainShots::RollInSeconds
		&& TrainShots::ArriveSeconds < 8.f);

	// The two shots in a test level (no player there to hold): their moments in order, the train where they leave it.
	FScenesOn On;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	USceneSubsystem* Scenes = World->GetSubsystem<USceneSubsystem>();
	ATrain* Train = SpawnTrain(World);
	if (!TestNotNull(TEXT("The level has scenes"), Scenes) || !TestNotNull(TEXT("...and a train"), Train))
	{
		return false;
	}
	float Clock = 0.f;
	TMap<FName, float> Heard;
	TArray<FName> Order;
	const FDelegateHandle Listening = Scenes->OnSceneEvent.AddLambda([&Heard, &Order, &Clock](FName Event)
	{
		Heard.Add(Event, Clock);
		Order.Add(Event);
	});
	int32 Blacks = 0;
	auto CountBlack = [&Blacks]() { return TFunction<void()>([&Blacks]() { ++Blacks; }); };

	TestTrue(TEXT("The departure plays"), Scenes->Play(TrainShots::MakeDeparture(*Scenes, *Train, CountBlack(), /*bTripFollows*/ false)));
	TestTrue(TEXT("...as the scene playing"), Scenes->GetPlayingName() == TrainShots::DepartureName());
	TickScenes(*Scenes, 3.f, Clock);
	TestNearlyEqual(TEXT("3 s in, out as far as its course says"), Train->GetTravel(), TrainShots::DepartDistanceAt(3.f), 15.f);
	while (Scenes->IsPlaying() && Clock < 10.f)
	{
		TickScenes(*Scenes, SceneFrame, Clock);
	}
	TestNearlyEqual(TEXT("The brakes came off 0.6 s in"), Heard.FindRef(TrainShots::PullOutMoment()), TrainShots::PullOutTime, 0.05f);
	TestNearlyEqual(TEXT("Black about 4 s in"), Heard.FindRef(TrainShots::BlackMoment()), TrainShots::DepartSeconds, 0.05f);
	TestEqual(TEXT("...then its end, once"), Blacks, 1);
	TestTrue(TEXT("Pulling out, then black, each once"), Order.Num() == 3 && Order[0] == TrainShots::PullOutMoment()
		&& Order[2] == TrainShots::BlackMoment());
	TestTrue(TEXT("No trip: the train is back at the platform"), FMath::IsNearlyZero(Train->GetTravel())
		&& Train->GetActorLocation().Equals(Parked, 0.1));
	TestTrue(TEXT("The missions heard it played"), Scenes->HasPlayed(TrainShots::DepartureName()));

	// The arrival: from out along the track to a stand at the platform, the view handed back after.
	Heard.Reset();
	Order.Reset();
	Clock = 0.f;
	TestTrue(TEXT("The arrival plays"), Scenes->Play(TrainShots::MakeArrival(*Scenes, *Train, Train->GetArrivalSpot())));
	TestNearlyEqual(TEXT("It begins out along the track"), Train->GetTravel(), TrainShots::ArriveDistanceAt(0.f), 1.f);
	while (Scenes->IsPlaying() && Clock < 12.f)
	{
		TickScenes(*Scenes, SceneFrame, Clock);
	}
	TestNearlyEqual(TEXT("It stops 4.5 s in"), Heard.FindRef(TrainShots::StopMoment()), TrainShots::RollInSeconds, 0.05f);
	TestNearlyEqual(TEXT("...and the view is handed back after"), Heard.FindRef(TrainShots::HandBackMoment()), TrainShots::HandBackAt, 0.05f);
	TestTrue(TEXT("...standing where it parks"), FMath::IsNearlyZero(Train->GetTravel()) && Train->GetActorLocation().Equals(Parked, 0.1));
	TestNearlyEqual(TEXT("The shot ends with control back"), Clock, TrainShots::ArriveSeconds, 0.1f);

	// Skipped as the brakes come off: its end at once, its moments in order, the train back (no trip).
	Order.Reset();
	TestTrue(TEXT("The departure plays again"), Scenes->Play(TrainShots::MakeDeparture(*Scenes, *Train, CountBlack(), false)));
	TickScenes(*Scenes, 0.5f, Clock);
	Scenes->SkipScene();
	TestEqual(TEXT("A skip ends it at once"), Blacks, 2);
	TestTrue(TEXT("...its moments still going out"), Order.Num() == 3 && Order[0] == TrainShots::PullOutMoment() && Order[2] == TrainShots::BlackMoment());
	TestTrue(TEXT("...nothing playing, the train at the platform"), !Scenes->IsPlaying() && FMath::IsNearlyZero(Train->GetTravel()));
	Scenes->OnSceneEvent.Remove(Listening);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrainTripsTest, "Looter.Train.Trips",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTrainTripsTest::RunTest(const FString& Parameters)
{
	// Which trips go by train: an opened area of the story, from a station whose train is in. Practice trips stay plain
	// fades both ways, the first cast-off is the skiff's, and a jetty has no train.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ATrainStation* Station = World->SpawnActor<ATrainStation>(FVector(765.0, 11400.0, 32.0), FRotator(0.0, 180.0, 0.0));
	ATrainStation* Lonely = World->SpawnActor<ATrainStation>(FVector(40000.0, 0.0, 0.0), FRotator::ZeroRotator);
	ASkiffJetty* Jetty = World->SpawnActor<ASkiffJetty>(FVector(-5000.0, 0.0, 0.0), FRotator::ZeroRotator);
	ATrain* Train = SpawnTrain(World);
	if (!Station || !Lonely || !Jetty || !Train)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	TestTrue(TEXT("The depot's train is the one at its platform"), ATrain::FindNear(Station) == Train);
	TestNull(TEXT("A station with no train in has none"), ATrain::FindNear(Lonely));

	FStationBoardLine Story;
	Story.Kind = EStationLine::Area;
	Story.AreaId = LilyId;
	Story.Name = FText::FromString(TEXT("The Gilded Lily"));
	TestTrue(TEXT("To an area of the story from the depot: by train"), UAreaTravelSubsystem::FindTrainFor(Story, Station) == Train);
	TestNull(TEXT("...not from a station with no train"), UAreaTravelSubsystem::FindTrainFor(Story, Lonely));
	TestNull(TEXT("...nor from Skyreach's jetty"), UAreaTravelSubsystem::FindTrainFor(Story, Jetty));
	TestNull(TEXT("...nor from the console (no board)"), UAreaTravelSubsystem::FindTrainFor(Story, nullptr));
	FStationBoardLine Practice = Story;
	Practice.Kind = EStationLine::Practice;
	TestNull(TEXT("Practice: a plain fade"), UAreaTravelSubsystem::FindTrainFor(Practice, Station));
	FStationBoardLine Here = Story;
	Here.bHere = true;
	TestNull(TEXT("Where the board stands: nowhere to go"), UAreaTravelSubsystem::FindTrainFor(Here, Station));
	FStationBoardLine CastOff = Story;
	CastOff.bFirstCastOff = true;
	TestNull(TEXT("The first cast-off: the skiff's ride"), UAreaTravelSubsystem::FindTrainFor(CastOff, Station));
	return true;
}

#endif

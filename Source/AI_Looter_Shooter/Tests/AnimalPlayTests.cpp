#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/FaunaTestWorld.h"
#include "World/BarnCat.h"
#include "World/ChickenYard.h"
#include "World/ChickenYardRules.h"
#include "World/CliffSwallowFlock.h"
#include "World/FaunaSubsystem.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// The small animals in play (World/ChickenYard, World/BarnCat, World/CliffSwallowFlock), moved on frame by frame with the
// contexts the tests make (FaunaTestWorld): the chickens stay in their pen whatever happens, scatter at a gunshot and
// calm again; the cat hides at gunfire and comes back, greets a player and walks off from one who lingers; the swallows
// loop out from their nests and come home, burst out at a gunshot and return once it's calm.

using namespace FaunaTestWorld;

namespace
{
	/** The farm's yard as make_town.py drew it, about the origin: 3.2 x 3.7 m inside its fence, the coop's box in it. */
	FChickenPen AnimalPlayPen()
	{
		FChickenPen Pen;
		Pen.Outline = { FVector2D(-160.0, -185.0), FVector2D(160.0, -185.0), FVector2D(160.0, 185.0), FVector2D(-160.0, 185.0) };
		FChickenPenBlock& Coop = Pen.Blocks.AddDefaulted_GetRef();
		Coop.Center = FVector2D(-50.0, -5.0);
		Coop.HalfSize = FVector2D(114.0, 124.0);
		Pen.Hens = 3;
		Pen.bRooster = true;
		return Pen;
	}

	AChickenYard* AnimalPlayYard(UWorld* World, int32 Seed)
	{
		AChickenYard* Yard = World->SpawnActor<AChickenYard>(FVector(0.0, 0.0, 0.0), FRotator::ZeroRotator);
		if (!Yard)
		{
			return nullptr;
		}
		Yard->Pens = { AnimalPlayPen() };
		Yard->Seed = Seed;
		Yard->DispatchBeginPlay();
		return Yard;
	}

	FFaunaNoise AnimalPlayShot(const FVector& Where, double Time)
	{
		FFaunaNoise Noise;
		Noise.Kind = EFaunaNoise::Gunshot;
		Noise.Location = Where;
		Noise.Radius = UFaunaSubsystem::GunshotRadius;
		Noise.Time = Time;
		return Noise;
	}

	/** A context with a gunshot heard at Time (it stays among the noises, as the subsystem keeps them). */
	FFaunaContext AnimalPlayShotContext(const FVector& Where, double Time)
	{
		FFaunaContext Context;
		Context.Noises.Add(AnimalPlayShot(Where, Time));
		return Context;
	}

	/** A sprinting player at Where, running along +Y. */
	FFaunaThreat AnimalPlaySprinter(const FVector& Where)
	{
		FFaunaThreat Threat;
		Threat.Location = Where;
		Threat.Velocity = FVector(0.0, 700.0, 0.0);
		Threat.Fear = 1.35f;
		Threat.bPlayer = true;
		return Threat;
	}

	int32 AnimalPlayFleeing(const AChickenYard& Yard)
	{
		int32 Count = 0;
		for (const FChicken& Bird : Yard.GetChickens())
		{
			Count += Bird.Doing == EChickenDoing::Flee ? 1 : 0;
		}
		return Count;
	}

	ACliffSwallowFlock* AnimalPlaySwallows(UWorld* World, int32 Seed)
	{
		ACliffSwallowFlock* Flock = World->SpawnActor<ACliffSwallowFlock>();
		if (!Flock)
		{
			return nullptr;
		}
		// Two clusters under an overhang 6 m up a cliff facing +X, 3 m apart; their model's mouths stand in.
		Flock->Nests = { FTransform(FVector(0.0, 0.0, 600.0)), FTransform(FVector(0.0, 300.0, 620.0)) };
		Flock->BirdsPerNest = 3;
		Flock->Seed = Seed;
		UStaticMesh* Cube = StandIn();
		Flock->PerchedMesh = Flock->HeadMesh = Flock->FlyingMesh = Cube;
		Flock->WingInnerMeshL = Flock->WingInnerMeshR = Flock->WingOuterMeshL = Flock->WingOuterMeshR = Cube;
		Flock->DispatchBeginPlay();
		return Flock;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAnimalYardKeepsInsideTest, "Looter.World.Animals.YardKeepsInside",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAnimalYardKeepsInsideTest::RunTest(const FString& Parameters)
{
	// Three hens and a rooster in a yard that's mostly coop (a strip round it): two minutes of pottering, a player walking
	// round the fence and gunshots now and then, and every frame every bird's feet are inside its pen, out of the coop,
	// on the ground.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	AChickenYard* Yard = AnimalPlayYard(TestLevel.GetTestWorld(), 17);
	if (!TestNotNull(TEXT("A yard"), Yard))
	{
		return false;
	}
	TestEqual(TEXT("Three hens and the rooster"), Yard->GetChickens().Num(), 4);
	int32 Roosters = 0;
	int32 White = 0;
	for (const FChicken& Bird : Yard->GetChickens())
	{
		Roosters += Bird.bRooster ? 1 : 0;
		White += !Bird.bRooster && Bird.bWhite ? 1 : 0;
	}
	TestEqual(TEXT("One rooster"), Roosters, 1);
	TestTrue(TEXT("Brown hens and white"), White >= 1 && White <= 2);

	const FChickenPen Pen = AnimalPlayPen();
	int32 Outside = 0;
	int32 OffGround = 0;
	int32 Ran = 0;
	int32 Walked = 0;
	double Now = 0.0;
	double LastShot = -100.0;
	for (int32 Frame30 = 0; Frame30 < 120 * 30; ++Frame30)
	{
		const double Seconds = Frame30 * static_cast<double>(Frame);
		Run(*Yard, Frame, Now, [&](double Time)
		{
			FFaunaContext Context;
			// Round the fence 2.5 m out, once every 40 s, for half the time; a shot every 30 s from 12 m.
			if (FMath::Fmod(Seconds, 40.0) < 20.0)
			{
				const double Angle = Seconds * UE_TWO_PI / 40.0;
				Context.Threats.Add(Walker(FVector(FMath::Cos(Angle) * 330.0, FMath::Sin(Angle) * 330.0, 90.0)));
			}
			if (Frame30 > 0 && Frame30 % (30 * 30) == 0)
			{
				LastShot = Time;
			}
			if (Time - LastShot < UFaunaSubsystem::NoiseMemory)
			{
				Context.Noises.Add(AnimalPlayShot(FVector(1200.0, 0.0, 150.0), LastShot));
			}
			return Context;
		});
		for (const FChicken& Bird : Yard->GetChickens())
		{
			Outside += ChickenPens::IsInside(Pen, Bird.Position, Yard->BodyRoom * 0.85f) ? 0 : 1;
			OffGround += FMath::Abs(Bird.Height - Yard->GroundAt(Bird.Position)) < 2.f ? 0 : 1;
			Ran += Bird.Doing == EChickenDoing::Flee && Bird.Delay <= 0.f ? 1 : 0;
			Walked += Bird.Doing == EChickenDoing::Walk ? 1 : 0;
		}
	}
	TestEqual(TEXT("No bird ever outside its pen or in the coop"), Outside, 0);
	TestEqual(TEXT("Every bird's feet always on the ground"), OffGround, 0);
	TestTrue(TEXT("They ran (scattered by the walker and the shots)"), Ran > 0);
	TestTrue(TEXT("...and walked about between"), Walked > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAnimalYardScatterTest, "Looter.World.Animals.YardScatter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAnimalYardScatterTest::RunTest(const FString& Parameters)
{
	// Left alone they potter; a gunshot 15 m off scatters them all; 5-8 s after it they calm. A shot 40 m off doesn't;
	// a player 4.5 m off is watched but doesn't scatter them; one in among them does.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	AChickenYard* Yard = AnimalPlayYard(TestLevel.GetTestWorld(), 5);
	if (!TestNotNull(TEXT("A yard"), Yard))
	{
		return false;
	}
	double Now = 0.0;
	Run(*Yard, 3.f, Now, Quiet);
	TestFalse(TEXT("Left alone they aren't scared"), Yard->IsScared());
	TestEqual(TEXT("...nobody runs"), AnimalPlayFleeing(*Yard), 0);

	const double ShotAt = Now + Frame;
	Run(*Yard, 0.3f, Now, [&](double) { return AnimalPlayShotContext(FVector(1500.0, 0.0, 150.0), ShotAt); });
	TestTrue(TEXT("A gunshot 15 m off scatters them"), Yard->IsScared());
	TestEqual(TEXT("...every bird running"), AnimalPlayFleeing(*Yard), Yard->GetChickens().Num());
	Run(*Yard, 4.f, Now, Quiet);
	TestTrue(TEXT("Four seconds on they're still uneasy"), Yard->IsScared());
	Run(*Yard, 6.f, Now, Quiet);
	TestFalse(TEXT("Ten seconds on they've calmed"), Yard->IsScared());
	TestEqual(TEXT("...nobody runs"), AnimalPlayFleeing(*Yard), 0);

	const double FarShot = Now + Frame;
	Run(*Yard, 1.f, Now, [&](double) { return AnimalPlayShotContext(FVector(4000.0, 0.0, 150.0), FarShot); });
	TestFalse(TEXT("A shot 40 m off doesn't scatter them"), Yard->IsScared());

	// The shot from the east sent them to the yard's west side: the watcher stands 4.5 m east of the easternmost bird, so
	// every bird is past the flee radius (3.3 m) and that one is inside the alert radius (6 m).
	FVector Eastmost = Yard->GetFeet(0);
	for (int32 Index = 1; Index < Yard->GetChickens().Num(); ++Index)
	{
		Eastmost = Yard->GetFeet(Index).X > Eastmost.X ? Yard->GetFeet(Index) : Eastmost;
	}
	const FVector Watcher = Eastmost + FVector(450.0, 0.0, 90.0);
	Run(*Yard, 1.f, Now, [&](double) { FFaunaContext Context; Context.Threats.Add(Walker(Watcher)); return Context; });
	TestFalse(TEXT("A player 4.5 m off doesn't scatter them"), Yard->IsScared());
	bool bWatched = false;
	for (const FChicken& Bird : Yard->GetChickens())
	{
		bWatched |= Bird.bWatching;
	}
	TestTrue(TEXT("...but is watched"), bWatched);
	Run(*Yard, 0.2f, Now, [&](double) { FFaunaContext Context; Context.Threats.Add(Walker(FVector(100.0, 0.0, 90.0))); return Context; });
	TestTrue(TEXT("A player in the yard among them scatters them"), Yard->IsScared());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAnimalCatHidesTest, "Looter.World.Animals.CatHides",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAnimalCatHidesTest::RunTest(const FString& Parameters)
{
	// A cat on a porch half a metre up: a shot 30 m off doesn't trouble it; one 10 m off sends it down off the deck and
	// under the porch, gone; 20-40 s after things are calm it's back up on the boards. A sprint right up to it sends it too.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	ABarnCat* Cat = TestLevel.GetTestWorld()->SpawnActor<ABarnCat>();
	if (!TestNotNull(TEXT("A cat"), Cat))
	{
		return false;
	}
	Cat->LoafSpot = FVector(0.0, 0.0, 50.0);
	Cat->PorchA = FVector(0.0, -150.0, 50.0);
	Cat->PorchB = FVector(0.0, 150.0, 50.0);
	Cat->PorchExit = FVector(80.0, 0.0, 50.0);
	Cat->PorchStep = FVector(115.0, 0.0, 0.0);
	Cat->HidePoint = FVector(60.0, 0.0, 0.0);
	Cat->Seed = 3;
	Cat->DispatchBeginPlay();
	double Now = 0.0;
	Run(*Cat, 5.f, Now, Quiet);
	TestFalse(TEXT("Left alone it stays out"), Cat->IsHiding());
	TestTrue(TEXT("...on its porch"), FMath::Abs(Cat->GetFeet().Z - 50.0) < 1.0);

	const double FarShot = Now + Frame;
	Run(*Cat, 2.f, Now, [&](double) { return AnimalPlayShotContext(FVector(3000.0, 0.0, 150.0), FarShot); });
	TestTrue(TEXT("A shot 30 m off doesn't send it"), Cat->GetDoing() != ECatDoing::Slink && !Cat->IsHiding());

	const double NearShot = Now + Frame;
	Run(*Cat, 0.2f, Now, [&](double) { return AnimalPlayShotContext(FVector(1000.0, 0.0, 150.0), NearShot); });
	TestEqual(TEXT("A shot 10 m off sends it slinking"), Cat->GetDoing(), ECatDoing::Slink);
	Run(*Cat, 4.f, Now, Quiet);
	TestTrue(TEXT("...under the porch within seconds"), Cat->IsHiding());
	TestTrue(TEXT("...at its hiding place"), Cat->GetFeet().Equals(Cat->HidePoint, 1.0));
	Run(*Cat, 15.f, Now, Quiet);
	TestTrue(TEXT("Fifteen seconds on it's still hiding"), Cat->IsHiding());
	Run(*Cat, 40.f, Now, Quiet);
	TestFalse(TEXT("Under a minute later it's out again"), Cat->IsHiding());
	TestTrue(*FString::Printf(TEXT("...back up on the boards (%s)"), *Cat->GetFeet().ToCompactString()),
		FMath::Abs(Cat->GetFeet().Z - 50.0) < 1.0);

	Run(*Cat, 0.3f, Now, [&](double) { FFaunaContext Context; Context.Threats.Add(AnimalPlaySprinter(Cat->GetFeet() + FVector(150.0, 0.0, 40.0))); return Context; });
	TestTrue(TEXT("A sprint right up to it sends it off"), Cat->GetDoing() == ECatDoing::Slink || Cat->IsHiding());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAnimalCatGreetsTest, "Looter.World.Animals.CatGreets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAnimalCatGreetsTest::RunTest(const FString& Parameters)
{
	// A player walking up: watched from 8 m, greeted within 3 m (it stands, tail up); one who stays sends it off along
	// the porch, away from them.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	ABarnCat* Cat = TestLevel.GetTestWorld()->SpawnActor<ABarnCat>();
	if (!TestNotNull(TEXT("A cat"), Cat))
	{
		return false;
	}
	Cat->LoafSpot = FVector(0.0, 0.0, 50.0);
	Cat->PorchA = FVector(0.0, -250.0, 50.0);
	Cat->PorchB = FVector(0.0, 250.0, 50.0);
	Cat->PorchExit = FVector(80.0, 0.0, 50.0);
	Cat->HidePoint = FVector(60.0, 0.0, 0.0);
	Cat->Seed = 9;
	Cat->DispatchBeginPlay();
	double Now = 0.0;
	Run(*Cat, 2.f, Now, Quiet);
	const FVector Close(200.0, -60.0, 90.0);
	Run(*Cat, 1.f, Now, [&](double) { FFaunaContext Context; Context.Threats.Add(Walker(Close)); return Context; });
	TestEqual(TEXT("A player 2 m off is greeted"), Cat->GetDoing(), ECatDoing::Greet);
	Run(*Cat, 12.f, Now, [&](double) { FFaunaContext Context; Context.Threats.Add(Walker(Close)); return Context; });
	TestTrue(*FString::Printf(TEXT("One who lingers sends it off along the porch, away from them (%s)"), *Cat->GetFeet().ToCompactString()),
		Cat->GetFeet().Y > 150.0 && !Cat->IsHiding());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAnimalSwallowsReturnTest, "Looter.World.Animals.SwallowsReturn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAnimalSwallowsReturnTest::RunTest(const FString& Parameters)
{
	// Swallows at two nest clusters: each leaves, loops out over the slope and darts home into its own mouth; a gunshot
	// bursts them all out to circle over the cliff; once it's calm each comes home again.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	ACliffSwallowFlock* Flock = AnimalPlaySwallows(TestLevel.GetTestWorld(), 21);
	if (!TestNotNull(TEXT("A colony"), Flock))
	{
		return false;
	}
	TestEqual(TEXT("Three birds to a cluster"), Flock->GetBirds().Num(), 6);
	TestEqual(TEXT("Four mouths to a cluster"), Flock->GetMouths().Num(), 8);

	// Track each bird's going out and coming home (at its own mouth).
	const int32 Count = Flock->GetBirds().Num();
	TArray<bool> Left;
	TArray<int32> Home;
	Left.Init(false, Count);
	Home.Init(0, Count);
	int32 WrongMouth = 0;
	int32 TooLow = 0;
	auto Watch = [&]()
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FCliffSwallow& Bird = Flock->GetBirds()[Index];
			const bool bAtHome = Bird.State == ESwallowState::Inside || Bird.State == ESwallowState::Clinging;
			if (!bAtHome)
			{
				Left[Index] = true;
				// The cliff's foot is 6 m under the nests here: never down to it.
				TooLow += Bird.Position.Z < 50.0 ? 1 : 0;
			}
			else if (Left[Index])
			{
				Left[Index] = false;
				++Home[Index];
				WrongMouth += Bird.Position.Equals(Flock->GetMouths()[Bird.Mouth].GetLocation(), 0.5) ? 0 : 1;
			}
		}
	};
	double Now = 0.0;
	for (int32 Step = 0; Step < 90 * 30; ++Step)
	{
		Run(*Flock, Frame, Now, Quiet);
		Watch();
	}
	int32 EveryOne = 0;
	for (const int32 Each : Home)
	{
		EveryOne += Each > 0 ? 1 : 0;
	}
	TestEqual(TEXT("In a minute and a half every bird has looped out and come home"), EveryOne, Count);
	TestEqual(TEXT("...each into its own mouth"), WrongMouth, 0);
	TestEqual(TEXT("...never down at the cliff's foot"), TooLow, 0);

	const double ShotAt = Now + Frame;
	Run(*Flock, 2.f, Now, [&](double) { return AnimalPlayShotContext(FVector(1500.0, 150.0, 200.0), ShotAt); });
	TestTrue(TEXT("A gunshot 15 m off alarms them"), Flock->IsAlarmed());
	TestEqual(TEXT("...every bird out of its nest"), Flock->CountHome(), 0);
	int32 Circling = 0;
	for (const FCliffSwallow& Bird : Flock->GetBirds())
	{
		Circling += Bird.State == ESwallowState::Circling ? 1 : 0;
	}
	TestEqual(TEXT("...circling over the cliff"), Circling, Count);

	Home.Init(0, Count);
	Left.Init(true, Count);
	for (int32 Step = 0; Step < 45 * 30; ++Step)
	{
		Run(*Flock, Frame, Now, Quiet);
		Watch();
	}
	TestFalse(TEXT("Once it's calm they settle down"), Flock->IsAlarmed());
	EveryOne = 0;
	for (const int32 Each : Home)
	{
		EveryOne += Each > 0 ? 1 : 0;
	}
	TestEqual(TEXT("...and every bird has come home"), EveryOne, Count);
	return true;
}

#endif

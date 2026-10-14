#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/AmbientAirRules.h"
#include "World/AmbientAirSubsystem.h"
#include "Engine/World.h"
#include "Templates/Function.h"
#include "Tests/AutomationCommon.h"

namespace AmbientAirTestKit
{
	/** One frame of the air tests (s). */
	inline constexpr float Frame = 1.f / 30.f;

	const FName Day(TEXT("Day"));
	const FName Dusk(TEXT("Dusk"));

	/** A player standing at Where (eye height over the ground at 0), looking north, in Lighting. */
	inline FAirView ViewAt(const FVector& Where, FName Lighting)
	{
		FAirView View;
		View.bValid = true;
		View.Location = Where;
		View.Direction = FVector::ForwardVector;
		View.Lighting = Lighting;
		View.BreezeDirection = FRotator(0.f, 19.f, 0.f).Vector();
		View.GustDirection = View.BreezeDirection;
		return View;
	}

	/** Every point the same place: ground at 0, open to the sky or under a roof, this surface, in the sun or not. */
	inline TFunction<bool(const FVector&, FAirPlace&)> Everywhere(EImpactMaterial Surface, bool bInSun, bool bOpenSky = true)
	{
		return [Surface, bInSun, bOpenSky](const FVector&, FAirPlace& Out)
		{
			Out.bGround = true;
			Out.GroundZ = 0.f;
			Out.bOpenSky = bOpenSky;
			Out.Surface = Surface;
			Out.bInSun = bInSun;
			return true;
		};
	}

	/** Moves the air on by Seconds in frames, round View; true if the budget and every kind's cap held every frame. */
	inline bool Run(UAmbientAirSubsystem& Air, const FAirView& View, float Seconds)
	{
		bool bHeld = true;
		for (float Done = 0.f; Done < Seconds; Done += Frame)
		{
			Air.Step(Frame, View);
			bHeld &= Air.NumLive() <= AmbientAirRules::Budget;
			for (int32 Index = 0; Index < static_cast<int32>(EAirKind::Count); ++Index)
			{
				bHeld &= Air.CountOf(static_cast<EAirKind>(Index)) <= AmbientAirRules::Cap(static_cast<EAirKind>(Index));
			}
		}
		return bHeld;
	}

	/** A test's air: the level's own subsystem with the given trees and lamps (none: none near), nothing out. */
	inline UAmbientAirSubsystem* MakeAir(FAutomationTestBase& Test, FTestWorldWrapper& TestLevel, const TArray<FAirTree>& Trees,
		const TArray<FAirLamp>& Lamps)
	{
		if (!Test.TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
		{
			return nullptr;
		}
		UAmbientAirSubsystem* Air = TestLevel.GetTestWorld()->GetSubsystem<UAmbientAirSubsystem>();
		if (!Test.TestNotNull(TEXT("The air runs in test levels"), Air))
		{
			return nullptr;
		}
		Air->SetTrees(Trees);
		Air->SetLamps(Lamps);
		Air->Clear();
		return Air;
	}
}

using namespace AmbientAirTestKit;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAmbientAirRulesTest, "Looter.World.Air.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAmbientAirRulesTest::RunTest(const FString& Parameters)
{
	// Each kind belongs only where and when it should, how many are wanted follows the place, and all of them together
	// never pass the budget.
	using namespace AmbientAirRules;
	int32 Caps = 0;
	for (int32 Index = 0; Index < static_cast<int32>(EAirKind::Count); ++Index)
	{
		TestTrue(FString::Printf(TEXT("%s have a share of the budget"), Name(static_cast<EAirKind>(Index))), Cap(static_cast<EAirKind>(Index)) > 0);
		Caps += Cap(static_cast<EAirKind>(Index));
	}
	TestEqual(TEXT("The caps make the budget of 250"), Caps, Budget);
	TestTrue(TEXT("The budget is about 250"), Budget <= 250);

	const FAirView DayView = ViewAt(FVector::ZeroVector, Day);
	const FAirView DuskView = ViewAt(FVector::ZeroVector, Dusk);
	FAirView Covered = DayView;
	Covered.bUnderCover = true;
	FAirView Sheltered = DayView;
	Sheltered.bSheltered = true;
	const FAirView NoStates = ViewAt(FVector::ZeroVector, NAME_None);

	FAirPlace Sunny;
	Sunny.bGround = Sunny.bOpenSky = Sunny.bInSun = true;
	Sunny.Surface = EImpactMaterial::Grass;
	FAirPlace Shade = Sunny;
	Shade.bInSun = false;
	FAirPlace Road = Sunny;
	Road.Surface = EImpactMaterial::Dirt;
	FAirPlace Porch = Sunny;
	Porch.bOpenSky = false;
	FAirPlace Hayfield = Sunny;
	Hayfield.Surface = EImpactMaterial::Hay;

	// Motes: in the sun, by day.
	TestTrue(TEXT("Motes catch the sun by day"), Belongs(EAirKind::Mote, Road, DayView));
	TestTrue(TEXT("...in a level without lighting states too"), Belongs(EAirKind::Mote, Road, NoStates));
	TestTrue(TEXT("...and in a doorway's shaft, under a roof"), Belongs(EAirKind::Mote, Porch, Covered));
	TestFalse(TEXT("...never in the shade"), Belongs(EAirKind::Mote, Shade, DayView));
	TestFalse(TEXT("...nor at dusk"), Belongs(EAirKind::Mote, Sunny, DuskView));
	// Pollen and fluff: over open grass and hay.
	TestTrue(TEXT("Pollen over open grass by day"), Belongs(EAirKind::Pollen, Sunny, DayView));
	TestTrue(TEXT("...and in the grass's shade"), Belongs(EAirKind::Pollen, Shade, DayView));
	TestTrue(TEXT("...over the hayfields"), Belongs(EAirKind::Pollen, Hayfield, DayView));
	TestFalse(TEXT("...never over a dirt road"), Belongs(EAirKind::Pollen, Road, DayView));
	TestFalse(TEXT("...nor under a roof"), Belongs(EAirKind::Pollen, Porch, DayView));
	TestFalse(TEXT("...nor at dusk"), Belongs(EAirKind::Pollen, Sunny, DuskView));
	TestTrue(TEXT("Fluff over open grass at dusk too"), Belongs(EAirKind::Fluff, Sunny, DuskView));
	TestFalse(TEXT("...never over the road"), Belongs(EAirKind::Fluff, Road, DayView));
	// Leaves and moths come from their trees and lamps; nothing belongs in the mine.
	TestFalse(TEXT("Leaves aren't placed by a point"), Belongs(EAirKind::Leaf, Sunny, DayView));
	TestFalse(TEXT("...nor moths"), Belongs(EAirKind::Moth, Sunny, DuskView));
	for (int32 Index = 0; Index < static_cast<int32>(EAirKind::Count); ++Index)
	{
		TestFalse(TEXT("Nothing belongs in the mine"), Belongs(static_cast<EAirKind>(Index), Sunny, Sheltered));
	}

	// How many are wanted.
	FAirSurroundings Meadow;
	Meadow.GrassShare = 1.f;
	Meadow.SunShare = 1.f;
	FAirSurroundings Town;
	Town.GrassShare = 0.f;
	Town.SunShare = 1.f;
	FAirSurroundings Orchard = Meadow;
	Orchard.NearTrees = 12;
	FAirSurroundings Lamplit = Town;
	Lamplit.NearLamps = 2;
	TestEqual(TEXT("No pollen in town"), Target(EAirKind::Pollen, DayView, Town, 1.f), 0);
	TestEqual(TEXT("All the pollen over a meadow"), Target(EAirKind::Pollen, DayView, Meadow, 1.f), Cap(EAirKind::Pollen));
	TestEqual(TEXT("No leaves with no tree near"), Target(EAirKind::Leaf, DayView, Meadow, 1.f), 0);
	TestEqual(TEXT("All the leaves in the orchard"), Target(EAirKind::Leaf, DayView, Orchard, 1.f), Cap(EAirKind::Leaf));
	TestEqual(TEXT("No moths by day"), Target(EAirKind::Moth, DayView, Lamplit, 1.f), 0);
	TestEqual(TEXT("A few moths a lit lamp at dusk"), Target(EAirKind::Moth, DuskView, Lamplit, 1.f), 2 * MothsPerLamp);
	TestEqual(TEXT("No motes at dusk"), Target(EAirKind::Mote, DuskView, Meadow, 1.f), 0);
	TestTrue(TEXT("More motes under a roof than in the open"), Target(EAirKind::Mote, Covered, Meadow, 1.f) > Target(EAirKind::Mote, DayView, Meadow, 1.f));
	TestTrue(TEXT("Fewer fluff at dusk"), Target(EAirKind::Fluff, DuskView, Meadow, 1.f) < Target(EAirKind::Fluff, DayView, Meadow, 1.f));
	FAirSurroundings Everything = Orchard;
	Everything.NearLamps = 50;
	int32 AllWanted = 0;
	int32 AllSheltered = 0;
	for (int32 Index = 0; Index < static_cast<int32>(EAirKind::Count); ++Index)
	{
		const EAirKind Kind = static_cast<EAirKind>(Index);
		AllWanted += FMath::Max(Target(Kind, Covered, Everything, 5.f), Target(Kind, DuskView, Everything, 5.f));
		AllSheltered += Target(Kind, Sheltered, Everything, 1.f);
		TestEqual(TEXT("Density 0 wants none"), Target(Kind, Covered, Everything, 0.f), 0);
	}
	TestTrue(TEXT("Everything wanted at once stays within the budget, however dense it's asked"), AllWanted <= Budget);
	TestEqual(TEXT("None wanted in the mine"), AllSheltered, 0);

	// The trees whose leaves fall.
	TestTrue(TEXT("Oaks drop leaves"), IsBroadleaf(TEXT("SM_Oak_B")));
	TestTrue(TEXT("...birches"), IsBroadleaf(TEXT("SM_Birch_A")));
	TestTrue(TEXT("...the orchard's apples"), IsBroadleaf(TEXT("SM_Apple_A")));
	TestFalse(TEXT("Pines don't"), IsBroadleaf(TEXT("SM_Pine_A")));
	TestFalse(TEXT("...nor dead trees"), IsBroadleaf(TEXT("SM_DeadTree_A")));
	TestFalse(TEXT("...nor the far cards past the boundary"), IsBroadleaf(TEXT("SM_FarBroadleaf")));
	TestFalse(TEXT("...nor junipers"), IsBroadleaf(TEXT("SM_Juniper_B")));

	// Fades: whole inside the reach, gone at its edge and at the eye.
	TestEqual(TEXT("Whole well inside its reach"), DistanceFade(100.f, 1000.f, 200.f), 1.f);
	TestEqual(TEXT("Gone at its reach's edge"), DistanceFade(1000.f, 1000.f, 1000.f), 0.f);
	TestEqual(TEXT("Gone at the eye"), DistanceFade(5.f, 1000.f, 5.f), 0.f);
	TestEqual(TEXT("Fading in as it's born"), AgeFade(0.f, 10.f), 0.f);
	TestEqual(TEXT("Whole in its middle"), AgeFade(5.f, 10.f), 1.f);
	TestEqual(TEXT("Gone at its end"), AgeFade(10.f, 10.f), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAmbientAirBudgetTest, "Looter.World.Air.Budget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAmbientAirBudgetTest::RunTest(const FString& Parameters)
{
	// Everywhere a sunny meadow, an orchard round the player and ten lit lamps: everything that may come out does, never
	// more than its cap, never more than 250 in all; by day no moths, at dusk no motes or pollen and moths at the lamps.
	TArray<FAirTree> Trees;
	for (int32 Index = 0; Index < 20; ++Index)
	{
		FAirTree& Tree = Trees.AddDefaulted_GetRef();
		Tree.Base = FVector(FMath::Cos(Index * 0.7f) * 1200.f, FMath::Sin(Index * 0.7f) * 1200.f, 0.f);
		Tree.CrownRadius = 300.f;
		Tree.CrownBottom = 300.f;
		Tree.CrownTop = 900.f;
	}
	TArray<FAirLamp> Lamps;
	for (int32 Index = 0; Index < 10; ++Index)
	{
		FAirLamp& Lamp = Lamps.AddDefaulted_GetRef();
		Lamp.Location = FVector(Index * 400.f - 2000.f, 600.f, 300.f);
		Lamp.bLit = true;
	}
	FTestWorldWrapper TestLevel;
	UAmbientAirSubsystem* Air = MakeAir(*this, TestLevel, Trees, Lamps);
	if (!Air)
	{
		return false;
	}
	Air->SetPlaceProbe(Everywhere(EImpactMaterial::Grass, true));
	const FVector Eye(0.0, 0.0, 170.0);
	TestTrue(TEXT("By day: the budget and every cap hold, frame by frame"), Run(*Air, ViewAt(Eye, Day), 20.f));
	TestTrue(TEXT("...motes are out"), Air->CountOf(EAirKind::Mote) > 0);
	TestTrue(TEXT("...pollen"), Air->CountOf(EAirKind::Pollen) > 0);
	TestTrue(TEXT("...fluff"), Air->CountOf(EAirKind::Fluff) > 0);
	TestTrue(TEXT("...leaves under the trees"), Air->CountOf(EAirKind::Leaf) > 0);
	TestEqual(TEXT("...and no moths"), Air->CountOf(EAirKind::Moth), 0);
	TestTrue(TEXT("...a good many in all"), Air->NumLive() > 100);

	TestTrue(TEXT("At dusk: the budget and caps hold"), Run(*Air, ViewAt(Eye, Dusk), 6.f));
	TestEqual(TEXT("...the motes are gone"), Air->CountOf(EAirKind::Mote), 0);
	TestEqual(TEXT("...and the pollen"), Air->CountOf(EAirKind::Pollen), 0);
	TestTrue(TEXT("...moths have come to the lamps"), Air->CountOf(EAirKind::Moth) > 0);
	bool bAtLamps = true;
	for (const FAirParticle& Particle : Air->GetParticles())
	{
		if (Particle.bLive && Particle.Kind == EAirKind::Moth)
		{
			bAtLamps &= Air->GetLamps().IsValidIndex(Particle.Anchor)
				&& FVector::Dist(Particle.Location, Air->GetLamps()[Particle.Anchor].Location) < 200.0;
		}
	}
	TestTrue(TEXT("...each circling its lamp"), bAtLamps);
	Air->Clear();
	TestEqual(TEXT("Cleared, nothing's out"), Air->NumLive(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAmbientAirPlacesTest, "Looter.World.Air.Places",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAmbientAirPlacesTest::RunTest(const FString& Parameters)
{
	// Each kind only where it belongs: a sunny dirt road has motes and nothing of the meadow's; shady grass has pollen and
	// no motes; under a roof, motes in the sun and no pollen; leaves only under a tree and in its fall; none in the mine,
	// and none left behind when the player has gone.
	FAirTree Oak;
	Oak.Base = FVector(600.0, 0.0, 0.0);
	Oak.CrownRadius = 300.f;
	Oak.CrownBottom = 300.f;
	Oak.CrownTop = 900.f;
	FTestWorldWrapper TestLevel;
	UAmbientAirSubsystem* Air = MakeAir(*this, TestLevel, {}, {});
	if (!Air)
	{
		return false;
	}
	const FVector Eye(0.0, 0.0, 170.0);
	const FAirView DayView = ViewAt(Eye, Day);

	Air->SetPlaceProbe(Everywhere(EImpactMaterial::Dirt, true));
	Run(*Air, DayView, 10.f);
	TestTrue(TEXT("A sunny road: motes"), Air->CountOf(EAirKind::Mote) > 0);
	TestEqual(TEXT("...no pollen"), Air->CountOf(EAirKind::Pollen), 0);
	TestEqual(TEXT("...no fluff"), Air->CountOf(EAirKind::Fluff), 0);
	TestEqual(TEXT("...no leaves with no tree"), Air->CountOf(EAirKind::Leaf), 0);
	TestEqual(TEXT("...no moths by day"), Air->CountOf(EAirKind::Moth), 0);

	Air->Clear();
	Air->SetPlaceProbe(Everywhere(EImpactMaterial::Grass, false));
	Run(*Air, DayView, 10.f);
	TestTrue(TEXT("Shady grass: pollen"), Air->CountOf(EAirKind::Pollen) > 0);
	TestEqual(TEXT("...and no motes (only the sun shows them)"), Air->CountOf(EAirKind::Mote), 0);
	bool bOverGrass = true;
	for (const FAirParticle& Particle : Air->GetParticles())
	{
		if (Particle.bLive && (Particle.Kind == EAirKind::Pollen || Particle.Kind == EAirKind::Fluff))
		{
			bOverGrass &= Particle.Location.Z >= 0.0 && Particle.Location.Z < 600.0;
		}
	}
	TestTrue(TEXT("...low over the grass"), bOverGrass);

	Air->Clear();
	FAirView Porch = DayView;
	Porch.bUnderCover = true;
	Air->SetPlaceProbe(Everywhere(EImpactMaterial::Grass, true, /*bOpenSky*/ false));
	Run(*Air, Porch, 10.f);
	TestTrue(TEXT("Under a roof: motes in the sun"), Air->CountOf(EAirKind::Mote) > 0);
	TestEqual(TEXT("...no pollen"), Air->CountOf(EAirKind::Pollen), 0);
	bool bNear = true;
	for (const FAirParticle& Particle : Air->GetParticles())
	{
		bNear &= !Particle.bLive || FVector::Dist2D(Particle.Location, Eye) < AmbientAirRules::Reach(EAirKind::Mote, true) * 1.2;
	}
	TestTrue(TEXT("...kept near, where the shafts are"), bNear);

	// A tree: leaves from its crown, falling no higher than it and coming down on the ground.
	Air->Clear();
	Air->SetTrees({ Oak });
	Air->SetPlaceProbe(Everywhere(EImpactMaterial::Dirt, false));
	Run(*Air, DayView, 15.f);
	TestTrue(TEXT("Under an oak: leaves"), Air->CountOf(EAirKind::Leaf) > 0);
	bool bFromTree = true;
	bool bSomeLanded = false;
	for (const FAirParticle& Particle : Air->GetParticles())
	{
		if (Particle.bLive && Particle.Kind == EAirKind::Leaf)
		{
			bFromTree &= Particle.Location.Z <= Oak.CrownTop + 1.f && Particle.Location.Z >= -1.0
				&& FVector::Dist2D(Particle.Location, Oak.Base) < Oak.CrownRadius + 1200.f;
			bSomeLanded |= Particle.Landed >= 0.f;
		}
	}
	TestTrue(TEXT("...each from its crown, drifting down"), bFromTree);
	TestTrue(TEXT("...some lying on the ground a moment"), bSomeLanded || Air->CountOf(EAirKind::Leaf) > 0);

	// The mine: everything goes, nothing comes.
	FAirView Mine = DayView;
	Mine.bSheltered = true;
	Run(*Air, Mine, 1.f);
	TestEqual(TEXT("In the mine everything has gone within a second"), Air->NumLive(), 0);
	Run(*Air, Mine, 2.f);
	TestEqual(TEXT("...and nothing comes"), Air->NumLive(), 0);

	// The player gone 200 m off: nothing is left behind, and the new place fills.
	Air->SetPlaceProbe(Everywhere(EImpactMaterial::Grass, true));
	Run(*Air, DayView, 5.f);
	const FVector Away(20000.0, 0.0, 170.0);
	Run(*Air, ViewAt(Away, Day), 0.5f);
	bool bNoneBehind = true;
	for (const FAirParticle& Particle : Air->GetParticles())
	{
		bNoneBehind &= !Particle.bLive || FVector::Dist2D(Particle.Location, Away) < AmbientAirRules::Reach(Particle.Kind, false) * 1.2;
	}
	TestTrue(TEXT("Gone 200 m off: none left behind"), bNoneBehind);
	Run(*Air, ViewAt(Away, Day), 5.f);
	TestTrue(TEXT("...and the new place has its own"), Air->CountOf(EAirKind::Pollen) > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAmbientAirGustTest, "Looter.World.Air.CarriedByGusts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAmbientAirGustTest::RunTest(const FString& Parameters)
{
	// A gust carries the pollen and fluff along its way, far faster than the breeze.
	FTestWorldWrapper TestLevel;
	UAmbientAirSubsystem* Air = MakeAir(*this, TestLevel, {}, {});
	if (!Air)
	{
		return false;
	}
	Air->SetPlaceProbe(Everywhere(EImpactMaterial::Grass, true));
	FAirView View = ViewAt(FVector(0.0, 0.0, 170.0), Day);
	Run(*Air, View, 6.f);
	auto Along = [Air](const FVector& Way)
	{
		double Sum = 0.0;
		int32 Count = 0;
		for (const FAirParticle& Particle : Air->GetParticles())
		{
			if (Particle.bLive && (Particle.Kind == EAirKind::Pollen || Particle.Kind == EAirKind::Fluff))
			{
				Sum += FVector::DotProduct(Particle.Velocity, Way);
				++Count;
			}
		}
		return Count > 0 ? Sum / Count : 0.0;
	};
	const FVector East(0.0, 1.0, 0.0);
	const double Before = Along(East);
	View.GustDirection = East;
	View.GustStrength = 1.f;
	Run(*Air, View, 1.5f);
	const double During = Along(East);
	TestTrue(TEXT("In a full gust they race along its way"), During > 200.0);
	TestTrue(TEXT("...far faster than in the breeze"), During > Before + 150.0);
	return true;
}

#endif

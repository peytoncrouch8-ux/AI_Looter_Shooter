#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/HUD/HudMinimapWidget.h"
#include "World/GraveMarkRules.h"
#include "World/GraveTravelRules.h"
#include "World/RoostRules.h"
#include "World/RoostSettings.h"
#include "World/RoostVeil.h"

// Hob's roosts as plain rules (the user's call, 2026-10-13: the one save, respawn and fast-travel point on every map): the
// beacon's look over time, the crows over the screen as they carry Ellis, Hob's words, and where the minimap shows roosts.

namespace
{
	/** A 16:9 screen, as the veil's frame takes it (its width over its height). */
	constexpr float Wide = 16.f / 9.f;

	/** Whether a crow at this place is on the screen (its middle inside the frame). */
	bool OnScreen(const FRoostVeilCrow& Crow)
	{
		return FMath::Abs(Crow.Position.X) <= Wide * 0.5f && FMath::Abs(Crow.Position.Y) <= 0.5f;
	}

	/** How many of the veil's crows are on the screen now. */
	int32 CountOnScreen(const FRoostVeil& Veil)
	{
		TArray<FRoostVeilCrow> Crows;
		Veil.GetCrows(Wide, Crows);
		return Crows.FilterByPredicate(OnScreen).Num();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoostBeaconRulesTest, "Looter.Roost.Rules.Beacon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRoostBeaconRulesTest::RunTest(const FString& Parameters)
{
	// The beacon: gone near the camera and whole from afar, a column that shoots up as the roost is found, a pulse that
	// climbs it; one colour, kept off the loot's.
	using namespace RoostRules;
	TestEqual(TEXT("Under the beam: gone"), NearFade(300.f, 700.f, 3800.f), 0.f);
	TestEqual(TEXT("From afar: whole"), NearFade(9000.f, 700.f, 3800.f), 1.f);
	float Last = 0.f;
	bool bRising = true;
	for (float Distance = 0.f; Distance <= 5000.f; Distance += 100.f)
	{
		const float Fade = NearFade(Distance, 700.f, 3800.f);
		bRising = bRising && Fade >= Last - 0.0001f && Fade >= 0.f && Fade <= 1.f;
		Last = Fade;
	}
	TestTrue(TEXT("...and only grows with the distance"), bRising);

	TestEqual(TEXT("At rest the column stands whole"), BeamRise(-1.f), 1.f);
	TestEqual(TEXT("Found: it starts from the post"), BeamRise(0.f), 0.f);
	TestTrue(TEXT("...shoots up: most of its height in the first half second"), BeamRise(0.5f) > 0.6f);
	TestEqual(TEXT("...and stands whole by its end"), BeamRise(RiseSeconds), 1.f);
	Last = 0.f;
	bool bClimbing = true;
	for (float Age = 0.f; Age < RiseSeconds; Age += 0.05f)
	{
		bClimbing = bClimbing && BeamRise(Age) >= Last;
		Last = BeamRise(Age);
	}
	TestTrue(TEXT("...never sinking on the way"), bClimbing);

	float Height = 0.f;
	float Strength = 0.f;
	TestFalse(TEXT("No pulse before it"), PulseAt(-0.1f, Height, Strength));
	TestFalse(TEXT("...nor after"), PulseAt(PulseSeconds, Height, Strength));
	TestTrue(TEXT("Halfway: half way up or more, bright"), PulseAt(PulseSeconds * 0.5f, Height, Strength) && Height >= 0.5f && Strength > 0.9f);
	float Lower = -1.f;
	bool bUp = true;
	for (float Age = 0.f; Age < PulseSeconds; Age += 0.05f)
	{
		PulseAt(Age, Height, Strength);
		bUp = bUp && Height >= Lower && Height <= 1.f;
		Lower = Height;
	}
	TestTrue(TEXT("The pulse only climbs, post to top"), bUp);

	// One colour, off the loot's: not white, and not the rarity hues' (green, blue, purple, orange, gold).
	const FLinearColor Beacon = URoostSettings::Get().GetBeaconColor();
	const FLinearColor Hsv = Beacon.LinearRGBToHSV();
	TestTrue(TEXT("The beacon's colour is a colour (not white or grey: the ammo's beams are white)"), Hsv.G > 0.35f);
	TestTrue(TEXT("...and its beam reaches over the ridges"), URoostSettings::Get().BeamHeight >= 3000.f);
	TestTrue(TEXT("...and fades before the player stands under it"), URoostSettings::Get().NearGone < URoostSettings::Get().NearFull);
	TestTrue(TEXT("A roost is found from a road beside it"), GraveMarkRules::DefaultRadius >= 800.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoostVeilRulesTest, "Looter.Roost.Rules.Veil",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRoostVeilRulesTest::RunTest(const FString& Parameters)
{
	// The crows over the screen: a fast travel's carry covers the screen in its fade's time, holds black until the player
	// is moved, then bursts away; a death's gathers at the edges first. The same seed plays the same.
	FRoostVeil Travel;
	TestFalse(TEXT("Idle at first: nothing on the screen"), Travel.IsActive() || Travel.GetShade() > 0.f);
	Travel.Start(ERoostVeilKind::Travel, GraveTravelRules::FadeOutSeconds, 42);
	TestTrue(TEXT("A travel covers at once (no gathering)"), Travel.GetPhase() == ERoostVeilPhase::Cover);
	TestTrue(TEXT("...the world still seen as it starts"), Travel.GetShade() < 0.05f);
	Travel.Advance(GraveTravelRules::FadeOutSeconds * 0.5f);
	const int32 Midway = CountOnScreen(Travel);
	TestTrue(FString::Printf(TEXT("Halfway: the flock pours across (%d crows on the screen)"), Midway), Midway >= 15);
	float LastShade = Travel.GetShade();
	bool bDarkening = true;
	for (int32 Step = 0; Step < 20 && Travel.GetPhase() == ERoostVeilPhase::Cover; ++Step)
	{
		Travel.Advance(0.02f);
		bDarkening = bDarkening && Travel.GetShade() >= LastShade - 0.0001f;
		LastShade = Travel.GetShade();
	}
	TestTrue(TEXT("...the screen only darkening"), bDarkening);
	Travel.Advance(GraveTravelRules::FadeOutSeconds);
	TestTrue(TEXT("Black by the fade's end, and held"), Travel.GetPhase() == ERoostVeilPhase::Hold && Travel.GetShade() == 1.f);
	Travel.Advance(5.f);
	TestTrue(TEXT("...however long the move takes"), Travel.GetPhase() == ERoostVeilPhase::Hold);

	Travel.Reveal();
	TestTrue(TEXT("Revealed: the crows burst off"), Travel.GetPhase() == ERoostVeilPhase::Reveal && CountOnScreen(Travel) >= 20);
	Travel.Advance(FRoostVeil::RevealSeconds * 0.5f);
	TestTrue(TEXT("...the world back within the first half"), Travel.GetShade() < 0.05f);
	Travel.Advance(FRoostVeil::RevealSeconds);
	TestTrue(TEXT("...and every crow gone off the screen"), !Travel.IsActive() && CountOnScreen(Travel) == 0 && Travel.GetShade() == 0.f);

	// A death: the crows gather at the edges round the dying first, then cover.
	FRoostVeil Death;
	Death.Start(ERoostVeilKind::Death, 2.1f, 7);
	TestTrue(TEXT("A death gathers first"), Death.GetPhase() == ERoostVeilPhase::Gather);
	TestTrue(TEXT("...for at most its share of the time"), Death.GetPhaseLength() <= FRoostVeil::GatherSeconds && Death.GetPhaseLength() <= 2.1f * 0.56f);
	Death.Advance(0.8f);
	TArray<FRoostVeilCrow> Crows;
	Death.GetCrows(Wide, Crows);
	bool bAtEdges = Crows.Num() > 0;
	for (const FRoostVeilCrow& Crow : Crows)
	{
		// The middle third of the screen stays clear: they circle the edges.
		bAtEdges = bAtEdges && (FMath::Abs(Crow.Position.X) > Wide / 6.f || FMath::Abs(Crow.Position.Y) > 1.f / 6.f);
	}
	TestTrue(FString::Printf(TEXT("...circling at the screen's edges (%d crows), the middle clear"), Crows.Num()), bAtEdges);
	const float GatherShade = Death.GetShade();
	Death.Advance(Death.GetPhaseLength());
	TestTrue(TEXT("Then it covers, carrying on from the gather's dimming"), Death.GetPhase() == ERoostVeilPhase::Cover && Death.GetShade() >= GatherShade - 0.01f);
	Death.Advance(2.1f);
	TestTrue(TEXT("...black by the time asked"), Death.GetPhase() == ERoostVeilPhase::Hold);

	// Every crow from the seed: the same seed, the same flock.
	FRoostVeil One;
	FRoostVeil Two;
	One.Start(ERoostVeilKind::Travel, 0.75f, 99);
	Two.Start(ERoostVeilKind::Travel, 0.75f, 99);
	One.Advance(0.3f);
	Two.Advance(0.3f);
	TArray<FRoostVeilCrow> First;
	TArray<FRoostVeilCrow> Second;
	One.GetCrows(Wide, First);
	Two.GetCrows(Wide, Second);
	bool bSame = First.Num() == Second.Num() && First.Num() > 0;
	for (int32 Index = 0; bSame && Index < First.Num(); ++Index)
	{
		bSame = First[Index].Position.Equals(Second[Index].Position, 0.0001f) && First[Index].Frame == Second[Index].Frame;
	}
	TestTrue(TEXT("The same seed plays the same flock"), bSame);
	bool bSorted = true;
	for (int32 Index = 1; Index < First.Num(); ++Index)
	{
		bSorted = bSorted && First[Index - 1].Size <= First[Index].Size;
	}
	TestTrue(TEXT("...drawn far (small) to near (big)"), bSorted);

	// Hidden at once when a level goes.
	One.Stop();
	TestFalse(TEXT("Stopped: idle"), One.IsActive());
	TestTrue(TEXT("The edge: straight up from the middle meets the top"), FRoostVeil::EdgePoint(-UE_HALF_PI, Wide, 1.f).Equals(FVector2f(0.f, -0.5f), 0.001f));
	TestTrue(TEXT("...and straight right the side"), FRoostVeil::EdgePoint(0.f, Wide, 1.f).Equals(FVector2f(Wide * 0.5f, 0.f), 0.001f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoostLinesRulesTest, "Looter.Roost.Rules.Lines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRoostLinesRulesTest::RunTest(const FString& Parameters)
{
	// Hob's words when his crows bring Ellis home: his, all said once before any again, never the same twice running.
	using namespace RoostRules;
	const int32 Count = NumDeathLines();
	TestTrue(TEXT("A good handful of lines after a death"), Count >= 8);
	TArray<FString> Seen;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FStoryLine Line = DeathLine(Index);
		TestTrue(TEXT("Each is Hob's"), Line.Speaker.ToString() == HobName().ToString());
		TestFalse(TEXT("...and says something"), Line.Text.IsEmpty());
		TestFalse(TEXT("...and none twice"), Seen.Contains(Line.Text.ToString()));
		Seen.Add(Line.Text.ToString());
	}
	for (const FStoryLine& Once : { FirstDeathLine(), FirstRoostLine(), FirstTravelLine() })
	{
		TestTrue(TEXT("The once-only lines are Hob's and say something"), Once.Speaker.ToString() == HobName().ToString() && !Once.Text.IsEmpty());
	}
	TestTrue(TEXT("The once-only lines' flags are their own"), FirstDeathFlag != FirstRoostFlag && FirstRoostFlag != FirstTravelFlag);

	TArray<int32> Order;
	int32 LastSaid = INDEX_NONE;
	FRandomStream Random(1234);
	bool bNeverTwice = true;
	bool bRoundsWhole = true;
	for (int32 Round = 0; Round < 6; ++Round)
	{
		TSet<int32> Said;
		for (int32 Each = 0; Each < Count; ++Each)
		{
			const int32 Before = LastSaid;
			const int32 Next = NextLine(Order, LastSaid, Count, Random);
			bNeverTwice = bNeverTwice && Next != Before && Next == LastSaid;
			Said.Add(Next);
		}
		bRoundsWhole = bRoundsWhole && Said.Num() == Count;
	}
	TestTrue(TEXT("Every line once in a round before any again"), bRoundsWhole);
	TestTrue(TEXT("...and never the same one twice running, across rounds too"), bNeverTwice);
	TArray<int32> Single;
	int32 OnlyLast = INDEX_NONE;
	TestEqual(TEXT("One line: always that one"), NextLine(Single, OnlyLast, 1, Random), 0);
	TestEqual(TEXT("No lines: none"), NextLine(Single, OnlyLast, 0, Random), static_cast<int32>(INDEX_NONE));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoostMinimapRulesTest, "Looter.Roost.Rules.Minimap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRoostMinimapRulesTest::RunTest(const FString& Parameters)
{
	// The minimap's roosts: near ones at their place (found or not), found ones farther off on the rim toward them, the
	// nearest few within reach; one not found yet never on the rim. The map turns with the view (yaw 0: north up).
	const float PixelsPerCm = 80.f / 3500.f;
	const float Inside = 78.f;
	const float Rim = 76.f;
	TArray<FMinimapRoost> Roosts;
	Roosts.Add({ FVector(1000.0, 0.0, 0.0), false });      // 10 m north: on the map, not found
	Roosts.Add({ FVector(0.0, 2000.0, 0.0), true });        // 20 m east: on the map, found
	Roosts.Add({ FVector(6000.0, 0.0, 0.0), true });        // 60 m north: the rim
	Roosts.Add({ FVector(0.0, -7000.0, 0.0), true });       // 70 m west: the rim
	Roosts.Add({ FVector(-9000.0, 0.0, 0.0), true });       // 90 m south: the rim
	Roosts.Add({ FVector(-11000.0, 0.0, 0.0), true });      // 110 m south: a fourth, left off
	Roosts.Add({ FVector(0.0, 30000.0, 0.0), true });       // 300 m: past the rim's reach
	Roosts.Add({ FVector(-5000.0, -5000.0, 0.0), false });  // far, not found: never shown
	const TArray<FMinimapRoostSpot> Spots = UHudMinimapWidget::PlaceRoosts(Roosts, FVector::ZeroVector, 0.f, PixelsPerCm, Inside, Rim,
		UHudMinimapWidget::RoostRimReach, UHudMinimapWidget::RoostsOnRim);
	auto Find = [&Spots](int32 Index) { return Spots.FindByPredicate([Index](const FMinimapRoostSpot& Spot) { return Spot.Index == Index; }); };
	const FMinimapRoostSpot* NearUnfound = Find(0);
	TestTrue(TEXT("Near and not found: on the map, at its place (up the screen, north), dim"), NearUnfound && NearUnfound->bInside && !NearUnfound->bFound
		&& NearUnfound->Position.Y < 0.0 && FMath::IsNearlyZero(NearUnfound->Position.X, 0.01));
	const FMinimapRoostSpot* NearFound = Find(1);
	TestTrue(TEXT("Near and found: on the map (to the right, east), lit"), NearFound && NearFound->bInside && NearFound->bFound && NearFound->Position.X > 0.0);
	int32 OnRim = 0;
	for (const FMinimapRoostSpot& Spot : Spots)
	{
		if (!Spot.bInside)
		{
			++OnRim;
			TestTrue(TEXT("A rim roost sits on the rim"), FMath::IsNearlyEqual(Spot.Position.Size(), static_cast<double>(Rim), 0.01));
			TestTrue(TEXT("...found"), Spot.bFound);
		}
	}
	TestEqual(TEXT("The nearest three found ones on the rim"), OnRim, UHudMinimapWidget::RoostsOnRim);
	TestTrue(TEXT("...the north one at the rim's top"), Find(2) && !Find(2)->bInside && Find(2)->Position.Y < -70.0);
	TestNull(TEXT("A fourth, farther, left off"), Find(5));
	TestNull(TEXT("One past the rim's reach left off"), Find(6));
	TestNull(TEXT("One far and not found never shows"), Find(7));

	// Turned with the view: facing east, a roost east of the player is up the screen.
	const TArray<FMinimapRoostSpot> Turned = UHudMinimapWidget::PlaceRoosts(Roosts, FVector::ZeroVector, 90.f, PixelsPerCm, Inside, Rim,
		UHudMinimapWidget::RoostRimReach, UHudMinimapWidget::RoostsOnRim);
	const FMinimapRoostSpot* East = Turned.FindByPredicate([](const FMinimapRoostSpot& Spot) { return Spot.Index == 1; });
	TestTrue(TEXT("Facing east, the east roost is up the screen"), East && East->Position.Y < 0.0 && FMath::Abs(East->Position.X) < 1.0);
	return true;
}

#endif

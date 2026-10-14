#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Tests/FaunaTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "World/GraveMarkRules.h"
#include "World/GraveTravelRules.h"
#include "World/GraveTravelSubsystem.h"
#include "World/RespawnMarker.h"
#include "World/RoostBeacon.h"
#include "World/RoostCarrySubsystem.h"
#include "World/RoostCrows.h"
#include "World/RoostKit.h"
#include "World/RoostRules.h"
#include "World/RoostSettings.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Hob's roosts in test levels (the user's call, 2026-10-13): the roost's look (the greybox until SM_HobRoost is in, the post
// at the player's front left, the beacon dark until found, then rising), its crows, being found, and the crows' carry for a
// fast travel and a death with Hob's words.

using namespace MissionTestWorld;

namespace
{
	const FName Valley(TEXT("TestValley"));

	/** A roost in a test level, named and opened (or not) before its play begins. */
	ARespawnMarker* PlaceRoost(UWorld* World, FName Id, const FVector& Where, double Yaw, bool bOpenFromStart)
	{
		ARespawnMarker* Roost = World->SpawnActor<ARespawnMarker>(Where, FRotator(0.0, Yaw, 0.0));
		if (Roost)
		{
			Roost->MarkerId = Id;
			Roost->bStartActive = bOpenFromStart;
		}
		return Roost;
	}

	/** Its crows made and on their perches, as play begins them (a test level never begins its actors' play). */
	ARoostCrows* ReadyCrows(ARespawnMarker& Roost)
	{
		Roost.DispatchBeginPlay();
		ARoostCrows* Birds = Roost.GetCrows();
		if (Birds)
		{
			Birds->SetupBirds();
		}
		return Birds;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoostLookTest, "Looter.Roost.Play.Look",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRoostLookTest::RunTest(const FString& Parameters)
{
	// The roost as it stands: the player's spot is the actor's, the post at their front left with its perches and its beacon
	// on top; dark until found, then the column rising out of the post and the light flaring and easing; the column gone
	// under the camera and whole from afar; seen across the map (never culled), its light shadowless.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ARespawnMarker* Roost = PlaceRoost(World, TEXT("TestRoost"), FVector(1000.0, 0.0, 0.0), 0.0, true);
	if (!TestTrue(TEXT("A roost with its beacon and light"), Roost && Roost->Roost && Roost->Beacon && Roost->BeaconLight))
	{
		return false;
	}
	if (!Roost->HasModel())
	{
		AddInfo(TEXT("SM_HobRoost isn't imported: the greybox stands in."));
		TestTrue(TEXT("The greybox: mast, yard, crosstrees, nest and arms in wood, the basket in iron, the cairn in stone"),
			Roost->GreyboxWood->GetInstanceCount() == 6 && Roost->GreyboxIron->GetInstanceCount() == 1 && Roost->GreyboxStone->GetInstanceCount() == 1
			&& Roost->GreyboxWood->IsVisible());
	}
	else
	{
		TestTrue(TEXT("The model in, the greybox gone"), Roost->GreyboxWood->GetInstanceCount() == 0 && !Roost->GreyboxWood->IsVisible());
	}

	// Waking here: on its spot, facing its arrow, out over the land with the mast at their back.
	const FRespawnWakeSpot Spot = Roost->GetWakeSpot();
	TestTrue(TEXT("The player wakes on the roost's own spot"), Spot.Grave == Roost && Spot.Location.Equals(Roost->GetActorLocation()));
	FTransform Foot;
	if (TestTrue(TEXT("The mast's foot"), Roost->GetRoostSocket(NAME_None, Foot)))
	{
		const FVector Toward = Foot.GetLocation() - Roost->GetActorLocation();
		const double Bearing = FMath::RadiansToDegrees(FMath::Atan2(Toward.Y, Toward.X));
		const double Apart = Toward.Size2D();
		TestTrue(FString::Printf(TEXT("...beside the spot, not on it (%.0f cm)"), Apart), Apart > 100.0 && Apart < 450.0);
		TestTrue(FString::Printf(TEXT("...at their back as they wake, facing out (%.0f degrees off the arrow)"), Bearing), FMath::Abs(Bearing) > 120.0);
	}
	const FVector Beacon = Roost->GetBeaconLocation();
	TestTrue(TEXT("The beacon on the roost's top, well over a man"), Beacon.Z - Roost->GetActorLocation().Z > 400.0);
	const TArray<FTransform> Perches = Roost->GetPerches();
	TestEqual(TEXT("Six perches for Hob's crows"), Perches.Num(), RoostKit::PerchCount);
	bool bHigh = true;
	for (const FTransform& Perch : Perches)
	{
		bHigh = bHigh && Perch.GetLocation().Z - Roost->GetActorLocation().Z > 200.0;
	}
	TestTrue(TEXT("...all up on the roost, over the player's head"), bHigh);
	FTransform Hook;
	TestTrue(TEXT("Its bell hangs from its socket, free of collision"), Roost->GetRoostSocket(RoostKit::BellSocket(), Hook)
		&& Roost->Bell->GetStaticMesh() != nullptr && !Roost->Bell->IsCollisionEnabled()
		&& FVector::Dist(Roost->Bell->GetComponentLocation(), Hook.GetLocation()) < 40.0);
	TestEqual(TEXT("...at rest"), Roost->GetBellSwing(), 0.f);

	// Dark until found.
	URoostBeaconComponent* Column = Roost->Beacon;
	TestTrue(TEXT("Not found: dark (no column, no light)"), !Roost->IsLit() && !Column->IsVisible() && !Roost->BeaconLight->IsVisible()
		&& Roost->GetLightCandela() == 0.f && Column->GetBeamLength() == 0.f);
	TestTrue(TEXT("Seen across the map: never culled by distance"), Column->bNeverDistanceCull && Column->LDMaxDrawDistance == 0.f
		&& Column->InstanceEndCullDistance == 0);
	TestTrue(TEXT("...one draw of its cards"), Column->GetInstanceCount() == URoostBeaconComponent::CardCount);
	TestTrue(TEXT("Its light: shadowless, small, gone beyond a long walk"), !Roost->BeaconLight->CastShadows
		&& Roost->BeaconLight->AttenuationRadius <= 1000.f && Roost->BeaconLight->MaxDrawDistance > 0.f);

	// Found: the column shoots up out of the post, the light catches and spikes, then both settle, lit for good.
	const URoostSettings& Settings = URoostSettings::Get();
	Roost->StartFlare(EGraveMark::First);
	TestTrue(TEXT("Found: lit, its column rising from the post"), Roost->IsLit() && Column->IsVisible() && Column->IsShowing()
		&& Column->GetBeamLength() < Settings.BeamHeight * 0.1f);
	TestTrue(TEXT("...the light catching from nothing"), Roost->GetLightCandela() == 0.f && Roost->BeaconLight->IsVisible());
	Roost->AdvanceLook(GraveMarkRules::FlareRiseSeconds);
	Column->Advance(GraveMarkRules::FlareRiseSeconds);
	TestTrue(TEXT("...and spiking within a breath"), Roost->GetLightCandela() > Settings.LightCandela * 3.f);
	// Its bell rings out and swings on its loop (a test level never ticks: moved on as the tick would).
	float Widest = 0.f;
	for (int32 Frame = 0; Frame < 30; ++Frame)
	{
		Roost->AdvanceLook(1.f / 30.f);
		Widest = FMath::Max(Widest, FMath::Abs(Roost->GetBellSwing()));
	}
	TestTrue(FString::Printf(TEXT("...the bell rung, swinging (%.0f degrees at most)"), Widest), Widest > 8.f);
	for (int32 Frame = 0; Frame < 300; ++Frame)
	{
		Roost->AdvanceLook(1.f / 30.f);
	}
	TestEqual(TEXT("...and settling"), Roost->GetBellSwing(), 0.f);
	Column->Advance(RoostRules::RiseSeconds);
	Roost->AdvanceLook(GraveMarkRules::FirstFlareSeconds);
	Column->Advance(RoostRules::PulseSeconds);
	TestTrue(TEXT("Then the column stands whole, the show over"), FMath::IsNearlyEqual(Column->GetBeamLength(), Settings.BeamHeight, 1.f) && !Column->IsShowing());
	TestTrue(TEXT("...the light at its rest, lit for good"), !Roost->IsFlaring() && FMath::IsNearlyEqual(Roost->GetLightCandela(), Settings.LightCandela, 0.01f));

	// The camera near it: the column fades away (the flame stays); far off, whole again.
	Column->SetViewDistance(300.f);
	TestTrue(TEXT("Under it: the column gone, the flame still burning"), Column->GetBeamStrength() == 0.f && Column->GetFlameStrength() > 0.f);
	Column->SetViewDistance(20000.f);
	TestTrue(TEXT("From afar: the column whole"), FMath::IsNearlyEqual(Column->GetBeamStrength(), Settings.BeamStrength, 0.01f));

	// Made the save point again: a pulse climbs it, the light lifts but not as high.
	Roost->StartFlare(EGraveMark::Again);
	TestTrue(TEXT("Again: a pulse climbing from the post"), Column->IsShowing() && Column->GetPulseHeight() >= 0.f && Column->GetPulseHeight() < 0.1f);
	Roost->AdvanceLook(GraveMarkRules::FlareRiseSeconds);
	TestTrue(TEXT("...the light lifting, not as a find's"), Roost->GetLightCandela() > Settings.LightCandela && Roost->GetLightCandela() < Settings.LightCandela * 3.f);

	// Dark again at once (the console takes a find back).
	Roost->SetLit(false);
	TestTrue(TEXT("Dark again"), !Roost->IsLit() && !Column->IsVisible() && !Roost->BeaconLight->IsVisible() && Roost->GetLightCandela() == 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoostCrowsTest, "Looter.Roost.Play.Crows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRoostCrowsTest::RunTest(const FString& Parameters)
{
	// Hob's crows: made as the roost's play begins, on its perches; the player walking right up doesn't scare them; found,
	// they lift off, wheel once and settle again within a few seconds, though the player still stands there.
	FaunaTestWorld::FFaunaSetting On(1);
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ARespawnMarker* Roost = PlaceRoost(World, TEXT("TestRoost"), FVector(0.0, 0.0, 0.0), 45.0, true);
	ARoostCrows* Birds = Roost ? ReadyCrows(*Roost) : nullptr;
	if (!TestTrue(TEXT("The roost made its crows"), Roost && Birds))
	{
		return false;
	}
	TestTrue(TEXT("...its own, on its perches"), Birds->GetOwner() == Roost && Birds->Perches.Num() == RoostKit::PerchCount);
	const int32 Count = FMath::Min(URoostSettings::Get().CrowCount, RoostKit::PerchCount);
	TestEqual(TEXT("...as many as the settings say"), Birds->GetBirds().Num(), Count);
	TestEqual(TEXT("...all perched"), Birds->CountPerched(), Count);

	// The player standing on the roost's spot (2.8 m from the post) for a while: Hob's crows stay put.
	const FVector Spot = Roost->GetActorLocation() + FVector(0.0, 0.0, 90.0);
	auto Standing = [&Spot](double)
	{
		FFaunaContext Context;
		Context.Threats.Add(FaunaTestWorld::Walker(Spot));
		return Context;
	};
	double Now = 0.0;
	FaunaTestWorld::Run(*Birds, 3.f, Now, Standing);
	TestFalse(TEXT("The player walking up doesn't scare them"), Birds->IsFlushed());

	// Found: they lift off as its light catches...
	const double LiftedAt = Now;
	Birds->Lift(Spot);
	FaunaTestWorld::Run(*Birds, 1.f, Now, Standing);
	TestTrue(TEXT("Found: the crows lift off"), Birds->IsFlushed() && Birds->CountPerched() < Count);
	// ...and settle again within a few seconds, the player still there: the moment they're all down is measured. Settled,
	// a crow hops to a free perch now and then (9-20 s on, the flock's settled life), so one may be between perches later.
	double SettledAt = -1.0;
	for (int32 Step = 0; Step < 14 * 30; ++Step)
	{
		FaunaTestWorld::Run(*Birds, FaunaTestWorld::Frame, Now, Standing);
		if (SettledAt < 0.0 && !Birds->IsFlushed() && Birds->CountPerched() == Count)
		{
			SettledAt = Now;
		}
	}
	TestTrue(FString::Printf(TEXT("...and settle again within a few seconds (all down %.1f s after the lift)"), SettledAt - LiftedAt),
		SettledAt >= 0.0 && SettledAt - LiftedAt < 10.0);
	int32 Down = 0;
	for (const FFaunaBird& Bird : Birds->GetBirds())
	{
		Down += Bird.State == EFaunaBirdState::Perched || Bird.State == EFaunaBirdState::Hopping ? 1 : 0;
	}
	TestTrue(FString::Printf(TEXT("...and stay settled (%d of %d perched or hopping between perches)"), Down, Count),
		!Birds->IsFlushed() && Down == Count);

	// The roost going takes its crows with it.
	Roost->Destroy();
	TestTrue(TEXT("Its crows go with it"), !IsValid(Birds) || Birds->IsActorBeingDestroyed());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoostFoundTest, "Looter.Roost.Play.Found",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRoostFoundTest::RunTest(const FString& Parameters)
{
	// Walking past a roost beside a road finds it: its column rises, its crows lift, and the story's first roost has Hob say
	// what they're for (once in a story); found, it's on the map's list (a travel point) and lit when the level begins again.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector(9000.0, 9000.0, 0.0));
	ARespawnMarker* Farm = PlaceRoost(World, TEXT("FamilyPlot"), FVector(0.0, 0.0, 0.0), 0.0, false);
	ARespawnMarker* Street = PlaceRoost(World, TEXT("MainStreet"), FVector(6000.0, 0.0, 0.0), 0.0, false);
	if (!TestTrue(TEXT("The runner, the player and two roosts"), Runner && Player && Farm && Street))
	{
		return false;
	}
	Runner->BeginForTesting({}, Campaign, Player, Valley);
	ARoostCrows* FarmCrows = ReadyCrows(*Farm);
	ReadyCrows(*Street);
	TestFalse(TEXT("Not found yet: dark, not open, not on the map"), Farm->IsLit() || Farm->IsActive(Campaign) || Farm->IsFound(Campaign));

	// Passing by on the road beside it, 7 m off: found.
	Player->SetActorLocation(Farm->GetActorLocation() + FVector(0.0, 700.0, 0.0));
	TestTrue(TEXT("Passing 7 m off on the road: found"), Farm->TryMark(Player) == EGraveMark::First);
	TestTrue(TEXT("...its column rising, its light caught"), Farm->IsLit() && Farm->Beacon->IsShowing() && Farm->IsFlaring());
	TestTrue(TEXT("...its crows lifting"), !FarmCrows || FarmCrows->IsFlushed());
	const int32 Circling = URoostSettings::Get().CirclingCrows;
	const ARoostCrows* Wheeling = Farm->GetCirclingCrows();
	TestTrue(TEXT("...and a few of Hob's crows circling its beacon, never landing"), Circling == 0 || (Wheeling
		&& Wheeling->Mode == EFaunaFlockMode::Aerial && Wheeling->BirdCount == Circling
		&& Wheeling->AerialArea.Center.Equals(Farm->GetBeaconLocation(), 1.0)));
	TestNull(TEXT("A roost still dark has none circling"), Street->GetCirclingCrows());
	TestTrue(TEXT("...open, found and the save point"), Farm->IsActive(Campaign) && Farm->IsFound(Campaign) && Campaign.GetLastGrave(Valley) == FName(TEXT("FamilyPlot")));
	TestTrue(TEXT("...the story's first roost: Hob says what they're for, once"), Campaign.HasFlag(RoostRules::FirstRoostFlag));

	// The next one found: the save point moves, Hob says nothing more of it.
	const int32 FlagsBefore = Campaign.StoryFlags.Num();
	Player->SetActorLocation(Street->GetActorLocation() + FVector(300.0, 0.0, 0.0));
	TestTrue(TEXT("Main Street found too"), Street->TryMark(Player) == EGraveMark::First && Campaign.GetLastGrave(Valley) == FName(TEXT("MainStreet")));
	TestEqual(TEXT("...no second first-roost word"), Campaign.StoryFlags.Num(), FlagsBefore);

	// Back at the farm: the save point again, a pulse up its column, no banner.
	Player->SetActorLocation(Farm->GetActorLocation() + FVector(0.0, -300.0, 0.0));
	Farm->Beacon->Advance(5.f);
	Farm->AdvanceLook(5.f);
	TestTrue(TEXT("Back at the farm: the save point again"), Farm->TryMark(Player) == EGraveMark::Again && Farm->Beacon->GetPulseHeight() >= 0.f);

	// A level beginning again with it found: lit from the start, no show.
	ARespawnMarker* Again = PlaceRoost(World, TEXT("FamilyPlot"), FVector(-6000.0, 0.0, 0.0), 0.0, false);
	if (TestNotNull(TEXT("The farm's roost placed again"), Again))
	{
		Again->DispatchBeginPlay();
		TestTrue(TEXT("Found before: lit as it begins, no show"), Again->IsLit() && !Again->IsFlaring() && !Again->Beacon->IsShowing());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoostCarryTest, "Looter.Roost.Play.Carry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRoostCarryTest::RunTest(const FString& Parameters)
{
	// Hob's crows carry Ellis: a fast travel's crows cover the screen in the fade's time and hold it black until the player
	// stands at the roost, then burst away as the roost's own crows scatter off it; Hob speaks after the first ride only,
	// and after every death (the first death its own line, then a round, never the same twice running). A death in a ride's
	// black keeps the crows covering until the wake-up.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UGraveTravelSubsystem* Travel = World->GetSubsystem<UGraveTravelSubsystem>();
	URoostCarrySubsystem* Carry = World->GetSubsystem<URoostCarrySubsystem>();
	ACharacter* Player = World->SpawnActor<ACharacter>(FVector(0.0, 0.0, 100.0), FRotator::ZeroRotator);
	ARespawnMarker* Far = PlaceRoost(World, TEXT("FarRoost"), FVector(6000.0, 0.0, 0.0), 90.0, true);
	if (!TestTrue(TEXT("The runner, travel, carry, player and a far roost"), Runner && Travel && Carry && Player && Far))
	{
		return false;
	}
	Runner->BeginForTesting({}, Campaign, Player, Valley);
	ARoostCrows* FarCrows = ReadyCrows(*Far);

	// Off: the crows pour across the screen until it's black.
	TestTrue(TEXT("Fast travel: on the way"), Travel->TravelTo(Player, *Far));
	TestTrue(TEXT("...the crows covering the screen"), Carry->GetVeil().GetKind() == ERoostVeilKind::Travel
		&& Carry->GetVeil().GetPhase() == ERoostVeilPhase::Cover);
	Carry->Advance(GraveTravelRules::FadeOutSeconds);
	TestTrue(TEXT("...black by the fade's end, held"), Carry->GetVeil().GetPhase() == ERoostVeilPhase::Hold);
	// A death in the black doesn't start the crows over: they hold until the wake-up.
	Carry->BeginDeath(nullptr, 2.f);
	TestTrue(TEXT("A death in the black: still held, not gathering again"), Carry->GetVeil().GetPhase() == ERoostVeilPhase::Hold);

	// Set down at the roost: the crows burst away, the roost's own scatter off round the player, Hob's first-ride word.
	Travel->Advance(GraveTravelRules::FadeOutSeconds + GraveTravelRules::HoldSeconds);
	TestTrue(TEXT("Arrived at the roost"), !Travel->IsTravelling()
		&& Player->GetActorLocation().Equals(GraveTravelRules::ArrivalLocation(Far->GetActorLocation(), Player->GetDefaultHalfHeight()), 1.0));
	TestTrue(TEXT("...the crows bursting off the screen"), Carry->GetVeil().GetPhase() == ERoostVeilPhase::Reveal);
	TestTrue(TEXT("...the roost's own scattering off it"), !FarCrows || FarCrows->IsFlushed());
	TestTrue(TEXT("...and Hob's word on the first ride"), Campaign.HasFlag(RoostRules::FirstTravelFlag));
	Carry->Advance(FRoostVeil::RevealSeconds + 0.1f);
	TestFalse(TEXT("...then gone, the world back"), Carry->GetVeil().IsActive());
	TestNull(TEXT("The second ride: Hob says nothing"), Carry->SayOnArrival(ERoostArrival::Travel));

	// A death's words: the first its own, then his round, never the same twice running.
	const FStoryLine* First = Carry->SayOnArrival(ERoostArrival::Death);
	TestTrue(TEXT("The first death: Hob's own line for it"), First && First->Text.ToString() == RoostRules::FirstDeathLine().Text.ToString());
	FString Last;
	bool bNeverTwice = true;
	for (int32 Death = 0; Death < RoostRules::NumDeathLines() * 2; ++Death)
	{
		const FStoryLine* Said = Carry->SayOnArrival(ERoostArrival::Death);
		bNeverTwice = bNeverTwice && Said && Said->Text.ToString() != Last;
		Last = Said ? Said->Text.ToString() : FString();
	}
	TestTrue(TEXT("...then a word after every death, never the same twice running"), bNeverTwice);

	// A death's carry: the crows gather at the screen's edges first.
	Carry->BeginDeath(nullptr, 2.1f);
	TestTrue(TEXT("A death: the crows gather round the dying"), Carry->GetVeil().GetKind() == ERoostVeilKind::Death
		&& Carry->GetVeil().GetPhase() == ERoostVeilPhase::Gather);
	Carry->Reveal();
	Carry->Advance(FRoostVeil::RevealSeconds + 0.1f);
	TestFalse(TEXT("...and burst away at the wake-up"), Carry->GetVeil().IsActive());
	return true;
}

#endif

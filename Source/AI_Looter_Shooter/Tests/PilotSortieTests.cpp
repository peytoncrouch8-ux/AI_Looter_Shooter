#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/CompanyTender.h"
#include "World/LamplighterLauncher.h"
#include "World/LamplighterShip.h"
#include "World/LandfallWinch.h"
#include "World/PilotSortie.h"
#include "World/PilotSortieRules.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"

// The Lamplighter's part of Main 6, "The Pilot Light" (lane MAIN67, W2; Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section
// 10): her sortie's rules, the winch's cast off, the escorts' gas-bags, Junie's launchers and the plank, and a whole run from
// the ledge out and home again, in a test level with no barge (the sortie's BerthWithoutBarge stands in for W3's).

namespace
{
	constexpr float PilotFrame = 1.f / 30.f;

	/** The ledge she lies on, and her course out east (a little climb, a turn, the barge's side at its end). */
	FTransform PilotMoored()
	{
		return FTransform(FRotator(0.0, 0.0, 0.0), FVector(4000.0, 3000.0, 0.0));
	}

	TArray<FVector> PilotCourse()
	{
		return { FVector(4600.0, 3000.0, 1100.0), FVector(9000.0, 4500.0, 1500.0), FVector(14000.0, 4000.0, 1600.0) };
	}

	FTransform PilotBerth()
	{
		return FTransform(FRotator(0.0, 10.0, 0.0), FVector(17000.0, 4500.0, 1600.0));
	}

	ALamplighterShip* SpawnPilotShip(UWorld* World)
	{
		const FTransform Moored = PilotMoored();
		ALamplighterShip* Ship = World->SpawnActorDeferred<ALamplighterShip>(ALamplighterShip::StaticClass(), Moored);
		if (!Ship)
		{
			return nullptr;
		}
		Ship->Moored = Moored;
		Ship->bSpawnCrew = false;
		Ship->bSpawnBoarders = false;
		Ship->FinishSpawning(Moored);
		Ship->DispatchBeginPlay();
		// Down on the ledge and anchored, as Main 6 finds her.
		Ship->SetFlight(EShipFlight::Moored);
		Ship->SetAnchored(false);
		return Ship;
	}

	APilotSortie* SpawnPilotSortie(UWorld* World, ALamplighterShip* Ship, bool bTenders)
	{
		APilotSortie* Sortie = World->SpawnActorDeferred<APilotSortie>(APilotSortie::StaticClass(), FTransform::Identity);
		if (!Sortie)
		{
			return nullptr;
		}
		Sortie->Ship = Ship;
		Sortie->Course = PilotCourse();
		Sortie->BerthWithoutBarge = PilotBerth();
		Sortie->bSpawnTenders = bTenders;
		Sortie->bSpawnBoarders = false;
		Sortie->bFollowStory = false;
		Sortie->FinishSpawning(FTransform::Identity);
		Sortie->DispatchBeginPlay();
		return Sortie;
	}

	/** Her frames: her own tick drives the sortie (APilotSortie::DriveShip from ALamplighterShip::Advance). */
	void AdvancePilot(ALamplighterShip& Ship, float Seconds)
	{
		for (float Done = 0.f; Done < Seconds - KINDA_SMALL_NUMBER; Done += PilotFrame)
		{
			Ship.Advance(FMath::Min(PilotFrame, Seconds - Done));
		}
	}

	/** A shot on Part of Victim as the bullet subsystem deals one, its actor's own events let through (a test level). */
	void ShootPilotPart(AActor* Victim, UPrimitiveComponent* Part, float Damage)
	{
		FEditorScriptExecutionGuard RunActorEvents;
		FHitResult Hit;
		Hit.Component = Part;
		Hit.Location = Part ? Part->GetComponentLocation() : FVector::ZeroVector;
		Hit.ImpactPoint = Hit.Location;
		UGameplayStatics::ApplyPointDamage(Victim, Damage, FVector::ForwardVector, Hit, nullptr, nullptr, UDamageType::StaticClass());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPilotSortieCourseTest, "Looter.World.PilotSortie.Course",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPilotSortieCourseTest::RunTest(const FString& Parameters)
{
	using namespace PilotLight;
	// Where a level finds her from Main 6's step.
	TestTrue(TEXT("Not Main 6: moored"), PhaseForStory(false, INDEX_NONE, false) == EPilotPhase::Idle);
	TestTrue(TEXT("Its first step: waiting to cast off"), PhaseForStory(true, CastOffStep, false) == EPilotPhase::WaitCastOff);
	TestTrue(TEXT("The tenders' and the launchers' steps: flying again from the lift"), PhaseForStory(true, TendersStep, false) == EPilotPhase::LiftOff
		&& PhaseForStory(true, GrappleStep, false) == EPilotPhase::LiftOff);
	TestTrue(TEXT("The barge's steps: in the berth"), PhaseForStory(true, DeckStep, false) == EPilotPhase::Boarded
		&& PhaseForStory(true, CageStep, false) == EPilotPhase::Boarded);
	TestTrue(TEXT("The escape: under the gangway"), PhaseForStory(true, EscapeStep, false) == EPilotPhase::Jump);
	TestTrue(TEXT("Ready to turn in: moored"), PhaseForStory(true, ReadyStep, true) == EPilotPhase::Idle);
	TestTrue(TEXT("Flying keeps the player aboard; the berth and the jump don't"), KeepsPlayerAboard(EPilotPhase::Cruise)
		&& !KeepsPlayerAboard(EPilotPhase::Boarded) && !KeepsPlayerAboard(EPilotPhase::Jump) && IsFlying(EPilotPhase::Jump));
	EPilotPhase Parsed = EPilotPhase::Idle;
	TestTrue(TEXT("The console's words"), ParsePhase(TEXT("grappled"), Parsed) && Parsed == EPilotPhase::Boarded
		&& ParsePhase(TEXT("Moored"), Parsed) && Parsed == EPilotPhase::Idle && !ParsePhase(TEXT("Nowhere"), Parsed));
	TestEqual(TEXT("The contract's names"), TendersDownEvent(), FName(TEXT("Pilot.TendersDown")));
	TestEqual(TEXT("...the launchers' tag"), LauncherTag(), FName(TEXT("Lamplighter_GrappleLauncher")));
	TestEqual(TEXT("...the deck's marker"), DeckMarkerTag(), FName(TEXT("Lamplighter_Deck")));
	TestTrue(TEXT("Two lines for two tenders down"), TenderDownLines(1) != TenderDownLines(2));

	// She brakes to a stop at her course's end, never past it.
	float Distance = 0.f;
	float Speed = 0.f;
	const float Length = 20000.f;
	for (float Clock = 0.f; Clock < 120.f; Clock += PilotFrame)
	{
		Speed = FLamplighterCourse::SpeedToward(Speed, SpeedFor(Length - Distance, CruiseSpeed), PilotFrame);
		Distance = FMath::Min(Distance + Speed * PilotFrame, Length);
	}
	TestTrue(TEXT("She gets to her course's end and stops there"), Distance >= Length - 50.f && Speed < 40.f);
	TestTrue(TEXT("Never faster than she can stop in"), SpeedFor(100.f, CruiseSpeed) < CruiseSpeed && SpeedFor(1.0e6f, CruiseSpeed) == CruiseSpeed);

	// The lift: up first, out and round later, exactly at the course's start at its end.
	const FTransform Moored = PilotMoored();
	const FTransform Start(FRotator(0.0, 40.0, 0.0), FVector(4600.0, 3000.0, 1100.0));
	TestTrue(TEXT("The lift starts on the ledge"), LiftPose(Moored, Start, 0.f).GetLocation().Equals(Moored.GetLocation(), 0.5));
	TestTrue(TEXT("...and ends at her course's start"), LiftPose(Moored, Start, 1.f).GetLocation().Equals(Start.GetLocation(), 0.5));
	const FVector Early = LiftPose(Moored, Start, 0.3f).GetLocation();
	TestTrue(TEXT("...rising before she moves out (clear of the cliff)"), Early.Z - Moored.GetLocation().Z > LiftRise * 0.6
		&& FVector::Dist2D(Early, Moored.GetLocation()) < FVector::Dist2D(Start.GetLocation(), Moored.GetLocation()) * 0.2);

	// The standoff: 15 m to her port (away from the barge on her starboard), a little higher.
	const FTransform Berth = PilotBerth();
	const FVector Off = Berth.InverseTransformPositionNoScale(StandoffFor(Berth).GetLocation());
	TestTrue(TEXT("The standoff is off her port beam"), FMath::IsNearlyEqual(Off.Y, -StandoffOut, 1.0) && FMath::IsNearlyEqual(Off.Z, StandoffUp, 1.0));
	TestTrue(TEXT("A glide ends where it's going"), Glide(Moored, Berth, 1.f).Equals(FTransform(Berth.GetRotation(), Berth.GetLocation()), 0.5));
	TestTrue(TEXT("The settle ends on the ledge"), SettlePose(Berth, Moored, 1.f).GetLocation().Equals(Moored.GetLocation(), 0.5));

	// The jump's catch: beside her, falling, under her deck; never while they're over on the barge's side or standing.
	const float Deck = ALamplighterShip::DeckTop;
	const float Rail = ALamplighterShip::RailOut;
	TestTrue(TEXT("Fallen past her side: caught"), IsCaughtBelow(FVector(0.0, Rail + 100.0, Deck - 400.0), true, Deck, Rail));
	TestFalse(TEXT("Over on the barge (in its hull): not caught"), IsCaughtBelow(FVector(0.0, Rail + 900.0, Deck - 400.0), true, Deck, Rail));
	TestFalse(TEXT("Not falling: not caught"), IsCaughtBelow(FVector(0.0, 0.0, Deck - 400.0), false, Deck, Rail));
	TestFalse(TEXT("Above her deck: not caught"), IsCaughtBelow(FVector(0.0, 0.0, Deck + 300.0), true, Deck, Rail));

	TestTrue(TEXT("A gas-bag grows tougher with the area's enemies"), BagHealthAt(FightLevel) > BagHealthAt(1) && BagHealthAt(1) >= BagBaseHealth - 0.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPilotSortieCastOffTest, "Looter.World.PilotSortie.CastOff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPilotSortieCastOffTest::RunTest(const FString& Parameters)
{
	// Main 6's first step: moored and anchored, the winch offers no cast off without its story (Landfall's winch is as it
	// was), Junie's launchers are on her starboard rail; the cast off reels the chain in and the sortie lifts her clear of the
	// ledge, up first, then out onto her course.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ALamplighterShip* Ship = SpawnPilotShip(World);
	APilotSortie* Sortie = Ship ? SpawnPilotSortie(World, Ship, /*bTenders*/ false) : nullptr;
	ALandfallWinch* Winch = nullptr;
	for (TActorIterator<ALandfallWinch> It(World); It; ++It)
	{
		Winch = *It;
	}
	if (!TestNotNull(TEXT("The ship"), Ship) || !TestNotNull(TEXT("The sortie"), Sortie) || !TestNotNull(TEXT("Her winch"), Winch))
	{
		return false;
	}
	TestTrue(TEXT("Moored and anchored"), Ship->GetFlight() == EShipFlight::Moored && Ship->IsAnchored());
	TestFalse(TEXT("No cast off without its story (Landfall's winch only drops the anchor)"), Winch->CanCastOff());
	Sortie->SetPhase(EPilotPhase::WaitCastOff);
	TestTrue(TEXT("Waiting to cast off"), Sortie->GetPhase() == EPilotPhase::WaitCastOff && Ship->GetFlight() == EShipFlight::Moored);
	TestTrue(TEXT("Junie's two launchers on her rail, tagged for the missions"), Sortie->GetLauncher(0) && Sortie->GetLauncher(1)
		&& Sortie->GetLauncher(0)->ActorHasTag(PilotLight::LauncherTag()) && Sortie->GetLauncher(1)->GetAttachParentActor() == Ship);
	TestFalse(TEXT("...not armed on the ledge"), Sortie->GetLauncher(0)->CanFire());

	TestTrue(TEXT("Cast off (forced: no missions here)"), Winch->CastOff(/*bForce*/ true));
	TestTrue(TEXT("...the chain in"), !Winch->Chain->IsVisible());
	TestTrue(TEXT("...she's the sortie's, lifting"), Ship->GetFlight() == EShipFlight::Sortie && Sortie->GetPhase() == EPilotPhase::LiftOff);
	AdvancePilot(*Ship, PilotLight::LiftSeconds * 0.35f);
	const FVector Rising = Ship->GetActorLocation();
	TestTrue(TEXT("Up first, barely out"), Rising.Z - PilotMoored().GetLocation().Z > PilotLight::LiftRise * 0.4
		&& FVector::Dist2D(Rising, PilotMoored().GetLocation()) < 300.0);
	AdvancePilot(*Ship, PilotLight::LiftSeconds * 0.7f + 0.2f);
	TestTrue(TEXT("Then onto her course"), Sortie->GetPhase() == EPilotPhase::Cruise
		&& Ship->GetActorLocation().Equals(PilotCourse()[0], 120.0));
	TestNotNull(TEXT("Her deck's marker rides with her"), [World, Ship]() -> AActor*
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(PilotLight::DeckMarkerTag()) && It->GetAttachParentActor() == Ship)
			{
				return *It;
			}
		}
		return nullptr;
	}());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPilotSortieTenderBagTest, "Looter.World.PilotSortie.TenderBag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPilotSortieTenderBagTest::RunTest(const FString& Parameters)
{
	// An escort keeps its station on her; shots on its gas-bag leak it past half, then tear it: it falls. Landfall's tender's
	// bag takes no shot at all.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ALamplighterShip* Ship = SpawnPilotShip(World);
	if (!TestNotNull(TEXT("The ship"), Ship))
	{
		return false;
	}
	const FVector Station(300.0, -2300.0, 420.0);
	ACompanyTender* Escort = ACompanyTender::SpawnEscort(*Ship, FVector(-6000.0, -3600.0, 2000.0), Station, -1.f, 2.f, 100.f);
	if (!TestNotNull(TEXT("An escort"), Escort))
	{
		return false;
	}
	for (float Done = 0.f; Done < 2.5f; Done += PilotFrame)
	{
		Escort->Advance(PilotFrame);
	}
	TestTrue(TEXT("On station off her port beam"), Escort->IsAlongside() && Escort->IsEscort()
		&& FVector::Dist(Ship->GetCalmTransform().InverseTransformPosition(Escort->GetActorLocation()), Station) < 30.0);
	ShootPilotPart(Escort, Escort->BagHit, 40.f);
	TestNearlyEqual(TEXT("A shot on its bag counts"), Escort->GetBagShare(), 0.4f, 0.01f);
	ShootPilotPart(Escort, Escort->GetRootComponent() ? Cast<UPrimitiveComponent>(Escort->Hull.Get()) : nullptr, 40.f);
	TestNearlyEqual(TEXT("...one on its hull doesn't"), Escort->GetBagShare(), 0.4f, 0.01f);
	ShootPilotPart(Escort, Escort->BagHit, 70.f);
	TestTrue(TEXT("Torn: it falls"), Escort->IsBagBurst() && Escort->IsFalling() && Escort->IsDown());
	TestTrue(TEXT("...its bag takes no more"), Escort->BagHit->GetCollisionEnabled() == ECollisionEnabled::NoCollision);

	ACompanyTender* Landfall = ACompanyTender::SpawnFor(*Ship);
	if (TestNotNull(TEXT("Landfall's tender"), Landfall))
	{
		ShootPilotPart(Landfall, Landfall->BagHit, 500.f);
		TestTrue(TEXT("Landfall's tender's bag takes no shot"), !Landfall->IsEscort() && Landfall->GetBagShare() == 0.f && !Landfall->IsFalling());
		TestFalse(TEXT("...nor can it be torn"), Landfall->BurstBag(nullptr));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPilotSortieLaunchersTest, "Looter.World.PilotSortie.Launchers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPilotSortieLaunchersTest::RunTest(const FString& Parameters)
{
	// Alongside her standoff the launchers arm; both fired bite the barge's rail; the chains haul her into the berth, where
	// she's dead still, the plank across; the escape cuts them.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ALamplighterShip* Ship = SpawnPilotShip(World);
	APilotSortie* Sortie = Ship ? SpawnPilotSortie(World, Ship, /*bTenders*/ false) : nullptr;
	if (!TestNotNull(TEXT("The ship"), Ship) || !TestNotNull(TEXT("The sortie"), Sortie))
	{
		return false;
	}
	Sortie->SetPhase(EPilotPhase::Alongside);
	AdvancePilot(*Ship, 0.5f);
	ALamplighterLauncher* Fore = Sortie->GetLauncher(0);
	ALamplighterLauncher* Aft = Sortie->GetLauncher(1);
	if (!TestTrue(TEXT("Alongside, both launchers"), Ship->GetFlight() == EShipFlight::Sortie && Fore && Aft))
	{
		return false;
	}
	TestTrue(TEXT("...armed"), Fore->CanFire() && Aft->CanFire());
	const FVector Standoff = PilotLight::StandoffFor(PilotBerth()).GetLocation();
	TestTrue(TEXT("...she holds her standoff (her bob only)"), Ship->GetActorLocation().Equals(Standoff, 40.0));
	TestEqual(TEXT("Both fire"), Sortie->FireLaunchers(), 2);
	AdvancePilot(*Ship, ALamplighterLauncher::FlightSeconds + 0.2f);
	TestEqual(TEXT("...and bite the rail"), Sortie->NumLaunchersBiting(), 2);
	TestTrue(TEXT("...the chains haul her in"), Sortie->GetPhase() == EPilotPhase::Pulling);
	AdvancePilot(*Ship, PilotLight::PullSeconds + 0.2f);
	TestTrue(TEXT("In the berth"), Sortie->GetPhase() == EPilotPhase::Boarded && Ship->GetActorLocation().Equals(PilotBerth().GetLocation(), 1.0));
	const FTransform Still = Ship->GetActorTransform();
	AdvancePilot(*Ship, 2.f);
	TestTrue(TEXT("...dead still there"), Ship->GetActorTransform().Equals(Still, 0.01));
	TestTrue(TEXT("...the plank down and walkable"), Sortie->Plank->IsVisible()
		&& Sortie->PlankFloor->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics);

	Sortie->SetPhase(EPilotPhase::Jump, /*bFresh*/ false);
	TestTrue(TEXT("The escape cuts the grapples"), Fore->GetState() == ELauncherState::Cut && Aft->GetState() == ELauncherState::Cut);
	AdvancePilot(*Ship, 1.f);
	TestTrue(TEXT("...the plank comes up"), Sortie->PlankFloor->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	AdvancePilot(*Ship, PilotLight::JumpGlideSeconds);
	TestTrue(TEXT("...and she's under the gangway"), Ship->GetActorLocation().Equals(Sortie->GetJumpBerth().GetLocation(), 40.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPilotSortiePhasesTest, "Looter.World.PilotSortie.Phases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPilotSortiePhasesTest::RunTest(const FString& Parameters)
{
	// A whole run: off the ledge, her course out (the tenders called and, with none here, down at once), up to her standoff;
	// then home on her course reversed, set down on the ledge at Moored, anchored, and Landfall's moored ship again.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ALamplighterShip* Ship = SpawnPilotShip(World);
	APilotSortie* Sortie = Ship ? SpawnPilotSortie(World, Ship, /*bTenders*/ false) : nullptr;
	if (!TestNotNull(TEXT("The ship"), Ship) || !TestNotNull(TEXT("The sortie"), Sortie))
	{
		return false;
	}
	TestTrue(TEXT("Her course built"), Sortie->GetCourse().IsValid());
	Sortie->SetPhase(EPilotPhase::LiftOff);
	float Waited = 0.f;
	while (Sortie->GetPhase() != EPilotPhase::Approach && Waited < 120.f)
	{
		AdvancePilot(*Ship, 0.5f);
		Waited += 0.5f;
	}
	TestTrue(TEXT("Out along her course, the tenders called and down"), Sortie->AreTendersCalled() && Sortie->AreTendersDown());
	TestTrue(TEXT("...to the approach"), Sortie->GetPhase() == EPilotPhase::Approach && Sortie->GetDistance() >= Sortie->GetCourse().GetLength() - 2.f);
	AdvancePilot(*Ship, PilotLight::ApproachSeconds + 0.2f);
	TestTrue(TEXT("Up to her standoff"), Sortie->GetPhase() == EPilotPhase::Alongside);

	Sortie->SetPhase(EPilotPhase::Home, /*bFresh*/ false);
	Waited = 0.f;
	while (Sortie->GetPhase() != EPilotPhase::Idle && Waited < 180.f)
	{
		AdvancePilot(*Ship, 0.5f);
		Waited += 0.5f;
	}
	TestTrue(TEXT("Home and set down"), Sortie->GetPhase() == EPilotPhase::Idle && Ship->GetFlight() == EShipFlight::Moored);
	TestTrue(TEXT("...exactly at Moored, anchored"), Ship->GetActorTransform().Equals(PilotMoored(), 0.5) && Ship->IsAnchored());
	TestTrue(TEXT("...the launchers put away"), Sortie->GetLauncher(0) == nullptr);
	return true;
}

#endif

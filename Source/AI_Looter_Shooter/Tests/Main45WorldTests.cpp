#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterGroup.h"
#include "Creatures/RappelSpawner.h"
#include "Creatures/UnpaidCreature.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionTypes.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/InteractionTestWorld.h"
#include "World/ChapelReliquary.h"
#include "World/ChapelWindowHose.h"
#include "World/CompanyTenderDrop.h"
#include "World/IntakeCharge.h"
#include "World/SinkIntake.h"
#include "World/SinkIntakeRules.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Tests/AutomationCommon.h"

// Mains 4-5's world pieces (the Wardens story, 2026-11-01, lanes/MAIN45.md), each on its own in a test level with no story:
// the chapel window's hose and its pull, the drop tender's flight, the ropes' slide, the intake's rules, charges and choke.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChapelHosePullTest, "Looter.World.ChapelHose.Pull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FChapelHosePullTest::RunTest(const FString& Parameters)
{
	// Its line in through the window: from the sill to its end; drinking the Reliquary (her ember lower); a 2 s hold pulls it;
	// it whips out and lies on the yard's ground; once out, nothing more to pull.
	const FVector End(-300.0, 40.0, -60.0);
	TestTrue(TEXT("Its line starts at the sill and ends at its end"), AChapelWindowHose::InsideCurve(End, 130.f, 0.f).IsNearlyZero()
		&& AChapelWindowHose::InsideCurve(End, 130.f, 1.f).Equals(End, 0.1));
	TestTrue(TEXT("...dipping to the nave's floor on the way"), AChapelWindowHose::InsideCurve(End, 130.f, 0.4f).Z < -80.0);

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	AChapelReliquary* Reliquary = World->SpawnActor<AChapelReliquary>(FVector(-300.0, 0.0, -170.0), FRotator::ZeroRotator);
	AChapelWindowHose* Hose = World->SpawnActor<AChapelWindowHose>(FVector::ZeroVector, FRotator::ZeroRotator);
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);
	if (!Reliquary || !Hose || !Player || !Interaction)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Reliquary->DispatchBeginPlay();
	Hose->DispatchBeginPlay();
	TestTrue(TEXT("In her window, drinking her"), Hose->IsInWindow() && Reliquary->IsDrinking());
	TestTrue(TEXT("...its nozzle on her ember"), FVector::Dist(Hose->GetNozzle(), Reliquary->GetEmberStart()) < 40.0);
	const FInteractionOptions Options = Hose->GetInteractionOptions(*Interaction);
	TestTrue(TEXT("A hold, 2 s, \"Pull out the hose\""), Options.bUsable && Options.bHold && !Options.bTap
		&& FMath::IsNearlyEqual(Options.HoldSeconds, 2.f) && Options.HoldPrompt.ToString() == TEXT("Pull out the hose"));
	TestTrue(TEXT("Pulled"), Hose->Pull(nullptr));
	TestTrue(TEXT("...whipping out, and she isn't drunk now"), !Hose->IsInWindow() && Hose->IsWhipping() && !Reliquary->IsDrinking());
	Hose->Advance(AChapelWindowHose::WhipSeconds + 0.05f);
	TestTrue(TEXT("...lying on the yard's ground under the window"), !Hose->IsWhipping()
		&& FVector::Dist(Hose->GetNozzle(), Hose->GetActorTransform().TransformPosition(Hose->PulledRest)) < 30.0);
	TestFalse(TEXT("Out: nothing more to pull"), Hose->Pull(nullptr) || Hose->GetInteractionOptions(*Interaction).bUsable);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTenderDropFlightTest, "Looter.World.TenderDrop.Flight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTenderDropFlightTest::RunTest(const FString& Parameters)
{
	// Its run in from the barge to the hover, rising over the midway; hanging there with its ropes; back once told to leave.
	const FVector From(0.0, 0.0, 5000.0);
	const FVector To(10000.0, 0.0, 1500.0);
	TestTrue(TEXT("Its run starts at the barge and ends at the hover"), ACompanyTenderDrop::RunAt(From, To, 0.f, 1800.f).Equals(From)
		&& ACompanyTenderDrop::RunAt(From, To, 1.f, 1800.f).Equals(To));
	TestTrue(TEXT("...rising over the straight line midway"), ACompanyTenderDrop::RunAt(From, To, 0.5f, 1800.f).Z > (From.Z + To.Z) * 0.5 + 1500.0);

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ACompanyTenderDrop* Tender = World->SpawnActor<ACompanyTenderDrop>(To, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("The tender"), Tender))
	{
		return false;
	}
	Tender->ComeFrom = From;
	Tender->HoverAt = To;
	Tender->HoverYaw = 90.f;
	Tender->DispatchBeginPlay();
	TestTrue(TEXT("Not called: away"), Tender->GetDropState() == ETenderDropState::Away);
	Tender->Come(true);
	TestTrue(TEXT("Called: flying in"), Tender->GetDropState() == ETenderDropState::Coming);
	Tender->Advance(Tender->FlySeconds + 0.1f);
	TestTrue(TEXT("Its run done: hovering over the drop"), Tender->IsHovering() && FVector::Dist(Tender->GetActorLocation(), To) < 100.0);
	TestTrue(TEXT("...turned broadside, its ropes hanging off its sides"), FMath::IsNearlyEqual(Tender->GetActorRotation().Yaw, 90.0, 1.0)
		&& Tender->NumRopeAnchors() == 4 && Tender->GetRopeAnchor(0).Z < Tender->GetActorLocation().Z);
	Tender->Leave();
	Tender->Advance(Tender->FlySeconds + 0.1f);
	TestTrue(TEXT("Told to leave: back to the barge, and away"), Tender->GetDropState() == ETenderDropState::Away);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRappelSlideTest, "Looter.Creatures.Rappel.Slide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRappelSlideTest::RunTest(const FString& Parameters)
{
	// The slide: quick at first, braked at the bottom; a drop's men lifted to their ropes' tops, held passive, slid to their
	// feet, then walking and on the player.
	TestTrue(TEXT("A slide starts at the top and ends on the ground"), FMath::IsNearlyZero(ARappelSpawner::SlideAlpha(0.f, 2.f))
		&& FMath::IsNearlyEqual(ARappelSpawner::SlideAlpha(2.f, 2.f), 1.f, 1e-3f));
	float Last = 0.f;
	bool bRising = true;
	for (int32 Step = 1; Step <= 20; ++Step)
	{
		const float Now = ARappelSpawner::SlideAlpha(Step * 0.1f, 2.f);
		bRising &= Now >= Last;
		Last = Now;
	}
	TestTrue(TEXT("...always downward"), bRising);
	TestTrue(TEXT("...most of the way in its first two thirds"), ARappelSpawner::SlideAlpha(1.4f, 2.f) >= 0.8f);

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(800.0, 0.0, 0.0));
	ARappelSpawner* Ropes = World->SpawnActor<ARappelSpawner>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!Player || !Ropes)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Ropes->SpawnerId = TEXT("TestRopes");
	Ropes->Groups = { EncounterTestWorld::MakeGroup(AUnpaidCreature::StaticClass(), 2) };
	Ropes->RopeTops = { FVector(0.0, 0.0, 1500.0), FVector(0.0, 300.0, 1500.0) };
	Ropes->SpawnPoints = { FVector(0.0, 0.0, 0.0), FVector(0.0, 300.0, 0.0) };
	Ropes->bHuntOnSpawn = true;
	Ropes->ActivationRadius = 3000.f;
	Ropes->DispatchBeginPlay();
	Ropes->SetRandomSeed(20261101);
	Ropes->UpdateEncounter(0.5f);
	const TArray<ACreatureBase*> Men = Ropes->GetAliveCreatures();
	if (!TestEqual(TEXT("Two down the ropes"), Men.Num(), 2))
	{
		return true;
	}
	TestTrue(TEXT("...each up at his rope's top, held passive"), Ropes->IsSliding() && !Men.ContainsByPredicate([](const ACreatureBase* Man)
	{
		return Man->GetActorLocation().Z < 1200.0 || !Man->IsPassive();
	}));
	Ropes->AdvanceRopes(Ropes->SlideSeconds * 0.5f);
	TestTrue(TEXT("Halfway: on the way down"), !Men.ContainsByPredicate([](const ACreatureBase* Man)
	{
		return Man->GetActorLocation().Z > 1300.0 || Man->GetActorLocation().Z < 100.0;
	}));
	Ropes->AdvanceRopes(Ropes->SlideSeconds);
	TestTrue(TEXT("Down: walking, no longer passive, on the player"), !Ropes->IsSliding() && !Men.ContainsByPredicate([Player](const ACreatureBase* Man)
	{
		const UCharacterMovementComponent* Movement = Man->GetCharacterMovement();
		return Man->IsPassive() || (Movement && Movement->MovementMode == MOVE_Flying) || Man->GetTarget() != Player;
	}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSinkIntakeDrawTest, "Looter.World.SinkIntake.Draw",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSinkIntakeDrawTest::RunTest(const FString& Parameters)
{
	// The draw: nothing past its edge, rising toward the mouth, doubled by the fuse but never more than the player's own walk;
	// the hose's shapes: the swing kicked out and dying, never through the floor, settling limp; the second hose's ends.
	using namespace SinkIntakeRules;
	TestEqual(TEXT("Past the draw's edge: no pull"), DrawInput(1001.f, 1000.f, 300.f, 1.f), 0.f);
	TestTrue(TEXT("At its edge: a light pull"), FMath::IsNearlyEqual(DrawInput(999.f, 1000.f, 300.f, 1.f), EdgeDraw, 0.01f));
	TestTrue(TEXT("Inside the drain: the mouth's pull"), FMath::IsNearlyEqual(DrawInput(100.f, 1000.f, 300.f, 1.f), MouthDraw, 1e-4f));
	TestTrue(TEXT("Rising toward the mouth"), DrawInput(500.f, 1000.f, 300.f, 1.f) > DrawInput(800.f, 1000.f, 300.f, 1.f));
	TestTrue(TEXT("Fused: doubled, capped under the player's own walk"), DrawInput(100.f, 1000.f, 300.f, 2.f) == MostDraw && MostDraw < 1.f
		&& DrawInput(800.f, 1000.f, 300.f, 2.f) > DrawInput(800.f, 1000.f, 300.f, 1.f));

	TestTrue(TEXT("The swing starts hanging"), FMath::IsNearlyZero(SwingAngle(0.f)));
	TestTrue(TEXT("...kicked out wide, then dying away"), FMath::Abs(SwingAngle(0.5f)) > 30.f && FMath::Abs(SwingAngle(6.f)) < 10.f);
	const FIntakeHoseShape Rest = RestShape(FVector(0.0, 0.0, 5000.0), FVector(600.0, 0.0, 3000.0), FVector(360.0, 0.0, 1500.0), FVector(0.0, 0.0, 200.0));
	const FIntakeHoseShape Limp = LimpShape(Rest.P0, FVector(900.0, 0.0, 60.0));
	TestTrue(TEXT("Limp: hanging from the drum, its end lying where it was put"), Limp.At(0.f).Equals(Rest.P0) && Limp.At(1.f).Equals(FVector(900.0, 0.0, 60.0)));
	bool bAboveFloor = true;
	for (int32 Step = 0; Step <= 40; ++Step)
	{
		bAboveFloor &= SwingShape(Rest, Limp, FVector(0.0, 1.0, 0.0), 0.f, Step * 0.1f, 4.f).P3.Z >= 39.0;
	}
	TestTrue(TEXT("Swinging: its end never through the floor"), bAboveFloor);
	TestTrue(TEXT("...and settled into the limp shape at its end"), SwingShape(Rest, Limp, FVector(0.0, 1.0, 0.0), 0.f, 4.f, 4.f).P3.Equals(Limp.P3, 1.0));
	const FIntakeHoseShape Second = SecondShape(FVector(0.0, 0.0, 6000.0), FVector(22000.0, 0.0, 7000.0));
	TestTrue(TEXT("The second hose from the barge to the shaft, sagging between"), Second.At(0.f).Equals(FVector(0.0, 0.0, 6000.0))
		&& Second.At(1.f).Equals(FVector(22000.0, 0.0, 7000.0)) && Second.At(0.5f).Z < 6500.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSinkIntakeChokeTest, "Looter.World.SinkIntake.Choke",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSinkIntakeChokeTest::RunTest(const FString& Parameters)
{
	// Three charges on its collar (a 1.5 s hold each, one a spot): the third lights the fuse; FuseSeconds later it blows, the
	// hose bucks and swings, then hangs limp; the charges blown.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ASinkIntake* Intake = World->SpawnActor<ASinkIntake>(FVector::ZeroVector, FRotator::ZeroRotator);
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);
	TArray<AIntakeCharge*> Clamps;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Clamps.Add(World->SpawnActor<AIntakeCharge>(FRotator(0.0, Index * 120.0, 0.0).Vector() * 185.0, FRotator(0.0, Index * 120.0, 0.0)));
	}
	if (!Intake || !Player || !Interaction || Clamps.Contains(nullptr))
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Intake->HoseTop = FVector(0.0, 0.0, 5000.0);
	Intake->HoseC1 = FVector(600.0, 0.0, 3000.0);
	Intake->HoseC2 = FVector(360.0, 0.0, 1500.0);
	Intake->HoseBottom = FVector(0.0, 0.0, 200.0);
	Intake->LimpRest = FVector(900.0, 0.0, 60.0);
	Intake->DispatchBeginPlay();
	for (AIntakeCharge* Clamp : Clamps)
	{
		Clamp->DispatchBeginPlay();
	}
	TestTrue(TEXT("Drinking, the hose Landfall's"), Intake->GetIntakeState() == EIntakeState::Drinking && !Intake->DrawsHose());
	const FInteractionOptions Options = Clamps[0]->GetInteractionOptions(*Interaction);
	TestTrue(TEXT("A clamp spot: a 1.5 s hold, \"Set a salt charge\""), Options.bHold && !Options.bTap
		&& FMath::IsNearlyEqual(Options.HoldSeconds, 1.5f) && Options.HoldPrompt.ToString() == TEXT("Set a salt charge"));
	TestTrue(TEXT("The first charge on"), Clamps[0]->SetCharge(Player) && Intake->GetChargesSet() == 1);
	TestFalse(TEXT("...a set spot can't take another"), Clamps[0]->SetCharge(Player) || Clamps[0]->GetInteractionOptions(*Interaction).bUsable);
	TestTrue(TEXT("The second on: still drinking"), Clamps[1]->SetCharge(Player) && Intake->GetIntakeState() == EIntakeState::Drinking);
	TestTrue(TEXT("The third on: the fuse burns, the draw doubled"), Clamps[2]->SetCharge(Player) && Intake->GetIntakeState() == EIntakeState::Fused
		&& Clamps[2]->IsLit() && Intake->IsDrawing() && Intake->GetDrawOn(500.f) > SinkIntakeRules::DrawInput(500.f, 1000.f, 300.f, 1.f));
	Intake->Advance(Intake->FuseSeconds - 0.5f);
	TestTrue(TEXT("Half a second short: still burning"), Intake->GetIntakeState() == EIntakeState::Fused);
	Intake->Advance(0.6f);
	TestTrue(TEXT("Blown: the hose its own now, bucking"), Intake->GetIntakeState() == EIntakeState::Swinging && Intake->DrawsHose()
		&& !Clamps[0]->IsLit() && !Intake->IsDrawing());
	TestTrue(TEXT("...its end torn off the mouth"), Intake->GetHoseShape().P3.Z >= 40.0);
	Intake->Advance(ASinkIntake::BuckSeconds + Intake->SwingSeconds + 0.1f);
	TestTrue(TEXT("Swung out: hanging limp, its end on the floor"), Intake->GetIntakeState() == EIntakeState::Limp
		&& Intake->GetHoseShape().P3.Equals(Intake->LimpRest, 1.0));
	TestFalse(TEXT("...and it can't be choked twice"), Intake->Choke());
	return true;
}

#endif

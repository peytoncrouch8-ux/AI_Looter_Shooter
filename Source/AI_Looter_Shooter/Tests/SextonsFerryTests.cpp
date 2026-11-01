#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Scenes/FerryLadderClimb.h"
#include "Story/MisterSexton.h"
#include "World/FallRecoverySubsystem.h"
#include "World/FerryLadder.h"
#include "World/SextonsFerry.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// The Ferryman's landing, his ferry and the rope ladder down to it (Main 2, The Ferryman's Price; lane MAIN23): the
// landing's safety over the canyon, the ferry carrying Sexton as it sways, the ladder's two ends, the climb's path.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSextonsFerryLandingTest, "Looter.World.SextonsFerry.Landing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSextonsFerryLandingTest::RunTest(const FString& Parameters)
{
	// The landing hangs less far under the deck than the fall recovery's open-edge drop (standing on it is never a fall),
	// its four sides walled for pawns over its rails; Sexton rides in the stern and sways with it.
	const UFallRecoverySubsystem* Recovery = GetDefault<UFallRecoverySubsystem>();
	TestTrue(FString::Printf(TEXT("The landing is %.0f cm under the boards, under the open-edge drop (%.0f)"), ASextonsFerry::ClimbDrop,
		Recovery->OpenEdgeDropHeight), ASextonsFerry::ClimbDrop + 60.f < Recovery->OpenEdgeDropHeight);

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ASextonsFerry* Ferry = World->SpawnActor<ASextonsFerry>(FVector::ZeroVector, FRotator::ZeroRotator);
	AMisterSexton* Sexton = World->SpawnActor<AMisterSexton>(FVector(1000.0, 1000.0, 0.0), FRotator::ZeroRotator);
	if (!Ferry || !Sexton)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	TestEqual(TEXT("Walled on all four sides"), Ferry->Blockers.Num(), 4);
	bool bPawnsOnly = true;
	bool bTallEnough = true;
	for (const UBoxComponent* Blocker : Ferry->Blockers)
	{
		bPawnsOnly &= Blocker && Blocker->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block
			&& Blocker->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Ignore
			&& Blocker->GetCollisionResponseToChannel(ECC_GameTraceChannel2) == ECR_Ignore;
		bTallEnough &= Blocker && Blocker->GetUnscaledBoxExtent().Z * 2.0 >= 180.0;
	}
	TestTrue(TEXT("...for pawns only (shots, sight and the Interact key's line pass)"), bPawnsOnly);
	TestTrue(TEXT("...too tall to jump"), bTallEnough);
	TestTrue(TEXT("Its planks are solid"), Ferry->Landing->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);

	Ferry->Passenger = Sexton;
	Ferry->DispatchBeginPlay();
	TestTrue(TEXT("Sexton sits in the stern"), Sexton->GetAttachParentActor() == Ferry
		&& Sexton->GetActorLocation().Equals(Ferry->GetSeatTransform().GetLocation(), 1.0));
	const FVector Before = Sexton->GetActorLocation();
	Ferry->Sway(1.7f);
	const FVector After = Sexton->GetActorLocation();
	TestTrue(TEXT("...and sways with it, a little"), !Before.Equals(After, 0.01) && FVector::Dist(Before, After) < 40.0);
	const FVector Spot = Ferry->GetLandingSpot(0.f, 60.f);
	TestTrue(TEXT("A spot on the landing is on its planks"), FMath::IsNearlyZero(Spot.Z, 0.1)
		&& Spot.X < ASextonsFerry::LandingDepth * 0.5f && Spot.X > -ASextonsFerry::LandingDepth * 0.5f);
	TestTrue(TEXT("...within talking reach of Sexton's seat (4 m)"), FVector::Dist(Spot, Ferry->GetSeatTransform().GetLocation()) < 400.0);
	// The fog wraps the hull (its floor 1 m under the planks, its gunwale at them), not 4 m under it (play check 2026-11-01),
	// and stays under the landing's planks (they're 14 cm thick).
	if (Ferry->Fog && Ferry->Fog->GetStaticMesh())
	{
		Ferry->Fog->UpdateBounds();
		const FBox Bank = Ferry->Fog->Bounds.GetBox();
		TestTrue(FString::Printf(TEXT("The fog bank (%.0f to %.0f cm) runs from under the hull's floor up its sides, under the planks"),
			Bank.Min.Z, Bank.Max.Z), Bank.Min.Z < -100.0 && Bank.Max.Z > -80.0 && Bank.Max.Z < -14.0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFerryLadderTest, "Looter.World.SextonsFerry.Ladder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFerryLadderTest::RunTest(const FString& Parameters)
{
	// Its two ends: the deck behind its edge (facing inland) and the landing out from its foot (facing the ferry), Drop
	// apart; which end a climber is at; with scenes off, a climb puts them at the far end at once.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	AFerryLadder* Ladder = World->SpawnActor<AFerryLadder>(FVector(0.0, 0.0, 1000.0), FRotator(0.0, 90.0, 0.0));
	if (!Ladder)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Ladder->DispatchBeginPlay();
	const FVector Top = Ladder->GetTopFeet();
	const FVector Foot = Ladder->GetBottomFeet();
	TestTrue(TEXT("Its foot is Drop under its top"), FMath::IsNearlyEqual(Top.Z - Foot.Z, static_cast<double>(Ladder->Drop), 0.5));
	TestTrue(TEXT("...the top behind the edge (inland, -Y here), the foot out from it (+Y)"), Top.Y < 0.0 && Foot.Y > 0.0);
	TestTrue(TEXT("Stepping off up top faces inland; at the foot, out to the ferry"),
		FMath::IsNearlyEqual(FRotator::NormalizeAxis(Ladder->GetTopYaw()), -90.f, 0.5f)
		&& FMath::IsNearlyEqual(FRotator::NormalizeAxis(Ladder->GetBottomYaw()), 90.f, 0.5f));
	TestTrue(TEXT("A climber on the deck is at its top; one on the landing at its foot"), Ladder->IsAtTop(Top) && !Ladder->IsAtTop(Foot));
	TestEqual(TEXT("Its rungs"), Ladder->Rungs.Num(), AFerryLadder::RungCount);
	TestTrue(TEXT("Hob's perch on its crossbar, over the deck"), Ladder->GetPerch().GetLocation().Z > Top.Z + 100.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFerryLadderClimbTest, "Looter.Scenes.FerryLadder.Climb",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFerryLadderClimbTest::RunTest(const FString& Parameters)
{
	// The climb's path: from where the eyes start, to the ladder, rung by rung (a step and a settle at each, never back up),
	// and off it, ending exactly at the body's eyes at the far end (no seam back to the player's view).
	TestTrue(TEXT("About two and a half seconds"), FerryLadderClimb::Duration() > 2.f && FerryLadderClimb::Duration() < 3.5f);
	TestEqual(TEXT("The rungs start at none"), FerryLadderClimb::RungAlpha(0.f, 9), 0.f);
	TestEqual(TEXT("...and end at all"), FerryLadderClimb::RungAlpha(1.f, 9), 1.f);
	bool bOnward = true;
	float Last = 0.f;
	for (int32 Step = 1; Step <= 90; ++Step)
	{
		const float Now = FerryLadderClimb::RungAlpha(Step / 90.f, 9);
		bOnward &= Now + 1e-5f >= Last;
		Last = Now;
	}
	TestTrue(TEXT("...never going back"), bOnward);
	TestTrue(TEXT("...a settle at each rung (a rung's end is the next one's start)"),
		FMath::IsNearlyEqual(FerryLadderClimb::RungAlpha(1.f / 9.f, 9), 1.f / 9.f, 1e-4f));

	const FVector Start(0.0, 0.0, 160.0);
	const FVector From(0.0, 50.0, 90.0);
	const FVector To(0.0, 50.0, -220.0);
	const FVector End(0.0, 140.0, -220.0);
	TestTrue(TEXT("It starts at the eyes"), FerryLadderClimb::EyeAt(0.f, Start, From, To, End, 9).Equals(Start, 0.01));
	TestTrue(TEXT("...is on the ladder when the turn's done"), FerryLadderClimb::EyeAt(FerryLadderClimb::TurnSeconds, Start, From, To, End, 9)
		.Equals(From, 0.01));
	TestTrue(TEXT("...at the ladder's far end when the rungs are"), FerryLadderClimb::EyeAt(FerryLadderClimb::TurnSeconds
		+ FerryLadderClimb::ClimbSeconds, Start, From, To, End, 9).Equals(To, 0.01));
	TestTrue(TEXT("...and ends at the far end's eyes"), FerryLadderClimb::EyeAt(FerryLadderClimb::Duration(), Start, From, To, End, 9)
		.Equals(End, 0.01));
	TestTrue(TEXT("...and stays there"), FerryLadderClimb::EyeAt(FerryLadderClimb::Duration() + 5.f, Start, From, To, End, 9).Equals(End, 0.01));
	return true;
}

#endif

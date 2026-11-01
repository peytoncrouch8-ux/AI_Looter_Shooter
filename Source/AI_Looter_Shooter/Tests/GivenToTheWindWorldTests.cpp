#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/UnpaidCreature.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionTypes.h"
#include "Scenes/SaintAdaWakes.h"
#include "Tests/InteractionTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "World/CarriedLantern.h"
#include "World/CarriedLanternRules.h"
#include "World/ChapelReliquary.h"
#include "World/ReliquaryLantern.h"
#include "World/SoulCoal.h"
#include "World/SoulFollowers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"

// Main 7's world after Abel's fight (the Wardens story, lane MAIN67; Docs/Work/2026-10-31-Wardens/main67_src/CONTRACT67.md),
// each on its own in a test level with no story: the soul coal and its followers, Abel's carried light and the lines on it,
// the lantern set on the Reliquary, and Saint Ada's waking's end state.

namespace
{
	/** A lump with no story on it (shown, free to crack), begun. */
	ASoulCoalLump* SpawnLump(UWorld* World, const FVector& Where, FName Id)
	{
		ASoulCoalLump* Lump = World->SpawnActor<ASoulCoalLump>(Where, FRotator::ZeroRotator);
		if (Lump)
		{
			Lump->LumpId = Id;
			Lump->ShownWhen = FStoryCondition();
			Lump->FreeWhen = FStoryCondition();
			Lump->DispatchBeginPlay();
		}
		return Lump;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulCoalFreeTest, "Looter.World.SoulCoal.Free",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSoulCoalFreeTest::RunTest(const FString& Parameters)
{
	// A whole lump with a soul in it: a 1 s hold "Free the soul"; cracked, used up for good (the lasting objective), its soul
	// in the lantern's trail; cracked, nothing more to free.
	TestEqual(TEXT("Its flag keeps it by its id"), GivenToTheWind::SoulCoalFlag(TEXT("Road3")).ToString(), FString(TEXT("SoulCoal.Road3")));
	TestEqual(TEXT("Missions find it by its tag"), GivenToTheWind::SoulCoalTag().ToString(), FString(TEXT("SoulCoal")));
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);
	ASoulCoalLump* Lump = SpawnLump(World, InteractionTestWorld::Ahead(150.0), TEXT("Test1"));
	UCarriedLanternSubsystem* Lantern = UCarriedLanternSubsystem::Get(World);
	if (!Player || !Interaction || !Lump || !Lantern)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	TestTrue(TEXT("Tagged for Main 7's lasting objective"), Lump->ActorHasTag(GivenToTheWind::SoulCoalTag()));
	const FInteractionOptions Options = Lump->GetInteractionOptions(*Interaction);
	TestTrue(TEXT("A hold, 1 s, \"Free the soul\""), Options.bUsable && Options.bHold && !Options.bTap
		&& FMath::IsNearlyEqual(Options.HoldSeconds, 1.f) && Options.HoldPrompt.ToString() == TEXT("Free the soul"));
	TestFalse(TEXT("Whole: not used up"), Lump->IsUsedUp());
	TestTrue(TEXT("Cracked"), Lump->Crack(Player));
	TestTrue(TEXT("...used up for good"), Lump->IsCracked() && Lump->IsUsedUp());
	TestEqual(TEXT("...its soul follows the lantern"), Lantern->NumFollowers(), 1);
	TestFalse(TEXT("Cracked: nothing more to free"), Lump->Crack(Player) || Lump->GetInteractionOptions(*Interaction).bUsable);
	ASoulCoalLump* Second = SpawnLump(World, InteractionTestWorld::Ahead(250.0), TEXT("Test2"));
	TestTrue(TEXT("A second lump's soul joins the trail"), Second && Second->Crack(nullptr, /*bForce*/ true) && Lantern->NumFollowers() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulCoalFollowersTest, "Looter.World.SoulCoal.Followers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSoulCoalFollowersTest::RunTest(const FString& Parameters)
{
	// The trail as math: behind the lantern, each a little farther back and higher; a ring at the gate; released, west and up.
	const FVector First = CarriedLanternRules::FollowerSlot(0, 0.f);
	const FVector Fifth = CarriedLanternRules::FollowerSlot(4, 0.f);
	TestTrue(TEXT("Behind the lantern"), First.X < -100.0 && Fifth.X < First.X);
	TestTrue(TEXT("...over the ground, never ahead"), First.Z > 50.0 && Fifth.Z > 50.0);
	const FVector Ring = CarriedLanternRules::WaitSlot(0, 6, 0.f);
	TestTrue(TEXT("Waiting in a ring at the gate"), Ring.Size2D() > 120.0 && Ring.Size2D() < 220.0);
	const FVector From(0.0, 0.0, 0.0);
	const FVector Later = CarriedLanternRules::ReleasedAt(From, 6.f, 0);
	TestTrue(TEXT("Released: west (-Y) and up"), Later.Y < -3000.0 && Later.Z > 500.0);
	TestTrue(TEXT("...seen rising in, gone far off"), CarriedLanternRules::ReleasedGlow(0.f) < 0.1f && CarriedLanternRules::ReleasedGlow(3.f) > 0.9f
		&& CarriedLanternRules::ReleasedGlow(CarriedLanternRules::ReleaseSeconds) < 0.01f);

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	AActor* Leader = MissionTestWorld::SpawnMarker(World, FVector(0.0, 0.0, 90.0));
	ASoulFollowers* Trail = World->SpawnActor<ASoulFollowers>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!Leader || !TestNotNull(TEXT("The trail"), Trail))
	{
		return false;
	}
	Trail->DispatchBeginPlay();
	Trail->Leader = Leader;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Trail->AddSoul(FVector(600.0 + Index * 100.0, 300.0, 20.0));
	}
	TestTrue(TEXT("Freed: rising out of the coal"), Trail->NumFollowing() == 3 && Trail->GetMode(0) == ESoulFollow::Rising);
	for (int32 Frame = 0; Frame < 150; ++Frame)
	{
		Trail->Advance(1.f / 30.f);
	}
	TestTrue(TEXT("...then following, behind the leader"), Trail->GetMode(0) == ESoulFollow::Following
		&& FVector::Dist(Trail->GetSoulLocation(0), Leader->GetActorLocation()) < 450.0
		&& FVector::DotProduct(Trail->GetSoulLocation(0) - Leader->GetActorLocation(), Leader->GetActorForwardVector()) < 0.0);
	for (int32 Index = 0; Index < 20; ++Index)
	{
		Trail->AddSoul(FVector::ZeroVector, /*bAtOnce*/ true);
	}
	TestEqual(TEXT("At most twelve follow"), Trail->NumFollowing(), CarriedLanternRules::MaxFollowers);
	Trail->Release({ FVector(0.0, 1000.0, 0.0), FVector(100.0, 1000.0, 0.0) });
	TestTrue(TEXT("Released: the Gravewind has them all, the six dead's lights besides"), Trail->IsReleased() && Trail->NumFollowing() == 0
		&& Trail->NumSouls() == CarriedLanternRules::MaxFollowers + 2);
	for (float Time = 0.f; Time < CarriedLanternRules::ReleaseSeconds + 2.f; Time += 0.1f)
	{
		Trail->Advance(0.1f);
	}
	TestEqual(TEXT("...and gone west"), Trail->NumSouls(), 0);
	TestEqual(TEXT("Released once: no more join"), Trail->AddSoul(FVector::ZeroVector), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCarriedLanternDrainTest, "Looter.World.CarriedLantern.Drain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCarriedLanternDrainTest::RunTest(const FString& Parameters)
{
	// His light as math: a line drains it, never out; free a moment, it fills again; then the subsystem with the lantern carried.
	TestTrue(TEXT("A line drains it"), CarriedLanternRules::Drained(1.f, 1, 2.f) < 1.f);
	TestTrue(TEXT("...two lines twice as fast"), FMath::IsNearlyEqual(1.f - CarriedLanternRules::Drained(1.f, 2, 1.f),
		2.f * (1.f - CarriedLanternRules::Drained(1.f, 1, 1.f)), 1e-4f));
	TestEqual(TEXT("...never out: nothing fails"), CarriedLanternRules::Drained(0.3f, 3, 60.f), CarriedLanternRules::MinLight);
	TestEqual(TEXT("Just let go: no fill yet"), CarriedLanternRules::Refilled(0.5f, 0.5f, 1.f), 0.5f);
	TestTrue(TEXT("...a moment free, it fills"), CarriedLanternRules::Refilled(0.5f, CarriedLanternRules::RefillDelay + 0.1f, 1.f) > 0.5f);
	TestEqual(TEXT("...to full, no more"), CarriedLanternRules::Refilled(0.99f, 10.f, 10.f), 1.f);

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UCarriedLanternSubsystem* Lantern = UCarriedLanternSubsystem::Get(World);
	AActor* HoseMan = MissionTestWorld::SpawnMarker(World, FVector(800.0, 0.0, 90.0));
	if (!Lantern || !HoseMan)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	TestFalse(TEXT("No story: not carried"), Lantern->IsCarried());
	Lantern->ForceCarried(1);
	TestTrue(TEXT("Carried"), Lantern->IsCarried() && FMath::IsNearlyEqual(Lantern->GetLight(), 1.f));
	Lantern->SetLatched(HoseMan, true);
	Lantern->Step(5.f);
	TestTrue(TEXT("A line on it five seconds: about a third gone"), FMath::IsNearlyEqual(Lantern->GetLight(), 1.f - 5.f * CarriedLanternRules::DrainPerLine, 0.01f)
		&& Lantern->IsDraining());
	Lantern->SetLatched(HoseMan, false);
	const float Drained = Lantern->GetLight();
	Lantern->Step(1.f);
	TestEqual(TEXT("Let go: it holds a moment"), Lantern->GetLight(), Drained);
	for (int32 Second = 0; Second < 8; ++Second)
	{
		Lantern->Step(1.f);
	}
	TestTrue(TEXT("...then fills to full"), FMath::IsNearlyEqual(Lantern->GetLight(), 1.f));
	Lantern->SetDown();
	TestFalse(TEXT("Set on the Reliquary: carried no more"), Lantern->IsCarried());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCarriedLanternLatchTest, "Looter.World.CarriedLantern.Latch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCarriedLanternLatchTest::RunTest(const FString& Parameters)
{
	// The line holds the lantern at the carrier's right hip; each hose-man's line counts once; a gone one lets go; not carried,
	// no line holds it.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UCarriedLanternSubsystem* Lantern = UCarriedLanternSubsystem::Get(World);
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);
	AActor* First = MissionTestWorld::SpawnMarker(World, FVector(800.0, 0.0, 90.0));
	AActor* Second = MissionTestWorld::SpawnMarker(World, FVector(0.0, 800.0, 90.0));
	if (!Lantern || !Player || !First || !Second)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	const FVector Hip = UCarriedLanternSubsystem::LanternPoint(Player);
	const FVector Local = Player->GetActorTransform().InverseTransformPositionNoScale(Hip);
	TestTrue(TEXT("At the right hip"), Local.Y > 15.0 && Local.Z < 0.0 && FMath::Abs(Local.X) < 40.0);
	Lantern->ForceCarried(1);
	Lantern->SetLatched(First, true);
	Lantern->SetLatched(First, true);
	TestEqual(TEXT("One hose-man's line counts once"), Lantern->NumLatched(), 1);
	Lantern->SetLatched(Second, true);
	TestEqual(TEXT("Two lines"), Lantern->NumLatched(), 2);
	Second->Destroy();
	Lantern->Step(0.1f);
	TestEqual(TEXT("A gone hose-man's line lets go"), Lantern->NumLatched(), 1);
	Lantern->ForceCarried(0);
	Lantern->Step(0.1f);
	TestEqual(TEXT("Not carried: nothing holds it"), Lantern->NumLatched(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReliquaryLanternSetTest, "Looter.World.ReliquaryLantern.Set",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FReliquaryLanternSetTest::RunTest(const FString& Parameters)
{
	// Seated in the lid's recess of the Reliquary; only while the lantern is carried; a 3 s hold; set, the lantern's carried
	// no more and it stays there; she can be lit by the scene before the story says so.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);
	AChapelReliquary* Saint = World->SpawnActor<AChapelReliquary>(InteractionTestWorld::Ahead(200.0) - FVector(0.0, 0.0, 90.0), FRotator::ZeroRotator);
	AReliquaryLantern* Rest = World->SpawnActor<AReliquaryLantern>(InteractionTestWorld::Ahead(200.0), FRotator::ZeroRotator);
	UCarriedLanternSubsystem* Lantern = UCarriedLanternSubsystem::Get(World);
	if (!Player || !Interaction || !Saint || !Rest || !Lantern)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Saint->DispatchBeginPlay();
	Rest->SetWhen = FStoryCondition();
	Rest->ShownWhen = FStoryCondition();
	Rest->DispatchBeginPlay();
	TestTrue(TEXT("On her ember, in the lid's recess"), FVector::Dist(Rest->GetActorLocation(), Saint->GetEmberStart()) < 5.0
		&& Rest->GetReliquary() == Saint);
	TestTrue(TEXT("Tagged for Main 7's last step"), Rest->ActorHasTag(GivenToTheWind::ReliquaryLanternTag()));
	TestFalse(TEXT("Not carried: nothing to set"), Rest->CanSet() || Rest->GetInteractionOptions(*Interaction).bUsable);
	Lantern->ForceCarried(1);
	const FInteractionOptions Options = Rest->GetInteractionOptions(*Interaction);
	TestTrue(TEXT("Carried: a 3 s hold, \"Set Abel's lantern on the Reliquary\""), Options.bUsable && Options.bHold
		&& FMath::IsNearlyEqual(Options.HoldSeconds, 3.f) && Options.HoldPrompt.ToString() == TEXT("Set Abel's lantern on the Reliquary"));
	TestFalse(TEXT("She's under ash yet"), Saint->IsLit());
	TestTrue(TEXT("Set"), Rest->SetLantern(Player, false, /*bQuiet*/ true));
	TestTrue(TEXT("...for good, the lantern carried no more"), Rest->IsSet() && Rest->IsUsedUp() && !Lantern->IsCarried());
	TestFalse(TEXT("...not set twice"), Rest->SetLantern(Player));
	Saint->SetLitNow(true);
	Saint->SetBlaze(1.f);
	TestTrue(TEXT("Lit by the scene, blazing"), Saint->IsLit() && Saint->GetEmberGlow() > 50.f);
	Saint->SetBlaze(0.f);
	TestTrue(TEXT("...then her steady light"), Saint->IsLit() && Saint->GetEmberGlow() < 50.f && Saint->GetEmberGlow() > 30.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaintAdaWakesEndStateTest, "Looter.Scenes.SaintAdaWakes.EndState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSaintAdaWakesEndStateTest::RunTest(const FString& Parameters)
{
	// Its words (four, the design's speakers, each a caption's length); the world as it leaves it with no scene: her lit, the
	// lantern on her lid, the trail taken west, the dead near gone quiet.
	const TArray<FStoryLine> Lines = SaintAdaWakes::Lines();
	TestEqual(TEXT("Four lines"), Lines.Num(), 4);
	for (const FStoryLine& Line : Lines)
	{
		TestTrue(TEXT("...each a caption's length"), Line.Text.ToString().Len() <= 90);
	}
	TestTrue(TEXT("...Abel's first, Delia's answer"), Lines.Num() == 4 && Lines[0].Speaker.ToString() == TEXT("Abel")
		&& Lines[1].Speaker.ToString().StartsWith(TEXT("Delia")));
	TestTrue(TEXT("About half a minute"), SaintAdaWakes::Duration() > 20.f && SaintAdaWakes::Duration() < 40.f);

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	AChapelReliquary* Saint = World->SpawnActor<AChapelReliquary>(FVector(0.0, 0.0, 0.0), FRotator::ZeroRotator);
	AReliquaryLantern* Rest = World->SpawnActor<AReliquaryLantern>(FVector(0.0, 0.0, 70.0), FRotator::ZeroRotator);
	UCarriedLanternSubsystem* Lantern = UCarriedLanternSubsystem::Get(World);
	if (!Saint || !Rest || !Lantern)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Saint->DispatchBeginPlay();
	Rest->SetWhen = FStoryCondition();
	Rest->ShownWhen = FStoryCondition();
	Rest->DispatchBeginPlay();
	Lantern->ForceCarried(1);
	Lantern->AddFollower(FVector(300.0, 0.0, 100.0), /*bAtOnce*/ true);
	AUnpaidCreature* Unpaid = World->SpawnActor<AUnpaidCreature>(FVector(1500.0, 0.0, 100.0), FRotator::ZeroRotator);
	TestTrue(TEXT("Set, quietly"), Rest->SetLantern(nullptr, /*bForce*/ true, /*bQuiet*/ true));
	SaintAdaWakes::ApplyEndState(*Rest, /*bInstant*/ true);
	TestTrue(TEXT("Saint Ada lit, her blaze settled"), Saint->IsLit() && FMath::IsNearlyZero(Saint->GetBlaze()));
	TestTrue(TEXT("The lantern carried no more"), !Lantern->IsCarried());
	TestTrue(TEXT("The trail taken west"), Lantern->GetFollowers() && Lantern->GetFollowers()->IsReleased() && Lantern->NumFollowers() == 0);
	if (Unpaid)
	{
		TestTrue(TEXT("The dead near her gone quiet (gone)"), !IsValid(Unpaid) || Unpaid->IsActorBeingDestroyed());
	}
	else
	{
		AddInfo(TEXT("No Unpaid could be spawned in the test level: the quieting isn't checked."));
	}
	SaintAdaWakes::ApplyEndState(*Rest, /*bInstant*/ true);
	TestTrue(TEXT("Twice is harmless"), Saint->IsLit() && Rest->IsSet());
	return true;
}

#endif

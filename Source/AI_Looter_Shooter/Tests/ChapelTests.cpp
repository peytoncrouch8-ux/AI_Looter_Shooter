#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionTypes.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/GraveSightFlash.h"
#include "Story/GraveSightSubsystem.h"
#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryLine.h"
#include "Story/StoryLineSet.h"
#include "Tests/HallowedGroundTestWorld.h"
#include "Tests/InteractionTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "World/ChapelBell.h"
#include "World/ChapelReliquary.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// The Chapel of Saint Ada in Main 4, "Hallowed Ground" (Docs/Areas/RansomsRest.md): the bell rung from its rope, the smashed
// Reliquary seen in a Grave Sight flash, and Father Aldana at the vestry door. Main 4 itself, its yard and what comes after
// it are HallowedGroundTests.cpp's.

using namespace HallowedGroundTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHallowedGroundBellTest, "Looter.Story.HallowedGround.Bell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHallowedGroundBellTest::RunTest(const FString& Parameters)
{
	// The chapel bell: held on its rope it rings and Main 4's bell step is done; it swings about its axis, the swings dying
	// away, tolling at the ends of the hard ones; it can't be rung again until it hangs still, nor while its story says not.
	TestEqual(TEXT("Still before the pull"), AChapelBell::SwingAt(0.f, 30.f, 2.4f, 9.f), 0.f);
	TestTrue(TEXT("A quarter swing in: nearly all the way out"), AChapelBell::SwingAt(0.6f, 30.f, 2.4f, 9.f) > 24.f
		&& AChapelBell::SwingAt(0.6f, 30.f, 2.4f, 9.f) <= 30.f);
	TestTrue(TEXT("Three quarters in: out the other way"), AChapelBell::SwingAt(1.8f, 30.f, 2.4f, 9.f) < -15.f);
	TestEqual(TEXT("Still at the end"), AChapelBell::SwingAt(9.f, 30.f, 2.4f, 9.f), 0.f);

	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);
	AChapelBell* Bell = SpawnBell(World, InteractionTestWorld::Ahead(150.0));
	if (!Runner || !Player || !Interaction || !Bell)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Runner->BeginForTesting({ MakeMainFourAt(CreatePackage(nullptr), 2) }, Campaign, Player, Valley);
	Runner->Update(0.f);
	Runner->SetStep(MainFour, 2);
	TestEqual(TEXT("Main 4 at the bell"), Runner->GetStep(MainFour), 2);
	if (!Bell->Bell->GetStaticMesh())
	{
		AddInfo(TEXT("SM_ChapelBell isn't imported in this checkout: the bell swings unseen."));
	}

	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("The rope is looked at: a hold, \"Ring the chapel bell\""), Interaction->GetFocusedActor() == Bell
		&& Interaction->GetFocusedOptions().bHold && !Interaction->GetFocusedOptions().bTap
		&& FMath::IsNearlyEqual(Interaction->GetFocusedOptions().HoldSeconds, Bell->HoldSeconds)
		&& Interaction->GetFocusedOptions().HoldPrompt.ToString() == TEXT("Ring the chapel bell"));
	Interaction->PressInteract();
	Interaction->UpdateInteraction(Bell->HoldSeconds * 0.5f);
	TestTrue(TEXT("Half a hold: the bar half full, nothing rung"), FMath::IsNearlyEqual(Interaction->GetHoldProgress(), 0.5f, 0.01f)
		&& !Bell->IsRinging() && Runner->GetStep(MainFour) == 2);
	Interaction->UpdateInteraction(Bell->HoldSeconds * 0.5f + 0.01f);
	TestTrue(TEXT("Held: it rings"), Bell->IsRinging());
	TestEqual(TEXT("...and the bell's step is done"), Runner->GetStep(MainFour), 3);
	Interaction->ReleaseInteract();
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Swinging, it can't be rung: never looked at"), !Bell->CanRing() && Interaction->GetFocusedActor() == nullptr);
	TestFalse(TEXT("...nor rung from the console"), Bell->Ring(Player));

	// The swing: out about its axis, the swings dying away, still by its time.
	float Widest = 0.f;
	float Late = 0.f;
	const float Lasts = Bell->RingSeconds;
	for (float Time = 0.f; Time < Lasts - 0.01f; Time += 0.05f)
	{
		Bell->Advance(0.05f);
		Widest = FMath::Max(Widest, FMath::Abs(Bell->GetSwing()));
		if (Time > Lasts - 1.f)
		{
			Late = FMath::Max(Late, FMath::Abs(Bell->GetSwing()));
		}
	}
	TestTrue(TEXT("It swings out nearly as far as it may, never farther"), Widest > Bell->SwingDegrees * 0.8f && Widest <= Bell->SwingDegrees + 0.01f);
	TestTrue(TEXT("...and in its last second barely moves"), Late < Bell->SwingDegrees * 0.15f);
	TestEqual(TEXT("It tolled at the ends of the hard swings: four strokes"), Bell->GetStrokes(), 4);
	Bell->Advance(0.1f);
	TestTrue(TEXT("Still again: it can be rung once more"), !Bell->IsRinging() && FMath::IsNearlyZero(Bell->GetSwing()) && Bell->CanRing());

	// Its story: a bell the story doesn't let ring yet is never offered.
	Bell->RingWhen.AfterMissions = { FName(TEXT("Test.NotYet")) };
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Not yet by its story: not offered, not rung"), !Bell->CanRing() && Interaction->GetFocusedActor() == nullptr && !Bell->Ring(Player));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHallowedGroundGraveSightTest, "Looter.Story.HallowedGround.GraveSight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHallowedGroundGraveSightTest::RunTest(const FString& Parameters)
{
	// Grave Sight's flash as rules: two seconds, the overlay in quickly, held, out slowly; the ember rising off the lid and
	// fading. Then the Reliquary: looked at only from Main 4's fourth step, the flash and the ember, nothing more while it
	// plays, and its step done when it times out.
	FGraveSightFlash Flash;
	TestTrue(TEXT("Nothing before it starts"), !Flash.IsPlaying() && Flash.GetOverlayAlpha() == 0.f);
	Flash.Start();
	TestTrue(TEXT("Two seconds, starting on the plain world"), FMath::IsNearlyEqual(Flash.GetSeconds(), 2.f) && Flash.GetOverlayAlpha() == 0.f
		&& Flash.GetEmberGlow() == 0.f);
	TestFalse(TEXT("0.3 s in, it's still on"), Flash.Advance(0.3f));
	TestEqual(TEXT("...the overlay all the way in"), Flash.GetOverlayAlpha(), 1.f, 1e-4f);
	TestEqual(TEXT("...the ember flared up"), Flash.GetEmberGlow(), 1.f, 1e-4f);
	Flash.Advance(0.7f);
	TestEqual(TEXT("Halfway: the ember three quarters of the way up"), Flash.GetEmberRise(), 0.75f, 1e-4f);
	Flash.Advance(0.6f);
	TestEqual(TEXT("1.6 s: the overlay half gone"), Flash.GetOverlayAlpha(), 0.5f, 1e-3f);
	TestTrue(TEXT("It ends at two seconds"), Flash.Advance(0.5f) && !Flash.IsPlaying());
	TestTrue(TEXT("...the overlay and the ember gone, risen all the way"), Flash.GetOverlayAlpha() == 0.f && Flash.GetEmberGlow() == 0.f
		&& FMath::IsNearlyEqual(Flash.GetEmberRise(), 1.f));
	TestFalse(TEXT("...and it ends once"), Flash.Advance(1.f));

	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UGraveSightSubsystem* Sight = UGraveSightSubsystem::Get(World);
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);
	AChapelReliquary* Reliquary = SpawnReliquary(World, FVector(150.0, 0.0, 0.0));
	if (!Runner || !Sight || !Player || !Interaction || !Reliquary)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Runner->BeginForTesting({ MakeMainFourAt(CreatePackage(nullptr), 3) }, Campaign, Player, Valley);
	Runner->Update(0.f);
	TestEqual(TEXT("Main 4 on its way up"), Runner->GetStep(MainFour), 0);
	TestTrue(TEXT("Before its step the Reliquary isn't offered"), !Reliquary->CanLook()
		&& !Reliquary->GetInteractionOptions(*Interaction).bUsable && !Reliquary->Look(Player) && !Sight->IsFlashing());

	Runner->SetStep(MainFour, 3);
	const FInteractionOptions Options = Reliquary->GetInteractionOptions(*Interaction);
	TestTrue(TEXT("At its step: a tap, \"Look at the Reliquary\""), Options.bUsable && Options.bTap && !Options.bHold
		&& Options.TapPrompt.ToString() == TEXT("Look at the Reliquary"));
	if (!Reliquary->HasModel())
	{
		AddInfo(TEXT("The smashed Reliquary isn't imported in this checkout: its stand-in stands in."));
		TestTrue(TEXT("The stand-in's ember starts over its lid"), Reliquary->GetEmberStart().Z > Reliquary->GetActorLocation().Z + 60.0);
	}
	const FVector EmberFrom = Reliquary->GetEmberStart();
	TestTrue(TEXT("Looked at"), Reliquary->Look(Player));
	TestTrue(TEXT("...the flash on the screen, two seconds"), Sight->IsFlashing() && FMath::IsNearlyEqual(Sight->GetFlash().GetSeconds(), 2.f));
	TestFalse(TEXT("...and while it plays, it can't be looked at again"), Reliquary->CanLook() || Reliquary->Look(Player));
	Reliquary->Advance(1.f);
	Sight->Advance(1.f);
	TestTrue(TEXT("A second in: the ember well up off the lid, burning"), Reliquary->GetEmberLocation().Z > EmberFrom.Z + Reliquary->EmberRise * 0.5
		&& Reliquary->GetEmberGlow() > 0.f && Sight->GetFlash().GetOverlayAlpha() > 0.5f);
	TestEqual(TEXT("...the step waits for the sight to end"), Runner->GetStep(MainFour), 3);
	Reliquary->Advance(1.1f);
	Sight->Advance(1.1f);
	TestTrue(TEXT("Two seconds: it times out, the ember gone"), !Reliquary->IsFlashing() && Reliquary->GetEmberGlow() == 0.f
		&& Reliquary->GetLooks() == 1);
	TestTrue(TEXT("...the screen plain again"), !Sight->IsFlashing() && Sight->GetFlash().GetOverlayAlpha() == 0.f);
	TestEqual(TEXT("...and the Reliquary's step done"), Runner->GetStep(MainFour), 4);
	TestTrue(TEXT("Over, it can be looked at again while its story allows"), Reliquary->CanLook());

	// The console's way: past what the story allows.
	Reliquary->LookWhen.AfterMissions = { FName(TEXT("Test.NotYet")) };
	TestFalse(TEXT("Not allowed: no look"), Reliquary->Look(nullptr));
	TestTrue(TEXT("...forced, it plays"), Reliquary->Look(nullptr, /*bForce*/ true) && Reliquary->IsFlashing());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHallowedGroundAldanaTest, "Looter.Story.HallowedGround.Aldana",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHallowedGroundAldanaTest::RunTest(const FString& Parameters)
{
	// Father Aldana's topics by the story: the door barred before Main 4 and while the yard is held, the bell and the
	// Reliquary once it's quiet, the doc's words at Main 4's last step, the Sink after.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Listener = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector);
	ASpeakerPoint* Aldana = PlaceAldana(World, FVector(300.0, 0.0, 170.0));
	if (!Runner || !Listener || !Aldana)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Runner->BeginForTesting({}, Campaign, Listener, Valley);
	Aldana->DispatchBeginPlay();

	auto Says = [Aldana](int32& OutTopic) { return Aldana->SpeakerPoint->GetLinesNow(&OutTopic); };
	int32 Topic = INDEX_NONE;
	Campaign.ActiveMission = MainThree;
	TArray<FStoryLine> Lines = Says(Topic);
	TestTrue(TEXT("Before Main 4: the door stays barred, in his name"), Topic == INDEX_NONE && !Lines.IsEmpty()
		&& Lines[0].Speaker.ToString() == TEXT("Father Aldana"));
	Campaign.Complete(MainThree);
	Campaign.ActiveMission = MainFour;
	Campaign.ActiveMissionStep = 0;
	Says(Topic);
	TestEqual(TEXT("On the way up: barred"), Topic, static_cast<int32>(INDEX_NONE));
	Campaign.ActiveMissionStep = 1;
	Says(Topic);
	TestEqual(TEXT("While the yard is held: barred"), Topic, static_cast<int32>(INDEX_NONE));
	Campaign.ActiveMissionStep = 2;
	Says(Topic);
	TestEqual(TEXT("The yard quiet: the bell and the Reliquary"), Topic, 1);
	Campaign.ActiveMissionStep = 3;
	Says(Topic);
	TestEqual(TEXT("...still, at the Reliquary"), Topic, 1);
	Campaign.ActiveMissionStep = 4;
	Lines = Says(Topic);
	TestEqual(TEXT("Main 4's last step: his words"), Topic, 0);
	TestTrue(TEXT("...the doc's, in order, in his name"), HoldsAldanasWords(Lines, /*bNamed*/ true));
	TestTrue(TEXT("...and Ellis asks, by name"), Lines.ContainsByPredicate([](const FStoryLine& Line)
	{
		return Line.Speaker.ToString() == TEXT("Ellis");
	}));
	Campaign.Complete(MainFour);
	Says(Topic);
	TestEqual(TEXT("After Main 4: a word about the Sink, not his words again"), Topic, 2);

	// The words as the story script writes them, when it has.
	if (const UStoryLineSet* Main = LoadLines(TEXT("DA_Lines_AldanaMain4")))
	{
		TestTrue(TEXT("His Main 4 set holds the doc's words, in order, in his own voice"), HoldsAldanasWords(Main->Lines, /*bNamed*/ false));
		TestTrue(TEXT("...ending \"Look in the Sink.\""), !Main->Lines.IsEmpty()
			&& Main->Lines.Last().Text.ToString() == AldanaWords[AldanaWordCount - 1]);
		const TCHAR* const Others[] = { TEXT("DA_Lines_AldanaBarred"), TEXT("DA_Lines_AldanaWaiting"), TEXT("DA_Lines_AldanaAfterMain4"),
			TEXT("DA_Lines_HobMain4Road"), TEXT("DA_Lines_HobMain4Yard"), TEXT("DA_Lines_HobMain4Bell"), TEXT("DA_Lines_HobMain4Reliquary"),
			TEXT("DA_Lines_HobMain4Aldana"), TEXT("DA_Lines_HobMain4") };
		for (const TCHAR* Name : Others)
		{
			TestNotNull(*FString::Printf(TEXT("%s is made"), Name), LoadLines(Name));
		}
	}
	else
	{
		AddWarning(TEXT("Main 4's line sets aren't made yet: run Tools/Unreal/create_story_lines.py."));
	}
	return true;
}

#endif

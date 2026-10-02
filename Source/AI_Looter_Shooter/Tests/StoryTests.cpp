#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Interaction/InteractionComponent.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionQueue.h"
#include "Story/CaptionSubsystem.h"
#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryCharacter.h"
#include "Story/StoryLine.h"
#include "Tests/InteractionTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

namespace
{
	FStoryLine MakeTestLine(const TCHAR* Speaker, const TCHAR* Words, float Seconds = 0.f)
	{
		return FStoryLine::Make(FText::FromString(Speaker), FText::FromString(Words), Seconds);
	}

	/** The words of the caption on screen, or "none". */
	FString CaptionWords(const FCaptionQueue& Queue)
	{
		const FCaptionEntry* Current = Queue.GetCurrent();
		return Current ? Current->Line.Text.ToString() : FString(TEXT("none"));
	}

	/** Who says the caption on screen, and what: "Tilly Bright: ...", or "none". */
	FString CaptionOnScreen(const UCaptionSubsystem& Captions)
	{
		const FCaptionEntry* Current = Captions.GetCurrent();
		return Current ? Current->Line.Speaker.ToString() + TEXT(": ") + Current->Line.Text.ToString() : FString(TEXT("none"));
	}

	/**
	 * Someone talking through a door or window in a test level, turned toward the player's stand-in at the origin, its
	 * speaker point at the actor's spot (level with the stand-in's eyes, so the cone finds it). Play begins for it as it
	 * does in a level: it joins the level's interactables.
	 */
	ASpeakerPoint* PlaceSpeaker(UWorld* World, const FVector& Where, FName Tag, const TCHAR* Name, const TArray<FStoryLine>& Said)
	{
		ASpeakerPoint* Point = World->SpawnActor<ASpeakerPoint>(Where, FRotator(0.0, (-Where).Rotation().Yaw, 0.0));
		if (!Point)
		{
			return nullptr;
		}
		Point->SpeakerPoint->SetRelativeLocation(FVector::ZeroVector);
		Point->SpeakerPoint->SpeakerName = FText::FromString(Name);
		Point->SpeakerPoint->Lines = Said;
		Point->Tags.Add(Tag);
		Point->DispatchBeginPlay();
		return Point;
	}

	/** A story character in a test level with one line to say; its play hasn't begun (set its condition first). */
	AStoryCharacter* PlaceCharacter(UWorld* World, const FVector& Where, double Yaw, FName Tag, const TCHAR* Name)
	{
		AStoryCharacter* Placed = World->SpawnActor<AStoryCharacter>(Where, FRotator(0.0, Yaw, 0.0));
		if (!Placed)
		{
			return nullptr;
		}
		Placed->SpeakerPoint->SpeakerName = FText::FromString(Name);
		Placed->SpeakerPoint->Lines = { MakeTestLine(TEXT(""), TEXT("Well. I've seen worse reunions."), 2.f) };
		Placed->Tags.Add(Tag);
		return Placed;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryCaptionsTest, "Looter.Story.Captions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStoryCaptionsTest::RunTest(const FString& Parameters)
{
	// The caption queue's clock: lines in the order they came, each on screen for its seconds, fading in at its start and
	// out at its end, the next right after; a conversation that interrupts cuts the line on screen short and drops what
	// waited. Then the level's caption subsystem: the same, held while a menu covers the game.
	FCaptionQueue Queue;
	TestTrue(TEXT("Nothing said: nothing on screen"), Queue.IsEmpty() && !Queue.GetCurrent() && Queue.GetAlpha() == 0.f);
	TestEqual(TEXT("Lines with no words: nothing queued"), Queue.Enqueue({ MakeTestLine(TEXT("Hob"), TEXT("")) }), 0);

	const int32 Porch = Queue.Enqueue({ MakeTestLine(TEXT("Delia"), TEXT("First"), 2.f), MakeTestLine(TEXT("Delia"), TEXT("Second"), 3.f) });
	const int32 Remark = Queue.Enqueue({ MakeTestLine(TEXT("Hob"), TEXT("Third"), 1.5f) });
	TestTrue(TEXT("Two conversations"), Porch != 0 && Remark != 0 && Porch != Remark);
	TestEqual(TEXT("Three lines queued"), Queue.Num(), 3);
	TestEqual(TEXT("The first line comes first"), CaptionWords(Queue), FString(TEXT("First")));
	TestEqual(TEXT("It starts unseen"), Queue.GetAlpha(), 0.f);
	Queue.Advance(FCaptionQueue::FadeInSeconds * 0.5f);
	TestEqual(TEXT("Halfway faded in"), Queue.GetAlpha(), 0.5f, 0.01f);
	Queue.Advance(1.f - FCaptionQueue::FadeInSeconds * 0.5f);
	TestEqual(TEXT("A second in: fully shown"), Queue.GetAlpha(), 1.f);
	Queue.Advance(0.8f);
	TestTrue(TEXT("Near its end it fades out"), CaptionWords(Queue) == TEXT("First") && FMath::IsNearlyEqual(Queue.GetAlpha(), 0.5f, 0.01f));
	Queue.Advance(0.3f);
	TestEqual(TEXT("Its two seconds up: the second line"), CaptionWords(Queue), FString(TEXT("Second")));
	TestEqual(TEXT("...a tenth of a second in (the rest of the step)"), Queue.GetElapsed(), 0.1f, 0.001f);
	Queue.Advance(2.95f);
	TestEqual(TEXT("Then the other conversation's line"), CaptionWords(Queue), FString(TEXT("Third")));
	TestTrue(TEXT("The first conversation is over, the second isn't"), !Queue.IsPlaying(Porch) && Queue.IsPlaying(Remark));
	Queue.Advance(1.5f);
	TestTrue(TEXT("All said"), Queue.IsEmpty() && !Queue.GetCurrent() && !Queue.IsPlaying(Remark));

	// One long frame ends several lines, in order, and the next starts with what's left of it.
	Queue.Enqueue({ MakeTestLine(TEXT("Hob"), TEXT("One"), 1.f), MakeTestLine(TEXT("Hob"), TEXT("Two"), 1.f), MakeTestLine(TEXT("Hob"), TEXT("Three"), 1.f) });
	Queue.Advance(2.5f);
	TestTrue(TEXT("A long frame: the third line, half a second in"), CaptionWords(Queue) == TEXT("Three") && FMath::IsNearlyEqual(Queue.GetElapsed(), 0.5f, 0.001f));
	Queue.Advance(100.f);
	TestTrue(TEXT("A very long one: all said"), Queue.IsEmpty());

	// Interrupting: the line on screen fades out quickly, what waited is dropped, the new lines follow.
	const int32 Old = Queue.Enqueue({ MakeTestLine(TEXT("Hob"), TEXT("Old one"), 4.f), MakeTestLine(TEXT("Hob"), TEXT("Old two"), 4.f) });
	Queue.Advance(1.f);
	const int32 New = Queue.Interrupt({ MakeTestLine(TEXT("Delia"), TEXT("New"), 2.f) });
	TestTrue(TEXT("Cut short: the old conversation is over at once"), !Queue.IsPlaying(Old) && Queue.IsPlaying(New));
	TestTrue(TEXT("The cut line fades out first, and what waited is gone"), CaptionWords(Queue) == TEXT("Old one") && Queue.Num() == 2);
	Queue.Advance(FCaptionQueue::CutSeconds * 0.5f);
	TestEqual(TEXT("Halfway out"), Queue.GetAlpha(), 0.5f, 0.01f);
	Queue.Advance(FCaptionQueue::CutSeconds * 0.5f + 0.05f);
	TestEqual(TEXT("Then the new line"), CaptionWords(Queue), FString(TEXT("New")));
	Queue.Advance(0.5f);
	Queue.Clear();
	TestTrue(TEXT("Cleared: the line fades out, its conversation over"), CaptionWords(Queue) == TEXT("New") && !Queue.IsPlaying(New));
	Queue.Advance(FCaptionQueue::CutSeconds + 0.01f);
	TestTrue(TEXT("...then nothing"), Queue.IsEmpty());
	Queue.Enqueue({ MakeTestLine(TEXT("Hob"), TEXT("Unseen"), 2.f) });
	Queue.Interrupt({ MakeTestLine(TEXT("Delia"), TEXT("At once"), 2.f) });
	TestTrue(TEXT("A line not yet seen gives way at once"), CaptionWords(Queue) == TEXT("At once") && Queue.Num() == 1);

	// No seconds set: long enough to read, longer for more words.
	const FStoryLine Short = MakeTestLine(TEXT("Hob"), TEXT("Hm."));
	const FStoryLine Long = MakeTestLine(TEXT("Delia"), TEXT("A keeper doesn't lie still while his saint is dark. He walks the boards at dusk."));
	TestTrue(TEXT("Reading time"), Short.GetSeconds() >= FStoryLine::MinSeconds && Long.GetSeconds() > Short.GetSeconds() + 2.f);
	TestEqual(TEXT("Seconds set: those"), MakeTestLine(TEXT("Hob"), TEXT("Hm."), 7.f).GetSeconds(), 7.f);

	// The level's captions: the same queue, which waits while held (a menu over the game).
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(TestLevel.GetTestWorld());
	if (!TestNotNull(TEXT("The level has captions"), Captions))
	{
		return false;
	}
	const int32 Said = Captions->Play({ MakeTestLine(TEXT("Delia"), TEXT("One"), 2.f), MakeTestLine(TEXT("Delia"), TEXT("Two"), 2.f) });
	TestTrue(TEXT("Playing"), Captions->IsPlaying(Said) && CaptionOnScreen(*Captions) == TEXT("Delia: One"));
	Captions->SetHeld(true);
	Captions->Update(5.f);
	TestEqual(TEXT("Held: the line waits"), CaptionOnScreen(*Captions), FString(TEXT("Delia: One")));
	Captions->SetHeld(false);
	Captions->Update(2.5f);
	TestEqual(TEXT("Let go: on to the next"), CaptionOnScreen(*Captions), FString(TEXT("Delia: Two")));
	const int32 Queued = Captions->Play({ MakeTestLine(TEXT("Hob"), TEXT("Three"), 1.f) }, ECaptionPlay::Queue);
	TestTrue(TEXT("Queued, not interrupting"), Captions->IsPlaying(Said) && Captions->IsPlaying(Queued));
	Captions->Update(2.f);
	TestEqual(TEXT("Its turn after"), CaptionOnScreen(*Captions), FString(TEXT("Hob: Three")));
	Captions->Update(1.f);
	TestTrue(TEXT("All said"), !Captions->IsPlaying(Said) && !Captions->IsPlaying(Queued) && !Captions->GetCurrent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStorySpeakerPointTest, "Looter.Story.SpeakerPoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStorySpeakerPointTest::RunTest(const FString& Parameters)
{
	// Talking at a speaker point through the Interact key: its tap plays the speaker's lines as captions and raises the
	// Talk event the talk objective waits for, about the actor carrying the speaker's tag (someone else's doesn't do it).
	// While the lines play it can't be talked to again. What it says follows the story, and a topic's event starts the
	// mission waiting on it.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	if (!TestNotNull(TEXT("The level has a mission runner"), Runner) || !TestNotNull(TEXT("and captions"), Captions))
	{
		return false;
	}

	// Main 1's last step, talk to Delia at the screen door; and a mission her next words start.
	const FName Valley(TEXT("TestValley"));
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Porch = MissionTestWorld::NewMission(Scratch, TEXT("TestPorch"), EMissionKind::Main, EMissionStart::Automatic, Valley);
	MissionTestWorld::AddObjective<UMissionTalkObjective>(Porch, 0)->SpeakerTag = TEXT("Speaker_Delia");
	UMissionDefinition* Lantern = MissionTestWorld::NewMission(Scratch, TEXT("TestLantern"), EMissionKind::Main, EMissionStart::OnEvent, Valley);
	Lantern->StartEvent = TEXT("Delia.Lantern");
	Lantern->Prerequisites = { FName(TEXT("TestPorch")) };
	MissionTestWorld::AddObjective<UMissionTalkObjective>(Lantern, 0)->SpeakerTag = TEXT("Speaker_Delia");

	// The stand-in at the origin; Delia's door straight ahead, Tilly's window off to the left.
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);
	ASpeakerPoint* Door = PlaceSpeaker(World, InteractionTestWorld::Ahead(150.0), TEXT("Speaker_Delia"), TEXT("Grandma Delia"),
		{ MakeTestLine(TEXT(""), TEXT("I was to lay you on the boards tonight."), 3.f), MakeTestLine(TEXT(""), TEXT("He walks the boards at dusk."), 3.f) });
	ASpeakerPoint* Window = PlaceSpeaker(World, FVector(0.0, 150.0, InteractionTestWorld::EyeHeight), TEXT("Speaker_Tilly"), TEXT("Tilly Bright"),
		{ MakeTestLine(TEXT(""), TEXT("You've ruined my collar, by the way."), 3.f) });
	if (!TestTrue(TEXT("Stand-in, door and window placed"), Player && Interaction && Door && Window))
	{
		return false;
	}
	Runner->BeginForTesting({ Porch, Lantern }, Campaign, Player, Valley);
	Runner->Update(0.f);
	TestTrue(TEXT("The porch mission runs"), Runner->IsRunning(TEXT("TestPorch")));

	// Looked at, the door offers a talk: a tap.
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Delia's door is what's looked at"), Interaction->GetFocusedActor() == Door);
	const FInteractionOptions DoorOptions = Interaction->GetFocusedOptions();
	TestTrue(TEXT("A tap, to talk"), DoorOptions.bTap && !DoorOptions.bHold && DoorOptions.TapPrompt.ToString() == TEXT("Talk"));

	// Tilly first: her lines play, named by her window, and Delia's objective isn't done.
	InteractionTestWorld::Face(Player, 90.0);
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Turned left: Tilly's window"), Interaction->GetFocusedActor() == Window);
	Interaction->PressInteract();
	Interaction->ReleaseInteract();
	TestTrue(TEXT("Tilly is talking"), Window->SpeakerPoint->IsTalking());
	TestEqual(TEXT("Her line, in her name"), CaptionOnScreen(*Captions), FString(TEXT("Tilly Bright: You've ruined my collar, by the way.")));
	TestTrue(TEXT("Talking to Tilly doesn't do Delia's objective"), Runner->IsRunning(TEXT("TestPorch")) && !Campaign.HasCompleted(TEXT("TestPorch")));

	// Delia: she has the floor, and the talk objective is done (its mission with it).
	InteractionTestWorld::Face(Player, 0.0);
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Back to Delia's door"), Interaction->GetFocusedActor() == Door);
	Interaction->PressInteract();
	Interaction->ReleaseInteract();
	TestTrue(TEXT("Talking at her door finishes the talk objective and its mission"), Campaign.HasCompleted(TEXT("TestPorch")));
	TestFalse(TEXT("The lantern waits for its own event"), Runner->IsRunning(TEXT("TestLantern")));
	TestTrue(TEXT("Tilly is cut off, Delia talks"), Door->SpeakerPoint->IsTalking() && !Window->SpeakerPoint->IsTalking());
	Captions->Update(FCaptionQueue::CutSeconds + 0.05f);
	TestEqual(TEXT("Delia's first line, named by her door"), CaptionOnScreen(*Captions),
		FString(TEXT("Grandma Delia: I was to lay you on the boards tonight.")));

	// While she talks there's nothing to press: no talking over her.
	Interaction->UpdateInteraction(0.f);
	TestNull(TEXT("Talking, the door offers nothing"), Interaction->GetFocusedActor());
	TestFalse(TEXT("...and can't be talked to"), Door->SpeakerPoint->Talk(Player));
	Captions->Update(10.f);
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Said and done: her door can be talked to again"), !Door->SpeakerPoint->IsTalking() && Interaction->GetFocusedActor() == Door);

	// What she says follows the story: before the porch one thing, after it the lantern, whose event starts the mission
	// waiting on it (and that same talk isn't taken as the new mission's first talk).
	FSpeakerTopic BeforePorch;
	BeforePorch.When.BeforeMissions = { FName(TEXT("TestPorch")) };
	BeforePorch.Lines = { MakeTestLine(TEXT(""), TEXT("Go on, now."), 2.f) };
	FSpeakerTopic AfterPorch;
	AfterPorch.When.AfterMissions = { FName(TEXT("TestPorch")) };
	AfterPorch.Lines = { MakeTestLine(TEXT(""), TEXT("Take him the lantern."), 2.f) };
	AfterPorch.Event = TEXT("Delia.Lantern");
	Door->SpeakerPoint->Topics = { BeforePorch, AfterPorch };
	int32 Topic = INDEX_NONE;
	const TArray<FStoryLine> Now = Door->SpeakerPoint->GetLinesNow(&Topic);
	TestTrue(TEXT("After the porch: the lantern, in her name"), Topic == 1 && Now.Num() == 1 && Now[0].Text.ToString() == TEXT("Take him the lantern.")
		&& Now[0].Speaker.ToString() == TEXT("Grandma Delia"));
	TestTrue(TEXT("Talked to"), Door->SpeakerPoint->Talk(Player));
	TestTrue(TEXT("Its event started the lantern"), Runner->IsRunning(TEXT("TestLantern")));
	const TArray<FMissionObjectiveView> LanternViews = Runner->GetObjectiveViews(TEXT("TestLantern"));
	TestTrue(TEXT("...whose talk objective that same talk didn't do"), LanternViews.Num() == 1 && !LanternViews[0].bDone);

	// Closed (a mission shuts the door a while): nothing to talk to.
	Captions->Update(10.f);
	Door->SpeakerPoint->bEnabled = false;
	Interaction->UpdateInteraction(0.f);
	TestNull(TEXT("A closed door offers nothing"), Interaction->GetFocusedActor());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryCharactersTest, "Looter.Story.Characters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStoryCharactersTest::RunTest(const FString& Parameters)
{
	// Story characters come and go with the story, read from the campaign record whenever the missions change: one there
	// while a mission is played, one after it's finished, one always. Hidden, they can't be talked to or bumped into. They
	// aren't creatures (nothing to hurt), and talking, one turns to whoever talks to it and back after.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	if (!TestNotNull(TEXT("The level has a mission runner"), Runner) || !TestNotNull(TEXT("and captions"), Captions))
	{
		return false;
	}
	const FName Valley(TEXT("TestValley"));
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Deal = MissionTestWorld::NewMission(Scratch, TEXT("TestDeal"), EMissionKind::Main, EMissionStart::Automatic, Valley);
	MissionTestWorld::AddObjective<UMissionEventObjective>(Deal, 0)->Event = TEXT("Deal.Struck");

	// Sexton on the rail while the deal is played; Abel on his board after it, his back to the player; Hob always.
	AActor* Listener = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector);
	AStoryCharacter* Sexton = PlaceCharacter(World, FVector(500.0, 0.0, 0.0), 180.0, TEXT("Speaker_Sexton"), TEXT("Mister Sexton"));
	AStoryCharacter* Abel = PlaceCharacter(World, FVector(300.0, 0.0, 0.0), 0.0, TEXT("Speaker_Abel"), TEXT("Abel Ransom"));
	AStoryCharacter* Hob = PlaceCharacter(World, FVector(0.0, 300.0, 0.0), 0.0, TEXT("Speaker_Hob"), TEXT("Hob"));
	if (!TestTrue(TEXT("Listener and characters placed"), Listener && Sexton && Abel && Hob))
	{
		return false;
	}
	Sexton->ShownWhen.DuringMission = TEXT("TestDeal");
	Abel->ShownWhen.AfterMissions = { FName(TEXT("TestDeal")) };
	Runner->BeginForTesting({ Deal }, Campaign, Listener, Valley);
	Sexton->DispatchBeginPlay();
	Abel->DispatchBeginPlay();
	Hob->DispatchBeginPlay();

	TestTrue(TEXT("Before the deal: no Sexton"), !Sexton->IsShown() && Sexton->IsHidden() && !Sexton->GetActorEnableCollision()
		&& !Sexton->SpeakerPoint->CanTalk());
	TestFalse(TEXT("No Abel either"), Abel->IsShown());
	TestTrue(TEXT("Hob is always about"), Hob->IsShown() && !Hob->IsHidden() && Hob->SpeakerPoint->CanTalk());

	Runner->Update(0.f);
	TestTrue(TEXT("The deal under way: Sexton is there, solid, to be talked to"), Runner->IsRunning(TEXT("TestDeal")) && Sexton->IsShown()
		&& !Sexton->IsHidden() && Sexton->GetActorEnableCollision() && Sexton->SpeakerPoint->CanTalk());

	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Deal.Struck")));
	TestTrue(TEXT("The deal struck: Sexton is gone"), Campaign.HasCompleted(TEXT("TestDeal")) && !Sexton->IsShown());
	TestTrue(TEXT("...and Abel sits on his board"), Abel->IsShown() && Abel->SpeakerPoint->CanTalk());
	TestNull(TEXT("Not a creature: nothing to hurt"), Abel->FindComponentByClass<UHealthComponent>());

	// Posed by code: talked to from behind, he turns to the listener while his line plays, then back as he was placed.
	TestTrue(TEXT("Talked to"), Abel->SpeakerPoint->Talk(Listener));
	Abel->UpdatePose(2.f);
	TestEqual(TEXT("Turned round to the listener"), FMath::Abs(FRotator::NormalizeAxis(Abel->Body->GetComponentRotation().Yaw)), 180.0, 1.0);
	Captions->Update(10.f);
	Abel->UpdatePose(2.f);
	TestEqual(TEXT("Said and done: back as placed"), FRotator::NormalizeAxis(Abel->Body->GetComponentRotation().Yaw), 0.0, 1.0);
	return true;
}

#endif

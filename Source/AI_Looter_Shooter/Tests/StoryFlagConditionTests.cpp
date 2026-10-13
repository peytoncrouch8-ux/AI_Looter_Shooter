#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Economy/Vendor.h"
#include "Interaction/InteractionComponent.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionQueue.h"
#include "Story/CaptionSubsystem.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryCondition.h"
#include "Story/StoryLine.h"
#include "Tests/InteractionTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Story conditions that follow story flags (FStoryCondition::WithFlags / WithoutFlags, read from the campaign record), and
// a vendor's hatch as a mission giver and turn-in (AVendor::Voice: Mrs. Pruitt takes the Gravemother's bounty there).

namespace
{
	const FName PaidFlag(TEXT("TestFlag.Paid"));
	const FName KeptFlag(TEXT("TestFlag.Kept"));
	const FName SeenFlag(TEXT("TestFlag.Seen"));
	const FName PruittTag(TEXT("Speaker_TestPruitt"));

	/** Who says the caption on screen, and what: "Mrs. Pruitt|...", or "none". */
	FString OnScreen(const UCaptionSubsystem& Captions)
	{
		const FCaptionEntry* Current = Captions.GetCurrent();
		return Current ? Current->Line.Speaker.ToString() + TEXT("|") + Current->Line.Text.ToString() : FString(TEXT("none"));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryFlagConditionTest, "Looter.Story.Condition.Flags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStoryFlagConditionTest::RunTest(const FString& Parameters)
{
	// A condition may ask for story flags set, or not set, as the campaign record keeps them: every WithFlags one must be
	// set and no WithoutFlags one, together with whatever else it asks. Flags count toward "asks nothing" and are said when
	// described.
	FCampaignRecord Campaign;

	// Nothing asked: always, flags or none.
	FStoryCondition Nothing;
	TestTrue(TEXT("Empty asks nothing"), Nothing.IsEmpty());
	TestTrue(TEXT("...and applies"), Nothing.IsMet(Campaign));
	TestEqual(TEXT("...always"), Nothing.Describe(), FString(TEXT("always")));
	Campaign.SetFlag(PaidFlag);
	TestTrue(TEXT("...a flag set changes nothing"), Nothing.IsMet(Campaign));
	Campaign.ClearFlag(PaidFlag);

	// With flags: every one set.
	FStoryCondition Paid;
	Paid.WithFlags = { PaidFlag };
	TestFalse(TEXT("A flag asked for isn't empty"), Paid.IsEmpty());
	TestFalse(TEXT("With the flag unset: not met"), Paid.IsMet(Campaign));
	Campaign.SetFlag(PaidFlag);
	TestTrue(TEXT("...set: met"), Paid.IsMet(Campaign));
	Campaign.ClearFlag(PaidFlag);
	TestFalse(TEXT("...cleared again: not met"), Paid.IsMet(Campaign));

	FStoryCondition Both;
	Both.WithFlags = { PaidFlag, SeenFlag };
	Campaign.SetFlag(PaidFlag);
	TestFalse(TEXT("Two asked, one set: not met"), Both.IsMet(Campaign));
	Campaign.SetFlag(SeenFlag);
	TestTrue(TEXT("...both set: met"), Both.IsMet(Campaign));
	Campaign.ClearFlag(PaidFlag);
	Campaign.ClearFlag(SeenFlag);

	// Without flags: none set.
	FStoryCondition NotKept;
	NotKept.WithoutFlags = { KeptFlag };
	TestFalse(TEXT("A flag ruled out isn't empty"), NotKept.IsEmpty());
	TestTrue(TEXT("Without the flag set: met"), NotKept.IsMet(Campaign));
	Campaign.SetFlag(KeptFlag);
	TestFalse(TEXT("...set: not met"), NotKept.IsMet(Campaign));
	Campaign.ClearFlag(KeptFlag);
	TestTrue(TEXT("...cleared again: met"), NotKept.IsMet(Campaign));

	// Together, and with missions: all of it holds.
	FStoryCondition Paid_NotKept_After;
	Paid_NotKept_After.AfterMissions = { FName(TEXT("TestMain1")) };
	Paid_NotKept_After.WithFlags = { PaidFlag };
	Paid_NotKept_After.WithoutFlags = { KeptFlag };
	Campaign.SetFlag(PaidFlag);
	TestFalse(TEXT("The flags right but the mission unfinished: not met"), Paid_NotKept_After.IsMet(Campaign));
	Campaign.Complete(TEXT("TestMain1"));
	TestTrue(TEXT("...the mission finished too: met"), Paid_NotKept_After.IsMet(Campaign));
	Campaign.SetFlag(KeptFlag);
	TestFalse(TEXT("...a ruled-out flag set as well: not met"), Paid_NotKept_After.IsMet(Campaign));
	Campaign.ClearFlag(KeptFlag);
	Campaign.ClearFlag(PaidFlag);
	TestFalse(TEXT("...the wanted flag gone: not met"), Paid_NotKept_After.IsMet(Campaign));

	// A flag with no name is skipped, as an empty mission id is.
	FStoryCondition Nameless;
	Nameless.WithFlags = { NAME_None };
	Nameless.WithoutFlags = { NAME_None };
	TestTrue(TEXT("Nameless flags are ignored"), Nameless.IsMet(Campaign));

	// Said in words, after the missions' and before the mission being played.
	FStoryCondition Said;
	Said.AfterMissions = { FName(TEXT("Main1")) };
	Said.WithFlags = { PaidFlag, SeenFlag };
	Said.WithoutFlags = { KeptFlag };
	TestEqual(TEXT("Described: the flags after the missions"), Said.Describe(),
		FString(TEXT("after Main1; with flags TestFlag.Paid, TestFlag.Seen; without flags TestFlag.Kept")));
	Said.DuringMission = TEXT("Main2");
	TestEqual(TEXT("...and before the mission being played"), Said.Describe(),
		FString(TEXT("after Main1; with flags TestFlag.Paid, TestFlag.Seen; without flags TestFlag.Kept; during Main2")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVendorMissionVoiceTest, "Looter.Economy.VendorVoice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FVendorMissionVoiceTest::RunTest(const FString& Parameters)
{
	// A vendor's hatch is a mission giver and turn-in through its Voice: while a mission waits to be turned in there or one is
	// offered, Interact talks to the voice (its prompt, its words in its name, shut shop or not); the rest of the time it's
	// the shop it always was.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);
	AVendor* Hatch = World->SpawnActor<AVendor>(InteractionTestWorld::Ahead(150.0), FRotator(0.0, 180.0, 0.0));
	AVendor* Plain = World->SpawnActor<AVendor>(FVector(0.0, 900.0, 0.0), FRotator::ZeroRotator);
	if (!Runner || !Captions || !Player || !Interaction || !Hatch || !Plain || !Hatch->Voice || !Plain->Voice)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}

	// As the build script places Pruitt's: the voice's name and tag set on the actor, taken as play begins.
	Hatch->Prompt = FText::FromString(TEXT("Trade at Pruitt's"));
	Hatch->VoiceName = FText::FromString(TEXT("Mrs. Pruitt"));
	Hatch->VoiceTag = PruittTag;
	Hatch->DispatchBeginPlay();
	Plain->DispatchBeginPlay();
	TestTrue(TEXT("The voice speaks only for the story"), Hatch->Voice->bMissionsOnly);
	TestEqual(TEXT("...under the vendor's voice name"), Hatch->Voice->SpeakerName.ToString(), FString(TEXT("Mrs. Pruitt")));
	TestTrue(TEXT("...the actor carries the voice's tag, and the vendor's"), Hatch->ActorHasTag(PruittTag) && Hatch->ActorHasTag(AVendor::VendorTag));
	TestTrue(TEXT("A vendor with no voice name or tag adds none"), Plain->Voice->SpeakerName.IsEmpty() && Plain->Tags.Num() == 1
		&& Plain->ActorHasTag(AVendor::VendorTag));

	// The bounty is turned in to the hatch; a chore of hers is offered once it's done.
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Bounty = MissionTestWorld::NewMission(Scratch, TEXT("TestBounty"), EMissionKind::Main, EMissionStart::Automatic, NAME_None);
	MissionTestWorld::AddObjective<UMissionEventObjective>(Bounty, 0)->Event = TEXT("Test.Bounty");
	Bounty->TurnIn.SpeakerTag = PruittTag;
	Bounty->TurnIn.GiverName = FText::FromString(TEXT("Mrs. Pruitt"));
	Bounty->TurnIn.Lines = { FStoryLine::Make(FText::GetEmpty(), FText::FromString(TEXT("Salt for the jar. Mind the step.")), 3.f) };
	UMissionDefinition* Chore = MissionTestWorld::NewMission(Scratch, TEXT("TestChore"), EMissionKind::Side, EMissionStart::Offered, NAME_None);
	MissionTestWorld::AddObjective<UMissionEventObjective>(Chore, 0)->Event = TEXT("Test.Chore");
	Chore->Offer.GiverTag = PruittTag;
	Chore->Offer.Lines = { FStoryLine::Make(FText::GetEmpty(), FText::FromString(TEXT("While you're here: the cellar's flooded.")), 3.f) };
	Chore->TurnIn.bAutomatic = true;
	Chore->Prerequisites = { FName(TEXT("TestBounty")) };
	Runner->BeginForTesting({ Bounty, Chore }, Campaign, Player);
	Runner->Update(0.f);

	// No mission business: the shop it always was.
	TestFalse(TEXT("Nothing to turn in or offer: the voice has no business"), Hatch->Voice->HasMissionBusiness());
	FInteractionOptions Options = Hatch->GetInteractionOptions(*Interaction);
	TestTrue(TEXT("The shop's own prompt, a tap"), Options.CanUse() && Options.bTap && !Options.bHold
		&& Options.TapPrompt.ToString() == TEXT("Trade at Pruitt's") && FMath::IsNearlyEqual(Options.Reach, Hatch->Reach));

	// Shut by the story: nothing, as before.
	Hatch->OpenWhen.AfterMissions = { FName(TEXT("TestNeverDone")) };
	TestFalse(TEXT("Shut and with no business: nothing to use"), Hatch->GetInteractionOptions(*Interaction).CanUse());
	TestFalse(TEXT("...and nothing happens"), Hatch->Interact(*Interaction, false));

	// Ready to turn in, the shop still shut: the voice's "Turn in", and it takes it.
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.Bounty")));
	TestTrue(TEXT("The bounty waits to be turned in at the hatch"), Runner->IsReadyToTurnIn(TEXT("TestBounty")) && Hatch->Voice->HasMissionBusiness());
	Options = Hatch->GetInteractionOptions(*Interaction);
	TestTrue(TEXT("The voice's key, shut shop or not: Turn in"), Options.CanUse() && Options.TapPrompt.ToString() == TEXT("Turn in"));
	TestTrue(TEXT("Interact talks to the voice"), Hatch->Interact(*Interaction, false));
	TestTrue(TEXT("...the bounty is turned in"), Campaign.HasCompleted(TEXT("TestBounty")));
	TestEqual(TEXT("...her words in her name"), OnScreen(*Captions), FString(TEXT("Mrs. Pruitt|Salt for the jar. Mind the step.")));

	// The chore is next, offered at the hatch once the words are said (no HUD in a test level: the card never opens).
	Captions->Update(30.f);
	Hatch->Voice->UpdatePendingOffer();
	TestTrue(TEXT("The chore is offered at the hatch"), Runner->IsAvailable(TEXT("TestChore")) && Hatch->Voice->HasMissionBusiness());
	Options = Hatch->GetInteractionOptions(*Interaction);
	TestTrue(TEXT("The voice's key for an offer: Talk, though the shop is shut"), Options.CanUse() && Options.TapPrompt.ToString() == TEXT("Talk"));
	TestTrue(TEXT("Interact talks to the voice"), Hatch->Interact(*Interaction, false));
	TestEqual(TEXT("...the offer's words in her name"), OnScreen(*Captions), FString(TEXT("Mrs. Pruitt|While you're here: the cellar's flooded.")));

	// Taken, nothing is left to say: the shop again, shut then open.
	Captions->Update(30.f);
	Hatch->Voice->UpdatePendingOffer();
	TestTrue(TEXT("The chore taken"), Runner->AcceptMission(TEXT("TestChore")));
	TestFalse(TEXT("...the voice has no business again"), Hatch->Voice->HasMissionBusiness());
	TestFalse(TEXT("The shop is shut again"), Hatch->GetInteractionOptions(*Interaction).CanUse());
	Hatch->OpenWhen = FStoryCondition();
	Options = Hatch->GetInteractionOptions(*Interaction);
	TestTrue(TEXT("...and open: its own prompt"), Options.CanUse() && Options.TapPrompt.ToString() == TEXT("Trade at Pruitt's"));
	// A tap opens the screen, which a test level has no HUD for; the voice stays quiet.
	TestFalse(TEXT("Interact tries the shop's screen, not the voice"), Hatch->Interact(*Interaction, false));
	TestEqual(TEXT("...nothing said"), OnScreen(*Captions), FString(TEXT("none")));
	return true;
}

#endif

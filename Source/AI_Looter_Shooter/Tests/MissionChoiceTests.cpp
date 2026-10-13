#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryLine.h"
#include "Tests/MissionTestWorld.h"
#include "UI/World/MissionOfferWidget.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// A turn-in with a choice of two (wave 2a, Back Pay): the talk at the giver doesn't finish it, the choice does, with that
// choice's rewards in place of the mission's own (its experience either way), its story flag set and kept; the console
// and a board turn it in with the first; the card's words say both outcomes before the player commits.

using namespace MissionTestWorld;

namespace
{
	const FName ShiftTag(TEXT("Speaker_TestShift"));
	const FName PaidFlag(TEXT("TestPay.Paid"));
	const FName KeptFlag(TEXT("TestPay.Kept"));

	/** One step (an event), turned in at the shift's tag board for 45 XP, with a choice: pay them (a flag) or keep 400 silver. */
	UMissionDefinition* MakeBackPay(UObject* Outer, const TCHAR* Id)
	{
		UMissionDefinition* Mission = NewMission(Outer, Id, EMissionKind::Side, EMissionStart::Automatic, NAME_None);
		AddObjective<UMissionEventObjective>(Mission, 0)->Event = TEXT("Test.Payroll");
		Mission->TurnIn.SpeakerTag = ShiftTag;
		Mission->TurnIn.GiverName = FText::FromString(TEXT("The shift"));
		Mission->TurnIn.ChoiceQuestion = FText::FromString(TEXT("Their wages, or the company's silver?"));
		Mission->Rewards.Experience = 45;
		Mission->Rewards.Silver = 30;
		FMissionTurnInChoice& Pay = Mission->TurnIn.Choices.AddDefaulted_GetRef();
		Pay.Title = FText::FromString(TEXT("Pay the eleven"));
		Pay.Line = FText::FromString(TEXT("The tags go on OUT with their wages."));
		Pay.Flag = PaidFlag;
		Pay.Stamp = FText::FromString(TEXT("Paid"));
		Pay.Lines = { FStoryLine::Make(FText::GetEmpty(), FText::FromString(TEXT("Paid in full. Shift's over, boys.")), 3.f) };
		FMissionTurnInChoice& Keep = Mission->TurnIn.Choices.AddDefaulted_GetRef();
		Keep.Title = FText::FromString(TEXT("Keep the payroll"));
		Keep.Line = FText::FromString(TEXT("The tags stay on IN, and the dead keep working."));
		Keep.Flag = KeptFlag;
		Keep.Rewards.Silver = 400;
		return Mission;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionChoiceTurnInTest, "Looter.Missions.Choice.TurnIn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionChoiceTurnInTest::RunTest(const FString& Parameters)
{
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	AActor* Board = SpawnMarker(World, FVector(400.0, 0.0, 0.0), ShiftTag);
	if (!Runner || !Player || !Board)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* BackPay = MakeBackPay(Scratch, TEXT("TestBackPay"));
	UMissionDefinition* Console = MakeBackPay(Scratch, TEXT("TestConsolePay"));
	const FName Id(TEXT("TestBackPay"));
	const FName ConsoleId(TEXT("TestConsolePay"));
	TestTrue(TEXT("Two choices make a choice"), BackPay->HasTurnInChoice());
	FMissionRewardsGiven Given;
	Runner->OnMissionCompleted.AddLambda([&Given](const UMissionDefinition&, const FMissionRewardsGiven& Heard) { Given = Heard; });
	Runner->BeginForTesting({ BackPay, Console }, Campaign, Player);
	Runner->Update(0.f);
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.Payroll")));
	TestTrue(TEXT("Both ready to turn in"), Runner->IsReadyToTurnIn(Id) && Runner->IsReadyToTurnIn(ConsoleId));

	// The talk at the board doesn't settle it: the player chooses on the card.
	Runner->NotifyEvent(FMissionEvent::Talked(Board));
	TestTrue(TEXT("A talk doesn't turn in a mission with a choice"), Runner->IsReadyToTurnIn(Id) && !Campaign.HasCompleted(Id));
	TestTrue(TEXT("...it's still the board's to take"), Runner->FindTurnInAt(Board) != nullptr);
	TestFalse(TEXT("A choice that isn't there does nothing"), Runner->TurnInWithChoice(Id, 2));

	// The card says both outcomes before anything is chosen.
	const FMissionOfferCard Card = FMissionOfferCard::MakeChoice(*BackPay, Board, World);
	TestEqual(TEXT("The card has the two"), Card.Choices.Num(), 2);
	TestTrue(TEXT("...each by its title"), Card.Choices.Num() == 2 && Card.Choices[0].Title.ToString() == TEXT("Pay the eleven")
		&& Card.Choices[1].Title.ToString() == TEXT("Keep the payroll"));
	TestTrue(TEXT("...what keeping it gives"), Card.Choices.Num() == 2 && Card.Choices[1].Rewards.ContainsByPredicate([](const FMissionOfferReward& Reward)
	{
		return Reward.Value.ToString().Contains(TEXT("400"));
	}));
	TestTrue(TEXT("...a choice that gives nothing says so"), Card.Choices.Num() == 2 && Card.Choices[0].Rewards.Num() == 1);
	TestTrue(TEXT("...the experience either way, above them"), Card.EitherWay.ContainsByPredicate([](const FMissionOfferReward& Reward)
	{
		return Reward.Value.ToString().Contains(TEXT("45"));
	}));
	TestEqual(TEXT("...the question"), Card.Question.ToString(), FString(TEXT("Their wages, or the company's silver?")));
	TestEqual(TEXT("...a stamp of its own, or SETTLED"), Card.Choices.Num() == 2 ? Card.Choices[1].Stamp.ToString() : FString(),
		FString(TEXT("SETTLED")));

	// Keep the payroll: its silver in place of the purse, the mission's experience, the flag kept.
	TestTrue(TEXT("Turned in with the second choice"), Runner->TurnInWithChoice(Id, 1));
	TestTrue(TEXT("Finished"), Campaign.HasCompleted(Id));
	TestTrue(TEXT("Its flag is set, the other's isn't"), Campaign.HasFlag(KeptFlag) && !Campaign.HasFlag(PaidFlag) && Runner->HasFlag(KeptFlag));
	TestEqual(TEXT("The choice is kept"), Runner->GetTurnInChoice(Id), 1);
	TestEqual(TEXT("Its silver, not the mission's purse"), Given.Silver, 400);
	TestEqual(TEXT("The mission's experience either way"), static_cast<int32>(Given.Experience), 45);
	TestEqual(TEXT("The banner hears which"), Given.Choice.ToString(), FString(TEXT("Keep the payroll")));

	// The console (or a board's screen) turns one in with the first.
	TestTrue(TEXT("Turned in without asking"), Runner->TurnIn(ConsoleId));
	TestTrue(TEXT("...with the first choice"), Runner->GetTurnInChoice(ConsoleId) == 0 && Campaign.HasFlag(PaidFlag));
	TestEqual(TEXT("...which gives no silver here"), Given.Silver, 0);

	// What a choice gives stands in for the mission's own, its experience the mission's when it names none.
	const FMissionRewards Kept = BackPay->RewardsFor(1);
	TestTrue(TEXT("RewardsFor: the choice's silver and the mission's experience"), Kept.Silver == 400 && Kept.Experience == 45);
	TestEqual(TEXT("RewardsFor without a choice: the mission's own"), BackPay->RewardsFor(INDEX_NONE).Silver, 30);
	BackPay->TurnIn.Choices.SetNum(1);
	TestFalse(TEXT("One choice is no choice"), BackPay->HasTurnInChoice());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionChoiceSpeakerTest, "Looter.Missions.Choice.SpeakerPoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionChoiceSpeakerTest::RunTest(const FString& Parameters)
{
	// Anything with a voice takes a turn-in: the shift's voice at a tag board, a speaking tube. One that speaks only for the
	// story (bMissionsOnly) is quiet until it has a mission; with a choice to make, its talk leaves the card to open after.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	ASpeakerPoint* Tube = World->SpawnActor<ASpeakerPoint>(FVector(200.0, 0.0, 0.0), FRotator(0.0, 180.0, 0.0));
	if (!Runner || !Player || !Tube)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Tube->Tags.Add(ShiftTag);
	Tube->SpeakerPoint->SetRelativeLocation(FVector(0.0, 0.0, 40.0));
	Tube->SpeakerPoint->SpeakerName = FText::FromString(TEXT("The shift"));
	Tube->SpeakerPoint->Lines = { FStoryLine::Make(FText::GetEmpty(), FText::FromString(TEXT("...")), 2.f) };
	Tube->SpeakerPoint->bMissionsOnly = true;
	Tube->DispatchBeginPlay();
	const FName Id(TEXT("TestTubePay"));
	UMissionDefinition* BackPay = MakeBackPay(CreatePackage(nullptr), TEXT("TestTubePay"));
	Runner->BeginForTesting({ BackPay }, Campaign, Player);
	Runner->Update(0.f);
	TestFalse(TEXT("Missions only, with nothing for the player: quiet"), Tube->SpeakerPoint->CanTalk());
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.Payroll")));
	TestTrue(TEXT("A turn-in waiting there: it talks"), Tube->SpeakerPoint->HasMissionBusiness() && Tube->SpeakerPoint->CanTalk());
	TestTrue(TEXT("Talked to"), Tube->SpeakerPoint->Talk(Player));
	TestTrue(TEXT("...the mission still waits for the choice"), Runner->IsReadyToTurnIn(Id) && !Campaign.HasCompleted(Id));
	TestTrue(TEXT("...which the card asks once the words are said"), Tube->SpeakerPoint->IsChoicePending()
		&& Tube->SpeakerPoint->GetPendingOffer() == Id);
	return true;
}

#endif

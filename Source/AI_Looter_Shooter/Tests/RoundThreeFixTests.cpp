#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Loot/LootBeamCrowd.h"
#include "Missions/MissionDefinition.h"
#include "UI/World/NoticeBoardWidget.h"
#include "World/NoticeBoard.h"
#include "UObject/Package.h"

// The rules from the Ransom's Rest play-check's round 3 (2026-10-23): one beam for a crowd of small drops, a job on a
// board opened by one click, and a turn-in that says why it moved to someone the steps never named.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoundThreeBeamCrowdTest, "Looter.Loot.BeamCrowd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRoundThreeBeamCrowdTest::RunTest(const FString& Parameters)
{
	using LootBeamCrowd::ChooseShown;
	const TArray<FVector> NoGuns;
	const TArray<FVector> Lone = { FVector(0.0, 0.0, 0.0) };
	TestTrue(TEXT("A lone drop keeps its beam (the user's 1.5 m white beam)"), ChooseShown(Lone, NoGuns)[0]);

	// A shower: five drops within a couple of metres, and one well apart.
	const TArray<FVector> Shower = { FVector(0.0, 0.0, 0.0), FVector(120.0, 0.0, 0.0), FVector(0.0, 200.0, 0.0), FVector(-150.0, -90.0, 0.0),
		FVector(250.0, 250.0, 0.0), FVector(1500.0, 0.0, 0.0) };
	const TArray<bool> Shown = ChooseShown(Shower, NoGuns);
	TestTrue(TEXT("The first to land stands for its crowd"), Shown[0]);
	TestTrue(TEXT("...the rest of the crowd dark"), !Shown[1] && !Shown[2] && !Shown[3] && !Shown[4]);
	TestTrue(TEXT("One far off keeps its own"), Shown[5]);

	// A gun lying among them: its beam shows the spot, the small ones near it go dark.
	const TArray<FVector> Gun = { FVector(1450.0, 0.0, 0.0) };
	const TArray<bool> ByGun = ChooseShown(Shower, Gun);
	TestTrue(TEXT("Beside a gun: no small beam"), !ByGun[5]);
	TestTrue(TEXT("...far from it, the crowd's one still stands"), ByGun[0]);

	// The first collected: the next of the crowd takes the beam.
	const TArray<FVector> Picked(Shower.GetData() + 1, Shower.Num() - 1);
	TestTrue(TEXT("Once it's taken, the next stands for the crowd"), ChooseShown(Picked, NoGuns)[0]);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoundThreeBoardClickTest, "Looter.UI.NoticeBoard.FirstClick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRoundThreeBoardClickTest::RunTest(const FString& Parameters)
{
	// A job up for the taking here opens its card at the first click; everything else waits for the second.
	TestTrue(TEXT("A job up for the taking here: one click"), UNoticeBoardWidget::OpensOnFirstClick(ENoticePosting::Offered, true, false));
	TestFalse(TEXT("...one picked up elsewhere: chosen only"), UNoticeBoardWidget::OpensOnFirstClick(ENoticePosting::Offered, false, false));
	TestFalse(TEXT("...a key acts on the chosen card only"), UNoticeBoardWidget::OpensOnFirstClick(ENoticePosting::Offered, true, true));
	TestFalse(TEXT("A turn-in waits for the second click"), UNoticeBoardWidget::OpensOnFirstClick(ENoticePosting::Ready, true, false));
	TestFalse(TEXT("So does tracking"), UNoticeBoardWidget::OpensOnFirstClick(ENoticePosting::Open, true, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoundThreeTurnInWhyTest, "Looter.Missions.TurnInWhy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRoundThreeTurnInWhyTest::RunTest(const FString& Parameters)
{
	UMissionDefinition* Mission = NewObject<UMissionDefinition>(GetTransientPackage());
	Mission->TurnIn.SpeakerTag = TEXT("Speaker_Tilly");
	Mission->TurnIn.GiverName = FText::FromString(TEXT("Tilly"));
	TestEqual(TEXT("Without a why: the plain line"), Mission->GetTurnInShortText(), FString(TEXT("Turn in to Tilly")));
	Mission->TurnIn.Why = FText::FromString(TEXT("Tilly's hearse car waits at the depot"));
	TestEqual(TEXT("Main 7: the tracker says why"), Mission->GetTurnInShortText(),
		FString(TEXT("Tilly's hearse car waits at the depot: turn in to Tilly")));
	TestEqual(TEXT("...and the Missions page"), Mission->GetTurnInText(),
		FString(TEXT("Ready to turn in: talk to Tilly. Tilly's hearse car waits at the depot.")));
	return true;
}

#endif

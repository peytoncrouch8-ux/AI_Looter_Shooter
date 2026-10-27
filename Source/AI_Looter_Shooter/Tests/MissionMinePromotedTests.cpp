#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Tests/MissionTestWorld.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"

// The mine in the main story (the user's note 32, 2026-10-27: "Cyrus Teague should be a part of the main story line. Abel
// should be the final boss so that means Cyrus is the second boss fight"): after Main 5 the story goes up the ore lift (Dead
// Lift, met at Hiram's stool) and into the Providence (Back Pay, met at the lift head's speaking tube, where Dead Lift is
// turned in), and only then to Delia's door for The Gravewind. Lost and Found (note 37) is Mr. Sloane's side job after
// the Old Fare. The data as create_mission_assets.py and create_side_mission_assets.py write it, played by a runner.

using namespace MissionTestWorld;

namespace
{
	const FName Rest(TEXT("RansomsRest"));

	UMissionDefinition* LoadMissionAsset(const TCHAR* Id)
	{
		const FString Name = FString(UMissionDefinition::AssetPrefix) + Id;
		const FString Path = FString(UMissionDefinition::AssetFolder) / Name;
		return FPackageName::DoesPackageExist(Path) ? LoadObject<UMissionDefinition>(nullptr, *(Path + TEXT(".") + Name)) : nullptr;
	}

	/** A runner in a fresh test level playing Missions from Campaign; null (with the test's error) when the level isn't whole. */
	UMissionRunner* StartRunner(FAutomationTestBase& Test, FTestWorldWrapper& Level, const TArray<UMissionDefinition*>& Missions,
		FCampaignRecord& Campaign)
	{
		if (!Test.TestTrue(TEXT("Test level made"), Level.CreateTestWorld(EWorldType::EditorPreview)))
		{
			return nullptr;
		}
		UMissionRunner* Runner = Level.GetTestWorld()->GetSubsystem<UMissionRunner>();
		AActor* Player = SpawnMarker(Level.GetTestWorld(), FVector::ZeroVector);
		if (!Runner || !Player)
		{
			Test.AddError(TEXT("The test level isn't whole."));
			return nullptr;
		}
		Runner->BeginForTesting(Missions, Campaign, Player, Rest);
		Runner->Update(0.f);
		return Runner;
	}

	TArray<FName> Done(std::initializer_list<const TCHAR*> Ids)
	{
		TArray<FName> Names;
		for (const TCHAR* Id : Ids)
		{
			Names.Add(FName(Id));
		}
		return Names;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionMinePromotedTest, "Looter.Missions.MinePromoted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionMinePromotedTest::RunTest(const FString& Parameters)
{
	UMissionDefinition* DeadLift = LoadMissionAsset(TEXT("DeadLift"));
	UMissionDefinition* BackPay = LoadMissionAsset(TEXT("BackPay"));
	UMissionDefinition* Gravewind = LoadMissionAsset(TEXT("Main6"));
	UMissionDefinition* LastWatch = LoadMissionAsset(TEXT("LastWatch"));
	UMissionDefinition* Gravemother = LoadMissionAsset(TEXT("Side3"));
	UMissionDefinition* LostAndFound = LoadMissionAsset(TEXT("LostAndFound"));
	if (!TestTrue(TEXT("Dead Lift, Back Pay, The Gravewind, The Last Watch, The Gravemother and Lost and Found are made"),
		DeadLift && BackPay && Gravewind && LastWatch && Gravemother && LostAndFound))
	{
		return false;
	}

	// The order, as data.
	TestTrue(TEXT("Dead Lift: a main mission after Main 5"), DeadLift->Kind == EMissionKind::Main
		&& DeadLift->Prerequisites == TArray<FName>{ FName(TEXT("Main5")) });
	TestTrue(TEXT("Back Pay: a main mission after Dead Lift"), BackPay->Kind == EMissionKind::Main
		&& BackPay->Prerequisites == TArray<FName>{ FName(TEXT("DeadLift")) });
	TestTrue(TEXT("The Gravewind (Abel, the last boss) waits for Back Pay (the Foreman, the second)"),
		Gravewind->Prerequisites.Contains(FName(TEXT("BackPay"))));
	TestTrue(TEXT("...sorted after the mine"), DeadLift->SortOrder < BackPay->SortOrder && BackPay->SortOrder < Gravewind->SortOrder);
	TestTrue(TEXT("Back Pay still ends in its choice at the tag board"), BackPay->HasTurnInChoice()
		&& BackPay->TurnIn.SpeakerTag == FName(TEXT("Speaker_TagBoard")));
	TestTrue(TEXT("The Last Watch and the Gravemother stay side missions"), LastWatch->Kind == EMissionKind::Side
		&& Gravemother->Kind == EMissionKind::Side);
	TestTrue(TEXT("Lost and Found: Mr. Sloane's side job at his ticket window after the Old Fare"),
		LostAndFound->Kind == EMissionKind::Side && LostAndFound->IsOffered()
		&& LostAndFound->Offer.GiverTag == FName(TEXT("Speaker_AgentWindow"))
		&& LostAndFound->Prerequisites.Contains(FName(TEXT("OldFare")))
		&& LostAndFound->TurnIn.SpeakerTag == FName(TEXT("Speaker_AgentWindow")));
	TestTrue(TEXT("...its keys a mission item"), LostAndFound->Items.ContainsByPredicate([](const FMissionItemInfo& Each)
	{
		return Each.Id == FName(TEXT("LostPropertyKeys"));
	}));
	TestTrue(TEXT("The mine's missions pay more than a side mission now (the story's turn-ins carry its levels)"),
		DeadLift->Rewards.Experience > Gravemother->Rewards.Experience && BackPay->Rewards.Experience > DeadLift->Rewards.Experience);

	const TArray<UMissionDefinition*> Missions = { DeadLift, BackPay, Gravewind, LastWatch, Gravemother };

	// Main 5 turned in: Hiram is the story's next meeting; Delia's door waits.
	FCampaignRecord AfterSink;
	AfterSink.CompletedMissions = Done({ TEXT("Main1"), TEXT("Main2"), TEXT("Main3"), TEXT("Main4"), TEXT("Main5") });
	FTestWorldWrapper FirstLevel;
	if (UMissionRunner* Runner = StartRunner(*this, FirstLevel, Missions, AfterSink))
	{
		TestTrue(TEXT("After Main 5: \"Talk to Hiram Coyle\""), Runner->IsMeeting(DeadLift->GetMissionId()));
		TestFalse(TEXT("...not yet the Providence"), Runner->IsMeeting(BackPay->GetMissionId()) || Runner->IsRunning(BackPay->GetMissionId()));
		TestFalse(TEXT("...nor The Gravewind"), Runner->IsMeeting(Gravewind->GetMissionId()) || Runner->IsRunning(Gravewind->GetMissionId()));
		TestTrue(TEXT("...the Gravemother on offer beside it"), Runner->IsAvailable(Gravemother->GetMissionId()));
	}

	// Dead Lift turned in: the speaking tube starts Back Pay; the Last Watch opens; Delia still waits.
	FCampaignRecord AfterLift = AfterSink;
	AfterLift.CompletedMissions.Add(DeadLift->GetMissionId());
	FTestWorldWrapper SecondLevel;
	if (UMissionRunner* Runner = StartRunner(*this, SecondLevel, Missions, AfterLift))
	{
		TestTrue(TEXT("After Dead Lift: \"Talk to Hiram Coyle\" at the lift head (Back Pay)"), Runner->IsMeeting(BackPay->GetMissionId()));
		TestTrue(TEXT("...the Last Watch on offer"), Runner->IsAvailable(LastWatch->GetMissionId()));
		TestFalse(TEXT("...The Gravewind still shut"), Runner->IsMeeting(Gravewind->GetMissionId()));
	}

	// Back Pay turned in (the Foreman down): Delia's door, The Gravewind.
	FCampaignRecord AfterMine = AfterLift;
	AfterMine.CompletedMissions.Add(BackPay->GetMissionId());
	FTestWorldWrapper ThirdLevel;
	if (UMissionRunner* Runner = StartRunner(*this, ThirdLevel, Missions, AfterMine))
	{
		TestTrue(TEXT("After Back Pay: \"Talk to Grandma Delia\" (The Gravewind)"), Runner->IsMeeting(Gravewind->GetMissionId()));
	}
	return true;
}

#endif

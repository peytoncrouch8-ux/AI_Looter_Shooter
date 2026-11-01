#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Economy/SecretCatalog.h"
#include "Economy/SecretRules.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/StoryLine.h"
#include "Tests/MissionTestWorld.h"
#include "Weapons/WeaponTypes.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"

// "Paid on Delivery" (the Wardens story, 2026-11-01; MAINS_2_7.md section 12; lanes/SECRETS.md): Sexton's price paid. Its
// asset as create_side_mission_assets.py makes it, its flow in a test level (it starts by itself after Main 6, the step
// ticks on the Ferryman's landing, never from the deck above), and the quarter off his secrets after it.

namespace
{
	const FName DeliveryMission(TEXT("PaidOnDelivery"));
	const FName DeliveryFerryTag(TEXT("SextonsFerry"));
	const TCHAR* const DeliveryAssetPath = TEXT("/Game/Data/Missions/DA_Mission_PaidOnDelivery.DA_Mission_PaidOnDelivery");

	UMissionDefinition* DeliveryLoad(FAutomationTestBase& Test)
	{
		if (!FPackageName::DoesPackageExist(TEXT("/Game/Data/Missions/DA_Mission_PaidOnDelivery")))
		{
			Test.AddWarning(TEXT("DA_Mission_PaidOnDelivery isn't made yet: run Tools/Unreal/create_side_mission_assets.py."));
			return nullptr;
		}
		return LoadObject<UMissionDefinition>(nullptr, DeliveryAssetPath);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPaidOnDeliveryMissionTest, "Looter.Story.PaidOnDelivery.Mission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPaidOnDeliveryMissionTest::RunTest(const FString& Parameters)
{
	// As §12 has it: a side job with no card after Main 6, one step down to the landing, his three lines after the card, 150
	// XP (300 until 2026-11-01: trimmed for the Pacing band, Docs/Progression.md) and a Rare gun; the manifest its own item.
	const UMissionDefinition* Asset = DeliveryLoad(*this);
	if (!Asset)
	{
		return true;
	}
	TestEqual(TEXT("Its id"), Asset->Id, DeliveryMission);
	TestEqual(TEXT("Paid on Delivery"), Asset->Title.ToString(), FString(TEXT("Paid on Delivery")));
	TestEqual(TEXT("A side job"), Asset->Kind, EMissionKind::Side);
	TestEqual(TEXT("No card: it starts by itself"), Asset->Start, EMissionStart::Automatic);
	TestTrue(TEXT("...after Main 6"), Asset->Prerequisites.Num() == 1 && Asset->Prerequisites[0] == FName(TEXT("Main6")));
	TestEqual(TEXT("One step"), Asset->Steps.Num(), 1);
	const UMissionReachObjective* Landing = Cast<UMissionReachObjective>(Asset->GetObjective(0, 0));
	TestTrue(TEXT("0: the Ferryman's landing"), Landing && Landing->Place.Actor.ActorTag == DeliveryFerryTag);
	TestTrue(TEXT("...within 4 m, height counted (never from the deck)"), Landing && FMath::IsNearlyEqual(Landing->Place.Radius, 400.f)
		&& !Landing->Place.bIgnoreHeight);
	TestEqual(TEXT("Turned in to Sexton"), Asset->TurnIn.SpeakerTag, FName(TEXT("Speaker_Sexton")));
	TestEqual(TEXT("...his three lines"), Asset->TurnIn.Lines.Num(), 3);
	TestTrue(TEXT("...the fortune, the account, the tip on Purcell"), Asset->TurnIn.Lines.Num() == 3
		&& Asset->TurnIn.Lines[0].Text.ToString().Contains(TEXT("Four hundred and twelve"))
		&& Asset->TurnIn.Lines[1].Text.ToString().Contains(TEXT("in the black"))
		&& Asset->TurnIn.Lines[2].Text.ToString().Contains(TEXT("Purcell")));
	for (const FStoryLine& Line : Asset->TurnIn.Lines)
	{
		TestTrue(FString::Printf(TEXT("\"%s\" fits a caption"), *Line.Text.ToString()), Line.Text.ToString().Len() <= 90);
	}
	TestEqual(TEXT("150 XP"), static_cast<int64>(Asset->Rewards.Experience), static_cast<int64>(150));
	TestTrue(TEXT("A Rare gun from the bilge"), Asset->Rewards.bGun && Asset->Rewards.GunRarityFloor == EWeaponRarity::Rare);
	TestNotNull(TEXT("The manifest is its item"), Asset->FindItem(TEXT("BargeManifest")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPaidOnDeliveryFlowTest, "Looter.Story.PaidOnDelivery.Flow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPaidOnDeliveryFlowTest::RunTest(const FString& Parameters)
{
	// In a test level after Main 6: under way by itself; on the deck above the landing the step waits; on the landing it's
	// ready to turn in.
	UMissionDefinition* Asset = DeliveryLoad(*this);
	if (!Asset)
	{
		return true;
	}
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Ferry = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector, DeliveryFerryTag);
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector(5000.0, 0.0, 90.0));
	if (!Runner || !Ferry || !Player)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Campaign.Complete(TEXT("Main6"));
	Runner->BeginForTesting({ Asset }, Campaign, Player, Asset->Area);
	Runner->Update(0.f);
	if (!TestTrue(TEXT("After Main 6: under way by itself"), Runner->IsRunning(DeliveryMission)))
	{
		return false;
	}
	// On the boards, 3.8 m over the landing and 2 m along: not there yet.
	Player->SetActorLocation(FVector(200.0, 0.0, 470.0));
	Runner->Update(0.1f);
	TestFalse(TEXT("From the deck above: not yet"), Runner->IsReadyToTurnIn(DeliveryMission));
	// Down the ladder, on the planks.
	Player->SetActorLocation(FVector(300.0, 0.0, 90.0));
	Runner->Update(0.1f);
	TestTrue(TEXT("On the landing: turn it in to Sexton"), Runner->IsReadyToTurnIn(DeliveryMission));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPaidOnDeliveryDiscountTest, "Looter.Story.PaidOnDelivery.Discount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPaidOnDeliveryDiscountTest::RunTest(const FString& Parameters)
{
	// "His secrets cost a quarter less from now on": the Rest's catalog names this mission, and its prices drop by a quarter.
	const FString Path = SecretRules::CatalogPathFor(TEXT("Lvl_RansomsRest"));
	if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(Path)))
	{
		AddWarning(TEXT("DA_Secrets_RansomsRest isn't made yet: run Tools/Unreal/create_secret_assets.py."));
		return true;
	}
	const USecretCatalog* Rest = LoadObject<USecretCatalog>(nullptr, *Path);
	if (!TestNotNull(TEXT("DA_Secrets_RansomsRest loads"), Rest))
	{
		return false;
	}
	TestEqual(TEXT("The discount's mission"), Rest->DiscountMission, DeliveryMission);
	FCampaignRecord Record;
	Record.Complete(TEXT("Main2"));
	const FSecretOffer* Box = Rest->FindOffer(TEXT("WidowsBox"));
	if (!TestNotNull(TEXT("The Widow's Strongbox is for sale"), Box))
	{
		return false;
	}
	TestEqual(TEXT("Full price before: 100"), SecretRules::PriceFor(*Box, 1, Rest->DiscountFor(Record)), 100);
	Record.Complete(DeliveryMission);
	TestEqual(TEXT("A quarter off after: 75"), SecretRules::PriceFor(*Box, 1, Rest->DiscountFor(Record)), 75);
	TestTrue(TEXT("...and his line says so"), Rest->GreetingFor(Record).ToString().Contains(TEXT("Paid in full")));
	return true;
}

#endif

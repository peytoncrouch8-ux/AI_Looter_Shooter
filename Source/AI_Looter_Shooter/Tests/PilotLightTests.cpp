#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionPlaceObjectives.h"
#include "World/PilotSortieRules.h"
#include "Misc/PackageName.h"

// Main 6, "The Pilot Light" (the Wardens story, 2026-11-01; MAINS_2_7.md section 10, lane MAIN67): its asset as
// create_mission_assets.py makes it, against the names the sortie, the barge and the scenes give the world
// (main67_src/CONTRACT67.md's step table). The world's own pieces have their tests (PilotSortieTests, BargeBoardingTests).

namespace
{
	const TCHAR* const PilotAssetPackage = TEXT("/Game/Data/Missions/DA_Mission_Main6");

	const UMissionDefinition* PilotLoad(FAutomationTestBase& Test)
	{
		if (!FPackageName::DoesPackageExist(PilotAssetPackage))
		{
			Test.AddWarning(TEXT("DA_Mission_Main6 isn't made yet: run Tools/Unreal/create_mission_assets.py."));
			return nullptr;
		}
		return LoadObject<UMissionDefinition>(nullptr, TEXT("/Game/Data/Missions/DA_Mission_Main6.DA_Mission_Main6"));
	}

	/** The step's objective of class T carrying Tag (on its target, place, spawner or event), or null. */
	template <typename T>
	const T* FindIn(const UMissionDefinition& Mission, int32 Step, TFunctionRef<bool(const T&)> Matches)
	{
		if (!Mission.Steps.IsValidIndex(Step))
		{
			return nullptr;
		}
		for (const UMissionObjective* Each : Mission.Steps[Step].Objectives)
		{
			if (const T* Typed = Cast<T>(Each); Typed && Matches(*Typed))
			{
				return Typed;
			}
		}
		return nullptr;
	}

	const UMissionInteractObjective* InteractOn(const UMissionDefinition& Mission, int32 Step, FName Tag)
	{
		return FindIn<UMissionInteractObjective>(Mission, Step, [Tag](const UMissionInteractObjective& Each)
		{
			return Each.Target.ActorTag == Tag;
		});
	}

	const UMissionClearObjective* ClearOf(const UMissionDefinition& Mission, int32 Step, FName Spawner)
	{
		return FindIn<UMissionClearObjective>(Mission, Step, [Spawner](const UMissionClearObjective& Each)
		{
			return Each.SpawnerId == Spawner;
		});
	}

	const UMissionSceneObjective* SceneIn(const UMissionDefinition& Mission, int32 Step, FName Scene)
	{
		return FindIn<UMissionSceneObjective>(Mission, Step, [Scene](const UMissionSceneObjective& Each)
		{
			return Each.Scene == Scene;
		});
	}

	const UMissionReachObjective* ReachTo(const UMissionDefinition& Mission, int32 Step, FName Tag)
	{
		return FindIn<UMissionReachObjective>(Mission, Step, [Tag](const UMissionReachObjective& Each)
		{
			return Each.Place.Actor.ActorTag == Tag;
		});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPilotLightMissionTest, "Looter.Story.PilotLight.Mission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPilotLightMissionTest::RunTest(const FString& Parameters)
{
	const UMissionDefinition* Asset = PilotLoad(*this);
	if (!Asset)
	{
		return true;
	}
	using namespace PilotLight;
	TestEqual(TEXT("Its id stays Main6 (saves and conditions key on it)"), Asset->Id, FName(TEXT("Main6")));
	TestEqual(TEXT("The Pilot Light"), Asset->Title.ToString(), FString(TEXT("The Pilot Light")));
	TestEqual(TEXT("Eight steps, the sortie's and the barge's"), Asset->Steps.Num(), ReadyStep);
	TestTrue(TEXT("After Back Pay"), Asset->Prerequisites.Contains(FName(TEXT("BackPay"))));
	TestTrue(TEXT("Met first at Holloway's wheel"), Asset->IsMetFirst() && Asset->Offer.GiverTag == FName(TEXT("Speaker_Holloway")));
	TestTrue(TEXT("...with her offer's words and the card's"), Asset->Offer.Lines.Num() >= 3 && !Asset->Offer.Briefing.IsEmpty());

	// The steps' order is the world's (PilotLight's constants): never reorder.
	const UMissionInteractObjective* CastOff = InteractOn(*Asset, CastOffStep, FName(TEXT("Landfall_AnchorWinch")));
	TestTrue(TEXT("0: cast off at the anchor winch, held"), CastOff && CastOff->bHold);
	TestTrue(TEXT("1: the tenders fought off (the sortie's event)"), FindIn<UMissionEventObjective>(*Asset, TendersStep,
		[](const UMissionEventObjective& Each) { return Each.Event == TendersDownEvent(); }) != nullptr);
	const UMissionInteractObjective* Launchers = InteractOn(*Asset, GrappleStep, LauncherTag());
	TestTrue(TEXT("2: both launchers, held"), Launchers && Launchers->bHold && Launchers->Count == 2);
	const UMissionClearObjective* Deck = ClearOf(*Asset, DeckStep, FName(TEXT("BargeDeck")));
	TestTrue(TEXT("3: the top deck's nine"), Deck && Deck->Count == 9 && !Deck->bOptional);
	const UMissionReachObjective* Hatch = ReachTo(*Asset, DeckStep, FName(TEXT("Barge_FurnaceHatch")));
	TestTrue(TEXT("...and the furnace hatch, height counted"), Hatch && !Hatch->Place.bIgnoreHeight);
	const UMissionInteractObjective* Drum = InteractOn(*Asset, DeckStep, FName(TEXT("Barge_HoseDrum")));
	TestTrue(TEXT("...the hose drum, optional and held"), Drum && Drum->bOptional && Drum->bHold);
	const UMissionKillObjective* Horns = FindIn<UMissionKillObjective>(*Asset, DeckStep, [](const UMissionKillObjective& Each)
	{
		return Each.Target.ActorTag == FName(TEXT("Barge_Horn"));
	});
	TestTrue(TEXT("...the four horns, optional"), Horns && Horns->bOptional && Horns->Count == 4);
	TestTrue(TEXT("4: the manifest (Paid on Delivery's item, an Interact on its tag)"),
		InteractOn(*Asset, ManifestStep, FName(TEXT("Item_BargeManifest"))) != nullptr);
	const UMissionInteractObjective* Valves = InteractOn(*Asset, FurnaceStep, FName(TEXT("Furnace_DraftValve")));
	TestTrue(TEXT("5: three valves, held"), Valves && Valves->bHold && Valves->Count == 3);
	const UMissionClearObjective* Hall = ClearOf(*Asset, FurnaceStep, FName(TEXT("FurnaceHall")));
	TestTrue(TEXT("...and the Chief Stoker's eight"), Hall && Hall->Count == 8);
	const UMissionInteractObjective* Cage = InteractOn(*Asset, CageStep, FName(TEXT("Furnace_SoulCage")));
	TestTrue(TEXT("6: the soul cage, held"), Cage && Cage->bHold);
	TestTrue(TEXT("...then CageBurst"), SceneIn(*Asset, CageStep, FName(TEXT("CageBurst"))) != nullptr);
	const UMissionReachObjective* Jump = ReachTo(*Asset, EscapeStep, DeckMarkerTag());
	TestTrue(TEXT("7: the jump to her deck, height counted"), Jump && !Jump->Place.bIgnoreHeight);
	TestTrue(TEXT("...then BargeFall"), SceneIn(*Asset, EscapeStep, FName(TEXT("BargeFall"))) != nullptr);

	// Turned in to Holloway (card, then four lines), its purse, XP trimmed for the Pacing band (lanes/MAIN67.md).
	TestTrue(TEXT("Turned in to Holloway"), Asset->NeedsTurnIn() && Asset->TurnIn.SpeakerTag == FName(TEXT("Speaker_Holloway")));
	TestEqual(TEXT("...with four lines"), Asset->TurnIn.Lines.Num(), 4);
	TestEqual(TEXT("3,000 XP"), Asset->Rewards.Experience, 3000);
	TestEqual(TEXT("50 silver"), Asset->Rewards.Silver, 50);
	TestTrue(TEXT("No item of its own (the manifest is Paid on Delivery's)"), Asset->Items.IsEmpty());
	return true;
}

#endif

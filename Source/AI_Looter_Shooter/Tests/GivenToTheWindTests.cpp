#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bosses/AbelKeeper.h"
#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionLastingInteractObjective.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionPlaceObjectives.h"
#include "World/CarriedLanternRules.h"
#include "Misc/PackageName.h"

// Main 7, "Given to the Wind" (the Wardens story, 2026-11-01; MAINS_2_7.md section 11, lane MAIN67): its asset as
// create_mission_assets.py makes it, against Abel's step constants (read on Main7 now) and the carry home's
// (World/CarriedLanternRules.h). Replaces Looter.Story.LanternLeans.* (the old Main 7). The fight's and the relighting's own
// tests: AbelCorruptionTests, GivenToTheWindWorldTests, GivenToTheWindPlacedTests.

namespace
{
	const TCHAR* const WindAssetPackage = TEXT("/Game/Data/Missions/DA_Mission_Main7");

	const UMissionDefinition* WindLoad(FAutomationTestBase& Test)
	{
		if (!FPackageName::DoesPackageExist(WindAssetPackage))
		{
			Test.AddWarning(TEXT("DA_Mission_Main7 isn't made yet: run Tools/Unreal/create_mission_assets.py."));
			return nullptr;
		}
		return LoadObject<UMissionDefinition>(nullptr, TEXT("/Game/Data/Missions/DA_Mission_Main7.DA_Mission_Main7"));
	}

	template <typename T>
	const T* WindFind(const UMissionDefinition& Mission, int32 Step, TFunctionRef<bool(const T&)> Matches)
	{
		if (!Mission.Steps.IsValidIndex(Step))
		{
			return nullptr;
		}
		for (const UMissionObjective* Each : Mission.Steps[Step].Objectives)
		{
			// Exact class: a lasting interact is an interact too, and the step's two are told apart by it.
			if (Each && Each->GetClass() == T::StaticClass())
			{
				if (const T* Typed = Cast<T>(Each); Matches(*Typed))
				{
					return Typed;
				}
			}
		}
		return nullptr;
	}

	const UMissionSceneObjective* WindScene(const UMissionDefinition& Mission, int32 Step, FName Scene)
	{
		return WindFind<UMissionSceneObjective>(Mission, Step, [Scene](const UMissionSceneObjective& Each) { return Each.Scene == Scene; });
	}

	const UMissionReachObjective* WindReach(const UMissionDefinition& Mission, int32 Step, FName Tag)
	{
		return WindFind<UMissionReachObjective>(Mission, Step, [Tag](const UMissionReachObjective& Each)
		{
			return Each.Place.Actor.ActorTag == Tag;
		});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGivenToTheWindMissionTest, "Looter.Story.GivenToTheWind.Mission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGivenToTheWindMissionTest::RunTest(const FString& Parameters)
{
	const UMissionDefinition* Asset = WindLoad(*this);
	if (!Asset)
	{
		return true;
	}
	TestEqual(TEXT("Its id stays Main7"), Asset->Id, FName(TEXT("Main7")));
	TestEqual(TEXT("Given to the Wind"), Asset->Title.ToString(), FString(TEXT("Given to the Wind")));
	TestEqual(TEXT("Abel's mission is Main 7 now"), AAbelKeeper::Mission, FName(TEXT("Main7")));
	TestEqual(TEXT("Six steps"), Asset->Steps.Num(), GivenToTheWind::ReadyStep);
	TestTrue(TEXT("After Main 6"), Asset->Prerequisites.Contains(FName(TEXT("Main6"))));
	TestTrue(TEXT("Holloway's, started on the move (one conversation after Main 6's turn-in)"),
		Asset->IsMainByItself() && Asset->Offer.GiverTag == FName(TEXT("Speaker_Holloway")));

	TestTrue(TEXT("0: the burial boards"), WindReach(*Asset, GivenToTheWind::PointStep, FName(TEXT("Place_GravewindPoint"))) != nullptr);
	const UMissionLastingInteractObjective* Coal = WindFind<UMissionLastingInteractObjective>(*Asset, GivenToTheWind::PointStep,
		[](const UMissionLastingInteractObjective& Each) { return Each.Target.ActorTag == GivenToTheWind::SoulCoalTag(); });
	TestTrue(TEXT("...the twelve lumps of soul coal, optional, lasting"), Coal && Coal->bOptional && Coal->Count == 12 && Coal->bHold);
	const UMissionReachObjective* Bridge = WindReach(*Asset, GivenToTheWind::PointStep, FName(TEXT("Place_KeepersBridge")));
	TestTrue(TEXT("...the keeper's path, optional"), Bridge && Bridge->bOptional);

	const UMissionInteractObjective* Hang = WindFind<UMissionInteractObjective>(*Asset, AAbelKeeper::HangStep,
		[](const UMissionInteractObjective& Each) { return Each.Target.ActorTag == FName(TEXT("LanternPost_Keeper")); });
	TestTrue(TEXT("1 (HangStep): his lantern on the keeper's post"), Hang != nullptr);
	const UMissionKillNamedObjective* Fight = WindFind<UMissionKillNamedObjective>(*Asset, AAbelKeeper::FightStep,
		[](const UMissionKillNamedObjective& Each) { return Each.ActorTag == AAbelKeeper::BossTag; });
	TestTrue(TEXT("2 (FightStep): Abel freed, any kill"), Fight && !Fight->bPlayerKillsOnly);
	const UMissionEventObjective* Six = WindFind<UMissionEventObjective>(*Asset, AAbelKeeper::FightStep,
		[](const UMissionEventObjective& Each) { return Each.Event == GivenToTheWind::SixSavedEvent(); });
	TestTrue(TEXT("...the six saved, optional"), Six && Six->bOptional);
	TestTrue(TEXT("3 (SceneStep): FreeAbel"), WindScene(*Asset, AAbelKeeper::SceneStep, FName(TEXT("FreeAbel"))) != nullptr);
	TestTrue(TEXT("4: carried home to the chapel"), WindReach(*Asset, GivenToTheWind::CarryStep, FName(TEXT("Place_Chapel"))) != nullptr);
	const UMissionInteractObjective* Lid = WindFind<UMissionInteractObjective>(*Asset, GivenToTheWind::ReliquaryStep,
		[](const UMissionInteractObjective& Each) { return Each.Target.ActorTag == GivenToTheWind::ReliquaryLanternTag(); });
	TestTrue(TEXT("5: set on the Reliquary, held"), Lid && Lid->bHold);
	TestTrue(TEXT("...then SaintAdaWakes"), WindScene(*Asset, GivenToTheWind::ReliquaryStep, FName(TEXT("SaintAdaWakes"))) != nullptr);

	// Delia at the chapel door: no lines of its own (her topic says them, the hose drum's flag choosing), the why.
	TestTrue(TEXT("Turned in to Delia at the chapel door"), Asset->NeedsTurnIn() && Asset->TurnIn.SpeakerTag == GivenToTheWind::DeliaChapelTag());
	TestTrue(TEXT("...her topic's words, not the mission's"), Asset->TurnIn.Lines.IsEmpty());
	TestFalse(TEXT("...and the tracker says why"), Asset->TurnIn.Why.IsEmpty());
	TestTrue(TEXT("Heirloom by her hand (ADoorHandoff retired: made sure of at the turn-in)"),
		Asset->Rewards.NamedGun == FName(TEXT("Heirloom")) && Asset->Rewards.bNamedGunByHand && !Asset->Rewards.bGun);
	TestTrue(TEXT("The Gilded Lily opened"), Asset->Rewards.UnlockAreas.Contains(FName(TEXT("GildedLily"))));
	TestEqual(TEXT("3,000 XP"), Asset->Rewards.Experience, 3000);
	TestEqual(TEXT("60 silver"), Asset->Rewards.Silver, 60);
	return true;
}

#endif

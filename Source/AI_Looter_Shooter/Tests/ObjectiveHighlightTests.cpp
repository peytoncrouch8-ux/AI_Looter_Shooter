#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureBase.h"
#include "Loot/WeaponRack.h"
#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Missions/MissionPlayerObjectives.h"
#include "Missions/MissionRunner.h"
#include "Missions/ObjectiveHighlightSubsystem.h"
#include "Missions/ObjectiveHighlightTargets.h"
#include "Session/CampaignRecord.h"
#include "Story/MisterSexton.h"
#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
#include "Tests/MissionTestWorld.h"
#include "Weapons/WeaponBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// The tracked objective's orange highlight (UObjectiveHighlightSubsystem, the user's ask of 2026-10-13): what an objective
// lights (never a fight's creatures), when (within the radius, with a margin as it lets go, eased in and out), on which
// meshes (whole, a spot of what a speaker talks through, or a character's figure look), and its material as the code
// drives it.

using namespace MissionTestWorld;

namespace
{
	const FName TestLeverTag(TEXT("HL_Lever"));
	const FName TestNoteTag(TEXT("HL_Note"));
	const FName TestWolfTag(TEXT("HL_Wolf"));
	const FName TestTalkerTag(TEXT("Speaker_HLTalker"));
	const FName TestGiverTag(TEXT("Speaker_HLGiver"));
	const FName TestCrateTag(TEXT("HL_Crate"));
	const FName TestDoorTag(TEXT("Speaker_HLDoor"));
	const FName TestFigureTag(TEXT("HL_Figure"));

	/** A plain actor shown as a skinned figure (the engine's skeletal cube): what a training dummy or an NPC is to the highlight. */
	AActor* SpawnSkinnedFigure(UWorld* World, const FVector& Where, FName Tag)
	{
		USkeletalMesh* Cube = LoadObject<USkeletalMesh>(nullptr, TEXT("/Engine/EngineMeshes/SkeletalCube.SkeletalCube"));
		AActor* Figure = Cube ? World->SpawnActor<AActor>() : nullptr;
		if (!Figure)
		{
			return nullptr;
		}
		USkeletalMeshComponent* Mesh = NewObject<USkeletalMeshComponent>(Figure, TEXT("Figure"));
		Figure->SetRootComponent(Mesh);
		Mesh->SetSkeletalMeshAsset(Cube);
		Mesh->RegisterComponent();
		Figure->SetActorLocation(Where);
		Figure->Tags.Add(Tag);
		return Figure;
	}

	/** A box of the engine's cube, Size cm, solid to queries (a wall, a crate, a floor). */
	AStaticMeshActor* SpawnHighlightBlock(UWorld* World, const FVector& Center, const FVector& Size, FName Tag = NAME_None)
	{
		UStaticMesh* Box = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		// Sized before it has a mesh, so its collision is made at that size.
		AStaticMeshActor* Block = Box ? World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(FQuat::Identity, Center, Size / 100.0)) : nullptr;
		if (Block)
		{
			UStaticMeshComponent* Mesh = Block->GetStaticMeshComponent();
			Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			Mesh->SetStaticMesh(Box);
			if (!Tag.IsNone())
			{
				Block->Tags.Add(Tag);
			}
		}
		return Block;
	}

	TArray<AActor*> GatherTargets(const UMissionRunner& Runner, FName MissionId)
	{
		TArray<AActor*> Found;
		ObjectiveHighlightTargets::Gather(Runner, MissionId, Found);
		return Found;
	}

	/** The material's scalar as an instance reads it, or -1 without one. */
	float ReadScalar(const UMaterialInstanceDynamic* Look, FName Parameter)
	{
		float Value = -1.f;
		if (Look)
		{
			Look->GetScalarParameterValue(Parameter, Value);
		}
		return Value;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FObjectiveHighlightRulesTest, "Looter.Missions.Highlight.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FObjectiveHighlightRulesTest::RunTest(const FString& Parameters)
{
	using Highlighter = UObjectiveHighlightSubsystem;
	TestTrue(TEXT("A close radius, 12 to 15 m"), Highlighter::DefaultRadius >= 1200.f && Highlighter::DefaultRadius <= 1500.f);

	// In range: takes hold within the radius, lets go only past its margin.
	const double Radius = 1300.0;
	TestTrue(TEXT("Within the radius it takes hold"), Highlighter::IsWithinRange(Radius - 1.0, false, Radius));
	TestFalse(TEXT("...not past it"), Highlighter::IsWithinRange(Radius + 1.0, false, Radius));
	TestTrue(TEXT("Lit, it holds a little past the radius"), Highlighter::IsWithinRange(Radius + Highlighter::RangeMargin - 1.0, true, Radius));
	TestFalse(TEXT("...and lets go past the margin"), Highlighter::IsWithinRange(Radius + Highlighter::RangeMargin + 1.0, true, Radius));

	// Eased in and out.
	float Strength = Highlighter::StepStrength(0.f, true, Highlighter::FadeInSeconds * 0.5f);
	TestEqual(TEXT("Half in at half the fade-in"), Strength, 0.5f, 0.01f);
	Strength = Highlighter::StepStrength(Strength, true, 10.f);
	TestEqual(TEXT("Full, and no further"), Strength, 1.f);
	Strength = Highlighter::StepStrength(Strength, false, Highlighter::FadeOutSeconds * 0.5f);
	TestEqual(TEXT("Half out at half the fade-out"), Strength, 0.5f, 0.01f);
	Strength = Highlighter::StepStrength(Strength, false, 10.f);
	TestEqual(TEXT("Out, and no further"), Strength, 0.f);
	TestTrue(TEXT("Shown eased: still at both ends, symmetric in the middle"), Highlighter::ShownStrength(0.f) == 0.f
		&& Highlighter::ShownStrength(1.f) == 1.f && FMath::IsNearlyEqual(Highlighter::ShownStrength(0.5f), 0.5f)
		&& Highlighter::ShownStrength(0.1f) < 0.1f && Highlighter::ShownStrength(0.9f) > 0.9f);

	// What each kind of objective lights.
	using ObjectiveHighlightTargets::FilterFor;
	UMissionInteractObjective* UseLever = NewObject<UMissionInteractObjective>();
	UseLever->Target.ActorTag = TestLeverTag;
	TestEqual(TEXT("Interact: the thing to use"), FilterFor(*UseLever).ActorTag, TestLeverTag);

	UMissionTalkObjective* TalkTo = NewObject<UMissionTalkObjective>();
	TalkTo->SpeakerTag = TestTalkerTag;
	TestEqual(TEXT("Talk: the speaker"), FilterFor(*TalkTo).ActorTag, TestTalkerTag);

	UMissionEventObjective* ShootSacs = NewObject<UMissionEventObjective>();
	ShootSacs->Event = TEXT("EggSac.Burst");
	ShootSacs->Waypoint = EMissionWaypoint::Actor;
	ShootSacs->WaypointActor.ActorTag = TEXT("EggSac");
	TestEqual(TEXT("An event with its arrow on a thing: that thing (the egg sacs)"), FilterFor(*ShootSacs).ActorTag, FName(TEXT("EggSac")));
	UMissionEventObjective* EventAnywhere = NewObject<UMissionEventObjective>();
	EventAnywhere->Event = TEXT("Test.Anywhere");
	TestFalse(TEXT("An event with no thing: nothing"), FilterFor(*EventAnywhere).IsSet());

	UMissionReachObjective* ReachDoor = NewObject<UMissionReachObjective>();
	ReachDoor->Place.Actor.ActorTag = TEXT("Speaker_Delia");
	TestEqual(TEXT("Reach a place's actor: that actor"), FilterFor(*ReachDoor).ActorTag, FName(TEXT("Speaker_Delia")));
	UMissionReachObjective* ReachSpot = NewObject<UMissionReachObjective>();
	ReachSpot->Place.Location = FVector(100.0, 0.0, 0.0);
	TestFalse(TEXT("Reach a bare spot: nothing"), FilterFor(*ReachSpot).IsSet());

	UMissionCollectObjective* TakeGun = NewObject<UMissionCollectObjective>();
	TakeGun->What = EMissionCollect::Weapons;
	TakeGun->Waypoint = EMissionWaypoint::Actor;
	TakeGun->WaypointActor.ActorClass = AWeaponRack::StaticClass();
	TestTrue(TEXT("Take a gun: the rack the arrow names (its gun is lit)"), FilterFor(*TakeGun).ActorClass == AWeaponRack::StaticClass());

	UMissionKillObjective* KillWolf = NewObject<UMissionKillObjective>();
	KillWolf->Target.ActorTag = TestWolfTag;
	KillWolf->Waypoint = EMissionWaypoint::Actor;
	KillWolf->WaypointActor.ActorTag = TestWolfTag;
	TestFalse(TEXT("A kill lights nothing, its arrow's actor neither"), FilterFor(*KillWolf).IsSet());
	UMissionKillNamedObjective* KillBoss = NewObject<UMissionKillNamedObjective>();
	KillBoss->ActorTag = TEXT("Boss_Test");
	TestFalse(TEXT("A named kill lights nothing"), FilterFor(*KillBoss).IsSet());
	UMissionClearObjective* ClearNest = NewObject<UMissionClearObjective>();
	ClearNest->SpawnerId = TEXT("TestNest");
	TestFalse(TEXT("An encounter lights nothing"), FilterFor(*ClearNest).IsSet());
	UMissionDefendObjective* HoldPlace = NewObject<UMissionDefendObjective>();
	HoldPlace->Place.Actor.ActorTag = TEXT("Place_Test");
	TestFalse(TEXT("A place to hold lights nothing"), FilterFor(*HoldPlace).IsSet());
	TestFalse(TEXT("A page of the inventory lights nothing"), FilterFor(*NewObject<UMissionOpenPageObjective>()).IsSet());

	// Never a creature, whatever names it.
	TestFalse(TEXT("A creature is never lit"), ObjectiveHighlightTargets::CanLight(*GetDefault<ACreatureBase>()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FObjectiveHighlightTargetsTest, "Looter.Missions.Highlight.Targets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FObjectiveHighlightTargetsTest::RunTest(const FString& Parameters)
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
	AActor* Lever = SpawnMarker(World, FVector(500.0, 0.0, 0.0), TestLeverTag);
	AActor* Note = SpawnMarker(World, FVector(0.0, 500.0, 0.0), TestNoteTag);
	AActor* Wolf = SpawnMarker(World, FVector(-500.0, 0.0, 0.0), TestWolfTag);
	AActor* Talker = SpawnMarker(World, FVector(0.0, -500.0, 0.0), TestTalkerTag);
	AActor* Giver = SpawnMarker(World, FVector(900.0, 0.0, 0.0), TestGiverTag);
	if (!Runner || !Player || !Lever || !Note || !Wolf || !Talker || !Giver)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	// Step 1: pull the lever, read the note (optional, its arrow on it), kill the wolf. Step 2: talk. Then turn it in.
	const FName Id(TEXT("TestHighlight"));
	UMissionDefinition* Mission = NewMission(CreatePackage(nullptr), TEXT("TestHighlight"), EMissionKind::Side, EMissionStart::Automatic, NAME_None);
	AddObjective<UMissionInteractObjective>(Mission, 0)->Target.ActorTag = TestLeverTag;
	UMissionEventObjective* ReadNote = AddObjective<UMissionEventObjective>(Mission, 0);
	ReadNote->Event = TEXT("Test.Note");
	ReadNote->bOptional = true;
	ReadNote->Waypoint = EMissionWaypoint::Actor;
	ReadNote->WaypointActor.ActorTag = TestNoteTag;
	AddObjective<UMissionKillObjective>(Mission, 0)->Target.ActorTag = TestWolfTag;
	AddObjective<UMissionTalkObjective>(Mission, 1)->SpeakerTag = TestTalkerTag;
	Mission->TurnIn.SpeakerTag = TestGiverTag;
	Runner->BeginForTesting({ Mission }, Campaign, Player);
	Runner->Update(0.f);
	TestEqual(TEXT("Running and tracked"), Runner->GetTrackedMission(), Id);
	TestEqual(TEXT("Its three objectives are live"), ObjectiveHighlightTargets::LiveObjectives(*Runner, Id).Num(), 3);

	TArray<AActor*> Found = GatherTargets(*Runner, Id);
	TestTrue(TEXT("The lever and the note (an optional objective's) are lit"), Found.Contains(Lever) && Found.Contains(Note));
	TestFalse(TEXT("The wolf to kill is not"), Found.Contains(Wolf));
	TestFalse(TEXT("Nor the next step's speaker, nor the giver yet"), Found.Contains(Talker) || Found.Contains(Giver));

	Runner->NotifyEvent(FMissionEvent::Interaction(Lever, /*bHeld*/ false));
	Found = GatherTargets(*Runner, Id);
	TestTrue(TEXT("The lever pulled goes dark; the note stays"), !Found.Contains(Lever) && Found.Contains(Note));
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.Note")));
	TestEqual(TEXT("The note read: only the fight is left, and nothing is lit"), GatherTargets(*Runner, Id).Num(), 0);

	TestTrue(TEXT("The step finished"), Runner->CompleteStep(Id));
	Found = GatherTargets(*Runner, Id);
	TestTrue(TEXT("The next step: its speaker alone"), Found.Num() == 1 && Found.Contains(Talker));

	Runner->NotifyEvent(FMissionEvent::Talked(Talker));
	TestTrue(TEXT("Waiting for its turn-in"), Runner->IsReadyToTurnIn(Id));
	Found = GatherTargets(*Runner, Id);
	TestTrue(TEXT("Its giver is lit"), Found.Num() == 1 && Found.Contains(Giver));

	Giver->SetActorHiddenInGame(true);
	TestEqual(TEXT("Hidden, never"), GatherTargets(*Runner, Id).Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FObjectiveHighlightRangeTest, "Looter.Missions.Highlight.Range",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FObjectiveHighlightRangeTest::RunTest(const FString& Parameters)
{
	using Highlighter = UObjectiveHighlightSubsystem;
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	Highlighter* Highlights = World->GetSubsystem<Highlighter>();
	const double Radius = Highlighter::GetRadius();
	AStaticMeshActor* Crate = SpawnHighlightBlock(World, FVector(Radius + 1500.0, 0.0, 50.0), FVector(50.0), TestCrateTag);
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	if (!Runner || !Highlights || !Crate || !Player)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UStaticMeshComponent* CrateMesh = Crate->GetStaticMeshComponent();
	const FVector Middle = Crate->GetActorLocation();
	const FName Id(TEXT("TestCrate"));
	UMissionDefinition* Mission = NewMission(CreatePackage(nullptr), TEXT("TestCrate"), EMissionKind::Side, EMissionStart::Automatic, NAME_None);
	AddObjective<UMissionInteractObjective>(Mission, 0)->Target.ActorTag = TestCrateTag;
	Runner->BeginForTesting({ Mission }, Campaign, Player);
	Runner->Update(0.f);
	const bool bDrawn = Highlights->HasMaterial();
	if (!bDrawn)
	{
		AddWarning(TEXT("M_ObjectiveHighlight is missing (run Tools/Unreal/build_highlight_material.py): the overlay itself isn't checked."));
	}

	Highlights->Refresh();
	TestTrue(TEXT("The crate is the tracked objective's"), Highlights->GetCandidates().Contains(Crate));
	TestNull(TEXT("Far off: not lit"), Highlights->FindHighlight(Crate));

	// Walking up.
	Player->SetActorLocation(Middle - FVector(Radius - 300.0, 0.0, 0.0));
	Highlights->Advance(Highlighter::RefreshInterval);
	const FObjectiveHighlight* Lit = Highlights->FindHighlight(Crate);
	TestTrue(TEXT("Within the radius: lit"), Lit && Lit->bWanted);
	TestTrue(TEXT("...easing in"), Lit && Lit->Strength > 0.f && Lit->Strength < 1.f);
	TestTrue(TEXT("Its box wears it whole"), Lit && Lit->Meshes.Num() == 1 && Lit->Meshes[0].Component.Get() == CrateMesh
		&& Lit->Meshes[0].Fit == EObjectiveHighlightFit::Whole);
	Highlights->Advance(Highlighter::FadeInSeconds);
	Lit = Highlights->FindHighlight(Crate);
	TestTrue(TEXT("Fully lit after the fade-in"), Lit && Lit->Strength == 1.f);
	if (bDrawn && Lit)
	{
		TestTrue(TEXT("The overlay is on its mesh"), Lit->WholeLook && CrateMesh->GetOverlayMaterial() == Lit->WholeLook);
		TestEqual(TEXT("...at full strength"), ReadScalar(Lit->WholeLook, TEXT("Strength")), 1.f, 0.001f);
		TestEqual(TEXT("...all of it (no spot)"), ReadScalar(Lit->WholeLook, TEXT("MaskRadius")), 0.f);
	}

	// A step back past the radius, inside the margin: still lit.
	Player->SetActorLocation(Middle - FVector(Radius + Highlighter::RangeMargin * 0.5, 0.0, 0.0));
	Highlights->Advance(Highlighter::RefreshInterval);
	Lit = Highlights->FindHighlight(Crate);
	TestTrue(TEXT("Just past the radius it holds"), Lit && Lit->bWanted);

	// Past the margin: it fades out and lets go of the mesh.
	Player->SetActorLocation(Middle - FVector(Radius + Highlighter::RangeMargin + 500.0, 0.0, 0.0));
	Highlights->Advance(Highlighter::RefreshInterval);
	Lit = Highlights->FindHighlight(Crate);
	TestTrue(TEXT("Out of range: fading"), Lit && !Lit->bWanted && Lit->Strength < 1.f);
	Highlights->Advance(Highlighter::FadeOutSeconds);
	TestNull(TEXT("...then gone"), Highlights->FindHighlight(Crate));
	TestNull(TEXT("Its mesh as it was"), CrateMesh->GetOverlayMaterial());

	// Untracked, nothing is lit however near.
	Player->SetActorLocation(Middle - FVector(300.0, 0.0, 0.0));
	Highlights->Advance(Highlighter::RefreshInterval);
	TestNotNull(TEXT("Near again: lit"), Highlights->FindHighlight(Crate));
	Runner->TrackMission(NAME_None);
	Highlights->Advance(Highlighter::RefreshInterval);
	Highlights->Advance(Highlighter::FadeOutSeconds);
	TestNull(TEXT("Untracked: dark"), Highlights->FindHighlight(Crate));

	// Tracked again, then done: its thing goes dark.
	Runner->TrackMission(Id);
	Highlights->Advance(Highlighter::RefreshInterval);
	TestNotNull(TEXT("Tracked again: lit"), Highlights->FindHighlight(Crate));
	Runner->NotifyEvent(FMissionEvent::Interaction(Crate, /*bHeld*/ false));
	TestTrue(TEXT("The objective done finishes the mission"), Campaign.HasCompleted(Id));
	Highlights->Advance(Highlighter::RefreshInterval);
	Highlights->Advance(Highlighter::FadeOutSeconds);
	TestNull(TEXT("Done: dark"), Highlights->FindHighlight(Crate));
	TestNull(TEXT("...its mesh as it was"), CrateMesh->GetOverlayMaterial());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FObjectiveHighlightSpotTest, "Looter.Missions.Highlight.Spot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FObjectiveHighlightSpotTest::RunTest(const FString& Parameters)
{
	using Highlighter = UObjectiveHighlightSubsystem;
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	Highlighter* Highlights = World->GetSubsystem<Highlighter>();
	// A house's wall with a door in it, its face at Y = 0; the door's speaker point just out from it (the point 130 cm up).
	AStaticMeshActor* Wall = SpawnHighlightBlock(World, FVector(0.0, 50.0, 150.0), FVector(600.0, 100.0, 300.0));
	ASpeakerPoint* Door = World->SpawnActor<ASpeakerPoint>(FVector(0.0, -10.0, 20.0), FRotator::ZeroRotator);
	AActor* Player = SpawnMarker(World, FVector(0.0, -500.0, 0.0));
	if (!Runner || !Highlights || !Wall || !Door || !Door->SpeakerPoint || !Player)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Door->Tags.Add(TestDoorTag);
	const FVector Focus = Highlighter::FocusOf(*Door);
	TestTrue(TEXT("A speaker's focus is its point of use"), Focus.Equals(Door->SpeakerPoint->GetComponentLocation(), 1.0));
	TArray<FObjectiveHighlightMesh> Meshes;
	Highlighter::PickMeshes(*Door, Focus, Meshes);
	TestTrue(TEXT("No mesh of its own: the wall it talks through lights, as a spot"), Meshes.Num() == 1
		&& Meshes[0].Component.Get() == Wall->GetStaticMeshComponent() && Meshes[0].Fit == EObjectiveHighlightFit::Spot);

	// A plain marker at the same point (a reach-place's) has no point of use: the wall beside it stays dark.
	AActor* Marker = SpawnMarker(World, Focus);
	Highlighter::PickMeshes(*Marker, Highlighter::FocusOf(*Marker), Meshes);
	TestEqual(TEXT("A marker lights nothing beside it"), Meshes.Num(), 0);

	// The ground never lights, even under a point of use.
	AStaticMeshActor* GroundBlock = SpawnHighlightBlock(World, FVector(3000.0, 0.0, -50.0), FVector(400.0, 400.0, 100.0), TEXT("Ground"));
	ASpeakerPoint* LowPoint = World->SpawnActor<ASpeakerPoint>(FVector(3000.0, 0.0, -100.0), FRotator::ZeroRotator);
	if (GroundBlock && LowPoint)
	{
		Highlighter::PickMeshes(*LowPoint, Highlighter::FocusOf(*LowPoint), Meshes);
		TestEqual(TEXT("The ground under a point of use stays dark"), Meshes.Num(), 0);
	}

	// Through the highlight: a talk at the door lights a spot of the wall round its point.
	const FName Id(TEXT("TestDoor"));
	UMissionDefinition* Mission = NewMission(CreatePackage(nullptr), TEXT("TestDoor"), EMissionKind::Side, EMissionStart::Automatic, NAME_None);
	AddObjective<UMissionTalkObjective>(Mission, 0)->SpeakerTag = TestDoorTag;
	Runner->BeginForTesting({ Mission }, Campaign, Player);
	Runner->Update(0.f);
	Highlights->Refresh();
	Highlights->Advance(Highlighter::FadeInSeconds);
	const FObjectiveHighlight* Lit = Highlights->FindHighlight(Door);
	TestTrue(TEXT("The door is lit, as a spot of the wall"), Lit && Lit->Meshes.Num() == 1 && Lit->Meshes[0].Fit == EObjectiveHighlightFit::Spot);
	if (Lit && Highlights->HasMaterial())
	{
		UStaticMeshComponent* WallMesh = Wall->GetStaticMeshComponent();
		TestTrue(TEXT("The wall wears the spot's overlay"), Lit->SpotLook && WallMesh->GetOverlayMaterial() == Lit->SpotLook);
		TestEqual(TEXT("...a spot of the door's size"), ReadScalar(Lit->SpotLook, TEXT("MaskRadius")), Highlighter::SpotRadius);
		FLinearColor SpotCenter = FLinearColor::Black;
		TestTrue(TEXT("...round the door's point"), Lit->SpotLook && Lit->SpotLook->GetVectorParameterValue(FName(TEXT("MaskCenter")), SpotCenter)
			&& FVector(SpotCenter.R, SpotCenter.G, SpotCenter.B).Equals(Focus, 1.0));
	}
	Highlights->ClearAll();
	TestNull(TEXT("Cleared: the wall as it was"), Wall->GetStaticMeshComponent()->GetOverlayMaterial());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FObjectiveHighlightFigureTest, "Looter.Missions.Highlight.Figure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FObjectiveHighlightFigureTest::RunTest(const FString& Parameters)
{
	// A character wears the rim alone, its own look showing through (Sexton read as a solid orange figure, 2026-10-14);
	// things keep their fill.
	using Highlighter = UObjectiveHighlightSubsystem;
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	Highlighter* Highlights = World->GetSubsystem<Highlighter>();
	AActor* Figure = SpawnSkinnedFigure(World, FVector(400.0, 0.0, 100.0), TestFigureTag);
	AStaticMeshActor* Crate = SpawnHighlightBlock(World, FVector(0.0, 400.0, 50.0), FVector(50.0), TestCrateTag);
	AMisterSexton* Sexton = World->SpawnActor<AMisterSexton>(FVector(-400.0, 0.0, 0.0), FRotator::ZeroRotator);
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	if (!Runner || !Highlights || !Figure || !Crate || !Sexton || !Player)
	{
		AddError(TEXT("The test level isn't whole (the skinned figure needs /Engine/EngineMeshes/SkeletalCube)."));
		return false;
	}

	// Who is a character.
	TestTrue(TEXT("A skinned figure is a character"), Highlighter::IsFigure(*Figure));
	TestTrue(TEXT("Sexton, a posed static model with no rig, is a character"), Highlighter::IsFigure(*Sexton));
	TestFalse(TEXT("A crate is not"), Highlighter::IsFigure(*Crate));
	TestFalse(TEXT("Nor a gun, skinned or not"), Highlighter::IsFigure(*GetDefault<AWeaponBase>()));
	TArray<FObjectiveHighlightMesh> Meshes;
	Highlighter::PickMeshes(*Sexton, Highlighter::FocusOf(*Sexton), Meshes);
	const UStaticMeshComponent* SextonBody = Sexton->Body.Get();
	TestTrue(TEXT("Sexton's body wears the figure look"), Meshes.ContainsByPredicate([SextonBody](const FObjectiveHighlightMesh& Each)
		{ return Each.Component.Get() == SextonBody && Each.Fit == EObjectiveHighlightFit::Figure; }));
	TestFalse(TEXT("...and nothing of his a thing's look"), Meshes.ContainsByPredicate([](const FObjectiveHighlightMesh& Each)
		{ return Each.Fit == EObjectiveHighlightFit::Whole; }));

	// Through the highlight: the figure and a crate beside it, both the objective's.
	UMissionDefinition* Mission = NewMission(CreatePackage(nullptr), TEXT("TestFigure"), EMissionKind::Side, EMissionStart::Automatic, NAME_None);
	AddObjective<UMissionInteractObjective>(Mission, 0)->Target.ActorTag = TestFigureTag;
	AddObjective<UMissionInteractObjective>(Mission, 0)->Target.ActorTag = TestCrateTag;
	Runner->BeginForTesting({ Mission }, Campaign, Player);
	Runner->Update(0.f);
	Highlights->Refresh();
	Highlights->Advance(Highlighter::FadeInSeconds);
	const FObjectiveHighlight* LitFigure = Highlights->FindHighlight(Figure);
	const FObjectiveHighlight* LitCrate = Highlights->FindHighlight(Crate);
	TestTrue(TEXT("The skinned figure is lit, as a figure"), LitFigure && LitFigure->Meshes.Num() == 1
		&& LitFigure->Meshes[0].Fit == EObjectiveHighlightFit::Figure);
	TestTrue(TEXT("The crate beside it, whole"), LitCrate && LitCrate->Meshes.Num() == 1 && LitCrate->Meshes[0].Fit == EObjectiveHighlightFit::Whole);
	UMeshComponent* FigureMesh = LitFigure && LitFigure->Meshes.Num() == 1 ? LitFigure->Meshes[0].Component.Get() : nullptr;
	if (FigureMesh && LitCrate && Highlights->HasMaterial())
	{
		TestTrue(TEXT("The figure wears the figure look"), LitFigure->FigureLook && FigureMesh->GetOverlayMaterial() == LitFigure->FigureLook);
		TestEqual(TEXT("...the character's look on"), ReadScalar(LitFigure->FigureLook, TEXT("Figure")), 1.f);
		TestEqual(TEXT("...on all of it (no spot)"), ReadScalar(LitFigure->FigureLook, TEXT("MaskRadius")), 0.f);
		TestEqual(TEXT("...at full strength"), ReadScalar(LitFigure->FigureLook, TEXT("Strength")), 1.f, 0.001f);
		TestEqual(TEXT("The crate keeps a thing's look"), ReadScalar(LitCrate->WholeLook, TEXT("Figure")), 0.f);
	}
	Highlights->ClearAll();
	TestTrue(TEXT("Cleared: the figure as it was"), FigureMesh && FigureMesh->GetOverlayMaterial() == nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FObjectiveHighlightMaterialTest, "Looter.Missions.Highlight.Material",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FObjectiveHighlightMaterialTest::RunTest(const FString& Parameters)
{
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, UObjectiveHighlightSubsystem::MaterialPath);
	if (!TestNotNull(TEXT("M_ObjectiveHighlight is made (Tools/Unreal/build_highlight_material.py)"), Base))
	{
		return false;
	}
	TestTrue(TEXT("Translucent, as an overlay must be"), Base->GetBlendMode() == BLEND_Translucent);
	const UMaterial* Master = Base->GetMaterial();
	TestTrue(TEXT("Drawn on the characters' skinned figures too"), Master && Master->bUsedWithSkeletalMesh);
	UMaterialInstanceDynamic* Look = UMaterialInstanceDynamic::Create(Base, GetTransientPackage());
	float ScalarValue = 0.f;
	FLinearColor ColorValue = FLinearColor::Black;
	TestTrue(TEXT("It has the Strength the code eases"), Look->GetScalarParameterValue(FName(TEXT("Strength")), ScalarValue));
	TestTrue(TEXT("...and a spot's MaskRadius, 0 by default (all of a mesh)"),
		Look->GetScalarParameterValue(FName(TEXT("MaskRadius")), ScalarValue) && ScalarValue == 0.f);
	TestTrue(TEXT("...and its Color"), Look->GetVectorParameterValue(FName(TEXT("Color")), ColorValue));
	TestTrue(TEXT("...orange by default"), ColorValue.R > ColorValue.G && ColorValue.G > ColorValue.B);
	TestTrue(TEXT("...and a spot's MaskCenter"), Look->GetVectorParameterValue(FName(TEXT("MaskCenter")), ColorValue));
	TestTrue(TEXT("...and the character's look, off by default"),
		Look->GetScalarParameterValue(FName(TEXT("Figure")), ScalarValue) && ScalarValue == 0.f);
	float Fill = 0.f;
	float FigureFill = 1.f;
	float RimPower = 0.f;
	float FigureRimPower = 0.f;
	const bool bTuned = Look->GetScalarParameterValue(FName(TEXT("FillOpacity")), Fill)
		&& Look->GetScalarParameterValue(FName(TEXT("FigureFillOpacity")), FigureFill)
		&& Look->GetScalarParameterValue(FName(TEXT("RimPower")), RimPower)
		&& Look->GetScalarParameterValue(FName(TEXT("FigureRimPower")), FigureRimPower);
	TestTrue(TEXT("A character's body nearly clear (a thing's fill is not), its rim tighter"), bTuned && FigureFill <= 0.05f
		&& FigureFill < Fill && FigureRimPower > RimPower);
	return true;
}

#endif

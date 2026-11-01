#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/SpiderCreature.h"
#include "Embers/HeroDefinition.h"
#include "Interaction/InteractionComponent.h"
#include "Player/Animation/BodyFigureComponent.h"
#include "Story/DeliaRansom.h"
#include "Story/DeliaRansomRules.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/WardenCrewAnimInstance.h"
#include "Story/WardenCrewLife.h"
#include "Story/WardenCrewMember.h"
#include "Story/WardenCrewPose.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "Tests/PlayerAnimTestKit.h"
#include "Combat/CombatRules.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// The Lamplighter's crew on deck (AWardenCrewMember) and Delia Ransom on her porch (ADeliaRansom): who they are, how they
// look about and brace, the pose layer on the mannequin (WardenCrewPose), and Delia's fight: only with creatures near,
// strong but leaving the finish to the player, never dying, and talking face to face once the porch is clear.

namespace
{
	constexpr float CrewStep = 1.f / 30.f;

	void StepLife(FWardenLife& Life, const FWardenLookSense& Sense, float Seconds)
	{
		for (float Time = 0.f; Time < Seconds; Time += CrewStep)
		{
			Life.UpdateLook(CrewStep, Sense);
			Life.UpdateBrace(CrewStep);
		}
	}

	void StepCrew(AWardenCrewMember& Crew, float Seconds)
	{
		for (float Time = 0.f; Time < Seconds; Time += CrewStep)
		{
			Crew.UpdatePose(CrewStep);
		}
	}

	void StepDelia(ADeliaRansom& Delia, float Seconds)
	{
		for (float Time = 0.f; Time < Seconds; Time += CrewStep)
		{
			Delia.Step(CrewStep);
		}
	}

	FStoryLine TestLine()
	{
		return FStoryLine::Make(FText(), FText::FromString(TEXT("Mind the rail.")), 3.f);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWardenCrewRolesTest, "Looter.Story.WardenCrew.Roles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWardenCrewRolesTest::RunTest(const FString& Parameters)
{
	// Each of the three: their speaker name and tag (the missions' and STORY's), a stand-in from the roster until their own
	// model's path has one, the mannequin animated by the crew's own anim instance, a hull shots pass through, Junie's wrench.
	struct FExpected
	{
		EWardenCrew Crew;
		const TCHAR* Name;
		const TCHAR* Tag;
		const TCHAR* Model;
	};
	const FExpected Expected[] = { { EWardenCrew::Captain, TEXT("Captain Holloway"), TEXT("Speaker_Holloway"), TEXT("SK_Holloway") },
		{ EWardenCrew::Keeper, TEXT("Brother Finch"), TEXT("Speaker_Finch"), TEXT("SK_Finch") },
		{ EWardenCrew::Engineer, TEXT("Junie Calloway"), TEXT("Speaker_Junie"), TEXT("SK_Junie") } };
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	for (const FExpected& Each : Expected)
	{
		TestEqual(TEXT("Speaker name"), AWardenCrewMember::NameFor(Each.Crew).ToString(), FString(Each.Name));
		TestEqual(TEXT("Speaker tag"), AWardenCrewMember::SpeakerTagFor(Each.Crew), FName(Each.Tag));
		const FWardenLook Look = AWardenCrewMember::LookFor(Each.Crew);
		TestTrue(*FString::Printf(TEXT("%s's own model's path: %s"), Each.Name, *Look.Model.ToString()), Look.Model.ToString().Contains(Each.Model));
		TestTrue(TEXT("...a roster hero standing in until then, never the player's own Ellis"),
			UHeroDefinition::RosterIds().Contains(Look.StandInHero) && Look.StandInHero != UHeroDefinition::DefaultId());

		AWardenCrewMember* Crew = World->SpawnActor<AWardenCrewMember>(FVector(0.0, 300.0 * static_cast<int32>(Each.Crew), 0.0), FRotator::ZeroRotator);
		if (!TestNotNull(TEXT("Crew member placed"), Crew))
		{
			return false;
		}
		Crew->SetCrew(Each.Crew);
		Crew->DispatchBeginPlay();
		TestEqual(TEXT("Their speaker point speaks as them"), Crew->SpeakerPoint->SpeakerName.ToString(), FString(Each.Name));
		TestTrue(TEXT("Tagged for the missions, once"), Crew->Tags.Contains(FName(Each.Tag)) && Crew->Tags.Num() == 1);
		TestTrue(TEXT("Their body is the crew's animated mannequin"),
			Crew->Mannequin->AnimClass.Get() == UWardenCrewAnimInstance::StaticClass() || Crew->HasOwnModel());
		TestTrue(TEXT("Shots and the camera pass through their hull; the player doesn't"),
			Crew->Body->GetCollisionResponseToChannel(ECC_GameTraceChannel2) == ECR_Ignore
			&& Crew->Body->GetCollisionResponseToChannel(ECC_Camera) == ECR_Ignore && Crew->Body->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);
		TestEqual(TEXT("A wrench in Junie's hand only"), Crew->Wrench->IsVisible(), Each.Crew == EWardenCrew::Engineer);
		if (!Crew->HasOwnModel() && !(Crew->Figure && Crew->Figure->IsWorn()))
		{
			AddWarning(FString::Printf(TEXT("%s: no stand-in body worn (the mannequin or %s's body isn't in this checkout)."), Each.Name,
				*Look.StandInHero.ToString()));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWardenCrewLooksTest, "Looter.Story.WardenCrew.Looks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWardenCrewLooksTest::RunTest(const FString& Parameters)
{
	// Where they look: the player coming near in front, after a beat, a few seconds at a time; never one behind them or
	// far off; the one they talk to over everything; glances about with nobody near.
	const FVector Eye(0.0, 0.0, 150.0);
	TestTrue(TEXT("Near and in front: noticed"), FWardenLife::Notices(Eye, FVector::ForwardVector, FVector(300.0, 100.0, 150.0), 450.f));
	TestFalse(TEXT("Behind: not"), FWardenLife::Notices(Eye, FVector::ForwardVector, FVector(-300.0, 20.0, 150.0), 450.f));
	TestFalse(TEXT("Far: not"), FWardenLife::Notices(Eye, FVector::ForwardVector, FVector(900.0, 0.0, 150.0), 450.f));

	FWardenLookSense Sense;
	Sense.Eye = Eye;
	Sense.bHasPlayer = true;
	Sense.Player = FVector(250.0, 250.0, 160.0);
	Sense.NoticeRadius = 450.f;
	FWardenLife Life(7);
	StepLife(Life, Sense, 0.2f);
	TestFalse(TEXT("A beat before the head comes round"), Life.IsWatchingPlayer());
	StepLife(Life, Sense, 1.f);
	TestTrue(TEXT("Then they watch the player"), Life.IsWatchingPlayer());
	const FVector ToPlayer = (Sense.Player - Eye).GetSafeNormal();
	TestTrue(TEXT("...looking right at them"), ((Life.GetLookAt() - Eye).GetSafeNormal() | ToPlayer) > 0.95 && Life.GetLookWeight() > 0.5f);
	bool bLookedAway = false;
	for (float Time = 0.f; Time < FWardenLife::WatchMax + 0.5f; Time += CrewStep)
	{
		Life.UpdateLook(CrewStep, Sense);
		bLookedAway |= !Life.IsWatchingPlayer();
	}
	TestTrue(TEXT("Nobody stares: within a watch they look away"), bLookedAway);

	FWardenLookSense Talking = Sense;
	Talking.bHasListener = true;
	Talking.Listener = FVector(100.0, -250.0, 160.0);
	StepLife(Life, Talking, 1.5f);
	TestTrue(TEXT("The one they talk to has their eyes"),
		((Life.GetLookAt() - Eye).GetSafeNormal() | (Talking.Listener - Eye).GetSafeNormal()) > 0.97 && Life.GetLookWeight() > 0.8f);

	FWardenLookSense Alone;
	Alone.Eye = Eye;
	FWardenLife Quiet(11);
	float MostOff = 0.f;
	for (float Time = 0.f; Time < 3.f * FWardenLife::GlanceGapMax; Time += CrewStep)
	{
		Quiet.UpdateLook(CrewStep, Alone);
		const FVector Way = (Quiet.GetLookAt() - Eye).GetSafeNormal();
		MostOff = FMath::Max(MostOff, FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(static_cast<float>(Way | FVector::ForwardVector), -1.f, 1.f))));
	}
	TestTrue(*FString::Printf(TEXT("Alone they glance about (%.0f degrees off at most)"), MostOff), MostOff > 15.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWardenCrewBraceTest, "Looter.Story.WardenCrew.Brace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWardenCrewBraceTest::RunTest(const FString& Parameters)
{
	// The ship lurches: the brace comes on quick, holds, eases off; the sway rings out to nothing.
	FWardenLife Life(3);
	Life.Brace(1.f);
	StepLife(Life, FWardenLookSense(), 0.2f);
	TestTrue(TEXT("Braced at once"), Life.GetBrace() > 0.9f && Life.IsBracing());
	TestTrue(TEXT("...and swaying"), FMath::Abs(Life.GetSwayRoll()) > 0.5f || FMath::Abs(Life.GetSwayPitch()) > 0.5f);
	StepLife(Life, FWardenLookSense(), 2.f);
	TestTrue(TEXT("Off again once the lurch is past"), !Life.IsBracing() && Life.GetBrace() < 0.05f);
	StepLife(Life, FWardenLookSense(), 2.f);
	TestTrue(TEXT("The sway rung out"), FMath::Abs(Life.GetSwayRoll()) < 0.05f && FMath::Abs(Life.GetSwayPitch()) < 0.05f);

	// On a crew member: the knees go and the body leans in (BraceAll reaches every one in the level).
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	AWardenCrewMember* Keeper = TestLevel.GetTestWorld()->SpawnActor<AWardenCrewMember>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Keeper placed"), Keeper))
	{
		return false;
	}
	Keeper->SetCrew(EWardenCrew::Keeper);
	Keeper->DispatchBeginPlay();
	StepCrew(*Keeper, 0.5f);
	const float Standing = Keeper->GetPoseInput().Crouch;
	AWardenCrewMember::BraceAll(Keeper, 1.f);
	StepCrew(*Keeper, 0.3f);
	TestTrue(TEXT("Braced, the knees bend"), Keeper->GetPoseInput().Crouch > Standing + 8.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWardenCrewPoseTest, "Looter.Story.WardenCrew.Pose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWardenCrewPoseTest::RunTest(const FString& Parameters)
{
	// The layer on the mannequin's reference pose: a hand to its world spot, the hips down with the feet kept, the head
	// turned to a look; and the gun's frame shouldered on the aim, carried low and to the left at rest.
	USkeleton* Skeleton = PlayerAnimTestKit::MannequinSkeleton();
	if (!Skeleton)
	{
		AddWarning(TEXT("The mannequin's animations aren't in this checkout: the pose wasn't checked."));
		return true;
	}
	FMemMark Mark(FMemStack::Get());
	PlayerAnimTestKit::FPoseRig Rig(*Skeleton);
	const FTransform Mesh(PlayerAnimTestKit::MeshToWorld());
	FCompactPose Pose = Rig.RefPose();
	const FVector Shoulder = Mesh.TransformPosition(Rig.Where(Pose, TEXT("upperarm_r")));
	const FVector Foot = Mesh.TransformPosition(Rig.Where(Pose, TEXT("foot_l")));
	const FVector Hips = Mesh.TransformPosition(Rig.Where(Pose, TEXT("pelvis")));
	const FVector HeadAt = Mesh.TransformPosition(Rig.Where(Pose, TEXT("head")));
	FCSPose<FCompactPose> Before;
	Before.InitPose(Pose);

	FWardenPoseInput Input;
	Input.Crouch = 10.f;
	Input.Hands[1].Weight = 1.f;
	Input.Hands[1].Wrist = Shoulder + FVector(30.0, 4.0, -30.0);
	Input.Hands[1].Elbow = Shoulder + FVector(0.0, 40.0, -35.0);
	Input.LookAt = HeadAt + FVector(100.0, 100.0, 0.0);
	Input.LookWeight = 1.f;
	WardenCrewPose::Apply(Pose, Input, Mesh);
	TestTrue(TEXT("A sound pose"), PlayerAnimTestKit::FPoseRig::IsSound(Pose));
	const FVector Hand = Mesh.TransformPosition(Rig.Where(Pose, TEXT("hand_r")));
	TestTrue(*FString::Printf(TEXT("The right wrist on its spot (%.1f cm off)"), FVector::Dist(Hand, Input.Hands[1].Wrist)),
		FVector::Dist(Hand, Input.Hands[1].Wrist) < 1.5);
	TestTrue(TEXT("The hips 10 cm down"), FMath::IsNearlyEqual(Mesh.TransformPosition(Rig.Where(Pose, TEXT("pelvis"))).Z, Hips.Z - 10.0, 0.5));
	TestTrue(TEXT("...the feet where they stood"), FVector::Dist(Mesh.TransformPosition(Rig.Where(Pose, TEXT("foot_l"))), Foot) < 1.0);
	FCSPose<FCompactPose> After;
	After.InitPose(Pose);
	FBoneReference HeadBone(TEXT("head"));
	HeadBone.Initialize(Rig.Bones);
	const float HeadTurn = FMath::RadiansToDegrees(static_cast<float>(After.GetComponentSpaceTransform(HeadBone.GetCompactPoseIndex(Rig.Bones)).GetRotation()
		.AngularDistance(Before.GetComponentSpaceTransform(HeadBone.GetCompactPoseIndex(Rig.Bones)).GetRotation())));
	TestTrue(*FString::Printf(TEXT("The head turned to the look, 45 degrees off (%.0f)"), HeadTurn), HeadTurn > 30.f && HeadTurn < 60.f);

	const FVector Forward(0.0, 1.0, 0.0);
	const FVector Right(-1.0, 0.0, 0.0);
	const FVector Up(0.0, 0.0, 1.0);
	FWardenGunHold Gun;
	Gun.Shoulder = 1.f;
	const FVector GunShoulder(0.0, 0.0, 140.0);
	const FVector Aim = GunShoulder + Forward * 800.0 + Right * 200.0;
	const FTransform Up1 = WardenCrewPose::GunFrame(Gun, GunShoulder, Aim, Forward, Right, Up);
	TestTrue(TEXT("Shouldered, the barrels on the aim"), (Up1.GetRotation().GetAxisX() | (Aim - GunShoulder).GetSafeNormal()) > 0.99);
	Gun.Shoulder = 0.f;
	const FVector Carried = WardenCrewPose::GunFrame(Gun, GunShoulder, Aim, Forward, Right, Up).GetRotation().GetAxisX();
	TestTrue(TEXT("At rest, carried low across her, the muzzle down and to her left"), Carried.Z < -0.2 && (Carried | Right) < -0.2 && (Carried | Forward) > 0.3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWardenCrewStationTest, "Looter.Story.WardenCrew.Station",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWardenCrewStationTest::RunTest(const FString& Parameters)
{
	// Hands at their station (scaled with the body, moved with the ship's actor); talked to, they turn to whoever it is,
	// Finch letting go of the lantern, the captain keeping a hand on the wheel and turning only so far.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector(0.0, 300.0, 0.0));
	for (const EWardenCrew Each : { EWardenCrew::Captain, EWardenCrew::Keeper })
	{
		AWardenCrewMember* Crew = World->SpawnActor<AWardenCrewMember>(FVector::ZeroVector, FRotator::ZeroRotator);
		if (!TestNotNull(TEXT("Crew member placed"), Crew) || !Player)
		{
			return false;
		}
		Crew->SetCrew(Each);
		Crew->DispatchBeginPlay();
		StepCrew(*Crew, 0.2f);
		const FWardenStation& Station = AWardenCrewMember::StationFor(Each);
		const float Scale = static_cast<float>(Crew->Mannequin->GetRelativeScale3D().Z);
		const FWardenHand& Left = Crew->GetPoseInput().Hands[0];
		TestTrue(TEXT("Both hands at the station"), Left.Weight > 0.99f && Crew->GetPoseInput().Hands[1].Weight > 0.99f);
		TestTrue(TEXT("...the left where the station has it, at their size"), FVector::Dist(Left.Wrist, Station.Hands[0] * Scale) < 10.0 * Scale);

		Crew->SpeakerPoint->bMissionsOnly = false;
		Crew->SpeakerPoint->Lines = { TestLine() };
		TestTrue(TEXT("Talked to"), Crew->SpeakerPoint->Talk(Player));
		StepCrew(*Crew, 1.2f);
		const FWardenPoseInput& Talking = Crew->GetPoseInput();
		TestTrue(TEXT("They turn to whoever talked to them"), Crew->GetTalkAlpha() > 0.9f && Talking.BodyYaw > 20.f
			&& Talking.BodyYaw <= Station.MaxTalkTurn + 0.5f);
		TestTrue(TEXT("...letting go of the station, but a captain's wheel hand"), Talking.Hands[0].Weight < 0.1f
			&& (Station.bKeepsRightHand ? Talking.Hands[1].Weight > 0.99f : Talking.Hands[1].Weight < 0.1f));
		Crew->Destroy();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeliaRulesTest, "Looter.Story.Delia.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDeliaRulesTest::RunTest(const FString& Parameters)
{
	using namespace DeliaFightRules;
	TestTrue(TEXT("A creature 17 m off her porch is in reach"), InReach(FVector::ZeroVector, FVector(1700.0, 0.0, -150.0)));
	TestFalse(TEXT("...19 m off isn't"), InReach(FVector::ZeroVector, FVector(0.0, 1900.0, 0.0)));

	// A Dredger rifleman's 140: a blast landing as her spread lands it takes about a quarter.
	const float Pellet = PelletDamage(140.f);
	TestTrue(*FString::Printf(TEXT("A blast is about 25%% of a Dredger (%.1f)"), Pellet * Pellets * LandingShare), FMath::IsNearlyEqual(Pellet * Pellets * LandingShare, 35.f, 0.1f));
	TestEqual(TEXT("Whole while the floor's far"), LandingScale(140.f, 140.f, Pellet), 1.f);
	TestEqual(TEXT("Nothing at the floor (a tenth)"), LandingScale(14.f, 140.f, Pellet), 0.f);
	const float Near = LandingScale(17.f, 140.f, Pellet);
	TestTrue(TEXT("Near it, only what leaves the floor"), Near > 0.f && Near * LooterCombat::MaxHitDamage(Pellet) <= 3.f + 0.01f);
	TestTrue(TEXT("A creature at a fifth is worth a shell"), WorthABlast(28.f, 140.f));
	TestFalse(TEXT("...one at the floor's margin isn't: the player's to finish"), WorthABlast(20.f, 140.f));

	// Whom she turns on: the nearest seen one worth a shell; else the nearest seen, held at gunpoint.
	TArray<FDeliaTarget> Targets;
	auto Add = [&Targets](double X, float Health, bool bSeen) {
		FDeliaTarget& Each = Targets.AddDefaulted_GetRef();
		Each.Where = FVector(X, 0.0, 0.0);
		Each.Health = Health;
		Each.MaxHealth = 140.f;
		Each.bSeen = bSeen;
	};
	Add(400.0, 140.f, false);
	Add(600.0, 15.f, true);
	Add(900.0, 100.f, true);
	bool bWorth = false;
	TestEqual(TEXT("The nearest seen one worth a shell"), Choose(FVector::ZeroVector, Targets, bWorth), 2);
	TestTrue(TEXT("...worth it"), bWorth);
	Targets[2].Health = 15.f;
	TestEqual(TEXT("All at the floor: the nearest seen, covered"), Choose(FVector::ZeroVector, Targets, bWorth), 1);
	TestFalse(TEXT("...and not fired on"), bWorth);

	TestTrue(TEXT("Her spread closes with distance, within its bounds"), SpreadFor(300.f) > SpreadFor(1500.f) && SpreadFor(100.f) <= MaxSpread
		&& SpreadFor(5000.f) >= MinSpread);
	TestTrue(TEXT("The player between her and it holds her fire"), PlayerInLine(FVector::ZeroVector, FVector(1000.0, 0.0, 0.0), FVector(500.0, 40.0, 0.0)));
	TestFalse(TEXT("...beside her or past it doesn't"), PlayerInLine(FVector::ZeroVector, FVector(1000.0, 0.0, 0.0), FVector(500.0, 200.0, 0.0))
		|| PlayerInLine(FVector::ZeroVector, FVector(1000.0, 0.0, 0.0), FVector(1300.0, 0.0, 0.0)));
	float Break = 0.f;
	float Pocket = 0.f;
	ReloadPose(0.42f, Break, Pocket);
	TestTrue(TEXT("Mid-reload: broken open, her hand at her pocket"), Break > 0.99f && Pocket > 0.99f);
	ReloadPose(1.f, Break, Pocket);
	TestTrue(TEXT("...shut and back on the gun at its end"), Break < 0.01f && Pocket < 0.01f);
	TestTrue(TEXT("Her barks by STORY's order: \"Off my porch!\" opens, \"Reloading!\" reloads, \"Hose-man coming!\" warns"),
		BarkLines(EDeliaBark::Opening) == TArray<int32>{ 0 } && BarkLines(EDeliaBark::Reload) == TArray<int32>{ 3 } && BarkLines(EDeliaBark::HoseMan) == TArray<int32>{ 4 });
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeliaFightTest, "Looter.Story.Delia.Fight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDeliaFightTest::RunTest(const FString& Parameters)
{
	// Only with creatures near her porch: calm with one 25 m off; at 10 m the gun up, two blasts, a reload; never below the
	// floor; a script's word overrides it either way. Fighting, she can't be talked to.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ADeliaRansom* Delia = World->SpawnActor<ADeliaRansom>(FVector(0.0, 0.0, 68.0), FRotator::ZeroRotator);
	ACreatureBase* Spider = EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), FVector(2500.0, 0.0, 0.0));
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector(-300.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("Delia placed"), Delia) || !TestNotNull(TEXT("A spider"), Spider) || !Player)
	{
		return false;
	}
	Delia->DispatchBeginPlay();
	StepDelia(*Delia, 1.f);
	TestTrue(TEXT("25 m off: calm, the gun low, no shot"), !Delia->IsFighting() && Delia->GetShotsFired() == 0 && Delia->GetShoulder() < 0.01f);

	Spider->SetActorLocation(FVector(1000.0, 0.0, 0.0));
	StepDelia(*Delia, 1.f);
	TestTrue(TEXT("10 m off: she fights it"), Delia->GetFight() != EDeliaFight::Calm && Delia->GetFightTarget() == Spider);
	TestTrue(TEXT("...the gun to her shoulder and a blast"), Delia->GetShotsFired() >= 1);
	UInteractionComponent* User = NewObject<UInteractionComponent>(Player);
	Delia->SpeakerPoint->Lines = { TestLine() };
	TestFalse(TEXT("...and no talking mid-fight"), Delia->GetInteractionOptions(*User).bUsable);
	StepDelia(*Delia, 2.f);
	TestTrue(TEXT("Two shells, then the reload"), Delia->GetShotsFired() == 2 && Delia->GetFight() == EDeliaFight::Reloading);
	StepDelia(*Delia, DeliaFightRules::ReloadSeconds + 0.1f);
	TestTrue(TEXT("...loaded again"), Delia->GetShellsLoaded() == DeliaFightRules::ShellsPerLoad || Delia->GetShotsFired() > 2);

	// Her pellets through the game's bullets: wherever they land, never below the floor.
	UHealthComponent* SpiderHealth = Spider->FindComponentByClass<UHealthComponent>();
	if (UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>(); Bullets && SpiderHealth)
	{
		for (int32 Frame = 0; Frame < 30; ++Frame)
		{
			Bullets->Tick(CrewStep);
		}
		TestTrue(TEXT("Never below a tenth of its health"), SpiderHealth->GetHealth() >= SpiderHealth->GetMaxHealth() * DeliaFightRules::LeaveShare - 0.01f);
		if (SpiderHealth->GetHealth() >= SpiderHealth->GetMaxHealth())
		{
			AddWarning(TEXT("Her pellets didn't land in the test level (no collision there): the floor was checked by the rules only."));
		}
	}

	Delia->SetFighting(false);
	const int32 Fired = Delia->GetShotsFired();
	StepDelia(*Delia, 3.5f);
	TestTrue(TEXT("Told not to fight: she doesn't, whatever's near"), Delia->GetShotsFired() == Fired && Delia->GetShoulder() < 0.05f);
	Spider->SetActorLocation(FVector(4000.0, 0.0, 0.0));
	Delia->SetFighting(true);
	StepDelia(*Delia, 1.f);
	TestTrue(TEXT("Told to: the gun up with nothing near"), Delia->IsFighting() && Delia->GetShoulder() > 0.95f && Delia->GetShotsFired() == Fired);
	Delia->ClearFightingOverride();
	StepDelia(*Delia, DeliaFightRules::StayReadySeconds + DeliaFightRules::ReloadSeconds + 1.f);
	TestFalse(TEXT("Left to herself with nothing near: calm again"), Delia->IsFighting());
	if (!Delia->GetBarksSaid())
	{
		AddWarning(TEXT("No barks said (DA_Lines_DeliaFight isn't in this checkout yet: create_story_lines.py)."));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeliaNeverDiesTest, "Looter.Story.Delia.NeverDies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDeliaNeverDiesTest::RunTest(const FString& Parameters)
{
	// The dead and Dredgers may turn on her (a character with health); she takes it, flinches and stands her porch.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	ADeliaRansom* Delia = TestLevel.GetTestWorld()->SpawnActor<ADeliaRansom>(FVector(0.0, 0.0, 68.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Delia placed"), Delia))
	{
		return false;
	}
	Delia->DispatchBeginPlay();
	TestTrue(TEXT("Something the hunt can take: a character with health"), Delia->Health != nullptr && Delia->CanBeDamaged());
	// The hit straight to what her health listens to: a pawn turns ApplyDamage away in a world with no game mode (APawn::
	// ShouldTakeDamage), as the test world is; in play the game mode is there.
	// A bare test world never finishes initializing its actors, and AActor::ProcessEvent drops delegate calls into an actor's
	// own functions until it does (her flinch is one): the guard lets them through, as BossTestWorld::Hurt does.
	FEditorScriptExecutionGuard ScriptGuard;
	Delia->OnTakeAnyDamage.Broadcast(Delia, 1.0e6f, GetDefault<UDamageType>(), nullptr, nullptr);
	// Her health reports the hit (it may not hear the actor's damage in a bare test world): she flinches at its word.
	Delia->Health->OnDamaged.Broadcast(1.0e6f, false, Delia->GetActorLocation() + FVector(100.0, 0.0, 0.0), nullptr, nullptr);
	TestTrue(TEXT("A killing blow: she's standing, whole"), !Delia->Health->IsDead() && Delia->Health->GetHealth() >= Delia->Health->GetMaxHealth());
	TestTrue(TEXT("...and flinched"), Delia->GetFlinch() > 0.9f);
	const FVector Before = Delia->GetActorLocation();
	Delia->LaunchCharacter(FVector(0.0, 900.0, 400.0), true, true);
	StepDelia(*Delia, 0.5f);
	TestTrue(TEXT("A shove never moves her off her porch"), Delia->GetActorLocation().Equals(Before, 0.1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeliaTalksTest, "Looter.Story.Delia.Talks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDeliaTalksTest::RunTest(const FString& Parameters)
{
	// The turn-in's giver: tagged and named as the missions and STORY have her; talked to with the porch clear, she turns to
	// face whoever it is and looks them in the eye.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ADeliaRansom* Delia = World->SpawnActor<ADeliaRansom>(FVector(0.0, 0.0, 68.0), FRotator::ZeroRotator);
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector(0.0, 300.0, 68.0));
	if (!TestNotNull(TEXT("Delia placed"), Delia) || !Player)
	{
		return false;
	}
	Delia->DispatchBeginPlay();
	TestTrue(TEXT("Tagged Speaker_Delia"), Delia->Tags.Contains(FName(TEXT("Speaker_Delia"))) && ADeliaRansom::SpeakerTag == FName(TEXT("Speaker_Delia")));
	TestEqual(TEXT("Her speaker point is \"Delia Ransom\""), Delia->SpeakerPoint->SpeakerName.ToString(), FString(TEXT("Delia Ransom")));
	Delia->SpeakerPoint->Lines = { TestLine() };
	UInteractionComponent* User = NewObject<UInteractionComponent>(Player);
	TestTrue(TEXT("The porch clear: she can be talked to"), Delia->GetInteractionOptions(*User).bUsable);
	TestTrue(TEXT("Talked to"), Delia->SpeakerPoint->Talk(Player));
	StepDelia(*Delia, 1.f);
	TestTrue(*FString::Printf(TEXT("She turns to face them (%.0f)"), Delia->GetActorRotation().Yaw), FMath::IsNearlyEqual(static_cast<float>(Delia->GetActorRotation().Yaw), 90.f, 3.f));
	TestTrue(TEXT("...the gun low, her eyes on them"), Delia->GetShoulder() < 0.01f && Delia->GetLife().GetLookWeight() > 0.7f);
	return true;
}

#endif

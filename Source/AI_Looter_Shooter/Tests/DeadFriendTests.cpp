#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The friendly dead of the expansion (Hiram Coyle, Thaddeus Bright): their pose tables on the Unpaid's skeleton, and the
// story characters themselves in a test level (a test level never ticks: the tests move their poses on).

#include "Story/DeadFriendPoses.h"
#include "Story/HiramCoyle.h"
#include "Story/HiramPoses.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/ThaddeusBright.h"
#include "Story/ThaddeusPoses.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "ReferenceSkeleton.h"
#include "Tests/AutomationCommon.h"

namespace
{
	const TCHAR* const UnpaidModel = TEXT("/Game/Art/Creatures/SK_Unpaid.SK_Unpaid");

	FVector Where(const FReferenceSkeleton& Skeleton, const TArray<FTransform>& Solved, FName Bone)
	{
		return DeadFriendPoses::BoneLocation(Skeleton, Solved, Bone);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeadFriendPosesTest, "Looter.Story.DeadFriends.Poses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDeadFriendPosesTest::RunTest(const FString& Parameters)
{
	const USkeletalMesh* Model = LoadObject<USkeletalMesh>(nullptr, UnpaidModel);
	if (!TestNotNull(TEXT("SK_Unpaid, the rig they're posed on"), Model))
	{
		return false;
	}
	const FReferenceSkeleton& Skeleton = Model->GetRefSkeleton();
	const TArray<FTransform> Rest = DeadFriendPoses::Solve(Skeleton, FDeadFriendPose());
	const FVector RestPelvis = Where(Skeleton, Rest, DeadFriendPoses::Pelvis);
	const FVector RestHead = Where(Skeleton, Rest, DeadFriendPoses::Head);
	TestTrue(TEXT("An empty pose is the rest pose: the head over the pelvis"), RestHead.Z > RestPelvis.Z + 40.0);

	// Hiram sat on his stool: his pelvis at a stool's height, his head still over it, his shroud running forward over the edge.
	const TArray<FTransform> Sat = DeadFriendPoses::Solve(Skeleton, HiramPoses::Make(EHiramPose::Sit));
	const FVector SatPelvis = Where(Skeleton, Sat, DeadFriendPoses::Pelvis);
	const FVector SatHead = Where(Skeleton, Sat, DeadFriendPoses::Head);
	TestTrue(FString::Printf(TEXT("Sitting, his pelvis drops to a stool's height (%.0f cm)"), SatPelvis.Z),
		RestPelvis.Z - SatPelvis.Z >= 35.0 && SatPelvis.Z >= 40.0);
	TestTrue(TEXT("...sitting up, his head over his pelvis"), SatHead.Z > SatPelvis.Z + 40.0 && FMath::Abs(SatHead.X - SatPelvis.X) < 30.0);
	TestTrue(TEXT("...his shroud's first link run out forward, like his knees"), Where(Skeleton, Sat, DeadFriendPoses::Tail(1)).X > SatPelvis.X + 20.0);
	TestTrue(TEXT("...his hands forward, on his knees"), Where(Skeleton, Sat, DeadFriendPoses::ArmBone(1, 2)).X > SatPelvis.X + 15.0);

	// Leaned to the tube on his right: his head over to the right and turned to it; on his left, the other way.
	const TArray<FTransform> Right = DeadFriendPoses::Solve(Skeleton, HiramPoses::Make(EHiramPose::Tube, 1.f));
	const TArray<FTransform> Left = DeadFriendPoses::Solve(Skeleton, HiramPoses::Make(EHiramPose::Tube, -1.f));
	TestTrue(TEXT("At the tube on his right, his head leans and turns right"), Where(Skeleton, Right, DeadFriendPoses::Head).Y > SatHead.Y + 5.0
		&& DeadFriendPoses::BoneYaw(Skeleton, Right, DeadFriendPoses::Head) > 15.f);
	TestTrue(TEXT("...on his left, left"), Where(Skeleton, Left, DeadFriendPoses::Head).Y < SatHead.Y - 5.0
		&& DeadFriendPoses::BoneYaw(Skeleton, Left, DeadFriendPoses::Head) < -15.f);
	TestTrue(TEXT("...his near hand up by his face"), Where(Skeleton, Right, DeadFriendPoses::ArmBone(1, 2)).Z
		> Where(Skeleton, Sat, DeadFriendPoses::ArmBone(1, 2)).Z + 30.0);

	// Thaddeus at his bench: hands on the board before him at a bench's height; the tape's stretch draws them apart.
	const TArray<FTransform> Together = DeadFriendPoses::Solve(Skeleton, ThaddeusPoses::Make(EThaddeusPose::Measure, 0.f));
	const TArray<FTransform> Apart = DeadFriendPoses::Solve(Skeleton, ThaddeusPoses::Make(EThaddeusPose::Measure, 1.f));
	const FVector HandL = Where(Skeleton, Together, DeadFriendPoses::ArmBone(0, 2));
	const FVector HandR = Where(Skeleton, Together, DeadFriendPoses::ArmBone(1, 2));
	TestTrue(FString::Printf(TEXT("Measuring, his hands at a bench's height (%.0f, %.0f cm)"), HandL.Z, HandR.Z),
		HandL.Z > 70.0 && HandL.Z < 115.0 && HandR.Z > 70.0 && HandR.Z < 115.0);
	TestTrue(TEXT("...before him"), HandL.X > 20.0 && HandR.X > 20.0);
	const double Narrow = FVector::Dist2D(HandL, HandR);
	const double Wide = FVector::Dist2D(Where(Skeleton, Apart, DeadFriendPoses::ArmBone(0, 2)), Where(Skeleton, Apart, DeadFriendPoses::ArmBone(1, 2)));
	TestTrue(FString::Printf(TEXT("...the stretch draws them apart (%.0f to %.0f cm)"), Narrow, Wide), Wide > Narrow + 20.0);
	TestTrue(TEXT("...his head bowed to the board"), Where(Skeleton, Together, DeadFriendPoses::Head).X > Where(Skeleton, Rest, DeadFriendPoses::Head).X + 5.0);
	const TArray<FTransform> Up = DeadFriendPoses::Solve(Skeleton, ThaddeusPoses::Make(EThaddeusPose::Knock, 0.f));
	const TArray<FTransform> Down = DeadFriendPoses::Solve(Skeleton, ThaddeusPoses::Make(EThaddeusPose::Knock, 1.f));
	TestTrue(TEXT("A knock: his fist up, then down on the board"), Where(Skeleton, Up, DeadFriendPoses::ArmBone(1, 2)).Z
		> Where(Skeleton, Down, DeadFriendPoses::ArmBone(1, 2)).Z + 20.0);
	const TArray<FTransform> Look = DeadFriendPoses::Solve(Skeleton, ThaddeusPoses::Make(EThaddeusPose::Look));
	TestTrue(TEXT("Talking, he straightens: his head higher than over his work"), Where(Skeleton, Look, DeadFriendPoses::Head).Z
		> Where(Skeleton, Together, DeadFriendPoses::Head).Z + 3.0);

	// A blend runs between two poses; the head turned and the jaw open on top.
	const FDeadFriendPose Half = FDeadFriendPose::Blend(HiramPoses::Make(EHiramPose::Sit), FDeadFriendPose(), 0.5f);
	const double HalfPelvis = Where(Skeleton, DeadFriendPoses::Solve(Skeleton, Half), DeadFriendPoses::Pelvis).Z;
	TestTrue(TEXT("Half way from sitting to standing: his pelvis half way up"), HalfPelvis > SatPelvis.Z + 10.0 && HalfPelvis < RestPelvis.Z - 10.0);
	FDeadFriendLayer Talking;
	Talking.HeadYaw = 40.f;
	TestTrue(TEXT("Talking, his head turns to the listener"),
		DeadFriendPoses::BoneYaw(Skeleton, DeadFriendPoses::Solve(Skeleton, FDeadFriendPose(), Talking), DeadFriendPoses::Head) > 30.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeadFriendCharactersTest, "Looter.Story.DeadFriends.Characters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDeadFriendCharactersTest::RunTest(const FString& Parameters)
{
	// Who they are, when they're there, and how they're found.
	const AHiramCoyle* HiramDefaults = GetDefault<AHiramCoyle>();
	const AThaddeusBright* ThaddeusDefaults = GetDefault<AThaddeusBright>();
	TestTrue(TEXT("Hiram: Speaker_Hiram, there once Main 5 is done (Dead Lift opens)"), HiramDefaults->Tags.Contains(AHiramCoyle::SpeakerTag)
		&& HiramDefaults->ShownWhen.AfterMissions.Contains(FName(TEXT("Main5"))));
	TestEqual(TEXT("...named"), HiramDefaults->SpeakerPoint->SpeakerName.ToString(), FString(TEXT("Hiram Coyle")));
	TestTrue(TEXT("Thaddeus: Speaker_Thaddeus, there once Main 4 is done (Knocking Below opens)"),
		ThaddeusDefaults->Tags.Contains(AThaddeusBright::SpeakerTag) && ThaddeusDefaults->ShownWhen.AfterMissions.Contains(FName(TEXT("Main4"))));
	TestEqual(TEXT("...named"), ThaddeusDefaults->SpeakerPoint->SpeakerName.ToString(), FString(TEXT("Thaddeus Bright")));

	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	AHiramCoyle* Hiram = World->SpawnActor<AHiramCoyle>(FVector::ZeroVector, FRotator::ZeroRotator);
	AThaddeusBright* Thaddeus = World->SpawnActor<AThaddeusBright>(FVector(1000.0, 0.0, 0.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Hiram"), Hiram) || !TestNotNull(TEXT("Thaddeus"), Thaddeus))
	{
		return false;
	}
	// Shown whatever the story here: a test level has no campaign, and their tick moves their poses.
	Hiram->ShownWhen = FStoryCondition();
	Thaddeus->ShownWhen = FStoryCondition();
	Hiram->DispatchBeginPlay();
	Thaddeus->DispatchBeginPlay();
	TestTrue(TEXT("Both on a body posed by code"), Hiram->HasModel() && Thaddeus->HasModel());
	TestTrue(TEXT("Hiram sits: his head lower than Thaddeus's, who stands"), Hiram->GetHeadLocation().Z < Thaddeus->GetHeadLocation().Z - 20.0);
	TestTrue(TEXT("...his words come from his head"), FVector::Dist(Hiram->SpeakerPoint->GetComponentLocation(), Hiram->GetHeadLocation()) < 40.0);

	// Hiram leans to the speaking tube, and back to his stool after.
	Hiram->LeanToTube(1.f);
	TestTrue(TEXT("Hiram at the tube"), Hiram->IsAtTube());
	for (float Time = 0.f; Time < 1.5f; Time += 0.1f)
	{
		Hiram->UpdatePose(0.1f);
	}
	TestFalse(TEXT("...back on his stool after"), Hiram->IsAtTube());

	// Thaddeus at his work, and his knock.
	TestTrue(TEXT("Thaddeus measures or marks at his bench"), Thaddeus->GetActivity() == EThaddeusPose::Measure
		|| Thaddeus->GetActivity() == EThaddeusPose::Mark);
	Thaddeus->KnockNow();
	TestTrue(TEXT("...knocks on the board"), Thaddeus->GetActivity() == EThaddeusPose::Knock && Thaddeus->GetKnocks() == 1);
	for (float Time = 0.f; Time < ThaddeusPoses::KnockSeconds + 0.3f; Time += 0.1f)
	{
		Thaddeus->UpdatePose(0.1f);
	}
	TestFalse(TEXT("...and goes back to his measuring"), Thaddeus->GetActivity() == EThaddeusPose::Knock);
	return true;
}

#endif

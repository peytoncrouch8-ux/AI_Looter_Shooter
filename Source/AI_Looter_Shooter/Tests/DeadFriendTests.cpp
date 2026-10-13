#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The friendly dead of the expansion (Hiram Coyle, Thaddeus Bright): their pose tables as the code reads them (generated from
// their model scripts' tables by Tools/dead_friend_poses.py, measured without their models), their models posed from them
// once imported, and the story characters themselves in a test level (a test level never ticks: the tests move their poses on).

#include "Story/DeadFriendPoses.h"
#include "Story/HiramCoyle.h"
#include "Story/HiramPoses.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/ThaddeusBright.h"
#include "Story/ThaddeusPoses.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "ReferenceSkeleton.h"
#include "Tests/AutomationCommon.h"

namespace
{
	using DeadFriendPoses::FTableHeads;

	FTableHeads SolveHiram(const FDeadFriendPose& Pose, const FDeadFriendLayer& Layer = FDeadFriendLayer())
	{
		return DeadFriendPoses::SolveTable(HiramPoses::Table(), Pose, Layer);
	}

	FTableHeads SolveThaddeus(const FDeadFriendPose& Pose)
	{
		return DeadFriendPoses::SolveTable(ThaddeusPoses::Table(), Pose);
	}

	FVector HiramAt(const FTableHeads& Solved, const TCHAR* Bone)
	{
		return DeadFriendPoses::TableHead(HiramPoses::Table(), Solved, FName(Bone));
	}

	FVector ThaddeusAt(const FTableHeads& Solved, const TCHAR* Bone)
	{
		return DeadFriendPoses::TableHead(ThaddeusPoses::Table(), Solved, FName(Bone));
	}

	/** Whether an asset's been imported (looked for by path: nothing's loaded while it's missing). */
	bool IsMade(const TCHAR* ObjectPath)
	{
		return FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath)));
	}

	/** A table's shape: 35 bones, parents first; the idle his model's rest; the talk only his jaw, opened 8 degrees. */
	void CheckTable(FAutomationTestBase& Test, const TCHAR* Who, const FDeadFriendPoseTable& Table, int32 Poses, int32 IdleRow, int32 TalkRow)
	{
		Test.TestEqual(*FString::Printf(TEXT("%s table: 35 bones (the Unpaid's and the front skirt chain)"), Who), Table.NumBones, 35);
		Test.TestEqual(*FString::Printf(TEXT("%s table: a row per pose"), Who), Table.NumPoses, Poses);
		bool bParentsFirst = Table.Bones != nullptr;
		for (int32 Bone = 0; Table.Bones && Bone < Table.NumBones; ++Bone)
		{
			bParentsFirst &= Table.Bones[Bone].Parent < Bone;
		}
		Test.TestTrue(*FString::Printf(TEXT("%s table: every bone after its parent"), Who), bParentsFirst);
		for (const TCHAR* Bone : { TEXT("pelvis"), TEXT("spine_02"), TEXT("head"), TEXT("jaw"), TEXT("hat"), TEXT("hand_l"), TEXT("hand_r"),
			TEXT("skirt_f_02") })
		{
			Test.TestTrue(*FString::Printf(TEXT("%s table has %s"), Who, Bone), DeadFriendPoses::FindTableBone(Table, FName(Bone)) != INDEX_NONE);
		}
		const FDeadFriendPose Idle = DeadFriendPoses::FromTable(Table, IdleRow);
		Test.TestTrue(*FString::Printf(TEXT("%s idle is his model's rest: no turn, no shift"), Who), Idle.Turns.Num() == 0 && Idle.Shifts.Num() == 0);
		const FDeadFriendPose Talk = DeadFriendPoses::FromTable(Table, TalkRow);
		const FQuat* Jaw = Talk.Turns.Find(DeadFriendPoses::Jaw);
		Test.TestTrue(*FString::Printf(TEXT("%s talk only opens his jaw, about 8 degrees"), Who), Jaw && Talk.Turns.Num() == 1
			&& Talk.Shifts.Num() == 0 && FMath::IsNearlyEqual(FMath::RadiansToDegrees(Jaw->GetAngle()), 8.0, 0.5));
	}

	/** Once his model's imported: each pose of the table laid on its skeleton puts every bone where the table does. */
	void CheckModel(FAutomationTestBase& Test, const TCHAR* Who, const TCHAR* ModelPath, const FDeadFriendPoseTable& Table,
		TFunctionRef<const TCHAR*(int32)> PoseName)
	{
		if (!IsMade(ModelPath))
		{
			Test.AddInfo(FString::Printf(TEXT("%s model isn't imported yet: he's posed on SK_Unpaid until it is"), Who));
			return;
		}
		const USkeletalMesh* Model = LoadObject<USkeletalMesh>(nullptr, ModelPath);
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s model"), Who), Model))
		{
			return;
		}
		const FReferenceSkeleton& Skeleton = Model->GetRefSkeleton();
		for (int32 Row = 0; Row < Table.NumPoses; ++Row)
		{
			const FDeadFriendPose Pose = DeadFriendPoses::FromTable(Table, Row);
			const FTableHeads Wanted = DeadFriendPoses::SolveTable(Table, Pose);
			const TArray<FTransform> Posed = DeadFriendPoses::Solve(Skeleton, Pose);
			double Worst = 0.0;
			FString WorstBone;
			int32 Missing = 0;
			for (int32 Bone = 0; Bone < Table.NumBones; ++Bone)
			{
				const FName Name(Table.Bones[Bone].Name);
				const int32 Index = Skeleton.FindBoneIndex(Name);
				if (!Posed.IsValidIndex(Index))
				{
					++Missing;
					continue;
				}
				const double Off = FVector::Dist(Posed[Index].GetLocation(), Wanted.Heads[Bone]);
				if (Off > Worst)
				{
					Worst = Off;
					WorstBone = Name.ToString();
				}
			}
			Test.TestEqual(*FString::Printf(TEXT("%s %s: his model has every table bone"), Who, PoseName(Row)), Missing, 0);
			Test.TestTrue(*FString::Printf(TEXT("%s %s: every bone where the table puts it (worst %.2f cm, %s)"), Who, PoseName(Row), Worst,
				*WorstBone), Worst < 2.0);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeadFriendTablesTest, "Looter.Story.DeadFriends.Tables",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDeadFriendTablesTest::RunTest(const FString& Parameters)
{
	CheckTable(*this, TEXT("Hiram's"), HiramPoses::Table(), static_cast<int32>(EHiramPose::Count), static_cast<int32>(EHiramPose::Idle),
		static_cast<int32>(EHiramPose::Talk));
	CheckTable(*this, TEXT("Thaddeus's"), ThaddeusPoses::Table(), static_cast<int32>(EThaddeusPose::Count),
		static_cast<int32>(EThaddeusPose::Idle), static_cast<int32>(EThaddeusPose::Talk));

	// Quoted from the model scripts' tables (Intermediate/<Name>Model/<Name>_poses.json) when the .inl files were written:
	// the code's solve puts these bones where the models' own do.
	struct FQuoted
	{
		bool bHiram;
		int32 Row;
		const TCHAR* Bone;
		FVector Head;
	};
	const FQuoted Quoted[] = {
		{ true, static_cast<int32>(EHiramPose::Tube), TEXT("head"), FVector(26.65, 22.88, 124.09) },
		{ true, static_cast<int32>(EHiramPose::Tube), TEXT("hand_r"), FVector(19.98, 54.0, 127.24) },
		{ true, static_cast<int32>(EHiramPose::TubeLeft), TEXT("hand_l"), FVector(19.98, -54.0, 127.24) },
		{ false, static_cast<int32>(EThaddeusPose::KnockUp), TEXT("hand_r"), FVector(37.37, 35.79, 145.71) },
		{ false, static_cast<int32>(EThaddeusPose::KnockDown), TEXT("hand_r"), FVector(51.69, 29.35, 106.64) },
		{ false, static_cast<int32>(EThaddeusPose::Mark), TEXT("hand_r"), FVector(46.89, 23.0, 108.98) },
		{ false, static_cast<int32>(EThaddeusPose::Look), TEXT("head"), FVector(4.8, 0.0, 161.05) },
	};
	for (const FQuoted& Row : Quoted)
	{
		const FDeadFriendPoseTable& Table = Row.bHiram ? HiramPoses::Table() : ThaddeusPoses::Table();
		const TCHAR* Pose = Row.bHiram ? HiramPoses::Name(static_cast<EHiramPose>(Row.Row)) : ThaddeusPoses::Name(static_cast<EThaddeusPose>(Row.Row));
		const FVector Head = DeadFriendPoses::TableHead(Table, DeadFriendPoses::SolveTable(Table, DeadFriendPoses::FromTable(Table, Row.Row)), FName(Row.Bone));
		TestTrue(*FString::Printf(TEXT("%s %s %s at %s, as the model's table has it (%s)"), Row.bHiram ? TEXT("Hiram") : TEXT("Thaddeus"), Pose,
			Row.Bone, *Head.ToCompactString(), *Row.Head.ToCompactString()), Head.Equals(Row.Head, 0.5));
	}

	// Their models, once imported: posed from the tables bone for bone.
	CheckModel(*this, TEXT("Hiram's"), AHiramCoyle::ModelPath, HiramPoses::Table(),
		[](int32 Row) { return HiramPoses::Name(static_cast<EHiramPose>(Row)); });
	CheckModel(*this, TEXT("Thaddeus's"), AThaddeusBright::ModelPath, ThaddeusPoses::Table(),
		[](int32 Row) { return ThaddeusPoses::Name(static_cast<EThaddeusPose>(Row)); });
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeadFriendPosesTest, "Looter.Story.DeadFriends.Poses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDeadFriendPosesTest::RunTest(const FString& Parameters)
{
	// Hiram on his 56 cm stool: his pelvis 76 cm up over it, sat up, his forearms on his knees.
	const FTableHeads Sat = SolveHiram(HiramPoses::Make(EHiramPose::Idle));
	const FVector Pelvis = HiramAt(Sat, TEXT("pelvis"));
	const FVector SatHead = HiramAt(Sat, TEXT("head"));
	TestTrue(*FString::Printf(TEXT("Hiram sits on his stool: his pelvis 76 cm up over it (%s)"), *Pelvis.ToCompactString()),
		FMath::IsNearlyEqual(Pelvis.Z, 76.0, 1.0) && FMath::Abs(Pelvis.X) < 5.0);
	TestTrue(TEXT("...sat up, his head over him"), SatHead.Z > Pelvis.Z + 40.0);
	const FVector KneeL = HiramAt(Sat, TEXT("hand_l"));
	const FVector KneeR = HiramAt(Sat, TEXT("hand_r"));
	TestTrue(TEXT("...his forearms on his knees: his hands out before him at his seat's height"), KneeL.X > Pelvis.X + 30.0
		&& KneeR.X > Pelvis.X + 30.0 && FMath::Abs(KneeL.Z - Pelvis.Z) < 10.0 && FMath::Abs(KneeR.Z - Pelvis.Z) < 10.0);

	// Leaned to the tube on his right: his head over to it and turned to it, his near hand up to the mouthpiece; on his left,
	// the same mirrored.
	const FTableHeads Right = SolveHiram(HiramPoses::AtTube(1.f));
	const FTableHeads Left = SolveHiram(HiramPoses::AtTube(-1.f));
	const float RightYaw = DeadFriendPoses::TableYaw(HiramPoses::Table(), Right, DeadFriendPoses::Head);
	const float LeftYaw = DeadFriendPoses::TableYaw(HiramPoses::Table(), Left, DeadFriendPoses::Head);
	TestTrue(*FString::Printf(TEXT("At the tube on his right, his head leans over and turns to it (%.0f degrees)"), RightYaw),
		HiramAt(Right, TEXT("head")).Y > SatHead.Y + 10.0 && RightYaw > 15.f);
	TestTrue(*FString::Printf(TEXT("...on his left, the other way (%.0f degrees)"), LeftYaw), HiramAt(Left, TEXT("head")).Y < SatHead.Y - 10.0
		&& LeftYaw < -15.f);
	TestTrue(TEXT("...his near hand up to the mouthpiece"), HiramAt(Right, TEXT("hand_r")).Z > KneeR.Z + 30.0
		&& HiramAt(Left, TEXT("hand_l")).Z > KneeL.Z + 30.0);
	TestTrue(TEXT("...his far forearm left on his knee"), HiramAt(Right, TEXT("hand_l")).Z < Pelvis.Z + 10.0);
	TestTrue(TEXT("...the one lean the other mirrored"), HiramAt(Right, TEXT("head")).Equals(HiramAt(Left, TEXT("head")) * FVector(1.0, -1.0, 1.0), 0.5));
	const double HalfY = HiramAt(SolveHiram(FDeadFriendPose::Blend(HiramPoses::Make(EHiramPose::Idle), HiramPoses::AtTube(1.f), 0.5f)), TEXT("head")).Y;
	TestTrue(TEXT("Half way to the tube: his head half way over"), HalfY > SatHead.Y + 2.0 && HalfY < HiramAt(Right, TEXT("head")).Y - 2.0);

	// Thaddeus at full stretch (his idle): stooped over his work, his hands 98 cm up, 50 cm apart and before him.
	const FTableHeads Stretched = SolveThaddeus(ThaddeusPoses::Make(EThaddeusPose::Idle));
	const FVector HandL = ThaddeusAt(Stretched, TEXT("hand_l"));
	const FVector HandR = ThaddeusAt(Stretched, TEXT("hand_r"));
	const FVector StoopedHead = ThaddeusAt(Stretched, TEXT("head"));
	const double Apart = FVector::Dist2D(HandL, HandR);
	TestTrue(*FString::Printf(TEXT("Thaddeus at full stretch: his hands 98 cm up (%.1f, %.1f)"), HandL.Z, HandR.Z),
		FMath::IsNearlyEqual(HandL.Z, 98.0, 2.0) && FMath::IsNearlyEqual(HandR.Z, 98.0, 2.0));
	TestTrue(*FString::Printf(TEXT("...50 cm apart (%.1f)"), Apart), FMath::IsNearlyEqual(Apart, 50.0, 2.0));
	TestTrue(TEXT("...out before him"), HandL.X > 30.0 && HandR.X > 30.0);
	TestTrue(*FString::Printf(TEXT("Hiram seated has his head lower than Thaddeus stooped at his bench (%.0f, %.0f cm)"), SatHead.Z, StoopedHead.Z),
		SatHead.Z < StoopedHead.Z - 20.0);

	// The measure: from his hands together to the tape at full stretch.
	const auto Spread = [](const FDeadFriendPose& Pose)
	{
		const FTableHeads Solved = SolveThaddeus(Pose);
		return FVector::Dist2D(ThaddeusAt(Solved, TEXT("hand_l")), ThaddeusAt(Solved, TEXT("hand_r")));
	};
	const double Start = Spread(ThaddeusPoses::Measure(0.f));
	const double Half = Spread(ThaddeusPoses::Measure(0.5f));
	const double Full = Spread(ThaddeusPoses::Measure(1.f));
	TestTrue(*FString::Printf(TEXT("The measure draws his hands apart (%.0f, %.0f, %.0f cm)"), Start, Half, Full),
		Start < Apart - 20.0 && Half > Start + 5.0 && Half < Full - 5.0 && FMath::IsNearlyEqual(Full, Apart, 0.5));

	// The mark, the knock and the look.
	const FTableHeads Marked = SolveThaddeus(ThaddeusPoses::Make(EThaddeusPose::Mark));
	TestTrue(TEXT("A mark: his right hand lifted for the pencil's tick, his left still on the tape"),
		ThaddeusAt(Marked, TEXT("hand_r")).Z > HandR.Z + 5.0 && ThaddeusAt(Marked, TEXT("hand_l")).Equals(HandL, 0.5));
	const FTableHeads Up = SolveThaddeus(ThaddeusPoses::Knock(0.f));
	const FTableHeads Down = SolveThaddeus(ThaddeusPoses::Knock(1.f));
	TestTrue(TEXT("A knock: his fist raised, then brought down on the board"), ThaddeusAt(Up, TEXT("hand_r")).Z > ThaddeusAt(Down, TEXT("hand_r")).Z + 20.0);
	TestTrue(TEXT("...his left hand on the tape all the while"), ThaddeusAt(Up, TEXT("hand_l")).Equals(HandL, 0.5)
		&& ThaddeusAt(Down, TEXT("hand_l")).Equals(HandL, 0.5));
	const FVector Looking = ThaddeusAt(SolveThaddeus(ThaddeusPoses::Make(EThaddeusPose::Look)), TEXT("head"));
	TestTrue(TEXT("Talking, he straightens from his work: his head up and back"), Looking.Z > StoopedHead.Z + 2.0 && Looking.X < StoopedHead.X - 5.0);

	// A talk's layer on any pose: his head turned to the listener about his body's up, his jaw opened under it.
	FDeadFriendLayer Talking;
	Talking.HeadYaw = 40.f;
	Talking.JawOpen = 8.f;
	const FTableHeads Turned = SolveHiram(HiramPoses::Make(EHiramPose::Idle), Talking);
	TestTrue(TEXT("Talking, his head turns to the listener"), DeadFriendPoses::TableYaw(HiramPoses::Table(), Turned, DeadFriendPoses::Head) > 30.f);
	const int32 HeadBone = DeadFriendPoses::FindTableBone(HiramPoses::Table(), DeadFriendPoses::Head);
	const int32 JawBone = DeadFriendPoses::FindTableBone(HiramPoses::Table(), DeadFriendPoses::Jaw);
	if (TestTrue(TEXT("The table has his head and jaw"), Turned.Turns.IsValidIndex(HeadBone) && Turned.Turns.IsValidIndex(JawBone)))
	{
		const double JawDegrees = FMath::RadiansToDegrees((Turned.Turns[HeadBone].Inverse() * Turned.Turns[JawBone]).GetAngle());
		TestTrue(*FString::Printf(TEXT("...his jaw opened under it (%.1f degrees)"), JawDegrees), FMath::IsNearlyEqual(JawDegrees, 8.0, 0.5));
	}
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
	TestTrue(TEXT("...his words come from his head"), FVector::Dist(Hiram->SpeakerPoint->GetComponentLocation(), Hiram->GetHeadLocation()) < 40.0);

	// Their own models and things, once imported: Hiram seated lower than Thaddeus stooped; Hiram's cap, Thaddeus's
	// spectacles on their hat bones, Thaddeus's words from the spectacles' socket.
	if (IsMade(AHiramCoyle::ModelPath) && IsMade(AThaddeusBright::ModelPath))
	{
		TestTrue(TEXT("Hiram sits: his head lower than Thaddeus's at his bench"), Hiram->GetHeadLocation().Z < Thaddeus->GetHeadLocation().Z - 20.0);
	}
	if (IsMade(AHiramCoyle::CapPath))
	{
		TestTrue(TEXT("Hiram wears his cap"), Hiram->Hat->GetStaticMesh() && Hiram->Hat->GetStaticMesh()->GetName() == TEXT("SM_HiramCap")
			&& Hiram->Hat->GetAttachSocketName() == DeadFriendPoses::Hat);
	}
	if (IsMade(AThaddeusBright::SpectaclesPath))
	{
		const UStaticMesh* Spectacles = Thaddeus->Hat->GetStaticMesh();
		TestTrue(TEXT("Thaddeus wears his spectacles"), Spectacles && Spectacles->GetName() == TEXT("SM_ThaddeusSpectacles")
			&& Thaddeus->Hat->GetAttachSocketName() == DeadFriendPoses::Hat);
		if (Spectacles && Spectacles->FindSocket(ADeadFriend::SpeakerSocket))
		{
			TestTrue(TEXT("...his words from their socket"), Thaddeus->SpeakerPoint->GetAttachParent() == Thaddeus->Hat.Get());
		}
	}

	// Hiram leans to the speaking tube, and back to his stool after.
	Hiram->LeanToTube(1.f);
	TestTrue(TEXT("Hiram at the tube"), Hiram->IsAtTube());
	for (float Time = 0.f; Time < 1.5f; Time += 0.1f)
	{
		Hiram->UpdatePose(0.1f);
	}
	TestFalse(TEXT("...back on his stool after"), Hiram->IsAtTube());

	// Thaddeus at his work, and his knock.
	const EThaddeusActivity Working = Thaddeus->GetActivity();
	TestTrue(TEXT("Thaddeus measures or marks at his bench"), Working == EThaddeusActivity::Measure || Working == EThaddeusActivity::Mark);
	Thaddeus->KnockNow();
	TestTrue(TEXT("...knocks on the board"), Thaddeus->GetActivity() == EThaddeusActivity::Knock && Thaddeus->GetKnocks() == 1);
	for (float Time = 0.f; Time < ThaddeusPoses::KnockSeconds + 0.3f; Time += 0.1f)
	{
		Thaddeus->UpdatePose(0.1f);
	}
	TestFalse(TEXT("...and goes back to his measuring"), Thaddeus->GetActivity() == EThaddeusActivity::Knock);
	return true;
}

#endif

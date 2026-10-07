#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bestiary/BestiaryEntry.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/AmosPoses.h"
#include "Story/AmosWhitlock.h"
#include "Story/CaptionQueue.h"
#include "Story/CaptionSubsystem.h"
#include "Story/SpeakerPointComponent.h"
#include "Tests/MissionTestWorld.h"
#include "Tests/UnfinishedBusinessTestWorld.h"
#include "AnimationRuntime.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Misc/PackageName.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Amos Whitlock at his fence (Side 2, "Unfinished Business"): his pose table (the lean on the top rail, the sit on it, the
// talk), what he says at each point of the story, turning to whoever he talks to, settling onto the rail when Side 2 is
// done, ticking only while something of him moves, and his page in the Ledger. The mission itself is
// UnfinishedBusinessTests.cpp's.

using namespace UnfinishedBusinessTestWorld;

namespace
{
	/** The fence's top rail at the middle of a span (Amos.py's fence probe of FenceRail, cm). */
	constexpr double RailTop = 100.8;

	/** Lets whatever is being said play out to its end. */
	void PlayOut(UCaptionSubsystem& Captions)
	{
		for (int32 Line = 0; Line < 40 && Captions.GetCurrent(); ++Line)
		{
			Captions.Update(Captions.GetCurrent()->Seconds + 0.01f);
		}
	}

	/** Which of Amos's topics a talk would say now (EAmosTopic's order), or INDEX_NONE for his plain lines. */
	int32 TopicNow(const AAmosWhitlock& Amos)
	{
		int32 Topic = INDEX_NONE;
		Amos.SpeakerPoint->GetLinesNow(&Topic);
		return Topic;
	}

	/** A bone's head in the actor's frame for a seat: the pose table's, through where the seat stands the figure. */
	FVector InActor(EAmosPose Seat, const TCHAR* Bone, const FAmosPoseLayer& Layer = FAmosPoseLayer())
	{
		const FAmosPoseHeads Solved = AmosPoses::SolveHeads(Seat, Layer);
		const int32 Index = AmosPoses::FindBone(Bone);
		const FVector Head = Solved.Heads.IsValidIndex(Index) ? Solved.Heads[Index] : FVector::ZeroVector;
		return GetDefault<AAmosWhitlock>()->GetSeatTransform(Seat).TransformPosition(Head);
	}

	/** Which way a bone looks (its front after its whole turn), as a yaw in degrees. */
	float LookYaw(EAmosPose Seat, const TCHAR* Bone, const FAmosPoseLayer& Layer = FAmosPoseLayer())
	{
		const FAmosPoseHeads Solved = AmosPoses::SolveHeads(Seat, Layer);
		const int32 Index = AmosPoses::FindBone(Bone);
		const FVector Looking = Solved.Turns.IsValidIndex(Index) ? Solved.Turns[Index].RotateVector(FVector::ForwardVector) : FVector::ForwardVector;
		return FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(Looking.Y, Looking.X)));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAmosPosesTest, "Looter.Story.Amos.Poses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAmosPosesTest::RunTest(const FString& Parameters)
{
	// The pose table as the code reads it (Story/AmosPoseData.inl, from Amos.py's table): the Unpaid's bones and his three;
	// the lean with his forearms on the top rail and the fork against the post at his right; the sit on the rail, his head
	// turned into the sun; the talk only opening his jaw; the head and jaw a talk layers on top.
	TestEqual(TEXT("36 bones"), AmosPoses::NumBones(), 36);
	for (const FName Extra : AmosPoses::ExtraBones())
	{
		TestTrue(*FString::Printf(TEXT("His own bone %s"), *Extra.ToString()), AmosPoses::FindBone(Extra) != INDEX_NONE);
	}
	bool bParentsFirst = true;
	for (int32 Bone = 0; Bone < AmosPoses::NumBones(); ++Bone)
	{
		bParentsFirst &= AmosPoses::ParentOf(Bone) < Bone;
	}
	TestTrue(TEXT("Every bone after its parent"), bParentsFirst);
	const int32 Jaw = AmosPoses::FindBone(TEXT("jaw"));
	bool bTalkOnlyJaw = true;
	for (int32 Bone = 0; Bone < AmosPoses::NumBones(); ++Bone)
	{
		bTalkOnlyJaw &= Bone == Jaw || (AmosPoses::Own(EAmosPose::Talk, Bone).Equals(FQuat::Identity, 1e-4)
			&& AmosPoses::Shift(EAmosPose::Talk, Bone).IsNearlyZero(0.05));
	}
	TestTrue(TEXT("The talk moves only his jaw"), bTalkOnlyJaw);
	TestTrue(TEXT("...opening it about 8 degrees"), FMath::IsNearlyEqual(FMath::RadiansToDegrees(AmosPoses::Own(EAmosPose::Talk, Jaw).GetAngle()), 8.0, 0.5));

	// Where he stands for each seat: behind the fence's line leaning, on it sitting, turned to his right.
	TestTrue(TEXT("Leaning, he stands 35 cm behind the fence's line"), FMath::IsNearlyEqual(AmosPoses::LeanBack(), 35.f, 0.5f));
	TestTrue(TEXT("Sitting, he's turned 25 degrees to his right"), FMath::IsNearlyEqual(AmosPoses::SitTurn(), 25.f, 0.5f));

	// The lean: forearms folded on the top rail, over the fence's line; the fork let go against the post at his right.
	for (const TCHAR* Hand : { TEXT("hand_l"), TEXT("hand_r") })
	{
		const FVector At = InActor(EAmosPose::Lean, Hand);
		TestTrue(*FString::Printf(TEXT("Leaning, %s rests on the rail over the fence's line (%s)"), Hand, *At.ToCompactString()),
			FMath::Abs(At.X) < 15.0 && At.Z > RailTop - 5.0 && At.Z < RailTop + 15.0);
	}
	const FVector Fork = InActor(EAmosPose::Lean, TEXT("fork"));
	TestTrue(*FString::Printf(TEXT("...his fork against the post at his right, 75 cm along (%s)"), *Fork.ToCompactString()),
		Fork.Y > 55.0 && Fork.Y < 80.0);
	const FVector Head = InActor(EAmosPose::Lean, TEXT("head"));
	TestTrue(TEXT("...his head over the rail, bent toward his field"), Head.X > -5.0 && Head.Z > RailTop + 25.0 && Head.Z < 160.0);

	// The sit: on the rail over the fence's line, his head turned into the sun.
	const FVector Pelvis = InActor(EAmosPose::Sit, TEXT("pelvis"));
	TestTrue(*FString::Printf(TEXT("Sitting, his seat is on the top rail (%s)"), *Pelvis.ToCompactString()),
		FMath::Abs(Pelvis.X) < 10.0 && Pelvis.Z > RailTop + 10.0 && Pelvis.Z < RailTop + 30.0);
	const float SunLook = AmosPoses::HeadYawOf(EAmosPose::Sit);
	TestTrue(*FString::Printf(TEXT("...his head turned %.0f degrees further to his right, into the low sun"), SunLook), SunLook > 30.f && SunLook < 45.f);
	TestTrue(TEXT("Leaning, he looks straight ahead into his field"), FMath::Abs(AmosPoses::HeadYawOf(EAmosPose::Lean)) < 5.f);

	// A talk's layer: the head turned about his body's up (the hat riding it), the jaw opened as the talk pose opens it.
	FAmosPoseLayer Turned;
	Turned.HeadYaw = 30.f;
	TestTrue(TEXT("The layer turns his head 30 degrees"), FMath::IsNearlyEqual(LookYaw(EAmosPose::Lean, TEXT("head"), Turned)
		- LookYaw(EAmosPose::Lean, TEXT("head")), 30.f, 1.f));
	TestTrue(TEXT("...and the hat with it"), FMath::IsNearlyEqual(LookYaw(EAmosPose::Lean, TEXT("hat"), Turned), LookYaw(EAmosPose::Lean, TEXT("head"), Turned), 0.5f));
	FAmosPoseLayer Speaking;
	Speaking.JawOpen = 1.f;
	const FAmosPoseHeads Open = AmosPoses::SolveHeads(EAmosPose::Sit, Speaking);
	const FAmosPoseHeads Shut = AmosPoses::SolveHeads(EAmosPose::Sit);
	TestTrue(TEXT("...the jaw opened 8 degrees under the head"), FMath::IsNearlyEqual(FMath::RadiansToDegrees(
		Open.Turns[Jaw].AngularDistance(Shut.Turns[Jaw])), 8.0, 0.5));

	// His model, posed: the lean on the poseable mesh where the table puts it, the hat and fork on their bones.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector(-400.0, 0.0, 0.0));
	if (!Runner || !Player)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Runner->BeginForTesting({}, Campaign, Player, Valley);
	AAmosWhitlock* Amos = PlaceAmos(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Amos placed"), Amos))
	{
		return false;
	}
	Amos->DispatchBeginPlay();
	TestTrue(TEXT("His hull: found by the Interact key's line only (a ghost: the player and shots pass through)"),
		Amos->Body->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Block && Amos->Body->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Ignore
		&& Amos->Body->GetCollisionResponseToChannel(ECC_GameTraceChannel2) == ECR_Ignore && !Amos->Figure->CastShadow);
	if (!Amos->HasModel())
	{
		AddWarning(TEXT("SK_Amos isn't in this checkout (Art/Models/Creatures/Amos.py): his model's pose wasn't checked."));
		return true;
	}
	TestTrue(TEXT("His hat on his hat bone, his fork on his fork bone"), Amos->Hat->GetAttachSocketName() == FName(TEXT("hat"))
		&& Amos->Fork->GetAttachSocketName() == FName(TEXT("fork")) && Amos->Hat->GetStaticMesh() && Amos->Fork->GetStaticMesh());
	if (Amos->Hat->GetStaticMesh() && Amos->Hat->GetStaticMesh()->FindSocket(AAmosWhitlock::SpeakerSocket))
	{
		TestTrue(TEXT("...his words from his mouth, the hat's socket"), Amos->SpeakerPoint->GetAttachParent() == Amos->Hat.Get());
	}
	const FVector Posed = Amos->Figure->GetBoneLocationByName(TEXT("head"), EBoneSpaces::ComponentSpace);
	const int32 HeadBone = AmosPoses::FindBone(TEXT("head"));
	TestTrue(*FString::Printf(TEXT("His model leans as the table has it: the head at %s"), *Posed.ToCompactString()),
		Posed.Equals(AmosPoses::SolveHeads(EAmosPose::Lean).Heads[HeadBone], 2.0));
	TestTrue(TEXT("...the figure behind the fence's line"), Amos->Figure->GetRelativeLocation().Equals(FVector(-AmosPoses::LeanBack(), 0.0, 0.0), 0.1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAmosTopicsTest, "Looter.Story.Amos.Topics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAmosTopicsTest::RunTest(const FString& Parameters)
{
	// What Amos says by the story: nobody at the fence before Main 4; the first meeting, the bales, the hands and his
	// thanks through Side 2's steps; sitting on his fence after it, and a word of Abel after Main 6. He turns his head to
	// whoever he talks to and moves his jaw, settles onto the rail once Side 2 is done and his words are over, and ticks only
	// while something of him moves. A level begun after Side 2 finds him on the rail.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	// Off to his right, on the side of the fence he leans from.
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector(-150.0, 250.0, 0.0));
	if (!Runner || !Captions || !Player)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	Runner->BeginForTesting({ MakeMainFour(Scratch), MakeSideTwoAt(Scratch, SideTwoSteps), MakeMainSix(Scratch) }, Campaign, Player, Valley);
	AAmosWhitlock* Amos = PlaceAmos(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Amos placed"), Amos))
	{
		return false;
	}
	Amos->DispatchBeginPlay();
	Runner->Update(0.f);
	TestFalse(TEXT("Before Main 4: nobody at the fence"), Amos->IsShown());
	TestFalse(TEXT("...nobody to talk to"), Amos->SpeakerPoint->CanTalk() || Amos->SpeakerPoint->Talk(Player));

	const auto Expect = [this, Amos](const TCHAR* When, EAmosTopic Topic)
	{
		TestEqual(*FString::Printf(TEXT("%s: %s"), When, TopicSets[static_cast<int32>(Topic)]), TopicNow(*Amos), static_cast<int32>(Topic));
	};
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.MainFourDone")));
	TestTrue(TEXT("Main 4 done: Amos leans on his fence"), Amos->IsShown() && Amos->GetSeat() == EAmosPose::Lean);
	TestFalse(TEXT("...and nothing of him ticks"), Amos->IsActorTickEnabled());
	Expect(TEXT("Side 2's first step"), EAmosTopic::Meet);
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.Step0")));
	Expect(TEXT("Its second"), EAmosTopic::Bales);
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.Step1")));
	Expect(TEXT("Its third"), EAmosTopic::Hands);
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.Step2")));
	Expect(TEXT("Its last"), EAmosTopic::Thanks);

	// Talked to: he turns his head toward the player at his right, as far as his neck allows, and his jaw moves.
	TestTrue(TEXT("Talked to"), Amos->SpeakerPoint->Talk(Player));
	TestTrue(TEXT("...he ticks while he talks"), Amos->IsActorTickEnabled());
	for (int32 Frame = 0; Frame < 20; ++Frame)
	{
		Amos->UpdatePose(0.1f);
	}
	TestTrue(*FString::Printf(TEXT("...his head turned to his right, to the player (%.0f degrees)"), Amos->GetHeadYaw()),
		Amos->GetHeadYaw() > 20.f && Amos->GetHeadYaw() <= Amos->HeadTurnLimit + 0.1f);
	TestTrue(TEXT("...his jaw moving"), Amos->GetJawOpen() > 0.2f);

	// Side 2 done while he's still talking: he leans on until his words are over, then settles onto the rail.
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.Step3")));
	TestTrue(TEXT("Side 2 done"), Campaign.HasCompleted(SideTwo));
	Expect(TEXT("After it"), EAmosTopic::Fence);
	Amos->UpdatePose(0.1f);
	TestTrue(TEXT("...still leaning while he talks"), Amos->GetSeat() == EAmosPose::Lean && Amos->WantsToSit());
	PlayOut(*Captions);
	Amos->UpdatePose(0.1f);
	TestTrue(TEXT("His words over: he settles onto the rail"), Amos->IsSettling());
	for (int32 Frame = 0; Frame < 40; ++Frame)
	{
		Amos->UpdatePose(0.1f);
	}
	TestTrue(TEXT("...sitting on his fence, his head back where the sit has it"), Amos->IsSitting()
		&& FMath::IsNearlyZero(Amos->GetHeadYaw(), 0.1f) && Amos->GetJawOpen() == 0.f);
	TestFalse(TEXT("...and still: nothing of him ticks"), Amos->IsActorTickEnabled());
	TestTrue(TEXT("...his figure on the fence's line, turned to his right"), Amos->Figure->GetRelativeLocation().IsNearlyZero(0.1)
		&& FMath::IsNearlyEqual(Amos->Figure->GetRelativeRotation().Yaw, static_cast<double>(AmosPoses::SitTurn()), 0.1));

	// After Main 6: a word of Abel on his board.
	TestTrue(TEXT("Main 6 started"), Runner->StartMission(MainSix, 0, /*bForce*/ true));
	TestTrue(TEXT("...and finished"), Runner->CompleteMission(MainSix));
	Expect(TEXT("After Main 6"), EAmosTopic::AfterMainSix);

	// A level begun after Side 2: on the rail from the start, with no settling; the console leans him again for a while.
	AAmosWhitlock* Later = PlaceAmos(World, FVector(0.0, 3000.0, 0.0));
	if (TestNotNull(TEXT("Amos in a level begun later"), Later))
	{
		Later->DispatchBeginPlay();
		TestTrue(TEXT("...sitting on his fence from the start"), Later->IsShown() && Later->IsSitting() && !Later->IsSettling());
		TestFalse(TEXT("...not ticking"), Later->IsActorTickEnabled());
		Later->LeanNow();
		TestTrue(TEXT("The console leans him on the rail again"), Later->GetSeat() == EAmosPose::Lean);
		Later->RefreshShown();
		TestTrue(TEXT("...and the story settles him back onto it"), Later->IsSettling());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAmosPageTest, "Looter.Story.Amos.Page",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAmosPageTest::RunTest(const FString& Parameters)
{
	// His Ledger page (create_bestiary_pages.py): a story character's, among the Friends, open once Side 2 can begin (after
	// Main 4), with SK_Amos on the stand wearing his hat and fork on their bones, as the page itself names them (he has no
	// actor class for the stand to read them from).
	const bool bModelMade = FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(AAmosWhitlock::ModelPath)));
	const USkeletalMesh* Model = bModelMade ? LoadObject<USkeletalMesh>(nullptr, AAmosWhitlock::ModelPath) : nullptr;
	if (!Model)
	{
		AddWarning(TEXT("SK_Amos isn't in this checkout (Art/Models/Creatures/Amos.py): his stand wasn't checked."));
		return true;
	}
	// A page's bone parts: each on its bone as the model has it at rest; one on a bone the model lacks is left off.
	UBestiaryEntry* Entry = NewObject<UBestiaryEntry>(GetTransientPackage(), NAME_None, RF_Transient);
	Entry->Page = EBestiaryPage::StoryCharacter;
	FBestiaryBonePart Hat;
	Hat.Mesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(FString(AAmosWhitlock::HatPath)));
	Hat.Bone = TEXT("hat");
	FBestiaryBonePart Fork;
	Fork.Mesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(FString(AAmosWhitlock::ForkPath)));
	Fork.Bone = TEXT("fork");
	FBestiaryBonePart Stray = Hat;
	Stray.Bone = TEXT("no_such_bone");
	Entry->PreviewBoneParts = { Hat, Fork, Stray };
	const TArray<FBestiaryStandPart> Worn = Entry->GetPreviewParts(Model);
	TestEqual(TEXT("The stand wears his hat and fork, and nothing on a bone he lacks"), Worn.Num(), 2);
	const int32 HatBone = Model->GetRefSkeleton().FindBoneIndex(TEXT("hat"));
	const FBestiaryStandPart* OnHead = Worn.FindByPredicate([](const FBestiaryStandPart& Part) { return Part.Bone == FName(TEXT("hat")); });
	TestTrue(TEXT("...his hat at its bone, turned back by the bone's rest turn"), OnHead && OnHead->Mesh && HatBone != INDEX_NONE
		&& OnHead->Relative.GetLocation().IsNearlyZero() && (OnHead->Relative.GetRotation()
			* FAnimationRuntime::GetComponentSpaceTransformRefPose(Model->GetRefSkeleton(), HatBone).GetRotation()).Equals(FQuat::Identity, 1e-4));

	// The page itself, once the script has made it.
	if (!FPackageName::DoesPackageExist(TEXT("/Game/Data/Bestiary/DA_Bestiary_Amos")))
	{
		AddWarning(TEXT("DA_Bestiary_Amos isn't made yet: run Tools/Unreal/create_bestiary_pages.py Amos."));
		return true;
	}
	const UBestiaryEntry* Asset = LoadObject<UBestiaryEntry>(nullptr, TEXT("/Game/Data/Bestiary/DA_Bestiary_Amos.DA_Bestiary_Amos"));
	if (!TestNotNull(TEXT("DA_Bestiary_Amos loads"), Asset))
	{
		return false;
	}
	TestTrue(TEXT("Amos Whitlock: a story character among the Friends, with no actor"), Asset->Page == EBestiaryPage::StoryCharacter
		&& Asset->Category == EBestiaryCategory::Friend && Asset->ActorClass.IsNull() && !Asset->NeedsActor());
	TestTrue(TEXT("...open once Side 2 can begin, after Main 4"), Asset->KnownWhen.AfterMissions.Contains(MainFour));
	TestTrue(TEXT("...his model on the stand"), Asset->LoadPreviewMesh() == Model);
	TestEqual(TEXT("...wearing his hat and fork"), Asset->GetPreviewParts(Model).Num(), 2);
	return true;
}

#endif

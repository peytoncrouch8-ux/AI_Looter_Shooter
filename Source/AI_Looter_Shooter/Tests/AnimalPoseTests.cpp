#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/AnimalPose.h"
#include "World/AnimalPoseTables.h"
#include "World/ChickenYardRules.h"

// The small animals' pose tables as the code reads them (World/*PoseData.inl from Tools/animal_poses.py), blending
// between their poses staying inside the tables' extremes, and the chicken pens' rules (World/ChickenYardRules.h).

namespace
{
	/** Every row of a table and its mirror, as the animals blend them. */
	TArray<FAnimalPose> AnimalTestRows(const FAnimalPoseTable& Table)
	{
		TArray<FAnimalPose> Rows;
		for (int32 Pose = 0; Pose < Table.NumPoses; ++Pose)
		{
			Rows.Add(AnimalPose::FromTable(Table, Pose));
			Rows.Add(AnimalPose::Mirrored(Table, Rows.Last()));
		}
		return Rows;
	}

	FVector AnimalTestHead(const FAnimalPoseTable& Table, const FAnimalPose& Pose, const TCHAR* Bone)
	{
		FAnimalPoseSolve Solved;
		AnimalPose::Solve(Table, Pose, Solved);
		const int32 Index = Table.FindBone(Bone);
		return Solved.Heads.IsValidIndex(Index) ? Solved.Heads[Index] : FVector::ZeroVector;
	}

	/** A square pen 3 m across with a 1 m box in its middle (the tests' coop). */
	FChickenPen AnimalTestPen(float BlockYaw = 0.f)
	{
		FChickenPen Pen;
		Pen.Outline = { FVector2D(0.0, 0.0), FVector2D(300.0, 0.0), FVector2D(300.0, 300.0), FVector2D(0.0, 300.0) };
		FChickenPenBlock& Block = Pen.Blocks.AddDefaulted_GetRef();
		Block.Center = FVector2D(150.0, 150.0);
		Block.HalfSize = FVector2D(50.0, 50.0);
		Block.Yaw = BlockYaw;
		return Pen;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAnimalPoseTablesTest, "Looter.World.Animals.PoseTables",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAnimalPoseTablesTest::RunTest(const FString& Parameters)
{
	// The three tables: bones parents first, every bone's mirror the other side's (and back), the rest symmetric; a row per
	// pose of the animal's enum; the hens' crow is their rest; and the poses do what their names say.
	const FAnimalPoseTable& Hen = AnimalPoseTables::Hen();
	const FAnimalPoseTable& Rooster = AnimalPoseTables::Rooster();
	const FAnimalPoseTable& Cat = AnimalPoseTables::Cat();
	TestEqual(TEXT("The hens' 24 bones"), Hen.NumBones, 24);
	TestEqual(TEXT("The rooster's 24"), Rooster.NumBones, 24);
	TestEqual(TEXT("The cat's 29"), Cat.NumBones, 29);
	TestEqual(TEXT("A hen row per chicken pose"), Hen.NumPoses, static_cast<int32>(EChickenPose::Count));
	TestEqual(TEXT("A rooster row per chicken pose"), Rooster.NumPoses, static_cast<int32>(EChickenPose::Count));
	TestEqual(TEXT("A cat row per cat pose"), Cat.NumPoses, static_cast<int32>(ECatPose::Count));
	for (const FAnimalPoseTable* Table : { &Hen, &Rooster, &Cat })
	{
		bool bParentsFirst = true;
		bool bMirrors = true;
		bool bSymmetric = true;
		for (int32 Bone = 0; Bone < Table->NumBones; ++Bone)
		{
			bParentsFirst &= Table->ParentOf(Bone) < Bone;
			const int32 Other = Table->MirrorOf(Bone);
			bMirrors &= Table->MirrorOf(Other) == Bone;
			const FVector Rest = Table->RestHead(Bone);
			bSymmetric &= Table->RestHead(Other).Equals(FVector(Rest.X, -Rest.Y, Rest.Z), 0.1);
		}
		TestTrue(TEXT("Every bone after its parent"), bParentsFirst);
		TestTrue(TEXT("Every bone's mirror mirrors back to it"), bMirrors);
		TestTrue(TEXT("The rest pose is symmetric (a mirrored pose lands on the other side's joints)"), bSymmetric);
	}
	TestEqual(TEXT("Chickens' pose names: the table's"), FString(Hen.PoseNames[static_cast<int32>(EChickenPose::FlutterDown)]), FString(TEXT("flutter_down")));
	TestEqual(TEXT("The cat's: the table's"), FString(Cat.PoseNames[static_cast<int32>(ECatPose::LoafWatch)]), FString(TEXT("loaf_watch")));

	const AnimalPoseTables::FChickenBones& Chicken = AnimalPoseTables::ChickenBones();
	TestTrue(TEXT("The chicken's named bones are in the table"), Chicken.Head != INDEX_NONE && Chicken.Wattles != INDEX_NONE
		&& Chicken.TailTip != INDEX_NONE && Chicken.Jaw != INDEX_NONE && Chicken.Neck2 != INDEX_NONE);
	TestEqual(TEXT("...both legs, five bones each"), Chicken.Legs.Num(), 10);
	TestEqual(TEXT("...and the rest above them"), Chicken.Upper.Num(), 14);
	const AnimalPoseTables::FCatBones& CatBones = AnimalPoseTables::CatBones();
	TestEqual(TEXT("The cat's tail, six links"), CatBones.Tail.Num(), 6);
	TestTrue(TEXT("...its ears, neck and head"), CatBones.EarL != INDEX_NONE && CatBones.EarR != INDEX_NONE && CatBones.Neck != INDEX_NONE);

	// A hen has no crow: it's her rest. The rooster's opens his beak.
	bool bHenCrowIsRest = true;
	for (int32 Bone = 0; Bone < Hen.NumBones; ++Bone)
	{
		bHenCrowIsRest &= Hen.Own(static_cast<int32>(EChickenPose::Crow), Bone).Equals(Hen.Own(static_cast<int32>(EChickenPose::Rest), Bone), 1e-4)
			&& Hen.Shift(static_cast<int32>(EChickenPose::Crow), Bone).Equals(Hen.Shift(static_cast<int32>(EChickenPose::Rest), Bone), 0.01);
	}
	TestTrue(TEXT("A hen's crow is her rest"), bHenCrowIsRest);
	TestTrue(TEXT("The rooster's crow opens his beak"),
		AnimalPose::Degrees(Rooster.Own(static_cast<int32>(EChickenPose::Crow), Chicken.Jaw)) > 10.f);

	// The poses do what they say: the peck's beak at the ground, the sit and the loaf down on the belly.
	const FVector RestBeak = AnimalTestHead(Hen, AnimalPoseTables::Chicken(false, EChickenPose::Rest), TEXT("jaw"));
	const FVector PeckBeak = AnimalTestHead(Hen, AnimalPoseTables::Chicken(false, EChickenPose::Peck), TEXT("jaw"));
	TestTrue(*FString::Printf(TEXT("The peck brings the beak down to the ground (%.1f cm, from %.1f)"), PeckBeak.Z, RestBeak.Z),
		PeckBeak.Z < 20.0 && PeckBeak.Z < RestBeak.Z - 15.0);
	const FVector RestBody = AnimalTestHead(Hen, AnimalPoseTables::Chicken(false, EChickenPose::Rest), TEXT("body"));
	const FVector SitBody = AnimalTestHead(Hen, AnimalPoseTables::Chicken(false, EChickenPose::Sit), TEXT("body"));
	TestTrue(TEXT("Sitting, a hen's body is down on its belly"), SitBody.Z < RestBody.Z - 7.0);
	const FVector Standing = AnimalTestHead(Cat, AnimalPoseTables::CatPose(ECatPose::Stand), TEXT("pelvis"));
	const FVector Loafing = AnimalTestHead(Cat, AnimalPoseTables::CatPose(ECatPose::Loaf), TEXT("pelvis"));
	TestTrue(TEXT("Loafing, the cat lies low"), Loafing.Z < Standing.Z - 7.0);

	// A one-sided pose and its mirror: the walk's left toes are the mirrored walk's right toes, reflected.
	const FAnimalPose& Walk = AnimalPoseTables::Chicken(false, EChickenPose::Walk);
	const FAnimalPose& MirroredWalk = AnimalPoseTables::Chicken(false, EChickenPose::Walk, true);
	const FVector LeftToes = AnimalTestHead(Hen, Walk, TEXT("toes_l"));
	const FVector RightToes = AnimalTestHead(Hen, MirroredWalk, TEXT("toes_r"));
	TestTrue(*FString::Printf(TEXT("The mirrored walk puts the right foot where the walk put the left (%s, %s)"), *LeftToes.ToCompactString(),
		*RightToes.ToCompactString()), RightToes.Equals(FVector(LeftToes.X, -LeftToes.Y, LeftToes.Z), 0.05));
	const FAnimalPose Twice = AnimalPose::Mirrored(Hen, MirroredWalk);
	bool bSame = true;
	for (int32 Bone = 0; Bone < Hen.NumBones; ++Bone)
	{
		bSame &= Twice.Own[Bone].Equals(Walk.Own[Bone], 1e-4) && Twice.Shift[Bone].Equals(Walk.Shift[Bone], 0.01);
	}
	TestTrue(TEXT("Mirrored twice, a pose is itself"), bSame);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAnimalPoseBlendTest, "Looter.World.Animals.PoseBlend",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAnimalPoseBlendTest::RunTest(const FString& Parameters)
{
	// Blending stays inside the tables' extremes: between any two poses (and their mirrors), at every share, every joint
	// stays within the box the table's own poses span (a few cm for the arcs a turn swings through); and the blends the
	// animals make directly (a peck, a stride, the flutter, sitting down, the cat's watch and gaits) never turn a bone
	// further than the farther of their two ends.
	struct FTableCase
	{
		const FAnimalPoseTable* Table;
		TArray<TPair<int32, int32>> Direct;
	};
	auto RowOf = [](int32 Pose, bool bMirrored) { return Pose * 2 + (bMirrored ? 1 : 0); };
	auto ChickenRow = [](EChickenPose Pose) { return static_cast<int32>(Pose); };
	auto CatRow = [](ECatPose Pose) { return static_cast<int32>(Pose); };
	const TArray<TPair<int32, int32>> ChickenDirect = {
		{ RowOf(ChickenRow(EChickenPose::Rest), false), RowOf(ChickenRow(EChickenPose::Peck), false) },
		{ RowOf(ChickenRow(EChickenPose::Rest), false), RowOf(ChickenRow(EChickenPose::Scratch), false) },
		{ RowOf(ChickenRow(EChickenPose::Rest), false), RowOf(ChickenRow(EChickenPose::Scratch), true) },
		{ RowOf(ChickenRow(EChickenPose::Walk), false), RowOf(ChickenRow(EChickenPose::Walk), true) },
		{ RowOf(ChickenRow(EChickenPose::FlutterUp), false), RowOf(ChickenRow(EChickenPose::FlutterDown), false) },
		{ RowOf(ChickenRow(EChickenPose::FlutterUp), false), RowOf(ChickenRow(EChickenPose::FlutterUp), true) },
		{ RowOf(ChickenRow(EChickenPose::Rest), false), RowOf(ChickenRow(EChickenPose::Sit), false) },
		{ RowOf(ChickenRow(EChickenPose::Rest), false), RowOf(ChickenRow(EChickenPose::Crow), false) },
		{ RowOf(ChickenRow(EChickenPose::Rest), false), RowOf(ChickenRow(EChickenPose::Alert), false) },
		{ RowOf(ChickenRow(EChickenPose::Peck), false), RowOf(ChickenRow(EChickenPose::Alert), false) },
	};
	const TArray<FTableCase> Cases = {
		{ &AnimalPoseTables::Hen(), ChickenDirect },
		{ &AnimalPoseTables::Rooster(), ChickenDirect },
		{ &AnimalPoseTables::Cat(), {
			{ RowOf(CatRow(ECatPose::Loaf), false), RowOf(CatRow(ECatPose::LoafWatch), false) },
			{ RowOf(CatRow(ECatPose::WalkA), false), RowOf(CatRow(ECatPose::WalkB), false) },
			{ RowOf(CatRow(ECatPose::Slink), false), RowOf(CatRow(ECatPose::Slink), true) },
			{ RowOf(CatRow(ECatPose::Stand), false), RowOf(CatRow(ECatPose::Stretch), false) },
			{ RowOf(CatRow(ECatPose::WalkA), false), RowOf(CatRow(ECatPose::Slink), false) },
			{ RowOf(CatRow(ECatPose::WalkB), false), RowOf(CatRow(ECatPose::Stand), false) } } },
	};
	const float Shares[] = { 0.1f, 0.25f, 0.5f, 0.75f, 0.9f };
	for (const FTableCase& Case : Cases)
	{
		const FAnimalPoseTable& Table = *Case.Table;
		const TArray<FAnimalPose> Rows = AnimalTestRows(Table);
		TArray<FBox> Boxes;
		Boxes.Init(FBox(ForceInit), Table.NumBones);
		for (const FAnimalPose& Pose : Rows)
		{
			FAnimalPoseSolve Solved;
			AnimalPose::Solve(Table, Pose, Solved);
			for (int32 Bone = 0; Bone < Table.NumBones; ++Bone)
			{
				Boxes[Bone] += Solved.Heads[Bone];
			}
		}
		int32 Outside = 0;
		float Worst = 0.f;
		FAnimalPose Blended;
		FAnimalPoseSolve Solved;
		for (int32 A = 0; A < Rows.Num(); ++A)
		{
			for (int32 B = A + 1; B < Rows.Num(); ++B)
			{
				for (const float Share : Shares)
				{
					AnimalPose::Blend(Rows[A], Rows[B], Share, Blended);
					AnimalPose::Solve(Table, Blended, Solved);
					for (int32 Bone = 0; Bone < Table.NumBones; ++Bone)
					{
						const float Out = FMath::Sqrt(static_cast<float>(Boxes[Bone].ComputeSquaredDistanceToPoint(Solved.Heads[Bone])));
						Worst = FMath::Max(Worst, Out);
						Outside += Out > 4.f ? 1 : 0;
					}
				}
			}
		}
		TestEqual(*FString::Printf(TEXT("Blending two poses keeps every joint within 4 cm of the table's own reach (worst %.2f cm)"), Worst),
			Outside, 0);
		int32 Overturned = 0;
		for (const TPair<int32, int32>& Pair : Case.Direct)
		{
			for (int32 Step = 0; Step <= 20; ++Step)
			{
				AnimalPose::Blend(Rows[Pair.Key], Rows[Pair.Value], Step / 20.f, Blended);
				for (int32 Bone = 0; Bone < Table.NumBones; ++Bone)
				{
					const float Most = FMath::Max(AnimalPose::Degrees(Rows[Pair.Key].Own[Bone]), AnimalPose::Degrees(Rows[Pair.Value].Own[Bone]));
					Overturned += AnimalPose::Degrees(Blended.Own[Bone]) > Most + 0.5f ? 1 : 0;
				}
			}
		}
		TestEqual(TEXT("The animals' own blends turn no bone past the farther of their ends"), Overturned, 0);
	}

	// The layers on top: a stride eases from one contact to the next and back; a look turns the head and what rides it.
	const FAnimalPoseTable& Hen = AnimalPoseTables::Hen();
	FAnimalPose Stride;
	const float StrideStep = AnimalPose::StrideBlend(AnimalPoseTables::Chicken(false, EChickenPose::Walk),
		AnimalPoseTables::Chicken(false, EChickenPose::Walk, true), 0.f, Stride);
	TestTrue(TEXT("A stride begins at its first contact"), StrideStep < 0.01f
		&& Stride.Own[Hen.FindBone(TEXT("shin_l"))].Equals(AnimalPoseTables::Chicken(false, EChickenPose::Walk).Own[Hen.FindBone(TEXT("shin_l"))], 1e-3));
	FAnimalPose Looking = AnimalPoseTables::Chicken(false, EChickenPose::Rest);
	const FVector BeakBefore = AnimalTestHead(Hen, Looking, TEXT("jaw"));
	AnimalPose::Look(Looking, Hen.FindBone(TEXT("head")), 60.f, 0.f);
	const FVector BeakAfter = AnimalTestHead(Hen, Looking, TEXT("jaw"));
	TestTrue(*FString::Printf(TEXT("A look 60 degrees right swings the beak to the right (%s)"), *BeakAfter.ToCompactString()),
		BeakAfter.Y > BeakBefore.Y + 1.5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAnimalPenRulesTest, "Looter.World.Animals.PenRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAnimalPenRulesTest::RunTest(const FString& Parameters)
{
	// A pen: inside its outline with room to spare, out of its block; ways round the block, not through it; a run away
	// from a threat that ends farther from it, by a way that's clear.
	const FChickenPen Pen = AnimalTestPen();
	TestTrue(TEXT("A corner spot with room is inside"), ChickenPens::IsInside(Pen, FVector2D(20.0, 20.0), 10.f));
	TestFalse(TEXT("Too near the fence isn't"), ChickenPens::IsInside(Pen, FVector2D(5.0, 150.0), 10.f));
	TestFalse(TEXT("In the coop isn't"), ChickenPens::IsInside(Pen, FVector2D(150.0, 150.0), 10.f));
	TestFalse(TEXT("Brushing the coop isn't"), ChickenPens::IsInside(Pen, FVector2D(150.0, 95.0), 10.f));
	TestTrue(TEXT("Clear of it is"), ChickenPens::IsInside(Pen, FVector2D(150.0, 80.0), 10.f));
	TestFalse(TEXT("Outside the pen isn't"), ChickenPens::IsInside(Pen, FVector2D(350.0, 150.0), 10.f));
	TestFalse(TEXT("No way through the coop"), ChickenPens::IsWayClear(Pen, FVector2D(40.0, 150.0), FVector2D(260.0, 150.0), 10.f));
	TestTrue(TEXT("A way along beside it"), ChickenPens::IsWayClear(Pen, FVector2D(40.0, 40.0), FVector2D(260.0, 40.0), 10.f));

	const FChickenPen Turned = AnimalTestPen(45.f);
	TestTrue(TEXT("A turned block leaves its box's corners free"), ChickenPens::IsInside(Turned, FVector2D(95.0, 95.0), 5.f));
	TestFalse(TEXT("...but not its middle"), ChickenPens::IsInside(Turned, FVector2D(150.0, 110.0), 5.f));

	const TArray<FVector2D> Spots = ChickenPens::Spots(Pen, 15.f, 10.f);
	const TArray<FVector2D> Corners = ChickenPens::Corners(Pen, 10.f);
	bool bSpotsInside = Spots.Num() > 100;
	for (const FVector2D& Spot : Spots)
	{
		bSpotsInside &= ChickenPens::IsInside(Pen, Spot, 10.f);
	}
	TestTrue(TEXT("The pen's spots are all inside"), bSpotsInside);
	TestEqual(TEXT("A way round each of the coop's four corners"), Corners.Num(), 4);

	FRandomStream Random(7);
	const FVector2D From(40.0, 150.0);
	const FVector2D Threat(-120.0, 150.0);
	const ChickenPens::FFleeRun Run = ChickenPens::FindFleeRun(Pen, Spots, Corners, From, Threat, TConstArrayView<FVector2D>(), 10.f, Random);
	TestTrue(TEXT("A run is found"), Run.bFound);
	TestTrue(*FString::Printf(TEXT("...that ends well away from the threat (%s)"), *Run.Target.ToString()),
		FVector2D::Distance(Run.Target, Threat) > FVector2D::Distance(From, Threat) + 100.0);
	const bool bClear = Run.bVia ? ChickenPens::IsWayClear(Pen, From, Run.Via, 10.f) && ChickenPens::IsWayClear(Pen, Run.Via, Run.Target, 10.f)
		: ChickenPens::IsWayClear(Pen, From, Run.Target, 10.f);
	TestTrue(TEXT("...by a clear way (straight or round a corner)"), bClear);
	return true;
}

#endif

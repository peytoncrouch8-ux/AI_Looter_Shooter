#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bestiary/BestiaryEntry.h"
#include "Bosses/AbelKeeper.h"
#include "Bosses/AbelPoses.h"
#include "Bosses/AbelRules.h"
#include "Bosses/BossComponent.h"
#include "Bosses/BossRules.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/UnpaidCreature.h"
#include "Progression/ProgressionSettings.h"
#include "Tests/AbelTestWorld.h"
#include "AnimationRuntime.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "ReferenceSkeleton.h"
#include "Tests/AutomationCommon.h"

// Abel, the Keeper (Docs/Areas/RansomsRest.md, "The boss"): his pose table, his fight as data, and his body. His fight in
// play is AbelFightTests.cpp's; Main 6 and his board are GravewindTests.cpp's.

using namespace AbelTestWorld;

namespace
{
	bool Near(const FVector& A, const FVector& B, double Tolerance)
	{
		return FVector::Dist(A, B) <= Tolerance;
	}

	USkeletalMesh* LoadModel()
	{
		return FPackageName::DoesPackageExist(TEXT("/Game/Art/Creatures/SK_Abel"))
			? LoadObject<USkeletalMesh>(nullptr, AAbelKeeper::ModelPath) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbelPosesTest, "Looter.Bosses.Abel.Poses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAbelPosesTest::RunTest(const FString& Parameters)
{
	// The table (Tools/abel_poses.py from Abel.py): his 39 bones, parents first, the six the Unpaid lacks among them.
	TestEqual(TEXT("39 bones"), AbelPoses::NumBones(), 39);
	for (int32 Bone = 0; Bone < AbelPoses::NumBones(); ++Bone)
	{
		TestTrue(*FString::Printf(TEXT("%s: its parent comes first"), *AbelPoses::BoneName(Bone).ToString()), AbelPoses::ParentOf(Bone) < Bone);
	}
	for (const FName Extra : AbelPoses::ExtraBones())
	{
		TestTrue(*FString::Printf(TEXT("%s is in the table"), *Extra.ToString()), AbelPoses::FindBone(Extra) != INDEX_NONE);
	}

	// The idle is the rest pose (his model is bound in it).
	bool bIdleRests = true;
	for (int32 Bone = 0; Bone < AbelPoses::NumBones(); ++Bone)
	{
		bIdleRests &= AbelPoses::Own(EAbelPose::Idle, Bone).Equals(FQuat::Identity, 1e-4) && AbelPoses::Shift(EAbelPose::Idle, Bone).IsNearlyZero(0.05);
	}
	TestTrue(TEXT("The idle is his rest pose"), bIdleRests);

	// The art notes' shifts: the pelvis lunging, kneeling and sitting; the shroud's first link and the coat's sides knelt and sat.
	const int32 Pelvis = AbelPoses::FindBone(TEXT("pelvis"));
	const int32 Tail = AbelPoses::FindBone(TEXT("tail_01"));
	const int32 SideL = AbelPoses::FindBone(TEXT("skirt_l_01"));
	const int32 SideR = AbelPoses::FindBone(TEXT("skirt_r_01"));
	const int32 Gun = AbelPoses::FindBone(TEXT("gun"));
	TestTrue(TEXT("Lunge: the pelvis (8, 0, -3)"), Near(AbelPoses::Shift(EAbelPose::Lunge, Pelvis), FVector(8.0, 0.0, -3.0), 0.05));
	TestTrue(TEXT("Kneel: the pelvis (-3, 0, -70)"), Near(AbelPoses::Shift(EAbelPose::Kneel, Pelvis), FVector(-3.0, 0.0, -70.0), 0.05));
	TestTrue(TEXT("Sit: the pelvis (-3, 0, -90)"), Near(AbelPoses::Shift(EAbelPose::Sit, Pelvis), FVector(-3.0, 0.0, -90.0), 0.05));
	TestTrue(TEXT("Kneel: tail_01 (0, 0, -8.6)"), Near(AbelPoses::Shift(EAbelPose::Kneel, Tail), FVector(0.0, 0.0, -8.6), 0.05));
	TestTrue(TEXT("Sit: tail_01 (8, 0, 11)"), Near(AbelPoses::Shift(EAbelPose::Sit, Tail), FVector(8.0, 0.0, 11.0), 0.05));
	TestTrue(TEXT("Kneel: the coat's sides about (2.1, 0, -13.5)"), Near(AbelPoses::Shift(EAbelPose::Kneel, SideL), FVector(2.1, 0.0, -13.5), 0.3)
		&& Near(AbelPoses::Shift(EAbelPose::Kneel, SideR), FVector(2.1, 0.0, -13.5), 0.3));
	TestTrue(TEXT("Sit: the coat's sides about (6.1, -/+1.4, 0)"), Near(AbelPoses::Shift(EAbelPose::Sit, SideL), FVector(6.1, -1.45, 0.0), 0.3)
		&& Near(AbelPoses::Shift(EAbelPose::Sit, SideR), FVector(6.1, 1.4, 0.0), 0.4));

	// Knelt and sat his shroud lies as the table lays it; standing, the chains swing it.
	for (int32 Pose = 0; Pose < static_cast<int32>(EAbelPose::Count); ++Pose)
	{
		const EAbelPose Each = static_cast<EAbelPose>(Pose);
		TestEqual(*FString::Printf(TEXT("%s: the table lays the shroud only knelt or sat"), AbelPoses::Name(Each)), AbelPoses::LaysShroud(Each),
			Each == EAbelPose::Kneel || Each == EAbelPose::Sit);
	}

	// The lunge turns the pump half a turn on its bone, butt first.
	TestNearlyEqual(TEXT("The lunge turns the pump half a turn"), FMath::RadiansToDegrees(AbelPoses::Own(EAbelPose::Lunge, Gun).GetAngle()), 180.0, 1.0);

	// Knelt, the pump lies on the boards where the art notes put it: (70, 30, 3.35) cm, turned (-0.6241, 0.32659, -0.27837, 0.65295).
	const FAbelPoseHeads Knelt = AbelPoses::SolveHeads(EAbelPose::Kneel);
	const FQuat Laid(-0.6241, 0.32659, -0.27837, 0.65295);
	TestTrue(FString::Printf(TEXT("Knelt, the pump lies at (70, 30, 3.35) (%s)"), *Knelt.Heads[Gun].ToCompactString()), Near(Knelt.Heads[Gun], FVector(70.0, 30.0, 3.35), 0.1));
	TestTrue(TEXT("...turned as the art notes give it"), Knelt.Turns[Gun].AngularDistance(Laid.GetNormalized()) < 0.01);
	TestTrue(TEXT("...and the table's own note of it agrees"), Near(AbelPoses::KneelPumpHead(), Knelt.Heads[Gun], 0.1)
		&& AbelPoses::KneelPumpTurn().AngularDistance(Knelt.Turns[Gun]) < 0.01);

	// His bones keep their lengths in every pose: only shifted bones leave where their parent carries them.
	for (int32 Pose = 0; Pose < static_cast<int32>(EAbelPose::Count); ++Pose)
	{
		const EAbelPose Each = static_cast<EAbelPose>(Pose);
		const FAbelPoseHeads Solved = AbelPoses::SolveHeads(Each);
		bool bRigid = true;
		for (int32 Bone = 0; Bone < AbelPoses::NumBones(); ++Bone)
		{
			const int32 Parent = AbelPoses::ParentOf(Bone);
			if (Parent != INDEX_NONE && AbelPoses::Shift(Each, Bone).IsNearlyZero(0.05))
			{
				const double Rest = FVector::Dist(AbelPoses::RestHead(Bone), AbelPoses::RestHead(Parent));
				bRigid &= FMath::IsNearlyEqual(FVector::Dist(Solved.Heads[Bone], Solved.Heads[Parent]), Rest, 0.1);
			}
		}
		TestTrue(*FString::Printf(TEXT("%s: every unshifted bone keeps its length"), AbelPoses::Name(Each)), bRigid);
	}

	// Where the shot leaves: ahead of him, about his chest.
	const FVector Muzzle = AbelPoses::FireMuzzle();
	TestTrue(FString::Printf(TEXT("The pump's muzzle in the shot is ahead at chest height (%s)"), *Muzzle.ToCompactString()), Muzzle.X > 80.0
		&& Muzzle.Z > 90.0 && Muzzle.Z < 145.0);

	// On his model: every bone in the table, at the table's rest heads; sat, his pelvis a hand over the board.
	USkeletalMesh* Model = LoadModel();
	if (!Model)
	{
		AddWarning(TEXT("SK_Abel isn't imported: the table isn't checked against his skeleton."));
		return true;
	}
	const FReferenceSkeleton& Skeleton = Model->GetRefSkeleton();
	for (int32 Bone = 0; Bone < AbelPoses::NumBones(); ++Bone)
	{
		const int32 Index = Skeleton.FindBoneIndex(AbelPoses::BoneName(Bone));
		if (TestTrue(*FString::Printf(TEXT("SK_Abel has %s"), *AbelPoses::BoneName(Bone).ToString()), Index != INDEX_NONE))
		{
			const FVector Rest = FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, Index).GetLocation();
			TestTrue(*FString::Printf(TEXT("%s rests where the table measured it (%s)"), *AbelPoses::BoneName(Bone).ToString(), *Rest.ToCompactString()),
				Near(Rest, AbelPoses::RestHead(Bone), 0.5));
		}
	}
	const TArray<FTransform> Sat = AbelPoses::Solve(Skeleton, EAbelPose::Sit);
	const int32 PelvisIndex = Skeleton.FindBoneIndex(TEXT("pelvis"));
	if (Sat.IsValidIndex(PelvisIndex))
	{
		TestTrue(FString::Printf(TEXT("Sat, his pelvis is a hand over the board (%.1f cm)"), Sat[PelvisIndex].GetLocation().Z),
			FMath::IsNearlyEqual(Sat[PelvisIndex].GetLocation().Z, 10.0, 1.0));
	}
	const TArray<FTransform> Kneeling = AbelPoses::Solve(Skeleton, EAbelPose::Kneel);
	const int32 GunIndex = Skeleton.FindBoneIndex(TEXT("gun"));
	if (Kneeling.IsValidIndex(GunIndex))
	{
		TestTrue(TEXT("On his skeleton too, the pump lies on the boards"), Near(Kneeling[GunIndex].GetLocation(), FVector(70.0, 30.0, 3.35), 0.5));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbelRulesTest, "Looter.Bosses.Abel.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAbelRulesTest::RunTest(const FString& Parameters)
{
	const TArray<FBossPhase> Phases = BossRules::Ordered(AbelRules::MakePhases());
	if (!TestEqual(TEXT("Three phases"), Phases.Num(), 3))
	{
		return false;
	}
	TestTrue(TEXT("\"You brought them here\" from full, \"The bell\" at 60%, \"Let me go\" at 25%"),
		Phases[0].Name.ToString() == TEXT("You brought them here") && Phases[1].Name.ToString() == TEXT("The bell")
		&& Phases[2].Name.ToString() == TEXT("Let me go") && Phases[0].HealthShare == 1.f && Phases[1].HealthShare == 0.6f && Phases[2].HealthShare == 0.25f);
	TestEqual(TEXT("At 61% he's in the first"), BossRules::PhaseAt(Phases, 0.61f), 0);
	TestEqual(TEXT("At 59% the second"), BossRules::PhaseAt(Phases, 0.59f), 1);
	TestEqual(TEXT("At 24% the third"), BossRules::PhaseAt(Phases, 0.24f), 2);

	auto Find = [](const FBossPhase& Phase, TFunctionRef<bool(const FBossPhaseEvent&)> Match) -> const FBossPhaseEvent*
	{
		return Phase.Events.FindByPredicate(Match);
	};
	auto Custom = [&Find](const FBossPhase& Phase, FName Name) { return Find(Phase, [Name](const FBossPhaseEvent& Event) { return Event.Kind == EBossEventKind::Custom && Event.Name == Name; }); };

	// "You brought them here": the buckshot, the grief every 12 s, two Unpaid every 25 s (at most 4), round the deck's middle.
	const FBossPhaseEvent* Buckshot = Custom(Phases[0], AbelRules::BuckshotEvent());
	const FBossPhaseEvent* Grief = Custom(Phases[0], AbelRules::GrieveEvent());
	const FBossPhaseEvent* Rising = Find(Phases[0], [](const FBossPhaseEvent& Event) { return Event.Kind == EBossEventKind::AddWave; });
	TestTrue(TEXT("Phase one: the buckshot, again and again"), Buckshot && Buckshot->RepeatEvery > 0.f);
	TestTrue(TEXT("...he grieves every 12 s"), Grief && Grief->Delay == 12.f && Grief->RepeatEvery == 12.f);
	TestTrue(TEXT("...two Unpaid rise every 25 s, at most 4, round the deck's middle"), Rising && Rising->Delay == 25.f && Rising->RepeatEvery == 25.f
		&& Rising->Wave.Count == 2 && Rising->Wave.MaxAlive == 4 && Rising->Wave.bAroundSpot
		&& Rising->Wave.CreatureClass == AUnpaidCreature::StaticClass() && Rising->Wave.Rank == ECreatureRank::Basic);

	// "The bell": the bell at once, out of reach until his own code ends it (the lanterns), 8 Unpaid in two waves of four.
	const FBossPhaseEvent* Bell = Custom(Phases[1], AbelRules::BellEvent());
	const FBossPhaseEvent* Spell = Find(Phases[1], [](const FBossPhaseEvent& Event) { return Event.Kind == EBossEventKind::Untargetable; });
	TArray<const FBossPhaseEvent*> Waves;
	for (const FBossPhaseEvent& Event : Phases[1].Events)
	{
		if (Event.Kind == EBossEventKind::AddWave)
		{
			Waves.Add(&Event);
		}
	}
	TestTrue(TEXT("Phase two: the bell as it starts"), Bell && Bell->Delay == 0.f && Bell->RepeatEvery == 0.f);
	TestTrue(TEXT("...he can't be hurt until the lanterns drag him back (no time limit, not his adds)"), Spell && Spell->Untargetable.Seconds == 0.f
		&& !Spell->Untargetable.bUntilAddsDie && Spell->Untargetable.bWithdraw && !Spell->Untargetable.Hint.IsEmpty());
	TestTrue(TEXT("...8 Unpaid in two waves of four, the second later"), Waves.Num() == 2 && Waves[0]->Wave.Count + Waves[1]->Wave.Count == 8
		&& Waves[1]->Delay > Waves[0]->Delay && Waves[0]->RepeatEvery == 0.f && Waves[1]->RepeatEvery == 0.f && Waves[0]->Wave.bAroundSpot);
	TestEqual(TEXT("Never more than 10 adds at once"), BossRules::AddsToSpawn(8, 4, 0, 10), 6);

	// "Let me go": the Gravewind at once, the walk into it every 10 s, the buckshot quicker.
	const FBossPhaseEvent* Wind = Custom(Phases[2], AbelRules::GravewindEvent());
	const FBossPhaseEvent* Walk = Custom(Phases[2], AbelRules::WalkOffEvent());
	const FBossPhaseEvent* Quicker = Custom(Phases[2], AbelRules::BuckshotEvent());
	TestTrue(TEXT("Phase three: the wind as it starts, the walk into it every 10 s"), Wind && Wind->Delay == 0.f && Walk && Walk->RepeatEvery == 10.f);
	TestTrue(TEXT("...the buckshot quicker than before"), Quicker && Buckshot && Quicker->RepeatEvery < Buckshot->RepeatEvery);

	// The buckshot: a one-second flare of his own (no muzzle glow of the boss's), slow pale pellets in a cone.
	const FBossVolley Volley = AbelRules::Buckshot();
	TestTrue(TEXT("The buckshot: a cone of slow pellets with no wind-up of the boss's own"), Volley.Pellets >= 5 && Volley.Speed < 1200.f
		&& Volley.WindupSeconds == 0.f && Volley.SpreadDegrees > 10.f && Volley.Muzzle.X > 60.0);
	TestEqual(TEXT("...after a one-second flare"), GetDefault<AAbelKeeper>()->FlareSeconds, 1.f);

	// His numbers: 40 Unpaid of his level (about 10,500 at level 9), Boss rank, 1.3 times, a 6 cm coal.
	const AAbelKeeper* Defaults = GetDefault<AAbelKeeper>();
	const UHealthComponent* Health = Defaults->FindComponentByClass<UHealthComponent>();
	if (TestNotNull(TEXT("His health"), Health))
	{
		const float Nine = Health->MaxHealth * GetDefault<UProgressionSettings>()->GetLevelRules().EnemyScale(9)
			* UCreatureRankSettings::Get(ECreatureRank::Boss).HealthMultiplier;
		TestTrue(FString::Printf(TEXT("About 10,500 health at level 9 (%.0f)"), Nine), FMath::IsNearlyEqual(Nine, 10500.f, 300.f));
		TestEqual(TEXT("...40 Unpaid at level 1"), Health->MaxHealth, 160.f * 40.f);
	}
	TestTrue(TEXT("Boss rank, 1.3 times, a 6 cm coal, no tag"), Defaults->StartingRank == ECreatureRank::Boss
		&& FMath::IsNearlyEqual(Defaults->BodyScale, 1.3f) && FMath::IsNearlyEqual(Defaults->CoalCritRadius, 6.f) && !Defaults->bShowsHealthTag);
	TestTrue(TEXT("His fight starts by itself only once the lantern hangs, never from a hit"), Defaults->GetBoss()->EngageRadius == 0.f
		&& !Defaults->GetBoss()->bStartWhenHurt && Defaults->GetBoss()->BossId == AAbelKeeper::BossId);
	TestTrue(TEXT("He's in the world during Main 6, his fight from its third step, the ending at its fourth"),
		Defaults->PresentWhen.DuringMission == AAbelKeeper::Mission && Defaults->FightWhen.FromStep == AAbelKeeper::FightStep
		&& Defaults->EndingWhen.FromStep == AAbelKeeper::SceneStep);

	// The lanterns drag him back a third each.
	TestEqual(TEXT("None lit: out in the fog"), AbelRules::DragShare(0, 3), 0.f);
	TestNearlyEqual(TEXT("One: a third of the way"), AbelRules::DragShare(1, 3), 1.f / 3.f, 0.001f);
	TestEqual(TEXT("All three: home"), AbelRules::DragShare(3, 3), 1.f);

	// The Gravewind: still until its first gust, then gusts that rise and die on their schedule.
	const FAbelGustRules Gusts;
	TestEqual(TEXT("Still before the first gust"), AbelRules::GustStrength(Gusts, Gusts.FirstAfter - 0.1f), 0.f);
	TestNearlyEqual(TEXT("A gust at its height"), AbelRules::GustStrength(Gusts, Gusts.FirstAfter + Gusts.Seconds * 0.5f), 1.f, 0.001f);
	TestTrue(TEXT("...rising at its start"), AbelRules::GustStrength(Gusts, Gusts.FirstAfter + Gusts.Ramp * 0.5f) < 0.9f);
	TestEqual(TEXT("Still between gusts"), AbelRules::GustStrength(Gusts, Gusts.FirstAfter + Gusts.Seconds + 1.f), 0.f);
	int32 Count = 0;
	TestNearlyEqual(TEXT("The next gust comes on its schedule"), AbelRules::GustStrength(Gusts, Gusts.FirstAfter + Gusts.Every + Gusts.Seconds * 0.5f, &Count), 1.f, 0.001f);
	TestEqual(TEXT("...the second"), Count, 2);
	TestTrue(TEXT("A gust carries the player slower than they walk"), Gusts.Push > 100.f && Gusts.Push < 400.f);
	TestEqual(TEXT("Past the end the wind takes a falling player down"), AbelRules::FallSpeedPastEnd(Gusts, 0.0), -static_cast<double>(Gusts.Downdraft));
	TestEqual(TEXT("...never slowing a faster fall"), AbelRules::FallSpeedPastEnd(Gusts, -2000.0), -2000.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbelBodyTest, "Looter.Bosses.Abel.Body",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAbelBodyTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	AAbelKeeper* Abel = SpawnAbel(World);
	if (!TestNotNull(TEXT("Abel"), Abel))
	{
		return false;
	}
	// As he waits: Boss rank at 1.3, his coal the Boss rank's gold, unhurt by anything, no tag.
	TestTrue(TEXT("Boss rank at 1.3"), Abel->GetRank() == ECreatureRank::Boss && FMath::IsNearlyEqual(Abel->GetSizeScale(), 1.3f, 0.001f));
	TestTrue(TEXT("His coal is the Boss rank's gold"), Abel->GetCoalColor().Equals(UCreatureRankSettings::Get(ECreatureRank::Boss).Color));
	TestTrue(TEXT("Waiting, he hunts nobody and can't be hurt"), Abel->IsPassive() && Abel->FindComponentByClass<UHealthComponent>()->bInvulnerable);
	const float Before = Abel->FindComponentByClass<UHealthComponent>()->GetHealth();
	BossTestWorld::Hurt(Abel, 500.f);
	TestEqual(TEXT("...a shot takes nothing"), Abel->FindComponentByClass<UHealthComponent>()->GetHealth(), Before);
	TestFalse(TEXT("...nor starts his fight"), Abel->GetBoss()->IsFighting());

	// His light: one, shadowless, at his lantern.
	TestTrue(TEXT("His lantern's light casts no shadow"), Abel->LanternLight && !Abel->LanternLight->CastShadows);
	// It lights the deck round him, never him (a hand's width from his chest it blew his dark coat out white): it shines on
	// the ghost light's own lighting channel, which his body and props never take (they keep the sun's and sky's, 0).
	TestTrue(TEXT("...on the ghost light's lighting channel alone"), Abel->LanternLight && AbelRules::IsOnGhostChannel(*Abel->LanternLight));
	const TArray<const UPrimitiveComponent*> Worn = { Abel->GetMesh(), Abel->GetHat(), Abel->Lantern.Get(), Abel->Pump.Get() };
	for (const UPrimitiveComponent* Part : Worn)
	{
		TestTrue(FString::Printf(TEXT("...which never lights his %s, lit by the sun and sky"), Part ? *Part->GetName() : TEXT("(missing part)")),
			Part && Part->LightingChannels.bChannel0 && !AbelRules::DoesGhostLightReach(*Part));
	}
	for (const TObjectPtr<AKeeperLanternPost>& Post : Abel->GetLanternPosts())
	{
		TestTrue(TEXT("...and lights the lantern posts round him"), Post && Post->Post->LightingChannels.bChannel0 && AbelRules::DoesGhostLightReach(*Post->Post));
	}
	USkeletalMesh* Model = LoadModel();
	if (!Model || Abel->GetMesh()->GetSkeletalMeshAsset() != Model)
	{
		AddWarning(TEXT("SK_Abel isn't imported (or wasn't when the editor started): his body isn't checked."));
		return true;
	}
	// His props on their bones, his hat on his head; the light at the lantern's globe; the rig poses the bones the Unpaid lacks.
	TestTrue(TEXT("The lantern on the lantern bone"), Abel->Lantern->GetAttachSocketName() == FName(TEXT("lantern")) && Abel->Lantern->IsVisible());
	TestTrue(TEXT("The pump on the gun bone"), Abel->Pump->GetAttachSocketName() == FName(TEXT("gun")) && Abel->Pump->IsVisible());
	TestTrue(TEXT("The hat on the hat bone"), Abel->GetHat()->GetAttachSocketName() == FName(TEXT("hat")) && Abel->GetHat()->IsVisible());
	if (Abel->Lantern->DoesSocketExist(TEXT("Light")))
	{
		TestTrue(TEXT("The light at the lantern's globe"), Abel->LanternLight->GetAttachSocketName() == FName(TEXT("Light")));
	}
	TestTrue(TEXT("The pump has its muzzle"), Abel->Pump->DoesSocketExist(TEXT("Muzzle")));
	TArray<FName> Posed;
	for (const FCreatureBonePose& Bone : Abel->GetBonePose())
	{
		Posed.Add(Bone.Bone);
	}
	for (const FName Extra : AbelPoses::ExtraBones())
	{
		TestTrue(*FString::Printf(TEXT("His rig poses %s"), *Extra.ToString()), Posed.Contains(Extra));
	}

	// In his idle the lantern arm stands before the coal: a shot at it from the front meets the arm or the lantern first.
	USkeletalMeshComponent* Mesh = Abel->GetMesh();
	const FVector Coal = Abel->GetCoalLocation();
	const FVector Ahead = Abel->GetCoalFacing();
	const FCollisionQueryParams BulletQuery(SCENE_QUERY_STAT(AbelGuardTest), /*bTraceComplex*/ true);
	FHitResult Guard;
	if (Mesh->LineTraceComponent(Guard, Coal + Ahead * 500.0, Coal - Ahead * 50.0, BulletQuery))
	{
		const TArray<FName> Guarding = { TEXT("upperarm_l"), TEXT("lowerarm_l"), TEXT("hand_l"), TEXT("lantern") };
		TestTrue(FString::Printf(TEXT("His lantern arm guards the coal (the shot met %s)"), *Guard.BoneName.ToString()), Guarding.Contains(Guard.BoneName));
	}
	else
	{
		AddWarning(TEXT("No hit zones met at the coal (is PA_Abel his physics asset?)."));
	}

	// The coal is critical only while it's open: a shot on the chest whose line runs through the coal.
	FHitResult OnCoal;
	OnCoal.Component = Mesh;
	OnCoal.BoneName = TEXT("spine_02");
	OnCoal.ImpactPoint = Coal + Ahead * 6.0;
	OnCoal.TraceStart = Coal + Ahead * 400.0;
	OnCoal.TraceEnd = Coal - Ahead * 100.0;
	TestFalse(TEXT("Guarded, his coal isn't critical even shot dead center"), Abel->IsCriticalSpot(OnCoal));
	ACharacter* Player = nullptr;
	if (StartFight(*this, World, Abel, Player) && TestTrue(TEXT("He grieves"), Abel->Grieve()))
	{
		TestTrue(TEXT("Grieving, his coal is open"), Abel->IsCoalOpen());
		TestTrue(TEXT("...and critical"), Abel->IsCriticalSpot(OnCoal));
		// The whole shot 30 cm higher: the rule follows its line on from where it hit, so that moves too.
		FHitResult Wide = OnCoal;
		Wide.ImpactPoint = OnCoal.ImpactPoint + FVector(0.0, 0.0, 30.0);
		Wide.TraceStart = Coal + Ahead * 400.0 + FVector(0.0, 0.0, 30.0);
		Wide.TraceEnd = Coal - Ahead * 100.0 + FVector(0.0, 0.0, 30.0);
		TestFalse(TEXT("...but only by a line within its 6 cm (8 at his size)"), Abel->IsCriticalSpot(Wide));
		Run(Abel, Abel->GrieveSeconds + 0.2f);
		TestFalse(TEXT("His grief over, guarded again"), Abel->IsCoalOpen() || Abel->IsCriticalSpot(OnCoal));
	}

	// The bestiary's stand wears what he wears: his hat, lantern and pump.
	UBestiaryEntry* Entry = NewObject<UBestiaryEntry>();
	Entry->ActorClass = AAbelKeeper::StaticClass();
	const TArray<FBestiaryStandPart> Parts = Entry->GetPreviewParts(Model);
	TestEqual(TEXT("The stand wears his hat, lantern and pump"), Parts.Num(), 3);
	return true;
}

#endif

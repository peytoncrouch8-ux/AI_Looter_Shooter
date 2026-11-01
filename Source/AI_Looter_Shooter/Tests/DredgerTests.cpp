#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// Vane's Dredgers (Creatures/Dredger*): their rules (accuracy, distance, flanks, cover's worth, the burst's pace, the latch,
// the hose's breaks, the drain, the tank), their classes, their bodies spawned on a test floor, a rifleman finding cover
// behind a low wall, the soul-hose thrown, latched, draining and broken by range, and the tank bursting at its share.
// A test level never ticks: the tests move the hose on with Advance and call the searches themselves.

#include "Combat/HealthComponent.h"
#include "Creatures/DredgerBody.h"
#include "Creatures/DredgerBodyAnimInstance.h"
#include "Creatures/DredgerCreature.h"
#include "Creatures/DredgerHoseComponent.h"
#include "Creatures/DredgerHoseMan.h"
#include "Creatures/DredgerRifleman.h"
#include "Creatures/DredgerRules.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "Weapons/WeaponModelComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

namespace
{
	constexpr float DredgerStep = 1.f / 60.f;

	/** A floor 100 m across, its top at Z 0. */
	void SpawnDredgerFloor(UWorld* World)
	{
		LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(10000.0, 10000.0, 100.0));
	}

	template <typename TDredger>
	TDredger* SpawnDredger(UWorld* World, const FVector& Feet)
	{
		return Cast<TDredger>(EncounterTestWorld::SpawnCreature(World, TDredger::StaticClass(), Feet));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDredgerRulesTest, "Looter.Creatures.Dredger.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDredgerRulesTest::RunTest(const FString& Parameters)
{
	const FDredgerTuning Tuning;

	// Accuracy: the contract's 70% at 15 m, truer near, falling off far, and a crossing target harder to hit.
	TestEqual(TEXT("Its 15 m share at 15 m"), DredgerRules::HitChance(1500.f, 0.f, Tuning), Tuning.Accuracy, 0.001f);
	TestEqual(TEXT("Near: the near share"), DredgerRules::HitChance(500.f, 0.f, Tuning), Tuning.AccuracyNear, 0.001f);
	TestEqual(TEXT("Far: the far share"), DredgerRules::HitChance(3500.f, 0.f, Tuning), Tuning.AccuracyFar, 0.001f);
	TestTrue(TEXT("Falling off with distance"), DredgerRules::HitChance(1000.f, 0.f, Tuning) > DredgerRules::HitChance(1500.f, 0.f, Tuning)
		&& DredgerRules::HitChance(1500.f, 0.f, Tuning) > DredgerRules::HitChance(2200.f, 0.f, Tuning));
	TestEqual(TEXT("A sprint across costs the moving penalty"), DredgerRules::HitChance(1500.f, 600.f, Tuning), Tuning.Accuracy * (1.f - Tuning.MovingPenalty), 0.001f);
	const FVector Chest(1500.f, 0.f, 100.f);
	for (const float Side : { 0.1f, 0.6f, 0.95f })
	{
		const float Off = static_cast<float>(FVector::Dist(DredgerRules::MissPoint(Chest, FVector::ZeroVector, Side, 0.5f), Chest));
		TestTrue(FString::Printf(TEXT("A miss goes by close but clear (%.0f cm)"), Off), Off >= DredgerRules::MissNear * 0.75f && Off <= DredgerRules::MissFar * 1.1f);
	}

	// Distance: in from past the band (his brain), back off inside it, to his spot, then hold.
	TestEqual(TEXT("Far off: his brain's"), DredgerRules::ChooseKeep(3000.f, Tuning.BandNear, Tuning.BandFar, true, 500.f), EDredgerKeep::None);
	TestEqual(TEXT("Too close: back off"), DredgerRules::ChooseKeep(600.f, Tuning.BandNear, Tuning.BandFar, true, 500.f), EDredgerKeep::BackOff);
	TestEqual(TEXT("In his band: to his spot"), DredgerRules::ChooseKeep(1500.f, Tuning.BandNear, Tuning.BandFar, true, 500.f), EDredgerKeep::ToSpot);
	TestEqual(TEXT("...and there, hold"), DredgerRules::ChooseKeep(1500.f, Tuning.BandNear, Tuning.BandFar, true, 20.f), EDredgerKeep::Hold);

	// Flanks: pairs spread from both sides of the first.
	TestEqual(TEXT("The first takes the squad's bearing"), DredgerRules::FlankOffset(0, 40.f), 0.f);
	TestEqual(TEXT("The second flanks one way"), DredgerRules::FlankOffset(1, 40.f), 40.f);
	TestEqual(TEXT("The third the other"), DredgerRules::FlankOffset(2, 40.f), -40.f);
	TestEqual(TEXT("The fourth farther round"), DredgerRules::FlankOffset(3, 40.f), 80.f);

	// Cover's worth: covered beats open, a taken spot loses, no shot at all is no spot.
	FDredgerSpotFacts Covered;
	Covered.ToTarget = Tuning.Preferred;
	Covered.bCovered = true;
	Covered.bCanShoot = true;
	FDredgerSpotFacts Open = Covered;
	Open.bCovered = false;
	FDredgerSpotFacts Taken = Covered;
	Taken.bClaimed = true;
	FDredgerSpotFacts Blind = Covered;
	Blind.bCanShoot = false;
	TestTrue(TEXT("Cover beats the open"), DredgerRules::ScoreSpot(Covered, Tuning) < DredgerRules::ScoreSpot(Open, Tuning));
	TestTrue(TEXT("Cover five metres off his liking still beats the open"),
		[&]() { FDredgerSpotFacts Off = Covered; Off.ToTarget += 500.f; return DredgerRules::ScoreSpot(Off, Tuning) < DredgerRules::ScoreSpot(Open, Tuning); }());
	TestTrue(TEXT("Another's spot is worse"), DredgerRules::ScoreSpot(Taken, Tuning) > DredgerRules::ScoreSpot(Covered, Tuning));
	TestEqual(TEXT("No shot: no spot"), DredgerRules::ScoreSpot(Blind, Tuning), TNumericLimits<float>::Max());
	const TArray<FVector2D> Candidates = DredgerRules::SpotCandidates(30.f, Tuning);
	TestEqual(TEXT("Fifteen candidates"), Candidates.Num(), 15);
	TestTrue(TEXT("All in his band"), Candidates.FindByPredicate([&](const FVector2D& C) { return C.X < Tuning.BandNear || C.X > Tuning.BandFar; }) == nullptr);

	// The burst's pace: its tell, its rounds and its rest make the contract's 2.2 s.
	TestEqual(TEXT("A burst every BurstPeriod"), Tuning.RifleWindup + DredgerRules::BurstRecovery(Tuning) + DredgerRules::BurstCooldown(Tuning), Tuning.BurstPeriod, 0.01f);

	// The latch: within 8 m, a clear line, rested; never point-blank.
	TestTrue(TEXT("6 m, clear, rested: throws"), DredgerRules::CanLatch(600.f, true, 0.f, Tuning));
	TestFalse(TEXT("9 m: no"), DredgerRules::CanLatch(900.f, true, 0.f, Tuning));
	TestFalse(TEXT("Blocked: no"), DredgerRules::CanLatch(600.f, false, 0.f, Tuning));
	TestFalse(TEXT("Resting: no"), DredgerRules::CanLatch(600.f, true, 1.f, Tuning));
	TestFalse(TEXT("1 m: the club's"), DredgerRules::CanLatch(100.f, true, 0.f, Tuning));

	// The breaks, first that fits.
	FDredgerHoseFacts Holding;
	Holding.Distance = 600.f;
	TestEqual(TEXT("Holding"), DredgerRules::HoseBreak(Holding, Tuning), EDredgerHoseBreak::None);
	FDredgerHoseFacts Far = Holding;
	Far.Distance = 1450.f;
	TestEqual(TEXT("14 m: broken"), DredgerRules::HoseBreak(Far, Tuning), EDredgerHoseBreak::Range);
	FDredgerHoseFacts Held = Holding;
	Held.bOwnerHeld = true;
	TestEqual(TEXT("Staggered: broken"), DredgerRules::HoseBreak(Held, Tuning), EDredgerHoseBreak::Stagger);
	FDredgerHoseFacts Burst = Held;
	Burst.bTankBurst = true;
	TestEqual(TEXT("His tank before his stagger"), DredgerRules::HoseBreak(Burst, Tuning), EDredgerHoseBreak::TankBurst);
	FDredgerHoseFacts Cut = Holding;
	Cut.BlockedFor = Tuning.BlockedBreak + 0.05f;
	TestEqual(TEXT("Cut by the world a moment: broken"), DredgerRules::HoseBreak(Cut, Tuning), EDredgerHoseBreak::Blocked);
	FDredgerHoseFacts Long = Holding;
	Long.LatchedFor = Tuning.LatchMaxSeconds + 0.1f;
	TestEqual(TEXT("Held too long: lets go"), DredgerRules::HoseBreak(Long, Tuning), EDredgerHoseBreak::TooLong);

	// The drain, the pull, the tank.
	TestEqual(TEXT("5 HP/s at level 1"), DredgerRules::DrainPerSecond(10.f, 10.f, Tuning), 5.f, 0.001f);
	TestEqual(TEXT("...grown as his damage grew"), DredgerRules::DrainPerSecond(20.f, 10.f, Tuning), 10.f, 0.001f);
	TestEqual(TEXT("No pull in his face"), DredgerRules::PullInput(100.f, Tuning), 0.f, 0.001f);
	TestEqual(TEXT("The pull at range"), DredgerRules::PullInput(800.f, Tuning), Tuning.PullShare, 0.001f);
	TestTrue(TEXT("The tank bursts at 30%"), DredgerRules::TankBursts(66.f, 220.f, Tuning));
	TestFalse(TEXT("...not before"), DredgerRules::TankBursts(60.f, 220.f, Tuning));

	// The body's blends.
	int32 A = 0;
	int32 B = 0;
	float Share = 0.f;
	DredgerBody::WayBlend(100.f, A, B, Share);
	TestTrue(TEXT("100 degrees: right, a little back-right"), A == 2 && B == 3 && FMath::IsNearlyEqual(Share, 10.f / 45.f, 0.01f));
	DredgerBody::WayBlend(-45.f, A, B, Share);
	TestTrue(TEXT("-45 degrees: ahead-left"), A == 7 && B == 0 && Share < 0.01f);
	float Idle = 0.f;
	float Walk = 0.f;
	float Jog = 0.f;
	DredgerBody::GaitShares(0.f, Tuning.WalkClipSpeed, Tuning.JogClipSpeed, Idle, Walk, Jog);
	TestEqual(TEXT("Standing: the idle"), Idle, 1.f, 0.001f);
	DredgerBody::GaitShares(Tuning.WalkClipSpeed, Tuning.WalkClipSpeed, Tuning.JogClipSpeed, Idle, Walk, Jog);
	TestTrue(TEXT("At the walk's speed: the walk"), Walk > 0.99f && Jog < 0.01f);
	DredgerBody::GaitShares(Tuning.JogClipSpeed, Tuning.WalkClipSpeed, Tuning.JogClipSpeed, Idle, Walk, Jog);
	TestTrue(TEXT("At the jog's speed: the jog"), Jog > 0.99f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDredgerClassesTest, "Looter.Creatures.Dredger.Classes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDredgerClassesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("A rifleman's class"), ADredgerCreature::ClassFor(EDredgerRole::Rifleman) == ADredgerRifleman::StaticClass());
	TestTrue(TEXT("A hose-man's class"), ADredgerCreature::ClassFor(EDredgerRole::HoseMan) == ADredgerHoseMan::StaticClass());
	const ADredgerRifleman* Rifleman = GetDefault<ADredgerRifleman>();
	const ADredgerHoseMan* HoseMan = GetDefault<ADredgerHoseMan>();
	TestEqual(TEXT("The rifleman's job"), Rifleman->DredgerRole, EDredgerRole::Rifleman);
	TestEqual(TEXT("The hose-man's job"), HoseMan->DredgerRole, EDredgerRole::HoseMan);
	const UHealthComponent* RiflemanHealth = Rifleman->FindComponentByClass<UHealthComponent>();
	const UHealthComponent* HoseManHealth = HoseMan->FindComponentByClass<UHealthComponent>();
	if (TestNotNull(TEXT("The rifleman's health"), RiflemanHealth) && TestNotNull(TEXT("The hose-man's health"), HoseManHealth))
	{
		TestEqual(TEXT("Rifleman 140 HP"), RiflemanHealth->MaxHealth, 140.f);
		TestEqual(TEXT("Hose-man 220 HP"), HoseManHealth->MaxHealth, 220.f);
	}
	TestEqual(TEXT("7 a bullet"), Rifleman->AttackDamage, 7.f);
	TestTrue(TEXT("The rifleman fights at range"), Rifleman->GetFightRole() == EFightRole::Ranged);
	TestTrue(TEXT("The hose-man takes melee turns"), HoseMan->GetFightRole() == EFightRole::Melee);
	TestTrue(TEXT("The mask is the crit"), Rifleman->CriticalSpotBones.Contains(FName(TEXT("head"))));
	TestEqual(TEXT("Their pack"), Rifleman->PackTag, FName(TEXT("Dredger")));
	TestFalse(TEXT("Never Touched"), Rifleman->bMayBeTouched);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDredgerSpawnTest, "Looter.Creatures.Dredger.Spawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDredgerSpawnTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	SpawnDredgerFloor(World);
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 0.0, 89.0));
	ADredgerRifleman* Rifleman = SpawnDredger<ADredgerRifleman>(World, FVector(1500.0, 0.0, 0.0));
	ADredgerHoseMan* HoseMan = SpawnDredger<ADredgerHoseMan>(World, FVector(700.0, 400.0, 0.0));
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("A rifleman"), Rifleman) || !TestNotNull(TEXT("A hose-man"), HoseMan))
	{
		return false;
	}
	for (ADredgerCreature* Dredger : { static_cast<ADredgerCreature*>(Rifleman), static_cast<ADredgerCreature*>(HoseMan) })
	{
		const USkeletalMeshComponent* Body = Dredger->GetMesh();
		TestTrue(FString::Printf(TEXT("%s: begun"), *Dredger->GetName()), Dredger->HasActorBegunPlay());
		TestNotNull(FString::Printf(TEXT("%s: its body"), *Dredger->GetName()), Body->GetSkeletalMeshAsset());
		TestTrue(FString::Printf(TEXT("%s: animated by the Dredger body"), *Dredger->GetName()), const_cast<USkeletalMeshComponent*>(Body)->GetAnimClass() == UDredgerBodyAnimInstance::StaticClass());
		TestTrue(FString::Printf(TEXT("%s: its body takes shots"), *Dredger->GetName()), Body->GetCollisionResponseToChannel(ECC_GameTraceChannel2) == ECR_Block);
		TestTrue(FString::Printf(TEXT("%s: its capsule doesn't"), *Dredger->GetName()),
			Dredger->GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_GameTraceChannel2) == ECR_Ignore);
		TestTrue(FString::Printf(TEXT("%s: stands on the floor"), *Dredger->GetName()),
			FMath::Abs(static_cast<float>(Dredger->GetActorLocation().Z - Dredger->GetCapsuleComponent()->GetScaledCapsuleHalfHeight())) < 30.f);
		FHitResult Head;
		Head.Component = const_cast<USkeletalMeshComponent*>(Body);
		Head.BoneName = TEXT("head");
		TestTrue(FString::Printf(TEXT("%s: a head shot is a crit"), *Dredger->GetName()), Dredger->IsCriticalSpot(Head));
		Head.BoneName = TEXT("spine_03");
		TestFalse(FString::Printf(TEXT("%s: a chest shot isn't"), *Dredger->GetName()), Dredger->IsCriticalSpot(Head));
	}
	TestTrue(TEXT("The rifleman holds the company rifle"), Rifleman->GetRifle() && Rifleman->GetRifle()->IsAssembled());

	// The hose-man's tank takes shots and is his crit.
	UStaticMeshComponent* Tank = HoseMan->GetTank();
	TestTrue(TEXT("The tank takes shots"), Tank->GetCollisionResponseToChannel(ECC_GameTraceChannel2) == ECR_Block);
	FHitResult OnTank;
	OnTank.Component = Tank;
	TestTrue(TEXT("A tank hit is a crit"), HoseMan->IsCriticalSpot(OnTank));

	// A frame on, his body's input is a rifleman's: aiming with the torso, the left hand on the rifle.
	Rifleman->AlertTo(Player);
	Rifleman->Tick(DredgerStep);
	const FDredgerBodyInput& Input = Rifleman->GetBodyInput();
	TestTrue(TEXT("Aims with the torso"), Input.Stance.bAimWithTorso);
	TestTrue(TEXT("Faces somewhere"), !Input.Stance.Forward.IsNearlyZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDredgerCoverTest, "Looter.Creatures.Dredger.Cover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDredgerCoverTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	SpawnDredgerFloor(World);
	// A low wall 1.1 m high, 2 m short of where he stands, between him and the player 16 m off.
	LocomotionTestWorld::SpawnBlock(World, FVector(1385.0, 0.0, 55.0), FVector(70.0, 300.0, 110.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 0.0, 89.0));
	ADredgerRifleman* Rifleman = SpawnDredger<ADredgerRifleman>(World, FVector(1600.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("A rifleman"), Rifleman))
	{
		return false;
	}
	Rifleman->AlertTo(Player);
	if (!TestTrue(TEXT("He hunts the player"), Rifleman->GetTarget() == Player))
	{
		return false;
	}
	FVector Spot;
	bool bCovered = false;
	TestTrue(TEXT("He finds a spot"), Rifleman->DevFindSpot() && Rifleman->GetSpot(Spot, bCovered));
	TestTrue(TEXT("...behind the wall"), bCovered);
	TestTrue(FString::Printf(TEXT("...the wall between him and the player (%s)"), *Spot.ToCompactString()), Spot.X > 1420.0 && FMath::Abs(Spot.Y) < 400.0);

	// A second rifleman off to the side: the one cover there is taken, so his best is a spot of his own in the open.
	ADredgerRifleman* Second = SpawnDredger<ADredgerRifleman>(World, FVector(0.0, -1600.0, 0.0));
	if (!TestNotNull(TEXT("A second rifleman"), Second))
	{
		return false;
	}
	Second->AlertTo(Player);
	FVector SecondSpot;
	bool bSecondCovered = true;
	TestTrue(TEXT("The second finds a spot"), Second->DevFindSpot() && Second->GetSpot(SecondSpot, bSecondCovered));
	TestFalse(TEXT("...in the open (the wall's is the first's)"), bSecondCovered);
	TestTrue(TEXT("...not on top of the first's"), FVector::Dist2D(SecondSpot, Spot) > DredgerRules::ClaimRadius);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDredgerPostTest, "Looter.Creatures.Dredger.Post",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDredgerPostTest::RunTest(const FString& Parameters)
{
	// A checkpoint's rifleman at his post (FEncounterGroup::PostRadius; the town gate's, play check 2026-11-01): the player
	// out in the field past his band, a low wall in front of his post. He keeps his post's cover rather than walk out, his
	// brain never walks him in, and every goal stops at the post's edge; let go, he is as before.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	SpawnDredgerFloor(World);
	LocomotionTestWorld::SpawnBlock(World, FVector(2685.0, 0.0, 55.0), FVector(70.0, 600.0, 110.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 0.0, 89.0));
	ADredgerRifleman* Rifleman = SpawnDredger<ADredgerRifleman>(World, FVector(2900.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("A rifleman"), Rifleman))
	{
		return false;
	}
	const float ClassRange = Rifleman->GetAttackRange();
	constexpr float Post = 600.f;
	Rifleman->SetPost(Post);
	TestTrue(TEXT("At his post his brain never walks him in (his chase holds past any fight he sees)"),
		Rifleman->GetAttackRange() * 0.75f > 6000.f);
	const FVector Home = Rifleman->GetHome().GetLocation();
	const FVector Kept = Rifleman->KeepToPost(Player->GetActorLocation());
	TestTrue(FString::Printf(TEXT("A goal out in the field stops at his post's edge (%s)"), *Kept.ToCompactString()),
		FMath::IsNearlyEqual(FVector::Dist2D(Kept, Home), static_cast<double>(Post), 1.0) && Kept.X < Home.X);
	TestTrue(TEXT("...one inside it is his to walk to"), Rifleman->KeepToPost(Home + FVector(200.0, 100.0, 0.0)).Equals(Home + FVector(200.0, 100.0, 0.0)));

	Rifleman->AlertTo(Player);
	if (!TestTrue(TEXT("He hunts the player"), Rifleman->GetTarget() == Player))
	{
		return false;
	}
	FVector Spot;
	bool bCovered = false;
	TestTrue(TEXT("He finds a spot"), Rifleman->DevFindSpot() && Rifleman->GetSpot(Spot, bCovered));
	TestTrue(FString::Printf(TEXT("...in his post (%s)"), *Spot.ToCompactString()), FVector::Dist2D(Spot, Home) <= Post + 1.0);
	TestTrue(TEXT("...behind its wall, not in the open at its edge"), bCovered && Spot.X > 2720.0);

	Rifleman->SetPost(0.f);
	TestEqual(TEXT("Let go: his own range again"), Rifleman->GetAttackRange(), ClassRange, 0.5f);
	TestTrue(TEXT("...and no post"), Rifleman->IsWithinPost(FVector(-1.0e5, 0.0, 0.0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDredgerLatchTest, "Looter.Creatures.Dredger.Latch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDredgerLatchTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	SpawnDredgerFloor(World);
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(600.0, 0.0, 89.0));
	ADredgerHoseMan* HoseMan = SpawnDredger<ADredgerHoseMan>(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("A hose-man"), HoseMan))
	{
		return false;
	}
	HoseMan->AlertTo(Player);
	UDredgerHoseComponent* Hose = HoseMan->GetHose();
	if (!TestTrue(TEXT("He throws his line"), Hose->Fire(Player)))
	{
		return false;
	}
	TestFalse(TEXT("Not twice at once"), Hose->Fire(Player));
	for (float Clock = 0.f; Clock < 1.f && !Hose->IsLatched(); Clock += DredgerStep)
	{
		Hose->Advance(DredgerStep);
	}
	if (!TestTrue(TEXT("It latches at 6 m"), Hose->IsLatched()))
	{
		return false;
	}

	// Two seconds latched: the drain at his rate (counted, though a test level deals no damage).
	for (float Clock = 0.f; Clock < 2.f; Clock += DredgerStep)
	{
		Hose->Advance(DredgerStep);
	}
	const float Rate = HoseMan->GetDrainPerSecond();
	TestTrue(FString::Printf(TEXT("Drained about %.1f in 2 s (%.1f)"), Rate * 2.f, Hose->GetDrainDealt()),
		Hose->GetDrainDealt() > Rate * 1.5f && Hose->GetDrainDealt() <= Rate * 2.05f);
	TestTrue(TEXT("Still latched"), Hose->IsLatched());
	TestTrue(TEXT("The HUD can see the drain"), UDredgerHoseComponent::GetDrainOn(Player) > 0.5f);

	// The player gets 15 m away: it breaks, and whips back.
	Player->SetActorLocation(FVector(1500.0, 0.0, 89.0));
	Hose->Advance(DredgerStep);
	TestEqual(TEXT("Broken by range"), Hose->GetLastBreak(), EDredgerHoseBreak::Range);
	TestFalse(TEXT("No longer latched"), Hose->IsLatched());
	for (float Clock = 0.f; Clock < 0.5f; Clock += DredgerStep)
	{
		Hose->Advance(DredgerStep);
	}
	TestEqual(TEXT("Back on the nozzle"), Hose->GetState(), EDredgerHoseState::Stowed);
	TestTrue(TEXT("He rests before the next throw"), HoseMan->GetLatchRestLeft() > 0.f);

	// Thrown out of reach, it misses.
	Player->SetActorLocation(FVector(2500.0, 0.0, 89.0));
	TestTrue(TEXT("Thrown again"), Hose->Fire(Player));
	for (float Clock = 0.f; Clock < 1.5f && Hose->IsOut(); Clock += DredgerStep)
	{
		Hose->Advance(DredgerStep);
	}
	TestEqual(TEXT("Out of reach: a miss"), Hose->GetLastBreak(), EDredgerHoseBreak::Missed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDredgerTankTest, "Looter.Creatures.Dredger.Tank",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDredgerTankTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	SpawnDredgerFloor(World);
	ADredgerHoseMan* HoseMan = SpawnDredger<ADredgerHoseMan>(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("A hose-man"), HoseMan))
	{
		return false;
	}
	const float MaxHealth = HoseMan->FindComponentByClass<UHealthComponent>()->GetMaxHealth();
	HoseMan->NoteTankHit(MaxHealth * 0.2f, nullptr);
	TestFalse(TEXT("A fifth of his health on it: holding"), HoseMan->IsTankBurst());
	HoseMan->NoteTankHit(MaxHealth * 0.11f, nullptr);
	TestTrue(TEXT("Past 30%: it bursts"), HoseMan->IsTankBurst());
	TestFalse(TEXT("The tank is gone"), HoseMan->GetTank()->IsVisible());
	FHitResult OnTank;
	OnTank.Component = HoseMan->GetTank();
	TestFalse(TEXT("...and no crit spot now"), HoseMan->IsCriticalSpot(OnTank));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(600.0, 0.0, 89.0));
	TestFalse(TEXT("No line to throw"), HoseMan->GetHose()->Fire(Player));
	TestTrue(TEXT("He fights on up close"), HoseMan->AttackRange < 300.f);
	return true;
}

#endif

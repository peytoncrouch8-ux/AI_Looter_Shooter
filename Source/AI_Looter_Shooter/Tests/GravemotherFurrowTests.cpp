#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The Gravemother's Furrow (the user's call, 2026-10-19: "a 2 s tell: the ground heaves and splits in a line toward the
// player, dust jetting from the crack; then she ploughs the line head-low and throws up a berm each side (cover for 20 s,
// then they crumble)"; GravemotherCreatureFurrow.cpp, AGravemotherFurrow).

#include "Bosses/BossComponent.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/GravemotherFurrow.h"
#include "Tests/BossTestWorld.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/GravemotherTestWorld.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"

using namespace GravemotherTestWorld;

namespace
{
	/** Ticks her through her tell until Done, Clock counting (at most Limit s). */
	void RunTell(AGravemotherCreature* Mother, float& Clock, float Until)
	{
		while (Clock + Step * 0.5f < Until && Mother->GetFurrowState() == EGravemotherFurrow::Heave)
		{
			Mother->Tick(Step);
			Clock += Step;
		}
	}

	/** Carries her down her line as her movement would (a test level doesn't move her) until the plough ends. */
	void CarryDownLine(AGravemotherCreature* Mother)
	{
		const FVector Start = Mother->GetFurrowStart();
		const FVector Line = Mother->GetFurrowDirection();
		const float Speed = Mother->GetPloughSpeed();
		float Covered = 0.f;
		while (Covered < Mother->GetFurrowLength() + Speed * Step && Mother->GetFurrowState() == EGravemotherFurrow::Plough)
		{
			Covered = FMath::Min(Covered + Speed * Step, Mother->GetFurrowLength());
			Mother->SetActorLocation(Start + Line * Covered);
			Mother->Tick(Step);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherFurrowLineTest, "Looter.Creatures.Gravemother.Furrow.Line",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherFurrowLineTest::RunTest(const FString& Parameters)
{
	// With her target 5 to 16 m off and her way clear she digs in and tracks them, then holds her aim while the ground heaves
	// and splits along her line (all of it by 1.8 s); only at the 2 s tell's end she ploughs straight down it, running down
	// whoever is in it once, berms rising behind her, and slams down at its end; then it cools down and she bites.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(1000.0, 0.0, 0.0));
	AGravemotherCreature* Mother = SpawnMother(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("The Gravemother"), Mother))
	{
		return false;
	}
	TestTrue(TEXT("The user's 2 s tell, her aim taken early in it"), Mother->Furrows.Tell >= 2.f && Mother->Furrows.AimAt < Mother->Furrows.CrawlEnd
		&& Mother->Furrows.CrawlEnd < Mother->Furrows.Tell);
	TestFalse(TEXT("She sizes up whoever she first sees before she ploughs"), Mother->IsFurrowReady());
	Mother->AlertTo(Player);
	Mother->ReadyFurrow();
	Mother->Tick(Step);
	TestTrue(TEXT("10 m off, the furrow ready: she digs in"), Mother->GetCreatureState() == ECreatureState::Attack
		&& Mother->GetFurrowState() == EGravemotherFurrow::Heave);
	TestNearlyEqual(TEXT("The attack's wind-up is the tell"), Mother->AttackWindup, Mother->Furrows.Tell, 0.001f);

	float Clock = 0.f;
	RunTell(Mother, Clock, 0.2f);
	Player->SetActorLocation(FVector(1000.0, 300.0, 0.0));
	RunTell(Mother, Clock, Mother->Furrows.AimAt - 0.1f);
	TestNull(TEXT("Nothing heaves before she takes her aim"), Mother->GetFurrow());
	const double Toward = FMath::RadiansToDegrees(FMath::Atan2(300.0, 1000.0));
	TestNearlyEqual(TEXT("She turns to follow her target as she digs in"), Mother->GetActorRotation().Yaw, Toward, 3.0);
	RunTell(Mother, Clock, Mother->Furrows.AimAt + 0.1f);
	TestTrue(TEXT("Her aim taken"), Mother->HasTakenAim());
	const FVector Line = Mother->GetFurrowDirection();
	TestNearlyEqual(TEXT("...at where they stood"), Line.Rotation().Yaw, Toward, 0.5);
	const float Scale = Mother->GetSizeScale();
	const float Distance = static_cast<float>(FVector::Dist2D(Mother->GetActorLocation(), Player->GetActorLocation()));
	TestNearlyEqual(TEXT("...running on past them"), Mother->GetFurrowLength(), Distance + Mother->Furrows.Overshoot * Scale, 1.f);
	AGravemotherFurrow* Furrow = Mother->GetFurrow();
	if (!TestNotNull(TEXT("The ground heaves along her line"), Furrow))
	{
		return false;
	}
	const FVector Feet = FeetOf(*Mother);
	const float Front = Mother->GetCapsuleComponent()->GetScaledCapsuleRadius();
	TestTrue(TEXT("...from under her forelegs along it"), Furrow->GetStart().Equals(Feet + Line * Front, 1.0)
		&& Furrow->GetDirection().Equals(Line, 1e-3));
	Player->SetActorLocation(FVector(1000.0, -400.0, 0.0));
	RunTell(Mother, Clock, 1.6f);
	TestTrue(*FString::Printf(TEXT("By 1.6 s the split has crawled most of the way (%.0f of %.0f cm)"), Mother->GetHeaveFront(), Mother->GetFurrowLength()),
		Mother->GetHeaveFront() >= Mother->GetFurrowLength() * 0.8f && Furrow->GetFront() == Mother->GetHeaveFront());
	RunTell(Mother, Clock, Mother->Furrows.Tell - 0.1f);
	TestNearlyEqual(TEXT("Her aim held when they moved"), Mother->GetActorRotation().Yaw, Toward, 0.5);
	TestNearlyEqual(TEXT("...all of the line split by now"), Mother->GetHeaveFront(), Mother->GetFurrowLength(), 1.f);
	TestTrue(TEXT("Still the tell: no plough before its end"), Mother->GetFurrowState() == EGravemotherFurrow::Heave && !Furrow->IsPloughed());
	TestFalse(TEXT("Nobody run down during the tell"), Mother->HasFurrowHit());

	// The plough, once the tell is over: full tilt down the line.
	while (Clock < 3.f && Mother->GetFurrowState() == EGravemotherFurrow::Heave)
	{
		Mother->Tick(Step);
		Clock += Step;
	}
	TestTrue(*FString::Printf(TEXT("She ploughs at the tell's end (%.2f s)"), Clock), Mother->GetFurrowState() == EGravemotherFurrow::Plough
		&& FMath::Abs(Clock - Mother->Furrows.Tell) <= Step * 1.5f);
	TestTrue(TEXT("...straight down her line at full speed"), Mother->GetVelocity().GetSafeNormal2D().Equals(Line, 1e-3)
		&& FMath::IsNearlyEqual(static_cast<float>(Mother->GetVelocity().Size2D()), Mother->GetPloughSpeed(), 1.f));

	// Someone standing in her line 6 m on is run down as she passes, once.
	const FVector Start = Mother->GetFurrowStart();
	Player->SetActorLocation(FVector(Start.X + Line.X * 600.0, Start.Y + Line.Y * 600.0, 0.0));
	CarryDownLine(Mother);
	TestTrue(TEXT("Run down as she passed"), Mother->HasFurrowHit());
	TestTrue(TEXT("At the line's end she slams down: her recovery"), Mother->GetFurrowState() == EGravemotherFurrow::Recover);
	TestTrue(*FString::Printf(TEXT("...berms risen each side behind her (%d)"), Furrow->NumBermsRisen()), Furrow->NumBermsRisen() > 0
		&& Furrow->IsStanding());
	for (int32 Frame = 0; Frame < 100 && Mother->GetFurrowState() != EGravemotherFurrow::None; ++Frame)
	{
		Mother->Tick(Step);
	}
	TestTrue(TEXT("Up again: the furrow is over"), Mother->GetFurrowState() == EGravemotherFurrow::None
		&& Mother->GetCreatureState() != ECreatureState::Attack);
	TestFalse(TEXT("...and cooling down"), Mother->IsFurrowReady());
	TestTrue(TEXT("...its ground let go (its berms stand their time)"), Mother->GetFurrow() == nullptr && IsValid(Furrow) && Furrow->IsStanding());
	TestNearlyEqual(TEXT("...her bite's timing back"), Mother->AttackWindup, GetDefault<AGravemotherCreature>()->AttackWindup, 0.001f);

	// Up close (inside the furrow's least distance) she bites, furrow ready or not.
	ACharacter* Near = EncounterTestWorld::SpawnPlayer(World, FVector(300.0, 6000.0, 0.0));
	AGravemotherCreature* Biter = SpawnMother(World, FVector(0.0, 6000.0, 0.0));
	if (TestNotNull(TEXT("Second player stand-in"), Near) && TestNotNull(TEXT("Second Gravemother"), Biter))
	{
		Biter->AlertTo(Near);
		Biter->ReadyFurrow();
		Biter->Tick(Step);
		TestTrue(TEXT("3 m off: a bite, not a furrow"), Biter->GetCreatureState() == ECreatureState::Attack
			&& Biter->GetFurrowState() == EGravemotherFurrow::None);
	}

	// A wall between them: no furrow through it.
	ACharacter* Walled = EncounterTestWorld::SpawnPlayer(World, FVector(1200.0, -6000.0, 0.0));
	AGravemotherCreature* Blocked = SpawnMother(World, FVector(0.0, -6000.0, 0.0));
	EncounterTestWorld::SpawnBlocks(World, { FBox(FVector(700.0, -6200.0, -100.0), FVector(740.0, -5800.0, 400.0)) });
	if (TestNotNull(TEXT("Third stand-in"), Walled) && TestNotNull(TEXT("Third Gravemother"), Blocked))
	{
		Blocked->AlertTo(Walled);
		Blocked->ReadyFurrow();
		Blocked->Tick(Step);
		TestTrue(TEXT("A wall between them: she doesn't plough through it"), Blocked->GetFurrowState() == EGravemotherFurrow::None);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherFurrowBermsTest, "Looter.Creatures.Gravemother.Furrow.Berms",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherFurrowBermsTest::RunTest(const FString& Parameters)
{
	// Her plough throws up a berm each side every metre of its line, BermOffset either side, cover (bullets, walking) for 20 s,
	// then they crumble and the furrow is gone. A tell broken off leaves no berms and its split fades.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(1200.0, 0.0, 0.0));
	AGravemotherCreature* Mother = SpawnMother(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("The Gravemother"), Mother))
	{
		return false;
	}
	Mother->AlertTo(Player);
	Mother->ReadyFurrow();
	const float PloughAt = TickUntil(Mother, 3.f, [Mother]() { return Mother->GetFurrowState() == EGravemotherFurrow::Plough; });
	AGravemotherFurrow* Furrow = Mother->GetFurrow();
	if (!TestTrue(TEXT("She ploughs"), PloughAt >= 0.f) || !TestNotNull(TEXT("...her furrow"), Furrow))
	{
		return false;
	}
	// The player off her line: nobody run down.
	Player->SetActorLocation(FVector(0.0, 3000.0, 0.0));
	CarryDownLine(Mother);
	const int32 Spots = Furrow->NumBermSpots();
	const int32 Expected = 2 * (FMath::FloorToInt32((Furrow->GetLength() + 1.f) / Mother->Furrows.BermEvery) + 1);
	TestEqual(TEXT("A berm spot each side every metre of the line"), Spots, Expected);
	// Every spot her middle got past rises; the last one or two under her where she stopped stay down.
	const float Passed = Furrow->GetCovered() - Mother->Furrows.RearLag;
	const int32 Risen = FMath::Min(Spots, 2 * (FMath::FloorToInt32(Passed / Mother->Furrows.BermEvery) + 1));
	TestTrue(*FString::Printf(TEXT("Every berm she got past risen (%d of %d spots)"), Furrow->NumBermsRisen(), Spots),
		Furrow->NumBermsRisen() == Risen && Risen >= Spots - 4);
	bool bBeside = true;
	for (const FVector& Spot : Furrow->GetRisenBermSpots())
	{
		const FVector Along = Spot - Furrow->GetStart();
		const FVector Across(-Furrow->GetDirection().Y, Furrow->GetDirection().X, 0.0);
		bBeside &= FMath::IsNearlyEqual(FMath::Abs(static_cast<float>(FVector::DotProduct(Along, Across))), Mother->Furrows.BermOffset, 2.f);
	}
	TestTrue(TEXT("...each BermOffset beside the line"), bBeside);
	UInstancedStaticMeshComponent* Berms = Furrow->GetBermInstances();
	TestTrue(TEXT("Cover: they stop bullets and walkers"), Berms && Berms->GetInstanceCount() == Risen
		&& Berms->GetCollisionResponseToChannel(ECC_GameTraceChannel2) == ECR_Block && Berms->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block
		&& Berms->GetCollisionEnabled() != ECollisionEnabled::NoCollision);
	Furrow->Advance(Mother->Furrows.BermSeconds * 0.5f);
	TestTrue(TEXT("Ten seconds on they still stand"), Furrow->IsStanding());
	Furrow->Advance(Mother->Furrows.BermSeconds * 0.5f + Step);
	TestTrue(TEXT("At 20 s they crumble: cover no more"), Furrow->IsCrumbling() && Berms->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	const TWeakObjectPtr<AGravemotherFurrow> Kept = Furrow;
	Furrow->Advance(Mother->Furrows.BermCrumbleSeconds + Step);
	TestTrue(TEXT("...and it's gone"), !Kept.IsValid() || Kept->IsActorBeingDestroyed());

	// A tell broken off (her stagger) before the plough: the split fades, no berms.
	ACharacter* Other = EncounterTestWorld::SpawnPlayer(World, FVector(1000.0, 8000.0, 0.0));
	AGravemotherCreature* Broken = SpawnMother(World, FVector(0.0, 8000.0, 0.0));
	if (TestNotNull(TEXT("Second stand-in"), Other) && TestNotNull(TEXT("Second Gravemother"), Broken))
	{
		Broken->AlertTo(Other);
		Broken->ReadyFurrow();
		TickFor(Broken, Broken->Furrows.AimAt + 0.3f);
		AGravemotherFurrow* Cut = Broken->GetFurrow();
		TestNotNull(TEXT("Her line heaving"), Cut);
		UBossComponent* Boss = Broken->GetBoss();
		Boss->StartFight(Other);
		TestTrue(TEXT("Staggered in her tell"), Boss->BeginStagger() && Broken->IsReeling());
		TickFor(Broken, 0.2f);
		TestTrue(TEXT("...the tell is broken off: no plough"), Broken->GetFurrowState() == EGravemotherFurrow::None);
		TestTrue(TEXT("...its split fades, with no berms"), Cut && Cut->IsCancelled() && Cut->NumBermsRisen() == 0);
	}
	return true;
}

#endif

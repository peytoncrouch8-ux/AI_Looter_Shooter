#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/LooterDamageTypes.h"
#include "Combat/MovementSlowComponent.h"
#include "Creatures/CreatureKnockComponent.h"
#include "Embers/Powers/EmberIronHide.h"
#include "Embers/Powers/EmberRailSpike.h"
#include "Embers/Powers/EmberSlagBomb.h"
#include "Tests/EmberHeroPowerBench.h"
#include "Tests/AutomationCommon.h"

// Gauge's three ember powers in a test level (Embers/Powers/*): Slag Bomb's lob, burst and burning pool, Iron Hide's plates
// (the damage cut, the pace, the biter burned), Rail Spike's line through every creature on it.

using namespace EmberHeroBench;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmberSlagBombTest, "Looter.Embers.Power.SlagBomb",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmberSlagBombTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	SpawnFloor(World);
	FEditorScriptExecutionGuard RunEvents;
	const FBench Bench = SpawnBench(World, FVector(-1500.0, 0.0, 0.0));
	ASpiderCreature* Target = SpawnSpider(World, FVector(-500.0, 0.0, 0.0));
	ASpiderCreature* Edge = SpawnSpider(World, FVector(-500.0, 400.0, 0.0));
	ASpiderCreature* Clear = SpawnSpider(World, FVector(-500.0, 1500.0, 0.0));
	if (!TestNotNull(TEXT("The player"), Bench.Body) || !TestTrue(TEXT("Spiders"), Target && Edge && Clear))
	{
		return false;
	}
	TestTrue(TEXT("Its icon (InkedIcons.py)"), HasIcon(TEXT("SlagBomb")));
	UEmberDefinition* Def = MakeDefinition(TEXT("SlagBomb"), 7.f, 48.f, 2000.f, 450.f, 0.f);
	Def->Extras.Add(TEXT("InnerRadius"), 250.f);
	Def->Extras.Add(TEXT("OuterDamage"), 26.f);
	Def->Extras.Add(TEXT("BurnDamage"), 10.f);
	Def->Extras.Add(TEXT("BurnInterval"), 0.5f);
	Def->Extras.Add(TEXT("BurnLinger"), 0.6f);
	Def->Extras.Add(TEXT("PoolSeconds"), 6.f);
	UEmberSlagBomb* Power = NewObject<UEmberSlagBomb>(Bench.Embers);
	const FEmberContext Context = MakeContext(Bench, Def);

	// Lobbed at the spider in its sights: onto its feet.
	TestTrue(TEXT("A spider in its sights: it may be used"), Power->CanActivate(Context) == EEmberDenied::None);
	Power->Activate(Context);
	TestTrue(TEXT("In the air"), Power->IsFlying());
	TestTrue(TEXT("...for the spider's feet"), FVector::Dist2D(Power->GetLanding(), Target->GetActorLocation()) < 10.f);
	int32 Frames = 0;
	while (Power->IsFlying() && Frames < 120)
	{
		Power->StepBomb(Frame);
		++Frames;
	}
	TestFalse(TEXT("Landed"), Power->IsFlying());
	TestTrue(FString::Printf(TEXT("...within a second (%d frames)"), Frames), Frames <= 62);
	TestTrue(TEXT("...and the pool is down"), Power->IsPooling());
	TestEqual(TEXT("The one it fell on took the heart of it"), DealtTo(*Power, Target), 48.f, 5.f);
	TestEqual(TEXT("...the one 4 m off the edge of it"), DealtTo(*Power, Edge), 26.f, 3.f);
	TestEqual(TEXT("...the one apart nothing"), DealtTo(*Power, Clear), 0.f);

	// The pool burns what stands in it, every half second.
	const float Before = DealtTo(*Power, Target);
	float Time = 0.f;
	while (Time < 0.5f + Frame)
	{
		Power->StepBomb(Frame);
		Time += Frame;
	}
	TestTrue(TEXT("A burn on its beat"), Power->NumBurns() >= 1);
	TestEqual(TEXT("...10 to the one in it"), DealtTo(*Power, Target) - Before, 10.f, 1.5f);
	// Its time up with the slag still hot (here, early): the pool burns out its own time (on the linger), then goes.
	Power->End(Context, false);
	TestTrue(TEXT("Time up, the slag still hot: the pool burns on"), Power->IsPooling());
	int32 Burning = 0;
	while (Power->IsPooling() && Burning < 600)
	{
		Power->StepBomb(Frame);
		++Burning;
	}
	TestFalse(TEXT("...until its 6 s are out"), Power->IsPooling());
	TestTrue(FString::Printf(TEXT("...about 5.5 s more (%d frames)"), Burning), Burning >= 300 && Burning <= 340);

	// Aimed at nothing: its full reach out, down onto the ground; over a drop with no ground: refused.
	Target->SetActorLocation(FVector(-500.0, 3000.0, Target->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
	FVector Landing;
	TestTrue(TEXT("Nothing in its sights: it lands at its reach"), UEmberSlagBomb::FindLanding(*Bench.Body, 2000.f, Landing));
	TestEqual(TEXT("...on the ground"), static_cast<float>(Landing.Z), 0.f, 2.f);
	Bench.Body->SetActorLocation(FVector(2900.0, 0.0, Bench.Body->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
	TestTrue(TEXT("Facing off the floor's edge with nothing under its reach: refused"), Power->CanActivate(Context) == EEmberDenied::Refused);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmberIronHideTest, "Looter.Embers.Power.IronHide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmberIronHideTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	SpawnFloor(World);
	FEditorScriptExecutionGuard RunEvents;
	const FBench Bench = SpawnBench(World, FVector::ZeroVector);
	ASpiderCreature* Biter = SpawnSpider(World, FVector(200.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("The player"), Bench.Body) || !TestNotNull(TEXT("...with health"), Bench.Health) || !TestNotNull(TEXT("A spider"), Biter))
	{
		return false;
	}
	TestTrue(TEXT("Its icon (InkedIcons.py)"), HasIcon(TEXT("IronHide")));
	UEmberDefinition* Def = MakeDefinition(TEXT("IronHide"), 6.f, 0.f, 0.f, 0.f, 0.6f);
	Def->Extras.Add(TEXT("Pace"), 0.75f);
	Def->Extras.Add(TEXT("BiteBurn"), 12.f);
	UEmberIronHide* Power = NewObject<UEmberIronHide>(Bench.Embers);
	const FEmberContext Context = MakeContext(Bench, Def);
	Power->Activate(Context);
	TestTrue(TEXT("Locked"), Power->IsLocked());
	TestEqual(TEXT("...the player's health takes 40% of a blow"), Bench.Health->GetDamageTakenScale(), 0.4f, 0.001f);
	const UMovementSlowComponent* Slow = Bench.Body->FindComponentByClass<UMovementSlowComponent>();
	TestTrue(TEXT("...at three-quarter pace"), Slow && FMath::IsNearlyEqual(Slow->GetMultiplier(), 0.75f, 0.001f));

	// A blow: 40% of it lands. A bite: the biter burns its teeth.
	const float Full = Bench.Health->GetHealth();
	Hurt(Bench.Body, 50.f);
	TestEqual(TEXT("A 50 blow takes 20"), Full - Bench.Health->GetHealth(), 20.f, 0.01f);
	TestEqual(TEXT("...a blow with no biter burns nobody"), Power->NumBurned(), 0);
	Bench.Body->OnTakeAnyDamage.Broadcast(Bench.Body, 10.f, GetDefault<UCreatureAttackDamageType>(), nullptr, Biter);
	TestEqual(TEXT("A bite: the biter burned"), Power->NumBurned(), 1);
	TestEqual(TEXT("...for 12"), DealtTo(*Power, Biter), 12.f, 1.5f);
	Bench.Body->OnTakeAnyDamage.Broadcast(Bench.Body, 10.f, GetDefault<UCreatureAttackDamageType>(), nullptr, Biter);
	TestEqual(TEXT("...once a moment, not every bite at once"), Power->NumBurned(), 1);
	Power->Tick(Context, UEmberIronHide::BiteEvery + 0.05f);
	Bench.Body->OnTakeAnyDamage.Broadcast(Bench.Body, 10.f, GetDefault<UCreatureAttackDamageType>(), nullptr, Biter);
	TestEqual(TEXT("...and again after it"), Power->NumBurned(), 2);

	Power->End(Context, false);
	TestFalse(TEXT("Open again"), Power->IsLocked());
	TestEqual(TEXT("...the whole blow lands again"), Bench.Health->GetDamageTakenScale(), 1.f, 0.001f);
	Bench.Health->ResetHealth();
	const float Again = Bench.Health->GetHealth();
	Hurt(Bench.Body, 30.f);
	TestEqual(TEXT("...a 30 blow takes 30"), Again - Bench.Health->GetHealth(), 30.f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmberRailSpikeTest, "Looter.Embers.Power.RailSpike",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmberRailSpikeTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	SpawnFloor(World);
	FEditorScriptExecutionGuard RunEvents;
	const FBench Bench = SpawnBench(World, FVector(-1500.0, 0.0, 0.0));
	ASpiderCreature* First = SpawnSpider(World, FVector(-1000.0, 0.0, 0.0));
	ASpiderCreature* Second = SpawnSpider(World, FVector(-500.0, 30.0, 0.0));
	ASpiderCreature* Aside = SpawnSpider(World, FVector(-500.0, 400.0, 0.0));
	if (!TestNotNull(TEXT("The player"), Bench.Body) || !TestTrue(TEXT("Spiders"), First && Second && Aside))
	{
		return false;
	}
	TestTrue(TEXT("Its icon (InkedIcons.py)"), HasIcon(TEXT("RailSpike")));
	UEmberDefinition* Def = MakeDefinition(TEXT("RailSpike"), 0.f, 60.f, 2500.f, 60.f, 0.f);
	Def->Extras.Add(TEXT("Shove"), 150.f);
	Def->Extras.Add(TEXT("Dizzy"), 0.3f);
	UEmberRailSpike* Power = NewObject<UEmberRailSpike>(Bench.Embers);
	const FEmberContext Context = MakeContext(Bench, Def);

	// Through every creature on the line, nearest first; none off it.
	const FRailSpikeLine Line = UEmberRailSpike::PlanLine(*Bench.Body, 2500.f, 60.f);
	TestEqual(TEXT("Two on the line"), Line.Creatures.Num(), 2);
	TestTrue(TEXT("...the nearer first"), Line.Creatures.Num() == 2 && Line.Creatures[0] == First && Line.Creatures[1] == Second);
	TestFalse(TEXT("Open air: it flies its whole reach"), Line.bEmbeds);
	Power->Activate(Context);
	Power->End(Context, false);
	TestTrue(TEXT("Flying"), Power->IsFlying());
	for (int32 Step = 0; Step < 12 && Power->IsFlying(); ++Step)
	{
		Power->StepSpike(Frame);
	}
	TestFalse(TEXT("Its flight done"), Power->IsFlying());
	TestEqual(TEXT("Through both"), Power->NumPierced(), 2);
	TestEqual(TEXT("...60 to the first"), DealtTo(*Power, First), 60.f, 7.f);
	TestEqual(TEXT("...60 to the second"), DealtTo(*Power, Second), 60.f, 7.f);
	TestEqual(TEXT("...none to the one aside"), DealtTo(*Power, Aside), 0.f);
	const UCreatureKnockComponent* Knock = UCreatureKnockComponent::Find(First);
	TestTrue(TEXT("...each driven back along the line"), Knock && Knock->IsKnocked());

	// A wall at 7 m: it embeds there, and what's behind is spared.
	LocomotionTestWorld::SpawnBlock(World, FVector(-750.0, 0.0, 200.0), FVector(100.0, 600.0, 400.0));
	const FRailSpikeLine Walled = UEmberRailSpike::PlanLine(*Bench.Body, 2500.f, 60.f);
	TestTrue(TEXT("It stops in the wall"), Walled.bEmbeds && Walled.Length < 800.f);
	TestTrue(TEXT("...only the one before it on the line"), Walled.Creatures.Num() == 1 && Walled.Creatures[0] == First);
	return true;
}

#endif

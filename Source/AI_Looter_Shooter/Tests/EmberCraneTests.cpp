#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureKnockComponent.h"
#include "Embers/Powers/EmberGraveWard.h"
#include "Embers/Powers/EmberLastRites.h"
#include "Embers/Powers/EmberPropActor.h"
#include "Embers/Powers/EmberTollChain.h"
#include "Tests/EmberHeroPowerBench.h"
#include "UI/World/EmberMarkActor.h"
#include "Tests/AutomationCommon.h"

// Crane's three ember powers in a test level (Embers/Powers/*): Toll Chain's hook and haul, Grave Ward's light (the slowed,
// the mending), Last Rites' mark settled both ways.

using namespace EmberHeroBench;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmberTollChainTest, "Looter.Embers.Power.TollChain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmberTollChainTest::RunTest(const FString& Parameters)
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
	if (!TestNotNull(TEXT("The player"), Bench.Body))
	{
		return false;
	}
	TestTrue(TEXT("Its icon (InkedIcons.py)"), HasIcon(TEXT("TollChain")));
	UEmberDefinition* Def = MakeDefinition(TEXT("TollChain"), 0.5f, 20.f, 1800.f, 0.f, 0.f);
	Def->Extras.Add(TEXT("Stun"), 1.5f);
	UEmberTollChain* Power = NewObject<UEmberTollChain>(Bench.Embers);
	const FEmberContext Context = MakeContext(Bench, Def);

	// Nothing in his sights: refused.
	TestTrue(TEXT("Nothing to hook: refused"), Power->CanActivate(Context) == EEmberDenied::Refused);

	// A spider 12 m ahead: hooked, bitten, hauled to a stride off, reeling.
	ASpiderCreature* Spider = SpawnSpider(World, FVector(-300.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("A spider"), Spider))
	{
		return false;
	}
	TestTrue(TEXT("A spider in his sights: it may be used"), Power->CanActivate(Context) == EEmberDenied::None);
	Power->Activate(Context);
	TestTrue(TEXT("Thrown at it"), Power->GetHooked() == Spider && Power->IsThrowing());
	const FVector Start = Spider->GetActorLocation();
	float Time = 0.f;
	while (Time < 0.5f - KINDA_SMALL_NUMBER)
	{
		Power->StepChain(Frame);
		Time += Frame;
	}
	Power->End(Context, false);
	TestTrue(TEXT("It bit"), Power->HasBitten());
	TestEqual(TEXT("...for its damage"), DealtTo(*Power, Spider), 20.f, 2.5f);
	const float Left = static_cast<float>(FVector::Dist2D(Spider->GetActorLocation(), Bench.Body->GetActorLocation()));
	const float Stride = Bench.Body->GetCapsuleComponent()->GetScaledCapsuleRadius() + Spider->GetCapsuleComponent()->GetScaledCapsuleRadius()
		+ UEmberTollChain::HaulGap;
	TestTrue(FString::Printf(TEXT("Hauled from %.0f cm to a stride off (%.0f cm, the stride %.0f)"), FVector::Dist2D(Start, Bench.Body->GetActorLocation()), Left, Stride),
		FMath::Abs(Left - Stride) < 25.f);
	TestEqual(TEXT("...along the ground"), static_cast<float>(Spider->GetActorLocation().Z), static_cast<float>(Start.Z), 5.f);
	const UCreatureKnockComponent* Knock = UCreatureKnockComponent::Find(Spider);
	TestTrue(TEXT("...left reeling"), Knock && Knock->IsDizzy());
	TestTrue(FString::Printf(TEXT("...for its stun (%.2f s left)"), Knock ? Knock->GetDizzyLeft() : 0.f), Knock && Knock->GetDizzyLeft() >= 1.3f);

	// A wall between: not in sight, refused.
	LocomotionTestWorld::SpawnBlock(World, FVector(-900.0, 0.0, 150.0), FVector(100.0, 800.0, 300.0));
	Spider->SetActorLocation(FVector(-300.0, 0.0, Start.Z), false, nullptr, ETeleportType::TeleportPhysics);
	TestTrue(TEXT("Behind a wall: refused"), Power->CanActivate(Context) == EEmberDenied::Refused);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmberGraveWardTest, "Looter.Embers.Power.GraveWard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmberGraveWardTest::RunTest(const FString& Parameters)
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
	ASpiderCreature* Inside = SpawnSpider(World, FVector(350.0, 150.0, 0.0));
	ASpiderCreature* Outside = SpawnSpider(World, FVector(1400.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("The player"), Bench.Body) || !TestNotNull(TEXT("...with health"), Bench.Health) || !TestTrue(TEXT("Spiders"), Inside && Outside))
	{
		return false;
	}
	TestTrue(TEXT("Its icon (InkedIcons.py)"), HasIcon(TEXT("GraveWard")));
	UEmberDefinition* Def = MakeDefinition(TEXT("GraveWard"), 8.f, 0.f, 0.f, 500.f, 0.5f);
	Def->Extras.Add(TEXT("HealPerSecond"), 5.f);
	UEmberGraveWard* Power = NewObject<UEmberGraveWard>(Bench.Embers);
	const FEmberContext Context = MakeContext(Bench, Def);
	TestTrue(TEXT("Ground to plant it on: it may be used"), Power->CanActivate(Context) == EEmberDenied::None);
	Power->Activate(Context);
	TestTrue(TEXT("Burning"), Power->IsBurning());
	TestTrue(TEXT("...a step in front of him"), FVector::Dist2D(Power->GetCenter(), Bench.Body->GetActorLocation()) < 120.f);
	TestNotNull(TEXT("...the lantern set down"), Power->GetLantern());

	// Hurt, standing in its light: he mends; the spider in it crawls, the one outside doesn't.
	Hurt(Bench.Body, 30.f);
	const float Hurted = Bench.Health->GetHealth();
	Power->StepWard(Frame);
	TestEqual(TEXT("The spider in the light at half its speed"), Inside->CustomTimeDilation, 0.5f, 0.001f);
	TestEqual(TEXT("...the one outside at its own"), Outside->CustomTimeDilation, 1.f, 0.001f);
	TestEqual(TEXT("...one slowed"), Power->NumSlowed(), 1);
	float Time = Frame;
	while (Time < 1.f - KINDA_SMALL_NUMBER)
	{
		Power->StepWard(Frame);
		Time += Frame;
	}
	TestEqual(TEXT("A second in its light mends 5"), Bench.Health->GetHealth() - Hurted, 5.f, 0.2f);

	// A hit-stop's brief stop gives the clock back: the ward takes it again; out of the light, it's its own again.
	Inside->CustomTimeDilation = 1.f;
	Power->StepWard(Frame);
	TestEqual(TEXT("Its clock given back by something else: slowed again"), Inside->CustomTimeDilation, 0.5f, 0.001f);
	Inside->SetActorLocation(Inside->GetActorLocation() + FVector(2000.0, 0.0, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
	Power->StepWard(Frame);
	TestEqual(TEXT("Out of the light: its own clock"), Inside->CustomTimeDilation, 1.f, 0.001f);
	Inside->SetActorLocation(Inside->GetActorLocation() - FVector(2000.0, 0.0, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
	Power->StepWard(Frame);

	Power->End(Context, false);
	TestFalse(TEXT("Over"), Power->IsBurning());
	TestEqual(TEXT("...every clock its own again"), Inside->CustomTimeDilation, 1.f, 0.001f);
	TestNull(TEXT("...the lantern taken up"), Power->GetLantern());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmberLastRitesTest, "Looter.Embers.Power.LastRites",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmberLastRitesTest::RunTest(const FString& Parameters)
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
	ASpiderCreature* Marked = SpawnSpider(World, FVector(-500.0, 0.0, 0.0));
	ASpiderCreature* Pack = SpawnSpider(World, FVector(-500.0, 300.0, 0.0));
	ASpiderCreature* Apart = SpawnSpider(World, FVector(-500.0, 1500.0, 0.0));
	if (!TestNotNull(TEXT("The player"), Bench.Body) || !TestTrue(TEXT("Spiders"), Marked && Pack && Apart))
	{
		return false;
	}
	TestTrue(TEXT("Its icon (InkedIcons.py)"), HasIcon(TEXT("LastRites")));
	UEmberDefinition* Def = MakeDefinition(TEXT("LastRites"), 8.f, 60.f, 4000.f, 450.f, 0.f);
	Def->Extras.Add(TEXT("Strike"), 90.f);
	UEmberLastRites* Power = NewObject<UEmberLastRites>(Bench.Embers);
	const FEmberContext Context = MakeContext(Bench, Def);

	// Marked, then killed in time: its soul bursts on its pack (the near one), not the far.
	TestTrue(TEXT("A spider in his sights: it may be used"), Power->CanActivate(Context) == EEmberDenied::None);
	Power->Activate(Context);
	TestTrue(TEXT("The one in his sights is marked"), Power->GetMarked() == Marked && Power->IsMarking());
	AEmberMarkActor* Sigil = Power->GetSigil();
	if (TestNotNull(TEXT("...its sigil over it"), Sigil))
	{
		Sigil->Advance(0.f);
		TestEqual(TEXT("...counting its 8 seconds"), Sigil->GetWords(), FString(TEXT("8")));
	}
	TestFalse(TEXT("Alive: not settled yet"), Power->StepMark(1.f));
	Hurt(Marked, 100000.f);
	TestTrue(TEXT("The marked one is dead"), Marked->IsDead());
	TestTrue(TEXT("Settled"), Power->StepMark(Frame));
	TestTrue(TEXT("...by the burst"), Power->DidBurst() && !Power->DidStrike());
	TestEqual(TEXT("Its pack took the burst"), DealtTo(*Power, Pack), 60.f, 7.f);
	TestEqual(TEXT("...the one apart didn't"), DealtTo(*Power, Apart), 0.f);
	const UCreatureKnockComponent* Knock = UCreatureKnockComponent::Find(Pack);
	TestTrue(TEXT("...and was thrown back"), Knock && Knock->HoldsBrain());
	Power->End(Context, false);
	TestFalse(TEXT("Nothing struck after it's settled"), Power->DidStrike());

	// Marked and left alive to the end: the mark strikes it.
	Pack->SetActorLocation(FVector(-500.0, 0.0, Pack->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
	if (UCreatureKnockComponent* PackKnock = UCreatureKnockComponent::Find(Pack))
	{
		PackKnock->Recover();
	}
	Power->Activate(Context);
	TestTrue(TEXT("The next one marked"), Power->GetMarked() == Pack);
	Power->StepMark(4.f);
	Power->End(Context, false);
	TestTrue(TEXT("It lived: the mark struck it"), Power->DidStrike() && !Power->DidBurst());
	TestEqual(TEXT("...for the strike's 90"), DealtTo(*Power, Pack), 90.f, 10.f);
	return true;
}

#endif

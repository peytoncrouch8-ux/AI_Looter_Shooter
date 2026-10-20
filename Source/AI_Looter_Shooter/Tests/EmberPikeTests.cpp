#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/BulletSubsystem.h"
#include "Creatures/CreatureKnockComponent.h"
#include "Embers/Powers/EmberLongShot.h"
#include "Embers/Powers/EmberSpyglass.h"
#include "Embers/Powers/EmberTripwire.h"
#include "Tests/EmberHeroPowerBench.h"
#include "Engine/HitResult.h"
#include "Tests/AutomationCommon.h"

// Pike's three ember powers in a test level (Embers/Powers/*): Spyglass's sweep and marks (the extra damage they take),
// Tripwire's wire and the creature that crosses it, Long Shot's distance and its marked creatures' sure critical.

using namespace EmberHeroBench;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmberSpyglassTest, "Looter.Embers.Power.Spyglass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmberSpyglassTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	// A floor 200 m across: one spider within the glass's 60 m (behind a wall), one past it.
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(20000.0, 20000.0, 100.0));
	LocomotionTestWorld::SpawnBlock(World, FVector(1000.0, 0.0, 200.0), FVector(100.0, 1000.0, 400.0));
	FEditorScriptExecutionGuard RunEvents;
	const FBench Bench = SpawnBench(World, FVector::ZeroVector);
	ASpiderCreature* Hidden = SpawnSpider(World, FVector(2000.0, 0.0, 0.0));
	ASpiderCreature* Distant = SpawnSpider(World, FVector(8000.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("The player"), Bench.Body) || !TestTrue(TEXT("Spiders"), Hidden && Distant))
	{
		return false;
	}
	TestTrue(TEXT("Its icon (InkedIcons.py)"), HasIcon(TEXT("Spyglass")));
	const UEmberDefinition* Def = MakeDefinition(TEXT("Spyglass"), 10.f, 0.f, 6000.f, 0.f, 0.3f);
	UEmberSpyglass* Power = NewObject<UEmberSpyglass>(Bench.Embers);
	const FEmberContext Context = MakeContext(Bench, Def);
	Power->Activate(Context);
	TestTrue(TEXT("Looking"), Power->IsLooking());
	Power->StepSight(0.1f);
	TestFalse(TEXT("The sweep hasn't reached 20 m yet"), Power->IsMarked(*Hidden));
	for (int32 Step = 0; Step < 15; ++Step)
	{
		Power->StepSight(0.1f);
	}
	TestEqual(TEXT("The sweep has run its 60 m"), Power->GetSweepRadius(), 6000.f, 1.f);
	TestTrue(TEXT("The one behind the wall is marked"), Power->IsMarked(*Hidden));
	TestFalse(TEXT("...the one past 60 m isn't"), Power->IsMarked(*Distant));
	UHealthComponent* HiddenHealth = Hidden->FindComponentByClass<UHealthComponent>();
	if (TestNotNull(TEXT("Its health"), HiddenHealth))
	{
		TestEqual(TEXT("Marked: it takes 30% more"), HiddenHealth->GetDamageTakenScale(), 1.3f, 0.001f);
		const float Before = HiddenHealth->GetHealth();
		Hurt(Hidden, 100.f);
		TestEqual(TEXT("...a 100 hit takes 130"), Before - HiddenHealth->GetHealth(), 130.f, 0.01f);
	}

	Power->End(Context, false);
	TestFalse(TEXT("Shut"), Power->IsLooking());
	TestEqual(TEXT("...no marks"), Power->NumMarked(), 0);
	TestTrue(TEXT("...its damage its own again"), HiddenHealth && FMath::IsNearlyEqual(HiddenHealth->GetDamageTakenScale(), 1.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmberTripwireTest, "Looter.Embers.Power.Tripwire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmberTripwireTest::RunTest(const FString& Parameters)
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
	ASpiderCreature* Spider = SpawnSpider(World, FVector(900.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("The player"), Bench.Body) || !TestNotNull(TEXT("A spider"), Spider))
	{
		return false;
	}
	TestTrue(TEXT("Its icon (InkedIcons.py)"), HasIcon(TEXT("Tripwire")));
	UEmberDefinition* Def = MakeDefinition(TEXT("Tripwire"), 20.f, 30.f, 600.f, 0.f, 0.f);
	Def->Extras.Add(TEXT("Stun"), 2.f);
	UEmberTripwire* Power = NewObject<UEmberTripwire>(Bench.Embers);
	const FEmberContext Context = MakeContext(Bench, Def);

	// Looking level over open ground: strung 4 m ahead, across the aim, its whole 6 m.
	TestTrue(TEXT("Open ground ahead: it may be used"), Power->CanActivate(Context) == EEmberDenied::None);
	Power->Activate(Context);
	TestTrue(TEXT("Strung"), Power->IsStrung());
	const FTripwirePlan& Plan = Power->GetPlan();
	TestEqual(TEXT("...its 6 m"), Plan.Length, 600.f, 30.f);
	TestEqual(TEXT("...4 m ahead"), static_cast<float>((Plan.A + Plan.B).X * 0.5), 400.f, 30.f);
	TestTrue(TEXT("...across the aim"), FMath::Abs(Plan.A.X - Plan.B.X) < 5.0);

	// The spider walking at him crosses it: tripped once, hurt, thrown down a short way, left lying.
	Power->StepWire(Frame);
	Spider->SetActorLocation(FVector(300.0, 0.0, Spider->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
	Power->StepWire(Frame);
	TestTrue(TEXT("It crossed and tripped"), Power->GetTripped().Contains(Spider));
	TestEqual(TEXT("...hurt for 30"), DealtTo(*Power, Spider), 30.f, 3.5f);
	const UCreatureKnockComponent* Knock = UCreatureKnockComponent::Find(Spider);
	TestTrue(TEXT("...thrown down"), Knock && Knock->IsKnocked());
	TestEqual(TEXT("...to lie stunned 2 s once it lands"), Knock ? Knock->GetDizzyLeft() : 0.f, 2.f, 0.01f);
	Spider->SetActorLocation(FVector(500.0, 0.0, Spider->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
	Power->StepWire(Frame);
	TestEqual(TEXT("Back over it: once a wire, not twice"), Power->GetDealt().Num(), 1);
	Power->End(Context, false);
	TestFalse(TEXT("Pulled"), Power->IsStrung());

	// Facing off the floor's edge with no ground in reach: refused.
	Bench.Body->SetActorLocation(FVector(2900.0, 0.0, Bench.Body->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
	TestTrue(TEXT("No ground ahead: refused"), Power->CanActivate(Context) == EEmberDenied::Refused);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmberLongShotTest, "Looter.Embers.Power.LongShot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmberLongShotTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	SpawnFloor(World);
	FEditorScriptExecutionGuard RunEvents;
	const FBench Bench = SpawnBench(World, FVector(-2000.0, 0.0, 0.0));
	ASpiderCreature* Sighted = SpawnSpider(World, FVector(-1000.0, 0.0, 0.0));
	ASpiderCreature* Behind = SpawnSpider(World, FVector(-2800.0, 0.0, 0.0));
	UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
	if (!TestNotNull(TEXT("The player"), Bench.Body) || !TestTrue(TEXT("Spiders"), Sighted && Behind) || !TestNotNull(TEXT("Bullets"), Bullets))
	{
		return false;
	}
	TestTrue(TEXT("Its icon (InkedIcons.py)"), HasIcon(TEXT("LongShot")));
	UEmberDefinition* Def = MakeDefinition(TEXT("LongShot"), 8.f, 0.f, 6000.f, 0.f, 0.15f);
	Def->Extras.Add(TEXT("MaxBonus"), 1.f);
	Def->Extras.Add(TEXT("ConeDegrees"), 30.f);
	Def->Extras.Add(TEXT("MarkFrom"), 2500.f);
	UEmberLongShot* Power = NewObject<UEmberLongShot>(Bench.Embers);
	const FEmberContext Context = MakeContext(Bench, Def);
	Power->Activate(Context);
	TestTrue(TEXT("Steady"), Power->IsSteady());
	TestTrue(TEXT("The one in his sights is marked"), Power->IsMarked(*Sighted));
	TestFalse(TEXT("...the one behind him isn't"), Power->IsMarked(*Behind));

	// Harder the farther it flies, to double; a marked creature always critical; a long hit marks.
	TestEqual(TEXT("Point blank: as it is"), Power->ScaleAt(0.f), 1.f, 0.001f);
	TestEqual(TEXT("30 m: +45%"), Power->ScaleAt(3000.f), 1.45f, 0.001f);
	TestEqual(TEXT("100 m: double, no more"), Power->ScaleAt(10000.f), 2.f, 0.001f);
	FBulletShot Shot;
	Shot.Shooter = Bench.Body;
	Shot.Damage = 20.f;
	FHitResult OnSighted(Sighted, Sighted->GetCapsuleComponent(), Sighted->GetActorLocation(), -FVector::ForwardVector);
	FBulletLanding Landing;
	Landing.Traveled = 1000.f;
	Power->NoteLanding(Shot, OnSighted, Landing);
	TestTrue(TEXT("A marked one hit: critical"), Landing.bCritical);
	TestEqual(TEXT("...and 10 m's 15% more"), Landing.DamageScale, 1.15f, 0.001f);
	FHitResult OnBehind(Behind, Behind->GetCapsuleComponent(), Behind->GetActorLocation(), FVector::ForwardVector);
	FBulletLanding Near;
	Near.Traveled = 800.f;
	Power->NoteLanding(Shot, OnBehind, Near);
	TestFalse(TEXT("An unmarked one hit near: not critical"), Near.bCritical);
	TestFalse(TEXT("...nor marked by it"), Power->IsMarked(*Behind));
	FBulletLanding Far;
	Far.Traveled = 3000.f;
	Power->NoteLanding(Shot, OnBehind, Far);
	TestTrue(TEXT("Hit from 30 m: marked, and critical"), Power->IsMarked(*Behind) && Far.bCritical);
	FBulletShot Stranger = Shot;
	Stranger.Shooter = Sighted;
	FBulletLanding Theirs;
	Theirs.Traveled = 3000.f;
	Power->NoteLanding(Stranger, OnSighted, Theirs);
	TestTrue(TEXT("Another's shot isn't his"), !Theirs.bCritical && FMath::IsNearlyEqual(Theirs.DamageScale, 1.f));

	// Through the bullets: a shot onto a wall 30 m off lands 45% harder.
	LocomotionTestWorld::SpawnBlock(World, FVector(1050.0, 0.0, 400.0), FVector(100.0, 2000.0, 800.0));
	FBulletShot Fired;
	Fired.Start = FVector(-2000.0, 0.0, 400.0);
	Fired.VisualStart = Fired.Start;
	Fired.Direction = FVector::ForwardVector;
	Fired.Speed = 0.f;
	Fired.Range = 5000.f;
	Fired.Damage = 20.f;
	Fired.Shooter = Bench.Body;
	float Landed = 0.f;
	const FDelegateHandle Watch = Bullets->OnBulletHit.AddLambda([&Landed](const FHitResult&, float Damage, bool) { Landed = Damage; });
	Bullets->Fire(Fired);
	Bullets->OnBulletHit.Remove(Watch);
	TestTrue(FString::Printf(TEXT("...%.1f, 20 x 1.45 give or take a tenth"), Landed), Landed >= 29.f * 0.9f - 0.01f && Landed <= 29.f * 1.1f + 0.01f);

	Power->End(Context, false);
	TestFalse(TEXT("Over"), Power->IsSteady());
	TestEqual(TEXT("...its marks let go"), Power->NumMarked(), 0);
	return true;
}

#endif

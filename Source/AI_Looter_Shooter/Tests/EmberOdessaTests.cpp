#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/BulletSubsystem.h"
#include "Embers/Powers/EmberDoubleOrNothing.h"
#include "Embers/Powers/EmberFiftyTwoPickup.h"
#include "Embers/Powers/EmberLuckyStreak.h"
#include "Tests/EmberHeroPowerBench.h"
#include "CoreGlobals.h"
#include "Engine/HitResult.h"
#include "Tests/AutomationCommon.h"

// Odessa's three ember powers in a test level (Embers/Powers/*): Lucky Streak's lucky rounds and ricochets, Fifty-Two
// Pickup's fan of cards, Double or Nothing's toss. A pull of the trigger is told apart by the frame it's fired in, so the
// tests move GFrameCounter on between pulls as the game's frames would.

using namespace EmberHeroBench;

namespace
{
	/** A shot of the player's as their gun would fire it, along +X from Start. */
	FBulletShot PlayerShot(ACharacter* Shooter, const FVector& Start, float Damage)
	{
		FBulletShot Shot;
		Shot.Start = Start;
		Shot.VisualStart = Start;
		Shot.Direction = FVector::ForwardVector;
		Shot.Speed = 0.f;
		Shot.Range = 5000.f;
		Shot.Damage = Damage;
		Shot.Shooter = Shooter;
		return Shot;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmberLuckyStreakTest, "Looter.Embers.Power.LuckyStreak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmberLuckyStreakTest::RunTest(const FString& Parameters)
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
	ASpiderCreature* Struck = SpawnSpider(World, FVector(-800.0, 0.0, 0.0));
	ASpiderCreature* Near = SpawnSpider(World, FVector(-800.0, 500.0, 0.0));
	ASpiderCreature* Far = SpawnSpider(World, FVector(-800.0, -2000.0, 0.0));
	UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
	if (!TestNotNull(TEXT("The player"), Bench.Body) || !TestTrue(TEXT("Spiders"), Struck && Near && Far) || !TestNotNull(TEXT("Bullets"), Bullets))
	{
		return false;
	}
	TestTrue(TEXT("Its icon (InkedIcons.py)"), HasIcon(TEXT("LuckyStreak")));
	UEmberDefinition* Def = MakeDefinition(TEXT("LuckyStreak"), 8.f, 14.f, 1200.f, 0.f, 0.f);
	Def->Extras.Add(TEXT("CritEvery"), 3.f);
	UEmberLuckyStreak* Power = NewObject<UEmberLuckyStreak>(Bench.Embers);
	const FEmberContext Context = MakeContext(Bench, Def);
	Power->Activate(Context);
	TestTrue(TEXT("Streaking"), Power->IsStreaking());

	// Every third pull is lucky: critical wherever it lands, its tracer gold; a pull's pellets go together.
	FBulletShot Shot = PlayerShot(Bench.Body, FVector(-1400.0, 0.0, 400.0), 20.f);
	for (int32 Pull = 1; Pull <= 3; ++Pull)
	{
		++GFrameCounter;
		FBulletShot Pellet = Shot;
		Power->NoteFiring(Pellet);
		TestEqual(*FString::Printf(TEXT("Pull %d: lucky only on the third"), Pull), Pellet.bSureCritical, Pull == 3);
		FBulletShot SecondPellet = Shot;
		Power->NoteFiring(SecondPellet);
		TestEqual(*FString::Printf(TEXT("...its other pellet the same (pull %d)"), Pull), SecondPellet.bSureCritical, Pull == 3);
	}
	TestEqual(TEXT("Three pulls counted"), Power->GetPulls(), 3);
	FBulletShot Stranger = Shot;
	Stranger.Shooter = Struck;
	++GFrameCounter;
	Power->NoteFiring(Stranger);
	TestEqual(TEXT("Another's shot isn't hers"), Power->GetPulls(), 3);

	// Through the bullets: the sixth pull's round lands critical on a wall (OnBulletFiring, then the hit).
	LocomotionTestWorld::SpawnBlock(World, FVector(1000.0, 0.0, 400.0), FVector(100.0, 2000.0, 600.0));
	bool bLastCritical = false;
	const FDelegateHandle Watch = Bullets->OnBulletHit.AddLambda([&bLastCritical](const FHitResult&, float, bool bCritical) { bLastCritical = bCritical; });
	for (int32 Pull = 4; Pull <= 6; ++Pull)
	{
		++GFrameCounter;
		Bullets->Fire(Shot);
		TestEqual(*FString::Printf(TEXT("Pull %d through the bullets: critical only when lucky"), Pull), bLastCritical, Pull == 6);
	}
	Bullets->OnBulletHit.Remove(Watch);

	// A hit on a creature ricochets into the nearest other within reach (one a pull), and lands for its damage.
	FHitResult Hit(Struck, Struck->GetCapsuleComponent(), Struck->GetActorLocation(), -FVector::ForwardVector);
	FBulletLanding Landing;
	++GFrameCounter;
	Power->NoteLanding(Shot, Hit, Landing);
	TestEqual(TEXT("A ricochet flies"), Power->NumRicochetsFlying(), 1);
	TestTrue(TEXT("...at the nearest other creature"), Power->GetLastRicochetTarget() == Near);
	Power->NoteLanding(Shot, Hit, Landing);
	TestEqual(TEXT("...one a pull"), Power->NumRicochetsFlying(), 1);
	for (int32 Step = 0; Step < 30; ++Step)
	{
		Power->StepFlight(Frame);
	}
	TestEqual(TEXT("It has landed"), Power->NumRicochetsFlying(), 0);
	TestEqual(TEXT("...for its damage (14, give or take a tenth)"), DealtTo(*Power, Near), 14.f, 1.5f);
	TestEqual(TEXT("...the far one untouched"), DealtTo(*Power, Far), 0.f);

	Power->End(Context, false);
	TestFalse(TEXT("Over"), Power->IsStreaking());
	++GFrameCounter;
	FBulletShot After = Shot;
	Power->NoteFiring(After);
	TestFalse(TEXT("...and its rounds are plain again"), After.bSureCritical);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmberFiftyTwoPickupTest, "Looter.Embers.Power.FiftyTwoPickup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmberFiftyTwoPickupTest::RunTest(const FString& Parameters)
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
	ASpiderCreature* Close = SpawnSpider(World, FVector(-1200.0, 0.0, 0.0));
	ASpiderCreature* Beyond = SpawnSpider(World, FVector(1500.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("The player"), Bench.Body) || !TestTrue(TEXT("Spiders"), Close && Beyond))
	{
		return false;
	}
	TestTrue(TEXT("Its icon (InkedIcons.py)"), HasIcon(TEXT("FiftyTwoPickup")));
	UEmberDefinition* Def = MakeDefinition(TEXT("FiftyTwoPickup"), 0.f, 8.f, 1400.f, 0.f, 0.f);
	Def->Extras.Add(TEXT("Cards"), 52.f);
	Def->Extras.Add(TEXT("ConeDegrees"), 35.f);
	Def->Extras.Add(TEXT("FanTall"), 12.f);
	UEmberFiftyTwoPickup* Power = NewObject<UEmberFiftyTwoPickup>(Bench.Embers);
	const FEmberContext Context = MakeContext(Bench, Def);
	Power->Activate(Context);
	Power->End(Context, false);
	TestEqual(TEXT("The whole deck thrown"), Power->NumThrown(), 52);
	TestEqual(TEXT("...all in the air"), Power->NumFlying(), 52);
	for (int32 Step = 0; Step < 40; ++Step)
	{
		Power->StepCards(Frame);
	}
	TestEqual(TEXT("Every card has landed somewhere"), Power->NumFlying(), 0);
	TestTrue(FString::Printf(TEXT("The spider 3 m off is cut by many (%d cards)"), Power->NumCuts()), Power->NumCuts() >= 10);
	const float Cut = DealtTo(*Power, Close);
	TestTrue(FString::Printf(TEXT("...for 8 a card (%.0f)"), Cut), Cut >= 8.f * 0.9f * Power->NumCuts() - 1.f && Cut <= 8.f * 1.1f * Power->NumCuts() + 1.f);
	TestTrue(TEXT("...dealt a frame at a time, not a card at a time"), Power->GetDealt().Num() < Power->NumCuts());
	TestEqual(TEXT("The one past its reach is untouched"), DealtTo(*Power, Beyond), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmberDoubleOrNothingTest, "Looter.Embers.Power.DoubleOrNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmberDoubleOrNothingTest::RunTest(const FString& Parameters)
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
	UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
	if (!TestNotNull(TEXT("The player"), Bench.Body) || !TestNotNull(TEXT("Bullets"), Bullets))
	{
		return false;
	}
	TestTrue(TEXT("Its icon (InkedIcons.py)"), HasIcon(TEXT("DoubleOrNothing")));
	UEmberDefinition* Def = MakeDefinition(TEXT("DoubleOrNothing"), 6.f, 0.f, 0.f, 0.f, 2.f);
	Def->Extras.Add(TEXT("Odds"), 0.5f);
	Def->Extras.Add(TEXT("MissStreak"), 1.f);
	UEmberDoubleOrNothing* Power = NewObject<UEmberDoubleOrNothing>(Bench.Embers);
	const FEmberContext Context = MakeContext(Bench, Def);
	Power->Activate(Context);
	Power->SetSeed(1234);
	TestTrue(TEXT("Gambling"), Power->IsGambling());

	// Each pull: doubled, or thrown wide and blank; never two blanks running.
	const FBulletShot Shot = PlayerShot(Bench.Body, FVector(-1400.0, 0.0, 400.0), 20.f);
	bool bLastWasTails = false;
	bool bTwoTails = false;
	for (int32 Pull = 0; Pull < 40; ++Pull)
	{
		++GFrameCounter;
		FBulletShot Pellet = Shot;
		Power->NoteFiring(Pellet);
		if (Power->WasLastHeads())
		{
			TestEqual(TEXT("Heads: double damage"), Pellet.Damage, 40.f, 0.01f);
			TestFalse(TEXT("...a live round"), Pellet.bBlank);
		}
		else
		{
			TestTrue(TEXT("Tails: a blank"), Pellet.bBlank);
			const float Off = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(static_cast<float>(FVector::DotProduct(Pellet.Direction, Shot.Direction)), -1.f, 1.f)));
			TestTrue(FString::Printf(TEXT("...thrown %.1f degrees wide"), Off), Off >= 4.9f && Off <= 9.1f);
			bTwoTails |= bLastWasTails;
		}
		bLastWasTails = !Power->WasLastHeads();
	}
	TestFalse(TEXT("Never two misses running"), bTwoTails);
	TestTrue(FString::Printf(TEXT("Both faces came up (%d heads, %d tails)"), Power->GetHeads(), Power->GetTails()), Power->GetHeads() > 0 && Power->GetTails() > 0);
	TestTrue(TEXT("...heads at least as often as tails (the house leans her way)"), Power->GetHeads() >= Power->GetTails());

	// Through the bullets, onto a wide wall 10 m off: double or nothing at all.
	LocomotionTestWorld::SpawnBlock(World, FVector(-400.0, 0.0, 400.0), FVector(100.0, 2000.0, 800.0));
	TArray<float> Hits;
	const FDelegateHandle Watch = Bullets->OnBulletHit.AddLambda([&Hits](const FHitResult&, float Damage, bool) { Hits.Add(Damage); });
	for (int32 Pull = 0; Pull < 12; ++Pull)
	{
		++GFrameCounter;
		Bullets->Fire(Shot);
	}
	Bullets->OnBulletHit.Remove(Watch);
	bool bAllDoubleOrNothing = Hits.Num() == 12;
	for (const float Damage : Hits)
	{
		bAllDoubleOrNothing &= Damage == 0.f || (Damage >= 40.f * 0.9f - 0.01f && Damage <= 40.f * 1.1f + 0.01f);
	}
	TestTrue(TEXT("Every shot on the wall dealt double or nothing"), bAllDoubleOrNothing);

	Power->End(Context, false);
	++GFrameCounter;
	FBulletShot After = Shot;
	Power->NoteFiring(After);
	TestTrue(TEXT("Over: shots are as fired"), !After.bBlank && FMath::IsNearlyEqual(After.Damage, 20.f));
	return true;
}

#endif

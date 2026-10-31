#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The den entrance (EAmbushEntrance::Den; the user's ask, 2026-10-31: the Sink floor's spiders crawl out of dens in the pit
// walls; SpiderCreatureDen.cpp, SpiderCreatureDenPose.cpp): its rules (the stagger, the dens in turn, a hard hit, the crawl's
// pull, the tip's depth, the slower climb), and a spider in a test level coming out of a den in a wall: unseen and unhurt at
// first, then out of the mouth (its front feet on the lip before its body), head down on the face, down it, off it, and
// hunting the player within a few seconds; killed in the mouth its body comes down out of the wall; a hard hit knocks it off;
// a den with no wall leaves it where it stands; an ambush sends a wave into its dens in turn. A test level never moves
// anything, so its drop is checked by its launch and its landing by setting it down where it was going (as the lurk's tests).

#include "Combat/HealthComponent.h"
#include "Creatures/AmbushSpawner.h"
#include "Creatures/SpiderAmbushRules.h"
#include "Creatures/SpiderCreature.h"
#include "Tests/EncounterTestWorld.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Tests/AutomationCommon.h"

namespace
{
	constexpr float Frame = 1.f / 60.f;

	/** A test level with ground (its top at 0) and a 9 m rock wall whose face is at X 500, facing -X. */
	UWorld* MakeWallWorld(FTestWorldWrapper& Wrapper)
	{
		if (!Wrapper.CreateTestWorld(EWorldType::EditorPreview))
		{
			return nullptr;
		}
		UWorld* World = Wrapper.GetTestWorld();
		EncounterTestWorld::SpawnBlocks(World, { FBox(FVector(-3000.0, -3000.0, -100.0), FVector(3000.0, 3000.0, 0.0)),
			FBox(FVector(500.0, -800.0, 0.0), FVector(900.0, 800.0, 900.0)) });
		return World;
	}

	/** A brown spider standing at Feet, its movement given its capsule (a test level's has none), so it can be held and dropped. */
	ASpiderCreature* SpawnSpider(UWorld* World, const FVector& Feet)
	{
		ASpiderCreature* Spider = Cast<ASpiderCreature>(EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), Feet));
		if (Spider)
		{
			Spider->GetCharacterMovement()->SetUpdatedComponent(Spider->GetCapsuleComponent());
			Spider->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		}
		return Spider;
	}

	/** A den's mouth in the wall, 2.8 m over the floor (the Sink's highest), facing out (-X). */
	FSpiderPerchSpot WallDen(double Y = 0.0)
	{
		FSpiderPerchSpot Den;
		Den.Kind = ESpiderPerchKind::Wall;
		Den.Spot = FVector(500.0, Y, 280.0);
		Den.FaceYaw = 180.f;
		return Den;
	}

	/** Ticks it until its phase isn't Phase, at most Most seconds; how long it took. */
	float TickWhile(ASpiderCreature& Spider, ESpiderLurkPhase Phase, float Most)
	{
		float Elapsed = 0.f;
		while (Spider.GetLurkPhase() == Phase && Elapsed < Most)
		{
			Spider.Tick(Frame);
			Spider.ConsumeMovementInputVector();
			Elapsed += Frame;
		}
		return Elapsed;
	}

	/** Ticks it for Seconds. */
	void TickFor(ASpiderCreature& Spider, float Seconds)
	{
		for (float Elapsed = 0.f; Elapsed < Seconds; Elapsed += Frame)
		{
			Spider.Tick(Frame);
			Spider.ConsumeMovementInputVector();
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderDenRulesTest, "Looter.Creatures.Spider.Den.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderDenRulesTest::RunTest(const FString& Parameters)
{
	using namespace SpiderAmbushRules;
	const FSpiderDenTuning Tuning;
	TestNearlyEqual(TEXT("The first comes out at once (no jitter)"), DenDelay(0, 0.f, Tuning), 0.f, 1e-4f);
	TestNearlyEqual(TEXT("...each next 0.45 s after the one before"), DenDelay(3, 0.f, Tuning) - DenDelay(2, 0.f, Tuning), 0.45f, 1e-4f);
	TestNearlyEqual(TEXT("...with up to a quarter second of jitter"), DenDelay(2, 1.f, Tuning) - DenDelay(2, 0.f, Tuning), 0.25f, 1e-4f);
	TestTrue(FString::Printf(TEXT("Six come out within about 2.5 s (the last %.2f s)"), DenDelay(5, 1.f, Tuning)), DenDelay(5, 1.f, Tuning) <= 2.5f + 1e-3f);
	TestTrue(TEXT("The jitter never takes one past the next"), DenDelay(1, 1.f, Tuning) < DenDelay(2, 0.f, Tuning) + Tuning.Jitter);

	// Six spiders, four dens: round the dens in turn, the first two dens sending two.
	TArray<int32> Picked;
	for (int32 Index = 0; Index < 6; ++Index)
	{
		Picked.Add(DenFor(Index, 4));
	}
	TestTrue(TEXT("Round the dens in turn (0 1 2 3 0 1)"), Picked == TArray<int32>({ 0, 1, 2, 3, 0, 1 }));
	TestEqual(TEXT("No dens: none"), DenFor(2, 0), static_cast<int32>(INDEX_NONE));

	TestTrue(TEXT("A Big hit knocks it off its wall"), KnockedOffDen(true, 0.05f, Tuning));
	TestTrue(TEXT("...so does a fifth of its health in one burst"), KnockedOffDen(false, 0.2f, Tuning));
	TestFalse(TEXT("...a rifle round doesn't"), KnockedOffDen(false, 0.08f, Tuning));

	// The crawl: its legs go first (the body hardly moves), then it pulls itself out, never sliding back.
	TestNearlyEqual(TEXT("It starts deep in the throat"), DenCrawlOut(0.f), 0.f, 1e-4f);
	TestNearlyEqual(TEXT("...and ends out over the lip"), DenCrawlOut(1.f), 1.f, 1e-4f);
	TestTrue(FString::Printf(TEXT("While its legs reach out, its body has come little of the way (%.2f)"), DenCrawlOut(0.3f)), DenCrawlOut(0.3f) < 0.15f);
	bool bOnward = true;
	for (int32 Step = 1; Step <= 200; ++Step)
	{
		bOnward &= DenCrawlOut(Step / 200.f) >= DenCrawlOut((Step - 1) / 200.f) - 1e-5f;
	}
	TestTrue(TEXT("...and never goes back"), bOnward);

	// The tip: under the mouth deep enough that it climbs a little, never past the burrow's lip or the floor.
	TestNearlyEqual(TEXT("A high mouth (3 m): it tips onto the face 1.2 m under its middle"), DenTurnBelow(300.f, 1.f, Tuning), FSpiderDenTuning::TurnBelowMost, 0.01f);
	TestNearlyEqual(TEXT("A low mouth (1.6 m): just under the lip"), DenTurnBelow(160.f, 1.f, Tuning), 50.f, 0.01f);
	TestNearlyEqual(TEXT("...never higher than the lip"), DenTurnBelow(100.f, 1.f, Tuning), FSpiderDenTuning::TurnBelowLeast, 0.01f);
	TestTrue(TEXT("A bigger spider takes longer to come out"), DenCrawlSeconds(1.8f, Tuning) > DenCrawlSeconds(1.f, Tuning)
		&& FMath::IsNearlyEqual(DenCrawlSeconds(1.f, Tuning), Tuning.CrawlSeconds));

	// Its climb down is slower and drops lower than a lurker's; a lurker keeps its own.
	FSpiderLurkState FromDen;
	FromDen.ClimbSpeed = Tuning.ClimbSpeed;
	FromDen.HopHeight = Tuning.HopHeight;
	const FSpiderLurkTuning Slow = ClimbTuning(FromDen);
	const FSpiderLurkTuning Own = ClimbTuning(FSpiderLurkState());
	TestTrue(TEXT("Out of a den it climbs at 150 cm/s and drops the last meter"), FMath::IsNearlyEqual(Slow.ClimbSpeed, 150.f) && FMath::IsNearlyEqual(Slow.ClimbHopHeight, 100.f));
	TestTrue(TEXT("...a lurker at its own"), FMath::IsNearlyEqual(Own.ClimbSpeed, FSpiderLurkTuning().ClimbSpeed) && FMath::IsNearlyEqual(Own.ClimbHopHeight, FSpiderLurkTuning().ClimbHopHeight));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderDenEmergeTest, "Looter.Creatures.Spider.Den.Emerge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderDenEmergeTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	UWorld* World = MakeWallWorld(WorldWrapper);
	if (!TestNotNull(TEXT("Test world created"), World))
	{
		return false;
	}
	// The player on the floor 8 m out; the spider spawned on its floor spot under the den.
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(-300.0, 0.0, 90.0));
	ASpiderCreature* Spider = SpawnSpider(World, FVector(350.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("A player"), Player) || !TestNotNull(TEXT("A spider"), Spider))
	{
		return false;
	}
	const UHealthComponent* Life = Spider->FindComponentByClass<UHealthComponent>();
	const bool bRig = !Spider->GetBonePose().IsEmpty();
	if (!bRig)
	{
		AddWarning(TEXT("SK_Spider isn't imported: its feet coming out aren't checked."));
	}
	if (!TestTrue(TEXT("It finds the wall at its den"), Spider->BeginDenEmerge(WallDen(), 0.3f, nullptr)))
	{
		return false;
	}
	const FSpiderDenState& Den = Spider->GetDen();
	TestTrue(TEXT("...a face under the mouth to tip onto"), Den.bFace && FMath::IsNearlyEqual(Den.ClingPoint.X, 500.0, 2.0) && Den.ClingPoint.Z < 280.0 - 47.0);
	TestTrue(FString::Printf(TEXT("...and the floor under it (%s)"), *Den.Floor.ToString()), FMath::IsNearlyEqual(Den.Floor.Z, 0.0, 2.0));
	TestTrue(TEXT("Waiting in its den: unseen"), Spider->IsInDen() && Spider->GetLurkPhase() == ESpiderLurkPhase::DenHidden && Spider->IsHidden());
	TestTrue(TEXT("...out of reach (no collision) and unhurt"), Spider->GetCapsuleComponent()->GetCollisionEnabled() == ECollisionEnabled::NoCollision
		&& Life && Life->bInvulnerable);
	TestTrue(TEXT("...held back: no target, no cry"), Spider->IsPassive() && Spider->GetTarget() == nullptr);
	TestTrue(TEXT("...its movement off"), Spider->GetCharacterMovement()->MovementMode == MOVE_None);
	const float Health = Life ? Life->GetHealth() : 0.f;
	BossTestWorld::Hurt(Spider, 50.f, Player->GetController());
	TestTrue(TEXT("A blast while it's unseen doesn't hurt it"), Life && FMath::IsNearlyEqual(Life->GetHealth(), Health));
	TickFor(*Spider, 0.2f);
	TestTrue(TEXT("Its turn not come: still in its den"), Spider->GetLurkPhase() == ESpiderLurkPhase::DenHidden && Spider->IsHidden());
	TickWhile(*Spider, ESpiderLurkPhase::DenHidden, 0.5f);
	TestTrue(TEXT("Its turn: it crawls out, seen"), Spider->GetLurkPhase() == ESpiderLurkPhase::DenCrawl && !Spider->IsHidden());
	TestTrue(TEXT("...in reach and hurt by shots"), Spider->GetCapsuleComponent()->GetCollisionEnabled() != ECollisionEnabled::NoCollision
		&& Life && !Life->bInvulnerable);
	if (bRig)
	{
		TestTrue(FString::Printf(TEXT("...its front feet still in the throat as it starts (X %.0f)"), Spider->GetPosedFoot(0, 1.f).X),
			Spider->GetPosedFoot(0, 1.f).X > 500.0);
		// Its legs come first: the front feet out on the lip while its body is still in the throat.
		TickFor(*Spider, 0.42f);
		const FVector Front = Spider->GetPosedFoot(0, 1.f);
		const FVector Back = Spider->GetPosedFoot(3, 1.f);
		TestTrue(FString::Printf(TEXT("...its front foot out gripping the lip beside the mouth (%s)"), *Front.ToString()),
			Front.X < 500.0 && Front.X > 460.0 && FMath::Abs(Front.Y) > 60.0);
		TestTrue(FString::Printf(TEXT("...its back legs still in the throat (X %.0f)"), Back.X), Back.X > 500.0);
	}
	const float Crawled = TickWhile(*Spider, ESpiderLurkPhase::DenCrawl, 2.f);
	TestTrue(FString::Printf(TEXT("Out of the mouth in about its crawl's time (%.2f s)"), Crawled), Crawled < FSpiderDenTuning().CrawlSeconds + 0.5f);
	TestTrue(TEXT("...then it tips over the lip"), Spider->GetLurkPhase() == ESpiderLurkPhase::DenTurn);
	TickWhile(*Spider, ESpiderLurkPhase::DenTurn, 1.5f);
	TestTrue(TEXT("Head down on the face, it climbs down"), Spider->GetLurkPhase() == ESpiderLurkPhase::Climbing && Spider->IsClinging()
		&& Spider->GetLurk().Forward.Equals(FVector::DownVector, 0.01));
	TestTrue(FString::Printf(TEXT("...its capsule off the wall (%s)"), *Spider->GetActorLocation().ToString()), Spider->GetActorLocation().X < 500.0);
	if (bRig)
	{
		for (int32 Pair = 0; Pair < 4; ++Pair)
		{
			const FVector Foot = Spider->GetPosedFoot(Pair, -1.f);
			TestTrue(FString::Printf(TEXT("Leg %d left's foot is on the face (X %.0f)"), Pair, Foot.X), Foot.X > 440.0 && Foot.X < 506.0);
		}
	}
	const float Climbed = TickWhile(*Spider, ESpiderLurkPhase::Climbing, 3.f);
	UCharacterMovementComponent* Movement = Spider->GetCharacterMovement();
	TestTrue(FString::Printf(TEXT("Low on the face it drops off (after %.2f s down it)"), Climbed), Spider->GetLurkPhase() == ESpiderLurkPhase::Leaping
		&& Movement->MovementMode == MOVE_Falling);
	const FSpiderLurkState& Lurk = Spider->GetLurk();
	TestTrue(FString::Printf(TEXT("...from about a meter up (%.0f cm)"), Lurk.LeapFrom.Z), Lurk.LeapFrom.Z < 250.0);
	TestTrue(FString::Printf(TEXT("...to the floor before the den, not at the player (%s)"), *Lurk.LeapTo.ToString()),
		Lurk.LeapTo.X < 500.0 && Lurk.LeapTo.X > 200.0 && FMath::Abs(Lurk.LeapTo.Y) < 50.0);

	// Set down where it was going: it lands, gathers itself, and hunts the player.
	Spider->SetActorLocation(Lurk.LeapTo);
	Movement->SetMovementMode(MOVE_Walking);
	TickWhile(*Spider, ESpiderLurkPhase::Leaping, 0.3f);
	TestTrue(TEXT("Down"), Spider->GetLurkPhase() == ESpiderLurkPhase::Landing);
	TickWhile(*Spider, ESpiderLurkPhase::Landing, 1.f);
	TestTrue(TEXT("...then hunts the player"), Spider->GetLurkPhase() == ESpiderLurkPhase::None && Spider->GetTarget() == Player);
	TestTrue(TEXT("...its hold off (not passive, out of its den)"), !Spider->IsPassive() && !Spider->GetLurk().bFromDen && !Spider->IsHidden());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderDenHitsTest, "Looter.Creatures.Spider.Den.Hits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderDenHitsTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	UWorld* World = MakeWallWorld(WorldWrapper);
	if (!TestNotNull(TEXT("Test world created"), World))
	{
		return false;
	}
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(-300.0, 0.0, 90.0));
	ASpiderCreature* Killed = SpawnSpider(World, FVector(350.0, -400.0, 0.0));
	ASpiderCreature* Knocked = SpawnSpider(World, FVector(350.0, 400.0, 0.0));
	ASpiderCreature* Lost = SpawnSpider(World, FVector(-1500.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("A player"), Player) || !TestNotNull(TEXT("Spiders"), Killed) || !TestNotNull(TEXT("..."), Knocked) || !TestNotNull(TEXT("..."), Lost))
	{
		return false;
	}
	BossTestWorld::NoLoot(Killed);

	// No wall at its den: it's left where it was spawned, as it was.
	FSpiderPerchSpot Nowhere = WallDen();
	Nowhere.Spot = FVector(-1800.0, 0.0, 280.0);
	TestFalse(TEXT("No wall at its den: it can't come out of one"), Lost->BeginDenEmerge(Nowhere, 0.f, nullptr));
	TestTrue(TEXT("...left on its floor spot, seen, not held"), !Lost->IsInDen() && !Lost->IsHidden() && !Lost->IsPassive()
		&& Lost->GetLurkPhase() == ESpiderLurkPhase::None);

	// Killed as it comes out of the mouth: its body comes down out of the wall.
	if (TestTrue(TEXT("Two spiders in their dens"), Killed->BeginDenEmerge(WallDen(-400.0), 0.f, nullptr) && Knocked->BeginDenEmerge(WallDen(400.0), 0.f, nullptr)))
	{
		const float Crawl = FSpiderDenTuning().CrawlSeconds;
		TickFor(*Killed, Crawl * 0.6f);
		TestTrue(TEXT("...one crawling out"), Killed->GetLurkPhase() == ESpiderLurkPhase::DenCrawl);
		BossTestWorld::Kill(Killed);
		Killed->Tick(Frame);
		const float Half = Killed->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		TestTrue(TEXT("Killed in the mouth, it's dead and out of its den"), Killed->IsDead() && !Killed->IsInDen() && !Killed->IsHidden());
		TestTrue(FString::Printf(TEXT("...its body set out of the wall, on the floor (%s)"), *Killed->GetActorLocation().ToString()),
			Killed->GetActorLocation().X < 500.0 && FMath::IsNearlyEqual(Killed->GetActorLocation().Z, static_cast<double>(Half), 5.0));

		// A hard hit once it's out of the mouth knocks it off: it drops (nobody to land on), then hunts.
		TickFor(*Knocked, Crawl * 0.95f);
		const UHealthComponent* Life = Knocked->FindComponentByClass<UHealthComponent>();
		BossTestWorld::Hurt(Knocked, Life ? Life->GetMaxHealth() * 0.3f : 100.f, Player->GetController());
		TestTrue(TEXT("A hard hit as it comes out knocks it off the wall"), Knocked->GetLurkPhase() == ESpiderLurkPhase::Leaping
			&& Knocked->GetCharacterMovement()->MovementMode == MOVE_Falling && !Knocked->IsDead());
		TestTrue(TEXT("...out from the wall"), Knocked->GetLurk().LeapTo.X < 500.0);
		Knocked->SetActorLocation(Knocked->GetLurk().LeapTo);
		Knocked->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		TickWhile(*Knocked, ESpiderLurkPhase::Leaping, 0.3f);
		TickWhile(*Knocked, ESpiderLurkPhase::Landing, 1.f);
		TestTrue(TEXT("...and down, it hunts"), Knocked->GetLurkPhase() == ESpiderLurkPhase::None && Knocked->GetTarget() == Player && !Knocked->IsPassive());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderDenAmbushTest, "Looter.Creatures.Spider.Den.Ambush",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderDenAmbushTest::RunTest(const FString& Parameters)
{
	// A den ambush on the floor before the wall: walked in, its wave goes into its dens in turn, each a little after the last.
	FTestWorldWrapper WorldWrapper;
	UWorld* World = MakeWallWorld(WorldWrapper);
	if (!TestNotNull(TEXT("Test world created"), World))
	{
		return false;
	}
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(-2500.0, 0.0, 90.0));
	AAmbushSpawner* Pit = World->SpawnActor<AAmbushSpawner>(FVector(0.0, 0.0, 50.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("A player"), Player) || !TestNotNull(TEXT("A den ambush"), Pit))
	{
		return false;
	}
	Pit->SpawnerId = TEXT("TestDens");
	Pit->Groups = { EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 3) };
	Pit->Entrance = EAmbushEntrance::Den;
	Pit->Dens = { WallDen(-300.0), WallDen(300.0) };
	Pit->SpawnPoints = { FVector(350.0, -300.0, -50.0), FVector(350.0, 300.0, -50.0), FVector(350.0, 0.0, -50.0) };
	Pit->AmbushCorners = { FVector(-1000.0, -1000.0, 0.0), FVector(450.0, -1000.0, 0.0), FVector(450.0, 1000.0, 0.0), FVector(-1000.0, 1000.0, 0.0) };
	Pit->AmbushMaxRise = 250.f;
	Pit->GiveUpRadius = 3000.f;
	Pit->DispatchBeginPlay();
	Pit->SetRandomSeed(20261031);
	const double PlayerZ = Player->GetActorLocation().Z;
	auto LookAt = [Player, PlayerZ, Pit](const FVector& Where)
	{
		Player->SetActorLocation(FVector(Where.X, Where.Y, PlayerZ));
		Pit->UpdateEncounter(0.5f);
	};
	LookAt(FVector(-1500.0, 0.0, 0.0));
	TestEqual(TEXT("Off its floor: nothing comes"), Pit->NumAlive(), 0);
	LookAt(FVector(-600.0, 0.0, 0.0));
	TArray<ACreatureBase*> Came = Pit->GetAliveCreatures();
	if (!TestEqual(TEXT("Walked onto its floor: its three come"), Came.Num(), 3))
	{
		return false;
	}
	TArray<float> Delays;
	int32 OnLeft = 0;
	for (ACreatureBase* Each : Came)
	{
		const ASpiderCreature* Spider = Cast<ASpiderCreature>(Each);
		if (!TestTrue(TEXT("...each in a den, unseen, hunting nobody yet"), Spider && Spider->GetLurkPhase() == ESpiderLurkPhase::DenHidden
			&& Spider->IsHidden() && Spider->GetTarget() == nullptr))
		{
			continue;
		}
		Delays.Add(Spider->GetDen().Delay);
		OnLeft += Spider->GetDen().Mouth.Y < 0.0 ? 1 : 0;
	}
	Delays.Sort();
	TestEqual(TEXT("...the dens in turn (two from the first, one from the second)"), OnLeft, 2);
	TestTrue(FString::Printf(TEXT("...each a little after the one before (%s)"), *FString::JoinBy(Delays, TEXT(", "), [](float D) { return FString::Printf(TEXT("%.2f"), D); })),
		Delays.Num() == 3 && Delays[0] <= 0.26f && Delays[1] >= 0.44f && Delays[2] >= 0.89f && Delays[2] <= 1.16f);
	return true;
}

#endif

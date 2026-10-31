#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// A lurking spider in a test level (the spiders' ambush, 2026-10-31; SpiderCreatureLurk.cpp, SpiderCreatureCling.cpp): it
// finds the wall near its perch and holds it, head down, its feet on the face, still; with nothing near its perch it lurks
// on the ground; a player passing under it springs it: its crouch, a leap aimed short of them (the launch lands where it
// was aimed), and coming down beside them it bites and hunts them. A test level never moves anything, so the leap's flight
// is checked by its launch, and its landing by setting it down where it was going (Docs/Pipeline/Workflow.md, "Tests").
// The rules alone: SpiderAmbushTests.cpp.

#include "Combat/HealthComponent.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/FightEvent.h"
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
	UWorld* MakeWallWorld(FTestWorldWrapper& Wrapper, bool bDirector = false)
	{
		// With the fight director (made in a test level only when a test lets it): its fight log counts a bite that lands. A test
		// level has no game mode, so a pawn ignores ApplyDamage (APawn::ShouldTakeDamage) and the player's health never shows it.
		UFightDirectorSubsystem::bAllowInTestWorlds = bDirector;
		const bool bMade = Wrapper.CreateTestWorld(EWorldType::EditorPreview);
		UFightDirectorSubsystem::bAllowInTestWorlds = false;
		if (!bMade)
		{
			return nullptr;
		}
		UWorld* World = Wrapper.GetTestWorld();
		EncounterTestWorld::SpawnBlocks(World, { FBox(FVector(-3000.0, -3000.0, -100.0), FVector(3000.0, 3000.0, 0.0)),
			FBox(FVector(500.0, -800.0, 0.0), FVector(900.0, 800.0, 900.0)) });
		return World;
	}

	/** A brown spider standing at Feet, its movement given its capsule (a test level's has none), so it can be held and launched. */
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

	FSpiderPerchSpot WallPerch()
	{
		FSpiderPerchSpot Perch;
		Perch.Kind = ESpiderPerchKind::Wall;
		Perch.Spot = FVector(500.0, 0.0, 380.0);
		Perch.FaceYaw = 180.f;
		return Perch;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderAmbushClingTest, "Looter.Creatures.Spider.Ambush.Cling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderAmbushClingTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	UWorld* World = MakeWallWorld(WorldWrapper);
	if (!TestNotNull(TEXT("Test world created"), World))
	{
		return false;
	}
	ASpiderCreature* Spider = SpawnSpider(World, FVector(250.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("A spider"), Spider))
	{
		return false;
	}
	TestTrue(TEXT("It finds the wall near its perch"), Spider->BeginLurk(WallPerch(), nullptr));
	const FSpiderLurkState& Lurk = Spider->GetLurk();
	TestTrue(TEXT("...and waits on it"), Spider->GetLurkPhase() == ESpiderLurkPhase::Perched && Spider->IsClinging());
	TestTrue(FString::Printf(TEXT("...on the face (%s), its back out from it"), *Lurk.Point.ToString()),
		FMath::IsNearlyEqual(Lurk.Point.X, 500.0, 1.0) && Lurk.Normal.Equals(FVector(-1.0, 0.0, 0.0), 0.01));
	TestTrue(TEXT("...head down"), Lurk.Forward.Equals(FVector::DownVector, 0.01));
	TestTrue(FString::Printf(TEXT("Its distances are from the ground under it (%s)"), *Lurk.Foot.ToString()),
		FMath::IsNearlyEqual(Lurk.Foot.Z, 0.0, 2.0) && Lurk.Foot.X < 500.0);
	TestTrue(TEXT("Held there: no walking, no falling"), Spider->GetCharacterMovement()->MovementMode == MOVE_None);
	const FVector Held = Spider->GetActorLocation();
	TestTrue(FString::Printf(TEXT("...its capsule just off the wall (%s)"), *Held.ToString()), Held.X < 500.0 && Held.X > 400.0 && Held.Z > 300.0);
	for (float Elapsed = 0.f; Elapsed < 1.f; Elapsed += Frame)
	{
		Spider->Tick(Frame);
	}
	TestTrue(TEXT("With nobody near, it stays: still on its perch a second on"),
		Spider->GetLurkPhase() == ESpiderLurkPhase::Perched && Spider->GetActorLocation().Equals(Held, 0.5));
	TestTrue(TEXT("...hunting nobody"), Spider->GetTarget() == nullptr);
	if (Spider->GetBonePose().IsEmpty())
	{
		AddWarning(TEXT("SK_Spider isn't imported: its feet on the wall aren't checked."));
	}
	else
	{
		double Highest = -1e9;
		double Lowest = 1e9;
		for (int32 Pair = 0; Pair < 4; ++Pair)
		{
			for (const float Side : { -1.f, 1.f })
			{
				const FVector Foot = Spider->GetPosedFoot(Pair, Side);
				TestTrue(FString::Printf(TEXT("Leg %d %s's foot is on the face (X %.0f)"), Pair, Side < 0.f ? TEXT("left") : TEXT("right"), Foot.X),
					Foot.X > 470.0 && Foot.X < 506.0);
				Highest = FMath::Max(Highest, Foot.Z);
				Lowest = FMath::Min(Lowest, Foot.Z);
			}
		}
		TestTrue(FString::Printf(TEXT("Its legs are splayed up and down the face (%.0f cm apart)"), Highest - Lowest), Highest - Lowest > 80.0);
	}

	// With nothing to hold near its perch it lurks crouched on the ground where it stands.
	ASpiderCreature* Lost = SpawnSpider(World, FVector(-1200.0, 600.0, 0.0));
	if (TestNotNull(TEXT("A second spider"), Lost))
	{
		FSpiderPerchSpot Nowhere = WallPerch();
		Nowhere.Spot = FVector(-1500.0, 600.0, 380.0);
		TestFalse(TEXT("No wall near its perch: none found"), Lost->BeginLurk(Nowhere, nullptr));
		TestTrue(TEXT("...it lurks on the ground, its back up"), Lost->GetLurkPhase() == ESpiderLurkPhase::Perched
			&& Lost->GetLurk().Surface == ESpiderSurface::Ledge && Lost->GetLurk().Normal.Equals(FVector::UpVector, 0.01));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderAmbushPounceTest, "Looter.Creatures.Spider.Ambush.Pounce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderAmbushPounceTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	UWorld* World = MakeWallWorld(WorldWrapper, /*bDirector*/ true);
	if (!TestNotNull(TEXT("Test world created"), World))
	{
		return false;
	}
	ASpiderCreature* Spider = SpawnSpider(World, FVector(250.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("A spider"), Spider) || !TestTrue(TEXT("...on the wall"), Spider->BeginLurk(WallPerch(), nullptr)))
	{
		return false;
	}
	// A player walks in under it (4 m out from the wall's foot), as the level's player.
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(100.0, 60.0, 90.0));
	UHealthComponent* PlayerHealth = Player ? Player->FindComponentByClass<UHealthComponent>() : nullptr;
	if (!TestNotNull(TEXT("A player"), Player) || !TestNotNull(TEXT("...with health"), PlayerHealth))
	{
		return false;
	}
	float Elapsed = 0.f;
	while (Spider->GetLurkPhase() == ESpiderLurkPhase::Perched && Elapsed < 1.f)
	{
		Spider->Tick(Frame);
		Elapsed += Frame;
	}
	TestTrue(FString::Printf(TEXT("A player under it springs it (in %.2f s)"), Elapsed), Spider->GetLurkPhase() == ESpiderLurkPhase::Coiling);
	TestTrue(TEXT("...its hunt begun on them"), Spider->GetTarget() == Player);
	const FSpiderLurkTuning Tuning;
	Elapsed = 0.f;
	while (Spider->GetLurkPhase() == ESpiderLurkPhase::Coiling && Elapsed < 1.f)
	{
		Spider->Tick(Frame);
		Elapsed += Frame;
	}
	TestTrue(FString::Printf(TEXT("Its crouch (the tell) lasts about %.2f s (%.2f)"), Tuning.CoilSeconds, Elapsed),
		FMath::IsNearlyEqual(Elapsed, Tuning.CoilSeconds, 2.f * Frame));
	UCharacterMovementComponent* Movement = Spider->GetCharacterMovement();
	if (!TestTrue(TEXT("...then it leaps"), Spider->GetLurkPhase() == ESpiderLurkPhase::Leaping && Movement->MovementMode == MOVE_Falling))
	{
		return false;
	}
	// The launch lands where it was aimed: short of the player, along its way, on their ground.
	const FSpiderLurkState& Lurk = Spider->GetLurk();
	const float Seconds = SpiderAmbushRules::LeapSeconds(Lurk.LeapFrom, Lurk.LeapTo);
	const FVector Lands = SpiderAmbushRules::LeapPositionAt(Lurk.LeapFrom, Movement->Velocity, Seconds, Movement->GetGravityZ());
	TestTrue(FString::Printf(TEXT("Its launch carries it to its aim (%.1f cm off)"), FVector::Dist(Lands, Lurk.LeapTo)), FVector::Dist(Lands, Lurk.LeapTo) < 2.0);
	const float Short = static_cast<float>(FVector::Dist2D(Lurk.LeapTo, Player->GetActorLocation()));
	TestTrue(FString::Printf(TEXT("...a little short of the player (%.0f cm)"), Short), Short < Tuning.LandShort + 10.f);
	TestTrue(TEXT("...off its wall"), Lurk.LeapFrom.X < 500.0 - Spider->GetCapsuleComponent()->GetScaledCapsuleRadius());

	// Set down where it was going: it bites as it lands, gathers itself, then hunts them.
	const UFightDirectorSubsystem* Director = World->GetSubsystem<UFightDirectorSubsystem>();
	if (!TestNotNull(TEXT("A fight director, to count the bite"), Director))
	{
		return false;
	}
	const int32 Before = Director->GetEventCount(EFightEvent::AttackLanded);
	Spider->SetActorLocation(Lurk.LeapTo);
	Movement->SetMovementMode(MOVE_Walking);
	for (Elapsed = 0.f; Spider->GetLurkPhase() == ESpiderLurkPhase::Leaping && Elapsed < 0.3f; Elapsed += Frame)
	{
		Spider->Tick(Frame);
	}
	TestTrue(TEXT("Down beside them"), Spider->GetLurkPhase() == ESpiderLurkPhase::Landing);
	TestEqual(TEXT("...it bites as it lands (one attack landed)"), Director->GetEventCount(EFightEvent::AttackLanded) - Before, 1);
	for (Elapsed = 0.f; Elapsed < Tuning.LandRecover + 0.2f; Elapsed += Frame)
	{
		Spider->Tick(Frame);
		Spider->ConsumeMovementInputVector();
	}
	TestTrue(TEXT("...then fights as any spider"), Spider->GetLurkPhase() == ESpiderLurkPhase::None && Spider->GetTarget() == Player);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderAmbushMatesTest, "Looter.Creatures.Spider.Ambush.Mates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderAmbushMatesTest::RunTest(const FString& Parameters)
{
	// Shot on its perch it goes at once; a mate's call brings it a moment later; a player off behind cover doesn't.
	FTestWorldWrapper WorldWrapper;
	UWorld* World = MakeWallWorld(WorldWrapper);
	if (!TestNotNull(TEXT("Test world created"), World))
	{
		return false;
	}
	ASpiderCreature* Shot = SpawnSpider(World, FVector(250.0, -300.0, 0.0));
	ASpiderCreature* Called = SpawnSpider(World, FVector(250.0, 300.0, 0.0));
	FSpiderPerchSpot Left = WallPerch();
	Left.Spot.Y = -300.0;
	FSpiderPerchSpot Right = WallPerch();
	Right.Spot.Y = 300.0;
	if (!TestNotNull(TEXT("Two spiders"), Shot) || !TestNotNull(TEXT("..."), Called) || !TestTrue(TEXT("...on the wall"),
		Shot->BeginLurk(Left, nullptr) && Called->BeginLurk(Right, nullptr)))
	{
		return false;
	}
	// The player 25 m off: too far to spring either by passing.
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(-2200.0, 0.0, 90.0));
	if (!TestNotNull(TEXT("A player"), Player))
	{
		return false;
	}
	for (float Elapsed = 0.f; Elapsed < 0.5f; Elapsed += Frame)
	{
		Shot->Tick(Frame);
		Called->Tick(Frame);
	}
	TestTrue(TEXT("Far off, neither springs"), Shot->GetLurkPhase() == ESpiderLurkPhase::Perched && Called->GetLurkPhase() == ESpiderLurkPhase::Perched);
	BossTestWorld::Hurt(Shot, 5.f, Player->GetController());
	Shot->Tick(Frame);
	TestTrue(TEXT("Shot, it springs at once (down its wall: they're out of a leap's reach)"), Shot->GetLurkPhase() == ESpiderLurkPhase::Climbing
		|| Shot->GetLurkPhase() == ESpiderLurkPhase::Coiling);
	Called->NoteMateSprang(Player, 0.5f);
	Called->Tick(Frame);
	TestTrue(TEXT("A mate's call waits its moment"), Called->GetLurkPhase() == ESpiderLurkPhase::Perched);
	for (float Elapsed = 0.f; Elapsed < 0.6f && Called->GetLurkPhase() == ESpiderLurkPhase::Perched; Elapsed += Frame)
	{
		Called->Tick(Frame);
	}
	TestTrue(TEXT("...then it goes too"), Called->GetLurkPhase() != ESpiderLurkPhase::Perched);
	return true;
}

#endif

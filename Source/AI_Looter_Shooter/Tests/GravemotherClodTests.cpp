#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The Gravemother's Grave Clod (the user's call, 2026-10-15/19: "front legs dig in and lift a clod overhead (the tell,
// 1.2 s); she lobs it (about 1.5 s to a player at 20 m; a landing shadow shows where); it bursts into dust and leaves a
// rubble pile (cover, 10 s, then it settles away). Shooting the clod while she holds it bursts it on her and stuns her 2 s";
// GravemotherCreatureClod.cpp, AGravemotherClod, AGravemotherRubble).

#include "Bosses/BossComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/GravemotherClod.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/GravemotherFight.h"
#include "Creatures/GravemotherRubble.h"
#include "Creatures/SpitterRules.h"
#include "Tests/BossTestWorld.h"
#include "Tests/GravemotherTestWorld.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"

using namespace GravemotherTestWorld;

namespace
{
	/** A mother in her fight at Feet with a player Distance ahead on a floor, past her roar, her clod ready. */
	AGravemotherCreature* MotherAtClodRange(FAutomationTestBase& Test, UWorld* World, const FVector& Feet, double Distance, ACharacter*& OutPlayer)
	{
		SpawnFloor(World, Feet + FVector(Distance * 0.5, 0.0, 0.0), Distance + 1500.0);
		OutPlayer = BossTestWorld::SpawnPlayer(World, Feet + FVector(Distance, 0.0, 90.0));
		AGravemotherCreature* Mother = SpawnMother(World, Feet);
		if (!Test.TestNotNull(TEXT("Player stand-in"), OutPlayer) || !Test.TestNotNull(TEXT("The Gravemother"), Mother))
		{
			return nullptr;
		}
		Mother->GetBoss()->StartFight(OutPlayer);
		Mother->AlertTo(OutPlayer);
		Mother->ReadyClod();
		return Mother;
	}

	/** Ticks her until her clod's tell starts; the time it took, or negative. */
	float UntilClodTell(AGravemotherCreature* Mother)
	{
		return TickUntil(Mother, 4.f, [Mother]()
		{
			return Mother->GetMove() == EGravemotherMove::Clod && Mother->GetCreatureState() == ECreatureState::Attack;
		});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherClodRulesTest, "Looter.Creatures.Gravemother.Clod.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherClodRulesTest::RunTest(const FString& Parameters)
{
	// About 1.5 s to a player at 20 m (the user's), quicker nearer (never under 0.8 s), and its lob comes down where it's aimed.
	TestNearlyEqual(TEXT("1.5 s to 20 m"), GravemotherFight::ClodFlightSeconds(2000.f), 1.5f, 0.001f);
	TestTrue(TEXT("Quicker nearer"), GravemotherFight::ClodFlightSeconds(1000.f) < GravemotherFight::ClodFlightSeconds(2000.f)
		&& GravemotherFight::ClodFlightSeconds(1000.f) > 0.9f);
	TestNearlyEqual(TEXT("Never under 0.8 s"), GravemotherFight::ClodFlightSeconds(50.f), 0.8f, 0.001f);
	TestNearlyEqual(TEXT("Never over 1.7 s"), GravemotherFight::ClodFlightSeconds(9000.f), 1.7f, 0.001f);
	const FGravemotherClodSettings Clods;
	TestNearlyEqual(TEXT("The user's tell: 1.2 s"), Clods.Tell, 1.2f, 0.001f);
	TestNearlyEqual(TEXT("The user's stun: 2 s"), Clods.StunSeconds, 2.f, 0.001f);
	TestNearlyEqual(TEXT("The user's rubble: 10 s"), Clods.RubbleSeconds, 10.f, 0.001f);
	TestTrue(TEXT("Torn up, then lifted, within the tell"), Clods.TearAt < Clods.LiftedAt && Clods.LiftedAt < Clods.Tell);
	const FVector From(0.0, 0.0, 400.0);
	const FVector Aim(1800.0, 300.0, 0.0);
	const float Seconds = GravemotherFight::ClodFlightSeconds(static_cast<float>(FVector::Dist2D(From, Aim)));
	const FVector Velocity = SpitterRules::LobVelocity(From, Aim, Seconds, Clods.Gravity);
	TestTrue(TEXT("Its lob comes down where it's aimed"), SpitterRules::LobPoint(From, Velocity, Seconds, Clods.Gravity).Equals(Aim, 1.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherClodLobTest, "Looter.Creatures.Gravemother.Clod.Lob",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherClodLobTest::RunTest(const FString& Parameters)
{
	// In her fight, at a player 18 m off: her front legs dig in, the clod comes up between them at TearAt and is held over
	// her (shootable) until the tell's end, then lobbed at where they stand; it comes down on them about when it should,
	// hurts them, and leaves rubble there: cover for 10 s, then it settles away.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = nullptr;
	AGravemotherCreature* Mother = MotherAtClodRange(*this, World, FVector::ZeroVector, 1800.0, Player);
	if (!Mother)
	{
		return false;
	}
	const float TellAt = UntilClodTell(Mother);
	if (!TestTrue(TEXT("After her roar, at 18 m: the clod's tell"), TellAt >= 0.f))
	{
		return false;
	}
	TestNearlyEqual(TEXT("...its wind-up the user's 1.2 s"), Mother->AttackWindup, Mother->Clods.Tell, 0.001f);
	TickFor(Mother, Mother->Clods.TearAt - 0.1f);
	TestNull(TEXT("Digging in: no clod yet"), Mother->GetHeldClod());
	TickFor(Mother, 0.2f);
	AGravemotherClod* Clod = Mother->GetHeldClod();
	if (!TestNotNull(TEXT("Torn up: a clod in her grip"), Clod))
	{
		return false;
	}
	TestTrue(TEXT("...shootable (the Weapon channel), showing its own hits"), Clod->IsHeld() && Clod->GetHitZone()->GetCollisionEnabled() != ECollisionEnabled::NoCollision
		&& Clod->GetHitZone()->GetCollisionResponseToChannel(ECC_GameTraceChannel2) == ECR_Block
		&& Clod->GetHitZone()->ComponentHasTag(FName(UBulletSubsystem::OwnLookTag)));
	TickFor(Mother, Mother->Clods.LiftedAt - Mother->Clods.TearAt);
	TestTrue(*FString::Printf(TEXT("Lifted over her (%.0f cm over her middle)"), Clod->GetActorLocation().Z - Mother->GetActorLocation().Z),
		Clod->GetActorLocation().Z > Mother->GetActorLocation().Z + 60.0);
	const float HeldFor = TickUntil(Mother, 1.f, [Mother]() { return Mother->GetHeldClod() == nullptr; });
	TestTrue(TEXT("Thrown at the tell's end"), HeldFor >= 0.f && Mother->GetThrownClod() == Clod && Clod->IsFlying());
	TestFalse(TEXT("...and her clod cools down"), Mother->IsClodReady());
	const float Planned = Clod->GetPlannedSeconds();
	const float Wanted = GravemotherFight::ClodFlightSeconds(static_cast<float>(FVector::Dist2D(Clod->GetActorLocation(), Player->GetActorLocation())));
	TestTrue(*FString::Printf(TEXT("Its flight planned about as the rule says (%.2f s, %.2f s)"), Planned, Wanted), FMath::Abs(Planned - Wanted) < 0.25f);
	TestTrue(*FString::Printf(TEXT("...coming down at their feet (%.0f cm off)"), FVector::Dist2D(Clod->GetPlannedLanding(), Player->GetActorLocation())),
		FVector::Dist2D(Clod->GetPlannedLanding(), Player->GetActorLocation()) < 120.0);
	float Flown = 0.f;
	while (Flown < 3.f && !Clod->HasLanded())
	{
		Clod->Advance(Step);
		Flown += Step;
	}
	TestTrue(*FString::Printf(TEXT("It came down on them (%.2f s)"), Flown), Clod->HasLanded() && Clod->GetPlayersHit() >= 1);
	AGravemotherRubble* Rubble = Clod->GetRubble();
	if (!TestNotNull(TEXT("...leaving rubble there"), Rubble))
	{
		return false;
	}
	TestTrue(TEXT("...on the ground at their feet"), FVector::Dist2D(Rubble->GetActorLocation(), Player->GetActorLocation()) < 150.0
		&& FMath::Abs(Rubble->GetActorLocation().Z) < 5.0);
	TestFalse(TEXT("...not trapping the player it came down on"), Rubble->BlocksPawns());
	Player->SetActorLocation(Player->GetActorLocation() + FVector(0.0, 600.0, 0.0));
	Rubble->Advance(Step);
	TestTrue(TEXT("Clear of them, it's cover: it stops walkers and bullets"), Rubble->BlocksPawns()
		&& Rubble->GetPile()->GetCollisionResponseToChannel(ECC_GameTraceChannel2) == ECR_Block);
	Rubble->Advance(Mother->Clods.RubbleSeconds);
	TestTrue(TEXT("At 10 s it settles away: cover no more"), Rubble->IsSettling() && Rubble->GetPile()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	const TWeakObjectPtr<AGravemotherRubble> Kept = Rubble;
	Rubble->Advance(Mother->Clods.RubbleSettleSeconds + Step);
	TestTrue(TEXT("...and it's gone"), !Kept.IsValid() || Kept->IsActorBeingDestroyed());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherClodCounteredTest, "Looter.Creatures.Gravemother.Clod.Countered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherClodCounteredTest::RunTest(const FString& Parameters)
{
	// Shot in her grip it bursts on her: nothing is thrown and she's stunned 2 s (her boss's stagger, stars over her head),
	// then she's up. Staggered with it in her grip, it falls and bursts harmlessly, no rubble.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = nullptr;
	AGravemotherCreature* Mother = MotherAtClodRange(*this, World, FVector::ZeroVector, 1800.0, Player);
	if (!Mother || !TestTrue(TEXT("Her clod's tell"), UntilClodTell(Mother) >= 0.f))
	{
		return false;
	}
	TickFor(Mother, Mother->Clods.TearAt + 0.2f);
	AGravemotherClod* Clod = Mother->GetHeldClod();
	if (!TestNotNull(TEXT("A clod in her grip"), Clod))
	{
		return false;
	}
	BossTestWorld::Hurt(Clod, 50.f);
	UBossComponent* Boss = Mother->GetBoss();
	TestTrue(TEXT("Shot in her grip: it bursts on her"), Clod->WasCountered() && Mother->GetHeldClod() == nullptr);
	TestTrue(TEXT("...and she's stunned: her boss's stagger"), Mother->IsStunned() && Mother->IsReeling() && Boss->IsStaggered());
	TickFor(Mother, Mother->Clods.Tell);
	TestNull(TEXT("Nothing thrown"), Mother->GetThrownClod());
	TestTrue(TEXT("...still stunned a moment on"), Mother->IsStunned());
	Boss->TickFight(Mother->Clods.StunSeconds + 0.1f);
	TestTrue(TEXT("2 s on she's up"), !Mother->IsStunned() && !Mother->IsReeling() && !Boss->IsStaggered());

	// Staggered with a clod in her grip: it falls and bursts harmlessly.
	ACharacter* Other = nullptr;
	AGravemotherCreature* Dropper = MotherAtClodRange(*this, World, FVector(0.0, 9000.0, 0.0), 1800.0, Other);
	if (!Dropper || !TestTrue(TEXT("Another's tell"), UntilClodTell(Dropper) >= 0.f))
	{
		return false;
	}
	TickFor(Dropper, Dropper->Clods.TearAt + 0.2f);
	AGravemotherClod* Held = Dropper->GetHeldClod();
	if (!TestNotNull(TEXT("A clod in her grip"), Held))
	{
		return false;
	}
	TestTrue(TEXT("Staggered"), Dropper->GetBoss()->BeginStagger());
	Dropper->Tick(Step);
	TestTrue(TEXT("...her grip lost: it falls"), Held->WasDropped() && Held->IsFlying() && Dropper->GetHeldClod() == nullptr);
	float Fell = 0.f;
	while (Fell < 3.f && !Held->HasLanded())
	{
		Held->Advance(Step);
		Fell += Step;
	}
	TestTrue(TEXT("...and bursts harmlessly: no one hit, no rubble"), Held->HasLanded() && Held->GetPlayersHit() == 0 && !Held->GetRubble());
	return true;
}

#endif

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The Venom Spitter (ASpitterSpider, ASpitterGlob, ASpitterPuddle, SpitterRules): its rules on their own, its tell before
// every spit and its pace, a lob coming down on a target standing still and missing one that moved, its puddle's burn and
// drying, its weak spots (and a spit choked by a shot to its sac), and its bestiary page. SpitterPlayTests.cpp has it
// keeping its distance on a floor.

#include "Bestiary/BestiaryEntry.h"
#include "Combat/HealthComponent.h"
#include "Creatures/FightTuning.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/SpitterGlob.h"
#include "Creatures/SpitterPuddle.h"
#include "Creatures/SpitterRules.h"
#include "Creatures/SpitterSpider.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

namespace
{
	constexpr float SpitterStep = 1.f / 60.f;

	ASpitterSpider* SpawnSpitter(UWorld* World, const FVector& Feet)
	{
		return Cast<ASpitterSpider>(EncounterTestWorld::SpawnCreature(World, ASpitterSpider::StaticClass(), Feet));
	}

	/** Moves it on a frame at a time until Done holds (at most Limit seconds); the seconds it took, or negative. */
	float TickSpitterUntil(ASpitterSpider* Spitter, float Limit, TFunctionRef<bool()> Done)
	{
		for (float Time = 0.f; Spitter && Time <= Limit; Time += SpitterStep)
		{
			if (Done())
			{
				return Time;
			}
			Spitter->Tick(SpitterStep);
		}
		return -1.f;
	}

	void TickSpitterFor(ASpitterSpider* Spitter, float Seconds)
	{
		for (float Time = 0.f; Spitter && Time < Seconds - KINDA_SMALL_NUMBER; Time += SpitterStep)
		{
			Spitter->Tick(SpitterStep);
		}
	}

	/** Flies a glob a frame at a time until it lands (at most Limit seconds); the seconds it flew, or negative. */
	float FlyGlob(ASpitterGlob* Glob, float Limit)
	{
		for (float Time = 0.f; Glob && Time <= Limit; Time += SpitterStep)
		{
			if (Glob->HasLanded())
			{
				return Time;
			}
			Glob->Tick(SpitterStep);
		}
		return -1.f;
	}

	int32 NumGlobs(UWorld* World)
	{
		int32 Count = 0;
		for (TActorIterator<ASpitterGlob> It(World); It; ++It)
		{
			Count += IsValid(*It) ? 1 : 0;
		}
		return Count;
	}

	/** A floor 60 m square with its top at z = 0. */
	void SpawnFloor(UWorld* World)
	{
		LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	}

	/** The player's stand-in standing on that floor at (X, Y). */
	ACharacter* SpawnStandingPlayer(UWorld* World, double X, double Y)
	{
		return EncounterTestWorld::SpawnPlayer(World, FVector(X, Y, 89.0));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpitterRulesTest, "Looter.Creatures.Spitter.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpitterRulesTest::RunTest(const FString& Parameters)
{
	// A lob comes down where it was aimed, after its flight, arcing well over the straight line.
	const FVector Start(0.0, 0.0, 60.0);
	const FVector Target(1400.0, 300.0, 0.0);
	const float Seconds = SpitterRules::FlightSeconds(static_cast<float>(FVector::Dist2D(Start, Target)));
	const FVector Velocity = SpitterRules::LobVelocity(Start, Target, Seconds);
	TestTrue(TEXT("A 14 m lob lands where it was aimed"), FVector::Dist(SpitterRules::LobPoint(Start, Velocity, Seconds), Target) < 0.5);
	TestTrue(TEXT("...over a second's flight"), Seconds > 0.9f && Seconds < SpitterRules::LongestFlight);
	const FVector Middle = SpitterRules::LobPoint(Start, Velocity, Seconds * 0.5f);
	TestTrue(TEXT("...arcing more than 2 m over its line at the top"), Middle.Z - (Start.Z + Target.Z) * 0.5 > 200.0);
	TestEqual(TEXT("A toss at its feet takes the shortest flight"), SpitterRules::FlightSeconds(0.f), SpitterRules::ShortestFlight);
	TestEqual(TEXT("...a far one the longest"), SpitterRules::FlightSeconds(10000.f), SpitterRules::LongestFlight);
	TestTrue(TEXT("...and a farther throw never flies shorter"), SpitterRules::FlightSeconds(1800.f) >= SpitterRules::FlightSeconds(900.f));

	// It leads a moving target by a share of the flight, along the ground only.
	const FVector Led = SpitterRules::AimPoint(FVector(1000.0, 0.0, 0.0), FVector(300.0, 0.0, 200.0), 1.f, 0.5f);
	TestTrue(TEXT("Half a lead on a target running 3 m/s for a 1 s flight: 1.5 m ahead, level"), Led.Equals(FVector(1150.0, 0.0, 0.0), 0.01));

	// What it does about its distance.
	TestTrue(TEXT("Far off: its brain brings it in"), SpitterRules::ChooseKeep(2000.f, 1000.f, 1600.f, true, true) == ESpitterKeep::Close);
	TestTrue(TEXT("In its band: it holds..."), SpitterRules::ChooseKeep(1300.f, 1000.f, 1600.f, true, false) == ESpitterKeep::Hold);
	TestTrue(TEXT("...or strafes"), SpitterRules::ChooseKeep(1300.f, 1000.f, 1600.f, true, true) == ESpitterKeep::Strafe);
	TestTrue(TEXT("Too close: it backs away"), SpitterRules::ChooseKeep(600.f, 1000.f, 1600.f, true, true) == ESpitterKeep::BackOff);
	TestTrue(TEXT("...or with no way back it's cornered"), SpitterRules::ChooseKeep(600.f, 1000.f, 1600.f, false, true) == ESpitterKeep::Cornered);

	// When it spits and when it bites.
	TestTrue(TEXT("It spits at a target in range when ready"), SpitterRules::CanSpit(1500.f, 300.f, 2200.f, true));
	TestFalse(TEXT("...not before"), SpitterRules::CanSpit(1500.f, 300.f, 2200.f, false));
	TestFalse(TEXT("...nor out of range"), SpitterRules::CanSpit(2500.f, 300.f, 2200.f, true) || SpitterRules::CanSpit(200.f, 300.f, 2200.f, true));
	TestTrue(TEXT("It bites one that caught it"), SpitterRules::WantsBite(200.f, 230.f, 260.f, false, true));
	TestTrue(TEXT("...or one in reach when it's cornered"), SpitterRules::WantsBite(225.f, 230.f, 200.f, true, false) == false
		&& SpitterRules::WantsBite(225.f, 230.f, 200.f, true, true));
	TestFalse(TEXT("...never one it could back away from"), SpitterRules::WantsBite(225.f, 230.f, 200.f, false, true));
	TestFalse(TEXT("...nor one out of reach, cornered or not"), SpitterRules::WantsBite(400.f, 230.f, 260.f, true, true));

	// Its rank's traits.
	TestEqual(TEXT("A Basic or Restless one throws one glob"), SpitterRules::GlobsFor(ECreatureRank::Basic) + SpitterRules::GlobsFor(ECreatureRank::Rare), 2);
	TestEqual(TEXT("...a Gravebound one three"), SpitterRules::GlobsFor(ECreatureRank::Epic), 3);
	TestTrue(TEXT("A Restless one spits sooner than a Basic one"), SpitterRules::IntervalShareFor(ECreatureRank::Rare) < SpitterRules::IntervalShareFor(ECreatureRank::Basic));

	// Whom a puddle burns: feet in it on its level; a toe in it counts.
	const FVector Pool(0.0, 0.0, 0.0);
	TestTrue(TEXT("Standing in it"), SpitterRules::PuddleTouches(Pool, 130.f, FVector(50.0, 0.0, 1.0), 34.f));
	TestTrue(TEXT("...a toe over its edge"), SpitterRules::PuddleTouches(Pool, 130.f, FVector(140.0, 0.0, 1.0), 34.f));
	TestFalse(TEXT("...not a step past it"), SpitterRules::PuddleTouches(Pool, 130.f, FVector(200.0, 0.0, 1.0), 34.f));
	TestFalse(TEXT("...nor on a ledge over it"), SpitterRules::PuddleTouches(Pool, 130.f, FVector(0.0, 0.0, 150.0), 34.f));
	TestTrue(TEXT("It spreads out, lies, then dries up"), SpitterRules::PuddleSpread(0.f, 5.f) == 0.f
		&& FMath::IsNearlyEqual(SpitterRules::PuddleSpread(2.5f, 5.f), 1.f) && SpitterRules::PuddleSpread(4.8f, 5.f) < 1.f
		&& SpitterRules::PuddleSpread(5.f, 5.f) == 0.f);

	// Its numbers: a spider's experience, the head and the sac critical, its band inside its spit's reach, a readable tell.
	const ASpitterSpider* Spitter = GetDefault<ASpitterSpider>();
	const ASpiderCreature* Spider = GetDefault<ASpiderCreature>();
	TestEqual(TEXT("A kill pays as a spider's does"), Spitter->XPReward, Spider->XPReward);
	TestTrue(TEXT("Its head and its sac are critical"), Spitter->CriticalSpotBones.Contains(FName(TEXT("head"))) && Spitter->CriticalSpotBones.Contains(SpitterLook::SacBone));
	// 9 m since the fight contract (2026-10-21; was 10): Looter.Fight.RangedBackOff stands for it in play, with the same default.
	TestTrue(TEXT("It keeps 9 to 16 m off, inside its 22 m spit"), Spitter->BackOffWithin == 900.f && Spitter->CloseInTo <= 1800.f
		&& Spitter->CloseInTo > Spitter->BackOffWithin && Spitter->SpitMaxDistance > Spitter->CloseInTo);
	TestEqual(TEXT("...the fight's back-off the same"), FFightTuning().RangedBackOff, Spitter->BackOffWithin);
	TestTrue(TEXT("Its brain's chase stops where its band begins"), FMath::IsNearlyEqual(Spitter->AttackRange * 0.75f, Spitter->CloseInTo, 1.f));
	TestTrue(TEXT("A spit every 3 s after a tell of most of a second"), FMath::IsNearlyEqual(Spitter->SpitEvery, 3.f) && Spitter->SpitTell >= 0.7f);
	TestTrue(TEXT("Its bite is softer than a spider's"), Spitter->AttackDamage < Spider->AttackDamage);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpitterTellTest, "Looter.Creatures.Spitter.Tell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpitterTellTest::RunTest(const FString& Parameters)
{
	// 14 m off a player it has just turned on: it sizes them up, then tells (rears, sac swelling and glowing) before every
	// glob, and spits about every 3 s.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(1400.0, 0.0, 0.0));
	ASpitterSpider* Spitter = SpawnSpitter(World, FVector::ZeroVector);
	ASpiderCreature* Spider = Cast<ASpiderCreature>(EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), FVector(0.0, 5000.0, 0.0)));
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("A Spitter"), Spitter) || !TestNotNull(TEXT("A spider"), Spider))
	{
		return false;
	}
	const UHealthComponent* SpitterHealth = Spitter->FindComponentByClass<UHealthComponent>();
	const UHealthComponent* SpiderHealth = Spider->FindComponentByClass<UHealthComponent>();
	TestTrue(TEXT("Frailer than a spider"), SpitterHealth && SpiderHealth && SpitterHealth->GetMaxHealth() < SpiderHealth->GetMaxHealth());

	Spitter->AlertTo(Player);
	TestTrue(TEXT("It hunts the player"), Spitter->GetCreatureState() == ECreatureState::Chase);
	TickSpitterFor(Spitter, 0.5f);
	TestTrue(TEXT("It sizes them up before its first spit"), Spitter->GetCreatureState() != ECreatureState::Attack);

	const float TellAt = TickSpitterUntil(Spitter, 3.f, [Spitter]() { return Spitter->IsTelling(); });
	if (!TestTrue(FString::Printf(TEXT("Then it rears to spit (%.2f s)"), TellAt + 0.5f), TellAt >= 0.f))
	{
		return false;
	}
	TestTrue(TEXT("...its move a spit, its attack's wind-up its tell"), Spitter->GetMove() == ESpitterMove::Spit
		&& FMath::IsNearlyEqual(Spitter->AttackWindup, Spitter->SpitTell));
	TestEqual(TEXT("...nothing flies during the tell"), NumGlobs(World), 0);
	TickSpitterFor(Spitter, Spitter->SpitTell * 0.6f);
	TestTrue(FString::Printf(TEXT("Its sac swells (%.2f) and glows brighter (%.2f) through the tell"), Spitter->GetSacScale(), Spitter->GetSacGlow()),
		Spitter->GetTellStrength() > 0.3f && Spitter->GetSacScale() > 1.05f && Spitter->GetSacGlow() > Spitter->SacRestGlow * 1.5f);
	TestEqual(TEXT("...still nothing thrown"), NumGlobs(World), 0);
	const float SpitAt = TickSpitterUntil(Spitter, Spitter->SpitTell, [Spitter]() { return Spitter->GetLastGlobs().Num() > 0; });
	TestTrue(TEXT("At the tell's end the glob flies"), SpitAt >= 0.f && NumGlobs(World) == 1 && !Spitter->IsTelling());
	TestFalse(TEXT("...its sac emptied, its spit cooling down"), Spitter->IsSpitReady());

	// The next tell comes about a spit's interval after this one's start.
	const float NextAt = TickSpitterUntil(Spitter, 5.f, [Spitter]() { return Spitter->IsTelling(); });
	const float Interval = NextAt + Spitter->SpitTell;
	TestTrue(FString::Printf(TEXT("It spits again about every 3 s (%.2f s)"), Interval), NextAt >= 0.f && FMath::Abs(Interval - Spitter->SpitEvery) <= 0.6f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpitterArcTest, "Looter.Creatures.Spitter.Arc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpitterArcTest::RunTest(const FString& Parameters)
{
	// On a floor, at a player standing 14 m off: the lob comes down on them and leaves a puddle at their feet. At one who
	// steps well aside as it flies: it misses, lands where they were, and its splash doesn't reach them.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	SpawnFloor(World);
	ACharacter* Player = SpawnStandingPlayer(World, 1400.0, 0.0);
	ASpitterSpider* Spitter = SpawnSpitter(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("A Spitter"), Spitter))
	{
		return false;
	}
	Spitter->AlertTo(Player);
	Spitter->ReadySpit();
	const float SpitAt = TickSpitterUntil(Spitter, 2.f, [Spitter]() { return Spitter->GetLastGlobs().Num() > 0; });
	if (!TestTrue(TEXT("It spits at once, its lob's way clear over the floor"), SpitAt >= 0.f))
	{
		return false;
	}
	ASpitterGlob* Glob = Spitter->GetLastGlobs()[0];
	const FVector Feet = Player->GetActorLocation() - FVector(0.0, 0.0, 88.0);
	const float Flew = FlyGlob(Glob, 3.f);
	TestTrue(FString::Printf(TEXT("The glob comes down after its arc (%.2f s)"), Flew), Flew >= SpitterRules::ShortestFlight * 0.8f && Flew <= SpitterRules::LongestFlight + 0.1f);
	TestTrue(TEXT("...squarely on the player who stood still"), Glob->GetDirectVictim() == Player);
	const ASpitterPuddle* Puddle = Glob->GetPuddle();
	TestTrue(TEXT("...leaving a puddle at their feet"), Puddle && FVector::Dist2D(Puddle->GetActorLocation(), Feet) < 150.0
		&& FMath::Abs(Puddle->GetActorLocation().Z) < 5.0);

	// A player who steps 6 m aside as it flies.
	Spitter->ReadySpit();
	TickSpitterFor(Spitter, Spitter->SpitRecovery + 0.1f);
	const float SecondAt = TickSpitterUntil(Spitter, 3.f, [Spitter, Glob]() { return Spitter->GetLastGlobs().Num() > 0 && Spitter->GetLastGlobs()[0] != Glob; });
	if (!TestTrue(TEXT("A second spit"), SecondAt >= 0.f))
	{
		return false;
	}
	ASpitterGlob* Missed = Spitter->GetLastGlobs()[0];
	const FVector Was = Player->GetActorLocation() - FVector(0.0, 0.0, 88.0);
	Player->SetActorLocation(Player->GetActorLocation() + FVector(0.0, 600.0, 0.0));
	TestTrue(TEXT("It comes down"), FlyGlob(Missed, 3.f) >= 0.f);
	TestNull(TEXT("...missing the player who moved"), Missed->GetDirectVictim());
	TestEqual(TEXT("...its splash short of them"), Missed->GetSplashed(), 0);
	TestTrue(TEXT("...landing where they were, with its puddle"), FVector::Dist2D(Missed->GetLandedAt(), Was) < 150.0 && Missed->GetPuddle() != nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpitterPuddleTest, "Looter.Creatures.Spitter.Puddle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpitterPuddleTest::RunTest(const FString& Parameters)
{
	// A puddle on a floor burns a player standing in it every half second, nobody beside it, over it or a creature in it,
	// and dries up after its life.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	SpawnFloor(World);
	ACharacter* Player = SpawnStandingPlayer(World, 40.0, 0.0);
	FSpitterPuddleSpec Spec;
	Spec.Radius = 130.f;
	Spec.Seconds = 5.f;
	Spec.Every = 0.5f;
	ASpitterPuddle* Puddle = ASpitterPuddle::Spawn(World, FVector::ZeroVector, FVector::UpVector, Spec);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("A puddle"), Puddle))
	{
		return false;
	}
	TestTrue(TEXT("It lies in several blobs on the floor"), Puddle->NumBlobs() >= 4);
	auto TickPuddle = [Puddle](float Seconds)
	{
		for (float Time = 0.f; Time < Seconds - KINDA_SMALL_NUMBER && IsValid(Puddle); Time += SpitterStep)
		{
			Puddle->Tick(SpitterStep);
		}
	};
	TickPuddle(0.3f);
	TestTrue(TEXT("Standing in it: burned within a moment"), Puddle->Touches(*Player) && Puddle->GetBurns() == 1);
	TickPuddle(1.f);
	TestEqual(TEXT("...and every half second after"), Puddle->GetBurns(), 3);

	// Beside it, over it on a ledge: nothing.
	const int32 Before = Puddle->GetBurns();
	Player->SetActorLocation(FVector(400.0, 0.0, 89.0));
	TickPuddle(1.f);
	TestTrue(TEXT("A step out of it: no more burns"), !Puddle->Touches(*Player) && Puddle->GetBurns() == Before);
	Player->SetActorLocation(FVector(0.0, 0.0, 239.0));
	TestFalse(TEXT("On a ledge over it: not in it"), Puddle->Touches(*Player));
	// A creature walks through it unharmed.
	ACreatureBase* Spider = EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), FVector::ZeroVector);
	TickPuddle(1.f);
	TestTrue(TEXT("A spider standing in it isn't burned"), Spider && Puddle->GetBurns() == Before);

	// It dries up.
	TickPuddle(Spec.Seconds);
	TestFalse(TEXT("After its life it's gone"), IsValid(Puddle));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpitterWeakSpotTest, "Looter.Creatures.Spitter.WeakSpot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpitterWeakSpotTest::RunTest(const FString& Parameters)
{
	// Its head and its sac are critical; a crit on the sac as it tells a spit chokes it.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(1400.0, 0.0, 0.0));
	ASpitterSpider* Spitter = SpawnSpitter(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("A Spitter"), Spitter))
	{
		return false;
	}
	USkeletalMeshComponent* Body = Spitter->GetMesh();
	auto CritOn = [Spitter, Body](const TCHAR* Bone)
	{
		FHitResult Hit;
		Hit.Component = Body;
		Hit.BoneName = Bone;
		return Spitter->IsCriticalSpot(Hit);
	};
	TestTrue(TEXT("A shot on its sac is critical"), CritOn(TEXT("sac")));
	TestTrue(TEXT("...and on its head"), CritOn(TEXT("head")));
	TestFalse(TEXT("...not on its abdomen or a leg"), CritOn(TEXT("abdomen")) || CritOn(TEXT("femur_0_l")));

	// Its model: the sac's bone and hit zone (once SK_Spitter is imported).
	if (Body->GetBoneIndex(SpitterLook::SacBone) != INDEX_NONE)
	{
		const UPhysicsAsset* Zones = Body->GetPhysicsAsset();
		TestTrue(TEXT("Its model has a hit zone on its sac"), Zones && Zones->FindBodyIndex(SpitterLook::SacBone) != INDEX_NONE);
		TestTrue(TEXT("A hit at its sac is a sac hit, one at its head isn't"), Spitter->IsSacHit(Body->GetBoneLocation(SpitterLook::SacBone))
			&& !Spitter->IsSacHit(Body->GetBoneLocation(TEXT("head"))));
	}
	else
	{
		AddWarning(TEXT("SK_Spitter isn't imported (Art/Models/Creatures/Spitter.py): its sac's bone and hit zone are checked once it is."));
	}

	// A shot to its sac in its tell: nothing flies, it flinches, and its spit starts over.
	Spitter->AlertTo(Player);
	Spitter->ReadySpit();
	const float TellAt = TickSpitterUntil(Spitter, 2.f, [Spitter]() { return Spitter->IsTelling(); });
	if (!TestTrue(TEXT("It tells a spit"), TellAt >= 0.f))
	{
		return false;
	}
	TickSpitterFor(Spitter, Spitter->SpitTell * 0.4f);
	Spitter->ChokeSpit(Spitter->GetActorLocation());
	TestTrue(TEXT("Its sac burst: the spit chokes"), Spitter->WasSpitChoked() && !Spitter->IsTelling());
	TickSpitterFor(Spitter, Spitter->SpitTell);
	TestEqual(TEXT("...nothing flies"), NumGlobs(World), 0);
	TestTrue(TEXT("...and its spit starts over from a full interval"), Spitter->GetSpitReadyIn() > Spitter->GetSpitInterval() - Spitter->SpitTell - 0.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpitterBestiaryTest, "Looter.Creatures.Spitter.Bestiary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpitterBestiaryTest::RunTest(const FString& Parameters)
{
	// Its page (Tools/Unreal/create_bestiary_pages.py Spitter): a creature's, about the Spitter (not the brown spider), its
	// numbers read from it, its model on the stand.
	const UBestiaryEntry* Page = LoadObject<UBestiaryEntry>(nullptr, TEXT("/Game/Data/Bestiary/DA_Bestiary_Spitter.DA_Bestiary_Spitter"));
	if (!TestNotNull(TEXT("DA_Bestiary_Spitter (run create_bestiary_pages.py Spitter after importing SK_Spitter)"), Page))
	{
		return false;
	}
	TestTrue(TEXT("Its actor class is the Spitter"), Page->ActorClass.LoadSynchronous() == ASpitterSpider::StaticClass());
	TestTrue(TEXT("...a page of its own, not the brown spider's"), Page->Describes(ASpitterSpider::StaticClass()) && !Page->Describes(ASpiderCreature::StaticClass()));
	TestEqual(TEXT("A creature"), Page->Category, EBestiaryCategory::Creature);
	TestFalse(TEXT("Written up"), Page->DisplayName.IsEmpty() || Page->Description.IsEmpty() || Page->Notes.Num() < 3);
	TestNotNull(TEXT("Its model on the stand"), Page->LoadPreviewMesh());
	const ASpitterSpider* Defaults = GetDefault<ASpitterSpider>();
	const FBestiaryStats Stats = Page->ReadStats();
	TestEqual(TEXT("Its experience, as a kill pays it: a spider's"), static_cast<int64>(Stats.XPReward),
		UPlayerProgressionSubsystem::GetLevelRules().KillXP(GetDefault<ASpiderCreature>()->XPReward, Defaults->Level, Defaults->Level));
	TestEqual(TEXT("Its attack"), Stats.AttackDamage, Defaults->AttackDamage);
	return true;
}

#endif

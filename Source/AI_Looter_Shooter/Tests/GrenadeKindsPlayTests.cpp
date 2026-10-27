#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/GraveSaltCinderBed.h"
#include "Combat/GraveSaltGrenade.h"
#include "Combat/GraveSaltRules.h"
#include "Combat/GraveSaltTuning.h"
#include "Combat/GrenadeKinds.h"
#include "Combat/GrenadeKindsTuning.h"
#include "Combat/HealthComponent.h"
#include "Combat/MovementSlowComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/SpiderCreature.h"
#include "Loot/GrenadePickup.h"
#include "Loot/LootDropComponent.h"
#include "Player/PlayerThrowComponent.h"
#include "Player/PlayerThrowRules.h"
#include "Player/PlayerThrowSave.h"
#include "Tests/LocomotionTestWorld.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Misc/ScopeExit.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// The grenade kinds in a test level (their rules alone: GrenadeKindsTests.cpp): the pitch jar sticking and tarring, the
// scatter jar's brood popping round it, the cinder jar's bed burning what stands in it (a boss no more than its cap),
// and a found tin's kind carried, kept and saved.

namespace
{
	constexpr float Frame = 1.f / 60.f;

	/** A creature on the ground at Feet, started as play starts it, dropping nothing (Looter.Grenade.Burst's way). */
	ASpiderCreature* SpawnSpider(UWorld* World, const FVector& Feet, ECreatureRank Rank = ECreatureRank::Basic)
	{
		ASpiderCreature* Spider = World->SpawnActor<ASpiderCreature>(Feet + FVector(0.0, 0.0, 300.0), FRotator::ZeroRotator);
		if (!Spider)
		{
			return nullptr;
		}
		Spider->StartingRank = Rank;
		if (ULootDropComponent* Loot = Spider->FindComponentByClass<ULootDropComponent>())
		{
			Loot->bDropOnDeath = false;
		}
		Spider->DispatchBeginPlay();
		const float HalfHeight = Spider->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Spider->SetActorLocation(Feet + FVector(0.0, 0.0, HalfHeight + 1.0), false, nullptr, ETeleportType::TeleportPhysics);
		UCharacterMovementComponent* Movement = Spider->GetCharacterMovement();
		Movement->SetUpdatedComponent(Spider->GetCapsuleComponent());
		Movement->SetMovementMode(MOVE_Walking);
		Movement->Activate(true);
		return Spider;
	}

	/** A jar of Kind thrown in a test level (no thrower) at LevelScale, stepped by the test: the level never ticks. */
	AGraveSaltGrenade* ThrowKind(UWorld* World, EGrenadeKind Kind, const FVector& From, const FVector& Velocity, float LevelScale)
	{
		AGraveSaltGrenade* Grenade = World->SpawnActor<AGraveSaltGrenade>(From, Velocity.Rotation());
		if (Grenade)
		{
			Grenade->SetKind(Kind);
			Grenade->Launch(Velocity, nullptr, LevelScale);
		}
		return Grenade;
	}

	void StepUntilBurst(AGraveSaltGrenade* Grenade, float Seconds)
	{
		for (float Time = 0.f; Time < Seconds && IsValid(Grenade) && !Grenade->HasBurst(); Time += Frame)
		{
			Grenade->Step(Frame);
		}
	}

	template <typename TActor>
	TArray<TActor*> AllOf(UWorld* World)
	{
		TArray<TActor*> Found;
		for (TActorIterator<TActor> It(World); It; ++It)
		{
			if (IsValid(*It) && !It->IsActorBeingDestroyed())
			{
				Found.Add(*It);
			}
		}
		return Found;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeKindsPitchTest, "Looter.Grenade.Kinds.Pitch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeKindsPitchTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(4000.0, 4000.0, 100.0));
	const FGrenadeKindTuning Kinds;

	// Thrown level over open ground: it sticks where it first lands and lies there (no bounce, no roll) till its fuse.
	{
		AGraveSaltGrenade* Jar = ThrowKind(World, EGrenadeKind::Pitch, FVector(-1500.0, -1500.0, 140.0), FThrowRules::LaunchVelocity(FVector::ForwardVector, FVector::ZeroVector), 0.1f);
		if (!TestNotNull(TEXT("A pitch jar"), Jar))
		{
			return false;
		}
		TestEqual(TEXT("...of its kind"), Jar->GetKind(), EGrenadeKind::Pitch);
		for (int32 Step = 0; Step < 70 && !Jar->IsStuck(); ++Step)
		{
			Jar->Step(Frame);
		}
		TestTrue(TEXT("It stuck to the ground"), Jar->IsStuck());
		TestEqual(TEXT("...with no bounce"), Jar->GetBounces(), 0);
		TestNull(TEXT("...holding nothing that moves"), Jar->GetStuckTo());
		const FVector Where = Jar->GetActorLocation();
		for (int32 Step = 0; Step < 10; ++Step)
		{
			Jar->Step(Frame);
		}
		TestTrue(TEXT("...and lies where it stuck"), Jar->GetActorLocation().Equals(Where, 0.1));
		TestTrue(FString::Printf(TEXT("...on the ground (%.1f cm up)"), Where.Z), Where.Z >= 0.0 && Where.Z < 20.0);
		StepUntilBurst(Jar, FGraveSaltRules::FuseSeconds);
		TestTrue(TEXT("It bursts at its fuse"), Jar->HasBurst());
	}

	// Thrown at a body with health: it holds on to it, and bursts within the stuck fuse, not the whole one.
	{
		AStaticMeshActor* Body = LocomotionTestWorld::SpawnBlock(World, FVector(-800.0, 1500.0, 60.0), FVector(80.0, 80.0, 120.0));
		UHealthComponent* Health = Body ? NewObject<UHealthComponent>(Body, TEXT("Health")) : nullptr;
		if (TestNotNull(TEXT("A body with health"), Health))
		{
			Health->bShowDamageNumbers = false;
			Body->AddInstanceComponent(Health);
			Health->RegisterComponent();
			AGraveSaltGrenade* Jar = ThrowKind(World, EGrenadeKind::Pitch, FVector(-1200.0, 1500.0, 70.0), FVector(1400.0, 0.0, 150.0), 0.1f);
			for (int32 Step = 0; Jar && Step < 60 && !Jar->IsStuck(); ++Step)
			{
				Jar->Step(Frame);
			}
			TestTrue(TEXT("It stuck to the body"), Jar && Jar->IsStuck() && Jar->GetStuckTo() == Body);
			TestTrue(FString::Printf(TEXT("...its fuse cut to the stuck fuse (%.2f s left)"), Jar ? Jar->GetFuseLeft() : 0.f),
				Jar && Jar->GetFuseLeft() <= Kinds.PitchStuckFuse + 0.01f);
			StepUntilBurst(Jar, Kinds.PitchStuckFuse + 0.1f);
			TestTrue(TEXT("...and burst on it"), Jar && Jar->HasBurst());
		}
	}

	// Its burst tars what it doesn't kill: a spider next to it walks slowly a while.
	{
		ASpiderCreature* Spider = SpawnSpider(World, FVector(1200.0, -1200.0, 0.0));
		if (TestNotNull(TEXT("A spider"), Spider))
		{
			AGraveSaltGrenade* Jar = ThrowKind(World, EGrenadeKind::Pitch, Spider->GetActorLocation() + FVector(-120.0, 0.0, -30.0), FVector::ZeroVector, 0.02f);
			if (TestNotNull(TEXT("A pitch jar by it"), Jar))
			{
				Jar->Detonate();
				const UHealthComponent* SpiderHealth = Spider->FindComponentByClass<UHealthComponent>();
				const UMovementSlowComponent* Slow = Spider->FindComponentByClass<UMovementSlowComponent>();
				if (SpiderHealth && !SpiderHealth->IsDead())
				{
					TestTrue(TEXT("The spider it caught is tarred: slowed"), Slow && Slow->IsSlowed());
					TestTrue(FString::Printf(TEXT("...to the tar's share (%.2f)"), Slow ? Slow->GetMultiplier() : 1.f), Slow && FMath::IsNearlyEqual(Slow->GetMultiplier(), Kinds.PitchSlow, 0.01f));
				}
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeKindsScatterTest, "Looter.Grenade.Kinds.Scatter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeKindsScatterTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(4000.0, 4000.0, 100.0));
	const FGrenadeKindTuning Kinds;

	AGraveSaltGrenade* Jar = ThrowKind(World, EGrenadeKind::Scatter, FVector(0.0, 0.0, FGraveSaltRules::CollisionRadius + 1.0), FVector::ZeroVector, 0.1f);
	if (!TestNotNull(TEXT("A scatter jar"), Jar))
	{
		return false;
	}
	Jar->Detonate();
	TArray<AGraveSaltGrenade*> Brood;
	for (AGraveSaltGrenade* Each : AllOf<AGraveSaltGrenade>(World))
	{
		if (Each->IsBrood())
		{
			Brood.Add(Each);
		}
	}
	TestEqual(TEXT("Its brood flung out"), Brood.Num(), Kinds.ScatterCount);
	bool bAllFlung = true;
	for (const AGraveSaltGrenade* Each : Brood)
	{
		bAllFlung &= Each->GetKind() == EGrenadeKind::Scatter && Each->GetVelocity().Z > 0.0 && !Each->HasBurst();
		bAllFlung &= Each->GetFuseLeft() <= Kinds.BroodFuseMax + 0.01f && Each->GetFuseLeft() >= Kinds.BroodFuseMin - 0.01f;
	}
	TestTrue(TEXT("...each up and out, on its own short fuse"), bAllFlung);
	for (AGraveSaltGrenade* Each : Brood)
	{
		StepUntilBurst(Each, 2.f);
	}
	int32 Burst = 0;
	float Spread = 0.f;
	for (const AGraveSaltGrenade* Each : Brood)
	{
		Burst += Each->HasBurst() ? 1 : 0;
		Spread = FMath::Max(Spread, static_cast<float>(FVector::Dist2D(Each->GetActorLocation(), FVector::ZeroVector)));
	}
	TestEqual(TEXT("Every small jar popped"), Burst, Brood.Num());
	TestTrue(FString::Printf(TEXT("...spread round the burst (%.0f cm out at most)"), Spread), Spread > 150.f);
	int32 Small = 0;
	for (AGraveSaltGrenade* Each : AllOf<AGraveSaltGrenade>(World))
	{
		Small += Each->IsBrood() ? 1 : 0;
	}
	TestEqual(TEXT("...and none of them flung more"), Small, Brood.Num());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeKindsCinderTest, "Looter.Grenade.Kinds.Cinder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeKindsCinderTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	const FGraveSaltTuning Base;
	const FGrenadeKindTuning Kinds;

	// Burst in the air over the floor: the coals fall to the ground under it.
	AGraveSaltGrenade* Jar = ThrowKind(World, EGrenadeKind::Cinder, FVector(0.0, 0.0, 150.0), FVector::ZeroVector, 0.1f);
	if (!TestNotNull(TEXT("A cinder jar"), Jar))
	{
		return false;
	}
	Jar->Detonate();
	TArray<AGraveSaltCinderBed*> Beds = AllOf<AGraveSaltCinderBed>(World);
	if (!TestEqual(TEXT("Its bed left burning"), Beds.Num(), 1))
	{
		return false;
	}
	AGraveSaltCinderBed* Bed = Beds[0];
	TestTrue(FString::Printf(TEXT("...on the ground under it (%.1f)"), Bed->GetActorLocation().Z), FMath::Abs(Bed->GetActorLocation().Z) < 2.0);
	TestTrue(TEXT("...facing up"), Bed->GetUp().Z > 0.99);
	TestEqual(TEXT("...its reach and life the tuning's"), Bed->GetRadius(), Kinds.BedRadius);

	// It burns what stands in it, not what stands off it. A test level has no game mode, so a pawn ignores
	// ApplyPointDamage (APawn::ShouldTakeDamage): the spiders show who it catches, a post with health (not a pawn: it
	// takes the damage) shows the burn landing. Actors' own events need letting through before play.
	ASpiderCreature* In = SpawnSpider(World, FVector(Kinds.BedRadius * 0.5f, 0.0, 0.0));
	ASpiderCreature* Out = SpawnSpider(World, FVector(Kinds.BedRadius + 400.f, 0.0, 0.0));
	AStaticMeshActor* Post = LocomotionTestWorld::SpawnBlock(World, FVector(-Kinds.BedRadius * 0.5f, 0.0, 60.0), FVector(60.0, 60.0, 120.0));
	UHealthComponent* PostHealth = Post ? NewObject<UHealthComponent>(Post, TEXT("Health")) : nullptr;
	if (PostHealth)
	{
		// A body the burn's search finds (it looks for pawns and dynamic things, never the world's static shapes).
		Post->GetStaticMeshComponent()->SetCollisionObjectType(ECC_WorldDynamic);
		PostHealth->bShowDamageNumbers = false;
		Post->AddInstanceComponent(PostHealth);
		PostHealth->RegisterComponent();
		// Its health's BeginPlay fills it and binds it to the post's damage (the test level never begins play).
		Post->DispatchBeginPlay();
		PostHealth->SetMaxHealth(200.f);
		PostHealth->ResetHealth();
	}
	if (TestNotNull(TEXT("Spiders"), In) && TestNotNull(TEXT("..."), Out) && TestNotNull(TEXT("A post with health"), PostHealth))
	{
		const float PostBefore = PostHealth->GetHealth();
		int32 Caught = 0;
		{
			FEditorScriptExecutionGuard RunActorEvents;
			Caught = Bed->Burn(Kinds.BedTick);
		}
		TestEqual(TEXT("A burn catches the two in it (the spider and the post), never the spider off it"), Caught, 2);
		TestTrue(FString::Printf(TEXT("...and hurts them (the post %.1f to %.1f)"), PostBefore, PostHealth->GetHealth()), PostHealth->GetHealth() < PostBefore);
	}

	// A boss in it: never more of its bar from the whole bed than half the jar's cap.
	ASpiderCreature* Boss = SpawnSpider(World, FVector(0.0, Kinds.BedRadius * 0.4f, 0.0), ECreatureRank::Boss);
	if (TestNotNull(TEXT("A boss"), Boss))
	{
		const UHealthComponent* BossHealth = Boss->FindComponentByClass<UHealthComponent>();
		for (int32 Burn = 0; Burn < 200; ++Burn)
		{
			Bed->Burn(Kinds.BedTick);
		}
		const float Cap = BossHealth ? BossHealth->GetMaxHealth() * GrenadeKinds::BedBossCap(Base) : 0.f;
		TestTrue(FString::Printf(TEXT("The boss took at most its cap (%.0f of %.0f)"), Bed->GetBossDealt(Boss), Cap), Bed->GetBossDealt(Boss) <= Cap + 0.01f);
	}

	// It burns out at its end and goes.
	for (float Time = 0.f; Time < Kinds.BedSeconds + 0.2f && IsValid(Bed) && !Bed->IsActorBeingDestroyed(); Time += Frame)
	{
		Bed->Step(Frame);
	}
	TestTrue(TEXT("Burnt out and gone after its seconds"), !IsValid(Bed) || Bed->IsActorBeingDestroyed());

	// High over a drop with nothing under it for metres: no bed.
	AGraveSaltGrenade* Lost = ThrowKind(World, EGrenadeKind::Cinder, FVector(0.0, 0.0, 2000.0), FVector::ZeroVector, 0.1f);
	if (TestNotNull(TEXT("A cinder jar high up"), Lost))
	{
		Lost->Detonate();
		TestEqual(TEXT("Its coals lost on the way down: no bed"), AllOf<AGraveSaltCinderBed>(World).Num(), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeKindsFoundTest, "Looter.Grenade.Kinds.Found",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeKindsFoundTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();

	// No player in the level is level 1: a dropped tin is always plain salt.
	for (int32 Index = 0; Index < 20; ++Index)
	{
		AGrenadePickup* Tin = AGrenadePickup::SpawnGrenades(World, 1, FVector(Index * 100.0, 0.0, 50.0));
		TestTrue(TEXT("Level 1's drops: salt"), Tin && Tin->GetKind() == EGrenadeKind::Salt);
	}
	AGrenadePickup* Cinder = AGrenadePickup::SpawnKind(World, 1, FVector(0.0, 500.0, 50.0), EGrenadeKind::Cinder);
	TestTrue(TEXT("A tin of a given kind"), Cinder && Cinder->GetKind() == EGrenadeKind::Cinder && Cinder->GetAmount() == 1);

	// A holder as the game has one (health and the throw).
	APawn* Holder = World->SpawnActor<APawn>();
	if (!TestNotNull(TEXT("A holder"), Holder))
	{
		return false;
	}
	UPlayerThrowComponent* Throw = NewObject<UPlayerThrowComponent>(Holder, TEXT("Throw"));
	Holder->AddInstanceComponent(Throw);
	Throw->RegisterComponent();
	int32 Swaps = 0;
	EGrenadeKind Told = EGrenadeKind::Salt;
	const FDelegateHandle Listening = Throw->OnGrenadeKindChanged.AddLambda([&Swaps, &Told](EGrenadeKind Kind, EGrenadeKind) { ++Swaps; Told = Kind; });
	ON_SCOPE_EXIT
	{
		Throw->OnGrenadeKindChanged.Remove(Listening);
	};

	TestEqual(TEXT("Salt to start"), Throw->GetKind(), EGrenadeKind::Salt);
	Throw->AddGrenades(1, EGrenadeChange::Given);
	TestEqual(TEXT("Salt given: still salt"), Throw->GetKind(), EGrenadeKind::Salt);
	TestEqual(TEXT("The cinder tin taken: one added"), Cinder ? Cinder->CollectInto(*Throw) : -1, 1);
	TestTrue(TEXT("...carrying cinder jars now, two of them"), Throw->GetKind() == EGrenadeKind::Cinder && Throw->GetGrenades() == 2);
	TestTrue(TEXT("...and told so"), Swaps == 1 && Told == EGrenadeKind::Cinder);
	TestTrue(TEXT("...the tin spent"), !IsValid(Cinder) || Cinder->IsActorBeingDestroyed());

	// Salt from anywhere fills the cinder jars; another kind swaps them, even at the most carried.
	Throw->AddGrenades(1, EGrenadeChange::Given);
	TestTrue(TEXT("The vendor's salt: a third cinder jar"), Throw->GetKind() == EGrenadeKind::Cinder && Throw->GetGrenades() == 3);
	AGrenadePickup* Pitch = AGrenadePickup::SpawnKind(World, 1, FVector(0.0, 800.0, 50.0), EGrenadeKind::Pitch);
	TestEqual(TEXT("Full: a pitch tin taken adds none"), Pitch ? Pitch->CollectInto(*Throw) : -1, 0);
	TestTrue(TEXT("...but the jars are pitch now, still three"), Throw->GetKind() == EGrenadeKind::Pitch && Throw->GetGrenades() == 3 && Swaps == 2);

	// The session keeps the kind with the count.
	FThrowablesSave Save;
	Throw->SaveThrowables(Save);
	TestEqual(TEXT("Saved: pitch"), GrenadeKinds::FromSaved(Save.Kind), EGrenadeKind::Pitch);
	FThrowablesSave Old;
	Old.Grenades = 1;
	Old.bUnlocked = true;
	Throw->RestoreThrowables(Old);
	TestTrue(TEXT("An old save: one salt jar"), Throw->GetKind() == EGrenadeKind::Salt && Throw->GetGrenades() == 1);
	Throw->RestoreThrowables(Save);
	TestTrue(TEXT("Back to three pitch jars"), Throw->GetKind() == EGrenadeKind::Pitch && Throw->GetGrenades() == 3);
	TestEqual(TEXT("A restore tells of no swap"), Swaps, 2);
	return true;
}

#endif

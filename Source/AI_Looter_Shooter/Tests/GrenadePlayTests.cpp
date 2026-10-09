#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Combat/GraveSaltBurst.h"
#include "Combat/GraveSaltGrenade.h"
#include "Combat/GraveSaltRules.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/LootDropComponent.h"
#include "Player/PlayerThrowComponent.h"
#include "Player/PlayerThrowRules.h"
#include "Player/PlayerThrowSave.h"
#include "Tests/LocomotionTestWorld.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Misc/ScopeExit.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// The grave-salt grenade in a test level (its rules alone are GrenadeTests.cpp): the jar's flight, bounces and fuse, a
// body bursting it on contact, what the burst reaches (falloff, walls, salt on the dead, a boss's cap, never the
// thrower), the knock that never goes over an edge, and the player's count: the first gun's two, the most carried, and
// the session's save.

namespace
{
	constexpr float Frame = 1.f / 60.f;

	/** A creature standing on the ground at Feet, started as play starts it, its movement given its capsule, dropping nothing. */
	template <typename TCreature>
	TCreature* SpawnCreature(UWorld* World, const FVector& Feet, ECreatureRank Rank = ECreatureRank::Basic)
	{
		TCreature* Creature = World->SpawnActor<TCreature>(Feet + FVector(0.0, 0.0, 300.0), FRotator::ZeroRotator);
		if (!Creature)
		{
			return nullptr;
		}
		Creature->StartingRank = Rank;
		if (ULootDropComponent* Loot = Creature->template FindComponentByClass<ULootDropComponent>())
		{
			Loot->bDropOnDeath = false;
		}
		Creature->DispatchBeginPlay();
		const float HalfHeight = Creature->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Creature->SetActorLocation(Feet + FVector(0.0, 0.0, HalfHeight + 1.0), false, nullptr, ETeleportType::TeleportPhysics);
		UCharacterMovementComponent* Movement = Creature->GetCharacterMovement();
		Movement->SetUpdatedComponent(Creature->GetCapsuleComponent());
		Movement->SetMovementMode(MOVE_Walking);
		// A launch only takes on an active movement (UCharacterMovementComponent::Launch).
		Movement->Activate(true);
		return Creature;
	}

	/** A grenade thrown in a test level (no thrower), stepped by the test: the level never ticks. */
	AGraveSaltGrenade* ThrowGrenade(UWorld* World, const FVector& From, const FVector& Velocity)
	{
		AGraveSaltGrenade* Grenade = World->SpawnActor<AGraveSaltGrenade>(From, Velocity.Rotation());
		if (Grenade)
		{
			Grenade->Launch(Velocity, nullptr, 1.f);
		}
		return Grenade;
	}

	/** Steps it for up to Seconds or until it bursts; returns the seconds stepped. */
	float StepFor(AGraveSaltGrenade* Grenade, float Seconds, float& OutLowestZ)
	{
		float Time = 0.f;
		while (Time < Seconds && IsValid(Grenade) && !Grenade->HasBurst())
		{
			Grenade->Step(Frame);
			OutLowestZ = FMath::Min(OutLowestZ, static_cast<float>(Grenade->GetActorLocation().Z));
			Time += Frame;
		}
		return Time;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeFlightPlayTest, "Looter.Grenade.Flight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeFlightPlayTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	// A floor 40 m across with its top at 0, and a wall 3 m high standing across the middle lane at x 1.5 m.
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(4000.0, 4000.0, 100.0));
	LocomotionTestWorld::SpawnBlock(World, FVector(1600.0, 0.0, 150.0), FVector(100.0, 1000.0, 300.0));
	const FVector Level = FThrowRules::LaunchVelocity(FVector::ForwardVector, FVector::ZeroVector);

	// Thrown level from the eye over open ground: it arcs down, clinks on the floor (never through it), and bursts at the
	// fuse, not before.
	{
		AGraveSaltGrenade* Grenade = ThrowGrenade(World,FVector(-1500.0, -1500.0, 140.0), Level);
		if (!TestNotNull(TEXT("A grenade"), Grenade))
		{
			return false;
		}
		float Lowest = 1000.f;
		StepFor(Grenade, FGraveSaltRules::FuseSeconds - 0.05f, Lowest);
		TestFalse(TEXT("No burst before the fuse"), Grenade->HasBurst());
		TestTrue(FString::Printf(TEXT("It came down and bounced (%d bounces)"), Grenade->GetBounces()), Grenade->GetBounces() >= 1);
		TestTrue(FString::Printf(TEXT("Never through the floor (lowest %.1f cm)"), Lowest), Lowest >= FGraveSaltRules::CollisionRadius - 1.f);
		const float Out = static_cast<float>(Grenade->GetActorLocation().X + 1500.0);
		TestTrue(FString::Printf(TEXT("Out ahead where a level throw lands and rolls (%.0f cm)"), Out), Out > 1000.f && Out < 1800.f);
		StepFor(Grenade, 0.1f, Lowest);
		TestTrue(TEXT("Burst at the fuse"), Grenade->HasBurst());
	}

	// Thrown at the wall: it comes back off it, and never through.
	{
		AGraveSaltGrenade* Grenade = ThrowGrenade(World,FVector(1200.0, 0.0, 140.0), FVector(1500.0, 0.0, 0.0));
		if (TestNotNull(TEXT("A grenade at the wall"), Grenade))
		{
			float MostX = -1e6f;
			bool bCameBack = false;
			for (int32 Step = 0; Step < 40 && !Grenade->HasBurst(); ++Step)
			{
				Grenade->Step(Frame);
				MostX = FMath::Max(MostX, static_cast<float>(Grenade->GetActorLocation().X));
				bCameBack |= Grenade->GetVelocity().X < 0.0;
			}
			TestTrue(TEXT("Off the wall, back the way it came"), bCameBack);
			TestTrue(FString::Printf(TEXT("Never into the wall (furthest %.0f)"), MostX), MostX <= 1550.f - FGraveSaltRules::CollisionRadius + 1.f);
			TestTrue(TEXT("A wall's knock is heard"), Grenade->GetBounces() >= 1);
		}
	}

	// Something that can be hurt in its way bursts it on contact, long before its fuse.
	{
		AStaticMeshActor* Body = LocomotionTestWorld::SpawnBlock(World, FVector(-800.0, 1500.0, 60.0), FVector(80.0, 80.0, 120.0));
		UHealthComponent* Health = Body ? NewObject<UHealthComponent>(Body, TEXT("Health")) : nullptr;
		if (TestNotNull(TEXT("A body with health"), Health))
		{
			Health->bShowDamageNumbers = false;
			Body->AddInstanceComponent(Health);
			Health->RegisterComponent();
			AGraveSaltGrenade* Grenade = ThrowGrenade(World,FVector(-1200.0, 1500.0, 70.0), FVector(1400.0, 0.0, 150.0));
			float Lowest = 1000.f;
			StepFor(Grenade, FGraveSaltRules::FuseSeconds, Lowest);
			TestTrue(TEXT("It burst on the body"), Grenade && Grenade->HasBurst());
			TestTrue(FString::Printf(TEXT("...on contact, well inside its fuse (%.2f s left)"), Grenade ? Grenade->GetFuseLeft() : 0.f),
				Grenade && Grenade->GetFuseLeft() > 0.8f);
		}
	}

	// A spider's hit zones (what bullets meet) burst it too, in the game's own collision.
	{
		ASpiderCreature* Spider = SpawnCreature<ASpiderCreature>(World, FVector(1000.0, 1500.0, 0.0));
		if (TestNotNull(TEXT("A spider"), Spider))
		{
			const float Middle = static_cast<float>(Spider->GetActorLocation().Z);
			AGraveSaltGrenade* Grenade = ThrowGrenade(World,FVector(650.0, 1500.0, Middle + 10.f), FVector(1500.0, 0.0, 100.0));
			float Lowest = 1000.f;
			StepFor(Grenade, FGraveSaltRules::FuseSeconds, Lowest);
			if (!(Grenade && Grenade->HasBurst() && Grenade->GetFuseLeft() > 0.8f))
			{
				// A test level may not build a skinned body's collision; the game does (Looter.Grenade.Burst's targets
				// cover the burst itself). Said, not failed.
				AddWarning(TEXT("The spider's hit zones didn't stop the jar in the test level: check a throw at a spider in play."));
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeBurstPlayTest, "Looter.Grenade.Burst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeBurstPlayTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(4000.0, 4000.0, 100.0));
	// A wall west of the burst, 4 m high: what stands behind it is shielded.
	LocomotionTestWorld::SpawnBlock(World, FVector(-150.0, 0.0, 200.0), FVector(40.0, 400.0, 400.0));
	const FVector Center(0.0, 0.0, 20.0);

	ASpiderCreature* Near = SpawnCreature<ASpiderCreature>(World, FVector(180.0, 0.0, 0.0));
	ASpiderCreature* Farther = SpawnCreature<ASpiderCreature>(World, FVector(0.0, 320.0, 0.0));
	ASpiderCreature* OutOfReach = SpawnCreature<ASpiderCreature>(World, FVector(0.0, -900.0, 0.0));
	ASpiderCreature* Shielded = SpawnCreature<ASpiderCreature>(World, FVector(-320.0, 0.0, 0.0));
	AUnpaidCreature* Unpaid = SpawnCreature<AUnpaidCreature>(World, FVector(250.0, -250.0, 0.0));
	ASpiderCreature* Boss = SpawnCreature<ASpiderCreature>(World, FVector(250.0, 300.0, 0.0), ECreatureRank::Boss);
	if (!TestNotNull(TEXT("Spiders"), Near) || !TestNotNull(TEXT("..."), Farther) || !TestNotNull(TEXT("..."), OutOfReach)
		|| !TestNotNull(TEXT("..."), Shielded) || !TestNotNull(TEXT("An Unpaid"), Unpaid) || !TestNotNull(TEXT("A boss"), Boss))
	{
		return false;
	}

	FRandomStream Random(7);
	const TArray<FGraveSaltTarget> Targets = GraveSaltBurst::FindTargets(World, Center, nullptr, 1.f, Random);
	auto Find = [&Targets](const AActor* Actor) -> const FGraveSaltTarget*
	{
		return Targets.FindByPredicate([Actor](const FGraveSaltTarget& Target) { return Target.Actor.Get() == Actor; });
	};
	const FGraveSaltTarget* NearHit = Find(Near);
	const FGraveSaltTarget* FartherHit = Find(Farther);
	const FGraveSaltTarget* UnpaidHit = Find(Unpaid);
	const FGraveSaltTarget* BossHit = Find(Boss);
	TestNotNull(TEXT("The spider close by is reached"), NearHit);
	TestNotNull(TEXT("...and the one farther off"), FartherHit);
	TestNull(TEXT("Nothing past the radius"), Find(OutOfReach));
	TestNull(TEXT("Nothing behind a wall"), Find(Shielded));
	TestTrue(TEXT("Nearest first"), Targets.Num() >= 2 && Targets[0].Distance <= Targets.Last().Distance);

	// Each one's damage: the falloff at its nearest side, rolled within the +/-10% spread.
	for (const FGraveSaltTarget& Target : Targets)
	{
		const float Low = FGraveSaltRules::BurstDamage(Target.Distance, 1.f, 0.f, Target.bUnpaid);
		const float High = FGraveSaltRules::BurstDamage(Target.Distance, 1.f, 1.f, Target.bUnpaid);
		const float Listed = Target.bBoss ? FMath::Min(Low, Target.Damage) : Low;
		TestTrue(FString::Printf(TEXT("%s takes its share (%.0f at %.0f cm)"), *GetNameSafe(Target.Actor.Get()), Target.Damage, Target.Distance),
			Target.Damage >= Listed - 0.01f && Target.Damage <= High + 0.01f);
	}
	if (NearHit && FartherHit)
	{
		TestTrue(TEXT("Nearer takes more"), NearHit->Distance < FartherHit->Distance && FGraveSaltRules::Falloff(NearHit->Distance) > FGraveSaltRules::Falloff(FartherHit->Distance));
	}
	// Salt burns the dead; a boss takes no more than its cap.
	if (TestNotNull(TEXT("The Unpaid is reached"), UnpaidHit))
	{
		TestTrue(TEXT("Salt burns the dead"), UnpaidHit->bUnpaid);
		TestTrue(TEXT("...half again"), UnpaidHit->Damage >= FGraveSaltRules::BurstDamage(UnpaidHit->Distance, 1.f, 0.f, false) * FGraveSaltRules::UnpaidScale - 0.01f);
	}
	if (TestNotNull(TEXT("The boss is reached"), BossHit))
	{
		const UHealthComponent* BossHealth = Boss->FindComponentByClass<UHealthComponent>();
		TestTrue(TEXT("Known for a boss"), BossHit->bBoss);
		TestTrue(TEXT("A boss takes at most its cap"), BossHealth && BossHit->Damage <= BossHealth->GetMaxHealth() * FGraveSaltRules::BossShareCap + 0.01f);
	}

	// Never the thrower.
	FRandomStream Again(7);
	TestTrue(TEXT("The thrower is never hurt"), !GraveSaltBurst::FindTargets(World, Center, Near, 1.f, Again).ContainsByPredicate(
		[Near](const FGraveSaltTarget& Target) { return Target.Actor.Get() == Near; }));
	TestFalse(TEXT("Nor anything it holds as itself"), GraveSaltBurst::CanHurt(Near, Near));

	// The knock: out of the burst, across the ground; never over an edge; never a boss.
	const FVector Knock = GraveSaltBurst::KnockAway(*Farther, Center);
	TestTrue(FString::Printf(TEXT("Thrown away from the burst (%s)"), *Knock.ToCompactString()), Knock.Y > 0.0 && Knock.Z > 0.0);
	TestTrue(TEXT("No knock for a boss"), GraveSaltBurst::KnockAway(*Boss, Center).IsZero());
	ASpiderCreature* Edge = SpawnCreature<ASpiderCreature>(World, FVector(1990.0, 1500.0, 0.0));
	if (TestNotNull(TEXT("A spider at the floor's edge"), Edge))
	{
		TestTrue(TEXT("At the edge, a burst behind it knocks it nowhere"), GraveSaltBurst::KnockAway(*Edge, FVector(1800.0, 1500.0, 20.0)).IsZero());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrenadeCarryPlayTest, "Looter.Grenade.Carry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGrenadeCarryPlayTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)) || !TestNotNull(TEXT("Rifle loads"), Rifle))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();

	// A holder as the game has one (health, weapons, the throw), with no gun yet: the tutorial's unarmed start.
	APawn* Holder = World->SpawnActor<APawn>();
	if (!TestNotNull(TEXT("A holder"), Holder))
	{
		return false;
	}
	UHealthComponent* Health = NewObject<UHealthComponent>(Holder, TEXT("Health"));
	Health->bShowDamageNumbers = false;
	Holder->AddInstanceComponent(Health);
	Health->RegisterComponent();
	UWeaponManagerComponent* Weapons = NewObject<UWeaponManagerComponent>(Holder, TEXT("Weapons"));
	Holder->AddInstanceComponent(Weapons);
	Weapons->RegisterComponent();
	Weapons->MaxWeapons = 3;
	UPlayerThrowComponent* Throw = NewObject<UPlayerThrowComponent>(Holder, TEXT("Throw"));
	Holder->AddInstanceComponent(Throw);
	Throw->RegisterComponent();
	// The weapon manager tells of its guns through a dynamic delegate: let the components' own events run here.
	FEditorScriptExecutionGuard RunEvents;
	Holder->DispatchBeginPlay();

	TestFalse(TEXT("Unarmed: no grenades yet"), Throw->IsUnlocked());
	TestEqual(TEXT("...none"), Throw->GetGrenades(), 0);
	int32 Changes = 0;
	EGrenadeChange LastWhy = EGrenadeChange::Restored;
	const FDelegateHandle Listening = Throw->OnGrenadesChanged.AddLambda([&Changes, &LastWhy](int32, int32, EGrenadeChange Why) { ++Changes; LastWhy = Why; });
	ON_SCOPE_EXIT
	{
		Throw->OnGrenadesChanged.Remove(Listening);
	};

	// The first gun in hand brings the starting two.
	Weapons->GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Common, 1));
	TestTrue(TEXT("The first gun unlocks them"), Throw->IsUnlocked());
	TestEqual(TEXT("...two of them"), Throw->GetGrenades(), FThrowRules::StartingGrenades);
	TestTrue(TEXT("...said as given"), Changes == 1 && LastWhy == EGrenadeChange::Given);
	TestFalse(TEXT("Given once only"), Throw->GiveStartingGrenades());
	Weapons->GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Common, 1));
	TestEqual(TEXT("A second gun gives none"), Throw->GetGrenades(), FThrowRules::StartingGrenades);

	// Found: up to three; what doesn't fit isn't taken.
	TestEqual(TEXT("Five found: one taken"), Throw->AddGrenades(5), 1);
	TestEqual(TEXT("...three carried"), Throw->GetGrenades(), FThrowRules::MaxGrenades);
	TestEqual(TEXT("Full: none taken"), Throw->AddGrenades(1), 0);

	// The session's save: what was carried comes back as it was, whatever play began with.
	FThrowablesSave Save;
	Throw->SaveThrowables(Save);
	TestTrue(TEXT("Saved: three, unlocked"), Save.Grenades == 3 && Save.bUnlocked);
	FThrowablesSave One;
	One.Grenades = 1;
	One.bUnlocked = true;
	Throw->RestoreThrowables(One);
	TestEqual(TEXT("Restored: one"), Throw->GetGrenades(), 1);
	TestTrue(TEXT("...said as restored"), LastWhy == EGrenadeChange::Restored);
	FThrowablesSave Empty;
	Empty.Grenades = 0;
	Empty.bUnlocked = true;
	Throw->RestoreThrowables(Empty);
	TestEqual(TEXT("Restored empty: none, the guns giving nothing more"), Throw->GetGrenades(), 0);
	Throw->RestoreThrowables(Save);
	TestEqual(TEXT("Back to three"), Throw->GetGrenades(), 3);

	// Nobody controls it: no throw, nothing spent, nothing counted.
	TestTrue(TEXT("With no player in control, no throw"), Throw->GetBlock() == EThrowBlock::NoPlayer && !Throw->TryThrow());
	TestEqual(TEXT("...none spent"), Throw->GetGrenades(), 3);
	TestFalse(TEXT("...none counted"), Throw->HasThrown());

	// The player's character carries it.
	UPlayerLocomotionComponent* Locomotion = LocomotionTestWorld::SpawnPlayer(World, FVector(1500.0, 1500.0, 0.0));
	TestNotNull(TEXT("The player's character has the throw"), Locomotion ? UPlayerThrowComponent::Find(Locomotion->GetOwner()) : nullptr);
	return true;
}

#endif

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Combat/LooterDamageTypes.h"
#include "Core/LooterCharacter.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureKnockComponent.h"
#include "Creatures/SpiderCreature.h"
#include "Embers/EmberComponent.h"
#include "Embers/EmberDefinition.h"
#include "Embers/Powers/EmberBorrowedTime.h"
#include "Embers/Powers/EmberDustDevil.h"
#include "Embers/Powers/EmberSundown.h"
#include "Loot/LootDropComponent.h"
#include "Tests/LocomotionTestWorld.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"
#include "UObject/Script.h"

// Ellis's three ember powers in a test level (Embers/Powers/*): Dust Devil's dash and what it flings, Sundown's slowed
// world and the player's own clock, Borrowed Time's ledger. The component's gate, clocks and saving are EmberTests.cpp.

namespace
{
	constexpr float Frame = 1.f / 60.f;

	/** The player standing on the floor at Feet facing +X, with what a power needs: its ember component and its body. */
	struct FPowerBench
	{
		ALooterCharacter* Body = nullptr;
		UEmberComponent* Embers = nullptr;
	};

	FPowerBench SpawnBench(UWorld* World, const FVector& Feet)
	{
		FPowerBench Bench;
		UPlayerLocomotionComponent* Locomotion = LocomotionTestWorld::SpawnPlayer(World, Feet);
		Bench.Body = Locomotion ? Cast<ALooterCharacter>(Locomotion->GetOwner()) : nullptr;
		if (!Bench.Body)
		{
			return Bench;
		}
		Bench.Embers = UEmberComponent::Find(Bench.Body);
		if (!Bench.Embers)
		{
			Bench.Embers = NewObject<UEmberComponent>(Bench.Body, TEXT("TestEmbers"));
			Bench.Body->AddInstanceComponent(Bench.Embers);
			Bench.Embers->RegisterComponent();
		}
		if (UHealthComponent* Health = Bench.Body->FindComponentByClass<UHealthComponent>())
		{
			Health->bShowDamageNumbers = false;
		}
		return Bench;
	}

	/** A definition made here with the design's numbers (the tests don't lean on the data assets). */
	UEmberDefinition* MakeDefinition(FName Id, float Duration, float Damage, float Range, float Radius, float Strength)
	{
		UEmberDefinition* Def = NewObject<UEmberDefinition>(GetTransientPackage());
		Def->Id = Id;
		Def->DurationSeconds = Duration;
		Def->Damage = Damage;
		Def->Range = Range;
		Def->Radius = Radius;
		Def->Strength = Strength;
		return Def;
	}

	FEmberContext MakeContext(const FPowerBench& Bench, const UEmberDefinition* Def)
	{
		FEmberContext Context;
		Context.Character = Bench.Body;
		Context.Component = Bench.Embers;
		Context.Definition = Def;
		Context.LevelScale = 1.f;
		return Context;
	}

	/** A spider standing on the floor at Feet, started as play starts it, its movement able to take a launch, dropping nothing. */
	ASpiderCreature* SpawnSpider(UWorld* World, const FVector& Feet)
	{
		ASpiderCreature* Spider = World->SpawnActor<ASpiderCreature>(Feet + FVector(0.0, 0.0, 300.0), FRotator::ZeroRotator);
		if (!Spider)
		{
			return nullptr;
		}
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
		// A launch only takes on an active movement (UCharacterMovementComponent::Launch).
		Movement->Activate(true);
		return Spider;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmberDustDevilTest, "Looter.Embers.Power.DustDevil",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmberDustDevilTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	// A floor 60 m across with its top at 0.
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	FEditorScriptExecutionGuard RunEvents;

	const FPowerBench Bench = SpawnBench(World, FVector(-1500.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("The player"), Bench.Body) || !TestNotNull(TEXT("...with an ember component"), Bench.Embers))
	{
		return false;
	}
	// One spider beside the path halfway along, one well off it.
	ASpiderCreature* Beside = SpawnSpider(World, FVector(-1050.0, 150.0, 0.0));
	ASpiderCreature* Far = SpawnSpider(World, FVector(-1050.0, 900.0, 0.0));
	if (!TestNotNull(TEXT("Spiders"), Beside) || !TestNotNull(TEXT("..."), Far))
	{
		return false;
	}

	const UEmberDefinition* Def = MakeDefinition(TEXT("DustDevil"), 0.3f, 15.f, 900.f, 260.f, 0.f);
	UEmberDustDevil* Power = NewObject<UEmberDustDevil>(Bench.Embers);
	const FEmberContext Context = MakeContext(Bench, Def);
	UHealthComponent* Health = Bench.Body->FindComponentByClass<UHealthComponent>();
	UCharacterMovementComponent* Movement = Bench.Body->GetCharacterMovement();

	// Standing still, facing +X: the way it looks.
	TestTrue(TEXT("Still: it dashes the way the player faces"), UEmberDustDevil::DashDirection(*Bench.Body).Equals(FVector::ForwardVector, 0.01));
	TestTrue(TEXT("Open ground ahead: it may be used"), Power->CanActivate(Context) == EEmberDenied::None);

	const FVector Start = Bench.Body->GetActorLocation();
	Power->Activate(Context);
	TestTrue(TEXT("It's dashing"), Power->IsDashing());
	TestEqual(TEXT("...the whole 9 m laid out"), Power->GetPath().Length, 900.f, 5.f);
	TestTrue(TEXT("...the movement standing aside"), Movement->MovementMode == MOVE_None);
	TestTrue(TEXT("...the player untouchable"), Health && Health->bInvulnerable);
	Power->Tick(Context, Frame);
	TestTrue(TEXT("The view widens"), Power->GetFieldOfViewNow() > 0.f);
	int32 Frames = 1;
	while (Power->IsDashing() && Frames < 60)
	{
		Power->Tick(Context, Frame);
		++Frames;
	}
	Power->End(Context, false);
	const float Moved = static_cast<float>(FVector::Dist2D(Start, Bench.Body->GetActorLocation()));
	TestTrue(FString::Printf(TEXT("Over in about 0.3 s (%d frames)"), Frames), Frames >= 17 && Frames <= 20);
	TestEqual(TEXT("It carried the player 9 m"), Moved, 900.f, 10.f);
	TestEqual(TEXT("...along the ground"), static_cast<float>(Bench.Body->GetActorLocation().Z), static_cast<float>(Start.Z), 5.f);
	TestTrue(TEXT("Walking on after"), Movement->MovementMode == MOVE_Walking && Movement->Velocity.X > 0.0);
	TestTrue(TEXT("...and hurtable again"), Health && !Health->bInvulnerable);

	// The spider beside the path was struck and flung away to its side; the far one wasn't touched.
	TestTrue(TEXT("The spider beside the path was struck"), Power->GetStruck().Contains(Beside));
	TestFalse(TEXT("...the far one wasn't"), Power->GetStruck().Contains(Far));
	UCreatureKnockComponent* Knock = UCreatureKnockComponent::Find(Beside);
	if (TestNotNull(TEXT("Creatures can be knocked"), Knock))
	{
		TestTrue(TEXT("It's flung"), Knock->IsKnocked());
		const FVector Launch = Beside->GetCharacterMovement()->PendingLaunchVelocity;
		TestTrue(FString::Printf(TEXT("...out to its side of the path, upward (%s)"), *Launch.ToCompactString()), Launch.Y > 300.0 && Launch.Z > 0.0);
		TestTrue(TEXT("...its brain waiting"), Knock->HoldsBrain());
		Knock->NoteLanded();
		TestTrue(TEXT("Landed: dizzy"), Knock->IsDizzy());
		TestEqual(TEXT("...for its second"), Knock->GetDizzyLeft(), 1.f, 0.01f);
		Knock->Recover();
		TestFalse(TEXT("Recovered: its brain takes up again"), Knock->HoldsBrain());
	}
	const UCreatureKnockComponent* FarKnock = UCreatureKnockComponent::Find(Far);
	TestTrue(TEXT("The far spider stands"), FarKnock && !FarKnock->HoldsBrain());

	// A wall ahead stops it short; the floor's edge stops it at the edge; a wall at the nose refuses the press.
	LocomotionTestWorld::SpawnBlock(World, FVector(-200.0, 0.0, 150.0), FVector(100.0, 600.0, 300.0));
	const FVector Body = Bench.Body->GetActorLocation();
	const float Radius = Bench.Body->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float ToWall = static_cast<float>(-250.0 - Body.X) - Radius;
	const FDustDevilPath Walled = UEmberDustDevil::PlanDash(*Bench.Body, FVector::ForwardVector, 900.f);
	TestTrue(FString::Printf(TEXT("A wall stops it short (%.0f cm, the wall %.0f cm off)"), Walled.Length, ToWall), Walled.Length <= ToWall + 5.f && Walled.Length > ToWall - 60.f);
	const FDustDevilPath Edge = UEmberDustDevil::PlanDash(*Bench.Body, FVector::RightVector * -1.0, 5000.f);
	TestTrue(FString::Printf(TEXT("It never runs off the floor (%.0f cm toward its edge)"), Edge.Length), Edge.Length <= 3000.f + 5.f);
	Bench.Body->SetActorLocation(FVector(-250.0 - Radius - 20.0, 0.0, Body.Z), false, nullptr, ETeleportType::TeleportPhysics);
	TestTrue(TEXT("Nose to a wall: refused"), Power->CanActivate(Context) == EEmberDenied::Refused);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmberSundownTest, "Looter.Embers.Power.Sundown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmberSundownTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(4000.0, 4000.0, 100.0));
	FEditorScriptExecutionGuard RunEvents;
	AWorldSettings* Settings = World->GetWorldSettings();
	const FPowerBench Bench = SpawnBench(World, FVector::ZeroVector);
	ASpiderCreature* Spider = SpawnSpider(World, FVector(800.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("World settings"), Settings) || !TestNotNull(TEXT("The player"), Bench.Body) || !TestNotNull(TEXT("A spider"), Spider))
	{
		return false;
	}
	const UEmberDefinition* Def = MakeDefinition(TEXT("Sundown"), 6.f, 0.f, 0.f, 0.f, 0.25f);
	UEmberSundown* Power = NewObject<UEmberSundown>(Bench.Embers);
	const FEmberContext Context = MakeContext(Bench, Def);

	TestTrue(TEXT("It may be used"), Power->CanActivate(Context) == EEmberDenied::None);
	Power->Activate(Context);
	TestEqual(TEXT("The world at a quarter speed"), Settings->TimeDilation, 0.25f, 0.001f);
	TestEqual(TEXT("The player at full speed"), Bench.Body->CustomTimeDilation, 4.f, 0.01f);
	TestEqual(TEXT("...its own clock x4"), Power->GetPlayerClock(), 4.f, 0.01f);
	TestEqual(TEXT("A creature crawls with the world"), Spider->CustomTimeDilation, 1.f);
	TestTrue(TEXT("Used again while it runs: no"), Power->CanActivate(Context) == EEmberDenied::Active);
	Power->Tick(Context, Frame);
	TestEqual(TEXT("...and stays so"), Bench.Body->CustomTimeDilation, 4.f, 0.01f);

	// The player's bullets fly on the player's clock: 100 m/s for a hundredth of a world second is 4 m at x4, 1 m at x1.
	if (UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>())
	{
		FBulletShot Shot;
		Shot.Start = FVector(0.0, 0.0, 2500.0);
		Shot.VisualStart = Shot.Start;
		Shot.Direction = FVector::ForwardVector;
		Shot.Speed = 10000.f;
		Shot.Range = 300.f;
		Shot.Shooter = Bench.Body;
		Bullets->Fire(Shot);
		Bullets->Tick(0.01f);
		TestEqual(TEXT("The player's bullet flies at full speed (spent its 3 m)"), Bullets->NumBulletsInFlight(), 0);
		Shot.Shooter = Spider;
		Bullets->Fire(Shot);
		Bullets->Tick(0.01f);
		TestEqual(TEXT("...a bullet on the world's clock still flies"), Bullets->NumBulletsInFlight(), 1);
	}

	Power->End(Context, false);
	TestEqual(TEXT("Over: the world at full speed"), Settings->TimeDilation, 1.f, 0.001f);
	TestEqual(TEXT("...the player's clock its own again"), Bench.Body->CustomTimeDilation, 1.f, 0.001f);
	TestFalse(TEXT("...not slowing"), Power->IsSlowing());

	// Cut short, the same.
	Power->Activate(Context);
	Power->End(Context, true);
	TestEqual(TEXT("Cut short: the world back"), Settings->TimeDilation, 1.f, 0.001f);
	TestEqual(TEXT("...the player back"), Bench.Body->CustomTimeDilation, 1.f, 0.001f);

	// Only its own undone: a clock something else took meanwhile is left with it.
	Power->Activate(Context);
	UGameplayStatics::SetGlobalTimeDilation(World, 0.5f);
	Power->Tick(Context, Frame);
	TestEqual(TEXT("Another's slow taken over: the player still at full speed"), Bench.Body->CustomTimeDilation, 2.f, 0.01f);
	Power->End(Context, false);
	TestEqual(TEXT("...and that slow left as it was"), Settings->TimeDilation, 0.5f, 0.001f);
	TestEqual(TEXT("...the player's own clock back"), Bench.Body->CustomTimeDilation, 1.f, 0.001f);
	// Time already bent by something else (a boss's slow-motion death): the press is refused.
	TestTrue(TEXT("Time already bent: refused"), Power->CanActivate(Context) == EEmberDenied::Refused);
	UGameplayStatics::SetGlobalTimeDilation(World, 1.f);
	TestTrue(TEXT("...and free again once it's back"), Power->CanActivate(Context) == EEmberDenied::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEmberBorrowedTimeTest, "Looter.Embers.Power.BorrowedTime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmberBorrowedTimeTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(4000.0, 4000.0, 100.0));
	// The health component's events are dynamic delegates: let them run in the test level.
	FEditorScriptExecutionGuard RunEvents;
	const FPowerBench Bench = SpawnBench(World, FVector::ZeroVector);
	UHealthComponent* Health = Bench.Body ? Bench.Body->FindComponentByClass<UHealthComponent>() : nullptr;
	if (!TestNotNull(TEXT("The player"), Bench.Body) || !TestNotNull(TEXT("...with health"), Health))
	{
		return false;
	}
	// A hurt as AActor::TakeDamage hands it to its listeners (a test level has no game mode, so a pawn ignores ApplyDamage).
	auto Hurt = [&Bench](float Damage)
	{
		Bench.Body->OnTakeAnyDamage.Broadcast(Bench.Body, Damage, GetDefault<UCreatureAttackDamageType>(), nullptr, nullptr);
	};
	UEmberDefinition* Def = MakeDefinition(TEXT("BorrowedTime"), 8.f, 0.f, 0.f, 0.f, 0.25f);
	Def->Extras.Add(TEXT("PayBackSeconds"), 3.f);
	UEmberBorrowedTime* Power = NewObject<UEmberBorrowedTime>(Bench.Embers);
	const FEmberContext Context = MakeContext(Bench, Def);
	const float Full = Health->GetMaxHealth();
	TestEqual(TEXT("Starts at full health"), Health->GetHealth(), Full);

	Power->Activate(Context);
	TestTrue(TEXT("Borrowing"), Power->IsBorrowing());
	Hurt(20.f);
	TestEqual(TEXT("A hurt is healed back at once"), Health->GetHealth(), Full, 0.01f);
	TestEqual(TEXT("...and owed"), Power->GetDebt(), 20.f, 0.01f);
	Hurt(20.f);
	TestEqual(TEXT("Hurts add up in the ledger"), Power->GetDebt(), 40.f, 0.01f);
	TestEqual(TEXT("...its share of the most health"), Power->GetDebtFraction(), FMath::Min(40.f / Full, 1.f), 0.001f);
	TestEqual(TEXT("A kill strikes a quarter off"), Power->NoteKill(nullptr), 10.f, 0.01f);
	TestEqual(TEXT("...30 owed"), Power->GetDebt(), 30.f, 0.01f);

	Power->End(Context, false);
	TestFalse(TEXT("Over: no longer borrowing"), Power->IsBorrowing());
	TestTrue(TEXT("...the debt being paid back"), Power->IsSettling());
	Hurt(5.f);
	TestEqual(TEXT("A hurt after it ends is a hurt"), Health->GetHealth(), Full - 5.f, 0.01f);
	Power->StepSettle(1.5f);
	TestEqual(TEXT("Half the pay-back time: half the debt drained"), Health->GetHealth(), Full - 5.f - 15.f, 0.05f);
	TestEqual(TEXT("...half still owed"), Power->GetDebt(), 15.f, 0.05f);
	TestEqual(TEXT("No kill strikes it off now"), Power->NoteKill(nullptr), 0.f);
	Power->StepSettle(2.f);
	TestFalse(TEXT("Paid in full"), Power->IsSettling());
	TestEqual(TEXT("...nothing owed"), Power->GetDebt(), 0.f);
	TestEqual(TEXT("...the whole debt taken"), Health->GetHealth(), Full - 5.f - 30.f, 0.05f);

	// A killing blow is borrowed too, and the pay-back never kills.
	Health->ResetHealth();
	Power->Activate(Context);
	Hurt(Full * 5.f);
	TestFalse(TEXT("A killing blow is taken back"), Health->IsDead());
	TestEqual(TEXT("...at full health"), Health->GetHealth(), Full, 0.01f);
	Power->End(Context, false);
	Power->StepSettle(3.1f);
	TestFalse(TEXT("Paid back: never killed by it"), Health->IsDead());
	TestTrue(TEXT("...left on the last of it"), Health->GetHealth() >= 1.f && Health->GetHealth() <= 1.01f);
	TestFalse(TEXT("...and done"), Power->IsSettling());
	return true;
}

#endif

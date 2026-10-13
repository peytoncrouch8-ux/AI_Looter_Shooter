#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The Venom Spitter keeping its distance on a floor, carried as its walking movement would carry it (a test level never
// moves anything itself): it backs away from a player too close, always facing them, and settles in its band; it comes in
// from far off and stops at its band's edge; penned in with the player close, it's cornered, and bites once they're in
// reach. SpitterTests.cpp has its rules, its spit, its puddles and its page.

#include "Creatures/SpitterRules.h"
#include "Creatures/SpitterSpider.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

namespace
{
	constexpr float KeepFrame = 1.f / 60.f;

	/** Its walking movement given its capsule (a test level's movement takes none), walking, standing still. */
	void GiveWalking(ACreatureBase& Creature)
	{
		UCharacterMovementComponent* Movement = Creature.GetCharacterMovement();
		Movement->SetUpdatedComponent(Creature.GetCapsuleComponent());
		Movement->SetMovementMode(MOVE_Falling);
		Movement->SetMovementMode(MOVE_Walking);
		Movement->Velocity = FVector::ZeroVector;
	}

	/**
	 * One frame of the walking movement carrying it the way it asked (taking its input) at its walking speed, sliding along
	 * what blocks it, as the steering play tests carry a spider; its velocity is what it moved.
	 */
	void CarryFrame(ACreatureBase& Creature)
	{
		UCharacterMovementComponent* Movement = Creature.GetCharacterMovement();
		const FVector Wanted = Creature.ConsumeMovementInputVector();
		if (Creature.IsUnsticking())
		{
			Movement->Velocity = FVector::ZeroVector;
			return;
		}
		const FVector Was = Creature.GetActorLocation();
		FVector Move = FVector(Wanted.X, Wanted.Y, 0.0).GetClampedToMaxSize(1.0) * (Movement->MaxWalkSpeed * KeepFrame);
		for (int32 Slide = 0; Slide < 2 && !Move.IsNearlyZero(); ++Slide)
		{
			FHitResult Hit;
			Creature.SetActorLocation(Creature.GetActorLocation() + Move, true, &Hit);
			if (!Hit.bBlockingHit)
			{
				break;
			}
			Creature.MoveBlockedBy(Hit);
			const FVector Normal = FVector(Hit.Normal.X, Hit.Normal.Y, 0.0).GetSafeNormal();
			const FVector Left = Move * (1.0 - Hit.Time);
			Move = Left - Normal * FVector::DotProduct(Left, Normal);
		}
		const FVector Moved = Creature.GetActorLocation() - Was;
		Movement->Velocity = FVector(Moved.X, Moved.Y, 0.0) / KeepFrame;
	}

	float FlatDistance(const AActor& A, const AActor& B)
	{
		return static_cast<float>(FVector::Dist2D(A.GetActorLocation(), B.GetActorLocation()));
	}

	/** How squarely it faces Other (1: straight at it). */
	float Facing(const AActor& Creature, const AActor& Other)
	{
		return static_cast<float>(FVector::DotProduct(Creature.GetActorForwardVector().GetSafeNormal2D(),
			(Other.GetActorLocation() - Creature.GetActorLocation()).GetSafeNormal2D()));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpitterKeepsRangeTest, "Looter.Creatures.Spitter.KeepsRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpitterKeepsRangeTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(10000.0, 10000.0, 100.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(500.0, 0.0, 89.0));
	ASpitterSpider* Spitter = Cast<ASpitterSpider>(EncounterTestWorld::SpawnCreature(World, ASpitterSpider::StaticClass(), FVector::ZeroVector));
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("A Spitter"), Spitter))
	{
		return false;
	}
	GiveWalking(*Spitter);
	// Its spit held off, to watch it move.
	Spitter->HoldSpit(1000.f);
	Spitter->AlertTo(Player);

	// 5 m off: it backs away, facing the player, and settles in its band.
	bool bBackedOff = false;
	float WorstFacing = 1.f;
	for (float Clock = 0.f; Clock < 4.f; Clock += KeepFrame)
	{
		Spitter->Tick(KeepFrame);
		CarryFrame(*Spitter);
		if (Spitter->GetKeep() == ESpitterKeep::BackOff)
		{
			bBackedOff = true;
			if (Clock > 0.5f)
			{
				WorstFacing = FMath::Min(WorstFacing, Facing(*Spitter, *Player));
			}
		}
	}
	const float Settled = FlatDistance(*Spitter, *Player);
	TestTrue(TEXT("A player 5 m off: it backs away"), bBackedOff);
	TestTrue(FString::Printf(TEXT("...facing them as it goes (worst %.2f)"), WorstFacing), WorstFacing > 0.8f);
	TestTrue(FString::Printf(TEXT("...and settles in its band, 10 to 16 m off (%.0f cm)"), Settled),
		Settled >= Spitter->BackOffWithin - 50.f && Settled <= Spitter->CloseInTo + 50.f);
	TestFalse(TEXT("...never once cornered on open ground"), Spitter->IsCornered());

	// In its band it holds or strafes round the player, never closing on them or running off.
	float Nearest = Settled;
	float Farthest = Settled;
	bool bStrafed = false;
	for (float Clock = 0.f; Clock < 4.f; Clock += KeepFrame)
	{
		Spitter->Tick(KeepFrame);
		CarryFrame(*Spitter);
		const float Distance = FlatDistance(*Spitter, *Player);
		Nearest = FMath::Min(Nearest, Distance);
		Farthest = FMath::Max(Farthest, Distance);
		bStrafed |= Spitter->GetKeep() == ESpitterKeep::Strafe;
	}
	TestTrue(TEXT("In its band it sidesteps now and then"), bStrafed);
	TestTrue(FString::Printf(TEXT("...staying in its band (%.0f to %.0f cm)"), Nearest, Farthest),
		Nearest >= Spitter->BackOffWithin - 100.f && Farthest <= Spitter->CloseInTo + 150.f);

	// The player 25 m off: its brain brings it in, turning to where it walks, and stops at its band's edge.
	const FVector Away = (Player->GetActorLocation() - Spitter->GetActorLocation()).GetSafeNormal2D();
	const FVector Far = Spitter->GetActorLocation() + Away * 2500.0;
	Player->SetActorLocation(FVector(Far.X, Far.Y, 89.0));
	bool bClosed = false;
	for (float Clock = 0.f; Clock < 5.f; Clock += KeepFrame)
	{
		Spitter->Tick(KeepFrame);
		CarryFrame(*Spitter);
		bClosed |= Spitter->GetKeep() == ESpitterKeep::Close && Spitter->GetCharacterMovement()->bOrientRotationToMovement;
	}
	const float Closed = FlatDistance(*Spitter, *Player);
	TestTrue(TEXT("25 m off: its brain brings it in"), bClosed);
	TestTrue(FString::Printf(TEXT("...to its band's edge (%.0f cm)"), Closed), Closed <= Spitter->CloseInTo + 100.f && Closed >= Spitter->BackOffWithin);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpitterCorneredTest, "Looter.Creatures.Spitter.Cornered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpitterCorneredTest::RunTest(const FString& Parameters)
{
	// Penned on three sides (a wall behind it and one each side) with the player 4 m in front: no way back, so it's
	// cornered and stands; it bites only once the player comes within reach.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(4000.0, 4000.0, 100.0));
	LocomotionTestWorld::SpawnBlock(World, FVector(-170.0, 0.0, 100.0), FVector(40.0, 380.0, 200.0));
	LocomotionTestWorld::SpawnBlock(World, FVector(-55.0, 170.0, 100.0), FVector(270.0, 40.0, 200.0));
	LocomotionTestWorld::SpawnBlock(World, FVector(-55.0, -170.0, 100.0), FVector(270.0, 40.0, 200.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(400.0, 0.0, 89.0));
	ASpitterSpider* Spitter = Cast<ASpitterSpider>(EncounterTestWorld::SpawnCreature(World, ASpitterSpider::StaticClass(), FVector::ZeroVector));
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("A Spitter"), Spitter))
	{
		return false;
	}
	GiveWalking(*Spitter);
	Spitter->HoldSpit(1000.f);
	Spitter->AlertTo(Player);
	const FVector Start = Spitter->GetActorLocation();
	for (float Clock = 0.f; Clock < 1.f; Clock += KeepFrame)
	{
		Spitter->Tick(KeepFrame);
		CarryFrame(*Spitter);
	}
	TestTrue(TEXT("Penned in with the player 4 m off: cornered"), Spitter->IsCornered() && Spitter->GetKeep() == ESpitterKeep::Cornered);
	TestTrue(TEXT("...it stands its ground"), FVector::Dist2D(Spitter->GetActorLocation(), Start) < 30.0);
	TestTrue(TEXT("...without biting at 4 m"), Spitter->GetCreatureState() != ECreatureState::Attack);

	Player->SetActorLocation(FVector(Start.X + 200.0, Start.Y, 89.0));
	float BitAt = -1.f;
	for (float Clock = 0.f; Clock < 0.5f; Clock += KeepFrame)
	{
		Spitter->Tick(KeepFrame);
		if (Spitter->GetCreatureState() == ECreatureState::Attack)
		{
			BitAt = Clock;
			break;
		}
	}
	TestTrue(FString::Printf(TEXT("The player within reach: it bites (%.2f s)"), BitAt), BitAt >= 0.f && Spitter->GetMove() == ESpitterMove::Bite);
	return true;
}

#endif

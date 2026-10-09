// ACreatureBase: room between creatures. Their capsules already keep their middles apart, but a body can reach well past
// its capsule (a spider's abdomen a meter behind it, a slime's foot, an Unpaid's arms), so a pack chasing in a line ran
// heads into the abdomens ahead and stood inside each other once it reached its prey. Each kind says what ground its body
// covers (GetFootprint: a flat capsule along its facing), and creatures keep those apart: moving, the crowd bends its way
// (SpacedDirection, from the steering); standing, it shuffles aside (KeepSpacing). A boss holds its ground, and others go
// round it.

#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/** How often (seconds) a standing creature looks at its neighbours again; moving ones look at every steering update. */
	constexpr float SpacingInterval = 0.1f;
	/** Footprints keep this much farther apart than touching (cm, at size 1). */
	constexpr float SpacingMargin = 10.f;
	/** How hard the crowd bends a moving creature's way, against its own way's 1. */
	constexpr float SteerWeight = 1.2f;
	/** How fast a standing creature shuffles aside at the deepest overlap (cm/s, at size 1). */
	constexpr float ShuffleSpeed = 110.f;
	/** A push weaker than this is let be: no shuffling over a touch. */
	constexpr float MinPush = 0.05f;
	/** Creatures this far apart in height (cm, at size 1) stand on different ground and never crowd each other. */
	constexpr float OtherGroundHeight = 250.f;
}

ACreatureBase::FFootprint ACreatureBase::GetFootprint() const
{
	FFootprint Footprint;
	Footprint.Radius = GetCapsuleComponent()->GetUnscaledCapsuleRadius();
	return Footprint;
}

void ACreatureBase::GetFootprintInWorld(FVector& OutFront, FVector& OutBack, float& OutRadius) const
{
	const FFootprint Footprint = GetFootprint();
	const FVector Middle = GetActorLocation();
	const FVector Ahead = FRotator(0.0, GetActorRotation().Yaw, 0.0).Vector();
	OutFront = Middle + Ahead * (Footprint.Front * SizeScale);
	OutBack = Middle + Ahead * (Footprint.Back * SizeScale);
	OutRadius = Footprint.Radius * SizeScale;
}

void ACreatureBase::ForEachCreature(TFunctionRef<void(ACreatureBase&)> Visit) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this))
	{
		// A copy of the list (on the stack for any ordinary level): a visit may set a creature on the player, and whatever
		// that sets off may spawn a creature, which joins the list while it's being walked.
		const TArray<TWeakObjectPtr<ACreatureBase>, TInlineAllocator<64>> Creatures(Encounters->GetTrackedCreatures());
		for (const TWeakObjectPtr<ACreatureBase>& Each : Creatures)
		{
			if (ACreatureBase* Creature = Each.Get())
			{
				Visit(*Creature);
			}
		}
		return;
	}
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		Visit(**It);
	}
}

FVector ACreatureBase::SpacingPushNow() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return FVector::ZeroVector;
	}
	const FVector Middle = GetActorLocation();
	FVector MyFront;
	FVector MyBack;
	float MyRadius = 0.f;
	GetFootprintInWorld(MyFront, MyBack, MyRadius);
	MyFront.Z = MyBack.Z = 0.0;
	const float MyReach = static_cast<float>(FMath::Max(FVector::Dist2D(MyFront, Middle), FVector::Dist2D(MyBack, Middle))) + MyRadius;

	FVector Push = FVector::ZeroVector;
	ForEachCreature([&](const ACreatureBase& Other)
	{
		if (&Other == this || Other.IsDead() || Other.IsHidden() || Other.IsActorBeingDestroyed())
		{
			return;
		}
		const FVector OtherMiddle = Other.GetActorLocation();
		const float Larger = FMath::Max(SizeScale, Other.SizeScale);
		if (FMath::Abs(OtherMiddle.Z - Middle.Z) > OtherGroundHeight * Larger)
		{
			return;
		}
		FVector OtherFront;
		FVector OtherBack;
		float OtherRadius = 0.f;
		Other.GetFootprintInWorld(OtherFront, OtherBack, OtherRadius);
		OtherFront.Z = OtherBack.Z = 0.0;
		const float Margin = SpacingMargin * 0.5f * (SizeScale + Other.SizeScale);
		const float OtherReach = static_cast<float>(FMath::Max(FVector::Dist2D(OtherFront, OtherMiddle), FVector::Dist2D(OtherBack, OtherMiddle)))
			+ OtherRadius;
		if (FVector::Dist2D(Middle, OtherMiddle) > MyReach + OtherReach + Margin)
		{
			return;
		}
		FVector Mine;
		FVector Theirs;
		FMath::SegmentDistToSegmentSafe(MyFront, MyBack, OtherFront, OtherBack, Mine, Theirs);
		const float Apart = MyRadius + OtherRadius + Margin;
		FVector Away = Mine - Theirs;
		Away.Z = 0.0;
		const float Gap = static_cast<float>(Away.Size());
		if (Gap >= Apart)
		{
			return;
		}
		if (Gap < 1.f)
		{
			// Lines crossing: apart along the middles, or, on top of each other, to whichever side its name puts it.
			Away = Middle - OtherMiddle;
			Away.Z = 0.0;
			if (Away.SizeSquared() < 1.0)
			{
				Away = GetActorRightVector() * (GetUniqueID() < Other.GetUniqueID() ? 1.0 : -1.0);
			}
		}
		Push += Away.GetSafeNormal2D() * ((Apart - Gap) / Apart);
	});
	return Push.GetClampedToMaxSize(1.5f);
}

FVector ACreatureBase::SpacedDirection(const FVector& Desired)
{
	SpacingTimer = SpacingInterval;
	SpacingPush = bPoseFrozen || CurrentRank == ECreatureRank::Boss ? FVector::ZeroVector : SpacingPushNow();
	if (SpacingPush.SizeSquared() < FMath::Square(MinPush))
	{
		return Desired;
	}
	FVector Bent = Desired + SpacingPush * SteerWeight;
	Bent.Z = 0.0;
	if (FVector::DotProduct(Bent, Desired) <= 0.0)
	{
		// The crowd is right in its way: round it, on the side the push leans to (never back the way it came).
		const FVector Across = FVector::CrossProduct(FVector::UpVector, Desired).GetSafeNormal2D();
		const double Lean = FVector::DotProduct(SpacingPush, Across);
		Bent = Across * (Lean >= 0.0 ? 1.0 : -1.0) + Desired * 0.25;
	}
	const FVector Way = Bent.GetSafeNormal2D();
	return Way.IsNearlyZero() ? Desired : Way;
}

void ACreatureBase::KeepSpacing(float DeltaSeconds)
{
	// Busy attacking or dead, held back, far away out of view, or a boss holding its ground: let be. Moving, its steering
	// keeps it clear already.
	if (State == ECreatureState::Dead || State == ECreatureState::Attack || bPassive || bPoseFrozen || CurrentRank == ECreatureRank::Boss
		|| !GetPendingMovementInputVector().IsNearlyZero())
	{
		return;
	}
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement || !Movement->IsMovingOnGround())
	{
		return;
	}
	SpacingTimer -= DeltaSeconds;
	if (SpacingTimer <= 0.f)
	{
		SpacingTimer = SpacingInterval;
		SpacingPush = SpacingPushNow();
	}
	const float Strength = static_cast<float>(SpacingPush.Size());
	if (Strength < MinPush)
	{
		return;
	}
	// A shuffle, not a run: slow, and slower the less it overlaps (the input's size scales the walking speed).
	const float Speed = FMath::Max(Movement->MaxWalkSpeed, 1.f);
	AddMovementInput(SpacingPush / Strength, FMath::Min(Strength, 1.f) * FMath::Min(ShuffleSpeed * SizeScale / Speed, 1.f));
}

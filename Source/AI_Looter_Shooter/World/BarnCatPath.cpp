// ABarnCat's ways about its porch: a few steps along the boards, a path walked or slunk point to point (a jump where
// it changes height: off the deck's edge, back up onto it), the porch's far end, the ground in front of its edge.

#include "World/BarnCat.h"
#include "World/FaunaRules.h"

namespace
{
	/** A jump down off the deck or up onto it (s), and how high it arcs (cm). */
	constexpr float JumpSeconds = 0.45f;
	constexpr float JumpArc = 16.f;

	/** A leg that changes height more than this (cm) is a jump. */
	constexpr float JumpRise = 15.f;

	/** How fast it turns walking (degrees a second); a walk turns round on the spot first past this much off its way. */
	constexpr float CatTurnRate = 300.f;
	constexpr float TurnFirstDegrees = 70.f;

	/** A stride (two steps) walking and running (cm): its paws keep pace with the boards. */
	constexpr float WalkStride = 50.f;
	constexpr float RunStride = 70.f;

	/** A deck point is within this height (cm) of its spot's. */
	constexpr float DeckSlack = 12.f;
}

void ABarnCat::StartFewSteps()
{
	// Half a metre to a metre and more along the boards between the porch's ends, back the other way near one.
	const FVector Along = PorchB - PorchA;
	const double Span = FMath::Max(Along.Size(), 1.0);
	const double Here = FMath::Clamp(FVector::DotProduct(Position - PorchA, Along) / (Span * Span), 0.0, 1.0);
	double There = Here + Random.FRandRange(50.f, 120.f) / Span * (Random.FRand() < 0.5f ? -1.0 : 1.0);
	if (There < 0.0 || There > 1.0)
	{
		There = Here - (There - Here);
	}
	bWalkingOff = false;
	const FVector Target[] = { FMath::Lerp(PorchA, PorchB, FMath::Clamp(There, 0.0, 1.0)) };
	StartPath(ECatDoing::Walk, Target, WalkSpeed);
}

void ABarnCat::StartPath(ECatDoing Kind, TConstArrayView<FVector> Points, float Speed)
{
	Doing = Kind;
	Clock = 0.f;
	Path.Reset();
	Path.Append(Points.GetData(), Points.Num());
	LegFrom = Position;
	LegDone = 0.f;
	PathSpeed = Speed;
}

bool ABarnCat::FollowPath(float DeltaSeconds)
{
	while (Path.Num() > 0)
	{
		const FVector Target = Path[0];
		const float Flat = static_cast<float>(FVector::Dist2D(LegFrom, Target));
		const float Rise = static_cast<float>(Target.Z - LegFrom.Z);
		const bool bJump = FMath::Abs(Rise) > JumpRise;
		if (Flat < 1.f && !bJump)
		{
			Path.RemoveAt(0);
			continue;
		}
		const FVector Way = Target - LegFrom;
		const float WayYaw = FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(Way.Y, Way.X)));
		if (Flat >= 1.f)
		{
			Yaw = FaunaRules::StepAngle(Yaw, WayYaw, CatTurnRate * DeltaSeconds * (bJump ? 3.f : 1.f));
			if (!bJump && FMath::Abs(FMath::FindDeltaAngleDegrees(Yaw, WayYaw)) > TurnFirstDegrees)
			{
				// Turning round on the spot before it sets off (under the porch's edge from in front of it), paws stepping.
				Stride += DeltaSeconds * 0.8f;
				Stride -= FMath::FloorToFloat(Stride);
				return false;
			}
		}
		if (bJump)
		{
			// A jump: over in a moment on an arc, the body pitched along it.
			LegDone = FMath::Min(1.f, LegDone + DeltaSeconds / JumpSeconds);
			const float Arc = (JumpArc + FMath::Max(0.f, Rise) * 0.4f) * 4.f * LegDone * (1.f - LegDone);
			Position = FMath::Lerp(LegFrom, Target, static_cast<double>(LegDone)) + FVector(0.0, 0.0, Arc);
			const float Climb = Rise + (JumpArc + FMath::Max(0.f, Rise) * 0.4f) * 4.f * (1.f - 2.f * LegDone);
			Pitch = FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(Climb, FMath::Max(Flat, 10.f))), -35.f, 35.f);
		}
		else
		{
			const float Step = PathSpeed * DeltaSeconds;
			LegDone = FMath::Min(1.f, LegDone + Step / Flat);
			Position = FMath::Lerp(LegFrom, Target, static_cast<double>(LegDone));
			Stride += Step / (Doing == ECatDoing::Slink && bRunning ? RunStride : WalkStride);
			Stride -= FMath::FloorToFloat(Stride);
			Pitch = 0.f;
		}
		if (LegDone >= 1.f)
		{
			LegFrom = Target;
			LegDone = 0.f;
			Position = Target;
			Path.RemoveAt(0);
		}
		return Path.IsEmpty();
	}
	return true;
}

FVector ABarnCat::FarEnd(const FVector& Where) const
{
	return FVector::DistSquared2D(PorchA, Where) > FVector::DistSquared2D(PorchB, Where) ? PorchA : PorchB;
}

FVector ABarnCat::StepPoint() const
{
	if (!PorchStep.IsZero())
	{
		return PorchStep;
	}
	// In front of the deck's edge, on the hiding place's ground.
	FVector Out = PorchExit - HidePoint;
	Out.Z = 0.0;
	const FVector Ahead = PorchExit + Out.GetSafeNormal() * 30.0;
	return FVector(Ahead.X, Ahead.Y, HidePoint.Z);
}

bool ABarnCat::IsOnDeck(const FVector& Where) const
{
	return FMath::Abs(Where.Z - LoafSpot.Z) < DeckSlack;
}

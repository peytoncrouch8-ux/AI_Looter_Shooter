#include "Player/Animation/LooterClimbPose.h"
#include "Player/PlayerTraversal.h"

namespace
{
	/** 0 up to From, 1 from To on, a smoothstep between. */
	float Ease(float Value, float From, float To)
	{
		if (To <= From)
		{
			return Value >= To ? 1.f : 0.f;
		}
		const float T = FMath::Clamp((Value - From) / (To - From), 0.f, 1.f);
		return T * T * (3.f - 2.f * T);
	}

	/** How long the legs take to tuck in, and how long they take to come back out before the move ends (s). */
	constexpr float LegsInSeconds = 0.10f;
	constexpr float LegsOutSeconds = 0.14f;
	/** Where in the move (0..1) the mantle's hands let go of the ledge: full until the first, gone by the second. */
	constexpr float MantleLetGoFrom = 0.5f;
	constexpr float MantleLetGoTo = 0.8f;
}

float LooterClimbPose::ApexTime(const FPlayerTraversal& Plan)
{
	const double Length = Plan.GetDuration();
	double BestTime = 0.0;
	double BestFeet = -UE_BIG_NUMBER;
	constexpr int32 Samples = 32;
	for (int32 Index = 0; Index <= Samples; ++Index)
	{
		const double At = Length * Index / Samples;
		const double Feet = Plan.FeetAt(At);
		if (Feet > BestFeet)
		{
			BestFeet = Feet;
			BestTime = At;
		}
	}
	return static_cast<float>(BestTime);
}

float LooterClimbPose::EstimateTop(const FPlayerTraversal& Plan)
{
	const double Feet0 = Plan.FeetAt(0.0);
	if (Plan.GetKind() == ETraversalKind::Vault)
	{
		// The hop's top passes the obstacle by a tuck (a little less, the plan lifts it when it has to).
		return static_cast<float>(FMath::Max(Plan.FeetAt(ApexTime(Plan)) - Feet0, 0.0)) + LooterTraversal::VaultTuck * 0.6f;
	}
	return static_cast<float>(FMath::Max(Plan.FeetAt(Plan.GetDuration()) - Feet0, 0.0));
}

FLooterClimbInput LooterClimbPose::Describe(const FPlayerTraversal& Plan, float Alpha, float TopOverFeet, float CapsuleRadius, float BodyScale,
	const FQuat& MeshRotation)
{
	FLooterClimbInput Out;
	const ETraversalKind Kind = Plan.GetKind();
	if ((Kind != ETraversalKind::Mantle && Kind != ETraversalKind::Vault) || Alpha <= UE_KINDA_SMALL_NUMBER)
	{
		return Out;
	}
	const bool bVault = Kind == ETraversalKind::Vault;
	const float Pose = FMath::Clamp(bVault ? Alpha / VaultAlphaShare : Alpha, 0.f, 1.f);
	const float Scale = FMath::Max(BodyScale, 0.1f);
	const float Length = FMath::Max(Plan.GetDuration(), UE_KINDA_SMALL_NUMBER);
	const float Time = FMath::Clamp(Plan.GetTime(), 0.f, Length);
	const float Progress = Time / Length;
	const float Remaining = Length - Time;

	// A vault's hop tops out as the body is over the obstacle's middle, a little over a third to two thirds into the move.
	const float ApexProgress = bVault ? FMath::Clamp(ApexTime(Plan) / Length, 0.3f, 0.65f) : 0.f;

	Out.bVault = bVault;
	Out.Lean = Pose;
	// The way the move goes, as the body went from its start to its end (the plan's own direction is private to it).
	FVector Way = Plan.CenterAt(static_cast<double>(Length)) - Plan.CenterAt(0.0);
	Way.Z = 0.0;
	if (!Way.IsNearlyZero())
	{
		FVector Heading = MeshRotation.UnrotateVector(Way.GetSafeNormal());
		Heading.Z = 0.0;
		Out.Heading = Heading.GetSafeNormal();
	}
	// The legs tuck in over a tenth of a second and come out over the last fraction before the move ends (so a walk on the
	// top starts from the graph's legs). The hands reach from the start; a mantle's let go of the ledge as the body comes
	// over it, a vault's once the hop is over the obstacle. Time-based ends, so a quick lip and a long climb both ease.
	Out.Legs = Pose * Ease(Time, 0.f, LegsInSeconds) * Ease(Remaining, 0.f, LegsOutSeconds);
	if (bVault)
	{
		Out.Hands = Pose * Ease(Time, 0.03f, 0.12f) * (1.f - Ease(Progress, ApexProgress - 0.05f, ApexProgress + 0.2f));
	}
	else
	{
		Out.Hands = Pose * Ease(Time, 0.f, 0.08f) * (1.f - Ease(Progress, MantleLetGoFrom, MantleLetGoTo));
	}

	// The point the hands hold is fixed in the world: the top over the feet when the move began, less how far the feet have
	// risen since, and the edge's (or the rail's) place along the way, less how far the body has gone. A mantle's body waits
	// at the face (the plan's hold, a third of the way in, is flat) with its surface a centimetre from it.
	const double Feet0 = Plan.FeetAt(0.0);
	const double FeetNow = Plan.FeetAt(Time);
	const float Top = TopOverFeet >= 0.f ? TopOverFeet : EstimateTop(Plan);
	const double PlantAlong = bVault
		? Plan.AlongAt(ApexProgress * Length) - VaultHandBefore
		: Plan.AlongAt(0.35 * Length) + CapsuleRadius + 1.0 + MantleHandDepth;
	const double Ahead = PlantAlong - Plan.AlongAt(Time);
	const double Up = Top - (FeetNow - Feet0);
	Out.LedgeAhead = FMath::Clamp(static_cast<float>(Ahead) / Scale, -30.f, 90.f);
	// A palm's depth over the top.
	Out.LedgeUp = FMath::Clamp(static_cast<float>(Up) / Scale + 2.f, -20.f, 200.f);
	return Out;
}

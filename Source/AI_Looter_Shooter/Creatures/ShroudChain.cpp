#include "Creatures/ShroudChain.h"
#include "Creatures/CreatureUpdateRate.h"

namespace
{
	/** The springs' longest step (s): short enough for the snap's stiff spring to stay steady. */
	constexpr float ChainStep = 1.f / 120.f;

	/** However the body moves, a link hangs between this far forward of straight down and just past level behind. */
	constexpr float MinHang = -30.f;
	constexpr float MaxHang = 105.f;
}

void FShroudChain::Init(TConstArrayView<float> RestAngles, float InPhaseOffset)
{
	Links.Reset();
	for (const float Angle : RestAngles)
	{
		FLink& Link = Links.AddDefaulted_GetRef();
		Link.RestAngle = Angle;
	}
	PhaseOffset = InPhaseOffset;
	RipplePhase = 0.f;
	Time = 0.f;
}

void FShroudChain::Settle()
{
	for (FLink& Link : Links)
	{
		Link.Back = Link.Side = 0.f;
		Link.BackSpeed = Link.SideSpeed = 0.f;
	}
}

float FShroudChain::HangAngle(const FVector& Direction)
{
	return static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(-Direction.X, -Direction.Z)));
}

float FShroudChain::StraightBack(const FShroudChainSettings& Settings, float RestAngle)
{
	return Settings.StraightAngle - RestAngle;
}

void FShroudChain::Step(const FShroudChainSettings& Settings, const FVector& LocalVelocity, float YawRate, float Snap, float DeltaSeconds)
{
	const float Seconds = FMath::Min(DeltaSeconds, FCreatureUpdateRate::MaxInterval);
	const int32 Count = Links.Num();
	if (Seconds <= 0.f || Count == 0)
	{
		return;
	}
	const float Snapped = FMath::Clamp(Snap, 0.f, 1.f);

	// Where the motion puts it: back behind a forward drift, aside against a sideways one and against a turn.
	const float Trail = FMath::Clamp(static_cast<float>(LocalVelocity.X) / 100.f * Settings.TrailPerMeter, -Settings.MaxTrail, Settings.MaxTrail);
	const float AsideReach = Settings.MaxTrail * 0.6f;
	const float Aside = FMath::Clamp(-static_cast<float>(LocalVelocity.Y) / 100.f * Settings.TrailPerMeter - YawRate / 100.f * Settings.TurnTrail,
		-AsideReach, AsideReach);
	// The ripple grows with speed; the sway is for rest. A lunge stills both.
	const float Speed = static_cast<float>(FVector2D(LocalVelocity.X, LocalVelocity.Y).Size());
	const float Strength = FMath::Clamp(Speed / FMath::Max(Settings.RippleFullSpeed, 1.f), 0.f, 1.f);
	const float Ripple = Settings.RippleDegrees * Strength * (1.f - Snapped);
	const float Sway = Settings.SwayDegrees * (1.f - 0.6f * Strength) * (1.f - Snapped);
	const float RippleRate = 2.f * UE_PI * Settings.RippleHz * (0.4f + 0.6f * Strength);
	const float DampingRatio = FMath::Lerp(Settings.DampingRatio, 1.f, Snapped);

	// Even steps no longer than ChainStep: a long update takes the same steps as the short ones it stands in for.
	const int32 Steps = FMath::Max(1, FMath::CeilToInt32(Seconds / ChainStep - 0.01f));
	const float Dt = Seconds / Steps;
	for (int32 StepIndex = 0; StepIndex < Steps; ++StepIndex)
	{
		Time += Dt;
		RipplePhase = FMath::Fmod(RipplePhase + RippleRate * Dt, 2.f * UE_PI);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FLink& Link = Links[Index];
			// Lower links trail further: the chain bends back behind the body, not just tilts.
			const float Down = Count > 1 ? static_cast<float>(Index) / static_cast<float>(Count - 1) : 0.f;
			const float Weight = 0.6f + 0.4f * Down;
			const float Wave = RipplePhase - Index * Settings.RippleLag + PhaseOffset;
			const float SwayAt = 2.f * UE_PI * Settings.SwayHz * Time - Index * 0.5f + PhaseOffset;
			float BackTarget = Trail * Weight + 0.35f * Ripple * FMath::Sin(Wave * 1.3f + 1.1f) + 0.5f * Sway * FMath::Sin(SwayAt * 0.73f + 0.7f);
			float SideTarget = Aside * Weight + Ripple * FMath::Sin(Wave) + Sway * FMath::Sin(SwayAt);
			BackTarget = FMath::Clamp(BackTarget, MinHang - Link.RestAngle, MaxHang - Link.RestAngle);
			// A lunge lines every link up straight behind, at once.
			BackTarget = FMath::Lerp(BackTarget, StraightBack(Settings, Link.RestAngle), Snapped);
			SideTarget = FMath::Lerp(SideTarget, 0.f, Snapped);

			const float Free = Settings.Stiffness * FMath::Max(1.f - Settings.LowerSlower * Index, 0.2f);
			const float Stiffness = FMath::Lerp(Free, Settings.SnapStiffness, Snapped);
			const float Damping = 2.f * DampingRatio * FMath::Sqrt(Stiffness);
			Link.BackSpeed += (Stiffness * (BackTarget - Link.Back) - Damping * Link.BackSpeed) * Dt;
			Link.SideSpeed += (Stiffness * (SideTarget - Link.Side) - Damping * Link.SideSpeed) * Dt;
			Link.Back += Link.BackSpeed * Dt;
			Link.Side += Link.SideSpeed * Dt;
		}
	}
}

FQuat FShroudChain::LinkRotation(int32 Index, const FVector& RestDirection) const
{
	if (!Links.IsValidIndex(Index))
	{
		return FQuat::Identity;
	}
	const FLink& Link = Links[Index];
	// Back turns it about the body's right-hand axis: from hanging down toward the back, from trailing back toward up.
	const FQuat Back(FVector::RightVector, FMath::DegreesToRadians(Link.Back));
	// Side turns it to the right, about the axis square to where it now points and to the body's right.
	const FVector Swung = Back.RotateVector(RestDirection.GetSafeNormal());
	FVector Axis = FVector::CrossProduct(Swung, FVector::RightVector);
	if (!Axis.Normalize())
	{
		Axis = FVector::ForwardVector;
	}
	return FQuat(Axis, FMath::DegreesToRadians(Link.Side)) * Back;
}

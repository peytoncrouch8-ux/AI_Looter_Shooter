// The ore lift's motion as plain math of time: the eased travel, the clanks, the swing on the grip and the bob on the
// cable (OreLiftMotion.h).

#include "World/OreLiftMotion.h"

namespace
{
	/** The cage bobbing on the stretching cable after a clank: how fast it bobs (Hz) and how fast that dies away (per s). */
	constexpr float BobHz = 2.4f;
	constexpr float BobDecay = 3.f;

	/** A clank's jolt through the floor: quick and short. */
	constexpr float JoltHz = 9.f;
	constexpr float JoltDecay = 14.f;

	/** The take-up: the cage rises as the strain comes on, overshooting a little (rate per s, Hz), and the rise melts into
	 *  the travel over its first RiseFadeSeconds. */
	constexpr float LiftOffRate = 7.f;
	constexpr float LiftOffHz = 1.75f;
	constexpr float RiseFadeSeconds = 1.6f;
	/** Its jolt is a share of a clank's, and slower. */
	constexpr float TakeUpJoltShare = 0.8f;

	/** Set down on the far landing: the dip's bounce (Hz, per s), and its jolt a share more than a clank's. */
	constexpr float LandBounceHz = 1.8f;
	constexpr float LandDecay = 4.f;
	constexpr float LandJoltShare = 1.4f;

	/** The breeze's slow rock (Hz), and how long it takes to come in once under way (s). */
	constexpr float WindHz = 0.17f;
	constexpr float WindInSeconds = 3.f;

	/** The landing's guides catch the swing over the last GuideSeconds of the travel (at most a quarter of it), and its
	 *  floor stills it over StillShare of the settle. */
	constexpr float GuideSeconds = 1.5f;
	constexpr float StillShare = 0.6f;

	float ClampedEase(float Travel, float Ease)
	{
		return FMath::Clamp(Ease, 0.f, FMath::Max(Travel, 0.f) * 0.5f);
	}

	/** The cruising speed that covers Length in Travel seconds with Ease seconds of easing at each end (cm/s). */
	float CruiseSpeed(float Travel, float Ease, float Length)
	{
		const float AtSpeed = Travel - Ease;
		return AtSpeed > UE_KINDA_SMALL_NUMBER ? Length / AtSpeed : 0.f;
	}

	/** Value eased toward Limit either way (a hyperbolic tangent): small swings as they are, a big one rounding off short of
	 *  Limit instead of stopping dead against it. */
	float SoftLimit(float Value, float Limit)
	{
		if (Limit <= UE_KINDA_SMALL_NUMBER)
		{
			return 0.f;
		}
		const float Doubled = FMath::Exp(FMath::Clamp(2.f * Value / Limit, -40.f, 40.f));
		return Limit * (Doubled - 1.f) / (Doubled + 1.f);
	}

	/** A damped wave from Since = 0 on: e^(-Decay Since) sin(2 pi Hz Since); nothing before it starts. */
	float Ring(float Since, float Hz, float Decay)
	{
		return Since <= 0.f ? 0.f : FMath::Exp(-Decay * Since) * FMath::Sin(UE_TWO_PI * Hz * Since);
	}

	/** Each sheave's place along the cable, in this run's order (cm). */
	TArray<float> SheavePlaces(const FOreLiftRunShape& Run)
	{
		TArray<float> Places;
		const float Middle = 0.5f * static_cast<float>(OreLiftMotion::SheavesPerTower - 1);
		for (const float Bend : Run.Bends)
		{
			for (int32 Wheel = 0; Wheel < OreLiftMotion::SheavesPerTower; ++Wheel)
			{
				const float Place = Bend + (static_cast<float>(Wheel) - Middle) * OreLiftMotion::SheaveSpacing;
				if (Place > 0.f && Place < Run.Length)
				{
					Places.Add(Place);
				}
			}
		}
		Places.Sort();
		return Places;
	}

	/** Carries the swing on, one fixed step at a time, to whole step Target (counted from the travel's start). */
	void StepTo(const FOreLiftRunShape& Run, const FOreLiftSwing& Swing, int32 Target, FOreLiftSwingState& State)
	{
		if (State.Step > Target)
		{
			State = FOreLiftSwingState();
		}
		if (State.Step >= Target)
		{
			return;
		}
		const float Step = OreLiftMotion::StepSeconds;
		const float Pendulum = FMath::Max(Swing.PendulumLength, 10.f);
		const float Stiffness = OreLiftMotion::Gravity / Pendulum;
		const float Damping = 2.f * FMath::Max(Swing.Damping, 0.f);
		const float Kick = FMath::DegreesToRadians(Swing.ClankKick);
		const TArray<float> Sheaves = SheavePlaces(Run);
		const float Start = Run.TakeUpSeconds;
		while (State.Step < Target)
		{
			const float From = Start + State.Step * Step;
			const float Near = OreLiftMotion::DistanceAt(Run, From);
			const float Far = OreLiftMotion::DistanceAt(Run, From + Step);
			// The grip's pull along the span it's on, over the ground: along the run's heading and across it.
			const FVector2D Shares = Run.SpanShares(Run.SpanAt(0.5f * (Near + Far)));
			const float Pull = OreLiftMotion::TravelAcceleration(From + 0.5f * Step - Start, Run.TravelSeconds, Run.EaseSeconds, Run.Length);
			// Over a tower the cable bends: the grip's way over the ground changes at once, and the cage, hanging, swings.
			const float Speed = (Far - Near) / Step;
			for (int32 Bend = 0; Bend < Run.Bends.Num(); ++Bend)
			{
				if (Near < Run.Bends[Bend] && Run.Bends[Bend] <= Far)
				{
					const FVector2D Turn = Run.SpanShares(Bend + 1) - Run.SpanShares(Bend);
					State.AlongRate += Turn.X * Speed / Pendulum;
					State.SideRate += Turn.Y * Speed / Pendulum;
				}
			}
			// Each wheel the grip lurches over checks it for a moment: the cage swings on forward.
			for (const float Sheave : Sheaves)
			{
				if (Near < Sheave && Sheave <= Far)
				{
					State.AlongRate -= Kick;
				}
			}
			// A pendulum carried by the grip (small angles): it trails what pulls it, and the swing dies away slowly.
			State.AlongRate += Step * (-Stiffness * State.Along - Damping * State.AlongRate + Shares.X * Pull / Pendulum);
			State.Along += Step * State.AlongRate;
			State.SideRate += Step * (-Stiffness * State.Side - Damping * State.SideRate + Shares.Y * Pull / Pendulum);
			State.Side += Step * State.SideRate;
			++State.Step;
		}
	}
}

int32 FOreLiftRunShape::SpanAt(float Distance) const
{
	int32 Span = 0;
	while (Span < Bends.Num() && Distance > Bends[Span])
	{
		++Span;
	}
	return Span;
}

FVector2D FOreLiftRunShape::SpanShares(int32 Span) const
{
	return Spans.IsValidIndex(Span) ? Spans[Span] : FVector2D(1.0, 0.0);
}

float OreLiftMotion::TravelDistance(float Seconds, float Travel, float Ease, float Length)
{
	if (Travel <= UE_KINDA_SMALL_NUMBER)
	{
		return Seconds > 0.f ? Length : 0.f;
	}
	const float Time = FMath::Clamp(Seconds, 0.f, Travel);
	const float Easing = ClampedEase(Travel, Ease);
	const float Cruise = CruiseSpeed(Travel, Easing, Length);
	if (Easing <= UE_KINDA_SMALL_NUMBER)
	{
		return Cruise * Time;
	}
	// The speed comes up along a smooth step and goes down along one: its integral, x^3 - x^4 / 2, at each end.
	if (Time < Easing)
	{
		const float X = Time / Easing;
		return Cruise * Easing * (X * X * X - 0.5f * X * X * X * X);
	}
	if (Time > Travel - Easing)
	{
		const float Y = (Travel - Time) / Easing;
		return Length - Cruise * Easing * (Y * Y * Y - 0.5f * Y * Y * Y * Y);
	}
	return Cruise * (Time - 0.5f * Easing);
}

float OreLiftMotion::TravelSpeed(float Seconds, float Travel, float Ease, float Length)
{
	if (Travel <= UE_KINDA_SMALL_NUMBER || Seconds <= 0.f || Seconds >= Travel)
	{
		return 0.f;
	}
	const float Easing = ClampedEase(Travel, Ease);
	const float Cruise = CruiseSpeed(Travel, Easing, Length);
	if (Easing <= UE_KINDA_SMALL_NUMBER)
	{
		return Cruise;
	}
	const float Edge = FMath::Min(Seconds, Travel - Seconds) / Easing;
	return Edge >= 1.f ? Cruise : Cruise * Edge * Edge * (3.f - 2.f * Edge);
}

float OreLiftMotion::TravelAcceleration(float Seconds, float Travel, float Ease, float Length)
{
	if (Travel <= UE_KINDA_SMALL_NUMBER || Seconds <= 0.f || Seconds >= Travel)
	{
		return 0.f;
	}
	const float Easing = ClampedEase(Travel, Ease);
	if (Easing <= UE_KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}
	const float Cruise = CruiseSpeed(Travel, Easing, Length);
	if (Seconds < Easing)
	{
		const float X = Seconds / Easing;
		return Cruise / Easing * 6.f * X * (1.f - X);
	}
	if (Seconds > Travel - Easing)
	{
		const float Y = (Travel - Seconds) / Easing;
		return -Cruise / Easing * 6.f * Y * (1.f - Y);
	}
	return 0.f;
}

float OreLiftMotion::TravelTimeAt(float Distance, float Travel, float Ease, float Length)
{
	if (Distance <= 0.f || Length <= 0.f)
	{
		return 0.f;
	}
	if (Distance >= Length)
	{
		return Travel;
	}
	// The distance only grows with time: halving the window finds it to well under a frame.
	float Low = 0.f;
	float High = Travel;
	for (int32 Pass = 0; Pass < 40; ++Pass)
	{
		const float Middle = 0.5f * (Low + High);
		if (TravelDistance(Middle, Travel, Ease, Length) < Distance)
		{
			Low = Middle;
		}
		else
		{
			High = Middle;
		}
	}
	return 0.5f * (Low + High);
}

float OreLiftMotion::DistanceAt(const FOreLiftRunShape& Run, float Seconds)
{
	return TravelDistance(Seconds - Run.TakeUpSeconds, Run.TravelSeconds, Run.EaseSeconds, Run.Length);
}

TArray<float> OreLiftMotion::ClankTimes(const FOreLiftRunShape& Run)
{
	TArray<float> Times;
	for (const float Place : SheavePlaces(Run))
	{
		Times.Add(Run.TakeUpSeconds + TravelTimeAt(Place, Run.TravelSeconds, Run.EaseSeconds, Run.Length));
	}
	return Times;
}

void OreLiftMotion::AdvanceSwing(const FOreLiftRunShape& Run, const FOreLiftSwing& Swing, float Seconds, FOreLiftSwingState& State)
{
	const float Traveled = FMath::Min(Seconds, Run.GetDuration()) - Run.TakeUpSeconds;
	StepTo(Run, Swing, Traveled > 0.f ? FMath::FloorToInt(Traveled / StepSeconds) : 0, State);
}

FOreLiftSway OreLiftMotion::SwayAt(const FOreLiftRunShape& Run, const FOreLiftSwing& Swing, float Seconds, FOreLiftSwingState& State)
{
	FOreLiftSway Sway;
	const float Start = Run.TakeUpSeconds;
	const float Arrival = Run.GetArrival();
	const float Time = FMath::Clamp(Seconds, 0.f, Run.GetDuration());

	// The swing, between the two whole steps either side of Time, so it's the same in any frames.
	float Along = 0.f;
	float Side = 0.f;
	if (Time > Start)
	{
		AdvanceSwing(Run, Swing, Time, State);
		FOreLiftSwingState Next = State;
		StepTo(Run, Swing, State.Step + 1, Next);
		const float Share = FMath::Clamp((Time - Start) / StepSeconds - static_cast<float>(State.Step), 0.f, 1.f);
		Along = FMath::Lerp(State.Along, Next.Along, Share);
		Side = FMath::Lerp(State.Side, Next.Side, Share);
	}
	// Coming in, the landing's guides catch it, and set down, its floor stills it.
	const float Settle = FMath::Max(Run.SettleSeconds, 0.01f);
	const float Guided = FMath::Min(GuideSeconds, Run.TravelSeconds * 0.25f);
	const float Free = 1.f - FMath::SmoothStep(Arrival - Guided, Arrival + StillShare * Settle, Time);
	const float Breeze = Swing.WindDegrees * FMath::SmoothStep(Start, Start + WindInSeconds, Time) * Free
		* FMath::Sin(UE_TWO_PI * WindHz * (Time - Start) + 0.6f);
	Sway.Along = SoftLimit(FMath::RadiansToDegrees(Along) * Free, Swing.MaxDegrees);
	Sway.Side = SoftLimit(FMath::RadiansToDegrees(Side) * Free + Breeze, Swing.MaxDegrees);

	// The lift-off: up off the landing with a little overshoot as the strain comes on, melting into the travel.
	const float Rise = Swing.TakeUpRise * (1.f - FMath::Exp(-LiftOffRate * Time) * FMath::Cos(UE_TWO_PI * LiftOffHz * Time));
	Sway.Bob = Rise * (1.f - FMath::SmoothStep(Start, Start + RiseFadeSeconds, Time));
	Sway.Jolt = Swing.ClankJolt * TakeUpJoltShare * Ring(Time, JoltHz * 0.7f, JoltDecay);

	// The clanks: a bob on the cable and a jolt through the floor, each dying away.
	for (const float Clank : ClankTimes(Run))
	{
		Sway.Bob += Swing.ClankBob * Ring(Time - Clank, BobHz, BobDecay);
		Sway.Jolt += Swing.ClankJolt * Ring(Time - Clank, JoltHz, JoltDecay);
	}

	// Set down: a dip as the cable slackens, a bounce, still by the settle's end.
	const float Landed = Time - Arrival;
	if (Landed > 0.f)
	{
		const float Fade = 1.f - FMath::SmoothStep(0.f, Settle, Landed);
		Sway.Bob -= Swing.LandDip * Ring(Landed, LandBounceHz, LandDecay) * Fade;
		Sway.Jolt += Swing.ClankJolt * LandJoltShare * Ring(Landed, JoltHz * 0.6f, JoltDecay * 0.7f) * Fade;
	}
	return Sway;
}

FOreLiftSway OreLiftMotion::SwayAt(const FOreLiftRunShape& Run, const FOreLiftSwing& Swing, float Seconds)
{
	FOreLiftSwingState Fresh;
	return SwayAt(Run, Swing, Seconds, Fresh);
}

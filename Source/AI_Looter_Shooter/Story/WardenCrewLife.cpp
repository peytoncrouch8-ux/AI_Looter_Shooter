#include "Story/WardenCrewLife.h"

namespace
{
	/** The way Forward turned Yaw to the right and Pitch up (degrees), world space. */
	FVector TurnedWay(const FVector& Forward, float Yaw, float Pitch)
	{
		const FVector Flat = FVector(Forward.X, Forward.Y, 0.0).GetSafeNormal();
		const FVector Ahead = Flat.IsNearlyZero() ? FVector::ForwardVector : Flat;
		return FRotator(Pitch, Ahead.Rotation().Yaw + Yaw, 0.f).Vector();
	}
}

FWardenLife::FWardenLife(int32 Seed)
	: Random(Seed)
{
	// Each person's first glance and watch come at their own time.
	NextGlance = Random.FRandRange(1.f, GlanceGapMax);
	WatchHold = Random.FRandRange(WatchMin, WatchMax);
}

bool FWardenLife::Notices(const FVector& Eye, const FVector& Forward, const FVector& Player, float Radius)
{
	const FVector To = Player - Eye;
	if (To.SizeSquared() > FMath::Square(static_cast<double>(Radius)))
	{
		return false;
	}
	const FVector FlatTo = FVector(To.X, To.Y, 0.0).GetSafeNormal();
	const FVector FlatForward = FVector(Forward.X, Forward.Y, 0.0).GetSafeNormal();
	if (FlatTo.IsNearlyZero() || FlatForward.IsNearlyZero())
	{
		return true;
	}
	return (FlatTo | FlatForward) >= FMath::Cos(FMath::DegreesToRadians(NoticeHalfAngle));
}

void FWardenLife::UpdateLook(float DeltaSeconds, const FWardenLookSense& Sense)
{
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	LookEye = Sense.Eye;
	if (!bLookStarted)
	{
		bLookStarted = true;
		LookWay = TurnedWay(Sense.Forward, 0.f, 0.f);
	}

	// The player near: watched a while, then a look away, then back (nobody stares).
	const bool bNear = Sense.bHasPlayer && !Sense.bFocusOnly && Notices(Sense.Eye, Sense.Forward, Sense.Player, Sense.NoticeRadius);
	if (bNear && !bPlayerNear)
	{
		NoticeLeft = NoticeDelay;
	}
	bPlayerNear = bNear;
	NoticeLeft = FMath::Max(NoticeLeft - Dt, 0.f);
	AwayLeft = FMath::Max(AwayLeft - Dt, 0.f);
	const bool bWatch = bNear && NoticeLeft <= 0.f && AwayLeft <= 0.f && !Sense.bHasListener;
	if (bWatch)
	{
		WatchedFor += Dt;
		if (WatchedFor >= WatchHold)
		{
			// Enough: look elsewhere a moment (a glance, or back to the work).
			WatchedFor = 0.f;
			WatchHold = Random.FRandRange(WatchMin, WatchMax);
			AwayLeft = Random.FRandRange(AwayMin, AwayMax);
			GlanceLeft = AwayLeft;
			GlanceYaw = Random.FRandRange(GlanceYawMin, GlanceYawMax) * (Random.FRand() < 0.5f ? -1.f : 1.f);
			GlancePitch = Random.FRandRange(-8.f, 6.f);
		}
	}
	else if (!bNear)
	{
		WatchedFor = 0.f;
	}
	bWatchingPlayer = bWatch && AwayLeft <= 0.f;

	// Glances round about when nothing holds the eyes.
	GlanceLeft = FMath::Max(GlanceLeft - Dt, 0.f);
	NextGlance -= Dt;
	if (NextGlance <= 0.f && GlanceLeft <= 0.f && !bWatchingPlayer && !Sense.bHasListener && !Sense.bFocusOnly)
	{
		NextGlance = Random.FRandRange(GlanceGapMin, GlanceGapMax);
		GlanceLeft = Random.FRandRange(GlanceHoldMin, GlanceHoldMax);
		GlanceYaw = Random.FRandRange(GlanceYawMin, GlanceYawMax) * (Random.FRand() < 0.5f ? -1.f : 1.f);
		GlancePitch = Random.FRandRange(-8.f, 10.f);
	}

	// What they look at, most wanted first.
	FVector Target = Sense.Eye + TurnedWay(Sense.Forward, 0.f, 0.f) * 300.0;
	float Weight = 0.f;
	if (Sense.bHasListener)
	{
		Target = Sense.Listener;
		Weight = 1.f;
	}
	else if (Sense.bFocusOnly && Sense.bHasFocus)
	{
		Target = Sense.Focus;
		Weight = 1.f;
	}
	else if (bWatchingPlayer)
	{
		Target = Sense.Player;
		Weight = 0.9f;
	}
	else if (GlanceLeft > 0.f)
	{
		Target = Sense.Eye + TurnedWay(Sense.Forward, GlanceYaw, GlancePitch) * 600.0;
		Weight = 0.75f;
	}
	else if (Sense.bHasFocus)
	{
		Target = Sense.Focus;
		Weight = FMath::Clamp(Sense.FocusWeight, 0.f, 1.f);
	}

	// The head comes round as a head does: quick to start, easing in, never faster than a snap.
	const FVector ToTarget = Target - Sense.Eye;
	const FVector WantWay = ToTarget.GetSafeNormal();
	if (!WantWay.IsNearlyZero())
	{
		const FQuat Between = FQuat::FindBetweenNormals(LookWay, WantWay);
		const float Angle = FMath::RadiansToDegrees(static_cast<float>(Between.GetAngle()));
		if (Angle > KINDA_SMALL_NUMBER)
		{
			const float Step = FMath::Min(Angle * (1.f - FMath::Exp(-TurnRate * Dt)), MaxTurnSpeed * Dt);
			LookWay = FQuat::Slerp(FQuat::Identity, Between, FMath::Clamp(Step / Angle, 0.f, 1.f)).RotateVector(LookWay).GetSafeNormal();
		}
		LookDistance = FMath::Max(static_cast<float>(ToTarget.Size()), 50.f);
	}
	LookWeight = FMath::FInterpTo(LookWeight, Weight, Dt, 4.f);
}

void FWardenLife::Brace(float Seconds)
{
	BraceLeft = FMath::Max(BraceLeft, FMath::Max(Seconds, 0.f));
	// A new lurch rings the sway again, to one side or the other.
	SwayClock = 0.f;
	SwaySide = Random.FRand() < 0.5f ? -1.f : 1.f;
}

void FWardenLife::UpdateBrace(float DeltaSeconds)
{
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	BraceLeft = FMath::Max(BraceLeft - Dt, 0.f);
	BraceAlpha = BraceLeft > 0.f ? FMath::Min(BraceAlpha + BraceOnRate * Dt, 1.f) : FMath::Max(BraceAlpha - BraceOffRate * Dt, 0.f);
	SwayClock += Dt;
	const float Ring = FMath::Exp(-SwayDecay * SwayClock);
	if (Ring < 0.01f)
	{
		SwayRoll = 0.f;
		SwayPitch = 0.f;
		return;
	}
	SwayRoll = SwaySide * SwayRollDegrees * Ring * FMath::Sin(2.f * UE_PI * 1.2f * SwayClock);
	SwayPitch = SwayPitchDegrees * Ring * FMath::Sin(2.f * UE_PI * 0.8f * SwayClock + 0.7f);
}

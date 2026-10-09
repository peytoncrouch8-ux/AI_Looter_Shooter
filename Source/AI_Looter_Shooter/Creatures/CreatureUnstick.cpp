#include "Creatures/CreatureUnstick.h"

FCreatureUnstick::EAction FCreatureUnstick::Update(const FFrame& Frame)
{
	if (bGliding)
	{
		return EAction::None;
	}
	if (!bAnchored)
	{
		Reset(Frame.Here);
	}
	RetryIn = FMath::Max(0.f, RetryIn - Frame.DeltaSeconds);
	SinceBlockedMove = FMath::Min(SinceBlockedMove + Frame.DeltaSeconds, 1000.f);

	if (FVector::DistSquared(Frame.Here, Anchor) > FMath::Square(ProgressRadius * Frame.SizeScale))
	{
		// Got somewhere: whatever held it has let go.
		Anchor = Frame.Here;
		StillTime = 0.f;
		HungTime = 0.f;
	}
	else
	{
		const bool bWedging = Frame.bPressing && Frame.bBlocked && SinceBlockedMove < TouchSeconds;
		StillTime = bWedging ? StillTime + Frame.DeltaSeconds : 0.f;
		HungTime = Frame.bFalling ? HungTime + Frame.DeltaSeconds : 0.f;
	}
	BlockedTime = Frame.bBlocked ? BlockedTime + Frame.DeltaSeconds : 0.f;

	if (RetryIn <= 0.f)
	{
		if (HungTime >= HungSeconds)
		{
			HungTime = 0.f;
			StillTime = 0.f;
			return EAction::NudgeHung;
		}
		if (StillTime >= WedgedSeconds)
		{
			StillTime = 0.f;
			return EAction::NudgeWedged;
		}
	}
	if (BlockedTime >= BlockedSeconds)
	{
		BlockedTime = 0.f;
		return EAction::GoRound;
	}
	return EAction::None;
}

void FCreatureUnstick::Reset(const FVector& Here)
{
	Anchor = Here;
	bAnchored = true;
	StillTime = 0.f;
	HungTime = 0.f;
	BlockedTime = 0.f;
	RetryIn = 0.f;
	bGliding = false;
	GlideTime = 0.f;
}

void FCreatureUnstick::NudgeFailed()
{
	RetryIn = RetrySeconds;
}

void FCreatureUnstick::StartGlide(const FVector& From, const FVector& To)
{
	bGliding = true;
	GlideFrom = From;
	GlideTo = To;
	GlideTime = 0.f;
	StillTime = 0.f;
	HungTime = 0.f;
	BlockedTime = 0.f;
}

FVector FCreatureUnstick::StepGlide(float DeltaSeconds, bool& bOutDone)
{
	if (!bGliding)
	{
		bOutDone = true;
		return GlideTo;
	}
	GlideTime += DeltaSeconds;
	const float Alpha = FMath::Clamp(GlideTime / GlideSeconds, 0.f, 1.f);
	bOutDone = Alpha >= 1.f;
	const FVector Where = FMath::Lerp(GlideFrom, GlideTo, static_cast<double>(FMath::SmoothStep(0.f, 1.f, Alpha)));
	if (bOutDone)
	{
		Reset(GlideTo);
	}
	return Where;
}

#include "Story/GraveSightFlash.h"

namespace
{
	/** The overlay is all the way in by this share of the flash, and starts going out at that one. */
	constexpr float OverlayInEnd = 0.15f;
	constexpr float OverlayOutStart = 0.6f;

	/** The ember flares up by this share of the flash, and starts fading at that one: it's gone by the flash's end. */
	constexpr float EmberFlareEnd = 0.1f;
	constexpr float EmberFadeStart = 0.5f;
}

void FGraveSightFlash::Start(float InSeconds)
{
	Seconds = FMath::Max(InSeconds, MinSeconds);
	Elapsed = 0.f;
	bPlaying = true;
}

bool FGraveSightFlash::Advance(float DeltaSeconds)
{
	if (!bPlaying)
	{
		return false;
	}
	Elapsed = FMath::Min(Elapsed + FMath::Max(DeltaSeconds, 0.f), Seconds);
	if (Elapsed < Seconds)
	{
		return false;
	}
	bPlaying = false;
	return true;
}

void FGraveSightFlash::Stop()
{
	bPlaying = false;
	Elapsed = Seconds;
}

float FGraveSightFlash::GetProgress() const
{
	return Seconds > 0.f ? FMath::Clamp(Elapsed / Seconds, 0.f, 1.f) : 1.f;
}

float FGraveSightFlash::OverlayAlphaAt(float Progress)
{
	// In quickly, as a flash comes; held while the ember lifts; out more slowly, so the sight lingers a moment. Both ends
	// are 0, so a flash starts and ends on the plain world.
	const float In = FMath::SmoothStep(0.f, OverlayInEnd, Progress);
	const float Out = 1.f - FMath::SmoothStep(OverlayOutStart, 1.f, Progress);
	return In * Out;
}

float FGraveSightFlash::EmberRiseAt(float Progress)
{
	// Off the lid at once, as if let go, then slowing: an ease out.
	const float Left = 1.f - FMath::Clamp(Progress, 0.f, 1.f);
	return 1.f - Left * Left;
}

float FGraveSightFlash::EmberGlowAt(float Progress)
{
	// It flares as the sight finds it and fades as it goes: nothing is left of it on the lid.
	const float Flare = FMath::SmoothStep(0.f, EmberFlareEnd, Progress);
	const float Fade = 1.f - FMath::SmoothStep(EmberFadeStart, 1.f, Progress);
	return Flare * Fade;
}

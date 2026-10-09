#include "Combat/WoundsClose.h"
#include "Combat/HealthComponent.h"
#include "Combat/RecoverySettings.h"

namespace
{
	/**
	 * Health this close to the most counts as full. Adding a heal to health in floats can land a hair under the maximum,
	 * and a run that chased that hair would never end (the HUD would shine forever).
	 */
	constexpr float FullTolerance = 0.01f;
}

void FWoundsClose::Reset()
{
	QuietSeconds = 0.f;
	RunSeconds = 0.f;
}

float FWoundsClose::Step(const FRecoverySettings& Settings, float DeltaSeconds, float Health, float MaxHealth, bool bHurt, bool bHeld)
{
	DeltaSeconds = FMath::Max(DeltaSeconds, 0.f);
	const float Delay = FMath::Max(Settings.RegenDelaySeconds, 0.f);

	if (bHurt)
	{
		// Any hurt, however small, is a fight still on: the whole wait again.
		QuietSeconds = 0.f;
		RunSeconds = 0.f;
		return 0.f;
	}
	if (bHeld)
	{
		// A scene or a death doesn't count as a rest, and doesn't spoil one already earned: the ramp starts over after it.
		RunSeconds = 0.f;
		return 0.f;
	}
	if (Health >= MaxHealth - FullTolerance)
	{
		// Nothing to close. The wait stays met, so a wound that comes from no hurt (a smaller maximum) heals after its ramp.
		RunSeconds = 0.f;
		QuietSeconds = FMath::Min(QuietSeconds + DeltaSeconds, Delay);
		return 0.f;
	}

	// The wait first; whatever is left of this step after it is the run's first moments.
	float Remaining = DeltaSeconds;
	if (QuietSeconds < Delay)
	{
		const float Waited = FMath::Min(Remaining, Delay - QuietSeconds);
		QuietSeconds += Waited;
		Remaining -= Waited;
	}
	if (QuietSeconds < Delay || Remaining <= 0.f)
	{
		return 0.f;
	}

	const float RunFrom = RunSeconds;
	RunSeconds += Remaining;
	const float Share = Settings.RegenShareBetween(RunFrom, RunSeconds);
	return FMath::Min(MaxHealth * Share, MaxHealth - Health);
}

float FWoundsClose::Tick(const FRecoverySettings& Settings, UHealthComponent& Health, float DeltaSeconds, bool bHeld)
{
	// A hurt since the last look: the count moved. The first look takes the count as it stands (a player loaded hurt waits
	// the whole delay from the start anyway).
	const uint32 HurtCount = Health.GetHurtCount();
	const bool bHurt = bSeenHurtCount && HurtCount != SeenHurtCount;
	SeenHurtCount = HurtCount;
	bSeenHurtCount = true;

	if (Health.IsDead())
	{
		Reset();
		return 0.f;
	}
	const float Give = Step(Settings, DeltaSeconds, Health.GetHealth(), Health.GetMaxHealth(), bHurt, bHeld);
	return Give > 0.f ? Health.Heal(Give) : 0.f;
}

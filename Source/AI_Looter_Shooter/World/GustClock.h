#pragma once

#include "CoreMinimal.h"
#include "World/GustEvent.h"

/**
 * When gusts come and how each blows (UGustSubsystem runs one; the tests run their own): a seeded stream, so a level
 * plays out the same from the same start. The first gust comes 20 to 60 s in, then one every 40 to 120 s (start to
 * start), each from the prevailing wind's quarter give or take DirectionVariance degrees, StrengthMin to StrengthMax
 * hard, DurationMin to DurationMax long. While one blows, its push (GetStrengthNow) rises over the first part of it,
 * holds, and dies away over its second half (Shape).
 */
struct AI_LOOTER_SHOOTER_API FGustClock
{
	static constexpr float FirstIntervalMin = 20.f;
	static constexpr float FirstIntervalMax = 60.f;
	static constexpr float IntervalMin = 40.f;
	static constexpr float IntervalMax = 120.f;
	static constexpr float DurationMin = 2.f;
	static constexpr float DurationMax = 5.f;
	static constexpr float StrengthMin = 0.4f;
	static constexpr float StrengthMax = 1.f;
	static constexpr float DirectionVariance = 25.f;

	/** Starts over with Seed, the prevailing wind blowing toward WindYaw (degrees), the first gust still to come. */
	void Reset(int32 Seed, float InWindYaw);

	/** Moves on by DeltaSeconds: true when a gust began in this step (GetCurrent is it). One step starts at most one. */
	bool Advance(float DeltaSeconds);

	/** Starts Gust now, whatever the clock says (Looter.Gust, tests); the next one still comes when it was due. False while one blows. */
	bool StartNow(const FGustEvent& Gust);

	/** The next gust as the stream would roll it (its direction, strength and length), without starting it. */
	FGustEvent Roll();

	bool IsBlowing() const { return bBlowing; }
	const FGustEvent& GetCurrent() const { return Current; }
	/** Seconds since the gust blowing now began. */
	float GetAge() const { return Age; }
	/** The push now: the gust's strength times its rise and fall (Shape); 0 between gusts. */
	float GetStrengthNow() const;
	float GetSecondsToNext() const { return Until; }
	/** Gusts begun since Reset. */
	int32 GetCount() const { return Count; }
	float GetWindYaw() const { return WindYaw; }

	/** A gust's rise and fall, 0 to 1, Age seconds into one Duration long: up over its first 30%, down over its last half. */
	static float Shape(float GustAge, float GustDuration);

private:
	FRandomStream Random{ 0x6057 };
	float WindYaw = 19.f;
	float Until = FirstIntervalMax;
	float Age = 0.f;
	bool bBlowing = false;
	int32 Count = 0;
	FGustEvent Current;
};

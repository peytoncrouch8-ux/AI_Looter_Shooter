#include "Audio/LooterSoundRules.h"

int32 LooterSoundRules::PickVariation(int32 Count, int32 Last, float Roll)
{
	if (Count <= 0)
	{
		return INDEX_NONE;
	}
	if (Count == 1)
	{
		return 0;
	}
	const float Clamped = FMath::Clamp(Roll, 0.f, 1.f);
	if (Last < 0 || Last >= Count)
	{
		return FMath::Min(FMath::FloorToInt32(Clamped * Count), Count - 1);
	}
	// One of the others: pick among Count - 1 and step over the last one.
	const int32 Other = FMath::Min(FMath::FloorToInt32(Clamped * (Count - 1)), Count - 2);
	return Other >= Last ? Other + 1 : Other;
}

float LooterSoundRules::DbToGain(float Db)
{
	return FMath::IsFinite(Db) ? FMath::Pow(10.f, Db / 20.f) : 1.f;
}

float LooterSoundRules::JitteredPitch(float PitchScale, float Jitter, float Roll)
{
	const float Spread = FMath::Clamp(Jitter, 0.f, 0.5f) * (2.f * FMath::Clamp(Roll, 0.f, 1.f) - 1.f);
	// The engine plays pitches from 0.4 to 2 by default; this keeps a scaled one well inside what it can do.
	return FMath::Clamp(PitchScale * (1.f + Spread), 0.25f, 4.f);
}

float LooterSoundRules::SliderToGain(float Share)
{
	const float Clamped = FMath::IsFinite(Share) ? FMath::Clamp(Share, 0.f, 1.f) : 1.f;
	return Clamped * Clamped;
}

FLooterClassGains LooterSoundRules::ClassGains(const FLooterVolumes& Volumes)
{
	FLooterClassGains Gains;
	Gains.Master = SliderToGain(Volumes.Master);
	Gains.Effects = SliderToGain(Volumes.Effects);
	Gains.Interface = SliderToGain(Volumes.Interface);
	Gains.Ambience = Gains.Effects;
	Gains.Music = SliderToGain(Volumes.Music);
	return Gains;
}

float LooterSoundRules::PitchForSize(float SizeScale)
{
	if (!FMath::IsFinite(SizeScale) || SizeScale <= 0.f)
	{
		return 1.f;
	}
	return FMath::Clamp(1.f / FMath::Sqrt(SizeScale), 0.6f, 1.6f);
}

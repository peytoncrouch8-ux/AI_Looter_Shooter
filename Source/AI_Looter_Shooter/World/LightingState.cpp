#include "World/LightingState.h"

FRotator FLightingState::GetSunRotation() const
{
	// A directional light shines along its forward axis: from the sun's bearing toward the opposite one, downward (as
	// build_area_environment.py turns the placed sun).
	return FRotator(-SunElevation, FRotator::ClampAxis(SunBearing + 180.f), 0.f);
}

void FLightingState::SunAnglesFromRotation(const FRotator& Rotation, float& OutBearing, float& OutElevation)
{
	// The sun stands against the light's direction. Unreal's +X is north and +Y east, so the bearing is the angle from
	// +X toward +Y.
	const FVector ToSun = -Rotation.Vector();
	OutBearing = static_cast<float>(FRotator::ClampAxis(FMath::RadiansToDegrees(FMath::Atan2(ToSun.Y, ToSun.X))));
	OutElevation = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(ToSun.Z, ToSun.Size2D())));
}

FLightingState FLightingState::Blend(const FLightingState& From, const FLightingState& To, float Alpha)
{
	const float T = FMath::Clamp(Alpha, 0.f, 1.f);
	FLightingState Mixed = To;
	// The short way round: from 350 to 10 degrees passes north, not south.
	Mixed.SunBearing = static_cast<float>(FRotator::ClampAxis(From.SunBearing + FMath::FindDeltaAngleDegrees(From.SunBearing, To.SunBearing) * T));
	Mixed.SunElevation = FMath::Lerp(From.SunElevation, To.SunElevation, T);
	Mixed.SunIntensity = FMath::Lerp(From.SunIntensity, To.SunIntensity, T);
	Mixed.SunTemperature = FMath::Lerp(From.SunTemperature, To.SunTemperature, T);
	Mixed.ShadowDistance = FMath::Lerp(From.ShadowDistance, To.ShadowDistance, T);
	Mixed.ShadowCascades = FMath::RoundToInt32(FMath::Lerp(static_cast<float>(From.ShadowCascades), static_cast<float>(To.ShadowCascades), T));
	Mixed.SkyIntensity = FMath::Lerp(From.SkyIntensity, To.SkyIntensity, T);
	Mixed.FogDensity = FMath::Lerp(From.FogDensity, To.FogDensity, T);
	Mixed.FogInscattering = FMath::Lerp(From.FogInscattering, To.FogInscattering, T);
	Mixed.FogDirectionalInscattering = FMath::Lerp(From.FogDirectionalInscattering, To.FogDirectionalInscattering, T);
	Mixed.FogDirectionalExponent = FMath::Lerp(From.FogDirectionalExponent, To.FogDirectionalExponent, T);
	Mixed.FogDirectionalStartDistance = FMath::Lerp(From.FogDirectionalStartDistance, To.FogDirectionalStartDistance, T);
	Mixed.BackdropTint = FMath::Lerp(From.BackdropTint, To.BackdropTint, T);
	Mixed.CloudTint = FMath::Lerp(From.CloudTint, To.CloudTint, T);
	Mixed.ExposureBias = FMath::Lerp(From.ExposureBias, To.ExposureBias, T);
	return Mixed;
}

namespace
{
	FString ColorText(const FLinearColor& Color)
	{
		return FString::Printf(TEXT("(%.2f, %.2f, %.2f)"), Color.R, Color.G, Color.B);
	}
}

FString FLightingState::Describe() const
{
	return FString::Printf(TEXT("sun at bearing %.1f, %.1f deg up, %.2f lux, %.0f K, shadows to %.0f m in %d cascades; sky light %.2f; ")
		TEXT("fog %.4f, haze %s, toward the sun %s exponent %.1f from %.0f m; backdrop tint %s, cloud tint %s; exposure bias %.2f"),
		SunBearing, SunElevation, SunIntensity, SunTemperature, ShadowDistance / 100.f, ShadowCascades, SkyIntensity,
		FogDensity, *ColorText(FogInscattering), *ColorText(FogDirectionalInscattering), FogDirectionalExponent,
		FogDirectionalStartDistance / 100.f, *ColorText(BackdropTint), *ColorText(CloudTint), ExposureBias);
}

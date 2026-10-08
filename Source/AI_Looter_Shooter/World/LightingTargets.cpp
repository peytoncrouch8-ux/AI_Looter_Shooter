#include "World/LightingTargets.h"
#include "World/LightingState.h"
#include "World/LightingStates.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/Level.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"

namespace
{
	/** The first actor of a kind in the level that Accept takes, or null. */
	template <typename ActorType, typename PredicateType>
	ActorType* FirstActorOf(const ULevel& Level, PredicateType Accept)
	{
		for (AActor* Actor : Level.Actors)
		{
			ActorType* Found = Cast<ActorType>(Actor);
			if (Found && IsValid(Found) && Accept(*Found))
			{
				return Found;
			}
		}
		return nullptr;
	}

	/** The sky atmosphere's absorption as the engine sets it: Earth's ozone layer, which a state's Ozone multiplies. */
	float EarthOzone()
	{
		return GetDefault<USkyAtmosphereComponent>()->OtherAbsorptionScale;
	}

	/**
	 * The share of the sky atmosphere's light the height fog takes: the inverse of the sky's color correction, which is
	 * meant for the sky alone. Otherwise a correction that turns a dusk sky blue-violet turns the haze lavender too.
	 */
	FLinearColor FogSkyShare(const FLinearColor& SkyLuminance)
	{
		return FLinearColor(1.f / FMath::Max(SkyLuminance.R, 0.01f), 1.f / FMath::Max(SkyLuminance.G, 0.01f),
			1.f / FMath::Max(SkyLuminance.B, 0.01f));
	}

	/** A reference the states actor holds, if it still points at a living actor. */
	template <typename ActorType>
	ActorType* Living(const TObjectPtr<ActorType>& Reference)
	{
		ActorType* Actor = Reference.Get();
		return IsValid(Actor) ? Actor : nullptr;
	}
}

FLightingTargets FLightingTargets::Find(const ULevel* Level, const ALightingStates* States)
{
	FLightingTargets Targets;
	if (States)
	{
		Targets.Sun = Living(States->Sun);
		Targets.SkyLight = Living(States->SkyLight);
		Targets.Atmosphere = Living(States->Atmosphere);
		Targets.HeightFog = Living(States->HeightFog);
		Targets.PostVolume = Living(States->PostVolume);
	}
	const ULevel* Where = Level ? Level : (States ? States->GetLevel() : nullptr);
	if (!Where)
	{
		return Targets;
	}
	if (!Targets.Sun)
	{
		// The sun is the light the sky atmosphere sets by; a level with other directional lights keeps them as they are.
		Targets.Sun = FirstActorOf<ADirectionalLight>(*Where, [](const ADirectionalLight& Light)
		{
			// GetLightComponent, not ADirectionalLight::GetComponent: that one is editor-only data, so a packaged game
			// couldn't compile it (found by the first test build).
			const UDirectionalLightComponent* Component = Cast<UDirectionalLightComponent>(Light.GetLightComponent());
			return Component && Component->bAtmosphereSunLight;
		});
	}
	if (!Targets.Sun)
	{
		Targets.Sun = FirstActorOf<ADirectionalLight>(*Where, [](const ADirectionalLight&) { return true; });
	}
	if (!Targets.SkyLight)
	{
		Targets.SkyLight = FirstActorOf<ASkyLight>(*Where, [](const ASkyLight&) { return true; });
	}
	if (!Targets.Atmosphere)
	{
		Targets.Atmosphere = FirstActorOf<ASkyAtmosphere>(*Where, [](const ASkyAtmosphere&) { return true; });
	}
	if (!Targets.HeightFog)
	{
		Targets.HeightFog = FirstActorOf<AExponentialHeightFog>(*Where, [](const AExponentialHeightFog&) { return true; });
	}
	if (!Targets.PostVolume)
	{
		// The level-wide one: a bounded volume grades only its own room.
		Targets.PostVolume = FirstActorOf<APostProcessVolume>(*Where, [](const APostProcessVolume& Volume) { return Volume.bUnbound != 0; });
	}
	return Targets;
}

FString FLightingTargets::DescribeMissing() const
{
	TArray<FString> Missing;
	if (!Sun)
	{
		Missing.Add(TEXT("sun"));
	}
	if (!SkyLight)
	{
		Missing.Add(TEXT("sky light"));
	}
	if (!Atmosphere)
	{
		Missing.Add(TEXT("sky atmosphere"));
	}
	if (!HeightFog)
	{
		Missing.Add(TEXT("height fog"));
	}
	if (!PostVolume)
	{
		Missing.Add(TEXT("unbound post process volume"));
	}
	return FString::Join(Missing, TEXT(", "));
}

void FLightingTargets::Write(const FLightingState& State) const
{
	if (Sun)
	{
		// The directional light component is the actor's root, so turning the actor turns the light.
		Sun->SetActorRotation(State.GetSunRotation());
		if (UDirectionalLightComponent* Light = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
		{
			Light->SetIntensity(State.SunIntensity);
			Light->SetUseTemperature(true);
			Light->SetTemperature(State.SunTemperature);
			Light->SetDynamicShadowDistanceMovableLight(State.ShadowDistance);
			Light->SetDynamicShadowCascades(State.ShadowCascades);
		}
	}
	if (USkyLightComponent* Sky = SkyLight ? SkyLight->GetLightComponent() : nullptr)
	{
		Sky->SetIntensity(State.SkyIntensity);
	}
	if (USkyAtmosphereComponent* Air = Atmosphere ? Atmosphere->GetComponent() : nullptr)
	{
		Air->SetSkyLuminanceFactor(State.SkyLuminance);
		Air->SetOtherAbsorptionScale(EarthOzone() * State.Ozone);
	}
	if (UExponentialHeightFogComponent* Fog = HeightFog ? HeightFog->GetComponent() : nullptr)
	{
		Fog->SetFogDensity(State.FogDensity);
		Fog->SetFogInscatteringColor(State.FogInscattering);
		Fog->SetDirectionalInscatteringColor(State.FogDirectionalInscattering);
		Fog->SetDirectionalInscatteringExponent(State.FogDirectionalExponent);
		Fog->SetDirectionalInscatteringStartDistance(State.FogDirectionalStartDistance);
		Fog->SetSkyAtmosphereAmbientContributionColorScale(FogSkyShare(State.SkyLuminance));
	}
	if (PostVolume)
	{
		// Volumes' settings are gathered every frame, so writing them is all a change takes.
		PostVolume->Settings.bOverride_AutoExposureBias = true;
		PostVolume->Settings.AutoExposureBias = State.ExposureBias;
	}
}

void FLightingTargets::Read(FLightingState& Out) const
{
	if (const UDirectionalLightComponent* Light = Sun ? Cast<UDirectionalLightComponent>(Sun->GetLightComponent()) : nullptr)
	{
		FLightingState::SunAnglesFromRotation(Light->GetRelativeRotation(), Out.SunBearing, Out.SunElevation);
		Out.SunIntensity = Light->Intensity;
		Out.SunTemperature = Light->Temperature;
		Out.ShadowDistance = Light->DynamicShadowDistanceMovableLight;
		Out.ShadowCascades = Light->DynamicShadowCascades;
	}
	if (const USkyLightComponent* Sky = SkyLight ? SkyLight->GetLightComponent() : nullptr)
	{
		Out.SkyIntensity = Sky->Intensity;
	}
	if (const USkyAtmosphereComponent* Air = Atmosphere ? Atmosphere->GetComponent() : nullptr)
	{
		Out.SkyLuminance = Air->SkyLuminanceFactor;
		Out.Ozone = EarthOzone() > 0.f ? Air->OtherAbsorptionScale / EarthOzone() : 1.f;
	}
	if (const UExponentialHeightFogComponent* Fog = HeightFog ? HeightFog->GetComponent() : nullptr)
	{
		Out.FogDensity = Fog->FogDensity;
		Out.FogInscattering = Fog->FogInscatteringLuminance;
		Out.FogDirectionalInscattering = Fog->DirectionalInscatteringLuminance;
		Out.FogDirectionalExponent = Fog->DirectionalInscatteringExponent;
		Out.FogDirectionalStartDistance = Fog->DirectionalInscatteringStartDistance;
	}
	if (PostVolume)
	{
		Out.ExposureBias = PostVolume->Settings.AutoExposureBias;
	}
}

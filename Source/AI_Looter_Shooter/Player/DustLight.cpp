#include "Player/DustLight.h"
#include "World/LightingStates.h"
#include "World/LightingStateSubsystem.h"
#include "CollisionQueryParams.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"

namespace
{
	/** As FSlideDust's: dust shows about twice its own color in full sun, about 0.7 of it in shade under the sky. */
	constexpr float DustSunLevel = 2.f;
	constexpr float DustShadeLevel = 0.7f;
	const FLinearColor DustSkyColor(0.86f, 0.93f, 1.05f);
	constexpr float DustSunCheckDistance = 6000.f;

	/** A light's color with its brightest channel at 1: the hue. */
	FLinearColor DustHue(const FLinearColor& Color)
	{
		const float Brightest = FMath::Max3(Color.R, Color.G, Color.B);
		return Brightest > 0.01f ? Color / Brightest : FLinearColor::White;
	}
}

void FDustLight::FindLight(const UWorld& World)
{
	FoundAt = World.GetTimeSeconds();
	if (!Sun.IsValid())
	{
		// The sun is the light the sky atmosphere sets by; any directional light will do in a level without one.
		const UDirectionalLightComponent* Fallback = nullptr;
		for (TActorIterator<ADirectionalLight> It(&World); It; ++It)
		{
			const UDirectionalLightComponent* Light = Cast<UDirectionalLightComponent>(It->GetLightComponent());
			if (!Light || !Light->IsVisible())
			{
				continue;
			}
			if (Light->bAtmosphereSunLight)
			{
				Sun = Light;
				break;
			}
			Fallback = Fallback ? Fallback : Light;
		}
		if (!Sun.IsValid())
		{
			Sun = Fallback;
		}
	}
	StateTint = FLinearColor::White;
	const UMaterialParameterCollection* Collection = LoadObject<UMaterialParameterCollection>(nullptr, ALightingStates::DefaultCollectionPath);
	const UMaterialParameterCollectionInstance* Instance = Collection ? World.GetParameterCollectionInstance(Collection) : nullptr;
	FLinearColor Tint;
	if (Instance && Instance->GetVectorParameterValue(ULightingStateSubsystem::BackdropTintParameter, Tint))
	{
		StateTint = Tint;
	}
}

FLinearColor FDustLight::At(const UWorld& World, const FVector& Where, const AActor& Ignore)
{
	if (World.GetTimeSeconds() - FoundAt >= RefindSeconds || World.GetTimeSeconds() < FoundAt)
	{
		FindLight(World);
	}
	const UDirectionalLightComponent* Light = Sun.Get();
	const FVector ToSun = Light ? -Light->GetDirection() : FVector::UpVector;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FootfallDustSun), false, &Ignore);
	const bool bLit = Light && Light->Intensity > 0.f && ToSun.Z > 0.0
		&& !World.LineTraceTestByChannel(Where, Where + ToSun * DustSunCheckDistance, ECC_Visibility, Params);
	// A sun low over the horizon lights it less.
	const float Height = FMath::Clamp(static_cast<float>(ToSun.Z) * 4.f, 0.f, 1.f);
	const FLinearColor SunColor = Light ? DustHue(Light->GetLightColor()) : FLinearColor::White;
	return (DustSkyColor * DustShadeLevel + SunColor * ((DustSunLevel - DustShadeLevel) * (bLit ? 1.f : 0.f) * Height)) * StateTint;
}

#include "Creatures/CreatureReactSubsystem.h"
#include "World/LightingStates.h"
#include "World/LightingStateSubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"

namespace
{
	/** The light is looked at again this often (s): a lighting state changes over seconds, never in a frame. */
	constexpr double CreatureDustLightSeconds = 2.0;

	/**
	 * How bright dust shows against its own color, as FSlideDust has it: about twice it in full sun, 0.7 in shade under the
	 * sky. A creature's feet are as often in shade as not, so the sun counts at this share (no trace toward it).
	 */
	constexpr float CreatureDustSunLevel = 2.f;
	constexpr float CreatureDustShadeLevel = 0.7f;
	constexpr float CreatureDustSunShare = 0.7f;
	const FLinearColor CreatureDustSkyColor(0.86f, 0.93f, 1.05f);
}

UCreatureReactSubsystem* UCreatureReactSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UCreatureReactSubsystem>() : nullptr;
}

bool UCreatureReactSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

int32 UCreatureReactSubsystem::TakeFootfalls(int32 Wanted)
{
	const UWorld* World = GetWorld();
	return Footfalls.Take(World ? World->GetTimeSeconds() : 0.0, Wanted);
}

int32 UCreatureReactSubsystem::TakeAsh(int32 Wanted)
{
	const UWorld* World = GetWorld();
	return Ash.Take(World ? World->GetTimeSeconds() : 0.0, Wanted);
}

void UCreatureReactSubsystem::RefreshViewers()
{
	if (ViewersFrame == GFrameCounter)
	{
		return;
	}
	ViewersFrame = GFrameCounter;
	Viewers.Reset();
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* Player = It->Get();
		if (!Player || !Player->IsLocalController())
		{
			continue;
		}
		if (const APlayerCameraManager* Camera = Player->PlayerCameraManager.Get())
		{
			Viewers.Add(Camera->GetCameraLocation());
		}
		if (const APawn* Pawn = Player->GetPawn())
		{
			Viewers.Add(Pawn->GetActorLocation());
		}
	}
}

bool UCreatureReactSubsystem::IsNearAViewer(const FVector& Point, float Within)
{
	RefreshViewers();
	const double WithinSquared = FMath::Square(static_cast<double>(Within));
	for (const FVector& Viewer : Viewers)
	{
		if (FVector::DistSquared(Viewer, Point) <= WithinSquared)
		{
			return true;
		}
	}
	return false;
}

FLinearColor UCreatureReactSubsystem::GetDustLight()
{
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Now - LightLookedAt >= CreatureDustLightSeconds || Now < LightLookedAt)
	{
		LightLookedAt = Now;
		RefreshLight();
	}
	return DustLight;
}

void UCreatureReactSubsystem::RefreshLight()
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (!Sun.IsValid())
	{
		// The sun is the light the sky atmosphere sets by; any directional light will do in a level without one.
		const UDirectionalLightComponent* Fallback = nullptr;
		for (TActorIterator<ADirectionalLight> It(World); It; ++It)
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
	// What the lighting state does to what can't see the light itself: white by day, darker and warmer at dusk.
	FLinearColor StateTint = FLinearColor::White;
	const UMaterialParameterCollection* Collection = LoadObject<UMaterialParameterCollection>(nullptr, ALightingStates::DefaultCollectionPath);
	const UMaterialParameterCollectionInstance* Instance = Collection ? World->GetParameterCollectionInstance(Collection) : nullptr;
	FLinearColor Tint;
	if (Instance && Instance->GetVectorParameterValue(ULightingStateSubsystem::BackdropTintParameter, Tint))
	{
		StateTint = Tint;
	}
	const UDirectionalLightComponent* Light = Sun.Get();
	const FVector ToSun = Light ? -Light->GetDirection() : FVector::UpVector;
	// The sun's hue (its brightest channel at 1), less of it as it sinks to the horizon.
	FLinearColor SunHue = Light ? Light->GetLightColor() : FLinearColor::White;
	const float Brightest = FMath::Max3(SunHue.R, SunHue.G, SunHue.B);
	SunHue = Brightest > 0.01f ? SunHue / Brightest : FLinearColor::White;
	const float Height = Light && Light->Intensity > 0.f ? FMath::Clamp(static_cast<float>(ToSun.Z) * 4.f, 0.f, 1.f) : 0.f;
	DustLight = (CreatureDustSkyColor * CreatureDustShadeLevel
		+ SunHue * ((CreatureDustSunLevel - CreatureDustShadeLevel) * CreatureDustSunShare * Height)) * StateTint;
	DustLight.A = 1.f;
}

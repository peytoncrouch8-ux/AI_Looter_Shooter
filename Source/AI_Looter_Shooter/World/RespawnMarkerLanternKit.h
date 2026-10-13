#pragma once

// What ARespawnMarker's constructor (RespawnMarker.cpp) and its lantern (RespawnMarkerLantern.cpp) share: the models it looks
// for, the lantern's light and flame figures. Not for anyone else.

#include "CoreMinimal.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

namespace RespawnLantern
{
	/** The lantern post (Art/Models/Props/LanternPost.py: its glass slot is LanternGlow, its Light socket at the flame). */
	inline const TCHAR* const PostPath = TEXT("/Game/Art/Props/SM_LanternPost.SM_LanternPost");
	/** What its glass shows while the lantern is dark: the house trim's window glass, as the burial deck's lanterns. */
	inline const TCHAR* const DarkGlassPath = TEXT("/Game/Art/Materials/MI_HouseTrim.MI_HouseTrim");
	/** The gunfire's additive glow on the engine's plane (R, G, B, strength, shape: 0 a streak, 1 round). */
	inline const TCHAR* const GlowPath = TEXT("/Game/Weapons/FX/M_FX_Glow.M_FX_Glow");
	inline const TCHAR* const PlanePath = TEXT("/Engine/BasicShapes/Plane.Plane");
	inline constexpr int32 GlowFloats = 5;
	inline constexpr float RoundShape = 1.f;

	/** The lantern's Light socket on the post; without the model, where the post's lantern hangs (cm, in the model's space). */
	inline FName LightSocket()
	{
		return FName(TEXT("Light"));
	}
	inline FVector FallbackFlame()
	{
		return FVector(52.0, 0.0, 182.0);
	}

	/**
	 * The light: warm and small, a lantern's, its reach (cm); it is gone past LightDrawDistance (fading over LightFadeRange),
	 * so a grave far across the valley costs nothing. Its rest brightness is the marker's LanternCandela.
	 */
	inline constexpr float LightRadius = 700.f;
	inline constexpr float LightDrawDistance = 5500.f;
	inline constexpr float LightFadeRange = 1000.f;
	inline FLinearColor Warm()
	{
		return FLinearColor(1.f, 0.72f, 0.42f);
	}

	/** The flame's glow cards' colour (the glow material's R, G, B). */
	inline FLinearColor FlameColor()
	{
		return FLinearColor(1.f, 0.66f, 0.3f);
	}

	/** A first mark's light spikes to this many times the rest light, a grave marked again to the second. */
	inline constexpr float FirstPeakFactor = 6.f;
	inline constexpr float AgainPeakFactor = 2.5f;

	/** How much the flame's cards swell at a flare's peak (a share of their size). */
	inline constexpr float FirstSwell = 0.9f;
	inline constexpr float AgainSwell = 0.5f;

	/** An asset found only once it's in this checkout, so the class works, and its tests run, without it. In a constructor. */
	template <typename T>
	T* FindIfMade(const TCHAR* ObjectPath)
	{
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath))))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<T> Finder(ObjectPath);
		return Finder.Object;
	}
}

#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;
class UMeshComponent;
class UDynamicMeshComponent;
class UStaticMeshComponent;

/**
 * How one material slot of a procedural mesh is painted by M_StylizedSurface (or M_StylizedGlow).
 * All the art direction lives in these few numbers, so every prop shares one material and one look:
 * soft painterly color variation, grass/moss that settles on upward faces, darker bases, wind sway and glow.
 */
struct FStylizedSurface
{
	/** Base albedo (linear). */
	FLinearColor Color = FLinearColor(0.5f, 0.5f, 0.5f);

	/** Color that covers faces pointing up (grass on ground, moss on rocks). */
	FLinearColor TopColor = FLinearColor(0.2f, 0.35f, 0.1f);

	/** 0 = off, 1 = full top cover. */
	float TopBlend = 0.f;

	/** World-normal Z above which the top color takes over (0.7 ~ 45 degrees). */
	float TopThreshold = 0.72f;

	/** Height (cm, from the actor origin) of the dark-to-light base gradient. 0 = off. */
	float GradHeight = 0.f;

	/** How dark the base of the gradient is (0-1). */
	float GradDark = 0.f;

	/** Horizontal strata banding strength for cliffs (0-1). */
	float Strata = 0.f;

	/** Wind sway amplitude in cm at GradHeight. Needs GradHeight. */
	float Wind = 0.f;

	/** Emissive multiplier of Color. */
	float Glow = 0.f;

	/** Painterly hue/value jitter (0-1). */
	float Variation = 0.12f;

	/** Bends shading normals toward straight up (0-1) so grass and flowers light like the ground they grow on. */
	float UpNormal = 0.f;

	/** Use the additive, unlit glow material (light beams). */
	bool bAdditive = false;

	/** Render both faces (single-sheet foliage such as grass ribbons). */
	bool bTwoSided = false;

	static FStylizedSurface Solid(const FLinearColor& InColor, float InVariation = 0.12f)
	{
		FStylizedSurface Surface;
		Surface.Color = InColor;
		Surface.Variation = InVariation;
		return Surface;
	}
};

namespace StylizedColors
{
	/** Authoring colors are picked in sRGB hex; materials want linear. */
	inline FLinearColor Hex(uint32 RGB)
	{
		return FLinearColor::FromSRGBColor(FColor((RGB >> 16) & 0xFF, (RGB >> 8) & 0xFF, RGB & 0xFF));
	}
}

namespace StylizedSurfaces
{
	/** One material instance per surface, in slot order. Share the result across components that use the same look. */
	AI_LOOTER_SHOOTER_API TArray<UMaterialInterface*> CreateMaterials(UObject* Outer, const TArray<FStylizedSurface>& Surfaces);

	/** Creates one material instance per surface and assigns them to the component's slots in order. */
	AI_LOOTER_SHOOTER_API void Apply(UDynamicMeshComponent* Component, const TArray<FStylizedSurface>& Surfaces);

	/**
	 * Turns a static mesh component into a soft vertical light beam standing on its parent's origin
	 * (sky beacons, loot pillars). No collision, no shadows; fades out toward the top.
	 */
	AI_LOOTER_SHOOTER_API void SetupBeam(UStaticMeshComponent* Beam, const FLinearColor& Color, float Glow, float Height, float Radius);
}

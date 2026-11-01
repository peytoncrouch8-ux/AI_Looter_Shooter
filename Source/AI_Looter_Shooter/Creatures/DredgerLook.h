#pragma once

#include "CoreMinimal.h"

/**
 * Every asset path and colour a Dredger wears, in one place (the contract's rule: one soft path per stand-in, so the user's
 * Tripo models are a one-line swap; TRIPO_MODELS.md 5 rifleman, 6 hose-man, 8 company rifle, 9 soul-hose nozzle). Until
 * they land: the mannequin darkened to black rubber, a kit of engine shapes on its bones, the project's AR from its parts in
 * the hand, a barrel prop as the tank. Colours are linear (M_StylizedSurface's Color).
 */
namespace DredgerLook
{
	/** The bodies: the user's Tripo Dredgers on the mannequin's skeleton (lanes/IMPORT.md; the stand-in was
	 *  /Game/Characters/Mannequins/Meshes/SKM_Manny_Simple for both). */
	inline constexpr const TCHAR* BodyMeshPath = TEXT("/Game/Art/Characters/SK_DredgerRifleman.SK_DredgerRifleman");
	inline constexpr const TCHAR* HoseManBodyMeshPath = TEXT("/Game/Art/Characters/SK_DredgerHoseMan.SK_DredgerHoseMan");
	/** The company rifle: a rolled AR (its parts' models) at a fixed seed, the same issue gun on every rifleman. */
	inline constexpr const TCHAR* RifleDefinitionPath = TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle");
	inline constexpr int32 RifleSeed = 1771;
	/** The hose-man's tank: a barrel prop, scaled down and painted brass and iron. */
	inline constexpr const TCHAR* TankMeshPath = TEXT("/Game/Art/Props/SM_Barrel_A.SM_Barrel_A");
	/** The kit's shapes and their master. */
	inline constexpr const TCHAR* SpherePath = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	inline constexpr const TCHAR* CylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	inline constexpr const TCHAR* CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	inline constexpr const TCHAR* SurfacePath = TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface");

	/** The rubber coat (the whole body), the mask's face, its iron snout and filter, the cap. */
	inline const FLinearColor Rubber(0.016f, 0.018f, 0.022f);
	inline const FLinearColor MaskRubber(0.028f, 0.03f, 0.027f);
	inline const FLinearColor Iron(0.07f, 0.07f, 0.075f);
	inline const FLinearColor CapCloth(0.03f, 0.032f, 0.045f);
	/** The lenses' glass: a sickly green-cyan, glowing faintly, flaring before a shot or a throw. */
	inline const FLinearColor Lens(0.12f, 0.5f, 0.42f);
	inline constexpr float LensGlow = 1.2f;
	inline constexpr float LensFlareGlow = 14.f;
	/** The tank's brass and iron, its gauge's soul-light, the nozzle's brass, the hose's rubber. */
	inline const FLinearColor Brass(0.42f, 0.27f, 0.08f);
	inline const FLinearColor SoulCyan(0.25f, 0.95f, 1.f);
	inline constexpr float GaugeGlow = 4.f;
	inline const FLinearColor HoseRubber(0.02f, 0.045f, 0.05f);
	/** A rifle round's tracer and glint (hot gold), the soul-smog leaking from a dead one's mask (black-violet). */
	inline const FLinearColor Tracer(1.f, 0.62f, 0.2f);
	inline const FLinearColor Smog(0.05f, 0.03f, 0.07f);
}

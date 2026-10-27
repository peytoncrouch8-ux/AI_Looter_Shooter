#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponFX.h"

/**
 * The world hits' colours (FWeaponFX's WeaponFXImpacts.cpp, by surface, and WeaponFXImpactsKinds.cpp, by gun kind: the
 * punch, the chunk bursts, a blast's sheet), kept here once for both. Private to the effects' files.
 */
namespace ImpactLooks
{
	// Dust, spray and smoke as the unlit smoke material shows them: about as the afternoon sun shows each (times ImpactShade:
	// darker in shade).
	inline const FLinearColor ImpactDirtDust(0.5f, 0.41f, 0.31f);
	inline const FLinearColor ImpactGrassDust(0.5f, 0.48f, 0.37f);
	inline const FLinearColor ImpactSandDust(0.74f, 0.65f, 0.5f);
	inline const FLinearColor ImpactStoneDust(0.6f, 0.57f, 0.52f);
	inline const FLinearColor ImpactSawdust(0.66f, 0.55f, 0.41f);
	inline const FLinearColor ImpactMetalSmoke(0.42f, 0.42f, 0.44f);
	inline const FLinearColor ImpactWaterMist(0.82f, 0.86f, 0.9f);
	inline const FLinearColor ImpactGlassDust(0.82f, 0.84f, 0.86f);
	inline const FLinearColor ImpactLintDust(0.62f, 0.59f, 0.54f);
	inline const FLinearColor ImpactHayDust(0.72f, 0.64f, 0.44f);
	inline const FLinearColor ImpactBoneDust(0.78f, 0.75f, 0.68f);

	// The lit chunks' base colours (linear).
	inline const FLinearColor ImpactClodColor(0.11f, 0.075f, 0.045f);
	inline const FLinearColor ImpactTurfColor(0.07f, 0.11f, 0.03f);
	inline const FLinearColor ImpactSplinterColor(0.5f, 0.34f, 0.18f);
	inline const FLinearColor ImpactStrawColor(0.62f, 0.5f, 0.2f);
	inline const FLinearColor ImpactShardColor(0.55f, 0.7f, 0.78f);
	inline const FLinearColor ImpactDropColor(0.5f, 0.62f, 0.7f);
	inline const FLinearColor ImpactFlakeColor(0.28f, 0.22f, 0.17f);
	inline const FLinearColor ImpactBoneColor(0.7f, 0.65f, 0.55f);
	inline const FLinearColor ImpactLintColor(0.5f, 0.47f, 0.42f);

	// The glows: only a hue (each is multiplied by its strength).
	inline const FLinearColor ImpactSparkColor(1.f, 0.5f, 0.16f);
	inline const FLinearColor ImpactHotSparkColor(1.f, 0.7f, 0.35f);
	inline const FLinearColor ImpactFlashColor(1.f, 0.72f, 0.4f);
	inline const FLinearColor ImpactGlintColor(0.85f, 0.95f, 1.f);
	inline const FLinearColor ImpactCritColor(1.f, 0.85f, 0.3f);
	/** A geyser's thrown soil and stone grit: darker than the hanging dust, so the column reads against the ground it rose from. */
	inline const FLinearColor ImpactGeyserDirt(0.17f, 0.105f, 0.055f);
	inline const FLinearColor ImpactGeyserStone(0.22f, 0.2f, 0.18f);
	/** A stone hit's dust jet: pale, against the dark of the rock (VFX-2). */
	inline const FLinearColor ImpactStoneJet(0.7f, 0.67f, 0.62f);
	/** Soil crumbs (lit, round) thrown in a fan with the clods; pale stone flakes with the dark chips (VFX-2). */
	inline const FLinearColor ImpactCrumbColor(0.085f, 0.06f, 0.038f);
	inline const FLinearColor ImpactFlakeStoneColor(0.55f, 0.52f, 0.47f);

	/** The dust a hit on Material leaves hanging (as full sun shows it). */
	inline FLinearColor DustOf(EImpactMaterial Material)
	{
		switch (Material)
		{
		case EImpactMaterial::Dirt: return ImpactDirtDust;
		case EImpactMaterial::Grass: return ImpactGrassDust;
		case EImpactMaterial::Sand: return ImpactSandDust;
		case EImpactMaterial::Wood: return ImpactSawdust;
		case EImpactMaterial::Metal:
		case EImpactMaterial::SheetMetal: return ImpactMetalSmoke;
		case EImpactMaterial::Water: return ImpactWaterMist;
		case EImpactMaterial::Glass: return ImpactGlassDust;
		case EImpactMaterial::Cloth: return ImpactLintDust;
		case EImpactMaterial::Hay: return ImpactHayDust;
		case EImpactMaterial::Bone: return ImpactBoneDust;
		case EImpactMaterial::Gravel:
		case EImpactMaterial::Stone:
		default: return ImpactStoneDust;
		}
	}

	/** What a column of it thrown up is (darker than its dust on soil, the pale jet off stone). */
	inline FLinearColor ThrownOf(EImpactMaterial Material)
	{
		switch (Material)
		{
		case EImpactMaterial::Dirt:
		case EImpactMaterial::Grass: return ImpactGeyserDirt;
		case EImpactMaterial::Gravel:
		case EImpactMaterial::Stone: return ImpactStoneJet;
		default: return DustOf(Material);
		}
	}
}

/** A world hit's place and sizes as SpawnSurfaceImpact worked them out, handed to its gun kind's layers. */
struct FWeaponFX::FImpactSpot
{
	/** Where it struck, and just off the surface (where its pieces start). */
	FVector Location = FVector::ZeroVector;
	FVector Point = FVector::ZeroVector;
	/** The surface's facing, the shot's way and its bounce off the surface, two ways across the surface. */
	FVector Up = FVector::UpVector;
	FVector Shot = FVector::ForwardVector;
	FVector Bounce = FVector::UpVector;
	FVector SideA = FVector::ForwardVector;
	FVector SideB = FVector::RightVector;
	EImpactMaterial Material = EImpactMaterial::Unknown;
	/** The hit's size (its mark's and its weight's), its throw (sqrt of it), and its pieces' drawn size (with the view's). */
	float Size = 1.f;
	float Reach = 1.f;
	float Seen = 1.f;
	/** The share of its pieces thrown here (far off and in a stream, less). */
	float Detail = 1.f;
	/** The light the dust shows in there. */
	FLinearColor Shade = FLinearColor::White;
	/** The floor its pieces land on, when it has one. */
	bool bFloor = false;
	FPlane Floor = FPlane(0.0, 0.0, 1.0, 0.0);
};

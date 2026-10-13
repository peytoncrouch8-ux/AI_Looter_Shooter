#include "Weapons/ImpactSurfaceMap.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInterface.h"

// GroundCues: x where green ends (warmth, red over green), y where bare soil starts, z how far the borders wander, w the
// dry grass's bare spots. GroundCues2: x the grass's share of green ground, y how dark the ground under trees is (linear
// luma), z its leaf litter, w the height blend. As build_world_materials.py's GROUND_DEFAULTS has them.
const FLinearColor UImpactSurfaceMapData::DefaultCues(-0.05f, 0.13f, 0.14f, 0.1f);
const FLinearColor UImpactSurfaceMapData::DefaultCues2(0.4f, 0.15f, 0.9f, 0.5f);

namespace
{
	/** Past this much of the material's rock (RockAmt, smoothstep(0.55, 0.9, alpha)) a cell is rock. */
	constexpr float GroundRockShare = 0.5f;
	/** Scree: the macro alpha's band between the roads and the rock (about 0.65). */
	constexpr float GroundScreeSelect = 0.5f;
	/** Past this much of the stony layer (bare soil, roads, the ground under trees) a cell is dirt. */
	constexpr float GroundStonyShare = 0.5f;
	/** The scatter's pebble band. */
	constexpr float GroundPebbleDensity = 0.5f;
	/** How far up the parents a material's map is looked for (an area's _Steep instance, its window, the master). */
	constexpr int32 MapParentSteps = 4;
}

bool UImpactSurfaceMapData::KindAt(const FVector2D& WorldXY, EImpactMaterial& OutMaterial) const
{
	const FVector2D Span = WorldMax - WorldMin;
	if (Size <= 0 || Span.X <= 0.0 || Span.Y <= 0.0 || Kinds.Num() != Size * Size)
	{
		return false;
	}
	const double Row = (WorldXY.X - WorldMin.X) / Span.X * Size;
	const double Column = (WorldXY.Y - WorldMin.Y) / Span.Y * Size;
	if (Row < 0.0 || Column < 0.0 || Row >= Size || Column >= Size)
	{
		return false;
	}
	const uint8 Kind = Kinds[FMath::FloorToInt32(Row) * Size + FMath::FloorToInt32(Column)];
	if (Kind == static_cast<uint8>(EImpactMaterial::Unknown) || Kind >= static_cast<uint8>(EImpactMaterial::Count))
	{
		return false;
	}
	OutMaterial = static_cast<EImpactMaterial>(Kind);
	return true;
}

const UImpactSurfaceMapData* UImpactSurfaceMapData::FindOn(const UMaterialInterface* Material)
{
	for (int32 Step = 0; Material && Step < MapParentSteps; ++Step)
	{
		// The engine's lookup isn't const, though it only reads the material's list.
		if (const UImpactSurfaceMapData* Map = Cast<UImpactSurfaceMapData>(
			const_cast<UMaterialInterface*>(Material)->GetAssetUserDataOfClass(UImpactSurfaceMapData::StaticClass())))
		{
			return Map;
		}
		const UMaterialInstance* Instance = Cast<UMaterialInstance>(Material);
		Material = Instance ? Instance->Parent.Get() : nullptr;
	}
	return nullptr;
}

EImpactMaterial UImpactSurfaceMapData::ClassifyGround(const FImpactGroundSample& Sample, const FLinearColor& Cues, const FLinearColor& Cues2)
{
	// The macro alpha first: the material lays its rock and scree by it, whatever the color.
	if (FMath::SmoothStep(0.55f, 0.9f, Sample.Select) >= GroundRockShare)
	{
		return EImpactMaterial::Stone;
	}
	if (Sample.Select >= GroundScreeSelect || Sample.Pebbles >= GroundPebbleDensity)
	{
		return EImpactMaterial::Gravel;
	}
	// The ground between, by GROUND_WEIGHTS with its noise at its middle: the stony layer (cracked crust, grit and leaf
	// litter: dirt to a bullet) takes the roads and footpaths, warm bare soil and the dark, dull ground under trees; the
	// grass and the dry thatch share the rest.
	const FLinearColor& C = Sample.Macro;
	const float Warm = (C.R - C.G) / FMath::Max(C.R + C.G, 0.002f);
	const float Luma = 0.299f * C.R + 0.587f * C.G + 0.114f * C.B;
	const float High = FMath::Max3(C.R, C.G, C.B);
	const float Saturation = (High - FMath::Min3(C.R, C.G, C.B)) / FMath::Max(High, 0.002f);
	const float Road = FMath::SmoothStep(0.1f, 0.2f, Sample.Select);
	const float Soil = FMath::SmoothStep(Cues.G - 0.04f, Cues.G + 0.04f, Warm);
	const float Shade = (1.f - FMath::SmoothStep(Cues2.G * 0.65f, Cues2.G, Luma)) * (1.f - FMath::SmoothStep(0.66f, 0.76f, Saturation));
	const float Stony = FMath::Clamp(FMath::Max3(Soil, Road, Shade * 0.85f), 0.f, 1.f);
	// Green ground, and the dry thatch between green and soil: grass either way. (Not the scatter's grass density: it's
	// none past an area's old boundary and where it's out of date, though the ground shows grass there.)
	return Stony >= GroundStonyShare ? EImpactMaterial::Dirt : EImpactMaterial::Grass;
}

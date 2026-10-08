#include "Audio/SoundSurface.h"
#include "Audio/LooterSoundCues.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/HitResult.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

namespace
{
	struct FSurfaceWord
	{
		const TCHAR* Word;
		ESoundSurface Surface;
	};

	/**
	 * The words that name a surface, the first match winning: so "RockCliff" is stone before anything else, and "Steep"
	 * (a terrain's slope material) is dirt before "Macro" makes it terrain. Names from /Game/Art/Materials.
	 */
	const FSurfaceWord SurfaceWords[] = {
		{ TEXT("Water"), ESoundSurface::Water },
		{ TEXT("Metal"), ESoundSurface::Metal },
		{ TEXT("Iron"), ESoundSurface::Metal },
		{ TEXT("Steel"), ESoundSurface::Metal },
		{ TEXT("Brass"), ESoundSurface::Metal },
		{ TEXT("Copper"), ESoundSurface::Metal },
		{ TEXT("Rock"), ESoundSurface::Stone },
		{ TEXT("Stone"), ESoundSurface::Stone },
		{ TEXT("Granite"), ESoundSurface::Stone },
		{ TEXT("Cliff"), ESoundSurface::Stone },
		{ TEXT("Brick"), ESoundSurface::Stone },
		{ TEXT("Cobble"), ESoundSurface::Stone },
		{ TEXT("Concrete"), ESoundSurface::Stone },
		{ TEXT("Gravel"), ESoundSurface::Stone },
		{ TEXT("Ballast"), ESoundSurface::Stone },
		{ TEXT("Wood"), ESoundSurface::Wood },
		{ TEXT("Plank"), ESoundSurface::Wood },
		{ TEXT("Board"), ESoundSurface::Wood },
		{ TEXT("Deck"), ESoundSurface::Wood },
		{ TEXT("Porch"), ESoundSurface::Wood },
		{ TEXT("HouseTrim"), ESoundSurface::Wood },
		// The town's painted surfaces are painted boards.
		{ TEXT("Paint"), ESoundSurface::Wood },
		{ TEXT("Grass"), ESoundSurface::Grass },
		{ TEXT("Meadow"), ESoundSurface::Grass },
		{ TEXT("Moss"), ESoundSurface::Grass },
		{ TEXT("Lawn"), ESoundSurface::Grass },
		{ TEXT("Hay"), ESoundSurface::Grass },
		{ TEXT("Dirt"), ESoundSurface::Dirt },
		{ TEXT("Mud"), ESoundSurface::Dirt },
		{ TEXT("Soil"), ESoundSurface::Dirt },
		{ TEXT("Sand"), ESoundSurface::Dirt },
		{ TEXT("Path"), ESoundSurface::Dirt },
		{ TEXT("Road"), ESoundSurface::Dirt },
		{ TEXT("DenFloor"), ESoundSurface::Dirt },
		{ TEXT("Steep"), ESoundSurface::Dirt },
	};

	/** Words of a terrain's own material: its grass and soil come from a macro map, so the slope decides. */
	const TCHAR* TerrainWords[] = { TEXT("Macro"), TEXT("Terrain") };

	const FName GroundTag(TEXT("Ground"));
	const TCHAR* SurfaceTagPrefix = TEXT("Surface.");

	bool IsTerrainName(const FString& Name)
	{
		for (const TCHAR* Word : TerrainWords)
		{
			if (Name.Contains(Word, ESearchCase::IgnoreCase))
			{
				return true;
			}
		}
		return false;
	}

	/** The surface a material (or a parent of it, a couple of steps up) is named for. */
	bool FromMaterial(const UMaterialInterface* Material, ESoundSurface& OutSurface, bool& bOutTerrain)
	{
		for (int32 Step = 0; Material && Step < 3; ++Step)
		{
			const FString Name = Material->GetName();
			if (SoundSurface::FromName(Name, OutSurface))
			{
				return true;
			}
			bOutTerrain = bOutTerrain || IsTerrainName(Name);
			const UMaterialInstance* Instance = Cast<UMaterialInstance>(Material);
			Material = Instance ? Instance->Parent.Get() : nullptr;
		}
		return false;
	}
}

bool SoundSurface::FromName(const FString& Name, ESoundSurface& OutSurface)
{
	for (const FSurfaceWord& Entry : SurfaceWords)
	{
		if (Name.Contains(Entry.Word, ESearchCase::IgnoreCase))
		{
			OutSurface = Entry.Surface;
			return true;
		}
	}
	return false;
}

bool SoundSurface::FromTag(FName Tag, ESoundSurface& OutSurface)
{
	const FString Text = Tag.ToString();
	return Text.StartsWith(SurfaceTagPrefix, ESearchCase::IgnoreCase) && FromName(Text.RightChop(FCString::Strlen(SurfaceTagPrefix)), OutSurface);
}

ESoundSurface SoundSurface::Of(const FHitResult& Hit)
{
	const UPrimitiveComponent* Component = Hit.GetComponent();
	const AActor* Actor = Hit.GetActor();
	ESoundSurface Surface = ESoundSurface::Dirt;

	// A tag says it outright.
	if (Component)
	{
		for (const FName& Tag : Component->ComponentTags)
		{
			if (FromTag(Tag, Surface))
			{
				return Surface;
			}
		}
	}
	if (Actor)
	{
		for (const FName& Tag : Actor->Tags)
		{
			if (FromTag(Tag, Surface))
			{
				return Surface;
			}
		}
	}

	// Then the names: a physical material's, the material at the face that was hit (else its first), and their parents'.
	if (const UPhysicalMaterial* Physical = Hit.PhysMaterial.Get())
	{
		if (FromName(Physical->GetName(), Surface))
		{
			return Surface;
		}
	}
	bool bTerrain = Actor && Actor->ActorHasTag(GroundTag);
	if (Component)
	{
		int32 Section = 0;
		const UMaterialInterface* Material = Hit.FaceIndex >= 0 ? Component->GetMaterialFromCollisionFaceIndex(Hit.FaceIndex, Section) : nullptr;
		if (!Material)
		{
			Material = Component->GetMaterial(0);
		}
		if (FromMaterial(Material, Surface, bTerrain))
		{
			return Surface;
		}
	}

	// Terrain: grass on the flat, the soil showing on a slope.
	if (bTerrain)
	{
		return Hit.ImpactNormal.Z >= GrassFloorZ ? ESoundSurface::Grass : ESoundSurface::Dirt;
	}
	return ESoundSurface::Dirt;
}

FName SoundSurface::FootstepCue(ESoundSurface Surface)
{
	switch (Surface)
	{
	case ESoundSurface::Grass:
		return LooterSoundCue::FootstepGrass;
	case ESoundSurface::Wood:
		return LooterSoundCue::FootstepWood;
	case ESoundSurface::Stone:
	case ESoundSurface::Metal:
		return LooterSoundCue::FootstepStone;
	case ESoundSurface::Dirt:
	case ESoundSurface::Water:
	default:
		return LooterSoundCue::FootstepDirt;
	}
}

#pragma once

#include "CoreMinimal.h"
#include "Misc/PackageName.h"

struct FStylizedSurface;
class UMaterialInstanceConstant;
class UMaterialInterface;
class UPackage;

/** Material instance assets painted with the stylized materials, for the prop baker and the model importer. */
namespace SurfaceMaterials
{
	/** The material a surface is painted with: the additive glow, two-sided foliage or the standard surface. */
	UMaterialInterface* ParentFor(const FStylizedSurface& Surface);

	/** Whether the material is (or is an instance of) one of the stylized materials. */
	bool IsStylized(const UMaterialInterface* Material);

	/**
	 * Makes sure the stylized materials allow what our meshes need: Nanite (not for the additive glow, which Nanite can't
	 * draw), instancing and skinning (rigged models). Without a flag, every instance sets it on itself and compiles shaders of its own, and a
	 * packaged game draws the default material. Returns the materials it changed, for saving.
	 */
	TArray<UPackage*> PrepareParents();

	/** Creates the material instance asset Folder/Name as a child of Parent. */
	UMaterialInstanceConstant* Create(const FString& Folder, const FString& Name, UMaterialInterface* Parent);

	/** Writes every paint setting of Surface into the instance (and lets it share its parent's shaders). */
	void Apply(UMaterialInstanceConstant* Instance, const FStylizedSurface& Surface);

	/** Drops usage flags an instance set on itself, so it shares its parent's shaders again. Returns whether it changed. */
	bool ClearUsageOverrides(UMaterialInstanceConstant* Instance);

	/** The asset saved at PackagePath (for example /Game/Art/Props/SM_Lantern), or null if there isn't one. */
	template <typename T>
	T* LoadExisting(const FString& PackagePath)
	{
		if (!FPackageName::DoesPackageExist(PackagePath))
		{
			return nullptr;
		}
		return LoadObject<T>(nullptr, *(PackagePath + TEXT(".") + FPackageName::GetShortName(PackagePath)));
	}
}

#pragma once

#include "CoreMinimal.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/WeaponTypes.h"

class UDynamicMesh;
class UDynamicMeshComponent;
struct FStylizedSurface;
struct FWeaponInstanceData;

namespace WeaponModels
{
	/** Key points of a built model, in mesh space. */
	struct FPoints
	{
		FVector Muzzle = FVector::ZeroVector;
		/** Where the right hand holds it (top of the pistol grip). */
		FVector Grip = FVector::ZeroVector;
		/** Where the left hand holds it (under the handguard / on the pump). */
		FVector Foregrip = FVector(30.f, 0.f, -3.f);
		/** The part a reload works on (built into the part mesh) and the direction it slides out / back along. */
		EWeaponReloadPart ReloadPart = EWeaponReloadPart::None;
		FVector ReloadPartAxis = FVector::ZeroVector;
	};

	/**
	 * Builds a chunky, toy-like weapon along +X (origin at the back of the receiver, +Z up).
	 * The seed picks paint, sight and barrel so every rolled gun looks a little different; rarity lights
	 * the accent strips. The part a reload moves (magazine or pump) goes into PartMesh, in the same space, so it can be
	 * animated on its own. Returns the surfaces for each material slot and the model's key points.
	 */
	void Build(EWeaponModel Model, int32 Seed, EWeaponRarity Rarity, const FLinearColor& RarityColor, UDynamicMesh* Mesh,
		UDynamicMesh* PartMesh, TArray<FStylizedSurface>& OutSurfaces, FPoints& OutPoints);

	/**
	 * Builds a rolled weapon's model (its definition's procedural model, with its seed and rarity) straight into two mesh
	 * components, the gun and its moving part, materials included. False if the definition has no procedural model.
	 */
	bool BuildInto(const FWeaponInstanceData& Instance, UDynamicMeshComponent* Mesh, UDynamicMeshComponent* PartMesh, FPoints& OutPoints);
}

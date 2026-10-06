#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LooterMaterialGraphTools.generated.h"

class UMaterial;

/** Material graph helpers for the material build scripts (Tools/Unreal/build_world_materials.py), called from Python. */
UCLASS()
class ULooterMaterialGraphTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Empties a material's graph so a script can build it again in place, keeping its instances linked. The engine's
	 * DeleteAllMaterialExpressions marks each node as garbage, and that asserts on a node loaded while the editor started:
	 * a class's defaults reached the material (the Unpaid's model reaches M_Ghost), and everything loaded then is rooted
	 * for good. Those nodes are moved out of the material into the transient package instead, where they sit unused
	 * until the editor closes. Returns how many nodes were removed; OutMovedOut says how many of them were moved out.
	 */
	UFUNCTION(BlueprintCallable, Category = "Looter|Materials")
	static int32 ClearMaterialGraph(UMaterial* Material, int32& OutMovedOut);
};

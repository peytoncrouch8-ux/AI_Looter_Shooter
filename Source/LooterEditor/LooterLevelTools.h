#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LooterLevelTools.generated.h"

/** Level helpers for the area build scripts (Tools/Unreal/build_area.py), called from Python. */
UCLASS()
class ULooterLevelTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Waits until every asset still compiling is done. A map's static meshes finish building after it loads, and until
	 * then their collision isn't there: a script that loads a level and traces for the ground at once got nothing (the
	 * spider nest stood at 0, 20 m under the bluff it was meant for). Returns how many assets it waited on.
	 */
	UFUNCTION(BlueprintCallable, Category = "Looter|Levels")
	static int32 FinishAssetCompilation();
};

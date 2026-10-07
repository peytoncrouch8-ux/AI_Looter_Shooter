#include "LooterLevelTools.h"
#include "AssetCompilingManager.h"

int32 ULooterLevelTools::FinishAssetCompilation()
{
	FAssetCompilingManager& Compiling = FAssetCompilingManager::Get();
	const int32 Remaining = Compiling.GetNumRemainingAssets();
	if (Remaining > 0)
	{
		Compiling.FinishAllCompilation();
	}
	return Remaining;
}

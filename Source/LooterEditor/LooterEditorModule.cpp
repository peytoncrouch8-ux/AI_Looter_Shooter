#include "PropBaker.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLooterEditor, Log, All);

namespace
{
	void BakeLevelProps(const TArray<FString>& Args, UWorld* World)
	{
		if (!World || World->WorldType != EWorldType::Editor)
		{
			UE_LOG(LogLooterEditor, Warning, TEXT("Looter.BakeLevelProps works on the level open in the editor: stop the play session first."));
			return;
		}
		FPropBaker Baker(World);
		const int32 Placed = Baker.ConvertLevel();
		const bool bSaved = Baker.SaveAll();
		UE_LOG(LogLooterEditor, Display, TEXT("Looter.BakeLevelProps: placed %d actors; %s."), Placed, bSaved ? TEXT("saved") : TEXT("SAVING FAILED"));
	}

	FAutoConsoleCommandWithWorldAndArgs BakeLevelPropsCommand(
		TEXT("Looter.BakeLevelProps"),
		TEXT("Bakes the open level's procedural props into static mesh assets, puts placed actors in their place, and saves."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BakeLevelProps));
}

IMPLEMENT_MODULE(FDefaultModuleImpl, LooterEditor);

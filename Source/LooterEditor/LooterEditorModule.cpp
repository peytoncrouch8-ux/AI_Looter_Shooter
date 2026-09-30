#include "ModelImporter.h"
#include "PropBaker.h"
#include "PropSettler.h"
#include "SurfaceMaterials.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/Paths.h"
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

	void SettleProps(const TArray<FString>& Args, UWorld* World)
	{
		if (!World || World->WorldType != EWorldType::Editor)
		{
			UE_LOG(LogLooterEditor, Warning, TEXT("Looter.SettleProps works on the level open in the editor: stop the play session first."));
			return;
		}
		const bool bSelectedOnly = Args.Contains(TEXT("selected"));
		const int32 Moved = PropSettler::SettleWorld(World, bSelectedOnly);
		UE_LOG(LogLooterEditor, Display, TEXT("Looter.SettleProps: seated %d %sprops on the ground. Save the level to keep them there."), Moved,
			bSelectedOnly ? TEXT("selected ") : TEXT(""));
	}

	void BakeGroundCover(const TArray<FString>& Args, UWorld* World)
	{
		FPropBaker Baker(World);
		const int32 Baked = Baker.BakeGroundCover();
		const bool bSaved = Baker.SaveAll(/*bIncludeLevel*/ false);
		UE_LOG(LogLooterEditor, Display, TEXT("Looter.BakeGroundCover: baked %d ground cover meshes; %s. Generate the Meadow volume again to scatter them."),
			Baked, bSaved ? TEXT("saved") : TEXT("SAVING FAILED"));
	}

	void ImportModels(const TArray<FString>& Args)
	{
		// Tools/models.ps1 exports here.
		const FString Folder = Args.Num() > 0 ? Args[0] : FPaths::ConvertRelativePathToFull(FPaths::ProjectIntermediateDir() / TEXT("ArtExport"));
		FModelImporter Importer;
		const int32 Imported = Importer.ImportFolder(Folder);
		const bool bSaved = Importer.SaveAll();
		UE_LOG(LogLooterEditor, Display, TEXT("Looter.ImportModels: imported %d models from %s; %s."), Imported, *Folder,
			bSaved ? TEXT("saved") : TEXT("SAVING FAILED"));
	}

	void FixStylizedMaterials(const TArray<FString>& Args)
	{
		TArray<UPackage*> Changed = SurfaceMaterials::PrepareParents();
		const int32 Parents = Changed.Num();

		TArray<FAssetData> Assets;
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get().GetAssetsByClass(
			UMaterialInstanceConstant::StaticClass()->GetClassPathName(), Assets);
		for (const FAssetData& Asset : Assets)
		{
			if (!Asset.PackageName.ToString().StartsWith(TEXT("/Game/")))
			{
				continue;
			}
			UMaterialInstanceConstant* Instance = Cast<UMaterialInstanceConstant>(Asset.GetAsset());
			if (SurfaceMaterials::IsStylized(Instance) && SurfaceMaterials::ClearUsageOverrides(Instance))
			{
				Changed.AddUnique(Instance->GetPackage());
			}
		}
		const bool bSaved = Changed.IsEmpty() || UEditorLoadingAndSavingUtils::SavePackages(Changed, false);
		UE_LOG(LogLooterEditor, Display, TEXT("Looter.FixStylizedMaterials: %d materials got usage flags, %d instances share their parent's shaders again; %s."),
			Parents, Changed.Num() - Parents, bSaved ? TEXT("saved") : TEXT("SAVING FAILED"));
	}

	FAutoConsoleCommandWithWorldAndArgs BakeLevelPropsCommand(
		TEXT("Looter.BakeLevelProps"),
		TEXT("Bakes the open level's procedural props into static mesh assets, puts placed actors in their place, and saves."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BakeLevelProps));

	FAutoConsoleCommandWithWorldAndArgs SettlePropsCommand(
		TEXT("Looter.SettleProps"),
		TEXT("Seats every prop standing on the open level's terrain on the ground (no gaps under their edges; low, wide ones lean with the slope). Argument 'selected': only the selected actors. Undoable; save the level after."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SettleProps));

	FAutoConsoleCommandWithWorldAndArgs BakeGroundCoverCommand(
		TEXT("Looter.BakeGroundCover"),
		TEXT("Bakes the meadow's grass and flower meshes again from the prop generator, keeping their materials and settings, and saves them."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BakeGroundCover));

	FAutoConsoleCommand ImportModelsCommand(
		TEXT("Looter.ImportModels"),
		TEXT("Imports the Blender models exported by Tools/models.ps1 into /Game/Art with the project's fixed settings, and saves. Optional argument: the export folder."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&ImportModels));

	FAutoConsoleCommand FixStylizedMaterialsCommand(
		TEXT("Looter.FixStylizedMaterials"),
		TEXT("Gives the stylized materials the usage flags our meshes need, and makes every instance of them share its parent's shaders again (instances that set a flag on themselves compile their own). Saves what changed."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&FixStylizedMaterials));
}

IMPLEMENT_MODULE(FDefaultModuleImpl, LooterEditor);

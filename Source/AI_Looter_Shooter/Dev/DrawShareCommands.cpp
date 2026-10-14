// Developer console commands that find what a view's frame pays for (not in shipping builds): where the draw calls come
// from (Ransom's Rest is render-thread bound on Medium: the frame follows the number of draws), and what the small
// scatter costs Lumen on High and Epic.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* DrawShareWorld(UWorld* World)
	{
		if (World && World->IsGameWorld())
		{
			return World;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && Context.World()->IsGameWorld())
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	/** What one group puts on screen: its parts, the instances they hold, their mesh sections (a draw each per pass). */
	struct FDrawShare
	{
		int32 Components = 0;
		int32 Instances = 0;
		int32 Sections = 0;
		/** Sections that also draw into the sun's shadow (once per cascade). */
		int32 ShadowSections = 0;
	};

	/**
	 * The actor's group: the first two levels of its outliner folder (the build scripts file everything by kind, e.g.
	 * "Town/Dressing"), or its class when it has none. The game run from the editor's binary keeps the folders.
	 */
	FString DrawGroupOf(const AActor& Actor)
	{
#if WITH_EDITOR
		const FString Folder = Actor.GetFolderPath().ToString();
		if (!Folder.IsEmpty())
		{
			TArray<FString> Parts;
			Folder.ParseIntoArray(Parts, TEXT("/"));
			return Parts.Num() > 1 ? Parts[0] + TEXT("/") + Parts[1] : Parts[0];
		}
#endif
		return Actor.GetClass()->GetName();
	}

	/** Mesh sections a part draws: a static mesh's at its first LOD (one draw each, instanced or not), else one. */
	int32 SectionsOf(const UPrimitiveComponent& Primitive)
	{
		if (const UStaticMeshComponent* MeshPart = Cast<UStaticMeshComponent>(&Primitive))
		{
			const UStaticMesh* Mesh = MeshPart->GetStaticMesh();
			return Mesh ? FMath::Max(1, Mesh->GetNumSections(0)) : 0;
		}
		return 1;
	}

	void LogTop(const TCHAR* Title, TMap<FString, FDrawShare>& Shares, int32 Top)
	{
		Shares.ValueSort([](const FDrawShare& A, const FDrawShare& B) { return A.Sections > B.Sections; });
		UE_LOG(LogLooter, Display, TEXT("Looter.Perf.DrawShares: %s (sections / parts / instances / shadow sections):"), Title);
		int32 Shown = 0;
		for (const TPair<FString, FDrawShare>& Each : Shares)
		{
			if (Shown++ >= Top)
			{
				break;
			}
			UE_LOG(LogLooter, Display, TEXT("Looter.Perf.DrawShares:   %-40s %5d %5d %6d %5d"), *Each.Key, Each.Value.Sections,
				Each.Value.Components, Each.Value.Instances, Each.Value.ShadowSections);
		}
	}

	/**
	 * Looter.Perf.DrawShares [label] [top]: what was drawn on screen in the last moment, by group and by mesh: each part's
	 * mesh sections (draws in the depth and base passes), the instances an instanced part holds (they share its draws),
	 * and how many also draw into the sun's shadow. Logs the top groups and meshes and appends every row to
	 * Saved/Tour/DrawShares.csv under the label (a tour view's "after": ["Looter.Perf.DrawShares <view>"]). Only parts
	 * seen on screen count: shadow casters off screen cost draws too and aren't listed.
	 */
	void DrawSharesCommand(const TArray<FString>& Args, UWorld* World)
	{
		World = DrawShareWorld(World);
		if (!World)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Perf.DrawShares [label] [top]: no world."));
			return;
		}
		const FString Label = Args.Num() > 0 ? Args[0] : TEXT("view");
		const int32 Top = Args.Num() > 1 ? FMath::Max(1, FCString::Atoi(*Args[1])) : 15;
		// The last frames' worth: a part drawn in any of them counts.
		constexpr float RecentSeconds = 0.25f;

		TMap<FString, FDrawShare> ByGroup;
		TMap<FString, FDrawShare> ByMesh;
		FDrawShare Total;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			const AActor* Actor = *It;
			const FString Group = DrawGroupOf(*Actor);
			TInlineComponentArray<UPrimitiveComponent*> Primitives(Actor);
			for (const UPrimitiveComponent* Primitive : Primitives)
			{
				if (!Primitive || !Primitive->IsRegistered() || !Primitive->WasRecentlyRendered(RecentSeconds))
				{
					continue;
				}
				FDrawShare Part;
				Part.Components = 1;
				Part.Sections = SectionsOf(*Primitive);
				const UInstancedStaticMeshComponent* Instanced = Cast<UInstancedStaticMeshComponent>(Primitive);
				Part.Instances = Instanced ? Instanced->GetInstanceCount() : 1;
				Part.ShadowSections = Primitive->CastShadow && Primitive->bCastDynamicShadow ? Part.Sections : 0;

				const UStaticMeshComponent* MeshPart = Cast<UStaticMeshComponent>(Primitive);
				const FString Mesh = MeshPart && MeshPart->GetStaticMesh() ? MeshPart->GetStaticMesh()->GetName()
					: Primitive->GetClass()->GetName();
				for (FDrawShare* Into : { &ByGroup.FindOrAdd(Group), &ByMesh.FindOrAdd(Mesh), &Total })
				{
					Into->Components += Part.Components;
					Into->Instances += Part.Instances;
					Into->Sections += Part.Sections;
					Into->ShadowSections += Part.ShadowSections;
				}
			}
		}

		// The file first: logging sorts the maps.
		const FString Csv = FPaths::ProjectSavedDir() / TEXT("Tour/DrawShares.csv");
		FString Rows;
		if (!FPaths::FileExists(Csv))
		{
			Rows += TEXT("When,View,Kind,Name,Sections,Parts,Instances,ShadowSections\n");
		}
		const FString When = FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M"));
		for (const TPair<FString, FDrawShare>& Each : ByGroup)
		{
			Rows += FString::Printf(TEXT("%s,%s,Group,%s,%d,%d,%d,%d\n"), *When, *Label, *Each.Key, Each.Value.Sections,
				Each.Value.Components, Each.Value.Instances, Each.Value.ShadowSections);
		}
		for (const TPair<FString, FDrawShare>& Each : ByMesh)
		{
			Rows += FString::Printf(TEXT("%s,%s,Mesh,%s,%d,%d,%d,%d\n"), *When, *Label, *Each.Key, Each.Value.Sections,
				Each.Value.Components, Each.Value.Instances, Each.Value.ShadowSections);
		}
		FFileHelper::SaveStringToFile(Rows, *Csv, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);

		UE_LOG(LogLooter, Display, TEXT("Looter.Perf.DrawShares %s: %d sections from %d parts on screen (%d instances), %d casting shadow."),
			*Label, Total.Sections, Total.Components, Total.Instances, Total.ShadowSections);
		LogTop(TEXT("by group"), ByGroup, Top);
		LogTop(TEXT("by mesh"), ByMesh, Top);
	}

	FAutoConsoleCommandWithWorldAndArgs DrawSharesCommandRegistration(
		TEXT("Looter.Perf.DrawShares"),
		TEXT("Lists what the current view drew, by outliner group and by mesh (sections = draws per pass), and appends it to Saved/Tour/DrawShares.csv: Looter.Perf.DrawShares [label] [top]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&DrawSharesCommand));

	/**
	 * Looter.Perf.HideFolder <folder> [1|0]: hides (1, the default) or shows again (0) every actor whose outliner folder
	 * is the folder or under it, to measure a group's cost by difference as Looter.Perf.HideTag does for tags.
	 */
	void HideFolderCommand(const TArray<FString>& Args, UWorld* World)
	{
		World = DrawShareWorld(World);
		if (!World || Args.Num() == 0)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Perf.HideFolder <folder> [1|0]: %s"), World ? TEXT("which folder?") : TEXT("no world."));
			return;
		}
#if WITH_EDITOR
		const FString Folder = Args[0];
		const bool bHide = Args.Num() < 2 || FCString::Atoi(*Args[1]) != 0;
		int32 Actors = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			const FString Path = It->GetFolderPath().ToString();
			if (Path == Folder || Path.StartsWith(Folder + TEXT("/")))
			{
				It->SetActorHiddenInGame(bHide);
				++Actors;
			}
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Perf.HideFolder: %s %d actors under %s."), bHide ? TEXT("hid") : TEXT("showed"), Actors, *Folder);
#else
		UE_LOG(LogLooter, Warning, TEXT("Looter.Perf.HideFolder: outliner folders exist only when the game runs from the editor's binary."));
#endif
	}

	/**
	 * Looter.Perf.LumenSkip <radius cm> [1|0]: takes every instanced mesh smaller than the radius out of Lumen and the
	 * distance fields (1, the default) or puts it back (0), to measure what the small scatter costs Lumen's surface cache
	 * and the global distance field on High and Epic. Logs how many parts and instances it touched.
	 */
	void LumenSkipCommand(const TArray<FString>& Args, UWorld* World)
	{
		World = DrawShareWorld(World);
		if (!World || Args.Num() == 0)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Perf.LumenSkip <radius cm> [1|0]: %s"), World ? TEXT("what radius?") : TEXT("no world."));
			return;
		}
		const float Radius = FCString::Atof(*Args[0]);
		const bool bSkip = Args.Num() < 2 || FCString::Atoi(*Args[1]) != 0;
		int32 Parts = 0;
		int32 Instances = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			TInlineComponentArray<UInstancedStaticMeshComponent*> Instanced(*It);
			for (UInstancedStaticMeshComponent* Each : Instanced)
			{
				const UStaticMesh* Mesh = Each ? Each->GetStaticMesh() : nullptr;
				if (!Mesh || Mesh->GetBounds().SphereRadius >= Radius)
				{
					continue;
				}
				Each->bAffectDistanceFieldLighting = !bSkip;
				Each->bAffectDynamicIndirectLighting = !bSkip;
				Each->MarkRenderStateDirty();
				++Parts;
				Instances += Each->GetInstanceCount();
			}
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Perf.LumenSkip: %s %d instanced parts (%d instances) under %.0f cm."),
			bSkip ? TEXT("took out") : TEXT("put back"), Parts, Instances, Radius);
	}

	FAutoConsoleCommandWithWorldAndArgs LumenSkipCommandRegistration(
		TEXT("Looter.Perf.LumenSkip"),
		TEXT("Takes instanced meshes smaller than a radius out of Lumen and the distance fields (1) or puts them back (0): Looter.Perf.LumenSkip <radius cm> [1|0]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&LumenSkipCommand));

	FAutoConsoleCommandWithWorldAndArgs HideFolderCommandRegistration(
		TEXT("Looter.Perf.HideFolder"),
		TEXT("Hides (1, the default) or shows again (0) every actor in an outliner folder and under it, to measure its cost by difference: Looter.Perf.HideFolder <folder> [1|0]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HideFolderCommand));
}

#endif

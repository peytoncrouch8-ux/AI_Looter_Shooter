// Developer console commands that split a view's GPU cost by mesh family, for the PERF lane's A/B tours (not in shipping
// builds): Medium is GPU-bound per pixel (Docs/Performance.md), so a family's cost is measured by switching one thing on
// it and timing the view again in the same run (a tour view's exec, its after putting it back).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "UObject/WeakObjectPtrTemplates.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* SplitWorld(UWorld* World)
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

	/** What a Looter.Perf.Scatter switch changed on one instanced part, so "reset" puts it back. */
	struct FScatterSaved
	{
		int32 StartCull = 0;
		int32 EndCull = 0;
		bool bCastShadow = true;
		int32 ForcedLod = 0;
		int32 WorldPositionOffsetDisableDistance = 0;
		/** Instances "thin" took out, in world space (their per-instance custom data is not kept: a test only). */
		TArray<FTransform> Removed;
	};

	TMap<TWeakObjectPtr<UInstancedStaticMeshComponent>, FScatterSaved>& ScatterSaved()
	{
		static TMap<TWeakObjectPtr<UInstancedStaticMeshComponent>, FScatterSaved> Saved;
		return Saved;
	}

	/** Whether the part's mesh name holds one of the '|'-separated pieces of Families ("GrassClump|TallGrass|Clover"). */
	bool IsFamily(const UInstancedStaticMeshComponent& Part, const TArray<FString>& Families)
	{
		const UStaticMesh* Mesh = Part.GetStaticMesh();
		if (!Mesh)
		{
			return false;
		}
		const FString Name = Mesh->GetName();
		return Families.ContainsByPredicate([&Name](const FString& Each) { return Name.Contains(Each); });
	}

	/** Puts back what the switches changed on one part. */
	void Restore(UInstancedStaticMeshComponent& Part, FScatterSaved& Saved)
	{
		Part.SetCullDistances(Saved.StartCull, Saved.EndCull);
		Part.SetCastShadow(Saved.bCastShadow);
		Part.SetForcedLodModel(Saved.ForcedLod);
		Part.SetWorldPositionOffsetDisableDistance(Saved.WorldPositionOffsetDisableDistance);
		if (Saved.Removed.Num() > 0)
		{
			Part.AddInstances(Saved.Removed, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true);
			Saved.Removed.Reset();
		}
	}

	/**
	 * Looter.Perf.Scatter <mesh names, '|'-separated> <thin|cull|shadow|lod|wpo|reset> [value]: switches one thing on every
	 * instanced part whose mesh name holds one of the names, to time what it costs: thin <share kept 0-1> takes out an even
	 * share of the instances; cull <scale> scales their cull distances; shadow 0|1; lod <n> is the engine's ForcedLodModel (0: it
	 * chooses, 2: LOD1 everywhere); wpo <cm> stops the wind's vertex motion past that distance (0: never); reset puts back everything any switch
	 * changed on those parts. "reset" with the names "*" puts back every part.
	 */
	void ScatterCommand(const TArray<FString>& Args, UWorld* World)
	{
		World = SplitWorld(World);
		if (!World || Args.Num() < 2)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Perf.Scatter <names|...> <thin|cull|shadow|lod|wpo|reset> [value]: %s"),
				World ? TEXT("what to switch?") : TEXT("no world."));
			return;
		}
		TArray<FString> Families;
		Args[0].ParseIntoArray(Families, TEXT("|"));
		const FString Switch = Args[1].ToLower();
		const float Value = Args.Num() > 2 ? FCString::Atof(*Args[2]) : 0.f;
		const bool bAll = Args[0] == TEXT("*");

		int32 Parts = 0;
		int32 Instances = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			TInlineComponentArray<UInstancedStaticMeshComponent*> Instanced(*It);
			for (UInstancedStaticMeshComponent* Part : Instanced)
			{
				if (!Part || !(bAll || IsFamily(*Part, Families)))
				{
					continue;
				}
				FScatterSaved* Saved = ScatterSaved().Find(Part);
				if (Switch == TEXT("reset"))
				{
					if (Saved)
					{
						Restore(*Part, *Saved);
						ScatterSaved().Remove(Part);
						++Parts;
					}
					continue;
				}
				if (!Saved)
				{
					Saved = &ScatterSaved().Add(Part);
					Saved->StartCull = Part->InstanceStartCullDistance;
					Saved->EndCull = Part->InstanceEndCullDistance;
					Saved->bCastShadow = Part->CastShadow;
					Saved->ForcedLod = Part->ForcedLodModel;
					Saved->WorldPositionOffsetDisableDistance = Part->GetWorldPositionOffsetDisableDistance();
				}
				++Parts;
				if (Switch == TEXT("thin"))
				{
					// Every instance past the kept share of each run of ten goes, back to front so the indices hold.
					const float Kept = FMath::Clamp(Value, 0.f, 1.f);
					TArray<int32> Remove;
					for (int32 Index = Part->GetInstanceCount() - 1; Index >= 0; --Index)
					{
						if (static_cast<float>(Index % 10) >= Kept * 10.f)
						{
							FTransform Transform;
							Part->GetInstanceTransform(Index, Transform, /*bWorldSpace*/ true);
							Saved->Removed.Add(Transform);
							Remove.Add(Index);
						}
					}
					Part->RemoveInstances(Remove, /*bInstanceArrayAlreadySortedInReverseOrder*/ true);
					Instances += Remove.Num();
				}
				else if (Switch == TEXT("cull"))
				{
					// A part never culled (0) gets the scale of 100 m, so "cull" can cut the never-culled far trees too.
					const float Base = Saved->EndCull > 0 ? Saved->EndCull : 10000.f;
					Part->SetCullDistances(FMath::RoundToInt32(Saved->StartCull * Value), FMath::RoundToInt32(Base * Value));
				}
				else if (Switch == TEXT("shadow"))
				{
					Part->SetCastShadow(Value != 0.f);
				}
				else if (Switch == TEXT("lod"))
				{
					Part->SetForcedLodModel(FMath::RoundToInt32(Value));
				}
				else if (Switch == TEXT("wpo"))
				{
					Part->SetWorldPositionOffsetDisableDistance(FMath::RoundToInt32(Value));
				}
				Instances += Part->GetInstanceCount();
			}
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Perf.Scatter: %s %s on %d parts (%d instances) holding %s."), *Switch,
			*FString::SanitizeFloat(Value), Parts, Instances, *Args[0]);
	}

	FAutoConsoleCommandWithWorldAndArgs ScatterCommandRegistration(
		TEXT("Looter.Perf.Scatter"),
		TEXT("Switches one thing on the instanced parts whose mesh name holds one of the names, to time its cost: Looter.Perf.Scatter <names|...> <thin share|cull scale|shadow 0/1|lod n|wpo cm|reset>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ScatterCommand));

	/**
	 * Looter.Perf.HideLabel <label start> [1|0]: hides (1, the default) or shows again (0) every actor whose outliner label
	 * starts so (the Beyond folder's Ring_, CanyonWall_, Backdrop_ and FarTrees_ pieces), to split a folder's cost.
	 */
	void HideLabelCommand(const TArray<FString>& Args, UWorld* World)
	{
		World = SplitWorld(World);
		if (!World || Args.Num() == 0)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Perf.HideLabel <label start> [1|0]: %s"), World ? TEXT("which label?") : TEXT("no world."));
			return;
		}
#if WITH_EDITOR
		const bool bHide = Args.Num() < 2 || FCString::Atoi(*Args[1]) != 0;
		int32 Actors = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->GetActorLabel().StartsWith(Args[0]))
			{
				It->SetActorHiddenInGame(bHide);
				++Actors;
			}
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Perf.HideLabel: %s %d actors labelled %s..."), bHide ? TEXT("hid") : TEXT("showed"), Actors, *Args[0]);
#else
		UE_LOG(LogLooter, Warning, TEXT("Looter.Perf.HideLabel: labels exist only when the game runs from the editor's binary."));
#endif
	}

	FAutoConsoleCommandWithWorldAndArgs HideLabelCommandRegistration(
		TEXT("Looter.Perf.HideLabel"),
		TEXT("Hides (1, the default) or shows again (0) every actor whose label starts so, to measure its cost by the difference: Looter.Perf.HideLabel <label start> [1|0]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HideLabelCommand));
}

#endif

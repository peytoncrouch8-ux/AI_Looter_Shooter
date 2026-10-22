#include "Weapons/WeaponFX.h"
#include "Components/InstancedStaticMeshComponent.h"

// FWeaponFX's instanced components filled each frame (WeaponFXDraw.cpp's Redraw hands every component its quads or chunks).

namespace
{
	/**
	 * Instances are kept in blocks of this many, the spare ones drawn at zero size with zeroed custom data (no opacity): a
	 * particle born or gone then rarely changes the count. A changed count rebuilds the component's render state (its scene
	 * proxy) on the render thread, and during a fight or a blast that was every frame (VFX-2, 2026-10-22).
	 */
	constexpr int32 InstanceBlock = 32;
}

void FWeaponFX::DrawInstances(UInstancedStaticMeshComponent* Instances, const TArray<FTransform>& Transforms, const TArray<float>& CustomData)
{
	if (!Instances)
	{
		return;
	}
	const int32 Wanted = Transforms.Num();
	const int32 Current = Instances->GetInstanceCount();
	// Grown to the next block; shrunk only once two blocks are spare (no flicker of counts at a block's edge); none at all when
	// nothing's drawn (an empty component costs nothing).
	int32 Count = Current;
	if (Wanted == 0)
	{
		Count = 0;
	}
	else if (Wanted > Current || Wanted < Current - 2 * InstanceBlock)
	{
		Count = FMath::DivideAndRoundUp(Wanted, InstanceBlock) * InstanceBlock;
	}
	const int32 Floats = Instances->NumCustomDataFloats;
	const bool bData = Wanted > 0 && Floats > 0 && CustomData.Num() == Wanted * Floats;
	TArray<FTransform> Padded;
	TArray<float> PaddedData;
	const TArray<FTransform>* Drawn = &Transforms;
	const TArray<float>* Data = &CustomData;
	if (Count > Wanted)
	{
		const FTransform Hidden(FQuat::Identity, Transforms[0].GetLocation(), FVector::ZeroVector);
		Padded.Reserve(Count);
		Padded.Append(Transforms);
		while (Padded.Num() < Count)
		{
			Padded.Add(Hidden);
		}
		Drawn = &Padded;
		if (bData)
		{
			PaddedData.Reserve(Count * Floats);
			PaddedData.Append(CustomData);
			PaddedData.AddZeroed((Count - Wanted) * Floats);
			Data = &PaddedData;
		}
	}
	const bool bCountChanged = Count != Current;
	if (bCountChanged)
	{
		Instances->ClearInstances();
		if (Count > 0)
		{
			Instances->AddInstances(*Drawn, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);
		}
	}
	else if (Count > 0)
	{
		Instances->BatchUpdateInstancesTransforms(0, *Drawn, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
	}
	if (bData)
	{
		Instances->SetCustomData(0, Count - 1, *Data, false);
	}
	// Moved instances need no render state rebuild: the instance data tracks its own changes and sends them at the end of the
	// frame (FPrimitiveInstanceDataManager). A changed count (past a block) still takes the full refresh.
	if (bCountChanged)
	{
		Instances->MarkRenderStateDirty();
	}
}

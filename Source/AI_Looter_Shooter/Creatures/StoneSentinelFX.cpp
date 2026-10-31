// StoneSentinelFX (StoneSentinelFX.h): the Stone Sentinel's dust kept off the lens and within a budget.

#include "Creatures/StoneSentinelFX.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The budget's tokens and when they were last topped up (world seconds); a new world starts it afresh. */
	float BudgetTokens = StoneSentinelFX::PuffBurst;
	double BudgetTime = -1.0;
	TWeakObjectPtr<const UWorld> BudgetWorld;

	/** Up to Count puffs from the budget: how many may spawn now. */
	int32 TakePuffs(const UWorld& World, int32 Count)
	{
		const double Now = World.GetTimeSeconds();
		if (BudgetWorld.Get() != &World || Now < BudgetTime)
		{
			BudgetWorld = &World;
			BudgetTokens = StoneSentinelFX::PuffBurst;
			BudgetTime = Now;
		}
		BudgetTokens = FMath::Min(StoneSentinelFX::PuffBurst,
			BudgetTokens + static_cast<float>(Now - BudgetTime) * StoneSentinelFX::PuffsPerSecond);
		BudgetTime = Now;
		const int32 Given = FMath::Clamp(FMath::FloorToInt32(BudgetTokens), 0, FMath::Max(Count, 0));
		BudgetTokens -= static_cast<float>(Given);
		return Given;
	}

	float NearestOf(const TArray<FVector, TInlineAllocator<2>>& Lenses, const FVector& At)
	{
		float Nearest = TNumericLimits<float>::Max();
		for (const FVector& Lens : Lenses)
		{
			Nearest = FMath::Min(Nearest, static_cast<float>(FVector::Dist(Lens, At)));
		}
		return Nearest;
	}
}

namespace StoneSentinelFX
{
	bool CrowdsLens(float Distance, float Size, float Travel)
	{
		return Distance - FMath::Max(Size, 0.f) * 0.5f - FMath::Max(Travel, 0.f) < LensClear;
	}

	void GatherLenses(const UObject* WorldContext, TArray<FVector, TInlineAllocator<2>>& OutLenses)
	{
		OutLenses.Reset();
		const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
		if (!World || !World->IsGameWorld())
		{
			return;
		}
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* PC = It->Get();
			if (PC && PC->IsLocalController() && PC->PlayerCameraManager)
			{
				OutLenses.Add(PC->PlayerCameraManager->GetCameraLocation());
			}
		}
	}

	float LensDistance(const UObject* WorldContext, const FVector& At)
	{
		TArray<FVector, TInlineAllocator<2>> Lenses;
		GatherLenses(WorldContext, Lenses);
		return NearestOf(Lenses, At);
	}

	bool IsClearOfLens(const UObject* WorldContext, const FVector& At, float Size, float Travel)
	{
		return !CrowdsLens(LensDistance(WorldContext, At), Size, Travel);
	}

	int32 Dust(const UObject* WorldContext, const FVector& At, const FGraveDustBurst& Burst)
	{
		UGravemotherFX* Effects = UGravemotherFX::Get(WorldContext);
		const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
		if (!Effects || !World || Burst.Count <= 0)
		{
			return 0;
		}
		TArray<FVector, TInlineAllocator<2>> Lenses;
		GatherLenses(WorldContext, Lenses);
		// How far a puff may carry over its life: its own drift and its push outward (it slows as it goes).
		const float Travel = (static_cast<float>(Burst.Drift.Size()) + Burst.Outward * 0.6f) * Burst.Life * 0.6f;
		FGraveDustBurst One = Burst;
		One.Count = 1;
		One.Spread = 0.f;
		One.SpreadUp = 0.f;
		int32 Spawned = 0;
		for (int32 Index = 0; Index < Burst.Count; ++Index)
		{
			const FVector Out = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0.f).GetSafeNormal();
			const FVector Spot = At + Out * FMath::FRandRange(0.f, Burst.Spread) + FVector(0.0, 0.0, FMath::FRandRange(0.f, Burst.SpreadUp));
			if (CrowdsLens(NearestOf(Lenses, Spot), Burst.EndSize * 1.2f, Travel) || TakePuffs(*World, 1) <= 0)
			{
				continue;
			}
			Effects->Puffs(Spot, One);
			++Spawned;
		}
		return Spawned;
	}
}

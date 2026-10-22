#include "Weapons/VfxSubsystem.h"
#include "Weapons/VfxTuning.h"
#include "Engine/World.h"

// UVfxSubsystem: the VFX pass's ground splats (FDeathSplats), ticked while one shows.

UVfxSubsystem* UVfxSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UVfxSubsystem>() : nullptr;
}

bool UVfxSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Played worlds, and the editor preview worlds the automated tests build.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

bool UVfxSubsystem::AddSplat(const FVector& Ground, const FVector& Normal, EVfxSplat Kind, const FLinearColor& Tint, float Size)
{
	UWorld* World = GetWorld();
	const FVfxTuning Tuning = FVfxTuning::Live();
	if (!World || !Tuning.bSplats)
	{
		return false;
	}
	if (!Splats.Add(*World, Ground, Normal, Kind, Tint, Size, Tuning.SplatHold, Tuning.SplatFade, Tuning.SplatMax, FMath::FRand()))
	{
		return false;
	}
	LiveUntil = FMath::Max(LiveUntil, World->GetTimeSeconds() + Tuning.SplatHold + Tuning.SplatFade + 0.1);
	return true;
}

void UVfxSubsystem::Advance(float DeltaSeconds)
{
	const UWorld* World = GetWorld();
	if (World)
	{
		Splats.Advance(World->GetTimeSeconds());
	}
}

int32 UVfxSubsystem::NumSplats() const
{
	const UWorld* World = GetWorld();
	return World ? Splats.NumLive(World->GetTimeSeconds()) : 0;
}

void UVfxSubsystem::Deinitialize()
{
	Splats.Shutdown();
	LiveUntil = -1.0;
	Super::Deinitialize();
}

void UVfxSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Advance(DeltaTime);
}

bool UVfxSubsystem::IsTickable() const
{
	const UWorld* World = GetWorld();
	return World && World->GetTimeSeconds() <= LiveUntil;
}

TStatId UVfxSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UVfxSubsystem, STATGROUP_Tickables);
}

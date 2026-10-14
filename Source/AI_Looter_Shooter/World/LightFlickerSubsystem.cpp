// ULightFlickerSubsystem: lamps and lanterns near the view burn like flames; far ones rest and cost nothing.

#include "World/LightFlickerSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/LightComponent.h"
#include "Engine/Engine.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarFlicker(TEXT("Looter.Flicker"), 1,
		TEXT("Lamplight that breathes: lamps and lanterns near the view flicker (0: all still at their own intensity, to measure it by difference)."));
}

const FName ULightFlickerSubsystem::FlickerTag(TEXT("Flicker"));

ULightFlickerSubsystem* ULightFlickerSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject && GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<ULightFlickerSubsystem>() : nullptr;
}

bool ULightFlickerSubsystem::IsEnabled()
{
	return CVarFlicker.GetValueOnGameThread() != 0;
}

bool ULightFlickerSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void ULightFlickerSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	GatherTagged();
	// A level streamed in later brings its own lamps.
	LevelAddedHandle = FWorldDelegates::LevelAddedToWorld.AddUObject(this, &ULightFlickerSubsystem::HandleLevelAdded);
}

void ULightFlickerSubsystem::Deinitialize()
{
	FWorldDelegates::LevelAddedToWorld.Remove(LevelAddedHandle);
	LevelAddedHandle.Reset();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LookTimer);
	}
	// The world is going with its lights (a played world is a copy): nothing to set back.
	Lights.Reset();
	NearCount = 0;
	Super::Deinitialize();
}

void ULightFlickerSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Step(DeltaTime);
}

bool ULightFlickerSubsystem::IsTickable() const
{
	// Only while a light is near the view: far off, nothing runs but the look twice a second.
	return NearCount > 0;
}

TStatId ULightFlickerSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULightFlickerSubsystem, STATGROUP_Tickables);
}

// ---------------------------------------------------------------------------
// Which lights
// ---------------------------------------------------------------------------

ULightFlickerSubsystem::FFlickerLight* ULightFlickerSubsystem::Find(const ULightComponent* Light)
{
	return Lights.FindByPredicate([Light](const FFlickerLight& Each) { return Each.Light.Get() == Light; });
}

const ULightFlickerSubsystem::FFlickerLight* ULightFlickerSubsystem::Find(const ULightComponent* Light) const
{
	return Lights.FindByPredicate([Light](const FFlickerLight& Each) { return Each.Light.Get() == Light; });
}

void ULightFlickerSubsystem::AddLight(ULightComponent* Light, UMaterialInstanceDynamic* Glass, FName GlassParameter, bool bCarried)
{
	// A sun never flickers, and a static light can't be changed in the game.
	if (!Light || Light->IsA<UDirectionalLightComponent>() || Light->Mobility == EComponentMobility::Static)
	{
		if (Light)
		{
			UE_LOG(LogLooter, Verbose, TEXT("Flicker: %s can't flicker (a sun, or static)."), *Light->GetPathName());
		}
		return;
	}
	FFlickerLight* Each = Find(Light);
	if (!Each)
	{
		Lights.RemoveAll([](const FFlickerLight& Known) { return !Known.Light.IsValid(); });
		Each = &Lights.AddDefaulted_GetRef();
		Each->Light = Light;
		Each->Own = Light->Intensity;
		Each->Written = Light->Intensity;
		Each->Flame.Start(Random, bCarried);
	}
	else if (bCarried)
	{
		// Keeps its clock: only its carries start.
		Each->Flame.bCarries = true;
	}
	if (Glass && !GlassParameter.IsNone())
	{
		Each->Glass = Glass;
		Each->GlassParameter = GlassParameter;
		Each->OwnGlow = Glass->K2_GetScalarParameterValue(GlassParameter);
		Each->GlowWritten = Each->OwnGlow;
	}
	UpdateTimer();
}

void ULightFlickerSubsystem::RemoveLight(ULightComponent* Light)
{
	const int32 Index = Lights.IndexOfByPredicate([Light](const FFlickerLight& Each) { return Each.Light.Get() == Light; });
	if (Index == INDEX_NONE)
	{
		return;
	}
	Rest(Lights[Index]);
	NearCount -= Lights[Index].bNear ? 1 : 0;
	Lights.RemoveAt(Index);
	UpdateTimer();
}

void ULightFlickerSubsystem::GatherTagged(const ULevel* Level)
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	auto GatherFrom = [this](const ULevel* From)
	{
		for (AActor* Actor : From->Actors)
		{
			if (!Actor)
			{
				continue;
			}
			const bool bEveryLight = Actor->ActorHasTag(FlickerTag);
			TInlineComponentArray<ULightComponent*> Found(Actor);
			for (ULightComponent* Light : Found)
			{
				if (Light && (bEveryLight || Light->ComponentHasTag(FlickerTag)) && !Find(Light))
				{
					AddLight(Light);
				}
			}
		}
	};
	if (Level)
	{
		GatherFrom(Level);
		return;
	}
	for (const ULevel* Each : World->GetLevels())
	{
		if (Each)
		{
			GatherFrom(Each);
		}
	}
}

void ULightFlickerSubsystem::HandleLevelAdded(ULevel* Level, UWorld* World)
{
	if (Level && World == GetWorld())
	{
		GatherTagged(Level);
	}
}

int32 ULightFlickerSubsystem::NumLights() const
{
	return Lights.FilterByPredicate([](const FFlickerLight& Each) { return Each.Light.IsValid(); }).Num();
}

bool ULightFlickerSubsystem::IsFlickering(const ULightComponent* Light) const
{
	const FFlickerLight* Each = Find(Light);
	return Each && Each->bNear;
}

float ULightFlickerSubsystem::GetOwnIntensity(const ULightComponent* Light) const
{
	const FFlickerLight* Each = Find(Light);
	return Each ? Each->Own : -1.f;
}

float ULightFlickerSubsystem::GetOwnGlow(const ULightComponent* Light) const
{
	const FFlickerLight* Each = Find(Light);
	return Each && Each->Glass.IsValid() ? Each->OwnGlow : -1.f;
}

const FFlameFlicker* ULightFlickerSubsystem::GetFlame(const ULightComponent* Light) const
{
	const FFlickerLight* Each = Find(Light);
	return Each ? &Each->Flame : nullptr;
}

// ---------------------------------------------------------------------------
// Near and far
// ---------------------------------------------------------------------------

void ULightFlickerSubsystem::UpdateTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	FTimerManager& Timers = World->GetTimerManager();
	if (Lights.IsEmpty())
	{
		Timers.ClearTimer(LookTimer);
		NearCount = 0;
	}
	else if (!Timers.IsTimerActive(LookTimer))
	{
		Timers.SetTimer(LookTimer, this, &ULightFlickerSubsystem::LookFromView, LightFlickerRules::LookSeconds, /*bLoop*/ true,
			Random.FRandRange(0.05f, LightFlickerRules::LookSeconds));
	}
}

void ULightFlickerSubsystem::LookFromView()
{
	const UWorld* World = GetWorld();
	const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
	if (Player && Player->PlayerCameraManager)
	{
		Look(Player->PlayerCameraManager->GetCameraLocation());
		return;
	}
	if (const APawn* Pawn = Player ? Player->GetPawn() : nullptr)
	{
		Look(Pawn->GetActorLocation());
		return;
	}
	// Nobody looking: every light at its own.
	for (FFlickerLight& Each : Lights)
	{
		if (Each.bNear)
		{
			Rest(Each);
			Each.bNear = false;
		}
	}
	NearCount = 0;
}

void ULightFlickerSubsystem::Look(const FVector& Eyes)
{
	const bool bOn = IsEnabled();
	const double ReachSquared = FMath::Square(static_cast<double>(LightFlickerRules::Radius));
	Lights.RemoveAll([](const FFlickerLight& Each) { return !Each.Light.IsValid(); });
	NearCount = 0;
	for (FFlickerLight& Each : Lights)
	{
		const ULightComponent* Light = Each.Light.Get();
		const AActor* Owner = Light->GetOwner();
		const bool bShown = Light->IsVisible() && !(Owner && Owner->IsHidden());
		const bool bNear = bOn && bShown && FVector::DistSquared(Light->GetComponentLocation(), Eyes) <= ReachSquared;
		if (!bNear && Each.bNear)
		{
			Rest(Each);
		}
		Each.bNear = bNear;
		NearCount += bNear ? 1 : 0;
	}
}

// ---------------------------------------------------------------------------
// The flames
// ---------------------------------------------------------------------------

void ULightFlickerSubsystem::Step(float DeltaSeconds)
{
	if (NearCount == 0)
	{
		return;
	}
	StepClock += FMath::Max(DeltaSeconds, 0.f);
	if (StepClock < LightFlickerRules::StepSeconds)
	{
		return;
	}
	const float Elapsed = StepClock;
	StepClock = 0.f;
	for (FFlickerLight& Each : Lights)
	{
		const ULightComponent* Light = Each.Light.Get();
		if (Each.bNear && Light && Light->IsVisible())
		{
			Each.Flame.Advance(Elapsed, Random);
			Write(Each);
		}
	}
}

void ULightFlickerSubsystem::Adopt(FFlickerLight& Each) const
{
	if (const ULightComponent* Light = Each.Light.Get())
	{
		// Set by somebody else since it was last written (the lighting state's lamp, the story): its own from now on.
		if (Light->Intensity != Each.Written)
		{
			Each.Own = Light->Intensity;
			Each.Written = Light->Intensity;
		}
	}
	if (UMaterialInstanceDynamic* Glass = Each.Glass.Get())
	{
		const float Glow = Glass->K2_GetScalarParameterValue(Each.GlassParameter);
		if (Glow != Each.GlowWritten)
		{
			Each.OwnGlow = Glow;
			Each.GlowWritten = Glow;
		}
	}
}

void ULightFlickerSubsystem::Write(FFlickerLight& Each)
{
	Adopt(Each);
	if (ULightComponent* Light = Each.Light.Get())
	{
		Light->SetIntensity(Each.Own * Each.Flame.GetFactor());
		// What it holds now: a light that can't be changed keeps its own, and isn't taken for somebody else's.
		Each.Written = Light->Intensity;
	}
	if (UMaterialInstanceDynamic* Glass = Each.Glass.Get())
	{
		const float Glow = Each.OwnGlow * Each.Flame.GetGlassFactor();
		Glass->SetScalarParameterValue(Each.GlassParameter, Glow);
		Each.GlowWritten = Glow;
	}
}

void ULightFlickerSubsystem::Rest(FFlickerLight& Each)
{
	Adopt(Each);
	if (ULightComponent* Light = Each.Light.Get())
	{
		Light->SetIntensity(Each.Own);
		Each.Written = Light->Intensity;
	}
	if (UMaterialInstanceDynamic* Glass = Each.Glass.Get())
	{
		Glass->SetScalarParameterValue(Each.GlassParameter, Each.OwnGlow);
		Each.GlowWritten = Each.OwnGlow;
	}
}

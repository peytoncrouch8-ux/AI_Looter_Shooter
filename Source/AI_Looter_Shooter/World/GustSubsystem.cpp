#include "World/GustSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "World/AmbientAirSubsystem.h"
#include "World/SwingComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Misc/PackageName.h"
#include "TimerManager.h"

const TCHAR* UGustSubsystem::CollectionPath = TEXT("/Game/Art/Materials/MPC_Wind.MPC_Wind");
const FName UGustSubsystem::GustStrengthParameter(TEXT("GustStrength"));
const FName UGustSubsystem::GustDirectionParameter(TEXT("GustDirection"));

namespace
{
	/** The longest step a gust's own update takes (s): after a hitch it catches up without leaping. */
	constexpr float LongestStep = 0.25f;

	/** The soonest the timer looks again (s), so a timer that fires a hair early never spins. */
	constexpr float ShortestWait = 0.05f;
}

UGustSubsystem* UGustSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UGustSubsystem>() : nullptr;
}

float UGustSubsystem::StrengthAt(const UObject* WorldContextObject)
{
	const UGustSubsystem* Gusts = Get(WorldContextObject);
	return Gusts ? Gusts->GetStrengthNow() : 0.f;
}

bool UGustSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UGustSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Clock.Reset(0x6057, DefaultWindYaw);
}

void UGustSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(NextGustTimer);
		World->GetTimerManager().ClearTimer(SwingWatchTimer);
	}
	if (bStirring)
	{
		StillWorld();
	}
	bStirring = false;
	Swings.Reset();
	OnGust.Clear();
	Super::Deinitialize();
}

void UGustSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// The level's own seed: every visit's gusts come at the same times, PIE's as the packaged game's.
	const FString Map = UWorld::RemovePIEPrefix(InWorld.GetMapName());
	ResetClock(static_cast<int32>(GetTypeHash(Map) ^ 0x6057u));
	bClockStarted = true;
	LastStepTime = InWorld.GetTimeSeconds();
	// The materials start still, whatever the collection was saved with.
	WriteCollection(0.f, GetPrevailingDirection());
	ArmTimer();
	if (Swings.Num() > 0)
	{
		SetSwingWatch(true);
	}
}

TStatId UGustSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGustSubsystem, STATGROUP_Tickables);
}

bool UGustSubsystem::IsTickable() const
{
	// Between gusts a timer wakes it; it ticks only while one blows.
	return Clock.IsBlowing();
}

void UGustSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	StepByWorldClock();
}

void UGustSubsystem::ResetClock(int32 Seed)
{
	if (Clock.IsBlowing())
	{
		EndGust();
	}
	Clock.Reset(Seed, DefaultWindYaw);
	bWasBlowing = false;
}

// ---------------------------------------------------------------------------
// The clock
// ---------------------------------------------------------------------------

void UGustSubsystem::StepByWorldClock()
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	if (!bClockStarted)
	{
		bClockStarted = true;
		LastStepTime = Now;
	}
	const float DeltaSeconds = static_cast<float>(FMath::Max(Now - LastStepTime, 0.0));
	LastStepTime = Now;
	Advance(DeltaSeconds);
}

void UGustSubsystem::Advance(float DeltaSeconds)
{
	const bool bStarted = Clock.Advance(DeltaSeconds);
	if (bStarted)
	{
		BeginGust();
	}
	if (Clock.IsBlowing())
	{
		// A gust that began in this step has blown only as long as the step overshot its start.
		UpdateGust(FMath::Min(bStarted ? Clock.GetAge() : DeltaSeconds, LongestStep));
	}
	else if (bWasBlowing || bStarted)
	{
		EndGust();
	}
	bWasBlowing = Clock.IsBlowing();
	if (!bWasBlowing)
	{
		ArmTimer();
	}
}

bool UGustSubsystem::Trigger(float Strength, float Seconds)
{
	FGustEvent Gust = Clock.Roll();
	if (Strength >= 0.f)
	{
		Gust.Strength = FMath::Clamp(Strength, 0.f, 1.f);
	}
	if (Seconds > 0.f)
	{
		Gust.Duration = Seconds;
	}
	if (!Clock.StartNow(Gust))
	{
		return false;
	}
	if (UWorld* World = GetWorld())
	{
		// The tick takes over from here, its first step a frame long.
		LastStepTime = World->GetTimeSeconds();
		bClockStarted = true;
		if (World->IsGameWorld())
		{
			World->GetTimerManager().ClearTimer(NextGustTimer);
		}
	}
	BeginGust();
	UpdateGust(0.f);
	bWasBlowing = true;
	return true;
}

void UGustSubsystem::ArmTimer()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	World->GetTimerManager().SetTimer(NextGustTimer, FTimerDelegate::CreateUObject(this, &UGustSubsystem::StepByWorldClock),
		FMath::Max(Clock.GetSecondsToNext(), ShortestWait), false);
}

float UGustSubsystem::GetStrengthNow() const
{
	return bStirring ? Clock.GetStrengthNow() : 0.f;
}

FVector UGustSubsystem::GetDirection() const
{
	return Clock.IsBlowing() ? Clock.GetCurrent().Direction : GetPrevailingDirection();
}

FVector UGustSubsystem::GetPrevailingDirection() const
{
	return FRotator(0.f, Clock.GetWindYaw(), 0.f).Vector();
}

bool UGustSubsystem::IsSilenced() const
{
	return !UAmbientAirSubsystem::IsEnabled();
}

// ---------------------------------------------------------------------------
// A gust's life
// ---------------------------------------------------------------------------

void UGustSubsystem::BeginGust()
{
	// Looter.Air 0 stills the wind with the air: the clock runs on, so turning it back on finds the gusts where they'd be.
	bStirring = !IsSilenced();
	if (!bStirring)
	{
		return;
	}
	const FGustEvent Gust = Clock.GetCurrent();
	UE_LOG(LogLooter, Verbose, TEXT("Gust: %.2f toward %.0f degrees for %.1f s."), Gust.Strength, Gust.Direction.Rotation().Yaw, Gust.Duration);
	OnGust.Broadcast(Gust);
	StirWorld();
}

void UGustSubsystem::UpdateGust(float DeltaSeconds)
{
	if (!bStirring)
	{
		return;
	}
	WriteCollection(Clock.GetStrengthNow(), Clock.GetCurrent().Direction);
	StirWorldUpdate(DeltaSeconds);
}

void UGustSubsystem::EndGust()
{
	if (bStirring)
	{
		StillWorld();
		WriteCollection(0.f, Clock.GetCurrent().Direction);
	}
	bStirring = false;
}

void UGustSubsystem::WriteCollection(float Strength, const FVector& Direction)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (!bCollectionLooked)
	{
		// Looked for once: until Tools/Unreal/wind_collection.py has made it, the gusts move everything but the plants and smoke.
		bCollectionLooked = true;
		if (FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(CollectionPath))))
		{
			WindCollection = LoadObject<UMaterialParameterCollection>(nullptr, CollectionPath);
		}
		if (!WindCollection)
		{
			UE_LOG(LogLooter, Log, TEXT("Gusts: no %s yet (Tools/Unreal/wind_collection.py makes it): the foliage and smoke stay as they are."), CollectionPath);
		}
	}
	UMaterialParameterCollectionInstance* Instance = WindCollection ? World->GetParameterCollectionInstance(WindCollection) : nullptr;
	if (!Instance)
	{
		return;
	}
	const FVector Level = Direction.GetSafeNormal2D();
	Instance->SetScalarParameterValue(GustStrengthParameter, Strength);
	Instance->SetVectorParameterValue(GustDirectionParameter, FLinearColor(static_cast<float>(Level.X), static_cast<float>(Level.Y), 0.f, 0.f));
}

// ---------------------------------------------------------------------------
// The swings
// ---------------------------------------------------------------------------

void UGustSubsystem::RegisterSwing(USwingComponent* Swing)
{
	if (!Swing)
	{
		return;
	}
	Swings.AddUnique(Swing);
	SetSwingWatch(true);
}

void UGustSubsystem::UnregisterSwing(USwingComponent* Swing)
{
	Swings.Remove(Swing);
	Swings.RemoveAll([](const TWeakObjectPtr<USwingComponent>& Each) { return !Each.IsValid(); });
	if (Swings.Num() == 0)
	{
		SetSwingWatch(false);
	}
}

void UGustSubsystem::SetSwingWatch(bool bOn)
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	FTimerManager& Timers = World->GetTimerManager();
	if (!bOn)
	{
		Timers.ClearTimer(SwingWatchTimer);
		return;
	}
	if (!Timers.IsTimerActive(SwingWatchTimer))
	{
		Timers.SetTimer(SwingWatchTimer, FTimerDelegate::CreateUObject(this, &UGustSubsystem::WatchSwings), SwingWatchSeconds, true,
			FMath::FRandRange(0.05f, SwingWatchSeconds));
	}
}

void UGustSubsystem::WatchSwings()
{
	FVector View;
	const bool bView = FindView(View);
	for (int32 Index = Swings.Num() - 1; Index >= 0; --Index)
	{
		USwingComponent* Swing = Swings[Index].Get();
		if (!Swing)
		{
			Swings.RemoveAtSwap(Index);
			continue;
		}
		Swing->SetNear(bView && FVector::DistSquared(View, Swing->GetComponentLocation()) < FMath::Square(static_cast<double>(Swing->ActiveRadius)));
	}
}

bool UGustSubsystem::FindView(FVector& OutLocation) const
{
	const UWorld* World = GetWorld();
	const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
	if (!Player)
	{
		return false;
	}
	if (Player->PlayerCameraManager)
	{
		OutLocation = Player->PlayerCameraManager->GetCameraLocation();
		return true;
	}
	if (const APawn* Pawn = Player->GetPawn())
	{
		OutLocation = Pawn->GetActorLocation();
		return true;
	}
	return false;
}

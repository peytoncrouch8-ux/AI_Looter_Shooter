#include "Loot/PickupPullSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Loot/SoulMotePickup.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** The bundle spins up as it flies (degrees a second). */
	constexpr float FlightSpin = 900.f;
	/** The glint's size tip to tip (cm), a little over the bundle's, a hair above its middle. */
	constexpr float FlightGlintSize = 30.f;
	constexpr double FlightGlintLift = 6.0;
	/** It starts shrinking this far along. */
	constexpr float FlightShrinkFrom = 0.55f;
}

UPickupPullSubsystem* UPickupPullSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UPickupPullSubsystem>() : nullptr;
}

bool UPickupPullSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Played worlds, and the editor preview worlds the automated tests build.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UPickupPullSubsystem::Deinitialize()
{
	Flights.Reset();
	Pool.Reset();
	Holder = nullptr;
	Super::Deinitialize();
}

void UPickupPullSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Advance(DeltaTime);
}

bool UPickupPullSubsystem::IsTickable() const
{
	return NumFlying() > 0;
}

TStatId UPickupPullSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPickupPullSubsystem, STATGROUP_Tickables);
}

int32 UPickupPullSubsystem::NumFlying() const
{
	int32 Count = 0;
	for (const FFlight& Flight : Flights)
	{
		Count += Flight.bActive ? 1 : 0;
	}
	return Count;
}

int32 UPickupPullSubsystem::FindFreeSlot()
{
	// Components whose actor went with the level are made again.
	if (!IsValid(Holder))
	{
		Holder = nullptr;
		Pool.Reset();
		Flights.Reset();
	}
	for (int32 Slot = 0; Slot < Flights.Num(); ++Slot)
	{
		if (!Flights[Slot].bActive && IsValid(Pool[Slot]))
		{
			return Slot;
		}
	}
	if (Pool.Num() >= MaxFlights)
	{
		return INDEX_NONE;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return INDEX_NONE;
	}
	if (!Holder)
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Holder = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
		if (!Holder)
		{
			return INDEX_NONE;
		}
#if WITH_EDITOR
		Holder->SetActorLabel(TEXT("PickupPull"));
#endif
		USceneComponent* Root = NewObject<USceneComponent>(Holder, TEXT("Root"), RF_Transient);
		Root->SetMobility(EComponentMobility::Movable);
		Holder->SetRootComponent(Root);
		Root->RegisterComponent();
	}
	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Holder, *FString::Printf(TEXT("Flight%d"), Pool.Num()), RF_Transient);
	Mesh->SetupAttachment(Holder->GetRootComponent());
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCanEverAffectNavigation(false);
	// A small thing flying for a fraction of a second: no shadow to pay for.
	Mesh->SetCastShadow(false);
	Mesh->bReceivesDecals = false;
	Mesh->SetVisibility(false);
	Mesh->RegisterComponent();
	Pool.Add(Mesh);
	Flights.AddDefaulted();
	return Pool.Num() - 1;
}

bool UPickupPullSubsystem::FlyIn(UStaticMesh* Mesh, const FTransform& From, APawn* Player, const AActor* Pickup, const FLinearColor& GlintColor)
{
	const UWorld* World = GetWorld();
	if (!Mesh || !IsValid(Player) || !World)
	{
		return false;
	}
	const UHealthComponent* Health = Player->FindComponentByClass<UHealthComponent>();
	if (Health && Health->IsDead())
	{
		return false;
	}
	const FVector Start = From.GetLocation();
	if (!PickupPull::CanPull(*World, Start, *Player, FlightRange, Pickup))
	{
		return false;
	}
	const int32 Slot = FindFreeSlot();
	if (Slot == INDEX_NONE)
	{
		return false;
	}
	UStaticMeshComponent* Shown = Pool[Slot];
	Shown->SetStaticMesh(Mesh);
	Shown->SetWorldTransform(From);
	Shown->SetVisibility(true);

	FFlight& Flight = Flights[Slot];
	Flight.Player = Player;
	Flight.Path.Start(Start, PickupPull::TargetOf(*Player), StartSpeed);
	Flight.Scale = From.GetScale3D();
	Flight.Rotation = From.Rotator();
	Flight.LastShown = Start;
	Flight.GlintColor = GlintColor;
	Flight.bActive = true;
	return true;
}

void UPickupPullSubsystem::EndFlight(int32 Slot)
{
	if (Flights.IsValidIndex(Slot))
	{
		Flights[Slot].bActive = false;
		Flights[Slot].Player.Reset();
	}
	if (Pool.IsValidIndex(Slot) && IsValid(Pool[Slot]))
	{
		Pool[Slot]->SetVisibility(false);
	}
}

void UPickupPullSubsystem::Advance(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	for (int32 Slot = 0; Slot < Flights.Num(); ++Slot)
	{
		FFlight& Flight = Flights[Slot];
		if (!Flight.bActive)
		{
			continue;
		}
		UStaticMeshComponent* Shown = Pool.IsValidIndex(Slot) ? Pool[Slot].Get() : nullptr;
		const APawn* Player = Flight.Player.Get();
		if (!IsValid(Shown) || !Player || !World)
		{
			EndFlight(Slot);
			continue;
		}
		// Into the body, however it moves meanwhile; one that can't catch a player running off is gone at its time.
		const bool bArrived = Flight.Path.Step(DeltaSeconds, PickupPull::TargetOf(*Player), Acceleration, MaxSpeed);
		if (bArrived || Flight.Path.GetAge() >= MostSeconds)
		{
			EndFlight(Slot);
			continue;
		}
		const FVector Where = Flight.Path.GetLocation();
		const float Progress = Flight.Path.GetProgress();
		const float Shrink = FMath::SmoothStep(FlightShrinkFrom, 1.f, Progress);
		Flight.Rotation.Yaw += FlightSpin * DeltaSeconds;
		Shown->SetWorldLocationAndRotation(Where, Flight.Rotation);
		Shown->SetWorldScale3D(Flight.Scale * FMath::Lerp(1.f, EndScale, Shrink));

		PickupPull::DrawGlint(*World, Where + FVector(0.0, 0.0, FlightGlintLift), PickupPull::GlintAt(Flight.Path.GetAge()), FlightGlintSize,
			Flight.GlintColor);
		PickupPull::DrawTrail(*World, Flight.LastShown, Where, 1.f - 0.6f * Progress, Flight.GlintColor);
		Flight.LastShown = Where;
	}
}

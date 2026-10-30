#include "Creatures/UnpaidPackSubsystem.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/UnpaidMoveTuning.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

// UUnpaidPackSubsystem's show for the failed-lunge shriek (2026-10-29): a pool of three point lights (the coal's flare lighting
// the ground and the Unpaid's own body: no shadows, hidden while unused), and the ground wave that rolls out under the scream's
// ring, throwing dust, grass and leaves up off its front (FWeaponFX::SpawnWaveLift, a ground trace a piece). Lights are the
// one cost the VFX pass avoided: three at most, 7 m at most, none in a test level, Looter.Unpaid.Shriek.Light 0 turns them off.

namespace
{
	/** A light given back fades out at this rate (1/s of its strength): about a fifth of a second. */
	constexpr float LightFadeRate = 5.f;

	/** The wave throws this many pieces a second at strength 1, and never more than this many in a frame. */
	constexpr float WavePiecesPerSecond = 70.f;
	constexpr int32 WavePiecesPerFrame = 4;

	/** A wave piece finds the ground within this far over and under the wave's feet (cm). */
	constexpr float WaveGroundAbove = 90.f;
	constexpr float WaveGroundBelow = 160.f;

	/** The golden angle (degrees): pieces spread round the ring evenly however many come. */
	constexpr float GoldenTurn = 137.508f;

	FWeaponFX* ShowEffects(UWorld* World)
	{
		UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
		if (!Bullets)
		{
			return nullptr;
		}
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		return &Effects;
	}
}

void UUnpaidPackSubsystem::Deinitialize()
{
	if (AActor* Owner = LightOwner.Get())
	{
		Owner->Destroy();
	}
	LightOwner.Reset();
	Waves.Reset();
	Boards.Reset();
	Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// The lights
// ---------------------------------------------------------------------------

UPointLightComponent* UUnpaidPackSubsystem::LightAt(int32 Index)
{
	if (Index < 0 || Index >= MaxLights)
	{
		return nullptr;
	}
	if (UPointLightComponent* Made = Lights[Index].Light.Get())
	{
		return Made;
	}
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return nullptr;
	}
	AActor* Owner = LightOwner.Get();
	if (!Owner)
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
		if (!Owner)
		{
			return nullptr;
		}
#if WITH_EDITOR
		Owner->SetActorLabel(TEXT("UnpaidShriekLights"));
#endif
		USceneComponent* Root = NewObject<USceneComponent>(Owner, TEXT("Root"), RF_Transient);
		Root->SetMobility(EComponentMobility::Movable);
		Owner->SetRootComponent(Root);
		Root->RegisterComponent();
		LightOwner = Owner;
	}
	UPointLightComponent* Light = NewObject<UPointLightComponent>(Owner, NAME_None, RF_Transient);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetupAttachment(Owner->GetRootComponent());
	// A flare, not a lamp: no shadows (a crowd of them would cost a shadow pass each), out of the indirect light.
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(0.f);
	Light->SetCastShadows(false);
	Light->SetIndirectLightingIntensity(0.f);
	Light->SetSourceRadius(12.f);
	Light->SetVisibility(false);
	Light->RegisterComponent();
	Lights[Index].Light = Light;
	return Light;
}

int32 UUnpaidPackSubsystem::TakeLight(const AActor* Holder)
{
	if (FUnpaidMoveTuning::Live().ShriekLight <= 0.f)
	{
		return INDEX_NONE;
	}
	for (int32 Index = 0; Index < MaxLights; ++Index)
	{
		if (!Lights[Index].bTaken && LightAt(Index))
		{
			Lights[Index].bTaken = true;
			Lights[Index].Holder = Holder;
			return Index;
		}
	}
	return INDEX_NONE;
}

void UUnpaidPackSubsystem::SetLight(int32 Handle, const FVector& Where, const FLinearColor& Color, float Candelas, float Radius)
{
	UPointLightComponent* Light = Handle >= 0 && Handle < MaxLights && Lights[Handle].bTaken ? Lights[Handle].Light.Get() : nullptr;
	if (!Light)
	{
		return;
	}
	const float Strength = FMath::Max(Candelas * FUnpaidMoveTuning::Live().ShriekLight, 0.f);
	Lights[Handle].Candelas = Strength;
	if (Strength <= 0.01f)
	{
		Light->SetVisibility(false);
		return;
	}
	Light->SetWorldLocation(Where);
	Light->SetLightColor(Color);
	Light->SetIntensity(Strength);
	if (!FMath::IsNearlyEqual(Light->AttenuationRadius, Radius, 1.f))
	{
		Light->SetAttenuationRadius(Radius);
	}
	if (!Light->IsVisible())
	{
		Light->SetVisibility(true);
	}
}

void UUnpaidPackSubsystem::GiveLight(int32 Handle)
{
	if (Handle >= 0 && Handle < MaxLights)
	{
		// It fades from where it was (AdvanceLights), so a light never snaps off mid-flare.
		Lights[Handle].bTaken = false;
	}
}

void UUnpaidPackSubsystem::AdvanceLights(float DeltaSeconds)
{
	for (FLight& Each : Lights)
	{
		UPointLightComponent* Light = Each.Light.Get();
		// Its shrieker gone without giving it back (removed mid-shriek): it fades out like any other.
		if (Each.bTaken && !Each.Holder.IsValid())
		{
			Each.bTaken = false;
		}
		if (Each.bTaken || Each.Candelas <= 0.f || !Light)
		{
			continue;
		}
		Each.Candelas = FMath::Max(Each.Candelas - Each.Candelas * LightFadeRate * DeltaSeconds - 2.f * DeltaSeconds, 0.f);
		if (Each.Candelas <= 0.5f)
		{
			Each.Candelas = 0.f;
			Light->SetVisibility(false);
			continue;
		}
		Light->SetIntensity(Each.Candelas);
	}
}

// ---------------------------------------------------------------------------
// The ground wave
// ---------------------------------------------------------------------------

void UUnpaidPackSubsystem::StartWave(const FVector& Feet, float Radius, float Seconds, float Strength, const AActor* Source)
{
	UWorld* World = GetWorld();
	const float Fx = FUnpaidMoveTuning::Live().ShriekFx;
	if (!World || Radius <= 0.f || Seconds <= 0.f || Strength * Fx <= 0.f)
	{
		return;
	}
	FWave& Wave = Waves.AddDefaulted_GetRef();
	Wave.Feet = Feet;
	Wave.Radius = Radius;
	Wave.Seconds = Seconds;
	Wave.Strength = Strength * Fx;
	Wave.Turn = FMath::FRandRange(0.f, 360.f);
	Wave.Source = Source;
	// The dust is shown in the light where the wave starts (one sun trace a wave: the yard is lit or shaded as a whole).
	Wave.Light = Source ? DustLight.At(*World, Feet + FVector(0.0, 0.0, 30.0), *Source) : FLinearColor::White;
}

void UUnpaidPackSubsystem::AdvanceWaves(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	FWeaponFX* Effects = Waves.IsEmpty() ? nullptr : ShowEffects(World);
	for (int32 Index = Waves.Num() - 1; Index >= 0; --Index)
	{
		FWave& Wave = Waves[Index];
		Wave.Age += DeltaSeconds;
		if (Wave.Age >= Wave.Seconds || !Effects || !World)
		{
			Waves.RemoveAtSwap(Index);
			continue;
		}
		// The front races out and slows (a shock's ease-out); the pieces it lifts thin as it spreads and weakens.
		const float Alpha = FMath::Clamp(Wave.Age / Wave.Seconds, 0.f, 1.f);
		const float Front = Wave.Radius * FMath::InterpEaseOut(0.f, 1.f, Alpha, 2.f);
		const float Strength = Wave.Strength * (1.f - 0.6f * Alpha);
		Wave.Owed += WavePiecesPerSecond * Wave.Strength * DeltaSeconds * Effects->DetailAt(Wave.Feet);
		const int32 Pieces = FMath::Min(FMath::FloorToInt32(Wave.Owed), WavePiecesPerFrame);
		Wave.Owed -= Pieces;
		if (Pieces <= 0 || Front < 40.f)
		{
			continue;
		}
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("UnpaidShriekWave"), Wave.Source.Get());
		for (int32 Piece = 0; Piece < Pieces; ++Piece)
		{
			Wave.Turn = FMath::Fmod(Wave.Turn + GoldenTurn + FMath::FRandRange(-12.f, 12.f), 360.f);
			const float Radians = FMath::DegreesToRadians(Wave.Turn);
			const FVector Out(FMath::Cos(Radians), FMath::Sin(Radians), 0.f);
			const FVector Over = Wave.Feet + Out * Front;
			// Each piece sits on the ground it lifts from (a slope, a step), never in the air or under the dirt.
			FHitResult Hit;
			if (!World->LineTraceSingleByObjectType(Hit, Over + FVector(0.f, 0.f, WaveGroundAbove), Over - FVector(0.f, 0.f, WaveGroundBelow),
				FCollisionObjectQueryParams(ECC_WorldStatic), Params))
			{
				continue;
			}
			Effects->SpawnWaveLift(Hit.ImpactPoint, Out, Wave.Light, Strength);
		}
	}
}

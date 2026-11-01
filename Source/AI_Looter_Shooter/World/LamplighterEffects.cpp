// LamplighterEffects: Landfall's dust, sparks, gas, smoke and fire (FWeaponFX), and the shakes and jolts on the view.

#include "World/LamplighterEffects.h"
#include "Combat/BulletSubsystem.h"
#include "Player/CameraShakeModifier.h"
#include "Player/ViewKick.h"
#include "Weapons/WeaponFX.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** Canyon dust as late sun shows it (the smoke material is unlit: the colour is the light on it). */
	const FLinearColor DustColor(0.46f, 0.38f, 0.30f);
	/** The bag's lifting gas: a pale breath, cooler than the dust. */
	const FLinearColor GasColor(0.78f, 0.80f, 0.80f);
	/** The tender's burning bag and its smoke: black with a little warmth in it. */
	const FLinearColor SmokeColor(0.07f, 0.06f, 0.06f);
	const FLinearColor FireColor(1.f, 0.48f, 0.16f);

	UWorld* WorldOf(const UObject* Context)
	{
		return GEngine ? GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull) : nullptr;
	}

	APlayerController* LocalPlayer(const UObject* Context)
	{
		UWorld* World = WorldOf(Context);
		APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller && Controller->IsLocalController() ? Controller : nullptr;
	}

	/** A direction within Spread (0-1) of Along, the way a spray scatters. */
	FVector Scatter(const FVector& Along, float Spread)
	{
		return (Along.GetSafeNormal() + FMath::VRand() * Spread).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	}
}

namespace LamplighterEffects
{
	FWeaponFX* Find(const UObject* Context)
	{
		UWorld* World = WorldOf(Context);
		if (!World || !World->IsGameWorld() || !LocalPlayer(Context))
		{
			return nullptr;
		}
		UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
		if (!Bullets)
		{
			return nullptr;
		}
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		return &Effects;
	}

	void Shake(const UObject* Context, float Strength, float Seconds)
	{
		if (UCameraShakeModifier* Modifier = UCameraShakeModifier::FindOrAdd(LocalPlayer(Context)))
		{
			Modifier->AddShake(Strength, Seconds);
		}
	}

	void Kick(const UObject* Context, const FViewKick& Jolt)
	{
		if (APlayerController* Controller = LocalPlayer(Context))
		{
			UCameraShakeModifier::Kick(Controller, Jolt);
		}
	}

	void DustSkirt(const UObject* Context, const FVector& Center, float Radius, int32 Count)
	{
		FWeaponFX* Effects = Find(Context);
		if (!Effects)
		{
			return;
		}
		for (int32 Index = 0; Index < Count; ++Index)
		{
			// Round the hull's footprint, thrown out and a little up, the near ones bigger and slower.
			const float Angle = UE_TWO_PI * (Index + FMath::FRand() * 0.6f) / FMath::Max(Count, 1);
			const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.0);
			const FVector From = Center + Out * (Radius * FMath::FRandRange(0.35f, 0.6f)) + FVector(0.0, 0.0, FMath::FRandRange(20.f, 80.f));
			const FVector Velocity = Out * FMath::FRandRange(600.f, 1500.f) + FVector(0.0, 0.0, FMath::FRandRange(80.f, 320.f));
			Effects->SpawnDustPuff(From, Velocity, DustColor, FMath::FRandRange(0.45f, 0.7f), FMath::FRandRange(160.f, 260.f),
				FMath::FRandRange(700.f, 1300.f), FMath::FRandRange(2.4f, 4.f));
			if (Index % 2 == 0)
			{
				Effects->SpawnGrit(From, Velocity * 0.7f + FVector(0.0, 0.0, 450.0), FMath::FRandRange(3.f, 7.f), 1.6f);
			}
		}
	}

	void Sparks(const UObject* Context, const FVector& Location, const FVector& Direction, int32 Count, float Scale)
	{
		FWeaponFX* Effects = Find(Context);
		if (!Effects)
		{
			return;
		}
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Effects->SpawnRicochet(Location + FMath::VRand() * 12.f, Scatter(Direction, 0.55f), Scale * FMath::FRandRange(0.7f, 1.3f));
		}
	}

	void GasPuff(const UObject* Context, const FVector& Location, const FVector& Direction)
	{
		if (FWeaponFX* Effects = Find(Context))
		{
			Effects->SpawnDustPuff(Location, Scatter(Direction, 0.25f) * FMath::FRandRange(700.f, 1100.f), GasColor,
				FMath::FRandRange(0.25f, 0.4f), 70.f, FMath::FRandRange(320.f, 480.f), FMath::FRandRange(0.9f, 1.4f));
		}
	}

	void SmokePuff(const UObject* Context, const FVector& Location, const FVector& Velocity)
	{
		if (FWeaponFX* Effects = Find(Context))
		{
			Effects->SpawnDustPuff(Location + FMath::VRand() * 60.f, Velocity, SmokeColor, FMath::FRandRange(0.55f, 0.8f),
				FMath::FRandRange(180.f, 260.f), FMath::FRandRange(700.f, 1000.f), FMath::FRandRange(3.f, 4.5f));
		}
	}

	void Fireball(const UObject* Context, const FVector& Location, const FVector& Velocity)
	{
		FWeaponFX* Effects = Find(Context);
		if (!Effects)
		{
			return;
		}
		Effects->SpawnFireball(Location, Velocity, 160.f, 620.f, FireColor, 8.f, 0.9f);
		Effects->SpawnFireball(Location + FMath::VRand() * 120.f, Velocity * 0.8f, 100.f, 420.f, FireColor, 6.f, 0.7f);
		for (int32 Index = 0; Index < 6; ++Index)
		{
			Effects->SpawnRicochet(Location, FMath::VRand(), 1.6f);
			SmokePuff(Context, Location, Velocity + FMath::VRand() * 300.f);
		}
	}
}

#include "Weapons/NiagaraFX.h"
#include "AI_Looter_Shooter.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

// The Niagara effects' spawn helpers (round 5). The systems are data only, built by Tools/Unreal/build_niagara_fx.py;
// what they do and when is decided here and by their callers (the cinder bed, the burst), as with every other effect.

namespace
{
	TAutoConsoleVariable<bool> CVarNiagara(TEXT("Looter.Fx.Niagara"), true,
		TEXT("1: effects rebuilt in Niagara draw that way where their systems are built (the cinder jar's bed and burst); ")
		TEXT("0: the code-drawn look everywhere (to compare). Read as each effect starts."), ECVF_Default);
	TAutoConsoleVariable<float> CVarNiagaraScale(TEXT("Looter.Fx.Niagara.Scale"), 1.f,
		TEXT("Every Niagara effect's spread, times (0.5..2; the sprites keep their size)."), ECVF_Default);

	const TCHAR* Paths[static_cast<int32>(ENiagaraFX::Num)] = {
		TEXT("/Game/Art/Effects/Niagara/NS_CinderBed.NS_CinderBed"),
		TEXT("/Game/Art/Effects/Niagara/NS_CinderBurst.NS_CinderBurst"),
	};
}

bool NiagaraFX::IsOn()
{
	return CVarNiagara.GetValueOnGameThread();
}

float NiagaraFX::ScaleKnob()
{
	return FMath::Clamp(CVarNiagaraScale.GetValueOnGameThread(), 0.5f, 2.f);
}

const TCHAR* NiagaraFX::PathOf(ENiagaraFX Which)
{
	const int32 Index = FMath::Clamp(static_cast<int32>(Which), 0, static_cast<int32>(ENiagaraFX::Num) - 1);
	return Paths[Index];
}

UNiagaraSystem* NiagaraFX::Find(ENiagaraFX Which)
{
	// Looked up when first needed (not in a constructor), so a build before the script's run only falls back to the code
	// look; looked for again while one isn't there, so a run during a session is picked up. /Game/Art/Effects is cooked
	// whole (DefaultGame.ini), so the path reaches a packaged game.
	static TWeakObjectPtr<UNiagaraSystem> Systems[static_cast<int32>(ENiagaraFX::Num)];
	const int32 Index = FMath::Clamp(static_cast<int32>(Which), 0, static_cast<int32>(ENiagaraFX::Num) - 1);
	if (!Systems[Index].IsValid())
	{
		Systems[Index] = LoadObject<UNiagaraSystem>(nullptr, Paths[Index], nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	return Systems[Index].Get();
}

bool NiagaraFX::UseFor(ENiagaraFX Which)
{
	return IsOn() && Find(Which) != nullptr;
}

UNiagaraComponent* NiagaraFX::SpawnAt(UWorld* World, ENiagaraFX Which, const FVector& Location, const FVector& Up, float Scale)
{
	if (!World || !UseFor(Which))
	{
		return nullptr;
	}
	const FVector Normal = Up.IsNearlyZero() ? FVector::UpVector : Up.GetSafeNormal();
	const FRotator Rotation = FRotationMatrix::MakeFromZ(Normal).Rotator();
	const float Size = FMath::Max(Scale, 0.05f) * ScaleKnob();
	// Not pooled (a few a minute at most) and never culled up front: a bed burns 5 s and the player may walk up to it.
	UNiagaraComponent* Component = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, Find(Which), Location, Rotation, FVector(Size),
		true, true, ENCPoolMethod::None, false);
	if (!Component)
	{
		UE_LOG(LogLooter, Verbose, TEXT("Niagara: %s didn't spawn"), PathOf(Which));
	}
	return Component;
}

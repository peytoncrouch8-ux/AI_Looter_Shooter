#include "Weapons/GunfeelSoundTuning.h"
#include "HAL/IConsoleManager.h"

// The Looter.Gunfeel.Sound.* variables (GunfeelSoundTuning.h) and the plain rules of a shot's layers. Each accessor reads
// its variable when asked, never caching it, so a change in play is heard on the next shot.

namespace
{
	namespace Def = GunfeelSound::Default;

	TAutoConsoleVariable<float> CVarTail(TEXT("Looter.Gunfeel.Sound.Tail"), Def::Tail,
		TEXT("The shot's tail (the land answering: open, town or inside) at this volume."), ECVF_Default);
	TAutoConsoleVariable<float> CVarMech(TEXT("Looter.Gunfeel.Sound.Mech"), Def::Mech,
		TEXT("The shooter's own gun's action (bolt, pump, cylinder) at this volume."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLow(TEXT("Looter.Gunfeel.Sound.Low"), Def::Low,
		TEXT("The shooter's own gun's chest thump at this volume."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDistantFrom(TEXT("Looter.Gunfeel.Sound.DistantFrom"), Def::DistantFrom,
		TEXT("Someone else's shot farther than this from the listener (cm) is heard as the far shot."), ECVF_Default);
	TAutoConsoleVariable<float> CVarProbeRange(TEXT("Looter.Gunfeel.Sound.ProbeRange"), Def::ProbeRange,
		TEXT("How far a shot's surroundings probe looks for a roof and walls (cm)."), ECVF_Default);
}

float GunfeelSound::TailVolume()
{
	return FMath::Max(0.f, CVarTail.GetValueOnGameThread());
}

float GunfeelSound::MechVolume()
{
	return FMath::Max(0.f, CVarMech.GetValueOnGameThread());
}

float GunfeelSound::LowVolume()
{
	return FMath::Max(0.f, CVarLow.GetValueOnGameThread());
}

float GunfeelSound::DistantFrom()
{
	return FMath::Max(0.f, CVarDistantFrom.GetValueOnGameThread());
}

float GunfeelSound::ProbeRange()
{
	return FMath::Max(0.f, CVarProbeRange.GetValueOnGameThread());
}

GunfeelSound::ESurroundings GunfeelSound::Classify(bool bRoof, int32 WallSides)
{
	const bool bWalled = WallSides >= 2;
	if (bRoof && bWalled)
	{
		return ESurroundings::Inside;
	}
	return (bRoof || bWalled) ? ESurroundings::Town : ESurroundings::Open;
}

EGunshotLayer GunfeelSound::TailFor(ESurroundings Surroundings)
{
	switch (Surroundings)
	{
	case ESurroundings::Inside: return EGunshotLayer::TailInside;
	case ESurroundings::Town: return EGunshotLayer::TailTown;
	default: return EGunshotLayer::TailOpen;
	}
}

GunfeelSound::FShotLayers GunfeelSound::Choose(bool bOwnGun, float DistanceToListener, float DistantFromCm,
	ESurroundings Surroundings)
{
	FShotLayers Layers;
	Layers.Tail = TailFor(Surroundings);
	// The shooter's own gun is never far from its listener, whatever the camera does.
	if (!bOwnGun && DistanceToListener > DistantFromCm)
	{
		Layers.bDistant = true;
		return Layers;
	}
	Layers.bCore = true;
	Layers.bTail = true;
	Layers.bMech = bOwnGun;
	Layers.bLow = bOwnGun;
	return Layers;
}

bool GunfeelSound::ProbeStale(double Now, double LastProbe, double MaxAge)
{
	return LastProbe < 0.0 || Now - LastProbe >= MaxAge || Now < LastProbe;
}

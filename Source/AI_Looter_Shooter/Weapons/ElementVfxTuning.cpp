#include "Weapons/ElementVfxTuning.h"
#include "HAL/IConsoleManager.h"

// The Looter.Vfx.Element.* cvars (the elements' firing and world looks, 2026-10-29). Their defaults are FElementVfxTuning's own
// field values.

namespace
{
	const FElementVfxTuning ElementVfxDefaults;

	TAutoConsoleVariable<float> CVarElementVfxMuzzle(TEXT("Looter.Vfx.Element.Muzzle"), ElementVfxDefaults.Muzzle,
		TEXT("An element gun's sparks round its flash (Cinder embers, Thunder arcs, Rot gobs, Wardbreak glints, Hallow motes), times (0: none)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarElementVfxMuzzleTint(TEXT("Looter.Vfx.Element.MuzzleTint"), ElementVfxDefaults.MuzzleTint,
		TEXT("How far an element gun's flash leans to its element's colour (0 the plain flash, 1 as designed; read as a gun is built)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarElementVfxTrail(TEXT("Looter.Vfx.Element.Trail"), ElementVfxDefaults.Trail,
		TEXT("The trail behind an element round's tracer (heat and embers, a forked spark line, drips, frost, a light ribbon), times (0: none)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarElementVfxTrailReach(TEXT("Looter.Vfx.Element.TrailReach"), ElementVfxDefaults.TrailReach,
		TEXT("How far along its flight an element round lays its trail (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarElementVfxIdle(TEXT("Looter.Vfx.Element.Idle"), ElementVfxDefaults.Idle,
		TEXT("The held element gun's idle motes, smoke, sparks or drips a second (first person only), times (0: none)."), ECVF_Default);
	TAutoConsoleVariable<bool> CVarElementVfxMarks(TEXT("Looter.Vfx.Element.Marks"), ElementVfxDefaults.bMarks,
		TEXT("An element round leaves its element's mark on the world (scorch, burn ring, acid, salt frost, gilt; needs the atlas with styles 15-19)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarElementVfxMarkFlare(TEXT("Looter.Vfx.Element.MarkFlare"), ElementVfxDefaults.MarkFlare,
		TEXT("The brief ember, spark, drip or glint on a fresh element mark, times (0: none)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarElementVfxDeath(TEXT("Looter.Vfx.Element.Death"), ElementVfxDefaults.Death,
		TEXT("The bigger element deaths (ash and heat, a shock pop, a melt, a shatter, light lifting), times (0: the old deaths alone)."), ECVF_Default);

	float NotBelowZero(const TAutoConsoleVariable<float>& Variable)
	{
		return FMath::Max(Variable.GetValueOnGameThread(), 0.f);
	}
}

FElementVfxTuning FElementVfxTuning::Live()
{
	FElementVfxTuning Tuning;
	Tuning.Muzzle = NotBelowZero(CVarElementVfxMuzzle);
	Tuning.MuzzleTint = FMath::Clamp(CVarElementVfxMuzzleTint.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.Trail = NotBelowZero(CVarElementVfxTrail);
	Tuning.TrailReach = FMath::Clamp(CVarElementVfxTrailReach.GetValueOnGameThread(), 0.f, 20000.f);
	Tuning.Idle = NotBelowZero(CVarElementVfxIdle);
	Tuning.bMarks = CVarElementVfxMarks.GetValueOnGameThread();
	Tuning.MarkFlare = NotBelowZero(CVarElementVfxMarkFlare);
	Tuning.Death = NotBelowZero(CVarElementVfxDeath);
	return Tuning;
}

int32 FElementVfxTuning::TrailEvery(EWeaponKind Kind)
{
	// An SMG's or an LMG's stream of rounds each with a trail is one long smear (and a pool's worth of pieces): every other.
	return Kind == EWeaponKind::SMG || Kind == EWeaponKind::LMG ? 2 : 1;
}

float FElementVfxTuning::IdleRate(ELooterElement Element)
{
	// A few a second each: embers and a curl of smoke off the vents, a crackle, a drip, a breath of frost, a mote.
	switch (Element)
	{
	case ELooterElement::Ember: return 3.f;
	case ELooterElement::Thunder: return 2.5f;
	case ELooterElement::Rot: return 1.5f;
	case ELooterElement::GraveSalt: return 2.f;
	case ELooterElement::SoulLight: return 2.5f;
	default: return 0.f;
	}
}

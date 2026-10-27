#include "Combat/GraveSaltTuning.h"
#include "HAL/IConsoleManager.h"

// The Looter.Grenade.* cvars: their defaults are FGraveSaltTuning's own field values, so the rules' tests (a default-made
// tuning) and a fresh game agree.

namespace
{
	const FGraveSaltTuning SaltDefaults;

	TAutoConsoleVariable<float> CVarDamage(TEXT("Looter.Grenade.Damage"), SaltDefaults.BaseDamage,
		TEXT("The grave-salt burst's damage at its heart at full strength, level 1 (a Common rifle magazine is 600)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRadius(TEXT("Looter.Grenade.Radius"), SaltDefaults.Radius,
		TEXT("The burst's reach (cm): nothing past it. The explosion is sized to it."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCore(TEXT("Looter.Grenade.CoreRadius"), SaltDefaults.CoreRadius,
		TEXT("Full damage within this many cm of the burst."), ECVF_Default);
	TAutoConsoleVariable<float> CVarEdge(TEXT("Looter.Grenade.EdgeShare"), SaltDefaults.EdgeShare,
		TEXT("The share of the damage left at the burst's radius."), ECVF_Default);
	TAutoConsoleVariable<float> CVarUnpaid(TEXT("Looter.Grenade.UnpaidScale"), SaltDefaults.UnpaidScale,
		TEXT("The Unpaid take this many times the burst's damage (salt burns the dead)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBossCap(TEXT("Looter.Grenade.BossCap"), SaltDefaults.BossShareCap,
		TEXT("A boss takes at most this share of its health from one burst."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKnock(TEXT("Looter.Grenade.KnockShare"), SaltDefaults.KnockShare,
		TEXT("Bodies within this share of the radius are knocked away from the burst."), ECVF_Default);
	TAutoConsoleVariable<float> CVarEarly(TEXT("Looter.Grenade.EarlyShare"), SaltDefaults.EarlyShare,
		TEXT("The burst's strength at the player's level 1, as a share of full; it grows to full by Looter.Grenade.FullLevel."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarFullLevel(TEXT("Looter.Grenade.FullLevel"), SaltDefaults.FullStrengthLevel,
		TEXT("The player's level at which the burst has its full strength."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarStarting(TEXT("Looter.Grenade.Starting"), SaltDefaults.StartingGrenades,
		TEXT("Grave-salt jars the first gun brings (at most three are carried)."), ECVF_Default);

	TAutoConsoleVariable<float> CVarLookScale(TEXT("Looter.Grenade.Look.Scale"), SaltDefaults.LookScale,
		TEXT("The burst's explosion size against its radius (1: it fills the radius)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRing(TEXT("Looter.Grenade.Look.Ring"), SaltDefaults.RingGlow,
		TEXT("The burst's shock ring's strength (0: none)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRingLife(TEXT("Looter.Grenade.Look.RingLife"), SaltDefaults.RingLife,
		TEXT("Seconds the shock ring takes to race out to the radius."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRingWidth(TEXT("Looter.Grenade.Look.RingWidth"), SaltDefaults.RingWidth,
		TEXT("The shock ring's thickness as a share of the radius."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSalt(TEXT("Looter.Grenade.Look.Salt"), SaltDefaults.SaltLayer,
		TEXT("The white salt and embers flung on top of the fire (0: none)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarScorch(TEXT("Looter.Grenade.Look.Scorch"), SaltDefaults.ScorchShare,
		TEXT("The scorch left on the ground, its size a share of the radius (0: none)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarGroundReach(TEXT("Looter.Grenade.Look.GroundReach"), SaltDefaults.GroundReach,
		TEXT("A burst this near the ground under it (cm) is drawn on it: ring, debris, scorch."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFlash(TEXT("Looter.Grenade.Look.Flash"), SaltDefaults.FlashCandelas,
		TEXT("The burst's flash of light (candelas, no shadows; 0: none)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFlashRadius(TEXT("Looter.Grenade.Look.FlashRadius"), SaltDefaults.FlashRadius,
		TEXT("How far the burst's flash reaches (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFlashTime(TEXT("Looter.Grenade.Look.FlashTime"), SaltDefaults.FlashSeconds,
		TEXT("Seconds the burst's flash takes to fade."), ECVF_Default);
}

FGraveSaltTuning FGraveSaltTuning::Current()
{
	FGraveSaltTuning Made;
	Made.BaseDamage = FMath::Max(CVarDamage.GetValueOnGameThread(), 0.f);
	// A reach under a metre is a firecracker, and the core never past the reach: the falloff divides by their gap.
	Made.Radius = FMath::Max(CVarRadius.GetValueOnGameThread(), 100.f);
	Made.CoreRadius = FMath::Clamp(CVarCore.GetValueOnGameThread(), 0.f, Made.Radius - 1.f);
	Made.EdgeShare = FMath::Clamp(CVarEdge.GetValueOnGameThread(), 0.f, 1.f);
	Made.UnpaidScale = FMath::Max(CVarUnpaid.GetValueOnGameThread(), 0.f);
	Made.BossShareCap = FMath::Clamp(CVarBossCap.GetValueOnGameThread(), 0.f, 1.f);
	Made.KnockShare = FMath::Clamp(CVarKnock.GetValueOnGameThread(), 0.f, 1.f);
	Made.EarlyShare = FMath::Clamp(CVarEarly.GetValueOnGameThread(), 0.f, 1.f);
	Made.FullStrengthLevel = FMath::Max(CVarFullLevel.GetValueOnGameThread(), 1);
	// Never more than can be carried (FThrowRules::MaxGrenades, 3).
	Made.StartingGrenades = FMath::Clamp(CVarStarting.GetValueOnGameThread(), 0, 3);

	Made.LookScale = FMath::Clamp(CVarLookScale.GetValueOnGameThread(), 0.f, 3.f);
	Made.RingGlow = FMath::Max(CVarRing.GetValueOnGameThread(), 0.f);
	Made.RingLife = FMath::Max(CVarRingLife.GetValueOnGameThread(), 0.05f);
	Made.RingWidth = FMath::Clamp(CVarRingWidth.GetValueOnGameThread(), 0.005f, 0.5f);
	Made.SaltLayer = FMath::Clamp(CVarSalt.GetValueOnGameThread(), 0.f, 2.f);
	Made.ScorchShare = FMath::Max(CVarScorch.GetValueOnGameThread(), 0.f);
	Made.GroundReach = FMath::Max(CVarGroundReach.GetValueOnGameThread(), 0.f);
	Made.FlashCandelas = FMath::Max(CVarFlash.GetValueOnGameThread(), 0.f);
	Made.FlashRadius = FMath::Max(CVarFlashRadius.GetValueOnGameThread(), 100.f);
	Made.FlashSeconds = FMath::Max(CVarFlashTime.GetValueOnGameThread(), 0.02f);
	return Made;
}

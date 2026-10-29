#include "Combat/HitFeelTuning.h"
#include "HAL/IConsoleManager.h"

// The Looter.Hits.* cvars of enemy hit reactions you can feel (HITS, 2026-10-28). Their defaults are FHitFeelTuning's own
// field values, so the rules' tests (a default-made tuning) and a fresh game agree. Each layer has a switch: a morning undo
// is one console line.

namespace
{
	const FHitFeelTuning FeelDefaults;

	// The far boost.
	TAutoConsoleVariable<float> CVarFarBoost(TEXT("Looter.Hits.FarBoost"), FeelDefaults.FarBoost,
		TEXT("A hit from FarFar cm or more flinches a creature this many times as much (x1 within FarNear; 1 = off)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFarNear(TEXT("Looter.Hits.FarNear"), FeelDefaults.FarNear,
		TEXT("The far boost starts past this distance from the shooter (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFarFar(TEXT("Looter.Hits.FarFar"), FeelDefaults.FarFar,
		TEXT("...and is whole from this distance (cm)."), ECVF_Default);

	// Poise and the elites' gap.
	TAutoConsoleVariable<float> CVarPoiseBreak(TEXT("Looter.Hits.PoiseBreak"), FeelDefaults.PoiseBreak,
		TEXT("Poise (hits' shares of health added up) this high, times the rank's scale, staggers a creature (0 = off; 0.35 if packs read stun-locked)."),
		ECVF_Default);
	TAutoConsoleVariable<float> CVarPoiseDrain(TEXT("Looter.Hits.PoiseDrain"), FeelDefaults.PoiseDrain,
		TEXT("Poise drains this much a second."), ECVF_Default);
	TAutoConsoleVariable<float> CVarStaggerGap(TEXT("Looter.Hits.StaggerGap"), FeelDefaults.StaggerGap,
		TEXT("Gravebound and tougher are never staggered again sooner than this after a stagger began (s)."), ECVF_Default);

	// The Big hit.
	TAutoConsoleVariable<float> CVarBigShare(TEXT("Looter.Hits.BigShare"), FeelDefaults.BigShare,
		TEXT("A single hit worth this share of a creature's health is Big (as is a sniper's, a rocket's, a point-blank blast)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBigShove(TEXT("Looter.Hits.BigShove"), FeelDefaults.BigShove,
		TEXT("A Big hit that neither kills nor staggers shoves a small creature back this fast (cm/s; 0 = off)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitStopBig(TEXT("Looter.Hits.HitStopBig"), FeelDefaults.HitStopBig,
		TEXT("A Big hit's hit-stop (s)."), ECVF_Default);

	// The overkill.
	TAutoConsoleVariable<float> CVarOverkillShare(TEXT("Looter.Hits.OverkillShare"), FeelDefaults.OverkillShare,
		TEXT("A killing hit this far past dead (share of max health) is an overkill (so is a Big kill)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitStopOverkill(TEXT("Looter.Hits.HitStopOverkill"), FeelDefaults.HitStopOverkill,
		TEXT("An overkill's hit-stop (s; 0 = none: the slime's over-inflate gets no anticipation frame, still bursts)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarOverkillKnock(TEXT("Looter.Hits.OverkillKnock"), FeelDefaults.OverkillKnock,
		TEXT("An overkill's corpse knock, times the gun's."), ECVF_Default);
	TAutoConsoleVariable<float> CVarOverkillLift(TEXT("Looter.Hits.OverkillLift"), FeelDefaults.OverkillLift,
		TEXT("A shotgun's or a rocket's overkill lifts the corpse this high (cm)."), ECVF_Default);

	// The layers' switches.
	TAutoConsoleVariable<int32> CVarZones(TEXT("Looter.Hits.Zones"), FeelDefaults.Zones ? 1 : 0,
		TEXT("1: hits know where they landed (zone, side, twist). 0: every hit a front body hit."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarWounds(TEXT("Looter.Hits.Wounds"), FeelDefaults.Wounds ? 1 : 0,
		TEXT("1: hurt creatures show it (limps, drips, a guttering coal). 0: off (a perf knob too)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarKillStyles(TEXT("Looter.Hits.KillStyles"), FeelDefaults.KillStyles ? 1 : 0,
		TEXT("1: kills go by the shot (lights out, torn, popped, overkill). 0: every kill as before HITS."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarHat(TEXT("Looter.Hits.Hat"), FeelDefaults.Hat ? 1 : 0,
		TEXT("1: hard head hits and head kills knock the Unpaid's hats off. 0: hats stay on."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarStaggerPose(TEXT("Looter.Hits.StaggerPose"), FeelDefaults.StaggerPose ? 1 : 0,
		TEXT("1: a staggered creature shows it in its pose. 0: off (the brain still pauses)."), ECVF_Default);

	// Each kind's new poses.
	TAutoConsoleVariable<float> CVarSpiderScale(TEXT("Looter.Hits.SpiderScale"), FeelDefaults.SpiderScale,
		TEXT("The spiders' new hit poses (zones, twist, buckle, stagger), times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarUnpaidScale(TEXT("Looter.Hits.UnpaidScale"), FeelDefaults.UnpaidScale,
		TEXT("The Unpaid's new hit poses (zones, whiplash, twist, stagger fold), times."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSlimeScale(TEXT("Looter.Hits.SlimeScale"), FeelDefaults.SlimeScale,
		TEXT("The slime's new hit poses (air knock, squash, ripple, stagger), times."), ECVF_Default);

	float NotBelowZero(const TAutoConsoleVariable<float>& Variable)
	{
		return FMath::Max(Variable.GetValueOnGameThread(), 0.f);
	}

	bool IsOn(const TAutoConsoleVariable<int32>& Variable)
	{
		return Variable.GetValueOnGameThread() != 0;
	}
}

FHitFeelTuning FHitFeelTuning::Live()
{
	FHitFeelTuning Tuning;
	// The boost never shrinks a flinch, and its far end never comes before its near one.
	Tuning.FarBoost = FMath::Max(CVarFarBoost.GetValueOnGameThread(), 1.f);
	Tuning.FarNear = NotBelowZero(CVarFarNear);
	Tuning.FarFar = FMath::Max(CVarFarFar.GetValueOnGameThread(), Tuning.FarNear + 1.f);

	Tuning.PoiseBreak = NotBelowZero(CVarPoiseBreak);
	Tuning.PoiseDrain = NotBelowZero(CVarPoiseDrain);
	Tuning.StaggerGap = NotBelowZero(CVarStaggerGap);

	Tuning.BigShare = FMath::Max(CVarBigShare.GetValueOnGameThread(), 0.01f);
	Tuning.BigShove = NotBelowZero(CVarBigShove);
	Tuning.HitStopBig = NotBelowZero(CVarHitStopBig);

	Tuning.OverkillShare = NotBelowZero(CVarOverkillShare);
	Tuning.HitStopOverkill = NotBelowZero(CVarHitStopOverkill);
	Tuning.OverkillKnock = NotBelowZero(CVarOverkillKnock);
	Tuning.OverkillLift = NotBelowZero(CVarOverkillLift);

	Tuning.Zones = IsOn(CVarZones);
	Tuning.Wounds = IsOn(CVarWounds);
	Tuning.KillStyles = IsOn(CVarKillStyles);
	Tuning.Hat = IsOn(CVarHat);
	Tuning.StaggerPose = IsOn(CVarStaggerPose);

	Tuning.SpiderScale = NotBelowZero(CVarSpiderScale);
	Tuning.UnpaidScale = NotBelowZero(CVarUnpaidScale);
	Tuning.SlimeScale = NotBelowZero(CVarSlimeScale);
	return Tuning;
}

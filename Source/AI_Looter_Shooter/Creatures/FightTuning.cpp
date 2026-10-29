#include "Creatures/FightTuning.h"
#include "HAL/IConsoleManager.h"

// The Looter.Fight.* cvars of how a fight is run (FIGHT-AI). Their defaults are FFightTuning's own field values, so the
// rules' tests (which use a default-made tuning) and a fresh game agree.

namespace
{
	const FFightTuning FightDefaults;

	TAutoConsoleVariable<int32> CVarFightDirector(TEXT("Looter.Fight.Director"), FightDefaults.Director,
		TEXT("1 runs the fight director (attack turns, the surround ring, ranged spots); 0 lets every creature attack whenever it can."), ECVF_Default);

	// Attack turns.
	TAutoConsoleVariable<int32> CVarMeleeTokens(TEXT("Looter.Fight.MeleeTokens"), FightDefaults.MeleeTokens,
		TEXT("Creatures that may be in a melee attack on one target at once."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarRangedTokens(TEXT("Looter.Fight.RangedTokens"), FightDefaults.RangedTokens,
		TEXT("Creatures that may be in a spit or shot at one target at once."), ECVF_Default);
	TAutoConsoleVariable<float> CVarTokenMaxSeconds(TEXT("Looter.Fight.TokenMaxSeconds"), FightDefaults.TokenMaxSeconds,
		TEXT("A turn held this long (s) is taken back."), ECVF_Default);
	TAutoConsoleVariable<float> CVarTokenRest(TEXT("Looter.Fight.TokenRest"), FightDefaults.TokenRest,
		TEXT("A creature that just attacked waits this long (s) before it asks for a turn again."), ECVF_Default);
	TAutoConsoleVariable<float> CVarTurnKeep(TEXT("Looter.Fight.TurnKeep"), FightDefaults.TurnKeep,
		TEXT("One in line that stops asking keeps its place this long (s); a staggered one keeps the head of the line."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRangedGap(TEXT("Looter.Fight.RangedGap"), FightDefaults.RangedGap,
		TEXT("Two ranged turns on one target begin at least this far apart (s)."), ECVF_Default);

	// The ring.
	TAutoConsoleVariable<float> CVarRingRadius(TEXT("Looter.Fight.RingRadius"), FightDefaults.RingRadius,
		TEXT("The surround ring's radius round the target (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRingMargin(TEXT("Looter.Fight.RingMargin"), FightDefaults.RingMargin,
		TEXT("A ring spot is at least this far (cm) past the creature's attack reach."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarRingSlots(TEXT("Looter.Fight.RingSlots"), FightDefaults.RingSlots,
		TEXT("Spots round the ring (3 to 32)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRingRefresh(TEXT("Looter.Fight.RingRefresh"), FightDefaults.RingRefresh,
		TEXT("How often the director looks at each fight (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRingDrift(TEXT("Looter.Fight.RingDrift"), FightDefaults.RingDrift,
		TEXT("The ring turns round the target this fast (degrees a second)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFeintChance(TEXT("Looter.Fight.FeintChance"), FightDefaults.FeintChance,
		TEXT("Each refresh a waiting creature feints in with this chance."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFeintDepth(TEXT("Looter.Fight.FeintDepth"), FightDefaults.FeintDepth,
		TEXT("A feint comes in to this share of its ring radius."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFeintSeconds(TEXT("Looter.Fight.FeintSeconds"), FightDefaults.FeintSeconds,
		TEXT("A feint's step in and back takes this long (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBehindBias(TEXT("Looter.Fight.BehindBias"), FightDefaults.BehindBias,
		TEXT("How much a ring slot in front of the player's view costs against one beside or behind (0 = none)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRingKeep(TEXT("Looter.Fight.RingKeep"), FightDefaults.RingKeep,
		TEXT("A creature's own ring slot costs this much less, so the ring doesn't reshuffle."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarRingMinPack(TEXT("Looter.Fight.RingMinPack"), FightDefaults.RingMinPack,
		TEXT("Fewer melee creatures than this on one target skip the ring."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRingApproach(TEXT("Looter.Fight.RingApproach"), FightDefaults.RingApproach,
		TEXT("Within this many ring radii a melee creature takes its ring spot and asks for its turn."), ECVF_Default);

	// Ranged kinds.
	TAutoConsoleVariable<float> CVarRangedOffAngle(TEXT("Looter.Fight.RangedOffAngle"), FightDefaults.RangedOffAngle,
		TEXT("A ranged spot lies at least this far round the target (degrees) from the melee pack."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRangedRelocate(TEXT("Looter.Fight.RangedRelocate"), FightDefaults.RangedRelocate,
		TEXT("After each shot a ranged creature moves to a new spot with this chance."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRangedBackOff(TEXT("Looter.Fight.RangedBackOff"), FightDefaults.RangedBackOff,
		TEXT("A ranged creature backs off from a player nearer than this (cm; 0 = the creature's own)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRangedMoveShare(TEXT("Looter.Fight.RangedMoveShare"), FightDefaults.RangedMoveShare,
		TEXT("A ranged creature walks to a new spot at this share of its chase speed."), ECVF_Default);

	// Pressure.
	TAutoConsoleVariable<float> CVarMeleeGap(TEXT("Looter.Fight.MeleeGap"), FightDefaults.MeleeGap,
		TEXT("Each kind's own melee gap (the slime's 0.45 s between its pack's leaps at one target), times this; 0 = no gaps."), ECVF_Default);
	TAutoConsoleVariable<float> CVarStillSeconds(TEXT("Looter.Fight.StillSeconds"), FightDefaults.StillSeconds,
		TEXT("A player who keeps within StillRadius this long (s) stands still (camping, with enough fighting them)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarStillRadius(TEXT("Looter.Fight.StillRadius"), FightDefaults.StillRadius,
		TEXT("The circle (cm, flat) a still player keeps within."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHiddenSeconds(TEXT("Looter.Fight.HiddenSeconds"), FightDefaults.HiddenSeconds,
		TEXT("No creature fighting a player has seen them this long (s): they hide (camping, with enough fighting them)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarCampMinFighters(TEXT("Looter.Fight.CampMinFighters"), FightDefaults.CampMinFighters,
		TEXT("Standing still or hiding is camping only while this many or more fight the player."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarEase(TEXT("Looter.Fight.Ease"), FightDefaults.Ease,
		TEXT("1: a nearly dead player gets one turn fewer of each kind at them for a while (EaseBelow, EaseSeconds, EaseGap); 0: never."), ECVF_Default);
	TAutoConsoleVariable<float> CVarEaseBelow(TEXT("Looter.Fight.EaseBelow"), FightDefaults.EaseBelow,
		TEXT("The ease opens under this share of the player's health and ward."), ECVF_Default);
	TAutoConsoleVariable<float> CVarEaseSeconds(TEXT("Looter.Fight.EaseSeconds"), FightDefaults.EaseSeconds,
		TEXT("The ease lasts this long (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarEaseGap(TEXT("Looter.Fight.EaseGap"), FightDefaults.EaseGap,
		TEXT("One ease per player in this long (s)."), ECVF_Default);

	// Pack leaders.
	TAutoConsoleVariable<float> CVarLeaderReach(TEXT("Looter.Fight.LeaderReach"), FightDefaults.LeaderReach,
		TEXT("A pack leader's fall shakes its packmates within this (cm); its pack counts as led this near it."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLeaderFalter(TEXT("Looter.Fight.LeaderFalter"), FightDefaults.LeaderFalter,
		TEXT("Its packmates falter this long (s) at its fall (0 = not at all)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLeaderBreakChance(TEXT("Looter.Fight.LeaderBreakChance"), FightDefaults.LeaderBreakChance,
		TEXT("A Basic spider or slime breaks off at its leader's fall with this chance."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLeaderLull(TEXT("Looter.Fight.LeaderLull"), FightDefaults.LeaderLull,
		TEXT("After its leader's fall a pack rallies and frenzies no more for this long (s)."), ECVF_Default);

	// Difficulty.
	TAutoConsoleVariable<float> CVarChaseScale(TEXT("Looter.Fight.ChaseScale"), FightDefaults.ChaseScale,
		TEXT("Every regular creature chases this many times its own speed (never a boss or Legendary monster)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDamageScale(TEXT("Looter.Fight.DamageScale"), FightDefaults.DamageScale,
		TEXT("Every regular creature's damage, times (bosses and Legendary monsters keep theirs)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRegenDelay(TEXT("Looter.Fight.RegenDelay"), FightDefaults.RegenDelay,
		TEXT("Seconds unhurt before the player's wounds start to close."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRegenRate(TEXT("Looter.Fight.RegenRate"), FightDefaults.RegenRate,
		TEXT("The share of the player's most health healed a second at the ramp's top."), ECVF_Default);

	float NotBelowZero(const TAutoConsoleVariable<float>& Variable)
	{
		return FMath::Max(Variable.GetValueOnGameThread(), 0.f);
	}
}

FFightTuning FFightTuning::Live()
{
	FFightTuning Tuning;
	Tuning.Director = CVarFightDirector.GetValueOnGameThread();

	Tuning.MeleeTokens = FMath::Max(CVarMeleeTokens.GetValueOnGameThread(), 1);
	Tuning.RangedTokens = FMath::Max(CVarRangedTokens.GetValueOnGameThread(), 1);
	Tuning.TokenMaxSeconds = FMath::Max(CVarTokenMaxSeconds.GetValueOnGameThread(), 0.5f);
	Tuning.TokenRest = NotBelowZero(CVarTokenRest);
	Tuning.TurnKeep = FMath::Max(CVarTurnKeep.GetValueOnGameThread(), 0.05f);
	Tuning.RangedGap = NotBelowZero(CVarRangedGap);

	Tuning.RingRadius = FMath::Max(CVarRingRadius.GetValueOnGameThread(), 100.f);
	Tuning.RingMargin = NotBelowZero(CVarRingMargin);
	Tuning.RingSlots = FMath::Clamp(CVarRingSlots.GetValueOnGameThread(), 3, 32);
	Tuning.RingRefresh = FMath::Max(CVarRingRefresh.GetValueOnGameThread(), 0.05f);
	Tuning.RingDrift = CVarRingDrift.GetValueOnGameThread();
	Tuning.FeintChance = FMath::Clamp(CVarFeintChance.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.FeintDepth = FMath::Clamp(CVarFeintDepth.GetValueOnGameThread(), 0.1f, 1.f);
	Tuning.FeintSeconds = FMath::Max(CVarFeintSeconds.GetValueOnGameThread(), 0.1f);
	Tuning.BehindBias = NotBelowZero(CVarBehindBias);
	Tuning.RingKeep = NotBelowZero(CVarRingKeep);
	Tuning.RingMinPack = FMath::Max(CVarRingMinPack.GetValueOnGameThread(), 1);
	Tuning.RingApproach = FMath::Max(CVarRingApproach.GetValueOnGameThread(), 1.f);

	Tuning.RangedOffAngle = FMath::Clamp(CVarRangedOffAngle.GetValueOnGameThread(), 0.f, 150.f);
	Tuning.RangedRelocate = FMath::Clamp(CVarRangedRelocate.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.RangedBackOff = LiveRangedBackOff();
	Tuning.RangedMoveShare = LiveRangedMoveShare();

	Tuning.MeleeGap = NotBelowZero(CVarMeleeGap);
	Tuning.StillSeconds = FMath::Max(CVarStillSeconds.GetValueOnGameThread(), 0.1f);
	Tuning.StillRadius = FMath::Max(CVarStillRadius.GetValueOnGameThread(), 1.f);
	Tuning.HiddenSeconds = FMath::Max(CVarHiddenSeconds.GetValueOnGameThread(), 0.1f);
	Tuning.CampMinFighters = FMath::Max(CVarCampMinFighters.GetValueOnGameThread(), 1);
	Tuning.Ease = CVarEase.GetValueOnGameThread();
	Tuning.EaseBelow = FMath::Clamp(CVarEaseBelow.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.EaseSeconds = NotBelowZero(CVarEaseSeconds);
	Tuning.EaseGap = NotBelowZero(CVarEaseGap);

	Tuning.LeaderReach = NotBelowZero(CVarLeaderReach);
	Tuning.LeaderFalter = NotBelowZero(CVarLeaderFalter);
	Tuning.LeaderBreakChance = FMath::Clamp(CVarLeaderBreakChance.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.LeaderLull = NotBelowZero(CVarLeaderLull);

	Tuning.ChaseScale = LiveChaseScale();
	Tuning.DamageScale = LiveDamageScale();
	Tuning.RegenDelay = NotBelowZero(CVarRegenDelay);
	Tuning.RegenRate = FMath::Clamp(CVarRegenRate.GetValueOnGameThread(), 0.f, 1.f);
	return Tuning;
}

float FFightTuning::LiveChaseScale()
{
	return FMath::Max(CVarChaseScale.GetValueOnGameThread(), 0.1f);
}

float FFightTuning::LiveDamageScale()
{
	return NotBelowZero(CVarDamageScale);
}

float FFightTuning::LiveRangedBackOff()
{
	return NotBelowZero(CVarRangedBackOff);
}

float FFightTuning::LiveRangedMoveShare()
{
	return FMath::Clamp(CVarRangedMoveShare.GetValueOnGameThread(), 0.1f, 1.5f);
}

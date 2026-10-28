#include "Creatures/UnpaidFightTuning.h"
#include "HAL/IConsoleManager.h"

// The Looter.Unpaid.* cvars of the Unpaid's fighting kit. Their defaults are FUnpaidFightTuning's own field values, so the
// tests (a default-made tuning) and a fresh game agree.

namespace
{
	const FUnpaidFightTuning KitDefaults;

	TAutoConsoleVariable<int32> CVarKit(TEXT("Looter.Unpaid.Kit"), KitDefaults.Kit,
		TEXT("1 gives the Unpaid their fighting kit (lead, rake, cinder, flank, rally, howl); 0 is the old shriek and straight lunge."), ECVF_Default);

	TAutoConsoleVariable<float> CVarLeadShare(TEXT("Looter.Unpaid.LeadShare"), KitDefaults.LeadShare,
		TEXT("How much of the target's run an Unpaid's lunge and cinder allow for (0 none, 1 a perfect lead)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLeadMaxSeconds(TEXT("Looter.Unpaid.LeadMaxSeconds"), KitDefaults.LeadMaxSeconds,
		TEXT("An Unpaid never leads its target more than this far ahead (s)."), ECVF_Default);

	TAutoConsoleVariable<float> CVarLungeStart(TEXT("Looter.Unpaid.LungeStart"), KitDefaults.LungeStart,
		TEXT("An Unpaid starts a lunge from this far out (cm at size 1)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLungeDistance(TEXT("Looter.Unpaid.LungeDistance"), KitDefaults.LungeDistance,
		TEXT("An Unpaid's lunge flies at most this far (cm at size 1)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLungeHoming(TEXT("Looter.Unpaid.LungeHoming"), KitDefaults.LungeHoming,
		TEXT("In flight a lunge turns after its target at most this fast (degrees a second); 0 commits its aim at the end of the wind-up."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLungeHitRadius(TEXT("Looter.Unpaid.LungeHitRadius"), KitDefaults.LungeHitRadius,
		TEXT("An Unpaid's lunge lands on a target this near its line (cm at size 1)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLungeDamageShare(TEXT("Looter.Unpaid.LungeDamageShare"), KitDefaults.LungeDamageShare,
		TEXT("A landed lunge hurts this share of the Unpaid's attack."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLungeCommit(TEXT("Looter.Unpaid.LungeCommit"), KitDefaults.LungeCommit,
		TEXT("The last stretch of a lunge's flight (s at full speed) is committed: no hit slow drags it, it flies at full speed to its aim."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitSlowGap(TEXT("Looter.Unpaid.HitSlowGap"), KitDefaults.HitSlowGap,
		TEXT("An Unpaid takes a gun hit's slow at most once every this many seconds (0: every hit)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarPressReach(TEXT("Looter.Unpaid.PressReach"), KitDefaults.PressReach,
		TEXT("Hurt (or its pack hurt) from this near past its ground's edge (cm), an Unpaid fights them there; the one hit phase-steps in. 0: never."), ECVF_Default);
	TAutoConsoleVariable<float> CVarPressSeconds(TEXT("Looter.Unpaid.PressSeconds"), KitDefaults.PressSeconds,
		TEXT("A press past its edge lasts this long once the player is out of its reach (s)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarBlindStep(TEXT("Looter.Unpaid.BlindStep"), KitDefaults.BlindStep,
		TEXT("1: an Unpaid waiting its turn may phase-step when it has stood stalled out of its target's sight, or pressed with them past its edge."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarSight(TEXT("Looter.Unpaid.Sight"), KitDefaults.Sight,
		TEXT("1: an Unpaid starts no melee turn and flies no lunge without a clear line at its target; out of sight it lets its turn go and may step to where it sees them."), ECVF_Default);
	TAutoConsoleVariable<float> CVarPackStepGap(TEXT("Looter.Unpaid.PackStepGap"), KitDefaults.PackStepGap,
		TEXT("At most one Unpaid phase-step or flank at one player every this many seconds (an owed press step first); 0: no gap."), ECVF_Default);
	TAutoConsoleVariable<float> CVarStepJitter(TEXT("Looter.Unpaid.StepJitter"), KitDefaults.StepJitter,
		TEXT("Spawning or noticing a target, an Unpaid's next phase-step waits up to this much longer (s), so a pack doesn't blink as one."), ECVF_Default);

	TAutoConsoleVariable<float> CVarRakeRange(TEXT("Looter.Unpaid.RakeRange"), KitDefaults.RakeRange,
		TEXT("Nearer than this (cm at size 1) an Unpaid may rake instead of lunging."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRakeChance(TEXT("Looter.Unpaid.RakeChance"), KitDefaults.RakeChance,
		TEXT("The chance an Unpaid that near rakes."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRakeWindup(TEXT("Looter.Unpaid.RakeWindup"), KitDefaults.RakeWindup,
		TEXT("The rake's wind-up before the first swipe (s), before the rank's quickening."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRakeGap(TEXT("Looter.Unpaid.RakeGap"), KitDefaults.RakeGap,
		TEXT("The second swipe comes this long (s) after the first."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRakeRecovery(TEXT("Looter.Unpaid.RakeRecovery"), KitDefaults.RakeRecovery,
		TEXT("After the second swipe an Unpaid hangs this long (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRakeRadius(TEXT("Looter.Unpaid.RakeRadius"), KitDefaults.RakeRadius,
		TEXT("A swipe reaches this far (cm at size 1)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRakeArc(TEXT("Looter.Unpaid.RakeArc"), KitDefaults.RakeArc,
		TEXT("A swipe's arc across its front (degrees)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRakeDamageShare(TEXT("Looter.Unpaid.RakeDamageShare"), KitDefaults.RakeDamageShare,
		TEXT("Each swipe hurts this share of the Unpaid's attack."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRakeDrift(TEXT("Looter.Unpaid.RakeDrift"), KitDefaults.RakeDrift,
		TEXT("An Unpaid drifts in this fast (cm/s) while it swipes."), ECVF_Default);

	TAutoConsoleVariable<float> CVarCinderWait(TEXT("Looter.Unpaid.CinderWait"), KitDefaults.CinderWait,
		TEXT("An Unpaid throws a cinder only after waiting on the ring this long (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCinderCooldown(TEXT("Looter.Unpaid.CinderCooldown"), KitDefaults.CinderCooldown,
		TEXT("One Unpaid throws at most once every this many seconds."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCinderGap(TEXT("Looter.Unpaid.CinderGap"), KitDefaults.CinderGap,
		TEXT("Cinders at one player at least this far apart (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCinderTell(TEXT("Looter.Unpaid.CinderTell"), KitDefaults.CinderTell,
		TEXT("The coal swells this long (s) before a cinder is thrown."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCinderSpeed(TEXT("Looter.Unpaid.CinderSpeed"), KitDefaults.CinderSpeed,
		TEXT("A cinder flies this fast (cm/s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCinderRadius(TEXT("Looter.Unpaid.CinderRadius"), KitDefaults.CinderRadius,
		TEXT("A cinder's radius (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCinderDamageShare(TEXT("Looter.Unpaid.CinderDamageShare"), KitDefaults.CinderDamageShare,
		TEXT("A cinder hurts this share of the Unpaid's attack."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCinderMinRange(TEXT("Looter.Unpaid.CinderMinRange"), KitDefaults.CinderMinRange,
		TEXT("An Unpaid throws only at a player at least this far off (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCinderMaxRange(TEXT("Looter.Unpaid.CinderMaxRange"), KitDefaults.CinderMaxRange,
		TEXT("An Unpaid throws only at a player at most this far off (cm)."), ECVF_Default);

	TAutoConsoleVariable<float> CVarFlankChance(TEXT("Looter.Unpaid.FlankChance"), KitDefaults.FlankChance,
		TEXT("Waiting on the ring in front of the player's eyes, an Unpaid's next turn opens with a flank with this chance (once a wait)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFlankFront(TEXT("Looter.Unpaid.FlankFront"), KitDefaults.FlankFront,
		TEXT("In front of the player's eyes: its frontness in their view at least this (0 behind, 1 dead ahead)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFlankMinRange(TEXT("Looter.Unpaid.FlankMinRange"), KitDefaults.FlankMinRange,
		TEXT("An Unpaid flanks a player at least this far off (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFlankMaxRange(TEXT("Looter.Unpaid.FlankMaxRange"), KitDefaults.FlankMaxRange,
		TEXT("An Unpaid flanks a player at most this far off (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFlankRadius(TEXT("Looter.Unpaid.FlankRadius"), KitDefaults.FlankRadius,
		TEXT("A flanking Unpaid comes back this far (cm) from the player, at their side or behind."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFlankCooldown(TEXT("Looter.Unpaid.FlankCooldown"), KitDefaults.FlankCooldown,
		TEXT("One Unpaid flanks at most once every this many seconds."), ECVF_Default);

	TAutoConsoleVariable<int32> CVarRallyMinPack(TEXT("Looter.Unpaid.RallyMinPack"), KitDefaults.RallyMinPack,
		TEXT("At least this many Unpaid hunting one player before one screams the pack on."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRallyWait(TEXT("Looter.Unpaid.RallyWait"), KitDefaults.RallyWait,
		TEXT("An Unpaid screams the pack on only after waiting on the ring this long (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRallyCooldown(TEXT("Looter.Unpaid.RallyCooldown"), KitDefaults.RallyCooldown,
		TEXT("One player hears a rally at most once every this many seconds."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRallyTell(TEXT("Looter.Unpaid.RallyTell"), KitDefaults.RallyTell,
		TEXT("The rally scream lasts this long (s) before the pack comes."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRallyRing(TEXT("Looter.Unpaid.RallyRing"), KitDefaults.RallyRing,
		TEXT("The rally scream's ring of light runs out this far (cm)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFrenzySeconds(TEXT("Looter.Unpaid.FrenzySeconds"), KitDefaults.FrenzySeconds,
		TEXT("After a rally the pack comes on together this long (s)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarFrenzyTokens(TEXT("Looter.Unpaid.FrenzyTokens"), KitDefaults.FrenzyTokens,
		TEXT("Melee turns at the player during a frenzy."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFrenzyChase(TEXT("Looter.Unpaid.FrenzyChase"), KitDefaults.FrenzyChase,
		TEXT("The pack chases this many times faster in a frenzy."), ECVF_Default);

	TAutoConsoleVariable<float> CVarClosingScale(TEXT("Looter.Unpaid.ClosingScale"), KitDefaults.ClosingScale,
		TEXT("Far from its target an Unpaid chases this many times faster."), ECVF_Default);
	TAutoConsoleVariable<float> CVarClosingRange(TEXT("Looter.Unpaid.ClosingRange"), KitDefaults.ClosingRange,
		TEXT("Farther than this (cm) from its target an Unpaid closes in faster."), ECVF_Default);

	TAutoConsoleVariable<float> CVarHowlCooldown(TEXT("Looter.Unpaid.HowlCooldown"), KitDefaults.HowlCooldown,
		TEXT("One player hears an Unpaid's howl at most once every this many seconds."), ECVF_Default);

	TAutoConsoleVariable<float> CVarTellFlare(TEXT("Looter.Unpaid.TellFlare"), KitDefaults.TellFlare,
		TEXT("An Unpaid's coal flares up to this through every wind-up."), ECVF_Default);

	float Positive(const TAutoConsoleVariable<float>& Variable)
	{
		return FMath::Max(Variable.GetValueOnGameThread(), 0.f);
	}

	float Share(const TAutoConsoleVariable<float>& Variable)
	{
		return FMath::Clamp(Variable.GetValueOnGameThread(), 0.f, 1.f);
	}

	/** At least a frame long: a zero wind-up or tell would skip its own warning. */
	float AtLeast(const TAutoConsoleVariable<float>& Variable, float Least)
	{
		return FMath::Max(Variable.GetValueOnGameThread(), Least);
	}
}

bool FUnpaidFightTuning::IsKitOn()
{
	return CVarKit.GetValueOnGameThread() != 0;
}

FUnpaidFightTuning FUnpaidFightTuning::Live()
{
	// Read at each ask (a few plain reads a creature a frame), so a change in PIE, or a test's, counts at once.
	FUnpaidFightTuning Numbers;
	Numbers.Kit = CVarKit.GetValueOnGameThread();
	Numbers.LeadShare = Share(CVarLeadShare);
	Numbers.LeadMaxSeconds = Positive(CVarLeadMaxSeconds);
	Numbers.LungeStart = AtLeast(CVarLungeStart, 100.f);
	Numbers.LungeDistance = Positive(CVarLungeDistance);
	Numbers.LungeHoming = Positive(CVarLungeHoming);
	Numbers.LungeHitRadius = AtLeast(CVarLungeHitRadius, 10.f);
	Numbers.LungeDamageShare = Positive(CVarLungeDamageShare);
	Numbers.LungeCommit = Positive(CVarLungeCommit);
	Numbers.HitSlowGap = Positive(CVarHitSlowGap);
	Numbers.PressReach = Positive(CVarPressReach);
	Numbers.PressSeconds = Positive(CVarPressSeconds);
	Numbers.BlindStep = CVarBlindStep.GetValueOnGameThread();
	Numbers.Sight = CVarSight.GetValueOnGameThread();
	Numbers.PackStepGap = Positive(CVarPackStepGap);
	Numbers.StepJitter = Positive(CVarStepJitter);
	Numbers.RakeRange = Positive(CVarRakeRange);
	Numbers.RakeChance = Share(CVarRakeChance);
	Numbers.RakeWindup = AtLeast(CVarRakeWindup, 0.1f);
	Numbers.RakeGap = AtLeast(CVarRakeGap, 0.05f);
	Numbers.RakeRecovery = AtLeast(CVarRakeRecovery, 0.05f);
	Numbers.RakeRadius = Positive(CVarRakeRadius);
	Numbers.RakeArc = FMath::Clamp(CVarRakeArc.GetValueOnGameThread(), 0.f, 360.f);
	Numbers.RakeDamageShare = Positive(CVarRakeDamageShare);
	Numbers.RakeDrift = Positive(CVarRakeDrift);
	Numbers.CinderWait = Positive(CVarCinderWait);
	Numbers.CinderCooldown = Positive(CVarCinderCooldown);
	Numbers.CinderGap = Positive(CVarCinderGap);
	Numbers.CinderTell = AtLeast(CVarCinderTell, 0.1f);
	Numbers.CinderSpeed = AtLeast(CVarCinderSpeed, 100.f);
	Numbers.CinderRadius = AtLeast(CVarCinderRadius, 1.f);
	Numbers.CinderDamageShare = Positive(CVarCinderDamageShare);
	Numbers.CinderMinRange = Positive(CVarCinderMinRange);
	Numbers.CinderMaxRange = Positive(CVarCinderMaxRange);
	Numbers.FlankChance = Share(CVarFlankChance);
	Numbers.FlankFront = Share(CVarFlankFront);
	Numbers.FlankMinRange = Positive(CVarFlankMinRange);
	Numbers.FlankMaxRange = Positive(CVarFlankMaxRange);
	Numbers.FlankRadius = AtLeast(CVarFlankRadius, 100.f);
	Numbers.FlankCooldown = Positive(CVarFlankCooldown);
	Numbers.RallyMinPack = FMath::Max(CVarRallyMinPack.GetValueOnGameThread(), 2);
	Numbers.RallyWait = Positive(CVarRallyWait);
	Numbers.RallyCooldown = Positive(CVarRallyCooldown);
	Numbers.RallyTell = AtLeast(CVarRallyTell, 0.1f);
	Numbers.RallyRing = AtLeast(CVarRallyRing, 100.f);
	Numbers.FrenzySeconds = Positive(CVarFrenzySeconds);
	Numbers.FrenzyTokens = FMath::Max(CVarFrenzyTokens.GetValueOnGameThread(), 1);
	Numbers.FrenzyChase = AtLeast(CVarFrenzyChase, 0.1f);
	Numbers.ClosingScale = AtLeast(CVarClosingScale, 0.1f);
	Numbers.ClosingRange = Positive(CVarClosingRange);
	Numbers.HowlCooldown = Positive(CVarHowlCooldown);
	Numbers.TellFlare = Positive(CVarTellFlare);
	return Numbers;
}

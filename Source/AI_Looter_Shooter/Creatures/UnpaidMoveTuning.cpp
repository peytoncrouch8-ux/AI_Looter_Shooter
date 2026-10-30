#include "Creatures/UnpaidMoveTuning.h"
#include "HAL/IConsoleManager.h"

// The Looter.Unpaid.* cvars of the Unpaid's varied fight and its failed-lunge shriek (2026-10-29). Their defaults are
// FUnpaidMoveTuning's own field values, so the tests (a default-made tuning) and a fresh game agree.

namespace
{
	const FUnpaidMoveTuning MoveDefaults;

	using FFloatVar = TAutoConsoleVariable<float>;

	TAutoConsoleVariable<int32> CVarVariety(TEXT("Looter.Unpaid.Variety"), MoveDefaults.Variety,
		TEXT("1: the Unpaid pick their moves by range and situation (lunge, rake, feint, phase-strike, low swipe, hang-back) and a missed lunge may turn into a shriek; 0: the kit as it was."), ECVF_Default);

	FFloatVar CVarLungeWeight(TEXT("Looter.Unpaid.Weight.Lunge"), MoveDefaults.LungeWeight, TEXT("The lunge's weight in an Unpaid's pick of its move."), ECVF_Default);
	FFloatVar CVarRakeWeight(TEXT("Looter.Unpaid.Weight.Rake"), MoveDefaults.RakeWeight, TEXT("The rake's weight in an Unpaid's pick."), ECVF_Default);
	FFloatVar CVarFeintWeight(TEXT("Looter.Unpaid.Weight.Feint"), MoveDefaults.FeintWeight, TEXT("The feint's weight in an Unpaid's pick."), ECVF_Default);
	FFloatVar CVarPhaseStrikeWeight(TEXT("Looter.Unpaid.Weight.PhaseStrike"), MoveDefaults.PhaseStrikeWeight, TEXT("The phase-strike's weight in an Unpaid's pick."), ECVF_Default);
	FFloatVar CVarLowSwipeWeight(TEXT("Looter.Unpaid.Weight.LowSwipe"), MoveDefaults.LowSwipeWeight, TEXT("The low swipe's weight in an Unpaid's pick."), ECVF_Default);
	FFloatVar CVarHangBackWeight(TEXT("Looter.Unpaid.Weight.HangBack"), MoveDefaults.HangBackWeight, TEXT("The hang-back's weight in an Unpaid's pick (only while pack-mates fight)."), ECVF_Default);

	FFloatVar CVarRakeCooldown(TEXT("Looter.Unpaid.Cooldown.Rake"), MoveDefaults.RakeCooldown, TEXT("One Unpaid rakes at most once every this many seconds."), ECVF_Default);
	FFloatVar CVarFeintCooldown(TEXT("Looter.Unpaid.Cooldown.Feint"), MoveDefaults.FeintCooldown, TEXT("One Unpaid feints at most once every this many seconds."), ECVF_Default);
	FFloatVar CVarPhaseStrikeCooldown(TEXT("Looter.Unpaid.Cooldown.PhaseStrike"), MoveDefaults.PhaseStrikeCooldown, TEXT("One Unpaid phase-strikes at most once every this many seconds."), ECVF_Default);
	FFloatVar CVarLowSwipeCooldown(TEXT("Looter.Unpaid.Cooldown.LowSwipe"), MoveDefaults.LowSwipeCooldown, TEXT("One Unpaid swipes low at most once every this many seconds."), ECVF_Default);
	FFloatVar CVarHangBackCooldown(TEXT("Looter.Unpaid.Cooldown.HangBack"), MoveDefaults.HangBackCooldown, TEXT("One Unpaid hangs back at most once every this many seconds."), ECVF_Default);

	FFloatVar CVarPairBoost(TEXT("Looter.Unpaid.PairBoost"), MoveDefaults.PairBoost, TEXT("A phase-strike weighs this many times more while a pack-mate comes at the player from the front."), ECVF_Default);
	FFloatVar CVarPressBoost(TEXT("Looter.Unpaid.PressBoost"), MoveDefaults.PressBoost, TEXT("The frontal moves weigh this many times more while a pack-mate is at the player's side or back."), ECVF_Default);
	FFloatVar CVarMissFeintBoost(TEXT("Looter.Unpaid.MissFeintBoost"), MoveDefaults.MissFeintBoost, TEXT("After its lunge missed, a feint weighs this many times more."), ECVF_Default);

	FFloatVar CVarCloseInRange(TEXT("Looter.Unpaid.CloseInRange"), MoveDefaults.CloseInRange, TEXT("A rake or low swipe may be planned from this near (cm); it closes in to its start first."), ECVF_Default);
	FFloatVar CVarCloseInSeconds(TEXT("Looter.Unpaid.CloseInSeconds"), MoveDefaults.CloseInSeconds, TEXT("Closing in longer than this (s), it lunges instead or lets its turn go."), ECVF_Default);
	FFloatVar CVarCloseInPace(TEXT("Looter.Unpaid.CloseInPace"), MoveDefaults.CloseInPace, TEXT("It closes in for a close move this many times its chase."), ECVF_Default);
	FFloatVar CVarCloseInShare(TEXT("Looter.Unpaid.CloseInShare"), MoveDefaults.CloseInShare, TEXT("Planned from beyond its start, a close move weighs this share of its weight."), ECVF_Default);
	FFloatVar CVarLungeMinRange(TEXT("Looter.Unpaid.LungeMinRange"), MoveDefaults.LungeMinRange, TEXT("Nearer than this (cm) an Unpaid doesn't lunge."), ECVF_Default);

	FFloatVar CVarFeintMinRange(TEXT("Looter.Unpaid.Feint.MinRange"), MoveDefaults.FeintMinRange, TEXT("An Unpaid feints only from this far (cm)."), ECVF_Default);
	FFloatVar CVarFeintShare(TEXT("Looter.Unpaid.Feint.Share"), MoveDefaults.FeintShare, TEXT("A feint flies this share of a lunge's way."), ECVF_Default);
	FFloatVar CVarFeintKeep(TEXT("Looter.Unpaid.Feint.Keep"), MoveDefaults.FeintKeep, TEXT("A feint stops no nearer the player than this (cm)."), ECVF_Default);
	FFloatVar CVarFeintSlide(TEXT("Looter.Unpaid.Feint.Slide"), MoveDefaults.FeintSlide, TEXT("A feint slides aside this far (cm)."), ECVF_Default);
	FFloatVar CVarFeintSlideSeconds(TEXT("Looter.Unpaid.Feint.SlideSeconds"), MoveDefaults.FeintSlideSeconds, TEXT("A feint's slide takes this long (s)."), ECVF_Default);

	FFloatVar CVarPhaseStrikeMinRange(TEXT("Looter.Unpaid.PhaseStrike.MinRange"), MoveDefaults.PhaseStrikeMinRange, TEXT("An Unpaid blinks to strike only from this far (cm)."), ECVF_Default);
	FFloatVar CVarPhaseStrikeTell(TEXT("Looter.Unpaid.PhaseStrike.Tell"), MoveDefaults.PhaseStrikeTell, TEXT("Back in, it holds its claw up this long (s) before it strikes."), ECVF_Default);
	FFloatVar CVarPhaseStrikeLand(TEXT("Looter.Unpaid.PhaseStrike.Land"), MoveDefaults.PhaseStrikeLand, TEXT("A phase-strike comes back this far from the player (cm)."), ECVF_Default);
	FFloatVar CVarPhaseStrikeReach(TEXT("Looter.Unpaid.PhaseStrike.Reach"), MoveDefaults.PhaseStrikeReach, TEXT("A phase-strike's claw reaches this far (cm)."), ECVF_Default);
	FFloatVar CVarPhaseStrikeArc(TEXT("Looter.Unpaid.PhaseStrike.Arc"), MoveDefaults.PhaseStrikeArc, TEXT("A phase-strike's claw sweeps this arc (degrees)."), ECVF_Default);
	FFloatVar CVarPhaseStrikeDamageShare(TEXT("Looter.Unpaid.PhaseStrike.DamageShare"), MoveDefaults.PhaseStrikeDamageShare, TEXT("A phase-strike's claw hurts this share of the attack."), ECVF_Default);
	FFloatVar CVarPhaseStrikeRecovery(TEXT("Looter.Unpaid.PhaseStrike.Recovery"), MoveDefaults.PhaseStrikeRecovery, TEXT("After a phase-strike it hangs this long (s)."), ECVF_Default);

	FFloatVar CVarLowSwipeStart(TEXT("Looter.Unpaid.LowSwipe.Start"), MoveDefaults.LowSwipeStart, TEXT("A low swipe skims in from this near (cm)."), ECVF_Default);
	FFloatVar CVarLowSwipeMinRange(TEXT("Looter.Unpaid.LowSwipe.MinRange"), MoveDefaults.LowSwipeMinRange, TEXT("Never a low swipe from nearer than this (cm)."), ECVF_Default);
	FFloatVar CVarLowSwipeWindup(TEXT("Looter.Unpaid.LowSwipe.Windup"), MoveDefaults.LowSwipeWindup, TEXT("A low swipe's wind-up, sunk low (s)."), ECVF_Default);
	FFloatVar CVarLowSwipeSpeed(TEXT("Looter.Unpaid.LowSwipe.Speed"), MoveDefaults.LowSwipeSpeed, TEXT("A low swipe skims this fast (cm/s)."), ECVF_Default);
	FFloatVar CVarLowSwipeDistance(TEXT("Looter.Unpaid.LowSwipe.Distance"), MoveDefaults.LowSwipeDistance, TEXT("A low swipe skims at most this far (cm)."), ECVF_Default);
	FFloatVar CVarLowSwipePast(TEXT("Looter.Unpaid.LowSwipe.Past"), MoveDefaults.LowSwipePast, TEXT("A low swipe skims on this far past the player (cm)."), ECVF_Default);
	FFloatVar CVarLowSwipeRadius(TEXT("Looter.Unpaid.LowSwipe.Radius"), MoveDefaults.LowSwipeRadius, TEXT("A low swipe catches feet this near its line (cm)."), ECVF_Default);
	FFloatVar CVarLowSwipeClear(TEXT("Looter.Unpaid.LowSwipe.Clear"), MoveDefaults.LowSwipeClear, TEXT("Feet this high over the Unpaid's own (cm) pass over a low swipe."), ECVF_Default);
	FFloatVar CVarLowSwipeDamageShare(TEXT("Looter.Unpaid.LowSwipe.DamageShare"), MoveDefaults.LowSwipeDamageShare, TEXT("A landed low swipe hurts this share of the attack."), ECVF_Default);
	FFloatVar CVarLowSwipeRecovery(TEXT("Looter.Unpaid.LowSwipe.Recovery"), MoveDefaults.LowSwipeRecovery, TEXT("After a low swipe it hangs this long (s)."), ECVF_Default);

	FFloatVar CVarHangBackSeconds(TEXT("Looter.Unpaid.HangBack.Seconds"), MoveDefaults.HangBackSeconds, TEXT("An Unpaid hangs back at most this long (s)."), ECVF_Default);
	FFloatVar CVarHangBackMin(TEXT("Looter.Unpaid.HangBack.Min"), MoveDefaults.HangBackMin, TEXT("It hangs back at least this long (s) before a pack-mate's commit ends it."), ECVF_Default);
	FFloatVar CVarHangBackOut(TEXT("Looter.Unpaid.HangBack.Out"), MoveDefaults.HangBackOut, TEXT("Hanging back it circles this far outside the ring (cm)."), ECVF_Default);
	FFloatVar CVarHangBackPace(TEXT("Looter.Unpaid.HangBack.Pace"), MoveDefaults.HangBackPace, TEXT("Hanging back it circles at this share of its chase."), ECVF_Default);

	FFloatVar CVarShriekChance(TEXT("Looter.Unpaid.Shriek.Chance"), MoveDefaults.ShriekChance, TEXT("A missed lunge turns into a shriek this often (0: never)."), ECVF_Default);
	FFloatVar CVarShriekCooldown(TEXT("Looter.Unpaid.Shriek.Cooldown"), MoveDefaults.ShriekCooldown, TEXT("One Unpaid shrieks at most once every this many seconds."), ECVF_Default);
	FFloatVar CVarShriekPackGap(TEXT("Looter.Unpaid.Shriek.PackGap"), MoveDefaults.ShriekPackGap, TEXT("One pack shrieks at one player at most once every this many seconds."), ECVF_Default);
	FFloatVar CVarShriekTell(TEXT("Looter.Unpaid.Shriek.Tell"), MoveDefaults.ShriekTell, TEXT("The shriek's tell (s): shoot its coal now to interrupt it."), ECVF_Default);
	FFloatVar CVarShriekScream(TEXT("Looter.Unpaid.Shriek.Scream"), MoveDefaults.ShriekScream, TEXT("The scream itself (s)."), ECVF_Default);
	FFloatVar CVarShriekRecover(TEXT("Looter.Unpaid.Shriek.Recover"), MoveDefaults.ShriekRecover, TEXT("Hunched and panting after it (s)."), ECVF_Default);
	FFloatVar CVarShriekRattleRadius(TEXT("Looter.Unpaid.Shriek.RattleRadius"), MoveDefaults.ShriekRattleRadius, TEXT("Players this near the shriek (cm) are rattled: aim sway and a slight slow."), ECVF_Default);
	FFloatVar CVarShriekFeltRadius(TEXT("Looter.Unpaid.Shriek.FeltRadius"), MoveDefaults.ShriekFeltRadius, TEXT("Players this near (cm) feel it: the shake falling off, the HUD arc, frost."), ECVF_Default);
	FFloatVar CVarShriekRallyRadius(TEXT("Looter.Unpaid.Shriek.RallyRadius"), MoveDefaults.ShriekRallyRadius, TEXT("Other Unpaid this near a shriek (cm) are rallied."), ECVF_Default);
	FFloatVar CVarShriekRallySeconds(TEXT("Looter.Unpaid.Shriek.RallySeconds"), MoveDefaults.ShriekRallySeconds, TEXT("A shriek's rally lasts this long (s)."), ECVF_Default);
	FFloatVar CVarShriekSlow(TEXT("Looter.Unpaid.Shriek.Slow"), MoveDefaults.ShriekSlow, TEXT("A fully rattled player moves at this share of their speed."), ECVF_Default);
	FFloatVar CVarShriekSlowSeconds(TEXT("Looter.Unpaid.Shriek.SlowSeconds"), MoveDefaults.ShriekSlowSeconds, TEXT("The rattle's slow lasts this long (s)."), ECVF_Default);
	FFloatVar CVarShriekSway(TEXT("Looter.Unpaid.Shriek.Sway"), MoveDefaults.ShriekSway, TEXT("A fully rattled player's aim sways this far (degrees)."), ECVF_Default);
	FFloatVar CVarShriekSwaySeconds(TEXT("Looter.Unpaid.Shriek.SwaySeconds"), MoveDefaults.ShriekSwaySeconds, TEXT("The rattle's sway lasts this long (s)."), ECVF_Default);
	FFloatVar CVarShriekShake(TEXT("Looter.Unpaid.Shriek.Shake"), MoveDefaults.ShriekShake, TEXT("The camera's shake at full rattle (0-1)."), ECVF_Default);
	FFloatVar CVarShriekRattleGap(TEXT("Looter.Unpaid.Shriek.RattleGap"), MoveDefaults.ShriekRattleGap, TEXT("One player is swayed and slowed at most once every this many seconds."), ECVF_Default);
	FFloatVar CVarShriekExposedSeconds(TEXT("Looter.Unpaid.Shriek.ExposedSeconds"), MoveDefaults.ShriekExposedSeconds, TEXT("Its coal shot in the tell, the coal stays exposed this long (s)."), ECVF_Default);
	FFloatVar CVarShriekExposedCoal(TEXT("Looter.Unpaid.Shriek.ExposedCoal"), MoveDefaults.ShriekExposedCoal, TEXT("Exposed, its coal's crit spot is this many times bigger."), ECVF_Default);
	FFloatVar CVarShriekLight(TEXT("Looter.Unpaid.Shriek.Light"), MoveDefaults.ShriekLight, TEXT("The shriek's coal light strength (0: no light)."), ECVF_Default);
	FFloatVar CVarShriekFx(TEXT("Looter.Unpaid.Shriek.Fx"), MoveDefaults.ShriekFx, TEXT("The shriek's particles' share (0: none)."), ECVF_Default);

	float Positive(const FFloatVar& Var) { return FMath::Max(Var.GetValueOnGameThread(), 0.f); }
	float AtLeast(const FFloatVar& Var, float Least) { return FMath::Max(Var.GetValueOnGameThread(), Least); }
	float Share(const FFloatVar& Var) { return FMath::Clamp(Var.GetValueOnGameThread(), 0.f, 1.f); }
}

bool FUnpaidMoveTuning::IsVarietyOn()
{
	return CVarVariety.GetValueOnGameThread() != 0;
}

FUnpaidMoveTuning FUnpaidMoveTuning::Live()
{
	FUnpaidMoveTuning Numbers;
	Numbers.Variety = CVarVariety.GetValueOnGameThread();
	Numbers.LungeWeight = Positive(CVarLungeWeight);
	Numbers.RakeWeight = Positive(CVarRakeWeight);
	Numbers.FeintWeight = Positive(CVarFeintWeight);
	Numbers.PhaseStrikeWeight = Positive(CVarPhaseStrikeWeight);
	Numbers.LowSwipeWeight = Positive(CVarLowSwipeWeight);
	Numbers.HangBackWeight = Positive(CVarHangBackWeight);
	Numbers.RakeCooldown = Positive(CVarRakeCooldown);
	Numbers.FeintCooldown = Positive(CVarFeintCooldown);
	Numbers.PhaseStrikeCooldown = Positive(CVarPhaseStrikeCooldown);
	Numbers.LowSwipeCooldown = Positive(CVarLowSwipeCooldown);
	Numbers.HangBackCooldown = Positive(CVarHangBackCooldown);
	Numbers.PairBoost = Positive(CVarPairBoost);
	Numbers.PressBoost = Positive(CVarPressBoost);
	Numbers.MissFeintBoost = Positive(CVarMissFeintBoost);
	Numbers.CloseInRange = Positive(CVarCloseInRange);
	Numbers.CloseInSeconds = AtLeast(CVarCloseInSeconds, 0.1f);
	Numbers.CloseInPace = AtLeast(CVarCloseInPace, 0.1f);
	Numbers.CloseInShare = Share(CVarCloseInShare);
	Numbers.LungeMinRange = Positive(CVarLungeMinRange);
	Numbers.FeintMinRange = Positive(CVarFeintMinRange);
	Numbers.FeintShare = Share(CVarFeintShare);
	Numbers.FeintKeep = Positive(CVarFeintKeep);
	Numbers.FeintSlide = Positive(CVarFeintSlide);
	Numbers.FeintSlideSeconds = AtLeast(CVarFeintSlideSeconds, 0.05f);
	Numbers.PhaseStrikeMinRange = Positive(CVarPhaseStrikeMinRange);
	Numbers.PhaseStrikeTell = AtLeast(CVarPhaseStrikeTell, 0.05f);
	Numbers.PhaseStrikeLand = AtLeast(CVarPhaseStrikeLand, 60.f);
	Numbers.PhaseStrikeReach = Positive(CVarPhaseStrikeReach);
	Numbers.PhaseStrikeArc = FMath::Clamp(CVarPhaseStrikeArc.GetValueOnGameThread(), 0.f, 360.f);
	Numbers.PhaseStrikeDamageShare = Positive(CVarPhaseStrikeDamageShare);
	Numbers.PhaseStrikeRecovery = AtLeast(CVarPhaseStrikeRecovery, 0.05f);
	Numbers.LowSwipeStart = Positive(CVarLowSwipeStart);
	Numbers.LowSwipeMinRange = Positive(CVarLowSwipeMinRange);
	Numbers.LowSwipeWindup = AtLeast(CVarLowSwipeWindup, 0.1f);
	Numbers.LowSwipeSpeed = AtLeast(CVarLowSwipeSpeed, 100.f);
	Numbers.LowSwipeDistance = Positive(CVarLowSwipeDistance);
	Numbers.LowSwipePast = Positive(CVarLowSwipePast);
	Numbers.LowSwipeRadius = Positive(CVarLowSwipeRadius);
	Numbers.LowSwipeClear = Positive(CVarLowSwipeClear);
	Numbers.LowSwipeDamageShare = Positive(CVarLowSwipeDamageShare);
	Numbers.LowSwipeRecovery = AtLeast(CVarLowSwipeRecovery, 0.05f);
	Numbers.HangBackSeconds = AtLeast(CVarHangBackSeconds, 0.1f);
	Numbers.HangBackMin = Positive(CVarHangBackMin);
	Numbers.HangBackOut = Positive(CVarHangBackOut);
	Numbers.HangBackPace = AtLeast(CVarHangBackPace, 0.05f);
	Numbers.ShriekChance = Share(CVarShriekChance);
	Numbers.ShriekCooldown = Positive(CVarShriekCooldown);
	Numbers.ShriekPackGap = Positive(CVarShriekPackGap);
	Numbers.ShriekTell = AtLeast(CVarShriekTell, 0.1f);
	Numbers.ShriekScream = AtLeast(CVarShriekScream, 0.2f);
	Numbers.ShriekRecover = AtLeast(CVarShriekRecover, 0.05f);
	Numbers.ShriekRattleRadius = Positive(CVarShriekRattleRadius);
	Numbers.ShriekFeltRadius = FMath::Max(CVarShriekFeltRadius.GetValueOnGameThread(), Numbers.ShriekRattleRadius);
	Numbers.ShriekRallyRadius = Positive(CVarShriekRallyRadius);
	Numbers.ShriekRallySeconds = Positive(CVarShriekRallySeconds);
	Numbers.ShriekSlow = Share(CVarShriekSlow);
	Numbers.ShriekSlowSeconds = Positive(CVarShriekSlowSeconds);
	Numbers.ShriekSway = Positive(CVarShriekSway);
	Numbers.ShriekSwaySeconds = AtLeast(CVarShriekSwaySeconds, 0.05f);
	Numbers.ShriekShake = Share(CVarShriekShake);
	Numbers.ShriekRattleGap = Positive(CVarShriekRattleGap);
	Numbers.ShriekExposedSeconds = Positive(CVarShriekExposedSeconds);
	Numbers.ShriekExposedCoal = AtLeast(CVarShriekExposedCoal, 1.f);
	Numbers.ShriekLight = Positive(CVarShriekLight);
	Numbers.ShriekFx = Positive(CVarShriekFx);
	return Numbers;
}

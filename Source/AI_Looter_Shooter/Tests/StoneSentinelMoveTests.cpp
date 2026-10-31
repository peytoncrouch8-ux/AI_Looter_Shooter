#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The Stone Sentinel made a threat (2026-10-31, Docs/Archive/Work/2026-10-31-Creatures/CONTRACT.md): its choice of attack by range and
// cooldowns, the boulder's arc, lead and splash, the shockwave's front, gaps and whom it catches, its health and open window,
// and its poses kept within what its rigid pieces bend to. All plain rules: no level, no model.

#include "Creatures/FightRules.h"
#include "Creatures/StoneSentinelCreature.h"
#include "Creatures/StoneSentinelFX.h"
#include "Creatures/StoneSentinelMoveRules.h"
#include "Creatures/StoneSentinelRules.h"

namespace
{
	FStoneSentinelMoveAsk AskAt(float Distance, bool bShock = true, bool bBoulder = true, bool bClear = true)
	{
		FStoneSentinelMoveAsk Ask;
		Ask.Distance = Distance;
		Ask.bShockReady = bShock;
		Ask.bBoulderReady = bBoulder;
		Ask.bLobClear = bClear;
		return Ask;
	}

	/** The pose stays inside what the Tripo statue's rigid pieces bend to (StoneSentinelPoses.cpp). */
	bool WithinPieces(const FStoneSentinelPose& Pose)
	{
		return Pose.ArmSwing <= 128.5f && Pose.ElbowBend <= 70.5f && Pose.Bow <= 34.5f && FMath::Abs(Pose.Twist) <= 14.5f
			&& Pose.Spread <= 0.43f && Pose.Crouch <= 6.5f;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoneSentinelChoiceTest, "Looter.Creatures.Sentinel.Choice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStoneSentinelChoiceTest::RunTest(const FString& Parameters)
{
	using namespace StoneSentinelMoves;
	const FSentinelShockTuning Shock;
	const FSentinelBoulderTuning Boulder;
	const EStoneSentinelMove Slam = EStoneSentinelMove::Slam;
	const EStoneSentinelMove Wave = EStoneSentinelMove::Shockwave;
	const EStoneSentinelMove Rock = EStoneSentinelMove::Boulder;
	const EStoneSentinelMove Nothing = EStoneSentinelMove::None;

	// Up close: its slam. Hugged, or after two slams, a ready shockwave throws them off instead.
	TestTrue(TEXT("2 m off: the slam"), Choose(AskAt(200.f), Shock, Boulder) == Slam);
	TestTrue(TEXT("Hugged (1.2 m), a shockwave ready: the shockwave"), Choose(AskAt(120.f), Shock, Boulder) == Wave);
	TestTrue(TEXT("...none ready: the slam"), Choose(AskAt(120.f, false), Shock, Boulder) == Slam);
	FStoneSentinelMoveAsk Run = AskAt(200.f);
	Run.Last = Slam;
	Run.Streak = 2;
	TestTrue(TEXT("After two slams: the shockwave"), Choose(Run, Shock, Boulder) == Wave);
	Run.Streak = 1;
	TestTrue(TEXT("...after one: another slam"), Choose(Run, Shock, Boulder) == Slam);

	// Near: the shockwave, or it closes in.
	TestTrue(TEXT("4 m off: the shockwave"), Choose(AskAt(400.f), Shock, Boulder) == Wave);
	TestTrue(TEXT("...not ready: it closes in"), Choose(AskAt(400.f, false), Shock, Boulder) == Nothing);
	FStoneSentinelMoveAsk Ledge = AskAt(400.f);
	Ledge.Rise = 300.f;
	TestTrue(TEXT("4 m off but 3 m up: no shockwave (it runs along its own ground)"), Choose(Ledge, Shock, Boulder) != Wave);

	// Out in the open: the boulder, if the arc to them is clear.
	TestTrue(TEXT("6 m off, the shockwave spent: a boulder"), Choose(AskAt(600.f, false), Shock, Boulder) == Rock);
	TestTrue(TEXT("15 m off: a boulder"), Choose(AskAt(1500.f), Shock, Boulder) == Rock);
	TestTrue(TEXT("...its arc blocked: it closes in"), Choose(AskAt(1500.f, true, true, false), Shock, Boulder) == Nothing);
	TestTrue(TEXT("...not ready: it closes in"), Choose(AskAt(1500.f, true, false), Shock, Boulder) == Nothing);
	TestTrue(TEXT("30 m off: out of its throw"), Choose(AskAt(3000.f), Shock, Boulder) == Nothing);
	FStoneSentinelMoveAsk Roof = AskAt(1500.f);
	Roof.Rise = 1200.f;
	TestTrue(TEXT("15 m off and 12 m up: out of its throw"), Choose(Roof, Shock, Boulder) == Nothing);

	// It presses at every distance: with everything ready, there's an attack from its feet to its throw's end.
	bool bEveryDistance = true;
	for (float Distance = 0.f; Distance <= Boulder.MaxStart; Distance += 25.f)
	{
		bEveryDistance &= Choose(AskAt(Distance), Shock, Boulder) != Nothing;
	}
	TestTrue(TEXT("No distance out to its throw's end without an attack (everything ready)"), bEveryDistance);
	TestTrue(TEXT("No dead zone between the shockwave and the boulder"), Shock.MaxStart >= Boulder.MinStart);

	// How far each may begin, and the turn each takes.
	TestTrue(TEXT("The slam starts from its reach"), FMath::IsNearlyEqual(StartRange(Slam, Shock, Boulder, 1.f), StoneSentinelRules::AttackRange));
	TestTrue(TEXT("The shockwave from 6.5 m"), FMath::IsNearlyEqual(StartRange(Wave, Shock, Boulder, 1.f), Shock.MaxStart));
	TestTrue(TEXT("...farther for a bigger one"), StartRange(Wave, Shock, Boulder, 1.2f) > Shock.MaxStart);
	TestTrue(TEXT("The boulder from 26 m"), FMath::IsNearlyEqual(StartRange(Rock, Shock, Boulder, 1.f), Boulder.MaxStart));
	TestTrue(TEXT("The boulder takes a ranged turn"), TokenRoleFor(Rock) == EFightRole::Ranged);
	TestTrue(TEXT("...the slam and the shockwave melee ones"), TokenRoleFor(Slam) == EFightRole::Melee && TokenRoleFor(Wave) == EFightRole::Melee);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoneSentinelArcTest, "Looter.Creatures.Sentinel.Arc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStoneSentinelArcTest::RunTest(const FString& Parameters)
{
	using namespace StoneSentinelMoves;
	const FSentinelBoulderTuning Boulder;

	// Its flight: quick near, hanging far, never outside those.
	TestTrue(TEXT("Its shortest flight at its nearest throw"), FMath::IsNearlyEqual(FlightSeconds(Boulder.MinStart, Boulder), Boulder.FlightNear));
	TestTrue(TEXT("...its longest at its farthest"), FMath::IsNearlyEqual(FlightSeconds(Boulder.MaxStart, Boulder), Boulder.FlightFar));
	TestTrue(TEXT("...longer the farther"), FlightSeconds(1500.f, Boulder) > FlightSeconds(800.f, Boulder));
	TestTrue(TEXT("...held at both ends"), FMath::IsNearlyEqual(FlightSeconds(50.f, Boulder), Boulder.FlightNear)
		&& FMath::IsNearlyEqual(FlightSeconds(9000.f, Boulder), Boulder.FlightFar));

	// The arc comes down where it was aimed, and high enough to read against the sky.
	const FVector Start(0.0, 0.0, 280.0);
	const FVector Target(1500.0, 300.0, 0.0);
	const float Seconds = FlightSeconds(static_cast<float>(FVector::Dist2D(Start, Target)), Boulder);
	const FVector Velocity = LobVelocity(Start, Target, Seconds, Boulder);
	TestTrue(TEXT("It lands where it was aimed"), FVector::Dist(LobPoint(Start, Velocity, Seconds, Boulder), Target) < 1.0);
	float Apex = static_cast<float>(Start.Z);
	for (float Time = 0.f; Time <= Seconds; Time += 0.02f)
	{
		Apex = FMath::Max(Apex, static_cast<float>(LobPoint(Start, Velocity, Time, Boulder).Z));
	}
	TestTrue(FString::Printf(TEXT("A 15 m throw climbs well over its hands (%.0f cm up)"), Apex - Start.Z), Apex - Start.Z >= 150.f);

	// The lead: at a still target, its feet; a straight run carried on, at most MaxLead ahead.
	const FVector Feet(1500.0, 0.0, 0.0);
	TestTrue(TEXT("A still target: at its feet"), FVector::Dist(AimPoint(Feet, FVector::ZeroVector, 1.3f, Boulder), Feet) < 0.1);
	const FVector Run(0.0, 400.0, 0.0);
	const FVector Led = AimPoint(Feet, Run, 1.3f, Boulder);
	TestTrue(TEXT("A runner: led along their run"), FMath::IsNearlyEqual(static_cast<float>(Led.Y), 400.f * 1.3f * Boulder.LeadShare, 0.5f));
	TestTrue(TEXT("...never up or down"), FMath::IsNearlyZero(static_cast<float>(Led.Z)));
	const FVector Fast = AimPoint(Feet, FVector(0.0, 2000.0, 300.0), 1.6f, Boulder);
	TestTrue(TEXT("...at most MaxLead ahead"), FMath::IsNearlyEqual(static_cast<float>(FVector::Dist2D(Fast, Feet)), Boulder.MaxLead, 0.5f));

	// Dodgeable: a straight run is caught by its burst; turning back at the throw gets clear of it.
	const float Flight = 1.3f;
	const FVector Aim = AimPoint(Feet, Run, Flight, Boulder);
	const FVector RanOn = Feet + Run * Flight;
	const FVector TurnedBack = Feet - Run * Flight;
	TestTrue(TEXT("Running straight on: caught in its burst"), SplashShare(static_cast<float>(FVector::Dist2D(Aim, RanOn)) - 35.f, Boulder) > 0.f);
	TestTrue(TEXT("Turning back as it's thrown: clear"), SplashShare(static_cast<float>(FVector::Dist2D(Aim, TurnedBack)) - 35.f, Boulder) <= 0.f);

	// Its burst: all of it at the middle, EdgeShare at its edge, nothing past.
	TestTrue(TEXT("Full at the middle"), FMath::IsNearlyEqual(SplashShare(0.f, Boulder), 1.f));
	TestTrue(TEXT("EdgeShare at the edge"), FMath::IsNearlyEqual(SplashShare(Boulder.SplashRadius, Boulder), Boulder.EdgeShare, 1.e-4f));
	TestTrue(TEXT("Nothing past it"), FMath::IsNearlyZero(SplashShare(Boulder.SplashRadius + 1.f, Boulder)));
	TestTrue(TEXT("Less the farther out"), SplashShare(50.f, Boulder) > SplashShare(150.f, Boulder));
	TestTrue(TEXT("It hits harder than its slam"), Boulder.Strength > 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoneSentinelShockwaveTest, "Looter.Creatures.Sentinel.Shockwave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStoneSentinelShockwaveTest::RunTest(const FString& Parameters)
{
	using namespace StoneSentinelMoves;
	const FSentinelShockTuning Shock;

	// The front: out from StartRadius at Speed, held at MaxRadius, everything bigger for a bigger one.
	TestTrue(TEXT("It leaves the fists at StartRadius"), FMath::IsNearlyEqual(FrontRadius(0.f, Shock, 1.f), Shock.StartRadius));
	TestTrue(TEXT("...out at Speed"), FMath::IsNearlyEqual(FrontRadius(0.4f, Shock, 1.f), Shock.StartRadius + Shock.Speed * 0.4f));
	TestTrue(TEXT("...stopping at MaxRadius"), FMath::IsNearlyEqual(FrontRadius(10.f, Shock, 1.f), Shock.MaxRadius));
	TestTrue(TEXT("...farther for a bigger one"), FrontRadius(10.f, Shock, 1.2f) > Shock.MaxRadius);
	const float Run = FrontSeconds(Shock);
	TestTrue(FString::Printf(TEXT("Its whole way takes under a second, with a travel to see (%.2f s)"), Run), Run > 0.5f && Run < 1.f);

	// Whom its band crosses.
	TestTrue(TEXT("Reaching them: caught"), FrontCrosses(300.f, 35.f, 200.f, 280.f, Shock.Band));
	TestFalse(TEXT("Not there yet: not"), FrontCrosses(300.f, 35.f, 100.f, 200.f, Shock.Band));
	TestTrue(TEXT("Its band behind its edge still has them"), FrontCrosses(300.f, 35.f, 380.f, 420.f, Shock.Band));
	TestFalse(TEXT("Passed by: not"), FrontCrosses(300.f, 35.f, 450.f, 530.f, Shock.Band));
	TestTrue(TEXT("Hugging it: caught as it leaves"), FrontCrosses(80.f, 35.f, Shock.StartRadius, Shock.StartRadius + 10.f, Shock.Band));

	// Jumped, on a ledge, or in a gap: passed by.
	TestTrue(TEXT("On the ground: caught"), Catches(false, 0.f, Shock));
	TestTrue(TEXT("...on a small step: caught"), Catches(false, 30.f, Shock));
	TestFalse(TEXT("In a jump over it: clear"), Catches(true, 45.f, Shock));
	TestTrue(TEXT("...a jump too late, barely off the ground: caught"), Catches(true, 8.f, Shock));
	TestFalse(TEXT("Up on a ledge: clear"), Catches(false, 220.f, Shock));
	TestFalse(TEXT("Down a step below it: clear"), Catches(false, -220.f, Shock));

	// Its gaps: evenly round, never where its target stood, each GapDegrees wide.
	bool bClearOfTarget = true;
	bool bEven = true;
	for (int32 Seed = 0; Seed < 40; ++Seed)
	{
		const float Bearing = -180.f + 9.f * static_cast<float>(Seed);
		const TArray<float> Gaps = MakeGaps(Bearing, Shock, Seed * 7919);
		bEven &= Gaps.Num() == Shock.Gaps;
		for (int32 Index = 0; Index < Gaps.Num(); ++Index)
		{
			bClearOfTarget &= FMath::Abs(FMath::FindDeltaAngleDegrees(Gaps[Index], Bearing)) >= Shock.GapDegrees * 0.5f + Shock.KeepClear - 0.01f;
			const float Next = Gaps[(Index + 1) % Gaps.Num()];
			bEven &= FMath::IsNearlyEqual(FMath::Abs(FMath::FindDeltaAngleDegrees(Gaps[Index], Next)), 360.f / Shock.Gaps, 0.05f);
		}
		bClearOfTarget &= !IsInGap(Bearing, Gaps, Shock.GapDegrees);
		bClearOfTarget &= Gaps.Num() == 0 || IsInGap(Gaps[0], Gaps, Shock.GapDegrees);
	}
	TestTrue(TEXT("Never a gap where its target stood"), bClearOfTarget);
	TestTrue(TEXT("Its gaps evenly round"), bEven);
	const TArray<float> Gaps = MakeGaps(0.f, Shock, 3);
	TestTrue(TEXT("A step round into a gap gets clear"), Gaps.Num() > 0 && IsInGap(Gaps[0] + Shock.GapDegrees * 0.4f, Gaps, Shock.GapDegrees));

	// Escapable: from 6 m, backing off at a walk through the tell, the front dies out before it reaches them.
	const float Start = 600.f;
	const float Back = 300.f;
	const float AtEnd = Start + Back * (Shock.Windup + Run);
	TestTrue(FString::Printf(TEXT("Backing off from 6 m at 3 m/s: past its end (%.0f cm)"), AtEnd), AtEnd > Shock.MaxRadius);
	TestTrue(TEXT("It hits harder than its slam"), Shock.Strength > 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoneSentinelHealthTest, "Looter.Creatures.Sentinel.Health",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStoneSentinelHealthTest::RunTest(const FString& Parameters)
{
	const FStoneSentinelTuning Tuning;
	// Twice its first health (the user's "harder to kill"): 1080 at level 1, its stone's quarter shrugged off on top.
	TestTrue(TEXT("Twice its first health (540)"), FMath::IsNearlyEqual(StoneSentinelRules::MaxHealth(), 2.f * 540.f));
	const float Guarded = StoneSentinelRules::MaxHealth() / (1.f - StoneSentinelRules::CombinedDefense(Tuning.StoneDefense, 0.f));
	TestTrue(FString::Printf(TEXT("Through its stone it takes about 1440 to break (%.0f)"), Guarded), FMath::IsNearlyEqual(Guarded, 1440.f, 1.f));

	// Open: its stone's guard down, its back and shoulders critical; closed, only its head.
	TestTrue(TEXT("Closed: its stone guards a quarter"), FMath::IsNearlyEqual(StoneSentinelRules::StoneDefenseNow(false, Tuning), Tuning.StoneDefense));
	TestTrue(TEXT("Open: none"), FMath::IsNearlyZero(StoneSentinelRules::StoneDefenseNow(true, Tuning)));
	TestTrue(TEXT("...a Restless one keeps only its tier's"), FMath::IsNearlyEqual(
		StoneSentinelRules::CombinedDefense(StoneSentinelRules::StoneDefenseNow(true, Tuning), 0.1f), 0.1f, 0.001f));
	TestTrue(TEXT("Its head is always critical"), StoneSentinelRules::IsCritBone(FName(TEXT("head")), false, Tuning));
	TestFalse(TEXT("Its back isn't, closed"), StoneSentinelRules::IsCritBone(FName(TEXT("spine_02")), false, Tuning));
	TestTrue(TEXT("...is, open"), StoneSentinelRules::IsCritBone(FName(TEXT("spine_02")), true, Tuning));
	TestFalse(TEXT("Its arms never are"), StoneSentinelRules::IsCritBone(FName(TEXT("lowerarm_l")), true, Tuning));

	// The class: its numbers, its moves' windows open long enough to use.
	const AStoneSentinelCreature* Defaults = GetDefault<AStoneSentinelCreature>();
	TestTrue(TEXT("It's open after a slam"), Defaults->Tuning.ExposedAfterSlam > 0.5f);
	TestTrue(TEXT("...longer after a shockwave"), Defaults->ShockTuning.ExposedSeconds > Defaults->Tuning.ExposedAfterSlam);
	TestTrue(TEXT("...longest after its rock bursts on it"), Defaults->BoulderTuning.CounterExposed > Defaults->ShockTuning.ExposedSeconds);
	TestTrue(TEXT("The rock in its grip breaks in a few shots (3-10% of its health)"),
		Defaults->BoulderTuning.HealthShare >= 0.03f && Defaults->BoulderTuning.HealthShare <= 0.1f);
	TestTrue(TEXT("...and hurts it when it does"), Defaults->BoulderTuning.CounterShare > 0.f);
	TestTrue(TEXT("The rock is in its grip a while before it flies (time to shoot it)"),
		Defaults->BoulderTuning.Windup - Defaults->BoulderTuning.TearAt >= 1.f);
	TestTrue(TEXT("Its steps are felt near it"), Defaults->Tuning.StepShake >= 0.1f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoneSentinelPosesTest, "Looter.Creatures.Sentinel.Poses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStoneSentinelPosesTest::RunTest(const FString& Parameters)
{
	using namespace StoneSentinelMoves;
	const FStoneSentinelTuning Tuning;
	const FSentinelBoulderTuning Boulder;
	const FSentinelShockTuning Shock;
	const FStoneSentinelPose Idle = StoneSentinelRules::IdlePose();

	// The throw: hands to the ground at the tear, the rock overhead, wound back, hurled, standing again.
	const FStoneSentinelPose Tear = ThrowPoseAt(Boulder.TearAt, Boulder);
	const FStoneSentinelPose Lifted = ThrowPoseAt(Boulder.LiftedAt, Boulder);
	const FStoneSentinelPose Drawn = ThrowPoseAt(Boulder.DrawnAt, Boulder);
	const FStoneSentinelPose Thrown = ThrowPoseAt(Boulder.Windup, Boulder);
	const FStoneSentinelPose ThrowEnd = ThrowPoseAt(Boulder.Windup + Boulder.Recovery, Boulder);
	TestTrue(TEXT("Bent to the ground as it tears the rock up"), Tear.ArmSwing <= 50.f && Tear.Bow >= 20.f);
	TestTrue(TEXT("...the rock overhead"), Lifted.ArmSwing >= 120.f);
	TestTrue(TEXT("...wound back, its trunk turned round"), Drawn.Twist >= 10.f && Drawn.Bow < 0.f);
	TestTrue(TEXT("...hurled: arms forward, trunk turned through"), Thrown.ArmSwing < 90.f && Thrown.Twist < 0.f && Thrown.Bow > 10.f);
	TestTrue(TEXT("...standing again"), FMath::IsNearlyEqual(ThrowEnd.ArmSwing, Idle.ArmSwing, 1.f) && FMath::IsNearlyEqual(ThrowEnd.Twist, 0.f, 0.5f));

	// The shockwave: reared back with its arms spread, fists in the ground, held, standing again.
	const float Rise = Shock.Windup - Shock.FallSeconds;
	const FStoneSentinelPose Reared = ShockPoseAt(Rise, Shock);
	const FStoneSentinelPose Pounded = ShockPoseAt(Shock.Windup, Shock);
	const FStoneSentinelPose Held = ShockPoseAt(Shock.Windup + Shock.Recovery * Shock.HoldShare * 0.5f, Shock);
	const FStoneSentinelPose ShockEnd = ShockPoseAt(Shock.Windup + Shock.Recovery, Shock);
	TestTrue(TEXT("Reared back, arms up and spread wide"), Reared.ArmSwing >= 110.f && Reared.Spread >= 0.3f && Reared.Bow < 0.f);
	TestTrue(TEXT("...fists driven into the ground, bowed over them"), Pounded.ArmSwing <= 45.f && Pounded.Bow >= 25.f);
	TestTrue(TEXT("...held there: the window"), FMath::IsNearlyEqual(Held.Bow, Pounded.Bow, 0.5f));
	TestTrue(TEXT("...standing again"), FMath::IsNearlyEqual(ShockEnd.ArmSwing, Idle.ArmSwing, 1.f) && FMath::IsNearlyEqual(ShockEnd.Bow, Idle.Bow, 1.f));
	TestTrue(TEXT("The shockwave's rear reads apart from the slam's heave (arms spread)"),
		Reared.Spread > StoneSentinelRules::SlamPoseAt(Tuning.SlamWindup - Tuning.SlamFallSeconds, Tuning).Spread + 0.2f);

	// Every move inside what its rigid pieces bend to.
	bool bWithin = true;
	for (float Time = 0.f; Time <= 4.f; Time += 0.01f)
	{
		bWithin &= WithinPieces(ThrowPoseAt(Time, Boulder));
		bWithin &= WithinPieces(ShockPoseAt(Time, Shock));
		bWithin &= WithinPieces(StoneSentinelRules::SlamPoseAt(Time, Tuning));
		bWithin &= WithinPieces(ReelPoseAt(Time, Boulder.CounterStun));
		bWithin &= WithinPieces(StoneSentinelRules::WakePoseAt(Time, Tuning.WakeSeconds));
	}
	TestTrue(TEXT("Every move's pose within what its stone pieces bend to"), bWithin);

	// Its waking: still as it cracks, a stretch, then its drop into its stance.
	const FStoneSentinelPose WakeStart = StoneSentinelRules::WakePoseAt(0.f, Tuning.WakeSeconds);
	const FStoneSentinelPose Stretching = StoneSentinelRules::WakePoseAt(Tuning.WakeSeconds * 0.8f, Tuning.WakeSeconds);
	const FStoneSentinelPose Stance = StoneSentinelRules::WakePoseAt(Tuning.WakeSeconds, Tuning.WakeSeconds);
	TestTrue(TEXT("Still as the stone cracks"), FMath::IsNearlyEqual(WakeStart.ArmSwing, Idle.ArmSwing));
	TestTrue(TEXT("...stretching free (arms out, head up)"), Stretching.Spread > 0.2f && Stretching.HeadPitch > 5.f);
	TestTrue(TEXT("...dropped into its stance"), Stance.Crouch > 2.f && Stance.Bow > Idle.Bow);

	// Its stride: each foot planted most of the cycle, hips dropping at each strike, its two sides half a cycle apart.
	const FStoneSentinelGait Strike = StoneSentinelRules::GaitAt(0.f);
	const FStoneSentinelGait Between = StoneSentinelRules::GaitAt(0.25f);
	TestTrue(TEXT("The left foot strikes ahead at 0"), Strike.Thigh[0] > 10.f);
	TestTrue(TEXT("...its hips drop as it does"), Strike.Drop > Between.Drop + 1.f);
	TestTrue(TEXT("...its arm swings back against it"), Strike.Arm[0] < 0.f);
	bool bMirrored = true;
	for (float Phase = 0.f; Phase < 1.f; Phase += 0.05f)
	{
		bMirrored &= FMath::IsNearlyEqual(StoneSentinelRules::GaitAt(Phase).Thigh[0], StoneSentinelRules::GaitAt(Phase + 0.5f).Thigh[1], 0.01f);
	}
	TestTrue(TEXT("Its two sides stride half a cycle apart"), bMirrored);
	TestTrue(TEXT("Its weight rolls over the planted foot (left, then right)"), Between.Roll < 0.f && StoneSentinelRules::GaitAt(0.75f).Roll > 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoneSentinelLensTest, "Looter.Creatures.Sentinel.Lens",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStoneSentinelLensTest::RunTest(const FString& Parameters)
{
	// Its dust never reaches the lens (2026-10-31: a boulder bursting on the player filled the screen with cards, 80 -> 12 fps).
	using namespace StoneSentinelFX;
	TestTrue(TEXT("A puff at the camera crowds it"), CrowdsLens(50.f, 130.f, 0.f));
	TestTrue(TEXT("...one 3.5 m off that grows to 1.3 m crowds it"), CrowdsLens(350.f, 130.f, 0.f));
	TestTrue(TEXT("...one 5 m off that drifts 1.5 m toward it crowds it"), CrowdsLens(500.f, 120.f, 150.f));
	TestFalse(TEXT("A puff 6 m off, 1.3 m across, drifting 1 m, is clear"), CrowdsLens(600.f, 130.f, 100.f));
	TestTrue(TEXT("The budget is small"), PuffsPerSecond <= 30.f && PuffBurst <= 12.f);
	return true;
}

#endif

#include "Player/PlayerCameraMotion.h"
#include "HAL/IConsoleManager.h"

// The camera's movement rules (PlayerCameraMotion.h) and the Looter.Camera.Motion.* variables behind them. Each variable is read
// when Live() is asked, never cached, so a number tuned in play shows on the next frame; Looter.Camera.Motion 0 turns the lot off.

namespace
{
	namespace Def = CameraMotion::Default;

	TAutoConsoleVariable<float> CVarMaster(TEXT("Looter.Camera.Motion"), Def::Master,
		TEXT("The camera answering the body's movement (landing dip, jump kick, footfall nod, sprint sway, strafe lean): 1 on, 0 off ")
		TEXT("(today's dead camera). The Camera shake setting scales all of it."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLandMinSpeed(TEXT("Looter.Camera.Motion.LandMinSpeed"), Def::LandMinSpeed,
		TEXT("Fall speed (cm/s) under which a landing moves nothing (a step's drop)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLandFullSpeed(TEXT("Looter.Camera.Motion.LandFullSpeed"), Def::LandFullSpeed,
		TEXT("Fall speed (cm/s) from which a landing dips the view and the eye as far as they go."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLandPitchMin(TEXT("Looter.Camera.Motion.LandPitchMin"), Def::LandPitchMin,
		TEXT("The view's dip (degrees) at the smallest felt landing."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLandPitchMax(TEXT("Looter.Camera.Motion.LandPitchMax"), Def::LandPitchMax,
		TEXT("The view's dip (degrees) at a hard landing."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLandDropMin(TEXT("Looter.Camera.Motion.LandDropMin"), Def::LandDropMin,
		TEXT("The eye's drop (cm) at the smallest felt landing."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLandDropMax(TEXT("Looter.Camera.Motion.LandDropMax"), Def::LandDropMax,
		TEXT("The eye's drop (cm) at a hard landing."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLandHertz(TEXT("Looter.Camera.Motion.LandHertz"), Def::LandHertz,
		TEXT("How quick the landing's dip of the view is (spring Hz)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLandDamping(TEXT("Looter.Camera.Motion.LandDamping"), Def::LandDamping,
		TEXT("How soon the landing's dip settles (0-1)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLandDropSeconds(TEXT("Looter.Camera.Motion.LandDropSeconds"), Def::LandDropSeconds,
		TEXT("Seconds after a landing that the eye is lowest (it is back in about eight times that)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarJumpPitch(TEXT("Looter.Camera.Motion.JumpPitch"), Def::JumpPitch,
		TEXT("The view's kick up (degrees) as a jump leaves the ground."), ECVF_Default);
	TAutoConsoleVariable<float> CVarJumpHertz(TEXT("Looter.Camera.Motion.JumpHertz"), Def::JumpHertz,
		TEXT("How quick the jump's kick is (spring Hz)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarJumpDamping(TEXT("Looter.Camera.Motion.JumpDamping"), Def::JumpDamping,
		TEXT("How soon the jump's kick settles (0-1)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarStepBobWalk(TEXT("Looter.Camera.Motion.StepBobWalk"), Def::StepBobWalk,
		TEXT("The view's nod (degrees) at each footfall of a walk."), ECVF_Default);
	TAutoConsoleVariable<float> CVarStepBobSprint(TEXT("Looter.Camera.Motion.StepBobSprint"), Def::StepBobSprint,
		TEXT("The view's nod (degrees) at each footfall of a full sprint."), ECVF_Default);
	TAutoConsoleVariable<float> CVarStepHertz(TEXT("Looter.Camera.Motion.StepHertz"), Def::StepHertz,
		TEXT("How quick the footfall's nod is (spring Hz); it should settle before the next step."), ECVF_Default);
	TAutoConsoleVariable<float> CVarStepDamping(TEXT("Looter.Camera.Motion.StepDamping"), Def::StepDamping,
		TEXT("How soon the footfall's nod settles (0-1)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSprintSway(TEXT("Looter.Camera.Motion.SprintSway"), Def::SprintSway,
		TEXT("The view's roll (degrees) toward the stepping foot's side at each footfall of a full sprint."), ECVF_Default);
	TAutoConsoleVariable<float> CVarStrafeRoll(TEXT("Looter.Camera.Motion.StrafeRoll"), Def::StrafeRoll,
		TEXT("The view's lean (degrees) into a strafe at the walk's speed (negative leans out of it)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarStrafeRate(TEXT("Looter.Camera.Motion.StrafeRate"), Def::StrafeRate,
		TEXT("How fast the lean comes and goes (1/s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarMantleDip(TEXT("Looter.Camera.Motion.MantleDip"), Def::MantleDip,
		TEXT("The view's dip (degrees) as a mantle ends on the ledge."), ECVF_Default);
	TAutoConsoleVariable<float> CVarVaultDip(TEXT("Looter.Camera.Motion.VaultDip"), Def::VaultDip,
		TEXT("The view's dip (degrees) as a vault lands on the far side."), ECVF_Default);
	TAutoConsoleVariable<float> CVarAimedShare(TEXT("Looter.Camera.Motion.AimedShare"), Def::AimedShare,
		TEXT("The share of all of it that shows through the sights (1: all, 0: none), so the reticle holds."), ECVF_Default);

	/** A dip is over after this many of its attack times. */
	constexpr float DipLifeAttacks = 8.f;
	/** A footfall's kick is given at most this share of a step early, whatever the speed. */
	constexpr float MaxLeadSteps = 0.45f;
	/** A spring is never quicker or less damped than the view kick stack's own (FViewKickStack) limits. */
	constexpr float MinHertz = 1.f;
	constexpr float MinDamping = 0.2f;
	constexpr float MaxDamping = 0.95f;

	FViewKick Spring(float Pitch, float Roll, float Hertz, float Damping)
	{
		FViewKick Kick;
		Kick.Pitch = Pitch;
		Kick.Roll = Roll;
		Kick.Frequency = Hertz;
		Kick.Damping = Damping;
		return Kick;
	}

	/** How far through the felt landings a fall is: 0 at the smallest, 1 at the hardest. */
	float LandingShare(float FallSpeed, const CameraMotion::FTuning& Tuning)
	{
		return FMath::Clamp((FallSpeed - Tuning.LandMinSpeed) / FMath::Max(Tuning.LandFullSpeed - Tuning.LandMinSpeed, 1.f), 0.f, 1.f);
	}
}

CameraMotion::FTuning CameraMotion::Live()
{
	FTuning Tuning;
	Tuning.Master = CVarMaster.GetValueOnAnyThread();
	Tuning.LandMinSpeed = CVarLandMinSpeed.GetValueOnAnyThread();
	Tuning.LandFullSpeed = CVarLandFullSpeed.GetValueOnAnyThread();
	Tuning.LandPitchMin = CVarLandPitchMin.GetValueOnAnyThread();
	Tuning.LandPitchMax = CVarLandPitchMax.GetValueOnAnyThread();
	Tuning.LandDropMin = CVarLandDropMin.GetValueOnAnyThread();
	Tuning.LandDropMax = CVarLandDropMax.GetValueOnAnyThread();
	Tuning.LandHertz = CVarLandHertz.GetValueOnAnyThread();
	Tuning.LandDamping = CVarLandDamping.GetValueOnAnyThread();
	Tuning.LandDropSeconds = CVarLandDropSeconds.GetValueOnAnyThread();
	Tuning.JumpPitch = CVarJumpPitch.GetValueOnAnyThread();
	Tuning.JumpHertz = CVarJumpHertz.GetValueOnAnyThread();
	Tuning.JumpDamping = CVarJumpDamping.GetValueOnAnyThread();
	Tuning.StepBobWalk = CVarStepBobWalk.GetValueOnAnyThread();
	Tuning.StepBobSprint = CVarStepBobSprint.GetValueOnAnyThread();
	Tuning.StepHertz = CVarStepHertz.GetValueOnAnyThread();
	Tuning.StepDamping = CVarStepDamping.GetValueOnAnyThread();
	Tuning.SprintSway = CVarSprintSway.GetValueOnAnyThread();
	Tuning.StrafeRoll = CVarStrafeRoll.GetValueOnAnyThread();
	Tuning.StrafeRate = CVarStrafeRate.GetValueOnAnyThread();
	Tuning.MantleDip = CVarMantleDip.GetValueOnAnyThread();
	Tuning.VaultDip = CVarVaultDip.GetValueOnAnyThread();
	Tuning.AimedShare = CVarAimedShare.GetValueOnAnyThread();
	return Tuning;
}

bool CameraMotion::LandingFelt(float FallSpeed, const FTuning& Tuning)
{
	return Tuning.Master > 0.f && FallSpeed >= Tuning.LandMinSpeed;
}

float CameraMotion::LandingPitch(float FallSpeed, const FTuning& Tuning)
{
	return LandingFelt(FallSpeed, Tuning) ? FMath::Lerp(Tuning.LandPitchMin, Tuning.LandPitchMax, LandingShare(FallSpeed, Tuning)) : 0.f;
}

float CameraMotion::LandingDrop(float FallSpeed, const FTuning& Tuning)
{
	return LandingFelt(FallSpeed, Tuning) ? FMath::Lerp(Tuning.LandDropMin, Tuning.LandDropMax, LandingShare(FallSpeed, Tuning)) : 0.f;
}

FViewKick CameraMotion::LandingKick(float FallSpeed, const FTuning& Tuning)
{
	// Down is negative pitch. The spring's first swing is the dip, its small swing past zero the view coming back up.
	return Spring(-LandingPitch(FallSpeed, Tuning), 0.f, Tuning.LandHertz, Tuning.LandDamping);
}

FViewKick CameraMotion::JumpKick(const FTuning& Tuning)
{
	return Tuning.Master > 0.f ? Spring(Tuning.JumpPitch, 0.f, Tuning.JumpHertz, Tuning.JumpDamping) : FViewKick();
}

FViewKick CameraMotion::TraversalKick(bool bVault, const FTuning& Tuning)
{
	return Tuning.Master > 0.f ? Spring(-(bVault ? Tuning.VaultDip : Tuning.MantleDip), 0.f, Tuning.LandHertz, Tuning.LandDamping) : FViewKick();
}

FViewKick CameraMotion::StepKick(float SprintAlpha, float MoveWeight, float Side, const FTuning& Tuning)
{
	if (Tuning.Master <= 0.f)
	{
		return FViewKick();
	}
	const float Sprint = FMath::Clamp(SprintAlpha, 0.f, 1.f);
	// The view model's weight runs past 1 in a sprint; a nod is the walk's at most, the sprint's share comes from the sprint.
	const float Moving = FMath::Clamp(MoveWeight, 0.f, 1.f);
	return Spring(-FMath::Lerp(Tuning.StepBobWalk, Tuning.StepBobSprint, Sprint) * Moving,
		FMath::Clamp(Side, -1.f, 1.f) * Tuning.SprintSway * Sprint * Moving, Tuning.StepHertz, Tuning.StepDamping);
}

float CameraMotion::StrafeTarget(float LateralSpeed, float WalkSpeed, const FTuning& Tuning)
{
	if (Tuning.Master <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(LateralSpeed / FMath::Max(WalkSpeed, 1.f), -1.f, 1.f) * Tuning.StrafeRoll;
}

float CameraMotion::AimedShare(float AimAlpha, const FTuning& Tuning)
{
	return FMath::Lerp(1.f, FMath::Clamp(Tuning.AimedShare, 0.f, 1.f), FMath::Clamp(AimAlpha, 0.f, 1.f));
}

float CameraMotion::PeakSeconds(float Hertz, float Damping)
{
	// A spring given a push swings as e^(-z w t) sin(wd t): its first peak is where the slope is zero, at atan(wd / (z w)) / wd
	// (the same clamps as FViewKickStack::Response, whose kicks these are).
	const float Zeta = FMath::Clamp(Damping, MinDamping, MaxDamping);
	const float Omega = 2.f * UE_PI * FMath::Max(Hertz, MinHertz);
	const float Ringing = Omega * FMath::Sqrt(1.f - Zeta * Zeta);
	return FMath::Atan2(Ringing, Zeta * Omega) / Ringing;
}

float CameraMotion::DipShape(float Age, float AttackSeconds)
{
	const float U = Age / FMath::Max(AttackSeconds, UE_KINDA_SMALL_NUMBER);
	if (U <= 0.f || U >= DipLifeAttacks)
	{
		return 0.f;
	}
	// u^2 e^(2 (1 - u)): starts at rest (no snap, however the frames fall), peaks at 1 when u is 1, then eases back without
	// swinging past the rest height (the eye is pushed down by the landing and comes back, it does not bounce up).
	return U * U * FMath::Exp(2.f * (1.f - U));
}

// ---------------------------------------------------------------------------
// The state
// ---------------------------------------------------------------------------

void FCameraMotion::Reset()
{
	Dips.Reset();
	Lean = 0.f;
	LastPhase = -1.f;
}

void FCameraMotion::AddEyeDip(float Depth, float AttackSeconds)
{
	if (Depth <= 0.f)
	{
		return;
	}
	if (Dips.Num() >= CameraMotion::Default::MaxEyeDips)
	{
		// The oldest is on its way back anyway.
		Dips.RemoveAt(0, 1, EAllowShrinking::No);
	}
	FDip& Added = Dips.AddDefaulted_GetRef();
	Added.Depth = Depth;
	Added.Attack = FMath::Max(AttackSeconds, UE_KINDA_SMALL_NUMBER);
}

void FCameraMotion::Advance(float DeltaSeconds)
{
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	for (int32 Index = Dips.Num() - 1; Index >= 0; --Index)
	{
		Dips[Index].Age += Dt;
		if (Dips[Index].Age >= Dips[Index].Attack * DipLifeAttacks)
		{
			Dips.RemoveAt(Index, 1, EAllowShrinking::No);
		}
	}
}

float FCameraMotion::GetEyeDrop() const
{
	float Sum = 0.f;
	for (const FDip& Each : Dips)
	{
		Sum += Each.Depth * CameraMotion::DipShape(Each.Age, Each.Attack);
	}
	return FMath::Min(Sum, CameraMotion::Default::MaxEyeDrop);
}

float FCameraMotion::EaseLean(float Target, float Rate, float DeltaSeconds)
{
	Lean = FMath::FInterpTo(Lean, Target, FMath::Clamp(DeltaSeconds, 0.f, 0.1f), Rate);
	return Lean;
}

bool FCameraMotion::StepFootfall(float StepPhase, float DeltaSeconds, float PeakSeconds, float& OutSide)
{
	const float Last = LastPhase;
	LastPhase = StepPhase;
	if (Last < 0.f || DeltaSeconds <= 0.f)
	{
		return false;
	}
	// The clock moved on this far, in steps (a step is pi of it), the wrap at 2 pi undone. Standing still moves it by nothing; a
	// move of a whole step in one frame is a jump of the clock (a teleport), not walking.
	float Moved = StepPhase - Last;
	if (Moved < 0.f)
	{
		Moved += 2.f * UE_PI;
	}
	const float StepsMoved = Moved / UE_PI;
	if (StepsMoved <= 0.f || StepsMoved >= 1.f)
	{
		return false;
	}
	// The gun's lowest dip is half a step in (sin^2 of the clock peaks at pi/2). The nod's spring takes PeakSeconds to reach its
	// peak, so it is kicked that much of a step early, by the pace the clock is going at now.
	const float Lead = FMath::Min(StepsMoved / DeltaSeconds * FMath::Max(PeakSeconds, 0.f), MaxLeadSteps);
	const float From = Last / UE_PI - (0.5f - Lead);
	const int32 Crossed = FMath::FloorToInt(From + StepsMoved);
	if (Crossed <= FMath::FloorToInt(From))
	{
		return false;
	}
	// Even-numbered dips are on the right (the gun's sway, sin of the clock, is positive then), odd on the left.
	OutSide = (Crossed & 1) ? -1.f : 1.f;
	return true;
}

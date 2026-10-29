#include "Player/ViewKick.h"
#include "Weapons/GunfeelKickTuning.h"

namespace
{
	/** Every spring is at least this quick and this damped, so a bad number can't make one ring on and on. */
	constexpr float MinFrequency = 1.f;
	constexpr float MinDamping = 0.2f;
	constexpr float MaxDamping = 0.95f;

	float ClampedDamping(float Damping)
	{
		return FMath::Clamp(Damping, MinDamping, MaxDamping);
	}

	/** Seconds after which a spring's swing stays under SettledShare of its peak. */
	float SettleSeconds(float Frequency, float Damping)
	{
		const float Decay = ClampedDamping(Damping) * 2.f * UE_PI * FMath::Max(Frequency, MinFrequency);
		return -FMath::Loge(FViewKickStack::SettledShare) / Decay;
	}

	/** The share of a kick kept through the sight: 1 at the hip, Share at AimAlpha 1. */
	float SightShare(float AimAlpha, float Share)
	{
		return FMath::Lerp(1.f, FMath::Max(Share, 0.f), FMath::Clamp(AimAlpha, 0.f, 1.f));
	}
}

FViewKick FViewKick::operator*(float Scale) const
{
	FViewKick Scaled = *this;
	Scaled.Pitch *= Scale;
	Scaled.Yaw *= Scale;
	Scaled.Roll *= Scale;
	Scaled.FieldOfView *= Scale;
	Scaled.Push *= Scale;
	return Scaled;
}

bool FViewKick::IsNone() const
{
	return FMath::IsNearlyZero(Pitch) && FMath::IsNearlyZero(Yaw) && FMath::IsNearlyZero(Roll) && FMath::IsNearlyZero(FieldOfView)
		&& FMath::IsNearlyZero(Push);
}

float FViewKickStack::Response(float Time, float Frequency, float Damping)
{
	if (Time <= 0.f)
	{
		return 0.f;
	}
	// x(t) = e^(-z w t) sin(wd t), the motion of a spring at rest given a push, divided by its first peak (at
	// t* = atan(wd / (z w)) / wd), so a kick's numbers are what the view reaches.
	const float Zeta = ClampedDamping(Damping);
	const float Omega = 2.f * UE_PI * FMath::Max(Frequency, MinFrequency);
	const float Decay = Zeta * Omega;
	const float Ringing = Omega * FMath::Sqrt(1.f - Zeta * Zeta);
	const float PeakTime = FMath::Atan2(Ringing, Decay) / Ringing;
	const float Peak = FMath::Exp(-Decay * PeakTime) * FMath::Sin(Ringing * PeakTime);
	return FMath::Exp(-Decay * Time) * FMath::Sin(Ringing * Time) / Peak;
}

float FViewKickStack::Swing(const FActive& Each)
{
	return Response(Each.Age - FMath::Max(Each.Kick.Delay, 0.f), Each.Kick.Frequency, Each.Kick.Damping);
}

float FViewKickStack::GetShotPitch() const
{
	float Sum = 0.f;
	for (const FActive& Each : Active)
	{
		if (Each.Kick.SustainCap > 0.f)
		{
			Sum += Each.Kick.Pitch * FMath::Max(Swing(Each), 0.f);
		}
	}
	return Sum;
}

void FViewKickStack::Add(const FViewKick& Kick)
{
	if (Active.Num() >= MaxKicks)
	{
		// The oldest has nearly settled anyway.
		Active.RemoveAt(0, 1, EAllowShrinking::No);
	}
	FViewKick Use = Kick;
	if (Kick.SustainCap > 0.f && Kick.Pitch > 0.f)
	{
		// An automatic's shots pile up on each other: what room is left under the cap (the cap itself, or the kick's own size
		// if that is bigger, so a lone heavy shot is never trimmed) is all the next one gets, but never less than the floor.
		const float Room = FMath::Max(Kick.SustainCap, Kick.Pitch) - GetShotPitch();
		const float Share = FMath::Clamp(Room / Kick.Pitch, FMath::Clamp(Kick.SustainFloor, 0.f, 1.f), 1.f);
		Use = Kick * Share;
	}
	Active.Add({ Use, 0.f });
}

void FViewKickStack::Tick(float DeltaSeconds)
{
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	for (int32 Index = Active.Num() - 1; Index >= 0; --Index)
	{
		FActive& Each = Active[Index];
		Each.Age += Dt;
		if (Each.Age >= FMath::Max(Each.Kick.Delay, 0.f) + SettleSeconds(Each.Kick.Frequency, Each.Kick.Damping))
		{
			Active.RemoveAt(Index, 1, EAllowShrinking::No);
		}
	}
}

FRotator FViewKickStack::GetRotation() const
{
	FRotator Sum = FRotator::ZeroRotator;
	for (const FActive& Each : Active)
	{
		Sum += FRotator(Each.Kick.Pitch, Each.Kick.Yaw, Each.Kick.Roll) * Swing(Each);
	}
	return FRotator(FMath::Clamp(Sum.Pitch, -MaxTurn, MaxTurn), FMath::Clamp(Sum.Yaw, -MaxTurn * 0.67f, MaxTurn * 0.67f),
		FMath::Clamp(Sum.Roll, -MaxTurn, MaxTurn));
}

float FViewKickStack::GetFieldOfView() const
{
	float Sum = 0.f;
	for (const FActive& Each : Active)
	{
		Sum += Each.Kick.FieldOfView * Swing(Each);
	}
	return FMath::Clamp(Sum, -MaxFieldOfView, MaxFieldOfView);
}

float FViewKickStack::GetPush() const
{
	float Sum = 0.f;
	for (const FActive& Each : Active)
	{
		Sum += Each.Kick.Push * Swing(Each);
	}
	return FMath::Clamp(Sum, -MaxPush, MaxPush);
}

FShotKickShape FShotKickShape::For(EWeaponKind Kind)
{
	const GunfeelKick::FKindFeel Feel = GunfeelKick::KindFeelDefault(Kind);
	FShotKickShape Shape;
	Shape.AimedTurn = Feel.AimedTurn;
	Shape.AimedFieldOfView = Feel.AimedFieldOfView;
	return Shape;
}

FViewKick ViewKicks::ForShot(EWeaponKind Kind, float RecoilStat, float AimAlpha, FRandomStream& Random)
{
	return ForShot(Kind, RecoilStat, AimAlpha, Random, FShotKickShape::For(Kind));
}

FViewKick ViewKicks::ForShot(EWeaponKind Kind, float RecoilStat, float AimAlpha, FRandomStream& Random, const FShotKickShape& Shape)
{
	// Each kind's peak at a base gun (Recoil 1.4), all inside the stack's 3 degrees, 5% and 4 cm, so no kind clips and the
	// heavy three read apart by their shape: the shotgun's lift and push, the sniper's long settle, the launcher's width.
	FViewKick Kick;
	// The draws in today's order and count, whatever the kind uses of them.
	const float Side = Random.FRandRange(-1.f, 1.f);
	const float Lean = Random.RandRange(0, 1) == 0 ? -1.f : 1.f;
	switch (Kind)
	{
	case EWeaponKind::SMG:
		// A hornet: a light buzz, quick to the eye, so 900 rounds a minute read as a hum.
		Kick.Pitch = 0.2f;
		Kick.Roll = 0.12f * Lean;
		Kick.Yaw = 0.07f * Side;
		Kick.FieldOfView = 0.003f;
		Kick.Frequency = 18.f;
		Kick.Damping = 0.65f;
		break;
	case EWeaponKind::LMG:
		// A mill wheel: a slower, deeper chug than the rifle's, the shoulders rolling with the belt's sway.
		Kick.Pitch = 0.6f;
		Kick.Roll = 0.3f * FMath::Clamp(Shape.Lean, -1.f, 1.f);
		Kick.Yaw = 0.13f * Side;
		Kick.FieldOfView = 0.008f;
		Kick.Frequency = 10.f;
		Kick.Damping = 0.55f;
		break;
	case EWeaponKind::Revolver:
		// The gunslinger's iron: a hard snap that rolls the wrist (always clockwise, a little right, not random) and drops
		// back, well damped, before the next pull.
		Kick.Pitch = 1.9f;
		Kick.Roll = 0.6f;
		Kick.Yaw = 0.1f;
		Kick.FieldOfView = 0.012f;
		Kick.Frequency = 11.f;
		Kick.Damping = 0.7f;
		break;
	case EWeaponKind::Shotgun:
		// A porch cannon: one shove through the shoulders, leaning one way, the camera pushed back.
		Kick.Pitch = 2.5f;
		Kick.Roll = 0.9f * Lean;
		Kick.Yaw = 0.33f * Side;
		Kick.FieldOfView = 0.036f;
		Kick.Push = 2.f;
		Kick.Frequency = 6.5f;
		Kick.Damping = 0.5f;
		break;
	case EWeaponKind::Sniper:
		// One thunderclap: a hard punch and a long settle while the bolt is worked (its aim kick holds meanwhile).
		Kick.Pitch = 2.f;
		Kick.Roll = 0.25f * Lean;
		Kick.Yaw = 0.15f * Side;
		Kick.FieldOfView = 0.016f;
		Kick.Push = 1.2f;
		Kick.Frequency = 8.f;
		Kick.Damping = 0.6f;
		break;
	case EWeaponKind::Launcher:
		// A keg on the shoulder: less lift than the shotgun, the widest whoomph and the deepest push, slow; ForHeave sags it
		// back after.
		Kick.Pitch = 1.4f;
		Kick.Roll = 0.7f * Lean;
		Kick.Yaw = 0.25f * Side;
		Kick.FieldOfView = 0.044f;
		Kick.Push = 3.5f;
		Kick.Frequency = 4.5f;
		Kick.Damping = 0.45f;
		break;
	default:
		// The workhorse: a crisp crack per round, quick enough that a burst reads as a rattle, not a sway.
		Kick.Pitch = 0.55f;
		Kick.Roll = 0.38f * Lean;
		Kick.Yaw = 0.1f * Side;
		Kick.FieldOfView = 0.006f;
		Kick.Frequency = 14.f;
		Kick.Damping = 0.6f;
		break;
	}
	// The table is a base gun's (1.4); the stat's whole range is felt, the calmest roll to the wildest.
	FViewKick Scaled = Kick * (FMath::Clamp(RecoilStat, 0.4f, 2.f) / BaseShotRecoil);
	const float Turn = SightShare(AimAlpha, Shape.AimedTurn);
	const float Field = SightShare(AimAlpha, Shape.AimedFieldOfView);
	Scaled.Pitch *= Turn;
	Scaled.Yaw *= Turn;
	Scaled.Roll *= Turn;
	Scaled.FieldOfView *= Field;
	Scaled.Push *= Field;
	return Scaled;
}

FViewKick ViewKicks::ForShotToday(EWeaponKind Kind, float RecoilStat, float AimAlpha, FRandomStream& Random)
{
	FViewKick Kick;
	const float Side = Random.FRandRange(-1.f, 1.f);
	const float Lean = Random.RandRange(0, 1) == 0 ? -1.f : 1.f;
	if (Kind == EWeaponKind::Shotgun)
	{
		// A heavy shove: slower, bigger, wider, so the blast is felt in the shoulders.
		Kick.Pitch = 1.15f;
		Kick.Roll = 0.55f * Lean;
		Kick.Yaw = 0.15f * Side;
		Kick.FieldOfView = 0.022f;
		Kick.Frequency = 6.5f;
		Kick.Damping = 0.5f;
	}
	else if (Kind == EWeaponKind::Revolver)
	{
		// A sharp snap up and straight back down: quick and well damped, so the sights are back before the next pull.
		Kick.Pitch = 0.9f;
		Kick.Roll = 0.3f * Lean;
		Kick.Yaw = 0.08f * Side;
		Kick.FieldOfView = 0.012f;
		Kick.Frequency = 11.f;
		Kick.Damping = 0.65f;
	}
	else if (Kind == EWeaponKind::Sniper)
	{
		// One big round: a hard punch up and a long settle, the view thumped wider.
		Kick.Pitch = 1.3f;
		Kick.Roll = 0.4f * Lean;
		Kick.Yaw = 0.1f * Side;
		Kick.FieldOfView = 0.02f;
		Kick.Frequency = 7.5f;
		Kick.Damping = 0.55f;
	}
	else if (Kind == EWeaponKind::Launcher)
	{
		// The rocket leaving: a slower, heavier shove than the shotgun's, the view thrown wide.
		Kick.Pitch = 1.4f;
		Kick.Roll = 0.7f * Lean;
		Kick.Yaw = 0.25f * Side;
		Kick.FieldOfView = 0.03f;
		Kick.Frequency = 5.f;
		Kick.Damping = 0.45f;
	}
	else if (Kind == EWeaponKind::LMG)
	{
		// The rifle's tick with weight behind it: a deeper, slower chug, so a long burst rolls rather than rattles.
		Kick.Pitch = 0.42f;
		Kick.Roll = 0.28f * Lean;
		Kick.Yaw = 0.09f * Side;
		Kick.FieldOfView = 0.007f;
		Kick.Frequency = 11.f;
		Kick.Damping = 0.6f;
	}
	else if (Kind == EWeaponKind::SMG)
	{
		// A light, fast buzz: smaller than the rifle's tick and quicker to settle.
		Kick.Pitch = 0.2f;
		Kick.Roll = 0.16f * Lean;
		Kick.Yaw = 0.07f * Side;
		Kick.FieldOfView = 0.003f;
		Kick.Frequency = 18.f;
		Kick.Damping = 0.65f;
	}
	else
	{
		// A crisp tick per round: quick enough that a full-auto burst reads as a rattle, not a sway.
		Kick.Pitch = 0.32f;
		Kick.Roll = 0.22f * Lean;
		Kick.Yaw = 0.06f * Side;
		Kick.FieldOfView = 0.005f;
		Kick.Frequency = 14.f;
		Kick.Damping = 0.6f;
	}
	FViewKick Scaled = Kick * FMath::Clamp(RecoilStat, 0.6f, 1.5f);
	// Through the sight: half the turn and less of the widening, so the reticle stays readable on target.
	const float Turn = SightShare(AimAlpha, GunfeelKick::Default::Today::AimedTurn);
	Scaled.Pitch *= Turn;
	Scaled.Yaw *= Turn;
	Scaled.Roll *= Turn;
	Scaled.FieldOfView *= SightShare(AimAlpha, GunfeelKick::Default::Today::AimedFieldOfView);
	return Scaled;
}

FViewKick ViewKicks::ForHeave(EWeaponKind Kind, float RecoilStat, float AimAlpha, const FShotKickShape& Shape)
{
	FViewKick Kick;
	if (Kind != EWeaponKind::Launcher)
	{
		return Kick;
	}
	// The tube's weight coming down after the rocket's shove: late and slower, so the shoulder is felt taking it.
	Kick.Pitch = -0.5f * (FMath::Clamp(RecoilStat, 0.4f, 2.f) / BaseShotRecoil) * SightShare(AimAlpha, Shape.AimedTurn);
	Kick.Delay = 0.16f;
	Kick.Frequency = 6.f;
	Kick.Damping = 0.55f;
	return Kick;
}

FViewKick ViewKicks::ForAimSettle(float Scale)
{
	// The gun meeting the cheek: a small dip and squeeze, so arriving on the sights is felt.
	FViewKick Kick;
	Kick.Pitch = -0.12f;
	Kick.FieldOfView = -0.003f;
	Kick.Frequency = 9.f;
	Kick.Damping = 0.6f;
	return Kick * FMath::Max(Scale, 0.f);
}

FViewKick ViewKicks::ForHurt(float DamageShare, float SourceSide, float SourceAhead)
{
	// A quarter of the bar or more in one hit is the full jolt; anything that hurts at all gets a fifth of it.
	const float Strength = FMath::Clamp(DamageShare / 0.25f, 0.2f, 1.f);
	const float Side = FMath::Clamp(SourceSide, -1.f, 1.f);
	const float Ahead = FMath::Clamp(SourceAhead, -1.f, 1.f);
	FViewKick Kick;
	// Knocked away from the hit: a blow from the right tips the head left (negative roll), one ahead tips it back.
	Kick.Roll = -Side * 2.5f * Strength;
	Kick.Yaw = -Side * 0.6f * Strength;
	Kick.Pitch = Ahead * 1.6f * Strength;
	Kick.FieldOfView = -0.015f * Strength;
	Kick.Frequency = 8.f;
	Kick.Damping = 0.45f;
	return Kick;
}

FViewKick ViewKicks::ForKill(bool bHeavy)
{
	FViewKick Kick;
	Kick.Pitch = 0.25f;
	Kick.FieldOfView = -0.016f;
	Kick.Frequency = 9.f;
	Kick.Damping = 0.5f;
	return bHeavy ? Kick * 1.4f : Kick;
}

FViewKick ViewKicks::ForRack(float AimAlpha)
{
	FViewKick Kick;
	Kick.Pitch = -0.1f;
	Kick.FieldOfView = -0.004f;
	Kick.Frequency = 12.f;
	Kick.Damping = 0.6f;
	Kick.Pitch *= SightShare(AimAlpha, 0.5f);
	Kick.FieldOfView *= SightShare(AimAlpha, 0.6f);
	return Kick;
}

FViewKick ViewKicks::ForBoltHome(float AimAlpha)
{
	// The bolt slammed home: felt, not a second shot.
	FViewKick Kick;
	Kick.Pitch = 0.08f * SightShare(AimAlpha, 0.5f);
	Kick.Frequency = 12.f;
	Kick.Damping = 0.6f;
	return Kick;
}

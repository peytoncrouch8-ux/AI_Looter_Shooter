#include "Weapons/WeaponRecoil.h"

namespace
{
	/** Base frequency of the gun's springs (Hz), scaled by the profile's snappiness. */
	constexpr float BaseFrequency = 9.f;
	/** Damping ratio of the gun's springs: under 1, so the gun overshoots a touch and settles like a real kick. */
	constexpr float SpringDamping = 0.55f;
	/** The aim kick goes in over a few frames (about 1/rate seconds) rather than snapping. */
	constexpr float AimKickRate = 45.f;
	/** Full recovery starts this long after the last shot; while firing, the aim settles back at a fraction of the rate. */
	constexpr float AimRecoveryDelay = 0.09f;
	constexpr float AimRecoveryWhileFiring = 0.35f;

	/**
	 * Velocity that makes a spring at rest peak at Peak. The impulse response of a spring with this damping peaks at
	 * about 0.52 * v / w.
	 */
	float ImpulseFor(float Peak, float AngularFrequency)
	{
		return Peak * AngularFrequency / 0.52f;
	}
}

void FWeaponRecoil::FSpring::Step(float DeltaSeconds, float AngularFrequency, float Damping)
{
	// The exact motion of an underdamped spring over the step, so the kick is the same at any frame rate:
	// x(t) = e^(-a t) (A cos(w t) + B sin(w t)).
	const float Decay = Damping * AngularFrequency;
	const float Ringing = AngularFrequency * FMath::Sqrt(1.f - Damping * Damping);
	const float A = Value;
	const float B = (Velocity + Decay * Value) / Ringing;
	const float Envelope = FMath::Exp(-Decay * DeltaSeconds);
	const float Cos = FMath::Cos(Ringing * DeltaSeconds);
	const float Sin = FMath::Sin(Ringing * DeltaSeconds);
	Value = Envelope * (A * Cos + B * Sin);
	Velocity = Envelope * ((B * Ringing - Decay * A) * Cos - (Decay * B + A * Ringing) * Sin);
}

void FWeaponRecoil::AddShot(const FWeaponRecoilProfile& Profile, FRandomStream& Random)
{
	AddShot(Profile, Random, 0.f);
}

void FWeaponRecoil::AddShot(const FWeaponRecoilProfile& Profile, FRandomStream& Random, float AimAlpha)
{
	Frequency = BaseFrequency * FMath::Max(Profile.Snappiness, 0.2f);
	const float Omega = 2.f * UE_PI * Frequency;
	// Through the sight the model kicks by the Aimed* shares (1 at the hip, so an old profile is unchanged).
	const float Sight = FMath::Clamp(AimAlpha, 0.f, 1.f);
	const float BackShare = FMath::Lerp(1.f, FMath::Max(Profile.AimedBack, 0.f), Sight);
	const float FlipShare = FMath::Lerp(1.f, FMath::Max(Profile.AimedFlip, 0.f), Sight);
	const float RollShare = FMath::Lerp(1.f, FMath::Max(Profile.AimedRoll, 0.f), Sight);
	Back.Velocity += ImpulseFor(Profile.KickBack * BackShare, Omega);
	Pitch.Velocity += ImpulseFor(Profile.MuzzleFlip * FlipShare, Omega);
	// The random draws stay in the old order and count, so a profile with no sight shares kicks exactly as it did.
	Yaw.Velocity += ImpulseFor(Random.FRandRange(-1.f, 1.f) * Profile.MuzzleTwist * FlipShare, Omega);
	RollSpring.Velocity += ImpulseFor(Random.FRandRange(-1.f, 1.f) * Profile.Roll * RollShare, Omega);

	// A burst climbs: each shot in it kicks the aim a little harder than the last (up to the profile's most), and a pause
	// of BurstGap starts it over.
	if (SinceShot > BurstGap)
	{
		BurstShots = 0;
	}
	const float Climb = GetClimb(Profile);
	BurstShots = FMath::Min(BurstShots + 1, 10000);

	AimPending.Pitch += CappedPitchKick(Profile, Profile.AimKick * Climb * Random.FRandRange(0.85f, 1.15f));
	AimPending.Yaw += Random.FRandRange(-1.f, 1.f) * Profile.AimKickSide;
	AimRecovery = Profile.AimRecovery;
	SinceShot = 0.f;
	// A hold is the shot's own (HoldRecovery after this sets it).
	HoldSeconds = 0.f;
}

float FWeaponRecoil::CappedPitchKick(const FWeaponRecoilProfile& Profile, float Kick) const
{
	if (Profile.ClimbCap <= 0.f || Kick <= 0.f)
	{
		return Kick;
	}
	// What the burst still holds the aim up by (kicked in or on its way, less what has recovered or the player pulled
	// down): only the room left under the cap is kicked whole; past it a shot keeps a nod, so the gun still feels alive.
	const float Held = FMath::Max(AimApplied.Pitch + AimPending.Pitch, 0.f);
	const float Room = FMath::Max(Profile.ClimbCap - Held, 0.f);
	const float Whole = FMath::Min(Kick, Room);
	return Whole + (Kick - Whole) * FMath::Clamp(Profile.ClimbCapFloor, 0.f, 1.f);
}

float FWeaponRecoil::GetClimb(const FWeaponRecoilProfile& Profile) const
{
	return FMath::Clamp(1.f + Profile.ClimbPerShot * static_cast<float>(BurstShots), 1.f, FMath::Max(Profile.ClimbMax, 1.f));
}

FRotator FWeaponRecoil::Tick(float DeltaSeconds, float PlayerPitchInput)
{
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	const float Omega = 2.f * UE_PI * Frequency;
	Back.Step(Dt, Omega, SpringDamping);
	Pitch.Step(Dt, Omega, SpringDamping);
	Yaw.Step(Dt, Omega, SpringDamping);
	RollSpring.Step(Dt, Omega, SpringDamping);

	// Pulling down against the kick counts toward its recovery, so the aim never settles below where the player put it.
	if (PlayerPitchInput < 0.f && AimApplied.Pitch > 0.f)
	{
		AimApplied.Pitch = FMath::Max(0.f, AimApplied.Pitch + PlayerPitchInput);
	}

	// The kick goes in quickly...
	const FRotator Kick = AimPending * (1.f - FMath::Exp(-AimKickRate * Dt));
	AimPending -= Kick;
	AimApplied += Kick;

	// ...and settles back: slowly while still firing, fully once the shooting stops; not at all while a hold lasts (a bolt
	// gun's kick stays up while the bolt is worked, then the world settles).
	SinceShot += Dt;
	const float Rate = SinceShot < HoldSeconds ? 0.f : AimRecovery * (SinceShot > AimRecoveryDelay ? 1.f : AimRecoveryWhileFiring);
	const FRotator Settle = AimApplied * (1.f - FMath::Exp(-Rate * Dt));
	AimApplied -= Settle;
	return Kick - Settle;
}

bool FWeaponRecoil::IsSettled() const
{
	for (const FSpring* Spring : { &Back, &Pitch, &Yaw, &RollSpring })
	{
		if (FMath::Abs(Spring->Value) > 0.01f || FMath::Abs(Spring->Velocity) > 0.1f)
		{
			return false;
		}
	}
	return AimPending.IsNearlyZero(0.001f) && AimApplied.IsNearlyZero(0.001f);
}

void FWeaponRecoil::Reset()
{
	*this = FWeaponRecoil();
}

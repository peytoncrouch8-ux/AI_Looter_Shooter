// AUnpaidCreature's element statuses on its body (the "Elements matter" job, 2026-10-29), in two layers: the pose's channels
// (StatusChannels, eased with the rest by AnimateBody: arms, lean, jaw, look) and quick motions laid on the trunk's bones
// after them (LayStatusBones: tremors, twists, the jerk, the cough), with its shroud shaken on top (StatusShroudShake), so
// it moves as a hanging shroud of the dead would:
//  - Burning, it beats at the flames: arms clawing up and raking across in turn, the trunk twisting, the jaw wide in a
//    scream, its shroud flapping.
//  - Jolted, it locks: arms thrown out stiff, fingers splayed, the trunk and head trembling and its shroud jittering; the
//    burst snaps it back.
//  - Rotting, it hunches over, hands drawn in to its middle; every few seconds it doubles over in a cough, the jaw
//    gaping.
//  - Salted, it stills: the jaw's twitch and its sway die away, its fingers held stiff, its clock slowed (AnimateBody).
//  - Smitten, it flinches from the light gathering in it: rearing back, head turned down and aside, an arm up, shivering
//    harder as the burst nears.
// Through an attack its tell must still read, so the channels take only AttackShare of the status pose then. The miners
// and the Bell-Ringer lay their own arms by StatusShown too (DeadMinerCreaturePose.cpp, BellRingerCreaturePose.cpp).

#include "Creatures/UnpaidCreature.h"
#include "Creatures/CreatureStatusPose.h"

namespace
{
	FQuat PitchBy(float Degrees)
	{
		return FQuat(FVector::RightVector, FMath::DegreesToRadians(Degrees));
	}

	FQuat RollBy(float Degrees)
	{
		return FQuat(FVector::ForwardVector, FMath::DegreesToRadians(Degrees));
	}

	FQuat YawBy(float Degrees)
	{
		return FQuat(FVector::UpVector, FMath::DegreesToRadians(Degrees));
	}

	/** Through an attack the channels take this share of the status pose (its tell must read). */
	constexpr float AttackShare = 0.3f;

	// --- The channels (degrees, or 0 to 1 as the channels are) ---

	/** A burn: arms clawing and raking, thrown wide, the jaw's scream, leaning back from the flames. */
	constexpr float BurnRake = 0.85f;
	constexpr float BurnSpread = 0.3f;
	constexpr float BurnJaw = 24.f;
	constexpr float BurnLeanBack = 0.4f;
	/** A jolt: arms out stiff (more at the burst), fingers splayed, the jaw clenched open; the burst rears it. */
	constexpr float JoltSpread = 0.55f;
	constexpr float JerkSpread = 0.4f;
	constexpr float JoltSplay = 18.f;
	constexpr float JoltJaw = 14.f;
	constexpr float JerkRear = 0.5f;
	/** The rot: hunched over, hands drawn in across its middle, the jaw slack; the cough's gape. */
	constexpr float RotLean = 16.f;
	constexpr float RotCurl = 10.f;
	constexpr float RotHands = 0.35f;
	constexpr float RotJaw = 10.f;
	constexpr float CoughJaw = 25.f;
	/** The salt: the jaw still, fingers held straight, its sway gone. */
	constexpr float SaltJaw = 5.f;
	constexpr float SaltCurl = 12.f;
	/** A smite: rearing back, head down and aside, arms up a little, the jaw open. */
	constexpr float SmiteLeanBack = 6.f;
	constexpr float SmiteRear = 0.3f;
	constexpr float SmiteLookDown = 25.f;
	constexpr float SmiteLookAside = 30.f;
	constexpr float SmiteSpread = 0.25f;
	constexpr float SmiteJaw = 12.f;

	// --- The bones (degrees, cm) ---

	/** A burn's twist at the chest and roll of the head. */
	constexpr float BurnChestYaw = 6.f;
	constexpr float BurnHeadRoll = 5.f;
	/** A jolt's tremor at the chest and head, and the burst's snap back at the waist and head. */
	constexpr float JoltChestRoll = 2.5f;
	constexpr float JoltHeadYaw = 3.f;
	constexpr float JerkWaist = 8.f;
	constexpr float JerkHead = 12.f;
	/** A cough doubling it over: the chest, the head, the hips dropping. */
	constexpr float CoughChest = 14.f;
	constexpr float CoughHead = 12.f;
	constexpr float CoughDrop = 3.f;
	/** A smite's shiver (times the square of the smite). */
	constexpr float SmiteShiver = 2.f;
	constexpr float SmiteShiverHz = 9.f;

	// --- The shroud's shake (cm/s on its drift) ---

	constexpr float JoltShroud = 220.f;
	constexpr float BurnShroud = 160.f;
	constexpr float JerkShroud = 300.f;
	constexpr float SmiteShroud = 60.f;
}

void AUnpaidCreature::StatusChannels(FPoseChannels& Goal) const
{
	const FCreatureStatusPose& Status = StatusShown;
	if (!Status.IsAny() || IsDead())
	{
		return;
	}
	const float Share = GetCreatureState() == ECreatureState::Attack ? AttackShare : 1.f;
	const float Burn = Status.Burn * Share;
	const float Jolt = Status.Jolt * Share;
	const float Jerk = Status.JoltJerk * Share;
	const float Rot = Status.Rot * Share;
	const float Cough = Status.Cough * Share;
	const float Salt = Status.Salt * Share;
	const float Smite = Status.Smite * Share;

	// Burning: beating at the flames.
	if (Burn > 0.f)
	{
		Goal.Hunt = FMath::Max(Goal.Hunt, Burn);
		Goal.Rake[0] = FMath::Lerp(Goal.Rake[0], BurnRake * Status.Flail(), Burn);
		Goal.Rake[1] = FMath::Lerp(Goal.Rake[1], BurnRake * Status.Flail(UE_PI), Burn);
		Goal.Spread = FMath::Max(Goal.Spread, BurnSpread * Burn);
		Goal.Jaw = FMath::Max(Goal.Jaw, BurnJaw * Burn);
		Goal.Lean = FMath::Lerp(Goal.Lean, -2.f, BurnLeanBack * Burn);
	}
	// Jolted: locked, arms out stiff.
	if (Jolt > 0.f || Jerk > 0.f)
	{
		Goal.Spread = FMath::Max(Goal.Spread, JoltSpread * Jolt + JerkSpread * Jerk);
		Goal.Curl = FMath::Lerp(Goal.Curl, -5.f, Jolt);
		Goal.Splay = FMath::Max(Goal.Splay, JoltSplay * Jolt);
		Goal.Jaw = FMath::Max(Goal.Jaw, JoltJaw * Jolt);
		Goal.Hunt *= 1.f - 0.7f * FMath::Min(Jolt, 1.f);
		Goal.Rear = FMath::Max(Goal.Rear, JerkRear * Jerk);
	}
	// Rotting: hunched, hands in to its middle, the cough's gape.
	if (Rot > 0.f)
	{
		Goal.Lean += RotLean * Rot;
		Goal.Curl += RotCurl * Rot;
		Goal.Hunt *= 1.f - 0.4f * FMath::Min(Rot, 1.f);
		Goal.Rake[0] = FMath::Lerp(Goal.Rake[0], RotHands, 0.6f * FMath::Min(Rot, 1.f));
		Goal.Rake[1] = FMath::Lerp(Goal.Rake[1], RotHands, 0.6f * FMath::Min(Rot, 1.f));
		Goal.Jaw = FMath::Max(Goal.Jaw, RotJaw * Rot + CoughJaw * Cough);
	}
	// Salted: stilled and stiff.
	if (Salt > 0.f)
	{
		const float Still = FMath::Min(Salt, 1.f);
		Goal.Jaw = FMath::Lerp(Goal.Jaw, SaltJaw, 0.7f * Still);
		Goal.Curl = FMath::Lerp(Goal.Curl, SaltCurl, Still);
		Goal.Sway *= 1.f - 0.8f * Still;
		Goal.Hunt *= 1.f - 0.3f * Still;
	}
	// Smitten: flinching from the light in it.
	if (Smite > 0.f)
	{
		Goal.Lean = FMath::Lerp(Goal.Lean, -SmiteLeanBack, FMath::Min(Smite, 1.f));
		Goal.Rear = FMath::Max(Goal.Rear, SmiteRear * Smite);
		Goal.LookPitch -= SmiteLookDown * Smite;
		Goal.LookYaw += SmiteLookAside * Smite;
		Goal.Spread = FMath::Max(Goal.Spread, SmiteSpread * Smite);
		Goal.Jaw = FMath::Max(Goal.Jaw, SmiteJaw * Smite);
	}
}

void AUnpaidCreature::LayStatusBones()
{
	const FCreatureStatusPose& Status = StatusShown;
	if (!Status.IsAny() || IsDead())
	{
		return;
	}
	const float Shiver = SmiteShiver * FMath::Square(Status.Smite) * Status.Wave(SmiteShiverHz);
	if (SpineBone != INDEX_NONE)
	{
		// The burst snaps it back at the waist; a cough doubles it over.
		Bones[SpineBone].Own = (Bones[SpineBone].Own * PitchBy(0.5f * CoughChest * Status.Cough - JerkWaist * Status.JoltJerk)).GetNormalized();
	}
	if (ChestBone != INDEX_NONE)
	{
		const float Yaw = BurnChestYaw * Status.Burn * Status.Flail(0.5f);
		const float Roll = JoltChestRoll * Status.Jolt * Status.Tremor() + Shiver;
		Bones[ChestBone].Own = (Bones[ChestBone].Own * YawBy(Yaw) * RollBy(Roll) * PitchBy(CoughChest * Status.Cough)).GetNormalized();
	}
	if (HeadBone != INDEX_NONE)
	{
		const float Yaw = JoltHeadYaw * Status.Jolt * Status.Tremor(1.7f) + Shiver;
		const float Roll = BurnHeadRoll * Status.Burn * Status.Flail(1.1f);
		const float Pitch = CoughHead * Status.Cough - JerkHead * Status.JoltJerk;
		Bones[HeadBone].Own = (Bones[HeadBone].Own * YawBy(Yaw) * RollBy(Roll) * PitchBy(Pitch)).GetNormalized();
	}
	if (PelvisBone != INDEX_NONE)
	{
		Bones[PelvisBone].Shift.Z -= CoughDrop * Status.Cough;
	}
}

FVector AUnpaidCreature::StatusShroudShake() const
{
	const FCreatureStatusPose& Status = StatusShown;
	if (!Status.IsAny() || IsDead())
	{
		return FVector::ZeroVector;
	}
	const FVector Jitter(Status.Tremor(), Status.Tremor(1.3f), 0.5f * Status.Tremor(2.2f));
	const FVector Flap(Status.Flail(0.3f), Status.Flail(1.9f), 0.f);
	const FVector Shiver(Status.Wave(SmiteShiverHz), Status.Wave(SmiteShiverHz, 1.2f), 0.f);
	// The burst throws it back along its front, as a gust would.
	return Jitter * (JoltShroud * Status.Jolt) + Flap * (BurnShroud * Status.Burn) + FVector(-JerkShroud * Status.JoltJerk, 0.f, 0.f)
		+ Shiver * (SmiteShroud * FMath::Square(Status.Smite));
}

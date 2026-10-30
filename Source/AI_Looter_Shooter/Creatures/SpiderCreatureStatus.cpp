// ASpiderCreature's element statuses on its body (the "Elements matter" job, 2026-10-29), laid on its hit feel's pose the way
// its stagger is (SpiderCreatureHurt.cpp's HitFeelPose) and on its legs' aims (PoseGaitLeg):
//  - Burning, it bucks and twists as it beats at the flames, its fangs chattering, each leg curling in and kicking up in
//    its own time (as a burning spider's legs draw in).
//  - Jolted, it locks rigid, legs splayed stiff and trembling fast; the burst jerks it up, nose snapped down, fangs wide.
//  - Rotting, it sags low with its nose down and its legs drawn in; every few seconds a cough heaves the body up, clenches
//    the abdomen and gapes the fangs.
//  - Salted, it crouches a little, legs held in, and its idle motion (breathing, fangs, sway) runs slow (ClockScale).
//  - Smitten, it cowers from the light gathering in it: crouched, nose tucked, shivering harder as the burst nears.
// Looks only: its speed, damage, attack timing and brain are untouched (the user's rule for the spiders' fight). The
// Gravemother is a boss: CreatureStatusPose::Of gives her nothing.

#include "Creatures/SpiderCreature.h"
#include "Creatures/CreatureStatusPose.h"

namespace
{
	// --- The body (degrees, cm at size 1) ---

	/** A burn's buck: its twist and roll, its bob, and its fangs' chatter. */
	constexpr float BurnTwist = 7.f;
	constexpr float BurnRoll = 6.f;
	constexpr float BurnBob = 4.f;
	constexpr float BurnFang = 14.f;
	constexpr float BurnChatterHz = 7.f;
	/** A jolt's tremor (roll and pitch), and its burst's jerk: up off its feet, the nose snapped down, the fangs thrown wide. */
	constexpr float JoltShake = 2.5f;
	constexpr float JerkLift = 9.f;
	constexpr float JerkNose = 10.f;
	constexpr float JoltFang = 28.f;
	/** The rot's sag and nose down, its abdomen drawn in; a cough's heave up, its nose, the abdomen's clench, the fangs' gape. */
	constexpr float RotSag = 9.f;
	constexpr float RotNose = 7.f;
	constexpr float RotAbdomen = 5.f;
	constexpr float CoughHeave = 7.f;
	constexpr float CoughNose = 9.f;
	constexpr float CoughClench = 10.f;
	constexpr float CoughGape = 18.f;
	/** The salt's stiff crouch. */
	constexpr float SaltSag = 3.f;
	/** A smite's cower: crouched, nose tucked, its shiver (grows with the square of the smite: most just before the burst). */
	constexpr float SmiteCrouch = 7.f;
	constexpr float SmiteTuck = 6.f;
	constexpr float SmiteShiver = 2.5f;
	constexpr float SmiteShiverHz = 9.f;

	// --- The legs (shares of a leg's reach) ---

	/** A burn's curl in and kick up. */
	constexpr float BurnCurl = 0.22f;
	constexpr float BurnKick = 0.28f;
	/** A jolt's stiff splay (more at the burst) and its twitch up. */
	constexpr float JoltSplay = 0.12f;
	constexpr float JerkSplay = 0.15f;
	constexpr float JoltTwitch = 0.06f;
	/** Drawn in under it: rotting, salted, cowering. */
	constexpr float RotCurl = 0.12f;
	constexpr float SaltCurl = 0.05f;
	constexpr float SmiteCurl = 0.1f;
}

void ASpiderCreature::TickStatusPose(float DeltaSeconds)
{
	StatusShown = IsDead() ? FCreatureStatusPose() : CreatureStatusPose::Of(this);
	// Salted, its idle motion runs slow: its clock (counted up in Tick) gives back what the salt holds off.
	AnimTime -= DeltaSeconds * (1.f - StatusShown.ClockScale);
}

void ASpiderCreature::AddStatusPose(FHitFeelPose& Pose) const
{
	const FCreatureStatusPose& Status = StatusShown;
	if (!Status.IsAny())
	{
		return;
	}
	const float Scale = GetSizeScale();
	// Burning: bucking and twisting as it beats at the flames, the fangs chattering.
	Pose.Yaw += BurnTwist * Status.Burn * Status.Flail();
	Pose.Roll += BurnRoll * Status.Burn * Status.Flail(1.7f);
	Pose.Drop += BurnBob * Scale * Status.Burn * (0.5f + 0.5f * Status.Flail(0.9f));
	Pose.FangAdd += BurnFang * Status.Burn * FMath::Abs(Status.Wave(BurnChatterHz));
	// Jolted: rigid and trembling; the burst jerks it up (a drop below 0 lifts it), nose snapped down, fangs wide.
	Pose.Roll += JoltShake * Status.Jolt * Status.Tremor();
	Pose.Pitch += JoltShake * Status.Jolt * Status.Tremor(2.f) - JerkNose * Status.JoltJerk;
	Pose.Drop -= JerkLift * Scale * Status.JoltJerk;
	Pose.FangAtLeast = FMath::Max(Pose.FangAtLeast, JoltFang * FMath::Max(0.6f * Status.Jolt, Status.JoltJerk));
	// Rotting: sagging, nose down; a cough heaves it up, clenches the abdomen and gapes the fangs.
	Pose.Drop += (RotSag * Status.Rot - CoughHeave * Status.Cough) * Scale;
	Pose.Pitch += CoughNose * Status.Cough - RotNose * Status.Rot;
	Pose.Abdomen -= RotAbdomen * Status.Rot + CoughClench * Status.Cough;
	Pose.FangAdd += CoughGape * Status.Cough;
	// Salted: a stiff, low crouch (its clock slowed: TickStatusPose).
	Pose.Drop += SaltSag * Scale * Status.Salt;
	// Smitten: cowering from the light in it, shivering harder as the burst nears.
	Pose.Drop += SmiteCrouch * Scale * Status.Smite;
	Pose.Pitch -= SmiteTuck * Status.Smite;
	Pose.Roll += SmiteShiver * FMath::Square(Status.Smite) * Status.Wave(SmiteShiverHz);
}

FVector ASpiderCreature::StatusFootOffset(const FLeg& Leg, const FVector& Outward) const
{
	const FCreatureStatusPose& Status = StatusShown;
	if (!Status.IsAny())
	{
		return FVector::ZeroVector;
	}
	const float Reach = static_cast<float>(Leg.Rest.Size2D()) * GetSizeScale();
	// Each leg in its own time: by its pair down the body, the two sides opposed.
	const float Phase = 1.9f * static_cast<float>(Leg.Pair) + (Leg.Side > 0.f ? 0.f : UE_PI);
	// Burning: curling in and kicking up.
	const float Kick = FMath::Max(Status.Flail(Phase), 0.f);
	float In = BurnCurl * Status.Burn * (0.4f + 0.6f * Kick);
	float Up = BurnKick * Status.Burn * Kick;
	// Jolted: splayed stiff, twitching.
	In -= JoltSplay * Status.Jolt + JerkSplay * Status.JoltJerk;
	Up += JoltTwitch * Status.Jolt * FMath::Max(Status.Tremor(Phase), 0.f);
	// Rotting, salted or cowering: drawn in under the body.
	In += RotCurl * Status.Rot + SaltCurl * Status.Salt + SmiteCurl * Status.Smite;
	return (FVector::UpVector * Up - Outward * In) * Reach;
}

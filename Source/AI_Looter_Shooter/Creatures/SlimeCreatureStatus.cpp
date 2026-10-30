// ASlimeCreature's element statuses on its gel (the "Elements matter" job, 2026-10-29), as kicks on its squash and core springs
// and as rests for its squash and lean (SlimeCreatureReact.cpp's ReactSquash and TickReactions take them), so it stays a
// slime: everything it shows is the jelly wobbling.
//  - Burning, it sizzles: the squash shivers in quick kicks, bubbles boil up through the core, and the jelly sloshes from
//    side to side as if trying to shake the flames off.
//  - Jolted, it stiffens tall and trembles fast; the burst jerks it up tall, the core flung up inside it.
//  - Rotting, it sags low and slumps forward; every few seconds a cough heaves it down and bulges it back up, the core
//    retched up toward its top.
//  - Salted, it sits a little low and its slow wobble and sway run slower still (ClockScale).
//  - Smitten, it shrinks down from the light gathering in it, shivering harder as the burst nears.
// Looks only: its hops, leap and timing are untouched.

#include "Creatures/SlimeCreature.h"
#include "Creatures/CreatureReactComponent.h"
#include "Creatures/CreatureStatusPose.h"

namespace
{
	/** The rests (height scale): the rot's sag and the cough's bulge, the salt's sit, a smite's cower, a jolt held tall. */
	constexpr float RotSag = 0.12f;
	constexpr float CoughBulge = 0.14f;
	constexpr float SaltSit = 0.05f;
	constexpr float SmiteCower = 0.1f;
	constexpr float JoltTall = 0.06f;

	/** The kicks on the squash spring (its speed): a burn's sizzle, a jolt's tremor, the burst's jerk, a cough's heave. */
	constexpr float SizzleHz = 11.f;
	constexpr float SizzleKick = 2.5f;
	constexpr float TremorKick = 3.f;
	constexpr float JerkKick = 9.f;
	constexpr float CoughKick = 5.f;
	/** A smite's shiver (times the square of the smite: most just before its burst). */
	constexpr float SmiteShiverKick = 2.f;
	/** The core's kicks (cm/s, the mesh's space): a burn's boiling bubbles (each kick's chance), the jerk's fling, the retch. */
	constexpr float BoilChance = 0.35f;
	constexpr float BoilSide = 35.f;
	constexpr float BoilUp[2] = { 40.f, 85.f };
	constexpr float JerkCoreUp = 120.f;
	constexpr float RetchCoreUp = 100.f;

	/** The lean (degrees): a burn's slosh forward and to the side, a jolt's shiver, the rot's slump and the cough's. */
	constexpr float BurnSloshForward = 3.5f;
	constexpr float BurnSloshSide = 4.f;
	constexpr float JoltShiver = 1.2f;
	constexpr float RotSlump = 2.f;
	constexpr float CoughSlump = 3.f;
}

void ASlimeCreature::TickStatusPose(float DeltaSeconds)
{
	StatusShown = IsDead() ? FCreatureStatusPose() : CreatureStatusPose::Of(this);
	const FCreatureStatusPose& Status = StatusShown;
	// Salted, its wobble's clock (counted up in AnimateBody) gives back what the salt holds off.
	if (!IsPoseFrozen())
	{
		AnimTime -= DeltaSeconds * (1.f - Status.ClockScale);
	}
	UCreatureReactComponent* Reacts = GetReact();
	if (!Reacts || IsDead() || IsPoseFrozen() || !Status.IsAny() || DeltaSeconds <= 0.f)
	{
		LastJerkShown = Status.JoltJerk;
		LastCoughShown = Status.Cough;
		return;
	}
	FRandomStream& Dice = Reacts->GetRandom();

	// The burst's jerk, as it starts: up tall at once, the core flung up.
	if (Status.JoltJerk > 0.5f && LastJerkShown <= 0.5f)
	{
		Squash.Velocity += JerkKick;
		CoreVelocity += FVector(0.f, 0.f, JerkCoreUp);
	}
	// A cough, as it starts: heaved down (the rest's bulge brings it back up), the core retched up.
	if (Status.Cough > 0.f && LastCoughShown <= 0.f)
	{
		Squash.Velocity -= CoughKick * Status.Rot;
		CoreVelocity += FVector(0.f, 0.f, RetchCoreUp * Status.Rot);
	}
	LastJerkShown = Status.JoltJerk;
	LastCoughShown = Status.Cough;

	// The sizzle, the tremor and the cower's shiver: the squash kicked one way and the other (quicker for the jolt), the burn's core boiling.
	const float Burn = Status.Burn;
	const float Jolt = Status.Jolt;
	const float Shiver = FMath::Square(Status.Smite);
	if ((Burn <= 0.f && Jolt <= 0.f && Shiver <= 0.f) || Squash.IsRamping())
	{
		return;
	}
	StatusKickClock -= DeltaSeconds;
	if (StatusKickClock > 0.f)
	{
		return;
	}
	const float Hz = Jolt > Burn ? CreatureStatusPose::TremorHz : SizzleHz;
	StatusKickClock = 0.5f / Hz;
	StatusKickSign = -StatusKickSign;
	Squash.Velocity += StatusKickSign * (SizzleKick * Burn + TremorKick * Jolt + SmiteShiverKick * Shiver);
	if (Burn > 0.f && Dice.FRand() < BoilChance * Burn)
	{
		CoreVelocity += FVector(Dice.FRandRange(-BoilSide, BoilSide), Dice.FRandRange(-BoilSide, BoilSide), Dice.FRandRange(BoilUp[0], BoilUp[1]));
	}
}

float ASlimeCreature::StatusSquash() const
{
	const FCreatureStatusPose& Status = StatusShown;
	return CoughBulge * Status.Cough - RotSag * Status.Rot - SaltSit * Status.Salt - SmiteCower * Status.Smite + JoltTall * Status.Jolt;
}

FVector2D ASlimeCreature::StatusLean() const
{
	const FCreatureStatusPose& Status = StatusShown;
	if (!Status.IsAny())
	{
		return FVector2D::ZeroVector;
	}
	const FVector2D Slosh(BurnSloshForward * Status.Flail(0.4f), BurnSloshSide * Status.Flail(2.1f));
	const FVector2D Shiver(JoltShiver * Status.Tremor(), JoltShiver * Status.Tremor(1.3f));
	const FVector2D Slump(RotSlump * Status.Rot + CoughSlump * Status.Cough, 0.f);
	return Slosh * Status.Burn + Shiver * Status.Jolt + Slump;
}

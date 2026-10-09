#include "Player/PlayerMeleeRules.h"
#include "Combat/CombatRules.h"
#include "Player/PlayerSize.h"

float FMeleeRules::Reach()
{
	return FullSizeReach * LooterPlayerSize::Scale;
}

float FMeleeRules::StrikeDamage(float LevelScale, float Roll)
{
	// Like any hit, a spread of numbers about the listed damage; a strike finds no critical spot.
	return LooterCombat::HitDamage(BaseDamage * FMath::Max(LevelScale, 0.f), false, Roll);
}

FVector FMeleeRules::KnockVelocity(const FVector& Away, float Size, ECreatureRank Rank, bool bLight)
{
	const FVector Flat = Away.GetSafeNormal2D();
	if (Flat.IsNearlyZero() || Rank >= ECreatureRank::Legendary)
	{
		// A Soulfed monster or a boss stands its ground: its fight is made around its own moves.
		return FVector::ZeroVector;
	}
	// A tougher rank gives less ground, as with a stagger's shove; a bigger body less again, a slime more.
	const float RankShare = Rank == ECreatureRank::Rare ? 0.85f : (Rank == ECreatureRank::Epic ? 0.7f : 1.f);
	const float Scale = RankShare * (bLight ? LightKnockScale : 1.f) / FMath::Sqrt(FMath::Max(Size, 0.3f));
	return (Flat * KnockSpeed + FVector::UpVector * KnockLift) * Scale;
}

EMeleeBlock FMeleeRules::WhyBlocked(const FMeleeGateInput& In)
{
	if (!In.bAlive)
	{
		return EMeleeBlock::NoPlayer;
	}
	if (In.bInScene)
	{
		return EMeleeBlock::Scene;
	}
	if (In.bMenuOpen)
	{
		return EMeleeBlock::Menu;
	}
	// The hands are on the ledge.
	if (In.bTraversing)
	{
		return EMeleeBlock::Traversing;
	}
	if (In.bSwinging || In.Now - In.LastStrike < CooldownSeconds)
	{
		return EMeleeBlock::Cooldown;
	}
	return EMeleeBlock::None;
}

bool FMeleeRules::FindContact(const FMeleeAim& Aim, const FMeleeBody& Body, float Reach, FMeleeContact& OutContact)
{
	// Height: some of the body between just under the feet (a slime in a dip) and a little over the eye.
	const double Bottom = Body.Center.Z - Body.HalfHeight;
	const double Top = Body.Center.Z + Body.HalfHeight;
	if (Top < Aim.FeetZ - ReachUnderFeet || Bottom > Aim.Eye.Z + ReachOverEye)
	{
		return false;
	}

	// Across the ground: from the eye's upright line to the body's near side.
	const FVector2D Eye(Aim.Eye.X, Aim.Eye.Y);
	const FVector2D ToCenter = FVector2D(Body.Center.X, Body.Center.Y) - Eye;
	const float CenterDistance = static_cast<float>(ToCenter.Size());
	const float Distance = FMath::Max(CenterDistance - Body.Radius, 0.f);
	if (Distance > Reach)
	{
		return false;
	}
	const FVector2D Way = CenterDistance > UE_KINDA_SMALL_NUMBER ? ToCenter / CenterDistance : FVector2D::ZeroVector;

	// The line of the strike: the way the player looks, across the ground. Looking straight down or up it has no way of
	// its own, so only the height and the up-and-down test below choose.
	const FVector2D Forward = FVector2D(Aim.Forward.X, Aim.Forward.Y).GetSafeNormal(UE_KINDA_SMALL_NUMBER);
	float Angle = 0.f;
	if (!Forward.IsZero() && !Way.IsZero())
	{
		const float CenterAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(static_cast<float>(FVector2D::DotProduct(Forward, Way)), -1.f, 1.f)));
		// The strike only has to reach the body's edge: a wide spider is hit off-centre as well as a thin slime dead on.
		const float HalfWidth = Body.Radius >= CenterDistance ? 90.f : FMath::RadiansToDegrees(FMath::Asin(Body.Radius / CenterDistance));
		Angle = FMath::Max(CenterAngle - HalfWidth, 0.f);
	}
	const bool bHugging = Distance <= HugDistance;
	if (Angle > HalfConeDegrees && !(bHugging && Angle < 90.f))
	{
		return false;
	}

	// The point it lands on: the near side, at the chest if the body is that tall, else its top (or bottom, if it's high).
	const FVector2D Near = CenterDistance > Body.Radius ? FVector2D(Body.Center.X, Body.Center.Y) - Way * Body.Radius
		: FVector2D(Body.Center.X, Body.Center.Y);
	const double StrikeZ = FMath::Clamp(Aim.Eye.Z - StrikeUnderEye, Bottom, Top);
	const FVector Point(Near.X, Near.Y, StrikeZ);

	// Up and down: near enough the way the player looks. A slime at the feet is hit looking ahead, not with the eyes on
	// the sky; one pressed against the shins is hit whenever the player looks anywhere but up.
	const FVector ToPoint = Point - Aim.Eye;
	if (!ToPoint.IsNearlyZero())
	{
		const float PitchTo = FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(ToPoint.Z), static_cast<float>(ToPoint.Size2D())));
		const float ViewPitch = FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(Aim.Forward.Z), static_cast<float>(Aim.Forward.Size2D())));
		if (FMath::Abs(PitchTo - ViewPitch) > (bHugging ? HugPitchOff : MaxPitchOff))
		{
			return false;
		}
	}

	OutContact.Point = Point;
	OutContact.Distance = Distance;
	OutContact.Angle = Angle;
	// Nearer first, then more in line: what's pressing in on the player is what they meant to hit.
	OutContact.Score = Distance / FMath::Max(Reach, 1.f) + 0.5f * Angle / HalfConeDegrees;
	return true;
}

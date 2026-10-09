#include "Combat/GraveSaltRules.h"
#include "Combat/CombatRules.h"

void FGraveSaltRules::Fly(FVector& Location, FVector& Velocity, float Dt, float GravityZ)
{
	// The exact ballistic step (not Euler's): the same arc whatever the frame rate, so a throw lands where the tests say.
	const float Step = FMath::Max(Dt, 0.f);
	Location += Velocity * Step + FVector(0.f, 0.f, 0.5f * GravityZ * Step * Step);
	Velocity.Z += GravityZ * Step;
}

FVector FGraveSaltRules::Bounce(const FVector& Velocity, const FVector& Normal)
{
	const FVector Face = Normal.GetSafeNormal();
	if (Face.IsNearlyZero())
	{
		return Velocity * -Restitution;
	}
	const float Into = static_cast<float>(FVector::DotProduct(Velocity, Face));
	if (Into >= 0.f)
	{
		// Already leaving the surface: nothing to bounce off.
		return Velocity;
	}
	const FVector Across = Velocity - Face * Into;
	return Across * SlideKeep - Face * (Into * Restitution);
}

FVector FGraveSaltRules::Contact(const FVector& Velocity, const FVector& Normal, float Dt, bool& bOutRolled)
{
	bOutRolled = false;
	const FVector Face = Normal.GetSafeNormal();
	const float Into = static_cast<float>(FVector::DotProduct(Velocity, Face));
	if (Face.Z < GroundMinUp || -Into * Restitution >= RollSpeed)
	{
		return Bounce(Velocity, Face);
	}
	// Rolling: only its way along the ground is left, and the tin's drag on the dirt eats it. Gravity down a slope still
	// rolls it on: each frame's fall into the slope comes back as speed along it.
	bOutRolled = true;
	const FVector Along = Velocity - Face * FMath::Min(Into, 0.f);
	const float Speed = static_cast<float>(Along.Size());
	if (Speed <= UE_KINDA_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}
	const float Slowed = FMath::Max(Speed - RollFriction * FMath::Max(Dt, 0.f), 0.f);
	return Along * (Slowed / Speed);
}

bool FGraveSaltRules::ShouldRest(const FVector& Velocity, const FVector& Normal)
{
	return Normal.GetSafeNormal().Z >= GroundMinUp && Velocity.Size() < RestSpeed;
}

float FGraveSaltRules::Falloff(float Distance)
{
	const float Out = FMath::Max(Distance, 0.f);
	if (Out > Radius)
	{
		return 0.f;
	}
	if (Out <= CoreRadius)
	{
		return 1.f;
	}
	// Straight down from the core's edge to the radius: what's caught at the edge still feels it, and the heart is
	// where it pays to land it.
	return FMath::Lerp(1.f, EdgeShare, (Out - CoreRadius) / (Radius - CoreRadius));
}

float FGraveSaltRules::BurstDamage(float Distance, float LevelScale, float Roll, bool bUnpaid)
{
	const float Share = Falloff(Distance);
	if (Share <= 0.f)
	{
		return 0.f;
	}
	// Like any hit, a spread of numbers about the listed damage; a burst finds no critical spot.
	const float Listed = BaseDamage * Share * FMath::Max(LevelScale, 0.f) * (bUnpaid ? UnpaidScale : 1.f);
	return LooterCombat::HitDamage(Listed, false, Roll);
}

float FGraveSaltRules::CapForBoss(float Damage, float MaxHealth)
{
	return FMath::Min(Damage, FMath::Max(MaxHealth, 0.f) * BossShareCap);
}

FVector FGraveSaltRules::NearestPoint(const FVector& Point, const FGraveSaltBody& Body)
{
	// An upright capsule: the nearest point of its middle segment, then out by its radius toward Point.
	const float Half = FMath::Max(Body.HalfHeight - Body.Radius, 0.f);
	const FVector Low = Body.Center - FVector(0.f, 0.f, Half);
	const FVector High = Body.Center + FVector(0.f, 0.f, Half);
	const FVector OnSpine = FMath::ClosestPointOnSegment(Point, Low, High);
	const FVector Out = Point - OnSpine;
	const float Gap = static_cast<float>(Out.Size());
	if (Gap <= Body.Radius)
	{
		return Point;
	}
	return OnSpine + Out / Gap * Body.Radius;
}

float FGraveSaltRules::DistanceToBody(const FVector& Point, const FGraveSaltBody& Body)
{
	return static_cast<float>(FVector::Dist(Point, NearestPoint(Point, Body)));
}

float FGraveSaltRules::KickShare(float Distance)
{
	const float Out = FMath::Max(Distance, 0.f);
	if (Out <= KickFullDistance)
	{
		return 1.f;
	}
	if (Out >= KickRadius)
	{
		return 0.f;
	}
	// Falling off as the square of what's left, so a burst across a field is a nudge, not a jolt.
	const float Left = 1.f - (Out - KickFullDistance) / (KickRadius - KickFullDistance);
	return Left * Left;
}

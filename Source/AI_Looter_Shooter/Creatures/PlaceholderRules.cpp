#include "Creatures/PlaceholderRules.h"

namespace
{
	// Breathing at rest, and a walk's bob and sway (heights as shares, cm, degrees, radians a second).
	constexpr float BreathDepth = 0.025f;
	constexpr float BreathRate = 2.2f;
	constexpr float BobDepth = 0.07f;
	constexpr float BobLift = 6.f;
	constexpr float StepRate = 9.f;
	constexpr float SwayDegrees = 4.f;
	constexpr float SwayRate = 4.5f;
	// The tell: how low a melee kind squashes (a big one lower), how high a shot rears, how far each leans back.
	constexpr float TellSquash = 0.72f;
	constexpr float BigTellSquash = 0.62f;
	constexpr float ShotRear = 1.18f;
	constexpr float TellLean = -8.f;
	// The strike: its stretch and lunge (share of its length) and lean forward; a shot's recoil (cm).
	constexpr float StrikeStretch = 1.22f;
	constexpr float LungeShare = 0.3f;
	constexpr float StrikeLean = 12.f;
	constexpr float ShotRecoil = 10.f;
	// A flyer bobs on its hover.
	constexpr float HoverBob = 8.f;
	constexpr float HoverRate = 3.f;
	// Its notice: a squat and a little hop.
	constexpr float NoticeSquat = 0.15f;
	constexpr float NoticeHop = 10.f;
	// Dead: flat by FlattenSeconds; sunk SinkShare of its height over SinkSeconds after its corpse time.
	constexpr float DeadSquash = 0.3f;
	constexpr float FlattenSeconds = 0.45f;
	constexpr float SinkSeconds = 2.5f;
	constexpr float SinkShare = 0.6f;

	float Ease(float T)
	{
		const float X = FMath::Clamp(T, 0.f, 1.f);
		return X * X * (3.f - 2.f * X);
	}

	/** Through an attack its statuses show this share (its tell must read). */
	constexpr float StatusAttackShare = 0.3f;

	/**
	 * Its element statuses on the clay (the "Elements matter" job): a burn rocks it side to side and hops it about, a jolt
	 * holds it tall and trembling and its burst jerks it up, the rot slumps it forward and its cough heaves it, salt sits it
	 * lower, a smite shrinks it back from the light, shivering. Share: how much of it shows.
	 */
	void AddStatus(FPlaceholderPose& Pose, const FCreatureStatusPose& Status, float Share)
	{
		if (!Status.IsAny() || Share <= 0.f)
		{
			return;
		}
		const float Burn = Status.Burn * Share;
		const float Jolt = Status.Jolt * Share;
		const float Jerk = Status.JoltJerk * Share;
		const float Rot = Status.Rot * Share;
		const float Cough = Status.Cough * Share;
		const float Salt = Status.Salt * Share;
		const float Smite = Status.Smite * Share;
		// Burning.
		Pose.LeanRoll += 7.f * Burn * Status.Flail();
		Pose.LeanPitch += 4.f * Burn * Status.Flail(1.3f);
		Pose.Squash += 0.06f * Burn * Status.Flail(2.f);
		Pose.Lift += 4.f * Burn * FMath::Max(Status.Flail(0.7f), 0.f);
		// Jolted.
		Pose.Squash += (0.08f + 0.02f * Status.Tremor()) * Jolt + 0.15f * Jerk;
		Pose.LeanRoll += 1.5f * Jolt * Status.Tremor(1.3f);
		Pose.Lift += 8.f * Jerk;
		// Rotting.
		Pose.Squash -= 0.1f * Rot + 0.08f * Cough;
		Pose.LeanPitch += 6.f * Rot + 10.f * Cough;
		Pose.Lunge += 3.f * Cough;
		// Salted.
		Pose.Squash -= 0.03f * Salt;
		// Smitten.
		Pose.Squash -= 0.1f * Smite;
		Pose.LeanPitch -= 6.f * Smite;
		Pose.LeanRoll += 2.f * FMath::Square(Smite) * Status.Wave(9.f);
	}
}

FText PlaceholderRules::TagName(const FText& Name, const FText& PlainType)
{
	if (PlainType.IsEmpty())
	{
		return Name;
	}
	if (Name.IsEmpty())
	{
		return PlainType;
	}
	return FText::Format(NSLOCTEXT("Placeholder", "TagName", "{0} ({1})"), Name, PlainType);
}

FVector PlaceholderRules::BodyStretch(float Length, float Width, float Height)
{
	return FVector(FMath::Max(Length, 1.f) / BlobFoot, FMath::Max(Width, 1.f) / BlobFoot, FMath::Max(Height, 1.f) / BlobHeight);
}

void PlaceholderRules::CapsuleFor(float Length, float Width, float Height, float& OutRadius, float& OutHalfHeight)
{
	OutRadius = FMath::Clamp(FMath::Min(Length, Width) * 0.4f, 18.f, 150.f);
	OutHalfHeight = FMath::Max(OutRadius, Height * 0.5f);
}

FVector PlaceholderRules::HeadSpot(float Length, float Height, float HeadHeight, float HeadForward)
{
	return FVector(Length * 0.5f * HeadForward, 0.f, Height * HeadHeight);
}

FVector PlaceholderRules::SquashScale(float Z)
{
	const float Side = 1.f / FMath::Sqrt(FMath::Max(Z, 0.05f));
	return FVector(Side, Side, Z);
}

FPlaceholderPose PlaceholderRules::PoseAt(const FPlaceholderPoseInput& In)
{
	FPlaceholderPose Pose;
	const bool bFlyer = In.Role == EPlaceholderRole::Flyer;
	const float Speed = FMath::Clamp(In.Speed01, 0.f, 1.f);

	if (In.bDead)
	{
		// It slumps flat where it fell (a flyer drops out of the air), lies there, then sinks into the ground.
		const float Flat = Ease(In.DeadTime / FlattenSeconds);
		Pose.Squash = FMath::Lerp(1.f, DeadSquash, Flat);
		Pose.Lift = bFlyer ? In.Hover * (1.f - Flat) : 0.f;
		const float Sink = FMath::Clamp((In.DeadTime - In.CorpseTime) / SinkSeconds, 0.f, 1.f);
		Pose.Lift -= In.Height * SinkShare * Sink;
		return Pose;
	}

	Pose.Squash = 1.f + BreathDepth * FMath::Sin(In.Clock * BreathRate);
	const float Step = FMath::Abs(FMath::Sin(In.Clock * StepRate));
	Pose.Squash += BobDepth * Speed * (Step - 0.5f);
	Pose.Lift = BobLift * Speed * Step;
	Pose.LeanRoll = SwayDegrees * Speed * FMath::Sin(In.Clock * SwayRate);
	if (bFlyer)
	{
		Pose.Lift = In.Hover + HoverBob * FMath::Sin(In.Clock * HoverRate);
	}

	if (In.bAttacking)
	{
		if (In.AttackTime < In.Windup)
		{
			// The tell, growing to the strike: long enough to read and step away from.
			const float Tell = Ease(In.AttackTime / FMath::Max(In.Windup, 0.05f));
			if (In.bShot)
			{
				Pose.Squash = FMath::Lerp(1.f, ShotRear, Tell);
			}
			else
			{
				Pose.Squash = FMath::Lerp(1.f, In.Role == EPlaceholderRole::Big ? BigTellSquash : TellSquash, Tell);
			}
			Pose.LeanPitch = TellLean * Tell;
			if (bFlyer)
			{
				Pose.Lift = In.Hover * (1.f - Tell);
			}
		}
		else
		{
			// The strike, then easing back.
			const float Back = Ease((In.AttackTime - In.Windup) / FMath::Max(In.Recovery, 0.05f));
			if (In.bShot)
			{
				Pose.Squash = FMath::Lerp(0.85f, 1.f, Back);
				Pose.Lunge = -ShotRecoil * (1.f - Back);
			}
			else
			{
				Pose.Squash = FMath::Lerp(StrikeStretch, 1.f, Back);
				Pose.Lunge = In.Length * LungeShare * (1.f - Back);
				Pose.LeanPitch = StrikeLean * (1.f - Back);
			}
			if (bFlyer)
			{
				Pose.Lift = In.Hover * Back;
			}
		}
	}

	const float Notice = FMath::Clamp(In.Notice, 0.f, 1.f);
	Pose.Squash *= 1.f - NoticeSquat * Notice;
	Pose.Lift += NoticeHop * Notice;
	// Rising into the world, it comes up out of the ground.
	Pose.Lift -= (In.Height + In.Hover) * FMath::Clamp(In.EntranceLeft, 0.f, 1.f);
	// Its element statuses on top.
	AddStatus(Pose, In.Status, In.bAttacking ? StatusAttackShare : 1.f);
	return Pose;
}

FVector PlaceholderRules::LeadAim(const FVector& From, const FVector& TargetAt, const FVector& TargetVelocity, float ShotSpeed,
	float LeadShare)
{
	const float Flight = static_cast<float>(FVector::Dist(From, TargetAt)) / FMath::Max(ShotSpeed, 1.f);
	const FVector Flat(TargetVelocity.X, TargetVelocity.Y, 0.0);
	return TargetAt + Flat * Flight * FMath::Clamp(LeadShare, 0.f, 1.f);
}

EPlaceholderKeep PlaceholderRules::ChooseKeep(float Distance, float BackOff, float Hold, bool bHasSpot, float SpotGap, float Arrive)
{
	if (Distance > Hold)
	{
		return EPlaceholderKeep::None;
	}
	if (Distance < BackOff)
	{
		return EPlaceholderKeep::BackOff;
	}
	return bHasSpot && SpotGap > Arrive ? EPlaceholderKeep::ToSpot : EPlaceholderKeep::Hold;
}

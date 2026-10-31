#include "Creatures/SpiderAmbushRules.h"
#include "HAL/IConsoleManager.h"

// The spiders' ambush rules (SpiderAmbushRules.h). The Looter.Spider.Lurk.* cvars' defaults are FSpiderLurkTuning's own field
// values, so the tests (a default-made tuning) and a fresh game agree.

namespace
{
	const FSpiderLurkTuning LurkDefaults;

	TAutoConsoleVariable<int32> CVarLurk(TEXT("Looter.Spider.Lurk"), LurkDefaults.Enabled,
		TEXT("1: a lurk ambush's spiders wait on their walls, ledges and trees and spring as the player passes; 0: they spring as the ambush springs."),
		ECVF_Default);
	TAutoConsoleVariable<float> CVarSpringRadius(TEXT("Looter.Spider.Lurk.SpringRadius"), LurkDefaults.SpringRadius,
		TEXT("A lurker springs on a player it sees within this (cm, flat from under its perch)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarPassRadius(TEXT("Looter.Spider.Lurk.PassRadius"), LurkDefaults.PassRadius,
		TEXT("Within this (cm) a player who has passed it, stares at it or lingers springs it."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLeapReach(TEXT("Looter.Spider.Lurk.LeapReach"), LurkDefaults.LeapReach,
		TEXT("A lurker's leap reaches this far at most (cm, at size 1); farther it climbs down instead."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCoilSeconds(TEXT("Looter.Spider.Lurk.CoilSeconds"), LurkDefaults.CoilSeconds,
		TEXT("A lurker's crouch and shriek before it leaps (s): the player's warning."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLeapBiteShare(TEXT("Looter.Spider.Lurk.LeapBiteShare"), LurkDefaults.LeapBiteShare,
		TEXT("A leap landing on the player bites for this share of its bite."), ECVF_Default);

	/** A wall: its normal no more upright than this (a cosine), and leaning out over the ground no more than OverhangMost. */
	constexpr float WallMostUp = 0.75f;
	constexpr float OverhangMost = -0.6f;
	/** A ledge's top at least this upright. */
	constexpr float LedgeLeastUp = 0.75f;
	/** A leap's flight (s): this, this much more per cm, within these. */
	constexpr float LeapSecondsBase = 0.42f;
	constexpr float LeapSecondsPerCm = 0.00025f;
	constexpr float LeapSecondsRange[2] = { 0.45f, 0.85f };
	/** A landing bites only a player within this far above or below it (cm). */
	constexpr float LandingBiteHeight = 200.f;
}

FSpiderLurkTuning FSpiderLurkTuning::Live()
{
	FSpiderLurkTuning Tuning;
	Tuning.Enabled = CVarLurk.GetValueOnGameThread();
	Tuning.SpringRadius = FMath::Max(CVarSpringRadius.GetValueOnGameThread(), 0.f);
	Tuning.PassRadius = FMath::Max(CVarPassRadius.GetValueOnGameThread(), 0.f);
	Tuning.LeapReach = FMath::Max(CVarLeapReach.GetValueOnGameThread(), 0.f);
	Tuning.CoilSeconds = FMath::Max(CVarCoilSeconds.GetValueOnGameThread(), 0.f);
	Tuning.LeapBiteShare = FMath::Max(CVarLeapBiteShare.GetValueOnGameThread(), 0.f);
	return Tuning;
}

namespace SpiderAmbushRules
{
	ESpiderSurface SurfaceOf(const FVector& Normal)
	{
		const FVector Up = Normal.GetSafeNormal();
		if (Up.IsNearlyZero())
		{
			return ESpiderSurface::None;
		}
		if (Up.Z >= LedgeLeastUp)
		{
			return ESpiderSurface::Ledge;
		}
		if (Up.Z > WallMostUp)
		{
			// A slope too steep to stand on as a ledge and too gentle to hang on as a wall: a lurker crouching there would read
			// as sliding off. (WallMostUp and LedgeLeastUp meet today; kept apart so either can be tuned.)
			return ESpiderSurface::None;
		}
		if (Up.Z >= 0.f)
		{
			return ESpiderSurface::Wall;
		}
		return Up.Z >= OverhangMost ? ESpiderSurface::Overhang : ESpiderSurface::None;
	}

	FVector FaceWay(float FaceYaw)
	{
		const float Radians = FMath::DegreesToRadians(FaceYaw);
		return FVector(FMath::Cos(Radians), FMath::Sin(Radians), 0.f);
	}

	FVector ClingForward(const FVector& Normal, ESpiderPerchKind Kind, float FaceYaw)
	{
		const FVector Up = Normal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
		FVector Way;
		if (Kind != ESpiderPerchKind::Ledge && SurfaceOf(Up) != ESpiderSurface::Ledge)
		{
			// Head down: straight down the surface, the way it would drop.
			Way = FVector::DownVector - Up * FVector::DotProduct(FVector::DownVector, Up);
			if (Way.Normalize())
			{
				return Way;
			}
		}
		// Out over the edge the way it was set to look, along the surface.
		Way = FaceWay(FaceYaw);
		Way -= Up * FVector::DotProduct(Way, Up);
		if (!Way.Normalize())
		{
			// Facing straight into the surface: any way along it.
			Way = FVector::CrossProduct(Up, FVector::RightVector).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
		}
		return Way;
	}

	FQuat ClingRotation(const FVector& Normal, const FVector& Forward)
	{
		return FRotationMatrix::MakeFromXZ(Forward, Normal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector)).ToQuat();
	}

	FVector TurnWatch(const FVector& Current, const FVector& Hang, const FVector& Toward, const FVector& Normal, float MostDegrees,
		float StepDegrees)
	{
		const FVector Up = Normal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
		auto OnSurface = [&Up](const FVector& Way, const FVector& Fallback)
		{
			FVector Flat = Way - Up * FVector::DotProduct(Way, Up);
			return Flat.Normalize() ? Flat : Fallback;
		};
		const FVector From = OnSurface(Hang, FVector::ForwardVector);
		const FVector Now = OnSurface(Current, From);
		// The wanted angle round the normal from its hang, at most MostDegrees either way.
		auto AngleFrom = [&Up, &From](const FVector& Way)
		{
			return FMath::RadiansToDegrees(FMath::Atan2(FVector::DotProduct(FVector::CrossProduct(From, Way), Up), FVector::DotProduct(From, Way)));
		};
		const float Wanted = FMath::Clamp(AngleFrom(OnSurface(Toward, From)), -MostDegrees, MostDegrees);
		const float Have = AngleFrom(Now);
		const float Next = Have + FMath::Clamp(Wanted - Have, -FMath::Max(StepDegrees, 0.f), FMath::Max(StepDegrees, 0.f));
		return FQuat(Up, FMath::DegreesToRadians(Next)).RotateVector(From);
	}

	bool WantsToSpring(const FSpiderLurkSense& Sense, const FSpiderLurkTuning& Tuning)
	{
		// Shot, called by a mate, or set on the player by its pack: it goes, seen or not.
		if (Sense.bHurt || Sense.bCalled || Sense.bHunting)
		{
			return true;
		}
		// Otherwise only on a player it can see: one who walks by behind a rock never knows it was there.
		if (!Sense.bSees)
		{
			return false;
		}
		const float Under = Sense.SpringRadius > 0.f ? Sense.SpringRadius : Tuning.SpringRadius;
		if (Sense.FlatDistance <= Under)
		{
			return true;
		}
		if (Sense.FlatDistance > FMath::Max(Tuning.PassRadius, Under))
		{
			return false;
		}
		// Near: on their back as they walk on past; or found out (stared at); or they're standing about under it.
		return Sense.bMovingAway || Sense.LookedAtSeconds >= Tuning.SpottedSeconds || Sense.StillSeconds >= Tuning.LingerSeconds;
	}

	bool CanLeap(float Distance, float Rise, bool bSees, float Scale, const FSpiderLurkTuning& Tuning)
	{
		const float Size = FMath::Max(Scale, 0.1f);
		return bSees && Distance <= Tuning.LeapReach * Size && Rise <= Tuning.LeapHighest * Size;
	}

	ESpiderSpring ChooseSpring(const FSpiderLurkSense& Sense, ESpiderPerchKind Kind, float Scale, const FSpiderLurkTuning& Tuning)
	{
		if (CanLeap(Sense.Distance, Sense.Rise, Sense.bSees, Scale, Tuning))
		{
			return ESpiderSpring::Leap;
		}
		// Off a ledge it can only jump; down a wall or a trunk it scrambles, and hunts from the ground.
		return Kind == ESpiderPerchKind::Ledge ? ESpiderSpring::Leap : ESpiderSpring::Climb;
	}

	FVector LeapLanding(const FVector& From, const FVector& PreyFeet, const FVector& PreyVelocity, float Scale, const FSpiderLurkTuning& Tuning)
	{
		const FVector Ahead = PreyFeet + FVector(PreyVelocity.X, PreyVelocity.Y, 0.f) * Tuning.LeadSeconds;
		FVector Way = Ahead - From;
		Way.Z = 0.f;
		const float Flat = static_cast<float>(Way.Size());
		if (Flat < 1.f)
		{
			return FVector(Ahead.X, Ahead.Y, PreyFeet.Z);
		}
		// Short of them along its way: on top of them it would land inside their body.
		const float Short = FMath::Min(Tuning.LandShort * FMath::Max(Scale, 0.1f), Flat * 0.5f);
		const FVector Landing = Ahead - Way / Flat * Short;
		return FVector(Landing.X, Landing.Y, PreyFeet.Z);
	}

	float LeapSeconds(const FVector& From, const FVector& To)
	{
		return FMath::Clamp(LeapSecondsBase + static_cast<float>(FVector::Dist(From, To)) * LeapSecondsPerCm, LeapSecondsRange[0],
			LeapSecondsRange[1]);
	}

	FVector LeapVelocity(const FVector& From, const FVector& To, float Seconds, float GravityZ)
	{
		const float Time = FMath::Max(Seconds, 0.05f);
		// From + V t + g t^2 / 2 = To.
		return (To - From - FVector(0.f, 0.f, 0.5f * GravityZ * Time * Time)) / Time;
	}

	FVector LeapPositionAt(const FVector& From, const FVector& Velocity, float Time, float GravityZ)
	{
		return From + Velocity * Time + FVector(0.f, 0.f, 0.5f * GravityZ * Time * Time);
	}

	bool LandingBites(const FVector& Landed, const FVector& PreyFeet, float Scale, const FSpiderLurkTuning& Tuning)
	{
		const float Size = FMath::Max(Scale, 1.f);
		return FVector::Dist2D(Landed, PreyFeet) <= Tuning.LeapBiteReach * Size && FMath::Abs(Landed.Z - PreyFeet.Z) <= LandingBiteHeight * Size;
	}

	float MateDelay(float Dice01, const FSpiderLurkTuning& Tuning)
	{
		return FMath::Lerp(Tuning.MateDelay[0], Tuning.MateDelay[1], FMath::Clamp(Dice01, 0.f, 1.f));
	}

	bool ClimbDone(float BodyOver, float Scale, const FSpiderLurkTuning& Tuning)
	{
		return BodyOver <= Tuning.ClimbHopHeight * FMath::Max(Scale, 0.1f);
	}
}

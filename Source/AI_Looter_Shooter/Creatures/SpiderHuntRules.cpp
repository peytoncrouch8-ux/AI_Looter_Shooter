#include "Creatures/SpiderHuntRules.h"
#include "HAL/IConsoleManager.h"

// A brown spider's cunning in a fight (SpiderHuntRules.h). The Looter.Spider.Cunning.* cvars' defaults are FSpiderHuntTuning's
// own field values, so the tests (a default-made tuning) and a fresh game agree.

namespace
{
	const FSpiderHuntTuning HuntDefaults;

	TAutoConsoleVariable<int32> CVarCunning(TEXT("Looter.Spider.Cunning"), HuntDefaults.Enabled,
		TEXT("1: brown spiders circle before they lunge, back off when hurt and keep a gap between their bites; 0: as before."),
		ECVF_Default);
	TAutoConsoleVariable<float> CVarBiteSpacing(TEXT("Looter.Spider.Cunning.BiteSpacing"), HuntDefaults.BiteSpacing,
		TEXT("A spider given its turn this soon (s) after a packmate's bite began at the same player circles first."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCircleChance(TEXT("Looter.Spider.Cunning.CircleChance"), HuntDefaults.CircleChance,
		TEXT("Given its turn, a spider circles the player first with this chance."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBackOffShare(TEXT("Looter.Spider.Cunning.BackOffShare"), HuntDefaults.BackOffShare,
		TEXT("A spider hurt by this share of its health in a moment may skitter back."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHitAndRun(TEXT("Looter.Spider.Cunning.HitAndRun"), HuntDefaults.HitAndRunChance,
		TEXT("After a bite a spider skips back out of reach with this chance."), ECVF_Default);

	/** In front of the player within this (a sine of the angle off their facing), its way round is its dice's. */
	constexpr float FrontSine = 0.2f;
	/** Backing off, it veers this much to its side (a share of the way straight back). */
	constexpr float BackOffVeer = 0.35f;
}

FSpiderHuntTuning FSpiderHuntTuning::Live()
{
	FSpiderHuntTuning Tuning;
	Tuning.Enabled = CVarCunning.GetValueOnGameThread();
	Tuning.BiteSpacing = FMath::Max(CVarBiteSpacing.GetValueOnGameThread(), 0.f);
	Tuning.CircleChance = FMath::Clamp(CVarCircleChance.GetValueOnGameThread(), 0.f, 1.f);
	Tuning.BackOffShare = FMath::Max(CVarBackOffShare.GetValueOnGameThread(), 0.f);
	Tuning.HitAndRunChance = FMath::Clamp(CVarHitAndRun.GetValueOnGameThread(), 0.f, 1.f);
	return Tuning;
}

namespace SpiderHuntRules
{
	bool WantsCircle(float Distance, float Scale, float Dice01, bool bCrowded, const FSpiderHuntTuning& Tuning)
	{
		if (bCrowded)
		{
			return true;
		}
		const float Radius = Tuning.CircleRadius * FMath::Max(Scale, 0.1f);
		return Dice01 < Tuning.CircleChance && Distance <= Radius * Tuning.CircleFrom;
	}

	float CircleSeconds(float Dice01, bool bCrowded, const FSpiderHuntTuning& Tuning)
	{
		const float Seconds = FMath::Lerp(Tuning.CircleSeconds[0], Tuning.CircleSeconds[1], FMath::Clamp(Dice01, 0.f, 1.f));
		return bCrowded ? FMath::Max(Seconds, Tuning.BiteSpacing) : Seconds;
	}

	float CircleSide(const FVector& Here, const FVector& Prey, const FVector& PreyFacing, float Dice01)
	{
		FVector Out = Here - Prey;
		Out.Z = 0.f;
		FVector Facing(PreyFacing.X, PreyFacing.Y, 0.f);
		if (!Out.Normalize() || !Facing.Normalize())
		{
			return Dice01 < 0.5f ? -1.f : 1.f;
		}
		// Which side of their facing it stands on; going on that way round takes it toward their back.
		const float Across = static_cast<float>(FVector::CrossProduct(Facing, Out).Z);
		const bool bInFront = FVector::DotProduct(Facing, Out) > 0.0;
		if (bInFront && FMath::Abs(Across) < FrontSine)
		{
			return Dice01 < 0.5f ? -1.f : 1.f;
		}
		return Across >= 0.f ? 1.f : -1.f;
	}

	FVector CircleWay(const FVector& Here, const FVector& Prey, float Side, float Radius, float RadialGain)
	{
		FVector Out = Here - Prey;
		Out.Z = 0.f;
		const float Distance = static_cast<float>(Out.Size());
		if (Distance < 1.f)
		{
			return FVector::ZeroVector;
		}
		Out /= Distance;
		// Along the circle: Out turned a quarter round (the way that turns +X toward +Y), by its side.
		const FVector Along = FVector(-Out.Y, Out.X, 0.f) * (Side < 0.f ? -1.f : 1.f);
		const float Pull = FMath::Clamp((Radius - Distance) / FMath::Max(Radius, 1.f) * RadialGain, -1.f, 1.f);
		return (Along + Out * Pull).GetSafeNormal();
	}

	float FadeHurt(float RecentHurt, float Seconds, const FSpiderHuntTuning& Tuning)
	{
		return RecentHurt * FMath::Exp(-FMath::Max(Seconds, 0.f) / FMath::Max(Tuning.BackOffWindow, 0.01f));
	}

	bool WantsBackOff(float RecentHurt, float SinceSkitter, float Dice01, const FSpiderHuntTuning& Tuning)
	{
		return RecentHurt >= Tuning.BackOffShare && SinceSkitter >= Tuning.BackOffCooldown && Dice01 < Tuning.BackOffChance;
	}

	bool WantsHitAndRun(float SinceSkitter, float Dice01, const FSpiderHuntTuning& Tuning)
	{
		return SinceSkitter >= Tuning.BackOffCooldown * 0.5f && Dice01 < Tuning.HitAndRunChance;
	}

	FVector BackOffWay(const FVector& Here, const FVector& Prey, float Side)
	{
		FVector Back = Here - Prey;
		Back.Z = 0.f;
		if (!Back.Normalize())
		{
			return FVector::ZeroVector;
		}
		const FVector Aside = FVector(-Back.Y, Back.X, 0.f) * (Side < 0.f ? -1.f : 1.f);
		return (Back + Aside * BackOffVeer).GetSafeNormal();
	}
}

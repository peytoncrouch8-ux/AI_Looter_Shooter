// WindowLifeRules: when and how a lived-in house's window shows somebody at home, as numbers.

#include "World/WindowLifeRules.h"

namespace
{
	/** A walker's bob behind the glass: how high (cm) and how many steps a second. */
	constexpr float BobHeight = 1.2f;
	constexpr float StepsPerSecond = 1.8f;

	/** A shadow walking through goes a little slower past the middle of the window (it glances out): how much. */
	constexpr float Glance = 0.06f;

	/** A shadow that stops: the share of its time coming in, and where it starts going back. */
	constexpr float ComeIn = 0.3f;
	constexpr float GoBack = 0.7f;

	/** While it stands at the window it sways this far (cm), this many times a second. */
	constexpr float SwayWidth = 2.f;
	constexpr float SwayRate = 0.35f;

	/** A shadow fades in over this first share of its time as it comes past the pane's edge, and out over the last. */
	constexpr float ShadowFade = 0.15f;

	/** A curtain: the share of its time it's pulled in over, then it falls back and settles; and how it fades at its ends. */
	constexpr float CurtainPull = 0.22f;
	constexpr float CurtainFade = 0.06f;

	/** How far a curtain is pulled at least (a share of its travel). */
	constexpr float CurtainReachMin = 0.6f;

	float EaseOut(float T)
	{
		return 1.f - FMath::Square(1.f - FMath::Clamp(T, 0.f, 1.f));
	}

	float EaseIn(float T)
	{
		return FMath::Square(FMath::Clamp(T, 0.f, 1.f));
	}

	float Bob(float Since)
	{
		return BobHeight * FMath::Sin(UE_TWO_PI * StepsPerSecond * Since);
	}
}

FLinearColor WindowLifeRules::ShadowColor()
{
	return FLinearColor(0.012f, 0.008f, 0.006f, 1.f);
}

FLinearColor WindowLifeRules::CurtainColor(bool bDusk)
{
	// By day the dim room behind the glass; at dusk the linen glows warm with the lamp behind it, a little over the panes'
	// own glow (WindowGlow at dusk, about 0.38, 0.22, 0.14).
	return bDusk ? FLinearColor(0.62f, 0.4f, 0.22f, 1.f) : FLinearColor(0.3f, 0.27f, 0.21f, 1.f);
}

bool WindowLifeRules::Allows(EWindowLifeKind Kind, bool bDusk)
{
	switch (Kind)
	{
	case EWindowLifeKind::Shadow:
		return bDusk;
	case EWindowLifeKind::Curtain:
		return true;
	case EWindowLifeKind::Shutter:
		return bDusk || bShuttersByDay;
	default:
		return false;
	}
}

float WindowLifeRules::WeightOf(EWindowLifeKind Kind)
{
	switch (Kind)
	{
	case EWindowLifeKind::Shadow:
		return ShadowWeight;
	case EWindowLifeKind::Curtain:
		return CurtainWeight;
	case EWindowLifeKind::Shutter:
		return ShutterWeight;
	default:
		return 0.f;
	}
}

EWindowLifeKind WindowLifeRules::PickKind(TConstArrayView<EWindowLifeKind> Kinds, bool bDusk, FRandomStream& Random)
{
	// Each kind once, however many windows offer it: a street of open shutters is no likelier to slam one than a lamplit
	// window is to show a shadow.
	TArray<EWindowLifeKind, TInlineAllocator<4>> Unique;
	float Total = 0.f;
	for (const EWindowLifeKind Kind : Kinds)
	{
		if (Allows(Kind, bDusk) && !Unique.Contains(Kind))
		{
			Unique.Add(Kind);
			Total += WeightOf(Kind);
		}
	}
	if (Unique.IsEmpty() || Total <= 0.f)
	{
		return EWindowLifeKind::None;
	}
	float Roll = Random.FRand() * Total;
	for (const EWindowLifeKind Kind : Unique)
	{
		Roll -= WeightOf(Kind);
		if (Roll < 0.f)
		{
			return Kind;
		}
	}
	return Unique.Last();
}

bool WindowLifeRules::Sees(const FVector& Eyes, const FVector& ViewDirection, const FVector& Window, const FVector& WindowOut)
{
	const FVector ToWindow = Window - Eyes;
	const double Distance = ToWindow.Size();
	if (Distance < NearRadius || Distance > FarRadius)
	{
		return false;
	}
	const FVector Toward = ToWindow / Distance;
	if (FVector::DotProduct(ViewDirection.GetSafeNormal(), Toward) < ViewCosine)
	{
		return false;
	}
	// The glass turned toward the player: never seen edge-on, nor through the house from behind.
	return FVector::DotProduct(WindowOut.GetSafeNormal(), -Toward) >= FacingCosine;
}

FWindowLifeEvent WindowLifeRules::MakeEvent(EWindowLifeKind Kind, bool bDusk, FRandomStream& Random)
{
	FWindowLifeEvent Event;
	Event.Kind = Kind;
	Event.Side = Random.FRand() < 0.5f ? -1.f : 1.f;
	switch (Kind)
	{
	case EWindowLifeKind::Shadow:
		Event.bLingers = Random.FRand() < LingerChance;
		Event.Seconds = Event.bLingers ? Random.FRandRange(LingerSecondsMin, LingerSecondsMax)
			: Random.FRandRange(ShadowSecondsMin, ShadowSecondsMax);
		Event.Color = ShadowColor();
		Event.Density = ShadowDensity;
		break;
	case EWindowLifeKind::Curtain:
		Event.Seconds = Random.FRandRange(CurtainSecondsMin, CurtainSecondsMax);
		Event.Reach = Random.FRandRange(CurtainReachMin, 1.f);
		Event.Color = CurtainColor(bDusk);
		Event.Density = CurtainDensity;
		break;
	case EWindowLifeKind::Shutter:
		Event.Seconds = ShutterSeconds + ShutterDelay;
		break;
	default:
		break;
	}
	return Event;
}

FWindowLifePose WindowLifeRules::PoseAt(const FWindowLifeEvent& Event, float Since)
{
	FWindowLifePose Pose;
	const float Length = FMath::Max(Event.Seconds, 0.1f);
	if (Since < 0.f || Since > Length)
	{
		return Pose;
	}
	const float Alpha = Since / Length;
	if (Event.Kind == EWindowLifeKind::Shadow)
	{
		// Comes past the pane's edge out of the wall and goes the same way: faded at both ends.
		Pose.Strength = Event.Density * FMath::SmoothStep(0.f, ShadowFade, Alpha) * (1.f - FMath::SmoothStep(1.f - ShadowFade, 1.f, Alpha));
		if (!Event.bLingers)
		{
			// Walks through, a little slower past the middle.
			const float Along = Alpha + Glance * FMath::Sin(UE_TWO_PI * Alpha);
			Pose.Slide = Event.Side * FMath::Lerp(SlideFar, -SlideFar, Along);
			Pose.Rise = Bob(Since);
		}
		else if (Alpha < ComeIn)
		{
			Pose.Slide = Event.Side * FMath::Lerp(SlideFar, SlideNear, EaseOut(Alpha / ComeIn));
			Pose.Rise = Bob(Since);
		}
		else if (Alpha < GoBack)
		{
			// Stands at the window a moment, swaying a little: someone looking out at the street.
			Pose.Slide = Event.Side * (SlideNear + SwayWidth * FMath::Sin(UE_TWO_PI * SwayRate * (Since - ComeIn * Length)));
		}
		else
		{
			Pose.Slide = Event.Side * FMath::Lerp(SlideNear, SlideFar, EaseIn((Alpha - GoBack) / (1.f - GoBack)));
			Pose.Rise = Bob(Since);
		}
		return Pose;
	}
	if (Event.Kind == EWindowLifeKind::Curtain)
	{
		// Pulled in quickly, let go: it falls back past where it hung, sways in a touch and settles.
		float Swing = 0.f;
		if (Alpha < CurtainPull)
		{
			Swing = FMath::Sin(UE_HALF_PI * Alpha / CurtainPull);
		}
		else
		{
			const float Back = (Alpha - CurtainPull) / (1.f - CurtainPull);
			Swing = FMath::Square(1.f - Back) * (0.5f + 0.5f * FMath::Cos(2.5f * UE_PI * Back));
		}
		Pose.Swing = Event.Reach * Swing;
		Pose.Slide = Event.Side * (CurtainRest - CurtainTravel * Pose.Swing);
		Pose.Strength = Event.Density * FMath::SmoothStep(0.f, CurtainFade, Alpha) * (1.f - FMath::SmoothStep(1.f - CurtainFade, 1.f, Alpha));
	}
	return Pose;
}

FName WindowLifeRules::CueFor(EWindowLifeKind Kind)
{
	switch (Kind)
	{
	case EWindowLifeKind::Shadow:
		return FName(StepsCue);
	case EWindowLifeKind::Curtain:
		return FName(CurtainCue);
	default:
		return NAME_None;
	}
}

const TCHAR* WindowLifeRules::KindName(EWindowLifeKind Kind)
{
	switch (Kind)
	{
	case EWindowLifeKind::Shadow:
		return TEXT("Shadow");
	case EWindowLifeKind::Curtain:
		return TEXT("Curtain");
	case EWindowLifeKind::Shutter:
		return TEXT("Shutter");
	default:
		return TEXT("None");
	}
}

// ---------------------------------------------------------------------------
// The clock
// ---------------------------------------------------------------------------

bool FWindowLifeClock::MayPlay(FObjectKey House, double Now) const
{
	if (!IsQuiet(Now))
	{
		return false;
	}
	const double* Ready = HouseNext.Find(House);
	return !Ready || Now >= *Ready;
}

void FWindowLifeClock::Played(FObjectKey House, double Now, float Seconds, FRandomStream& Random)
{
	BusyUntil = Now + FMath::Max(Seconds, 0.f);
	NextAny = BusyUntil + Random.FRandRange(WindowLifeRules::GapMin, WindowLifeRules::GapMax);
	HouseNext.Add(House, Now + Random.FRandRange(WindowLifeRules::HouseRestMin, WindowLifeRules::HouseRestMax));
}

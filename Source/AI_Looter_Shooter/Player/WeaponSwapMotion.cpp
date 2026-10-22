#include "Player/WeaponSwapMotion.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"

// The swap's path and timing (WeaponSwapMotion.h). The numbers are the view's, in camera space about the grip; the hands
// follow the gun, so they go down and come up with it. Down is about as deep as the menu's lowered pose (37 cm under
// the hip hold, WeaponHipHold) and tipped further, so the hands leave the view too before the guns change places.

namespace
{
	FWeaponSwapStyle MakeSwapStyle(float HolsterSeconds, float RiseShare, float Overshoot, const FVector& DownOffset,
		const FRotator& DownRotation, const FRotator& Flick)
	{
		FWeaponSwapStyle Style;
		Style.HolsterSeconds = HolsterSeconds;
		Style.RiseShare = RiseShare;
		Style.Overshoot = Overshoot;
		Style.DownOffset = DownOffset;
		Style.DownRotation = DownRotation;
		Style.Flick = Flick;
		return Style;
	}

	/**
	 * The bullpup (and any kind without a style of its own): off the shoulder and down to the right, muzzle first, about a
	 * fifth of a second; up again with a small swing past the hold.
	 */
	const FWeaponSwapStyle RifleSwap = MakeSwapStyle(0.21f, 0.66f, 0.07f, FVector(-6.f, 4.f, -36.f), FRotator(-40.f, 12.f, 26.f),
		FRotator::ZeroRotator);

	/** The pump shotgun, the heaviest: a little slower away, slower up, and more of its weight carried past the hold. */
	const FWeaponSwapStyle ShotgunSwap = MakeSwapStyle(0.24f, 0.72f, 0.09f, FVector(-8.f, 3.f, -38.f), FRotator(-44.f, 10.f, 22.f),
		FRotator::ZeroRotator);

	/**
	 * The Drover: dropped to the hip holster, muzzle down and rolled out; its handling (1.3) makes it the quickest both ways.
	 * It comes up fast, swings well past the hold with a flick of the wrist (a cant that snaps back) and settles: the one
	 * flourish.
	 */
	const FWeaponSwapStyle RevolverSwap = MakeSwapStyle(0.24f, 0.5f, 0.12f, FVector(-3.f, 7.f, -33.f), FRotator(-55.f, 22.f, 40.f),
		FRotator(0.f, -8.f, -45.f));

	/** The SMG, the lightest long gun: snapped off and up again quickest, with a little flick of its own as it comes up. */
	const FWeaponSwapStyle SMGSwap = MakeSwapStyle(0.18f, 0.6f, 0.08f, FVector(-5.f, 5.f, -34.f), FRotator(-38.f, 14.f, 30.f),
		FRotator(0.f, -3.f, -12.f));

	/** The sniper: long and front-heavy, swung down muzzle first and brought up level, steady past the hold. */
	const FWeaponSwapStyle SniperSwap = MakeSwapStyle(0.24f, 0.7f, 0.06f, FVector(-8.f, 3.f, -38.f), FRotator(-46.f, 9.f, 20.f),
		FRotator::ZeroRotator);

	/** The LMG, the heaviest: slow away, slow up, its weight carried well past the hold before it settles. */
	const FWeaponSwapStyle LMGSwap = MakeSwapStyle(0.27f, 0.74f, 0.11f, FVector(-9.f, 3.f, -40.f), FRotator(-42.f, 8.f, 18.f),
		FRotator::ZeroRotator);

	/** The launcher: lifted off the shoulder and dropped, heaved back up onto it. */
	const FWeaponSwapStyle LauncherSwap = MakeSwapStyle(0.27f, 0.72f, 0.1f, FVector(-10.f, 4.f, -40.f), FRotator(-30.f, 10.f, 14.f),
		FRotator::ZeroRotator);

	/** The holster's ease: a third of its pace from the first frame (the key answered at once), quickening out of view. */
	constexpr float HolsterAnswer = 0.35f;
}

const FWeaponSwapStyle& WeaponSwapMotion::StyleOf(EWeaponKind Kind)
{
	switch (Kind)
	{
	case EWeaponKind::Shotgun:
		return ShotgunSwap;
	case EWeaponKind::Revolver:
		return RevolverSwap;
	case EWeaponKind::SMG:
		return SMGSwap;
	case EWeaponKind::Sniper:
		return SniperSwap;
	case EWeaponKind::LMG:
		return LMGSwap;
	case EWeaponKind::Launcher:
		return LauncherSwap;
	case EWeaponKind::Rifle:
	case EWeaponKind::None:
	default:
		return RifleSwap;
	}
}

EWeaponKind WeaponSwapMotion::KindOf(const AWeaponBase* Weapon)
{
	const UWeaponDefinition* Definition = Weapon ? Weapon->GetInstance().Definition.Get() : nullptr;
	return Definition ? Definition->Kind : EWeaponKind::None;
}

float WeaponSwapMotion::HolsterSeconds(EWeaponKind Kind, float Handling)
{
	return FMath::Clamp(StyleOf(Kind).HolsterSeconds / FMath::Max(Handling, 0.1f), MinHolsterSeconds, MaxHolsterSeconds);
}

float WeaponSwapMotion::DrawLowered(EWeaponKind Kind, float Ready)
{
	const FWeaponSwapStyle& Style = StyleOf(Kind);
	const float Time = FMath::Clamp(Ready, 0.f, 1.f);
	const float Rise = FMath::Clamp(Style.RiseShare, 0.05f, 1.f);
	if (Time < Rise)
	{
		// Swung up fast and easing off (a cubic ease-out), on past the hold by the overshoot...
		const float Left = 1.f - Time / Rise;
		return 1.f - (1.f + Style.Overshoot) * (1.f - Left * Left * Left);
	}
	// ...then settled back onto it, eased both ways (no speed where the two meet, so no kink).
	const float Settle = Rise < 1.f ? (Time - Rise) / (1.f - Rise) : 1.f;
	return -Style.Overshoot * (1.f - FMath::SmoothStep(0.f, 1.f, Settle));
}

float WeaponSwapMotion::HolsterLowered(float From, float Time)
{
	const float T = FMath::Clamp(Time, 0.f, 1.f);
	const float Ease = HolsterAnswer * T + (1.f - HolsterAnswer) * T * T;
	return From + (1.f - From) * Ease;
}

void WeaponSwapMotion::ViewPose(EWeaponKind Kind, float Lowered, FVector& OutOffset, FRotator& OutRotation)
{
	const FWeaponSwapStyle& Style = StyleOf(Kind);
	if (Lowered >= 0.f)
	{
		// Going down it tips first and drops after; coming up it rises still tipped and swings level last, which is what
		// makes a draw read as a swing up into place rather than a lift.
		const float Down = FMath::Min(Lowered, 1.f);
		const float Tip = 1.f - FMath::Square(1.f - Down);
		const float Drop = Down * FMath::Sqrt(Down);
		OutOffset = Style.DownOffset * Drop;
		OutRotation = Style.DownRotation * Tip;
		return;
	}
	// Past the hold (a draw's overshoot): the tip carried on the way it was turning (muzzle up, a little to the left), at
	// the same rate as it arrived so there's no kink, a little lift, and the kind's flourish.
	const float Past = FMath::Min(-Lowered, 0.5f);
	OutOffset = Style.DownOffset * (-0.5f * Past);
	OutRotation = Style.DownRotation * (-2.f * Past) + Style.Flick * Past;
}

float WeaponSwapMotion::BodyShare(float Lowered)
{
	return FMath::Clamp(Lowered, 0.f, 1.f);
}

// ---------------------------------------------------------------------------
// The holster's clock
// ---------------------------------------------------------------------------

void FWeaponSwapClock::Lower(double Now, float InFrom, float Seconds)
{
	From = FMath::Clamp(InFrom, -0.5f, 1.f);
	Whole = FMath::Max(Seconds, 0.f);
	// Only the rest of the way down: a gun still low from its draw is out of view sooner.
	Leg = Whole * (1.f - From);
	Start = Now;
	bLowering = true;
	bRunning = true;
}

void FWeaponSwapClock::Raise(double Now)
{
	if (!bRunning)
	{
		return;
	}
	From = FMath::Clamp(GetLowered(Now), 0.f, 1.f);
	Leg = Whole * FMath::Clamp(From, WeaponSwapMotion::MinRaiseShare, 1.f);
	Start = Now;
	bLowering = false;
}

float FWeaponSwapClock::Progress(double Now) const
{
	return Leg > 0.f ? FMath::Clamp(static_cast<float>((Now - Start) / Leg), 0.f, 1.f) : 1.f;
}

float FWeaponSwapClock::GetLowered(double Now) const
{
	if (!bRunning)
	{
		return 0.f;
	}
	const float Time = Progress(Now);
	// Back up eased both ways: it was heading down, so it turns round gently instead of snapping up, and settles on its hold.
	return bLowering ? WeaponSwapMotion::HolsterLowered(From, Time) : From * (1.f - FMath::SmoothStep(0.f, 1.f, Time));
}

float FWeaponSwapClock::SecondsLeft(double Now) const
{
	return bRunning ? FMath::Max(Leg - static_cast<float>(Now - Start), 0.f) : 0.f;
}

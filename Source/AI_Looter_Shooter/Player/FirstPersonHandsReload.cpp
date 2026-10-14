#include "Player/FirstPersonHandsMotion.h"
#include "Player/FirstPersonHandsGrips.h"
#include "Weapons/ReloadMotion.h"

// The support hand's reloads: each path is a run of spots on the gun (or a belt pouch) eased between, timed by the gun's
// own reload steps (LooterReload::Steps) and, while it holds a moving part, placed on that part's travel as the gun's
// model moves it, so the hand and the magazine, pump or cylinder never part company.

namespace
{
	float Rise(float P, float A, float B)
	{
		return FMath::SmoothStep(A, B, P);
	}

	FHandPlacement Ease(const FHandPlacement& From, const FHandPlacement& To, float Alpha)
	{
		return FHandPlacement::Blend(From, To, Alpha);
	}

	/** When the Nth of a step comes in a reload of Part (its progress), or Fallback if the choreography has none. */
	float StepAt(EWeaponReloadPart Part, EReloadStep Step, int32 Nth, float Fallback)
	{
		int32 Seen = 0;
		for (const FReloadStepAt& At : LooterReload::Steps(Part))
		{
			if (At.Step == Step)
			{
				if (Seen == Nth)
				{
					return At.Progress;
				}
				++Seen;
			}
		}
		return Fallback;
	}

	/** Like a spot on the gun, but down at a belt pouch (camera space): off the view, so only its way there is seen. */
	FHandPlacement AtPouch(const FHandPlacement& Like, const FVector& Pouch)
	{
		FHandPlacement Placement = Like;
		Placement.Hold = Pouch;
		return Placement;
	}

	// The magazine's reload (ReloadMotion.cpp moves the magazine: released at 0.16, clear of the well at 0.28, out of
	// sight by 0.40, the fresh one up from 0.46, seated on the MagIn step, the charging handle on the Bolt step).
	/** On the magazine before its release, and the moment it drops out of the hand. */
	constexpr float MagazineReach = 0.13f;
	constexpr float MagazineLetGo = 0.30f;
	/** At the pouch, and away from it with the fresh one. */
	constexpr float MagazineAtPouch = 0.40f;
	constexpr float MagazineLeavePouch = 0.43f;
	/** Round the fresh one as it rises into the well. */
	constexpr float MagazineOnFresh = 0.50f;
	/** The handle is taken this long before its step and let go this long after. */
	constexpr float HandleBefore = 0.02f;
	constexpr float HandleAfter = 0.10f;
	/** Back on the handguard by here. */
	constexpr float MagazineHomeBy = 0.95f;

	/** The fresh magazine's travel where it first shows under the well (LooterReload::MagazineTravel, found once). */
	float FreshMagazineStart()
	{
		static const float Start = []
		{
			bool bWasShown = true;
			for (int32 Step = 0; Step <= 500; ++Step)
			{
				bool bShown = true;
				const float Travel = LooterReload::MagazineTravel(Step / 500.f, bShown);
				if (bShown && !bWasShown)
				{
					return Travel;
				}
				bWasShown = bShown;
			}
			return 26.f;
		}();
		return Start;
	}

	float MagazineTravel(float Progress)
	{
		bool bShown = true;
		return LooterReload::MagazineTravel(Progress, bShown);
	}

	/** The fresh magazine's travel: waiting where it will first show while it's still hidden, so the hand meets it there. */
	float FreshTravel(float Progress)
	{
		bool bShown = true;
		const float Travel = LooterReload::MagazineTravel(Progress, bShown);
		return bShown ? Travel : FreshMagazineStart();
	}

	FHandPlacement MagazinePath(const FHandsGun& Gun, float P)
	{
		using namespace FirstPersonHandsMotion;
		const FHandPlacement Home = FirstPersonHandsGrips::OnForegrip(Gun);
		const float Seated = StepAt(EWeaponReloadPart::Magazine, EReloadStep::MagIn, 0, 0.62f);
		const float HandleOn = StepAt(EWeaponReloadPart::Magazine, EReloadStep::Bolt, 0, 0.72f) - HandleBefore;
		const float HandleOff = HandleOn + HandleBefore + HandleAfter;
		if (P < MagazineReach)
		{
			return Ease(Home, OnMagazine(Gun, MagazineTravel(P)), Rise(P, 0.f, MagazineReach));
		}
		if (P < MagazineLetGo)
		{
			return OnMagazine(Gun, MagazineTravel(P));
		}
		const FHandPlacement LetGo = OnMagazine(Gun, MagazineTravel(MagazineLetGo));
		const FHandPlacement Pouch = AtPouch(LetGo, MagazinePouch());
		if (P < MagazineAtPouch)
		{
			return Ease(LetGo, Pouch, Rise(P, MagazineLetGo, MagazineAtPouch));
		}
		if (P < MagazineLeavePouch)
		{
			return Pouch;
		}
		if (P < MagazineOnFresh)
		{
			return Ease(Pouch, OnMagazine(Gun, FreshTravel(P)), Rise(P, MagazineLeavePouch, MagazineOnFresh));
		}
		if (P < Seated)
		{
			return OnMagazine(Gun, FreshTravel(P));
		}
		if (P < HandleOn)
		{
			return Ease(OnMagazine(Gun, 0.f), OnHandle(Gun, 0.f), Rise(P, Seated, HandleOn));
		}
		if (P < HandleOff)
		{
			// Yanked back hard, held a beat, let run home.
			const float Yank = Rise(P, HandleOn, HandleOn + 0.04f) * (1.f - Rise(P, HandleOn + 0.07f, HandleOff));
			return OnHandle(Gun, Yank);
		}
		return Ease(OnHandle(Gun, 0.f), Home, Rise(P, HandleOff, MagazineHomeBy));
	}

	// The pump gun's reload: a shell per ShellIn step, then the Pump step's rack (LooterReload::PumpTravel).
	/** Through each shell's turn (its share of the time between shell steps): up to the port, pushed in, held, back down. */
	constexpr float ShellUp = 0.4f;
	constexpr float ShellIn = 0.5f;
	constexpr float ShellHeld = 0.6f;
	/** After the last shell the hand goes for the pump this long after its push. */
	constexpr float PumpReachAfter = 0.01f;

	FHandPlacement PumpPath(const FHandsGun& Gun, float P)
	{
		using namespace FirstPersonHandsMotion;
		TArray<float, TInlineAllocator<8>> Shells;
		for (const FReloadStepAt& At : LooterReload::Steps(EWeaponReloadPart::Pump))
		{
			if (At.Step == EReloadStep::ShellIn)
			{
				Shells.Add(At.Progress);
			}
		}
		const float PumpAt = StepAt(EWeaponReloadPart::Pump, EReloadStep::Pump, 0, 0.8f);
		const FHandPlacement Home = FirstPersonHandsGrips::OnForegrip(Gun);
		const FHandPlacement Pumping = FirstPersonHandsGrips::OnForegrip(Gun, LooterReload::PumpTravel(P));
		if (Shells.IsEmpty())
		{
			return P < PumpAt ? Home : Pumping;
		}
		const float Half = Shells.Num() > 1 ? (Shells[1] - Shells[0]) * 0.5f : 0.07f;
		const FHandPlacement Belt = AtPouch(AtLoadingPort(Gun, 0.f), ShellBelt());
		const float First = Shells[0] - Half;
		const float Last = Shells.Last();
		if (P < First)
		{
			return Ease(Home, Belt, Rise(P, 0.f, First));
		}
		if (P >= Last + PumpReachAfter)
		{
			// From the last shell straight onto the pump, in time to rack it.
			return Ease(AtLoadingPort(Gun, 1.f), Pumping, Rise(P, Last + PumpReachAfter, PumpAt));
		}
		for (int32 Shell = 0; Shell < Shells.Num(); ++Shell)
		{
			const bool bLast = Shell == Shells.Num() - 1;
			if (P >= Shells[Shell] + Half && !bLast)
			{
				continue;
			}
			const float Turn = (P - (Shells[Shell] - Half)) / (2.f * Half);
			if (Turn < ShellUp)
			{
				return Ease(Belt, AtLoadingPort(Gun, 0.f), Rise(Turn, 0.f, ShellUp));
			}
			if (Turn < ShellIn)
			{
				return AtLoadingPort(Gun, Rise(Turn, ShellUp, ShellIn));
			}
			if (Turn < ShellHeld || bLast)
			{
				return AtLoadingPort(Gun, 1.f);
			}
			return Ease(AtLoadingPort(Gun, 1.f), Belt, Rise(Turn, ShellHeld, 1.f));
		}
		return Home;
	}

	// The revolver's reload (as the third-person body's, LooterStancePoseReload.cpp): between the gun's own steps.
	/** On the cylinder this long before its latch clicks. */
	constexpr float CylinderReach = 0.02f;
	/** To the rod and set on it before the Eject step, away after it. */
	constexpr float RodGo = 0.08f;
	constexpr float RodSet = 0.02f;
	constexpr float RodAway = 0.03f;
	/** At the pouch, and away from it. */
	constexpr float PouchGo = 0.09f;
	constexpr float PouchHold = 0.02f;
	/** Behind the cylinder with the loader this long before the RoundsIn step; off to bat it this long after. */
	constexpr float LoaderSet = 0.04f;
	constexpr float BatGo = 0.04f;
	/** Out at the cylinder's side this long before the CylinderIn step, riding it home, then on it this long after. */
	constexpr float BatSet = 0.07f;
	constexpr float BatStay = 0.14f;
	constexpr float CylinderHomeBy = 0.97f;

	FHandPlacement CylinderPath(const FHandsGun& Gun, float P)
	{
		using namespace FirstPersonHandsMotion;
		const EWeaponReloadPart Part = EWeaponReloadPart::Cylinder;
		const float Out = StepAt(Part, EReloadStep::CylinderOut, 0, 0.13f);
		const float Eject = StepAt(Part, EReloadStep::Eject, 0, 0.30f);
		const float RoundsIn = StepAt(Part, EReloadStep::RoundsIn, 0, 0.56f);
		const float In = StepAt(Part, EReloadStep::CylinderIn, 0, 0.72f);
		const FHandPlacement Home = FirstPersonHandsGrips::OnForegrip(Gun);

		const float OnCylinderBy = Out - CylinderReach;
		const float ToRod = Eject - RodGo;
		const float AtRod = Eject - RodSet;
		const float Away = Eject + RodAway;
		const float AtPouchBy = Away + PouchGo;
		const float LeavePouch = AtPouchBy + PouchHold;
		const float AtLoader = RoundsIn - LoaderSet;
		const float ToBat = RoundsIn + BatGo;
		const float AtBat = In - BatSet;
		const float Done = In + BatStay;

		if (P < OnCylinderBy)
		{
			return Ease(Home, OnCylinder(Gun), Rise(P, 0.f, OnCylinderBy));
		}
		if (P < ToRod)
		{
			// The thumb pushes it out on its crane, the hand riding it as it swings.
			return OnCylinder(Gun);
		}
		if (P < AtRod)
		{
			return Ease(OnCylinder(Gun), OnEjectorRod(Gun, 0.f), Rise(P, ToRod, AtRod));
		}
		if (P < Eject)
		{
			// The palm punches the rod, speeding into it: the empties go as it bottoms.
			return OnEjectorRod(Gun, FMath::InterpEaseIn(0.f, 1.f, (P - AtRod) / (Eject - AtRod), 2.f));
		}
		if (P < Away)
		{
			return OnEjectorRod(Gun, 1.f);
		}
		const FHandPlacement Pouch = AtPouch(WithSpeedloader(Gun, 0.f), MagazinePouch());
		if (P < AtPouchBy)
		{
			return Ease(OnEjectorRod(Gun, 1.f), Pouch, Rise(P, Away, AtPouchBy));
		}
		if (P < LeavePouch)
		{
			return Pouch;
		}
		if (P < AtLoader)
		{
			return Ease(Pouch, WithSpeedloader(Gun, 0.f), Rise(P, LeavePouch, AtLoader));
		}
		if (P < RoundsIn)
		{
			return WithSpeedloader(Gun, Rise(P, AtLoader, RoundsIn));
		}
		if (P < ToBat)
		{
			// The loader let go of, its rounds in the chambers.
			return WithSpeedloader(Gun, 1.f);
		}
		if (P < AtBat)
		{
			return Ease(WithSpeedloader(Gun, 1.f), BattingCylinder(Gun), Rise(P, ToBat, AtBat));
		}
		if (P < Done)
		{
			return BattingCylinder(Gun);
		}
		return Ease(BattingCylinder(Gun), Home, Rise(P, Done, CylinderHomeBy));
	}

	// The revolver's cylinder (Drover.py): half its length, the rod's tip ahead of its middle, its radius; where it sits
	// shut, for a gun whose model can't say.
	constexpr float CylinderHalfLength = 2.2f;
	constexpr float RodAhead = 6.75f;
	constexpr float CylinderRadius = 2.15f;
	const FVector ShutCylinderRear(2.1f, 0.f, -1.2f);
	/** A palm on something: the grip point sits this far off the palm, and this far toward the fingers from its middle. */
	constexpr float HoldOffPalm = 2.f;
	constexpr float HoldPastPalm = 2.f;
	/** The palm before the rod's tip, and how far it drives the rod in. */
	constexpr float RodClear = 2.f;
	constexpr float RodPunch = 3.5f;
	/** The speedloader's palm behind the cylinder's rear face, and how far it pushes in. */
	constexpr float LoaderBehind = 5.f;
	constexpr float LoaderPush = 2.f;

	void CylinderOf(const FHandsGun& Gun, FVector& OutRear, FVector& OutAxis)
	{
		OutRear = Gun.bHasCylinder ? Gun.CylinderRear : ShutCylinderRear;
		OutAxis = Gun.bHasCylinder ? Gun.CylinderAxis.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector) : FVector::ForwardVector;
	}

	/** A flat palm with its middle at PalmMiddle (gun space), facing Palm, fingers toward Fingers. */
	FHandPlacement PalmOn(const FHandsGun& Gun, const FVector& PalmMiddle, const FVector& Fingers, const FVector& Palm, EHandGrip Grip)
	{
		const FVector Hold = PalmMiddle + Palm.GetSafeNormal() * HoldOffPalm + Fingers.GetSafeNormal() * HoldPastPalm;
		return FirstPersonHandsGrips::InGun(Gun, Hold, Fingers, Palm, FHandFingers::Of(Grip));
	}
}

FVector FirstPersonHandsMotion::MagazinePouch()
{
	// Low on the left, under the view's bottom edge (the view model's frustum ends about 0.56 down per unit ahead).
	return FVector(22.f, -14.f, -46.f);
}

FVector FirstPersonHandsMotion::ShellBelt()
{
	return FVector(24.f, 9.f, -46.f);
}

FHandPlacement FirstPersonHandsMotion::SupportInReload(const FHandsGun& Gun)
{
	const float P = Gun.ReloadProgress;
	if (P < 0.f)
	{
		return FirstPersonHandsGrips::OnForegrip(Gun);
	}
	switch (Gun.ReloadPart)
	{
	case EWeaponReloadPart::Magazine:
		return MagazinePath(Gun, FMath::Min(P, 1.f));
	case EWeaponReloadPart::Pump:
		return PumpPath(Gun, FMath::Min(P, 1.f));
	case EWeaponReloadPart::Cylinder:
		return CylinderPath(Gun, FMath::Min(P, 1.f));
	default:
		return FirstPersonHandsGrips::OnForegrip(Gun);
	}
}

FHandPlacement FirstPersonHandsMotion::OnMagazine(const FHandsGun& Gun, float Travel)
{
	// Round the magazine from its left, the fingers over its front edge, wherever its travel has it.
	const FGunHands& Hands = FirstPersonHandsGrips::For(Gun.Kind);
	const FVector Seat = Gun.bHasMagazineSeat ? Gun.MagazineSeat : FVector(14.f, 0.f, -5.6f);
	const FVector Hold = Seat + Hands.MagazineGrab - FVector(0.f, 0.f, Travel);
	return FirstPersonHandsGrips::InGun(Gun, Hold, FVector::ForwardVector, FVector::RightVector, FHandFingers::Of(EHandGrip::Wrap));
}

FHandPlacement FirstPersonHandsMotion::OnHandle(const FHandsGun& Gun, float Yank)
{
	// Thumb and fingers pinched on the handle from its left and above, pulled straight back along the gun.
	const FGunHands& Hands = FirstPersonHandsGrips::For(Gun.Kind);
	constexpr float HandlePull = 4.f;
	const FVector Hold = Hands.Handle + FVector(-HandlePull * Yank, -1.6f, 0.4f);
	return FirstPersonHandsGrips::InGun(Gun, Hold, FVector(0.2f, 0.f, 1.f), FVector(0.f, 1.f, -0.3f), FHandFingers::Of(EHandGrip::Pinch));
}

FHandPlacement FirstPersonHandsMotion::AtLoadingPort(const FHandsGun& Gun, float Push)
{
	// Palm up under the receiver, the shell between thumb and fingers, thumbed up into the port.
	const FGunHands& Hands = FirstPersonHandsGrips::For(Gun.Kind);
	constexpr float ShellBelow = 3.f;
	constexpr float ShellPush = 2.f;
	const FVector Hold = Hands.LoadingPort + FVector(0.f, 0.f, -ShellBelow + ShellPush * FMath::Clamp(Push, 0.f, 1.f));
	return FirstPersonHandsGrips::InGun(Gun, Hold, FVector(1.f, 0.15f, 0.f), FVector(0.f, 0.25f, 1.f), FHandFingers::Of(EHandGrip::Pinch));
}

FHandPlacement FirstPersonHandsMotion::OnCylinder(const FHandsGun& Gun)
{
	// Cradled from its left and under it (it swings out to the gun's left), fingers along it, the thumb on its face.
	FVector Rear;
	FVector Axis;
	CylinderOf(Gun, Rear, Axis);
	const FVector Middle = Rear + Axis * CylinderHalfLength;
	return FirstPersonHandsGrips::InGun(Gun, Middle + FVector(0.f, -2.4f, -1.4f), Axis, FVector(0.f, 0.6f, 0.8f), FHandFingers::Of(EHandGrip::Wrap));
}

FHandPlacement FirstPersonHandsMotion::OnEjectorRod(const FHandsGun& Gun, float Punch)
{
	// The flat of the palm on the rod's tip, fingers up, driving it back along the cylinder.
	FVector Rear;
	FVector Axis;
	CylinderOf(Gun, Rear, Axis);
	const FVector Tip = Rear + Axis * (CylinderHalfLength + RodAhead);
	const FVector Palm = Tip + Axis * (RodClear - (RodClear + RodPunch * 0.5f) * FMath::Clamp(Punch, 0.f, 1.f));
	return PalmOn(Gun, Palm, FVector::UpVector, -Axis, EHandGrip::Open);
}

FHandPlacement FirstPersonHandsMotion::WithSpeedloader(const FHandsGun& Gun, float Push)
{
	// Fingertips round the loader behind the open chambers, palm toward them, pushing it in along the cylinder.
	FVector Rear;
	FVector Axis;
	CylinderOf(Gun, Rear, Axis);
	const FVector Palm = Rear - Axis * (LoaderBehind - LoaderPush * FMath::Clamp(Push, 0.f, 1.f));
	return PalmOn(Gun, Palm, FVector::UpVector, Axis, EHandGrip::Wrap);
}

FHandPlacement FirstPersonHandsMotion::BattingCylinder(const FHandsGun& Gun)
{
	// The flat of the hand against the cylinder's left side, riding it as it's batted home into the frame.
	FVector Rear;
	FVector Axis;
	CylinderOf(Gun, Rear, Axis);
	const FVector Middle = Rear + Axis * CylinderHalfLength;
	const FVector Palm = Middle - FVector(0.f, CylinderRadius + 0.3f, 0.f);
	return PalmOn(Gun, Palm, FVector(1.f, 0.f, 0.25f), FVector::RightVector, EHandGrip::Open);
}

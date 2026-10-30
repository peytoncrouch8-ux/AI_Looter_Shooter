#include "Player/FirstPersonHandsMotion.h"
#include "Player/FirstPersonHandsGrips.h"
#include "Player/FirstPersonHandsMarks.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/ReloadTuning.h"

// The support hand's reloads: each path is a run of spots on the gun (or a belt pouch) eased between, timed by the gun's
// own reload steps (LooterReload::Steps) and, while it holds a moving part, placed on that part's travel as the gun's
// model moves it, so the hand and the magazine, pump or cylinder never part company. The spots themselves (on the magazine,
// handle, cylinder, rod, speedloader, the pouches) are in FirstPersonHandsReloadSpots.cpp.

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

	/** A reload of Part's steps as Plan has them (LooterReload::StepsFor: a pump gun's shells; a tactical one's skipped work). */
	TArray<FReloadStepAt> PlannedSteps(EWeaponReloadPart Part, const FReloadPlan& Plan)
	{
		TArray<FReloadStepAt> Steps;
		LooterReload::StepsFor(Part, Plan, Steps);
		return Steps;
	}

	/** When the Nth of a step comes in a reload of Part as planned (its progress), or Fallback if the choreography has none. */
	float StepAt(EWeaponReloadPart Part, const FReloadPlan& Plan, EReloadStep Step, int32 Nth, float Fallback)
	{
		int32 Seen = 0;
		for (const FReloadStepAt& At : PlannedSteps(Part, Plan))
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

	/**
	 * The magazine's path, for the magazine-like reloads too (the SMG's helix, the LMG's box: the same beats, the knob
	 * racked or the belt laid where the rifle's handle is; the sniper's box a little later, and no handle: its trigger
	 * hand works the bolt). P is the reload's progress as the magazine's choreography counts it.
	 */
	FHandPlacement MagazinePath(const FHandsGun& Gun, float P, bool bHandle = true)
	{
		using namespace FirstPersonHandsMotion;
		const FHandPlacement Home = FirstPersonHandsGrips::OnForegrip(Gun);
		// The beats as the full reload has them: a tactical one keeps every beat but skips the handle's.
		const FReloadPlan Full = { true, 0 };
		const float Seated = StepAt(EWeaponReloadPart::Magazine, Full, EReloadStep::MagIn, 0, 0.62f);
		if (!bHandle && P >= Seated)
		{
			return Ease(OnMagazine(Gun, 0.f), Home, Rise(P, Seated, MagazineHomeBy - 0.1f));
		}
		// A tactical magazine or helix reload (a round chambered): the progress jumps the handle (LooterReload::
		// ChoreographyProgress, HandleFrom to HandleTo), so the hand is home from the seat by HandleFrom.
		const bool bSkipsHandle = !Gun.ReloadPlan.bEmpty
			&& (Gun.ReloadPart == EWeaponReloadPart::Magazine || Gun.ReloadPart == EWeaponReloadPart::Helix);
		if (bHandle && bSkipsHandle && P >= Seated)
		{
			return Ease(OnMagazine(Gun, 0.f), Home, Rise(P, Seated, LooterReload::HandleFrom));
		}
		const float HandleOn = StepAt(EWeaponReloadPart::Magazine, Full, EReloadStep::Bolt, 0, 0.72f) - HandleBefore;
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

	// The launcher's reload (ReloadMotion.cpp: the next rocket drawn on the RocketOut step, shown in front of the mouth, slid
	// in until it seats on the RocketIn step): down to the pouch for it, up in front of the mouth with it, pushing it in.
	/** Back up from the pouch with the rocket over this share of the reload, and home after it seats over this. */
	constexpr float RocketBringUp = 0.14f;
	constexpr float RocketLetGo = 0.06f;
	constexpr float RocketHomeBy = 0.92f;

	/** The support hand round the next rocket just behind its nose, Travel cm in front of its seat in the mouth. */
	FHandPlacement OnRocket(const FHandsGun& Gun, float Travel)
	{
		// On the spot the parts mark on the loaded rocket (Reload_L, seated), the next one Travel ahead of it, when they do.
		FHandPlacement Marked;
		if (FirstPersonHandsMarks::Place(Gun, EHandMarkRole::Reload, EFirstPersonHand::Left, Marked, FVector(Travel, 0.f, 0.f)))
		{
			return Marked;
		}
		const FGunHands& Hands = FirstPersonHandsGrips::For(Gun.Kind);
		const FVector Seat = Gun.bHasMagazineSeat ? Gun.MagazineSeat : FVector(90.f, 0.f, 0.f) * Gun.Scale;
		const FVector Hold = Seat + (Hands.RocketGrab + FVector(Travel, 0.f, 0.f)) * Gun.Scale;
		return FirstPersonHandsGrips::InGun(Gun, Hold, FVector::ForwardVector, FVector::RightVector, FHandFingers::Of(EHandGrip::Wrap),
			FVector(-0.55f, -0.5f, -0.65f));
	}

	FHandPlacement RocketPath(const FHandsGun& Gun, float P)
	{
		using namespace FirstPersonHandsMotion;
		const EWeaponReloadPart Part = EWeaponReloadPart::Rocket;
		const float Drawn = StepAt(Part, Gun.ReloadPlan, EReloadStep::RocketOut, 0, 0.14f);
		const float Seated = StepAt(Part, Gun.ReloadPlan, EReloadStep::RocketIn, 0, 0.66f);
		const FHandPlacement Home = FirstPersonHandsGrips::OnForegrip(Gun);
		const FHandPlacement Pouch = AtPouch(OnRocket(Gun, LooterReload::RocketReach), MagazinePouch());
		bool bShown = false;
		bool bSeated = false;
		const float Travel = LooterReload::RocketTravel(P, bShown, bSeated);
		if (P < Drawn)
		{
			return Ease(Home, Pouch, Rise(P, 0.f, Drawn));
		}
		if (!bShown && !bSeated)
		{
			// Up from the pouch to in front of the mouth, the rocket in hand.
			const float ShownAt = Seated - (Seated - Drawn) * 0.6f;
			return Ease(Pouch, OnRocket(Gun, LooterReload::RocketReach), Rise(P, ShownAt - RocketBringUp, ShownAt));
		}
		if (!bSeated)
		{
			return OnRocket(Gun, Travel);
		}
		if (P < Seated + RocketLetGo)
		{
			return OnRocket(Gun, 0.f);
		}
		return Ease(OnRocket(Gun, 0.f), Home, Rise(P, Seated + RocketLetGo, RocketHomeBy));
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
		const TArray<FReloadStepAt> Steps = PlannedSteps(EWeaponReloadPart::Pump, Gun.ReloadPlan);
		bool bPumps = false;
		for (const FReloadStepAt& At : Steps)
		{
			if (At.Step == EReloadStep::ShellIn)
			{
				Shells.Add(At.Progress);
			}
			bPumps |= At.Step == EReloadStep::Pump;
		}
		const float PumpAt = StepAt(EWeaponReloadPart::Pump, Gun.ReloadPlan, EReloadStep::Pump, 0, 0.8f);
		const FHandPlacement Home = FirstPersonHandsGrips::OnForegrip(Gun);
		const FHandPlacement Pumping = FirstPersonHandsGrips::OnForegrip(Gun, LooterReload::PumpTravel(P, Gun.ReloadPlan));
		if (Shells.IsEmpty())
		{
			return P < PumpAt ? Home : Pumping;
		}
		// Half a shell's turn: from the plan's seconds when it has them (one shell has no neighbour to measure by).
		const ReloadTuning::FShellTiming Timing = ReloadTuning::ShellTiming();
		const float PlannedSeconds = ReloadTuning::ShellReloadSeconds(Gun.ReloadPlan, Timing);
		const float Half = Shells.Num() > 1 ? (Shells[1] - Shells[0]) * 0.5f
			: Gun.ReloadPlan.Shells > 0 && PlannedSeconds > 0.f ? 0.5f * Timing.Shell / PlannedSeconds : 0.07f;
		const FHandPlacement Belt = AtPouch(AtLoadingPort(Gun, 0.f), ShellBelt());
		const float First = Shells[0] - Half;
		const float Last = Shells.Last();
		if (P < First)
		{
			return Ease(Home, Belt, Rise(P, 0.f, First));
		}
		if (!bPumps && P >= Last + PumpReachAfter)
		{
			// A round was chambered (tactical): no pump, the hand goes from the last shell back to the fore-end.
			return Ease(AtLoadingPort(Gun, 1.f), Home, Rise(P, Last + PumpReachAfter, FMath::Min(Last + Half, 0.98f)));
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
		const float Out = StepAt(Part, Gun.ReloadPlan, EReloadStep::CylinderOut, 0, 0.13f);
		const float Eject = StepAt(Part, Gun.ReloadPlan, EReloadStep::Eject, 0, 0.30f);
		const float RoundsIn = StepAt(Part, Gun.ReloadPlan, EReloadStep::RoundsIn, 0, 0.56f);
		const float In = StepAt(Part, Gun.ReloadPlan, EReloadStep::CylinderIn, 0, 0.72f);
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
}

FHandPlacement FirstPersonHandsMotion::SupportInReload(const FHandsGun& Gun)
{
	const float P = Gun.ReloadProgress;
	if (P < 0.f)
	{
		// A pump gun racked between shots: the hand rides the fore-end back and home.
		return FirstPersonHandsGrips::OnForegrip(Gun, Gun.PumpCycle >= 0.f ? LooterReload::PumpCycleTravel(Gun.PumpCycle) : 0.f);
	}
	switch (Gun.ReloadPart)
	{
	case EWeaponReloadPart::Magazine:
		return MagazinePath(Gun, FMath::Min(P, 1.f));
	case EWeaponReloadPart::Pump:
		return PumpPath(Gun, FMath::Min(P, 1.f));
	case EWeaponReloadPart::Cylinder:
		return CylinderPath(Gun, FMath::Min(P, 1.f));
	case EWeaponReloadPart::Helix:
	case EWeaponReloadPart::Box:
		return MagazinePath(Gun, FMath::Min(P, 1.f));
	case EWeaponReloadPart::Bolt:
		// The box's beats come a little later (the bolt opens first, by the trigger hand); the support hand only swaps it.
		return MagazinePath(Gun, FMath::Min(P, 1.f) - LooterReload::BoltMagazineLag, false);
	case EWeaponReloadPart::Rocket:
		return RocketPath(Gun, FMath::Min(P, 1.f));
	case EWeaponReloadPart::None:
	default:
		return FirstPersonHandsGrips::OnForegrip(Gun);
	}
}

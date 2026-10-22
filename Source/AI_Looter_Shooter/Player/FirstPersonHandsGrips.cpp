#include "Player/FirstPersonHandsGrips.h"
#include "AI_Looter_Shooter.h"
#include "Player/FirstPersonHandsFraming.h"
#include "HAL/IConsoleManager.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/WeaponHipHold.h"

// The hands' holds on each gun kind, read off the gun models (Art/Models/Weapons/<Gun>.py; their u, v, w are X, Z and
// -Y here) and tuned against the game's own frames (2026-10-14: the support glove palm up under the fore-end with its
// forearm in from the lower left, the trigger glove's back and wrist under the gun's right with its forearm in from the
// lower right), and the placements' own math. The tables are the tuning knobs: everything the hands do on a gun starts
// here, and the Looter.Hands.* console variables move them live (Looter.Hands.Dump prints what to bake).
//
// A wrist bends little: each hold's forearm way leaves the wrist near the knuckles' line reversed. Pointing the forearm
// mostly down under a hand that lies flat (the first holds) stood the forearm up the screen like a post.

namespace
{
	/**
	 * The Bullpup (DA_AssaultRifle): the trigger hand round the thumbhole loop's rear post (raked 8 degrees forward at its
	 * foot), held a touch to the right so the wrist rides beside the magazine just behind it; the handle crosses the palm
	 * on a slant, knuckles up, so the wrist sits low and the forearm drops out under the gun's right toward the frame's
	 * bottom right corner. The support hand palm up under the handguard, thumb along its near (left) side pointing ahead
	 * and a little in, fingers up round the far side; its forearm comes in from the lower left.
	 */
	FGunHands MakeRifle()
	{
		FGunHands Hands;
		Hands.GripHandle = FVector(-0.14f, 0.f, 0.99f);
		Hands.GripPalm = FVector(0.15f, -1.f, 0.f);
		Hands.GripTilt = -25.f;
		Hands.GripForearm = FVector(-0.45f, 0.35f, -0.82f);
		Hands.GripHold = FVector(0.f, 0.8f, 0.f);
		// The trigger is 4 cm ahead of the post: a short reach, the finger well bent.
		Hands.TriggerCurl[0] = 30.f;
		Hands.TriggerCurl[1] = 50.f;
		Hands.TriggerCurl[2] = 25.f;
		Hands.ForeHandle = FVector(0.92f, -0.4f, 0.f);
		Hands.ForePalm = FVector(0.f, 0.1f, 1.f);
		// Tuned in game (2026-10-14): steeper from below, so the sleeve's cut end stays under the frame's bottom edge.
		Hands.ForeForearm = FVector(-0.30f, -0.26f, -0.92f);
		Hands.ForeHold = FVector(0.f, 0.6f, 4.5f);
		Hands.ForeGrip = EHandGrip::Forend;
		// The charging handle on the shell's left, ahead of the sight rail (Bullpup.py's fittings).
		Hands.Handle = FVector(41.5f, -1.9f, 3.2f);
		// A 30's middle: its seat is the middle of its top, it runs 7 cm front to back and 18 down; the palm on its left face.
		Hands.MagazineGrab = FVector(1.5f, 1.2f, -8.f);
		return Hands;
	}

	/**
	 * The Ranchhand (DA_PumpShotgun): the trigger hand round the stock's semi-pistol wrist (its index-way up and ahead,
	 * toward the trigger), held as the rifle's; the support hand under the pump, which SOCKET_Foregrip marks the middle of,
	 * as under the rifle's handguard.
	 */
	FGunHands MakeShotgun()
	{
		FGunHands Hands;
		Hands.GripHandle = FVector(0.5f, 0.f, 0.87f);
		Hands.GripPalm = FVector(0.f, -1.f, 0.f);
		Hands.GripTilt = -25.f;
		Hands.GripForearm = FVector(-0.7f, 0.35f, -0.62f);
		Hands.GripHold = FVector(0.f, 1.f, 0.f);
		// The trigger is 12 cm ahead of the grip: the finger reaches out nearly straight.
		Hands.TriggerCurl[0] = 5.f;
		Hands.TriggerCurl[1] = 20.f;
		Hands.TriggerCurl[2] = 10.f;
		Hands.ForeHandle = FVector(0.92f, -0.4f, 0.f);
		Hands.ForePalm = FVector(0.f, 0.1f, 1.f);
		// Tuned in game (2026-10-15, the user: the hand angled oddly and didn't grab the pump): the hand rolled 20 degrees
		// about the pump so it cups it from the lower left, its forearm in line from there (the straight-down forearm bent
		// the wrist square), the pump through the middle of the grip and the fingers closed round it (EHandGrip::Pump);
		// turned 45 degrees in its own plane so the thumb lies straight along the pump's near side toward the muzzle
		// (curled, it read as "a nub").
		Hands.ForeTurn = FRotator(45.f, 20.f, 0.f);
		Hands.ForeForearm = FVector(-0.25f, -0.50f, -0.83f);
		Hands.ForeHold = FVector(0.f, 0.5f, 0.f);
		Hands.ForeGrip = EHandGrip::Pump;
		// Under the receiver, between the trigger guard and the magazine tube.
		Hands.LoadingPort = FVector(15.5f, 0.f, -3.2f);
		return Hands;
	}

	/**
	 * The Drover (DA_Revolver): the trigger hand round the raked grip (it leans back 17 degrees), palm wrapping a little
	 * round its back, knuckles level ahead; the support hand cupped round the shooting hand from the left, thumb up beside
	 * the frame, palm facing in, fingers ahead and round the front of the shooting hand's, its forearm in from the lower left.
	 */
	FGunHands MakeRevolver()
	{
		FGunHands Hands;
		Hands.GripHandle = FVector(0.3f, 0.f, 0.95f);
		Hands.GripPalm = FVector(0.2f, -1.f, 0.f);
		Hands.GripTilt = -20.f;
		Hands.GripForearm = FVector(-0.75f, 0.3f, -0.58f);
		Hands.GripHold = FVector(0.f, 0.3f, 0.f);
		Hands.TriggerCurl[0] = 25.f;
		Hands.TriggerCurl[1] = 50.f;
		Hands.TriggerCurl[2] = 25.f;
		Hands.ForeHandle = FVector(0.3f, 0.f, 0.95f);
		Hands.ForePalm = FVector(-0.2f, 1.f, 0.1f);
		Hands.ForeForearm = FVector(-0.6f, -0.55f, -0.58f);
		Hands.ForeHold = FVector(0.5f, 0.5f, -1.f);
		Hands.ForeGrip = EHandGrip::Support;
		return Hands;
	}

	// The four frontier-tech guns (Phase 12.3), first holds read off their models (Art/Models/Weapons/<Gun>.py) against
	// the Bullpup's tuned one; Looter.Hands.Kind 4-7 tunes them in game and Looter.Hands.Dump prints what to bake.

	/** A pistol grip raked back (the new guns' all are): the hand round it as the rifle's is, its forearm in from the lower right. */
	FGunHands PistolGrip(float TriggerReach)
	{
		FGunHands Hands;
		Hands.GripHandle = FVector(0.3f, 0.f, 0.95f);
		Hands.GripPalm = FVector(0.1f, -1.f, 0.f);
		Hands.GripTilt = -25.f;
		Hands.GripForearm = FVector(-0.55f, 0.35f, -0.76f);
		Hands.GripHold = FVector(0.f, 0.8f, 0.f);
		// The farther the trigger from the grip, the straighter the finger (the rifle's 4 cm: well bent; the shotgun's 12: out).
		const float Bend = FMath::Clamp(1.f - (TriggerReach - 4.f) / 8.f, 0.f, 1.f);
		Hands.TriggerCurl[0] = FMath::Lerp(5.f, 30.f, Bend);
		Hands.TriggerCurl[1] = FMath::Lerp(20.f, 50.f, Bend);
		Hands.TriggerCurl[2] = FMath::Lerp(10.f, 25.f, Bend);
		// Palm up under the fore-end, as on the rifle's handguard.
		Hands.ForeHandle = FVector(0.92f, -0.4f, 0.f);
		Hands.ForePalm = FVector(0.f, 0.1f, 1.f);
		Hands.ForeForearm = FVector(-0.30f, -0.26f, -0.92f);
		Hands.ForeHold = FVector(0.f, 0.6f, 0.f);
		Hands.ForeGrip = EHandGrip::Forend;
		return Hands;
	}

	/**
	 * The Rustler (DA_SMG): the support hand cups the helix under the barrel (SOCKET_Foregrip, its underside), as one holds
	 * a helical magazine; the charging knob is on the left of the upper, the helix taken at its middle.
	 */
	FGunHands MakeSMG()
	{
		FGunHands Hands = PistolGrip(6.f);
		Hands.ForeHold = FVector(0.f, 0.4f, 1.5f);
		Hands.Handle = FVector(26.6f, -3.f, 1.2f);
		Hands.MagazineGrab = FVector(10.f, 0.8f, -2.5f);
		return Hands;
	}

	/**
	 * The Longhorn (DA_Sniper): the trigger hand round the thumbhole's grip, near upright; the support hand under the
	 * fore-end ahead of the box. The bolt turns about the bore at the shroud's back (Longhorn.py's BOLT_AT), its brass knob
	 * out on the right, under the bore.
	 */
	FGunHands MakeSniper()
	{
		FGunHands Hands = PistolGrip(5.f);
		Hands.GripHandle = FVector(0.2f, 0.f, 0.98f);
		Hands.BoltPivot = FVector(13.4f, 0.f, 0.f);
		Hands.Handle = FVector(17.95f, 3.85f, -1.15f);
		Hands.MagazineGrab = FVector(3.f, 1.f, -4.f);
		return Hands;
	}

	/**
	 * The Thresher (DA_LMG): the support hand under the handguard ahead of the belt box; the reload's last work is the belt
	 * laid in the feed and the top cover slapped down (Handle: the cover over the feed tray). The box taken at its middle.
	 */
	FGunHands MakeLMG()
	{
		FGunHands Hands = PistolGrip(5.f);
		Hands.ForeHold = FVector(0.f, 0.6f, 0.5f);
		Hands.Handle = FVector(24.f, -1.5f, 5.2f);
		Hands.MagazineGrab = FVector(6.f, 1.5f, -6.f);
		return Hands;
	}

	/**
	 * The Powderkeg (DA_Launcher): the support hand round the upright front grip (SOCKET_Foregrip), fingers wrapped from the
	 * left, its forearm in from the lower left; the next rocket held just behind its nose as it goes into the mouth.
	 */
	FGunHands MakeLauncher()
	{
		FGunHands Hands = PistolGrip(6.f);
		Hands.ForeHandle = FVector(0.15f, 0.f, 0.99f);
		Hands.ForePalm = FVector(0.f, 1.f, 0.f);
		Hands.ForeForearm = FVector(-0.4f, -0.45f, -0.8f);
		Hands.ForeHold = FVector(0.f, -0.5f, 0.f);
		Hands.ForeGrip = EHandGrip::Wrap;
		Hands.RocketGrab = FVector(6.f, -1.f, -1.f);
		return Hands;
	}

	/** A hand's frame (knuckles, palm) turned in its own frame by Turn (roll about the knuckles, pitch about the palm's facing). */
	void TurnHand(const FRotator& Turn, FVector& InOutFingers, FVector& InOutPalm)
	{
		if (Turn.IsNearlyZero(0.01f))
		{
			return;
		}
		const FQuat Hand = FirstPersonHands::Frame(InOutFingers, InOutPalm) * Turn.Quaternion();
		InOutFingers = Hand.GetAxisX();
		InOutPalm = Hand.GetAxisY();
	}

#if !UE_BUILD_SHIPPING
	// Live tuning in a development build (2026-10-14, Main's screenshot loop): every number nudges the table's.
	TAutoConsoleVariable<int32> CVarHandsKind(TEXT("Looter.Hands.Kind"), 0,
		TEXT("Which gun the Looter.Hands.* tuning moves: 0 every gun, 1 rifle, 2 shotgun, 3 revolver, 4 SMG, 5 sniper, 6 LMG, 7 launcher."));
	TAutoConsoleVariable<float> CVarTriggerRoll(TEXT("Looter.Hands.TriggerPalmRoll"), 0.f, TEXT("Trigger hand: roll about its knuckles' line (degrees)."));
	TAutoConsoleVariable<float> CVarTriggerPitch(TEXT("Looter.Hands.TriggerPalmPitch"), 0.f, TEXT("Trigger hand: turn about its palm's facing (degrees)."));
	TAutoConsoleVariable<float> CVarTriggerYaw(TEXT("Looter.Hands.TriggerPalmYaw"), 0.f, TEXT("Trigger hand: turn about its third axis (degrees)."));
	TAutoConsoleVariable<float> CVarTriggerArmYaw(TEXT("Looter.Hands.TriggerForearmYaw"), 0.f, TEXT("Trigger forearm's way swung right (degrees, the gun's frame)."));
	TAutoConsoleVariable<float> CVarTriggerArmPitch(TEXT("Looter.Hands.TriggerForearmPitch"), 0.f, TEXT("Trigger forearm's way swung up (degrees, the gun's frame)."));
	TAutoConsoleVariable<float> CVarTriggerAhead(TEXT("Looter.Hands.TriggerHoldAhead"), 0.f, TEXT("Trigger hand's grip point ahead along the gun (cm)."));
	TAutoConsoleVariable<float> CVarTriggerRight(TEXT("Looter.Hands.TriggerHoldRight"), 0.f, TEXT("Trigger hand's grip point to the gun's right (cm)."));
	TAutoConsoleVariable<float> CVarTriggerUp(TEXT("Looter.Hands.TriggerHoldUp"), 0.f, TEXT("Trigger hand's grip point up the gun (cm)."));
	TAutoConsoleVariable<float> CVarSupportRoll(TEXT("Looter.Hands.SupportPalmRoll"), 0.f, TEXT("Support hand: roll about its knuckles' line (degrees)."));
	TAutoConsoleVariable<float> CVarSupportPitch(TEXT("Looter.Hands.SupportPalmPitch"), 0.f, TEXT("Support hand: turn about its palm's facing (degrees)."));
	TAutoConsoleVariable<float> CVarSupportYaw(TEXT("Looter.Hands.SupportPalmYaw"), 0.f, TEXT("Support hand: turn about its third axis (degrees)."));
	TAutoConsoleVariable<float> CVarSupportArmYaw(TEXT("Looter.Hands.SupportForearmYaw"), 0.f, TEXT("Support forearm's way swung right (degrees, the gun's frame)."));
	TAutoConsoleVariable<float> CVarSupportArmPitch(TEXT("Looter.Hands.SupportForearmPitch"), 0.f, TEXT("Support forearm's way swung up (degrees, the gun's frame)."));
	TAutoConsoleVariable<float> CVarSupportAhead(TEXT("Looter.Hands.SupportHoldAhead"), 0.f, TEXT("Support hand's grip point ahead along the gun (cm)."));
	TAutoConsoleVariable<float> CVarSupportRight(TEXT("Looter.Hands.SupportHoldRight"), 0.f, TEXT("Support hand's grip point to the gun's right (cm)."));
	TAutoConsoleVariable<float> CVarSupportUp(TEXT("Looter.Hands.SupportHoldUp"), 0.f, TEXT("Support hand's grip point up the gun (cm)."));
	TAutoConsoleVariable<float> CVarSupportFingers(TEXT("Looter.Hands.SupportFingerCurl"), 0.f,
		TEXT("Support hand on the gun: every joint of its four fingers closed this much more (degrees; bake into FHandFingers::Of)."));
	TAutoConsoleVariable<float> CVarSupportThumb(TEXT("Looter.Hands.SupportThumbCurl"), 0.f,
		TEXT("Support hand on the gun: its thumb's three joints closed this much more (degrees; bake into FHandFingers::Of)."));

	/** A forearm's way swung by Yaw (to the right) and Pitch (up), in the gun's frame. */
	FVector SwingForearm(const FVector& Forearm, float Yaw, float Pitch)
	{
		const FRotator Way = Forearm.GetSafeNormal().Rotation();
		const float SwungPitch = FMath::Clamp(static_cast<float>(Way.Pitch) + Pitch, -89.f, 89.f);
		const float SwungYaw = static_cast<float>(Way.Yaw) + Yaw;
		return FRotator(SwungPitch, SwungYaw, 0.f).Vector();
	}

	/** The support hand's fingers on a gun of Kind with the finger and thumb curl tuning on them. */
	void NudgeSupportFingers(EWeaponKind Kind, FHandFingers& Fingers)
	{
		const int32 Wanted = CVarHandsKind.GetValueOnGameThread();
		if (Wanted != 0 && Wanted != static_cast<int32>(Kind))
		{
			return;
		}
		for (int32 Finger = 0; Finger < FHandFingers::Count; ++Finger)
		{
			const float More = Finger == FHandFingers::Thumb ? CVarSupportThumb.GetValueOnGameThread() : CVarSupportFingers.GetValueOnGameThread();
			for (int32 Joint = 0; Joint < FHandFingers::Joints; ++Joint)
			{
				Fingers.Curl[Finger][Joint] += More;
			}
		}
	}

	FString HandsDumpVector(const FVector& Vector)
	{
		return FString::Printf(TEXT("FVector(%.2ff, %.2ff, %.2ff)"), Vector.X, Vector.Y, Vector.Z);
	}

	FString HandsDumpRotator(const FRotator& Rotator)
	{
		return FString::Printf(TEXT("FRotator(%.1ff, %.1ff, %.1ff)"), Rotator.Pitch, Rotator.Yaw, Rotator.Roll);
	}

	/** Looter.Hands.Dump: every gun's hands and hip hold with the tuning on them, as lines to paste into the tables. */
	void DumpHands()
	{
		const TPair<EWeaponKind, const TCHAR*> Kinds[] = {
			{ EWeaponKind::Rifle, TEXT("Rifle (MakeRifle)") }, { EWeaponKind::Shotgun, TEXT("Shotgun (MakeShotgun)") },
			{ EWeaponKind::Revolver, TEXT("Revolver (MakeRevolver)") }, { EWeaponKind::SMG, TEXT("SMG (MakeSMG)") },
			{ EWeaponKind::Sniper, TEXT("Sniper (MakeSniper)") }, { EWeaponKind::LMG, TEXT("LMG (MakeLMG)") },
			{ EWeaponKind::Launcher, TEXT("Launcher (MakeLauncher)") } };
		for (const TPair<EWeaponKind, const TCHAR*>& Kind : Kinds)
		{
			const FGunHands Hands = FirstPersonHandsGrips::Tuned(Kind.Key);
			UE_LOG(LogLooter, Display, TEXT("Hands, %s: GripTurn = %s; GripForearm = %s; GripHold = %s; ForeTurn = %s; ForeForearm = %s; ForeHold = %s;"),
				Kind.Value, *HandsDumpRotator(Hands.GripTurn), *HandsDumpVector(Hands.GripForearm.GetSafeNormal()), *HandsDumpVector(Hands.GripHold),
				*HandsDumpRotator(Hands.ForeTurn), *HandsDumpVector(Hands.ForeForearm.GetSafeNormal()), *HandsDumpVector(Hands.ForeHold));
			FVector Grip;
			FRotator Turn;
			const FVector Nudge = WeaponHipHold::TuningNudge(Kind.Key);
			const FRotator TurnNudge = WeaponHipHold::TuningTurn(Kind.Key);
			if (WeaponHipHold::For(Kind.Key, Grip, Turn))
			{
				UE_LOG(LogLooter, Display, TEXT("Hip hold, %s (WeaponHipHold.cpp): %s, %s"), Kind.Value, *HandsDumpVector(Grip + Nudge), *HandsDumpRotator(Turn + TurnNudge));
			}
			else
			{
				UE_LOG(LogLooter, Display, TEXT("Hip hold, %s (its definition's own: create_revolver.py): add %s to first_person_grip and %s to first_person_rotation"),
					Kind.Value, *HandsDumpVector(Nudge), *HandsDumpRotator(TurnNudge));
			}
		}
		UE_LOG(LogLooter, Display, TEXT("Support hand's curls (FHandFingers::Of, its gun's ForeGrip): fingers +%.1f, thumb +%.1f degrees on every joint"),
			CVarSupportFingers.GetValueOnGameThread(), CVarSupportThumb.GetValueOnGameThread());
		if (const IConsoleVariable* Scale = IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Hands.Scale")))
		{
			UE_LOG(LogLooter, Display, TEXT("Hands' size: Looter.Hands.Scale %.3f (0: the component's ArmsScale)"), Scale->GetFloat());
		}
	}

	FAutoConsoleCommand CmdHandsDump(TEXT("Looter.Hands.Dump"),
		TEXT("Prints the first-person hands' holds and the hip holds with the Looter.Hands.* and Looter.HipHold.* tuning on them, ready to bake."),
		FConsoleCommandDelegate::CreateStatic(&DumpHands));
#endif
}

const FGunHands& FirstPersonHandsGrips::For(EWeaponKind Kind)
{
	static const FGunHands Rifle = MakeRifle();
	static const FGunHands Shotgun = MakeShotgun();
	static const FGunHands Revolver = MakeRevolver();
	static const FGunHands SMG = MakeSMG();
	static const FGunHands Sniper = MakeSniper();
	static const FGunHands LMG = MakeLMG();
	static const FGunHands Launcher = MakeLauncher();
	switch (Kind)
	{
	case EWeaponKind::Shotgun:
		return Shotgun;
	case EWeaponKind::Revolver:
		return Revolver;
	case EWeaponKind::SMG:
		return SMG;
	case EWeaponKind::Sniper:
		return Sniper;
	case EWeaponKind::LMG:
		return LMG;
	case EWeaponKind::Launcher:
		return Launcher;
	case EWeaponKind::Rifle:
	case EWeaponKind::None:
	default:
		return Rifle;
	}
}

FVector FirstPersonHandsGrips::BoltKnobAt(const FGunHands& Hands, float Open)
{
	float Lift = 0.f;
	float Back = 0.f;
	LooterReload::BoltPose(Open, Lift, Back);
	// As the gun's model turns and draws its bolt (UWeaponModelComponent::SetBoltOpen): about the bore through the pivot.
	const FQuat Turn(FVector::ForwardVector, FMath::DegreesToRadians(Lift));
	return Hands.BoltPivot + Turn.RotateVector(Hands.Handle - Hands.BoltPivot) - FVector(Back, 0.f, 0.f);
}

FHandPlacement FirstPersonHandsGrips::OnBoltKnob(const FHandsGun& Gun, float Open)
{
	// Thumb and forefinger pinched on the knob from its right and above, the palm toward the gun, the forearm in from the
	// lower right as on the grip.
	const FGunHands Hands = Tuned(Gun.Kind);
	const FVector Knob = BoltKnobAt(Hands, Open) * Gun.Scale;
	const FVector Hold = Knob + FVector(0.f, 1.2f, 0.6f);
	return InGun(Gun, Hold, FVector(0.25f, -0.2f, -1.f), FVector(0.f, -1.f, 0.f), FHandFingers::Of(EHandGrip::Pinch), FVector(-0.5f, 0.55f, -0.65f));
}

FGunHands FirstPersonHandsGrips::Tuned(EWeaponKind Kind)
{
	FGunHands Hands = For(Kind);
#if !UE_BUILD_SHIPPING
	const int32 Wanted = CVarHandsKind.GetValueOnGameThread();
	if (Wanted != 0 && Wanted != static_cast<int32>(Kind))
	{
		return Hands;
	}
	Hands.GripTurn += FRotator(CVarTriggerPitch.GetValueOnGameThread(), CVarTriggerYaw.GetValueOnGameThread(), CVarTriggerRoll.GetValueOnGameThread());
	Hands.GripForearm = SwingForearm(Hands.GripForearm, CVarTriggerArmYaw.GetValueOnGameThread(), CVarTriggerArmPitch.GetValueOnGameThread());
	Hands.GripHold += FVector(CVarTriggerAhead.GetValueOnGameThread(), CVarTriggerRight.GetValueOnGameThread(), CVarTriggerUp.GetValueOnGameThread());
	Hands.ForeTurn += FRotator(CVarSupportPitch.GetValueOnGameThread(), CVarSupportYaw.GetValueOnGameThread(), CVarSupportRoll.GetValueOnGameThread());
	Hands.ForeForearm = SwingForearm(Hands.ForeForearm, CVarSupportArmYaw.GetValueOnGameThread(), CVarSupportArmPitch.GetValueOnGameThread());
	Hands.ForeHold += FVector(CVarSupportAhead.GetValueOnGameThread(), CVarSupportRight.GetValueOnGameThread(), CVarSupportUp.GetValueOnGameThread());
#endif
	return Hands;
}

void FirstPersonHandsGrips::OnHandle(EFirstPersonHand Hand, const FVector& Handle, const FVector& Palm, float Tilt, FVector& OutFingers,
	FVector& OutPalm)
{
	// The handle runs the index-and-thumb way across the palm. A hand's knuckles are square to that and to its palm: for a
	// right hand index-way cross palm, for a left hand palm cross index-way (they're mirror images).
	FVector IndexWay = Handle.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	OutPalm = (Palm - IndexWay * FVector::DotProduct(IndexWay, Palm)).GetSafeNormal(UE_SMALL_NUMBER, FVector(0.f, -1.f, 0.f));
	OutFingers = Hand == EFirstPersonHand::Right ? FVector::CrossProduct(IndexWay, OutPalm) : FVector::CrossProduct(OutPalm, IndexWay);
	if (!FMath::IsNearlyZero(Tilt))
	{
		// Turned about the palm's facing: the knuckles tip toward the handle's foot (a negative tilt: toward its top).
		const float Radians = FMath::DegreesToRadians(Tilt);
		OutFingers = (OutFingers * FMath::Cos(Radians) - IndexWay * FMath::Sin(Radians)).GetSafeNormal();
	}
}

FHandPlacement FirstPersonHandsGrips::InGun(const FHandsGun& Gun, const FVector& Hold, const FVector& FingersDir, const FVector& PalmDir,
	const FHandFingers& Fingers, const FVector& Forearm)
{
	return FHandPlacement::Make(Gun.ToCameraPoint(Hold), Gun.ToCameraDir(FingersDir), Gun.ToCameraDir(PalmDir), Fingers,
		Forearm.IsNearlyZero() ? FVector::ZeroVector : Gun.ToCameraDir(Forearm.GetSafeNormal()));
}

FHandPlacement FirstPersonHandsGrips::OnGrip(const FHandsGun& Gun)
{
	const FGunHands Hands = Tuned(Gun.Kind);
	FVector Fingers;
	FVector Palm;
	OnHandle(EFirstPersonHand::Right, Hands.GripHandle, Hands.GripPalm, Hands.GripTilt, Fingers, Palm);
	TurnHand(Hands.GripTurn, Fingers, Palm);
	FHandFingers Curls = FHandFingers::Of(EHandGrip::Trigger);
	for (int32 Joint = 0; Joint < FHandFingers::Joints; ++Joint)
	{
		Curls.Curl[FHandFingers::IndexFinger][Joint] = Hands.TriggerCurl[Joint];
	}
	// The framing's nudge on top (FPHandsFraming: a trigger glove sunk in a stock brought out where it reads).
	const FHandPlacement OnTheGrip = InGun(Gun, Gun.Grip + (Hands.GripHold + FPHandsFraming::TriggerNudge(Gun.Kind)) * Gun.Scale, Fingers, Palm, Curls,
		Hands.GripForearm);
	// A bolt gun's trigger hand leaves the grip to work the bolt (between shots and in its reload), riding its knob.
	if (Gun.BoltHand > 0.f && !Hands.BoltPivot.IsZero())
	{
		return FHandPlacement::Blend(OnTheGrip, OnBoltKnob(Gun, Gun.BoltOpen), Gun.BoltHand);
	}
	return OnTheGrip;
}

FHandPlacement FirstPersonHandsGrips::OnForegrip(const FHandsGun& Gun, float PumpBack)
{
	const FGunHands Hands = Tuned(Gun.Kind);
	FVector Fingers;
	FVector Palm;
	OnHandle(EFirstPersonHand::Left, Hands.ForeHandle, Hands.ForePalm, Hands.ForeTilt, Fingers, Palm);
	TurnHand(Hands.ForeTurn, Fingers, Palm);
	// A racked pump slides back along the gun's -X, and the hand round it goes with it. The framing's slide on top
	// (FPHandsFraming: back along the fore-end, so the glove sits low in the frame as Borderlands' do).
	const FVector Hold = Gun.Foregrip + (Hands.ForeHold + FPHandsFraming::SupportNudge(Gun.Kind) - FVector(PumpBack, 0.f, 0.f)) * Gun.Scale;
	FHandFingers Curls = FHandFingers::Of(Hands.ForeGrip);
#if !UE_BUILD_SHIPPING
	NudgeSupportFingers(Gun.Kind, Curls);
#endif
	return InGun(Gun, Hold, Fingers, Palm, Curls, Hands.ForeForearm);
}

// ---------------------------------------------------------------------------
// FHandPlacement
// ---------------------------------------------------------------------------

FHandPlacement FHandPlacement::Make(const FVector& InHold, const FVector& FingersDir, const FVector& PalmDir, const FHandFingers& InFingers,
	const FVector& InForearm)
{
	FHandPlacement Placement;
	Placement.Hold = InHold;
	Placement.Turn = FirstPersonHands::Frame(FingersDir, PalmDir);
	Placement.Fingers = InFingers;
	Placement.Forearm = InForearm.GetSafeNormal();
	return Placement;
}

FHandPlacement FHandPlacement::Blend(const FHandPlacement& From, const FHandPlacement& To, float Alpha)
{
	const float Share = FMath::Clamp(Alpha, 0.f, 1.f);
	FHandPlacement Placement;
	Placement.Hold = FMath::Lerp(From.Hold, To.Hold, Share);
	Placement.Turn = FQuat::Slerp(From.Turn, To.Turn, Share).GetNormalized();
	Placement.Fingers = FHandFingers::Blend(From.Fingers, To.Fingers, Share);
	// A way only one end has is kept whole (the other's "the hand's usual way" is only known to the rig).
	if (From.Forearm.IsNearlyZero() || To.Forearm.IsNearlyZero())
	{
		Placement.Forearm = Share < 0.5f && !From.Forearm.IsNearlyZero() ? From.Forearm : To.Forearm;
		if (Placement.Forearm.IsNearlyZero())
		{
			Placement.Forearm = From.Forearm;
		}
	}
	else
	{
		Placement.Forearm = FMath::Lerp(From.Forearm, To.Forearm, Share).GetSafeNormal(UE_SMALL_NUMBER, To.Forearm);
	}
	return Placement;
}

FHandPlacement FHandPlacement::Moved(const FTransform& Space) const
{
	FHandPlacement Placement = *this;
	Placement.Hold = Space.TransformPosition(Hold);
	Placement.Turn = (Space.GetRotation() * Turn).GetNormalized();
	Placement.Forearm = Space.GetRotation().RotateVector(Forearm);
	return Placement;
}

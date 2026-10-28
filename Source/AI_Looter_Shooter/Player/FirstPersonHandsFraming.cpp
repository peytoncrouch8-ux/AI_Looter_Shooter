#include "Player/FirstPersonHandsFraming.h"
#include "AI_Looter_Shooter.h"
#include "HAL/IConsoleManager.h"

// The first-person framing's numbers and rules (FirstPersonHandsFraming.h). The defaults come from comparing our frames
// with Borderlands 2 and 3 at 16:9 (Saved/FPHands/PLAN.md): there the gun's muzzle sits a little right of and under the
// crosshair (55-63% across, 52-64% down), the gun runs to the bottom right corner, and of the arms only the gloves and
// wrists show, low (the support glove 80-100% down); pistols are held in the right hand alone. Ours showed a support
// forearm standing straight up the screen from the bottom edge for a third of the frame's height.

namespace
{
	/** Each kind's console-variable names, by EWeaponKind (None has none: it reads the rifle's). */
	const TCHAR* const KindNames[] = { nullptr, TEXT("Rifle"), TEXT("Shotgun"), TEXT("Revolver"), TEXT("SMG"), TEXT("Sniper"), TEXT("LMG"),
		TEXT("Launcher") };
	constexpr int32 KindCount = UE_ARRAY_COUNT(KindNames);

	int32 KindIndex(EWeaponKind Kind)
	{
		const int32 Index = static_cast<int32>(Kind);
		return Index >= 1 && Index < KindCount ? Index : 1;
	}

	/**
	 * Tuned in PIE at 16:9 (2026-10-22, against the Borderlands measurements in Saved/FPHands/PLAN.md; the 1.25x forearms).
	 * Long guns a touch lower, and the support hand slid back along the fore-end where the gun has room (lower on screen, as
	 * the gun runs down to the bottom right): the Ranchhand's pump, the Rustler's helix, the Longhorn's fore-end ahead of its box
	 * (52 to 46; the box is at 35.6), the Thresher's handguard (44 to 38; it starts at 37.6). Never the Bullpup's: 6 cm back is
	 * the front of its grip loop (37.8); its trigger glove went 2.5 cm to the right so no finger showed through the thumbhole,
	 * until FPGRIP placed the trigger finger (2026-10-22). The rifle, LMG and sniper pulled in (the user's call, 2026-10-22: closer and lower, the muzzle tipped up a
	 * little about the grip, never down: the user rejected a muzzle-down tilt on 2026-10-14), so the support hand sits low in
	 * the frame; the guns read a bit shorter. The launcher further out so it fills less of the frame; the revolver
	 * one-handed, brought in toward the middle and stood nearly upright, as Borderlands holds a pistol. Aiming isn't
	 * touched: the sight slerps to the line of sight from wherever the hip hold is.
	 */
	FPHandsFraming::FKindFrame MakeDefaults(int32 Index)
	{
		FPHandsFraming::FKindFrame Frame;
		switch (static_cast<EWeaponKind>(Index))
		{
		case EWeaponKind::Rifle:
			// Pulled in (the user, 2026-10-22: "pull the guns in"): 12 cm nearer, 7 lower and 2 in toward the middle, the
			// muzzle tipped up 5 degrees about the grip so it stays at Borderlands' height (0.63 down) while the fore-end and
			// the support glove under it come down the frame (0.78 down; was 0.70). Tuned in PIE at 16:9.
			// The user's playtest (2026-10-27): "the guns need pushed forward, made 1.2x bigger and the forearm/hand needs to not
			// be so much in view": back out to the table's distance, 5 up, 1 further in, the gun drawn 1.2x in first person
			// (Looter.FPHands.GunScale) with the steeper, more turned-back forearms below. Tuned in PIE at 16:9.
			Frame.HipAhead = 0.f;
			Frame.HipRight = -3.f;
			// 5 up again once the support hand cups under the trigger housing (2026-10-27), so the cupping hand shows.
			Frame.HipUp = 3.f;
			Frame.HipPitch = 5.f;
			// FPGRIP (2026-10-22): no longer 2.5 cm right: the trigger finger is placed (on the trigger or along the frame),
			// so none shows through the thumbhole, and the palm is seated on the post.
			break;
		case EWeaponKind::Shotgun:
			Frame.HipUp = -1.5f;
			Frame.SupportAhead = -3.f;
			break;
		case EWeaponKind::Revolver:
			Frame.HipRight = -5.f;
			Frame.HipUp = 1.f;
			Frame.HipRoll = 6.f;
			Frame.OneHand = 1.f;
			break;
		case EWeaponKind::SMG:
			// (The support slides back toward the grip of the fore-end days are gone: the hand cups under the gun's middle
			// now, its spot in FirstPersonHandsCup.cpp, 2026-10-27; the Ranchhand's -3 stays for its pump.)
			Frame.HipUp = -1.5f;
			break;
		case EWeaponKind::Sniper:
			// Pulled in a little (glove 0.84, muzzle 0.60) until the play-check of 2026-10-23 ("the Sniper is held one-handed":
			// its trigger glove sat ~6 cm under the frame's bottom edge). The user's "lift slightly": back out to the table's
			// distance and 2.5 up, tuned in PIE so the trigger glove's knuckles show at the bottom edge.
			// The overnight playtest (2026-10-28; the 1.2x first-person gun since): at the hip the scope hid the right third of
			// the screen (an Unpaid half behind it). Framed like the LMG: the gun 4 cm lower (HipUp 2.5 -> -1.5) and the arms
			// 0.9x on it, so the scope drops under the crosshair's line and the hands fit its box.
			Frame.HipAhead = 0.f;
			Frame.HipUp = -1.5f;
			Frame.HipPitch = 2.f;
			Frame.ArmsScale = 0.9f;
			break;
		case EWeaponKind::LMG:
			// Pulled in 14 cm from its last hold (the support hand on the handguard was ~86 cm out), lower, the muzzle tipped up
			// 4 degrees (glove 0.80 down, muzzle 0.64). The user's "lift slightly" (2026-10-23, the trigger glove was under the
			// frame): back out to the table's distance and 3 up, tuned in PIE till its fingers show under the receiver (5 out and
			// 2 up still hid them).
			// "Pushed forward, 1.2x bigger, less arm" (2026-10-27): the table's distance with the 1.2x first-person scale and the
			// steeper forearms; tuned in PIE at 16:9.
			// "2cm back and 2 cm down" after that (the user, 2026-10-27).
			Frame.HipAhead = -2.f;
			Frame.HipRight = 0.f;
			Frame.HipUp = -3.f;
			Frame.HipPitch = 4.f;
			// The user (2026-10-27): the cupping hand clipped the belt box, too big for it: the arms 0.9x on this gun only (0.8
			// overall at Ellis's 0.88). Then "move the hand forward towards the front of the boxmag, and the gun needs to go
			// down": the cup 3 ahead of the box's middle, the gun 4 lower (HipUp 3 -> -1), tuned in PIE.
			Frame.SupportAhead = 3.f;
			Frame.ArmsScale = 0.9f;
			break;
		case EWeaponKind::Launcher:
			Frame.HipAhead = 9.f;
			Frame.HipRight = 2.f;
			Frame.HipUp = -2.f;
			break;
		default:
			break;
		}
		return Frame;
	}

	/** The live values: each field a console variable's own storage (Looter.FPHands.<Kind>.<Field>). */
	FPHandsFraming::FKindFrame Frames[KindCount] = {
		MakeDefaults(0), MakeDefaults(1), MakeDefaults(2), MakeDefaults(3), MakeDefaults(4), MakeDefaults(5), MakeDefaults(6), MakeDefaults(7) };

	struct FKindField
	{
		const TCHAR* Name;
		float FPHandsFraming::FKindFrame::*Field;
		const TCHAR* Help;
	};

	const FKindField KindFields[] = {
		{ TEXT("HipAhead"), &FPHandsFraming::FKindFrame::HipAhead, TEXT("Hip hold moved ahead (camera cm), on top of WeaponHipHold's.") },
		{ TEXT("HipRight"), &FPHandsFraming::FKindFrame::HipRight, TEXT("Hip hold moved right (camera cm).") },
		{ TEXT("HipUp"), &FPHandsFraming::FKindFrame::HipUp, TEXT("Hip hold moved up (camera cm).") },
		{ TEXT("HipPitch"), &FPHandsFraming::FKindFrame::HipPitch, TEXT("Hip hold's muzzle turned up (degrees).") },
		{ TEXT("HipYaw"), &FPHandsFraming::FKindFrame::HipYaw, TEXT("Hip hold's muzzle turned right (degrees; negative toes it in).") },
		{ TEXT("HipRoll"), &FPHandsFraming::FKindFrame::HipRoll, TEXT("Hip hold rolled (degrees).") },
		{ TEXT("SupportAhead"), &FPHandsFraming::FKindFrame::SupportAhead, TEXT("Support hand slid along the gun toward the muzzle (cm of the modeled gun; negative: back, lower on screen).") },
		{ TEXT("SupportRight"), &FPHandsFraming::FKindFrame::SupportRight, TEXT("Support hand moved to the gun's right (cm of the modeled gun).") },
		{ TEXT("SupportUp"), &FPHandsFraming::FKindFrame::SupportUp, TEXT("Support hand moved up the gun (cm of the modeled gun).") },
		{ TEXT("TriggerAhead"), &FPHandsFraming::FKindFrame::TriggerAhead, TEXT("Trigger hand slid along the gun (cm of the modeled gun).") },
		{ TEXT("TriggerRight"), &FPHandsFraming::FKindFrame::TriggerRight, TEXT("Trigger hand moved to the gun's right (cm; out of a stock it sinks into).") },
		{ TEXT("TriggerUp"), &FPHandsFraming::FKindFrame::TriggerUp, TEXT("Trigger hand moved up the gun (cm of the modeled gun).") },
		{ TEXT("OneHand"), &FPHandsFraming::FKindFrame::OneHand, TEXT("0-1: how much the support hand leaves the gun at rest, parked under the frame (a pistol's 1).") },
		{ TEXT("GunScale"), &FPHandsFraming::FKindFrame::GunScale, TEXT("This kind's first-person gun size on top of Looter.FPHands.GunScale (a multiplier).") },
		{ TEXT("ArmsScale"), &FPHandsFraming::FKindFrame::ArmsScale, TEXT("This kind's first-person arms size on top of the hero's (a multiplier).") },
	};

	/** Registers every kind's fields as console variables over Frames (static start-up, after Frames is filled). */
	struct FKindVariables
	{
		TArray<TUniquePtr<FAutoConsoleVariableRef>> Refs;

		FKindVariables()
		{
			for (int32 Index = 1; Index < KindCount; ++Index)
			{
				for (const FKindField& Field : KindFields)
				{
					const FString Name = FString::Printf(TEXT("Looter.FPHands.%s.%s"), KindNames[Index], Field.Name);
					Refs.Add(MakeUnique<FAutoConsoleVariableRef>(*Name, Frames[Index].*Field.Field, Field.Help, ECVF_Default));
				}
			}
		}
	};
	FKindVariables KindVariables;

	TAutoConsoleVariable<int32> CVarEnable(TEXT("Looter.FPHands.Enable"), 1,
		TEXT("1: the Borderlands framing (hip nudges, hand slides, one-handed pistol, forearm framing, ViewFov). 0: every rule off, as before."));
	TAutoConsoleVariable<float> CVarViewFov(TEXT("Looter.FPHands.ViewFov"), 0.f,
		TEXT("The view model's field of view (degrees; 0: the camera's own). Narrower draws the gun with less stretch near the eye; the holds go out to keep its place and size."));
	TAutoConsoleVariable<float> CVarBaseFov(TEXT("Looter.FPHands.BaseFov"), 90.f,
		TEXT("The view model's field of view the hip holds were tuned at (degrees): ViewFov's push-out is measured from it."));
	TAutoConsoleVariable<float> CVarAspect(TEXT("Looter.FPHands.Aspect"), 16.f / 9.f,
		TEXT("The frame's width over height the forearm framing keeps elbows out of (16:9; a wider screen shows less height, so less arm)."));
	// 72 / 78 and ForearmBack 0.5 since the user's "less arm" (2026-10-27): the sleeves leave the frame sooner, only the gloves
	// and a short cuff show.
	TAutoConsoleVariable<float> CVarSupportAngle(TEXT("Looter.FPHands.SupportForearmAngle"), 72.f,
		TEXT("Degrees under the horizontal the support forearm runs on screen, down to the left."));
	TAutoConsoleVariable<float> CVarTriggerAngle(TEXT("Looter.FPHands.TriggerForearmAngle"), 78.f,
		TEXT("Degrees under the horizontal the trigger forearm runs on screen, down to the right."));
	// 0.3 since FPGRIP (2026-10-22): at 0.6 the support sleeve's cuff hid the glove holding the gun.
	TAutoConsoleVariable<float> CVarBack(TEXT("Looter.FPHands.ForearmBack"), 0.5f,
		TEXT("0-1: how much of a forearm's way turns back toward the eye's plane (foreshortened, coming from behind the view; it also pushes the elbow out of the frame)."));
	TAutoConsoleVariable<float> CVarElbowMargin(TEXT("Looter.FPHands.ElbowMargin"), 0.06f,
		TEXT("How far outside the frame the elbow (the sleeve's cut end) stays (share of the half-frame)."));
	TAutoConsoleVariable<float> CVarMaxSwing(TEXT("Looter.FPHands.MaxForearmSwing"), 60.f,
		TEXT("The most the framing swings a forearm off the grip table's way (degrees), so the wrist stays believable."));
	TAutoConsoleVariable<float> CVarForearmShare(TEXT("Looter.FPHands.ForearmShare"), 1.f,
		TEXT("0-1: the forearm framing's share (0: the grip tables' forearms as they were)."));
	TAutoConsoleVariable<float> CVarWristBack(TEXT("Looter.FPHands.WristBack"), 7.f,
		TEXT("cm from a hand's grip point back to its wrist (the arms' size scales it), where the forearm framing starts."));
	TAutoConsoleVariable<float> CVarParkAhead(TEXT("Looter.FPHands.ParkAhead"), 20.f, TEXT("The parked support hand (a one-handed pistol): cm ahead of the eye."));
	TAutoConsoleVariable<float> CVarParkRight(TEXT("Looter.FPHands.ParkRight"), -26.f, TEXT("The parked support hand: cm right of the eye (negative: left)."));
	TAutoConsoleVariable<float> CVarParkUp(TEXT("Looter.FPHands.ParkUp"), -38.f, TEXT("The parked support hand: cm up from the eye (well under the frame)."));
	TAutoConsoleVariable<float> CVarGunScale(TEXT("Looter.FPHands.GunScale"), 1.2f,
		TEXT("How much bigger the gun in hand is drawn in first person only (about its grip; ADS, hands, muzzle and brass follow)."));
	TAutoConsoleVariable<float> CVarOneHandSeconds(TEXT("Looter.FPHands.OneHandSeconds"), 0.15f,
		TEXT("Seconds the support hand takes to leave a one-handed gun or come back to it (a reload, a throw)."));

	TAutoConsoleVariable<float> CVarPreviewAspect(TEXT("Looter.FPHands.PreviewAspect"), 0.f,
		TEXT("Development: draws the first-person view at this aspect inside the viewport (1.7778: the game's 16:9 inside the editor's wide one; 0 off)."));

	float SafeAngle(float Degrees)
	{
		return FMath::Clamp(Degrees, 1.f, 170.f);
	}

	/** From toward To on the shortest arc, at most MaxDegrees of the way. */
	FVector SwingToward(const FVector& From, const FVector& To, float MaxDegrees)
	{
		const double Cos = FMath::Clamp(FVector::DotProduct(From, To), -1.0, 1.0);
		const float Angle = FMath::RadiansToDegrees(static_cast<float>(FMath::Acos(Cos)));
		if (Angle <= MaxDegrees || Angle < UE_KINDA_SMALL_NUMBER)
		{
			return To;
		}
		const FQuat Whole = FQuat::FindBetweenNormals(From, To);
		return FQuat::Slerp(FQuat::Identity, Whole, MaxDegrees / Angle).RotateVector(From).GetSafeNormal(UE_SMALL_NUMBER, From);
	}

	/** An elbow nearer the eye than this (cm ahead) is never taken: the sleeve would pass the lens and fill the view. */
	constexpr float NearestElbow = 6.f;

	/** How far outside the frame a point is (0 on the edge, negative inside; one at or behind the lens counts as inside). */
	float OutBy(const FVector& Point, const FPHandsFraming::FFrameView& View)
	{
		FVector2D Screen;
		if (Point.X < NearestElbow || !FPHandsFraming::Project(Point, View, Screen))
		{
			return -1.f;
		}
		return static_cast<float>(FMath::Max(FMath::Abs(Screen.X), FMath::Abs(Screen.Y))) - 1.f;
	}

	FString DumpFrame(const FPHandsFraming::FKindFrame& Frame)
	{
		FString Line;
		for (const FKindField& Field : KindFields)
		{
			const float Value = Frame.*Field.Field;
			if (!FMath::IsNearlyZero(Value))
			{
				Line += FString::Printf(TEXT("Frame.%s = %.2ff; "), Field.Name, Value);
			}
		}
		return Line.IsEmpty() ? FString(TEXT("(none)")) : Line;
	}

	/** Looter.FPHands.Dump: every kind's framing as lines to paste into MakeDefaults, and the shared knobs. */
	void DumpFraming()
	{
		for (int32 Index = 1; Index < KindCount; ++Index)
		{
			UE_LOG(LogLooter, Display, TEXT("FPHands, %s (MakeDefaults): %s"), KindNames[Index], *DumpFrame(Frames[Index]));
		}
		UE_LOG(LogLooter, Display,
			TEXT("FPHands: Enable %d, ViewFov %.1f (BaseFov %.1f, depth x%.3f), forearm angles %.1f / %.1f, back %.2f, elbow margin %.2f, max swing %.1f, share %.2f, wrist back %.1f, park (%.1f, %.1f, %.1f), one-hand %.2f s"),
			CVarEnable.GetValueOnGameThread(), CVarViewFov.GetValueOnGameThread(), CVarBaseFov.GetValueOnGameThread(), FPHandsFraming::DepthScale(),
			CVarSupportAngle.GetValueOnGameThread(), CVarTriggerAngle.GetValueOnGameThread(), CVarBack.GetValueOnGameThread(),
			CVarElbowMargin.GetValueOnGameThread(), CVarMaxSwing.GetValueOnGameThread(), CVarForearmShare.GetValueOnGameThread(),
			CVarWristBack.GetValueOnGameThread(), CVarParkAhead.GetValueOnGameThread(), CVarParkRight.GetValueOnGameThread(),
			CVarParkUp.GetValueOnGameThread(), CVarOneHandSeconds.GetValueOnGameThread());
	}

	FAutoConsoleCommand CmdDump(TEXT("Looter.FPHands.Dump"),
		TEXT("Prints the first-person framing (every kind's Looter.FPHands.<Kind>.* and the shared knobs) as lines to bake."),
		FConsoleCommandDelegate::CreateStatic(&DumpFraming));
}

FPHandsFraming::FFrameView FPHandsFraming::FFrameView::FromFieldOfView(float Degrees, float InAspect)
{
	FFrameView View;
	View.HalfWideTan = FMath::Tan(FMath::DegreesToRadians(SafeAngle(Degrees)) * 0.5f);
	View.Aspect = FMath::Max(InAspect, 0.1f);
	return View;
}

FPHandsFraming::FForearmRules FPHandsFraming::FForearmRules::For(EFirstPersonHand Hand)
{
	FForearmRules Rules;
	Rules.ScreenAngle = Hand == EFirstPersonHand::Left ? CVarSupportAngle.GetValueOnAnyThread() : CVarTriggerAngle.GetValueOnAnyThread();
	Rules.Back = FMath::Clamp(CVarBack.GetValueOnAnyThread(), 0.f, 0.9f);
	Rules.ElbowMargin = FMath::Max(CVarElbowMargin.GetValueOnAnyThread(), 0.f);
	Rules.MaxSwing = FMath::Clamp(CVarMaxSwing.GetValueOnAnyThread(), 0.f, 180.f);
	Rules.Share = IsEnabled() ? FMath::Clamp(CVarForearmShare.GetValueOnAnyThread(), 0.f, 1.f) : 0.f;
	return Rules;
}

bool FPHandsFraming::IsEnabled()
{
	return CVarEnable.GetValueOnAnyThread() != 0;
}

FPHandsFraming::FKindFrame FPHandsFraming::For(EWeaponKind Kind)
{
	return IsEnabled() ? Frames[KindIndex(Kind)] : FKindFrame();
}

FPHandsFraming::FKindFrame FPHandsFraming::Defaults(EWeaponKind Kind)
{
	return MakeDefaults(KindIndex(Kind));
}

FVector FPHandsFraming::HipNudge(EWeaponKind Kind)
{
	const FKindFrame Frame = For(Kind);
	return FVector(Frame.HipAhead, Frame.HipRight, Frame.HipUp);
}

FRotator FPHandsFraming::HipTurn(EWeaponKind Kind)
{
	const FKindFrame Frame = For(Kind);
	return FRotator(Frame.HipPitch, Frame.HipYaw, Frame.HipRoll);
}

FVector FPHandsFraming::SupportNudge(EWeaponKind Kind)
{
	const FKindFrame Frame = For(Kind);
	return FVector(Frame.SupportAhead, Frame.SupportRight, Frame.SupportUp);
}

FVector FPHandsFraming::TriggerNudge(EWeaponKind Kind)
{
	const FKindFrame Frame = For(Kind);
	return FVector(Frame.TriggerAhead, Frame.TriggerRight, Frame.TriggerUp);
}

float FPHandsFraming::OneHandShare(EWeaponKind Kind)
{
	return FMath::Clamp(For(Kind).OneHand, 0.f, 1.f);
}

float FPHandsFraming::GunScale(EWeaponKind Kind)
{
	return FMath::Clamp(CVarGunScale.GetValueOnAnyThread() * For(Kind).GunScale, 0.5f, 2.f);
}

float FPHandsFraming::ViewModelFieldOfView(float Base)
{
	const float Wanted = CVarViewFov.GetValueOnAnyThread();
	return IsEnabled() && Wanted > 1.f ? FMath::Clamp(Wanted, 40.f, 120.f) : Base;
}

float FPHandsFraming::DepthScaleFor(float BaseDegrees, float ViewDegrees)
{
	const float Base = FMath::Tan(FMath::DegreesToRadians(SafeAngle(BaseDegrees)) * 0.5f);
	const float View = FMath::Tan(FMath::DegreesToRadians(SafeAngle(ViewDegrees)) * 0.5f);
	return FMath::Clamp(Base / FMath::Max(View, UE_KINDA_SMALL_NUMBER), 0.5f, 3.f);
}

float FPHandsFraming::DepthScale()
{
	const float Wanted = CVarViewFov.GetValueOnAnyThread();
	if (!IsEnabled() || Wanted <= 1.f)
	{
		return 1.f;
	}
	return DepthScaleFor(CVarBaseFov.GetValueOnAnyThread(), FMath::Clamp(Wanted, 40.f, 120.f));
}

float FPHandsFraming::FrameAspect()
{
	return FMath::Max(CVarAspect.GetValueOnAnyThread(), 0.1f);
}

bool FPHandsFraming::Project(const FVector& Point, const FFrameView& View, FVector2D& OutScreen)
{
	if (Point.X <= 1.0)
	{
		OutScreen = FVector2D::ZeroVector;
		return false;
	}
	OutScreen.X = Point.Y / (Point.X * View.HalfWideTan);
	OutScreen.Y = Point.Z / (Point.X * View.HalfHighTan());
	return true;
}

bool FPHandsFraming::IsOffFrame(const FVector& Point, const FFrameView& View, float Margin)
{
	return OutBy(Point, View) >= Margin;
}

float FPHandsFraming::VisibleForearm(const FVector& Wrist, const FVector& Way, float Length, const FFrameView& View)
{
	constexpr int32 Steps = 64;
	const FVector Dir = Way.GetSafeNormal();
	float Covered = 0.f;
	FVector2D Last;
	bool bLastIn = false;
	for (int32 Step = 0; Step <= Steps; ++Step)
	{
		FVector2D Screen;
		const bool bIn = Project(Wrist + Dir * (Length * Step / Steps), View, Screen) && FMath::Abs(Screen.X) <= 1.0 && FMath::Abs(Screen.Y) <= 1.0;
		if (bIn && bLastIn)
		{
			// Frame heights: the height spans 2 in Y; the width (Aspect heights) spans 2 in X.
			const double Across = (Screen.X - Last.X) * View.Aspect * 0.5;
			const double Up = (Screen.Y - Last.Y) * 0.5;
			Covered += static_cast<float>(FMath::Sqrt(Across * Across + Up * Up));
		}
		Last = Screen;
		bLastIn = bIn;
	}
	return Covered;
}

FVector FPHandsFraming::FrameForearm(EFirstPersonHand Hand, const FVector& Wrist, const FVector& TableWay, float Length, const FFrameView& View,
	const FForearmRules& Rules)
{
	const float Side = Hand == EFirstPersonHand::Left ? -1.f : 1.f;
	const FVector Table = TableWay.GetSafeNormal(UE_SMALL_NUMBER, FVector(-0.5f, 0.5f * Side, -0.7f).GetSafeNormal());
	if (Rules.Share <= 0.f || Length <= 0.f || Wrist.X <= 1.0)
	{
		return Table;
	}

	// Which way leaves the frame soonest is set by geometry: a view-model gun's fore-end is 65-75 cm out and a forearm only
	// about 29 cm (SK_FP_Arms' 32.6 at 0.88: FPArms.py draws first-person forearms 1.25x long for this; at 26.1 a wrist high
	// in the frame could only get its elbow out by running straight down, the "post" the user saw). So: the most slanted
	// way, from Rules.ScreenAngle toward straight down, whose elbow still leaves the frame; each slant tried with the most of
	// it turned back toward the eye's plane first (back from the wrist pushes a low point further out of the frame, and the
	// arm reads as coming from behind the view, foreshortened).
	FVector Best = Table;
	float BestOut = OutBy(Wrist + Table * Length, View);
	constexpr float AngleStep = 5.f;
	constexpr int32 BackTries = 4;
	const float First = FMath::Clamp(Rules.ScreenAngle, 0.f, 89.f);
	for (float Angle = First; Angle <= 89.5f; Angle += AngleStep)
	{
		// The slant on screen as a way at the wrist's own depth: moving across at one depth moves the picture straight across,
		// at the same scale both ways (square pixels), so the slant in pixels is the slant in Y and Z.
		const float Radians = FMath::DegreesToRadians(FMath::Min(Angle, 89.f));
		const FVector Across(0.f, Side * FMath::Cos(Radians), -FMath::Sin(Radians));
		for (int32 Try = 0; Try <= BackTries; ++Try)
		{
			const float Back = Rules.Back * (1.f - static_cast<float>(Try) / BackTries);
			const FVector Raw = (Across * (1.f - Back) - FVector::ForwardVector * Back).GetSafeNormal(UE_SMALL_NUMBER, Across);
			const FVector Way = SwingToward(Table, Raw, Rules.MaxSwing * Rules.Share);
			const float Out = OutBy(Wrist + Way * Length, View);
			if (Out >= Rules.ElbowMargin)
			{
				return Way;
			}
			if (Out > BestOut)
			{
				Best = Way;
				BestOut = Out;
			}
		}
	}
	// No slant gets the elbow out: whichever way gets it furthest (the table's when nothing beats it), never a worse one.
	return Best;
}

FVector FPHandsFraming::WristOf(const FHandPlacement& Hand, float Scale)
{
	const float Back = FMath::Max(CVarWristBack.GetValueOnAnyThread(), 0.f);
	return Hand.Hold - Hand.GetFingersDir() * (Back * Scale) - Hand.GetPalmDir() * (FirstPersonHands::GripOffPalm * Scale);
}

FHandPlacement FPHandsFraming::ParkedSupport()
{
	// Low on the left under the frame, knuckles ahead and a little up, palm in, loosely closed: where a one-handed shooter's
	// free hand rides, out of view.
	const FVector Hold(CVarParkAhead.GetValueOnAnyThread(), CVarParkRight.GetValueOnAnyThread(), CVarParkUp.GetValueOnAnyThread());
	return FHandPlacement::Make(Hold, FVector(1.f, 0.f, 0.3f), FVector(0.f, 1.f, 0.f), FHandFingers::Of(EHandGrip::Forend),
		FVector(-0.35f, -0.3f, -0.88f));
}

float FPHandsFraming::OneHandSeconds()
{
	return FMath::Max(CVarOneHandSeconds.GetValueOnAnyThread(), 0.01f);
}

float FPHandsFraming::PreviewAspect()
{
	return FMath::Max(CVarPreviewAspect.GetValueOnAnyThread(), 0.f);
}

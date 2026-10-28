#pragma once

#include "CoreMinimal.h"
#include "Player/FirstPersonHandsTypes.h"

/**
 * How the first-person gun and hands sit in the frame, Borderlands-style (the user, 2026-10-21: "our hands show too much
 * of the arms and clip into the guns"): gloves and wrists low in the frame, never a forearm standing up the screen like a
 * post, never an elbow or a sleeve's cut end; pistols one-handed. Every number is a Looter.FPHands.* console variable, read
 * live, so the framing is tuned in PIE (Looter.FPHands.Dump prints the lines to bake). Saved/FPHands/PLAN.md has the
 * measurements behind the defaults.
 *
 *  - Per gun kind (Looter.FPHands.<Kind>.<Field>, Kind = Rifle, Shotgun, Revolver, SMG, Sniper, LMG, Launcher): the hip
 *    hold's nudge on top of WeaponHipHold's table (HipAhead/HipRight/HipUp cm, HipPitch/HipYaw/HipRoll degrees), the
 *    support hand's slide along the gun from its foregrip (SupportAhead/SupportRight/SupportUp, cm of the modeled gun),
 *    the trigger hand's (TriggerAhead/TriggerRight/TriggerUp), and OneHand (0-1: how much the support hand leaves the gun
 *    at the hip, parked under the frame; a pistol's 1).
 *  - The forearms (FrameForearm): each one turned to leave the frame on a slant (SupportForearmAngle, TriggerForearmAngle:
 *    degrees under the horizontal on screen) and back toward the eye's plane (ForearmBack), foreshortened, with its elbow
 *    (the sleeve's capped cut) kept outside the frame (ElbowMargin); steeper only where the slant can't get the elbow out;
 *    never swung more than MaxForearmSwing off the grip table's way (the wrist stays believable).
 *  - The view model's own field of view (ViewFov; 0 keeps the camera's): narrower draws the gun with less stretch near the
 *    eye. The holds are pushed out by DepthScale so the gun keeps its place and size on screen; the hands go out with it,
 *    so a forearm needs DepthScale times its length to get its elbow out (the 1.25x first-person forearms leave a little room).
 */
namespace FPHandsFraming
{
	/** One gun kind's framing nudges (camera cm and degrees for the hip; the modeled gun's cm for the hands). */
	struct FKindFrame
	{
		float HipAhead = 0.f;
		float HipRight = 0.f;
		float HipUp = 0.f;
		float HipPitch = 0.f;
		float HipYaw = 0.f;
		float HipRoll = 0.f;
		float SupportAhead = 0.f;
		float SupportRight = 0.f;
		float SupportUp = 0.f;
		float TriggerAhead = 0.f;
		float TriggerRight = 0.f;
		float TriggerUp = 0.f;
		float OneHand = 0.f;
		/** This kind's first-person gun size on top of Looter.FPHands.GunScale (a multiplier; 1: the shared size). */
		float GunScale = 1.f;
	};

	/** The view model's frame: the tangent of half its field of view across, and its width over its height. */
	struct FFrameView
	{
		float HalfWideTan = 1.f;
		float Aspect = 16.f / 9.f;

		static FFrameView FromFieldOfView(float Degrees, float InAspect);
		float HalfHighTan() const { return HalfWideTan / FMath::Max(Aspect, 0.1f); }
	};

	/** What FrameForearm needs besides the hand: the framing knobs (the cvars' values by default; tests set their own). */
	struct FForearmRules
	{
		/**
		 * Degrees under the horizontal the forearm runs on screen, out toward its own side (the left arm down-left): the
		 * slant wanted; a steeper one is taken where only that gets the elbow out of the frame.
		 */
		float ScreenAngle = 50.f;
		/** 0-1: how much of its way turns back toward the eye's plane (foreshortened; it pushes a low elbow further out). */
		float Back = 0.3f;
		/** How far outside the frame the elbow stays (share of the half-frame), so the sleeve's cut end never shows. */
		float ElbowMargin = 0.06f;
		/** The most the way may swing off the grip table's (degrees): past it the wrist bends unlike a wrist. */
		float MaxSwing = 60.f;
		/** 0-1: the whole framing's share (0: the grip table's way as it was). */
		float Share = 1.f;

		static FForearmRules For(EFirstPersonHand Hand);
	};

	/** The framing for a gun kind, with its console variables' values now (None: the rifle's). */
	AI_LOOTER_SHOOTER_API FKindFrame For(EWeaponKind Kind);

	/** The shipped defaults (the cvars start at these; tests check them). */
	AI_LOOTER_SHOOTER_API FKindFrame Defaults(EWeaponKind Kind);

	/** The hip hold's nudge for a kind (camera space: X ahead, Y right, Z up; cm) and its turn. */
	AI_LOOTER_SHOOTER_API FVector HipNudge(EWeaponKind Kind);
	AI_LOOTER_SHOOTER_API FRotator HipTurn(EWeaponKind Kind);

	/** The support and trigger hands' nudges along the gun (its own space; cm of the modeled gun). */
	AI_LOOTER_SHOOTER_API FVector SupportNudge(EWeaponKind Kind);
	AI_LOOTER_SHOOTER_API FVector TriggerNudge(EWeaponKind Kind);

	/** 0-1: how much the support hand leaves a gun of this kind at rest (a pistol held one-handed: 1). */
	AI_LOOTER_SHOOTER_API float OneHandShare(EWeaponKind Kind);

	/**
	 * How much bigger the gun in hand is drawn in first person only (the user, 2026-10-27: "made 1.2x bigger"):
	 * Looter.FPHands.GunScale (1.2) times the kind's Looter.FPHands.<Kind>.GunScale (1), on top of the model's own
	 * UWeaponModelComponent::ScaleFor. Applied about the grip by the view model (PlayerLocomotionViewModel.cpp), so the hold
	 * stays put; third person, the loadout stand, the bench and loot never see it.
	 */
	AI_LOOTER_SHOOTER_API float GunScale(EWeaponKind Kind);

	/** Looter.FPHands.Enable: 0 puts every framing rule off (the game as before; for comparing). */
	AI_LOOTER_SHOOTER_API bool IsEnabled();

	/**
	 * The view model's field of view for a camera whose own is Base: Looter.FPHands.ViewFov when set (degrees), else Base.
	 * DepthScale: how much further out the holds go so the gun keeps its place and size on screen at that angle
	 * (tan(BaseFov / 2) / tan(ViewFov / 2); 1 when ViewFov is off). Only along the view: a point (X, Y, Z) at (kX, Y, Z)
	 * under a field of view k times narrower in its tangent lands on the same pixel.
	 */
	AI_LOOTER_SHOOTER_API float ViewModelFieldOfView(float Base);
	AI_LOOTER_SHOOTER_API float DepthScale();
	AI_LOOTER_SHOOTER_API float DepthScaleFor(float BaseDegrees, float ViewDegrees);

	/** The aspect the framing is made for (Looter.FPHands.Aspect; 16:9). */
	AI_LOOTER_SHOOTER_API float FrameAspect();

	/**
	 * A camera-space point on the frame: X across (-1 left edge, 1 right edge), Y up (-1 bottom, 1 top). False for a point
	 * at or behind the eye (it's nowhere on the frame).
	 */
	AI_LOOTER_SHOOTER_API bool Project(const FVector& Point, const FFrameView& View, FVector2D& OutScreen);

	/** The point is outside the frame by Margin (share of the half-frame), or behind the eye. */
	AI_LOOTER_SHOOTER_API bool IsOffFrame(const FVector& Point, const FFrameView& View, float Margin);

	/** How much of the frame's height the forearm from Wrist along Way for Length covers where it's in view (0-1). */
	AI_LOOTER_SHOOTER_API float VisibleForearm(const FVector& Wrist, const FVector& Way, float Length, const FFrameView& View);

	/**
	 * The forearm's way out of the wrist (camera space) for a hand whose wrist is at Wrist: the most slanted way from
	 * Rules.ScreenAngle toward straight down (each with Rules.Back of it turned back toward the eye's plane, then less) whose
	 * elbow (Wrist + way x Length) is outside the frame by Rules.ElbowMargin and at least 6 cm ahead of the eye, never more
	 * than Rules.MaxSwing x Rules.Share off TableWay (the grip table's). Where none gets the elbow out, the one (TableWay
	 * among them) that gets it furthest. Rules.Share 0 gives TableWay.
	 */
	AI_LOOTER_SHOOTER_API FVector FrameForearm(EFirstPersonHand Hand, const FVector& Wrist, const FVector& TableWay, float Length,
		const FFrameView& View, const FForearmRules& Rules);

	/** Roughly where a placed hand's wrist is (camera space; the arms' size Scale): behind the grip point, off the palm. */
	AI_LOOTER_SHOOTER_API FVector WristOf(const FHandPlacement& Hand, float Scale);

	/** The support hand parked under the frame while a pistol is held one-handed (camera space, before DepthScale). */
	AI_LOOTER_SHOOTER_API FHandPlacement ParkedSupport();

	/** Seconds the support hand takes to leave the gun or come back to it (Looter.FPHands.OneHandSeconds). */
	AI_LOOTER_SHOOTER_API float OneHandSeconds();

	/**
	 * Development only: the aspect the first-person view is drawn at inside the viewport (Looter.FPHands.PreviewAspect; 0
	 * off, 1.7778 the game's 16:9 inside the editor's wide viewport, for framing checks and grabs).
	 */
	AI_LOOTER_SHOOTER_API float PreviewAspect();
}

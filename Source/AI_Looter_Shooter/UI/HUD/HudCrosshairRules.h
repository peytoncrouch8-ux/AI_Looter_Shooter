#pragma once

#include "CoreMinimal.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Weapons/WeaponTypes.h"

/**
 * A crosshair for each gun kind (the user's pick, 2026-10-29, from Docs/Work/2026-10-28-Core/crosshair: one shape per kind,
 * white with a dark edge, gaps that still widen with spread, a kick per kind): the shapes as plain data, so the tests can
 * check them without a world, and the numbers the HUD runs them with. UHudCrosshairWidget paints them; UPlayerHUDWidget's
 * UpdateCrosshair (PlayerHUDWidgetCrosshair.cpp) picks the gun in hand's, opens it with the spread and kicks it per shot.
 *
 * Shapes are in the mockup's SVG units round the aim point (y down), as make.js drew them; UnitScale turns them into HUD
 * pixels at 1080p. No gun, or a gun of no kind, keeps the old four-tick cross.
 */
namespace HudCrosshair
{
	/**
	 * HUD pixels per mockup unit. The mockup showed the shapes at 1.6x over a 1080p frame blown up 1.25x, so against the
	 * world the user saw them at about 1.28x: 1.25 keeps that look and whole-ish line widths (2 units = 2.5 px).
	 */
	inline constexpr float UnitScale = 1.25f;

	/** The dark edge is this much wider than its line (mockup units: stroke + 2.4), and a dot's this much wider round it. */
	inline constexpr float OutlineExtra = 2.4f;
	inline constexpr float DotOutlineExtra = 1.2f;

	/** The square (HUD px) the shapes are painted in, centred on the screen: room for any shape at its widest (MaxReach) and its kick. */
	inline constexpr float CanvasSize = 150.f;

	/**
	 * How the gap follows the gun's spread (GetEffectiveSpread, degrees): the same curve as the old cross, whose box was
	 * 22 + 7 px per degree, kept between 24 and 80 px. Open is how far past its rest a shape's lines have moved out (px):
	 * half the box's growth past its smallest, so the old cross at rest is exactly the old one.
	 */
	inline constexpr float GapBoxBase = 22.f;
	inline constexpr float GapBoxPerDegree = 7.f;
	inline constexpr float GapBoxMin = 24.f;
	inline constexpr float GapBoxMax = 80.f;
	AI_LOOTER_SHOOTER_API float GapBoxFor(float Spread);
	AI_LOOTER_SHOOTER_API float OpenOf(float GapBox);
	/** The most a shape ever opens: (GapBoxMax - GapBoxMin) / 2. */
	inline constexpr float MaxOpen = (GapBoxMax - GapBoxMin) * 0.5f;

	/** One line of a shape: a polyline (closed: back to its first point), its width, and which way the spread pushes it. */
	struct FStroke
	{
		TArray<FVector2f> Points;
		float Width = 2.f;
		bool bClosed = false;
		/** Unit direction it moves out along as the gap opens (by Open px); zero stays put (the launcher's diamond and marks). */
		FVector2f Push = FVector2f::ZeroVector;
	};

	/** A broken ring round the aim point (the shotgun's): its radius grows with the open gap, its dashes keep their number. */
	struct FRing
	{
		float Radius = 0.f;
		float Width = 2.f;
		int32 Dashes = 8;
		/** How much of each dash's stretch of the ring is drawn (make.js's dasharray "12 6.85" on a radius of 24). */
		float DashShare = 12.f / 18.85f;
	};

	/** The centre dot, apart from the lines (CrosshairDot): it doesn't kick and stays through a third-person aim. */
	struct FDot
	{
		FVector2f At = FVector2f::ZeroVector;
		float Radius = 1.4f;
	};

	struct FShape
	{
		TArray<FStroke> Strokes;
		TArray<FRing> Rings;
		FDot Dot;
		/**
		 * Whether the dot shows from the hip with the lines. The rifle's cross and the launcher's diamond have none in the
		 * mockup; theirs is only the aim point through a third-person aim, where the lines have gone.
		 */
		bool bHipDot = true;
	};

	/** The gun kind's shape; None (no gun, or a gun of no kind) is the old four-tick cross. */
	AI_LOOTER_SHOOTER_API const FShape& ShapeFor(EWeaponKind Kind);

	/** The farthest any of the shape's ink reaches from the aim point (HUD px) with the gap opened by Open, its edge included. */
	AI_LOOTER_SHOOTER_API float ReachOf(const FShape& Shape, float Open);

	/**
	 * The farthest a shape may reach, opened (HUD px): short of the interaction prompt's words, whose top is 52 px under the
	 * middle (UHudInteractPromptWidget::Gap), and well inside the pickup feed's 76 px to the left. The old cross reached 41.
	 */
	inline constexpr float MaxReach = 50.f;

	/** How far the shape may open before it would reach past MaxReach (0 to MaxOpen): the big shapes open less. */
	AI_LOOTER_SHOOTER_API float OpenLimitFor(const FShape& Shape);

	/** Each shot kicks the lines out to Scale times their size, settling back over Seconds (easing out). */
	struct FKick
	{
		float Scale = 1.f;
		float Seconds = 0.f;
	};

	/**
	 * By kind (last night's plan, GUNS.md F): SMG small and quick, the heavy single shots big and slow. None keeps the old
	 * cross's 1.45x over 0.16 s.
	 */
	AI_LOOTER_SHOOTER_API FKick KickFor(EWeaponKind Kind);

	/** How big the lines are this far (s) into a kick: Scale on the frame of the shot, back to 1 at Seconds. */
	AI_LOOTER_SHOOTER_API float KickScaleAt(const FKick& Kick, float Age);

	/**
	 * Another gun kind came into hand: the new shape grows in from SwitchFromScale and fades up over SwitchSeconds, so the
	 * change reads as a change and not a flicker.
	 */
	inline constexpr float SwitchSeconds = 0.15f;
	inline constexpr float SwitchFromScale = 0.7f;
	AI_LOOTER_SHOOTER_API float SwitchFadeAt(float Age);
	AI_LOOTER_SHOOTER_API float SwitchScaleAt(float Age);

	/** The crosshair colour setting's colour (Settings > Interface), every one from the palette; White is the default. */
	AI_LOOTER_SHOOTER_API FLinearColor ColourOf(ECrosshairColour Colour);

	/** The dark edge under every line and dot, in every colour (the HUD's floating art's edge). */
	AI_LOOTER_SHOOTER_API FLinearColor EdgeColour();
}

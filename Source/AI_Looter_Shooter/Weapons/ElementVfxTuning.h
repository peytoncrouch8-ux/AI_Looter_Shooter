#pragma once

#include "CoreMinimal.h"
#include "Elements/LooterElement.h"
#include "Weapons/WeaponTypes.h"

/**
 * The strength of the elements' firing and world looks ("Elements matter", lane F, 2026-10-29: the user asked for each element's
 * own feedback when it fires, flies, lands and kills): the Looter.Vfx.Element.* cvars, read live through Live(). The fields'
 * own values are the cvars' defaults, so a default-made one (the tests) and a fresh game agree. 1 = as designed, 0 = off.
 */
struct AI_LOOTER_SHOOTER_API FElementVfxTuning
{
	/** Looter.Vfx.Element.Muzzle: the element's sparks round a shot's flash (embers, arcs, gobs, glints, motes), times. */
	float Muzzle = 1.f;
	/** Looter.Vfx.Element.MuzzleTint: how far the flash's layers lean to the element's colour (0 the plain orange flash; read as a gun is built). */
	float MuzzleTint = 1.f;
	/** Looter.Vfx.Element.Trail: the trail behind an element round's tracer (heat and embers, a forked spark line, drips, frost, a light ribbon), times. */
	float Trail = 1.f;
	/** Looter.Vfx.Element.TrailReach (cm): a round lays its trail this far along its flight (past it the tracer flies on alone). */
	float TrailReach = 2400.f;
	/** Looter.Vfx.Element.Idle: the held gun's idle pieces a second (first person only), times. */
	float Idle = 1.f;
	/** Looter.Vfx.Element.Marks: an element round leaves its element's mark on the world (scorch, burn ring, acid, frost, gilt). */
	bool bMarks = true;
	/** Looter.Vfx.Element.MarkFlare: the brief ember, spark, drip or glint on a fresh element mark, times. */
	float MarkFlare = 1.f;
	/** Looter.Vfx.Element.Death: the bigger element deaths' layers (ash and heat, a shock pop, a melt, a shatter, light lifting), times. */
	float Death = 1.f;

	/** The cvars as set now. */
	static FElementVfxTuning Live();

	/** A fast gun's rounds lay a trail every this many (its tracer's crowd would be a cloud): SMG and LMG 2, the rest 1. */
	static int32 TrailEvery(EWeaponKind Kind);

	/** The held gun's idle pieces a second for an element at Idle 1 (a few: they are a hint, not a fire). 0 for None. */
	static float IdleRate(ELooterElement Element);
};

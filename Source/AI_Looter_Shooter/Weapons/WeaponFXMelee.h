#pragma once

#include "CoreMinimal.h"
#include "Weapons/ImpactSurface.h"

class FWeaponFX;

/** What a melee tool's blow landed on, for its effects (MeleeFX::Spawn). */
enum class EMeleeStruck : uint8
{
	/** A creature, a dummy, an egg sac: anything with health. */
	Body,
	/** A wall, a rock, a post (its surface: EImpactMaterial). */
	World
};

/**
 * The heroes' melee tools' own sounds and impact effects on top of the shared blow (UPlayerMeleeComponent: the thud, the
 * body's layer, FWeaponFX::SpawnImpact's dust and chips), by the tool's key (FHeroMeleeTool::Kind; the user's round 4
 * picks, 2026-10-27, "melee polish"):
 *  - Spade (Ellis's grave spade): a ringing clang; sparks off stone and iron, a flick of the cyan ember in its crack on a body.
 *  - Fan (Odessa's razor fan): a slicing snick; card-edge glints, gold and white, wherever its steel edges cut.
 *  - Gaff (Crane's ferry gaff): the hook's bite; iron sparks on stone, grit dragged back toward him as it yanks.
 *  - Piston (Gauge's piston fist): a hiss-thump; a burst of steam off the knuckle plate, a ring of the kick, and steam
 *    from the cylinder's vent (UPlayerMeleeComponent finds where it shows).
 *  - Pick (Pike's rock pick): a bright chink; sparks and stone chips off rock, a teal glint on a body.
 * Drawn with the bullets' code effects (FWeaponFX): no particle assets. Their strength is Looter.Melee.FX.
 */
namespace MeleeFX
{
	/** The tools' keys. */
	AI_LOOTER_SHOOTER_API FName Spade();
	AI_LOOTER_SHOOTER_API FName Fan();
	AI_LOOTER_SHOOTER_API FName Gaff();
	AI_LOOTER_SHOOTER_API FName Piston();
	AI_LOOTER_SHOOTER_API FName Pick();

	/** The tool's own swing and hit cues: Player.Melee.<Kind>.Swing (2D) and .Hit (3D); None for a tool with no key. */
	AI_LOOTER_SHOOTER_API FName SwingCue(FName Kind);
	AI_LOOTER_SHOOTER_API FName HitCue(FName Kind);

	/** This tool throws sparks off this surface: iron on stone, gravel or metal (the fan's steel edges on metal only). */
	AI_LOOTER_SHOOTER_API bool Sparks(FName Kind, EImpactMaterial Material);

	/**
	 * The tool's own effects where its blow lands: At (world), Normal (off the surface; the way back toward the player on a
	 * body), Along (the blow's way), on a body or the world's Material. Nothing for a tool with no key.
	 */
	AI_LOOTER_SHOOTER_API void Spawn(FWeaponFX& Effects, FName Kind, const FVector& At, const FVector& Normal, const FVector& Along,
		EMeleeStruck Struck, EImpactMaterial Material, FRandomStream& Random);

	/** A burst of steam from a vent at Vent (world, where it shows), blowing out along Out, Strength 1 a full one. */
	AI_LOOTER_SHOOTER_API void SpawnSteam(FWeaponFX& Effects, const FVector& Vent, const FVector& Out, float Strength, FRandomStream& Random);

	// --- The swing's smear (round 5, 2026-10-27: the user's video pick, "swing smear + hit feel") ---

	/** One place the tool's head passed: its tip and a point down the head toward the hand (world, where they show). */
	struct FSmearSample
	{
		FVector Tip = FVector::ZeroVector;
		FVector Inner = FVector::ZeroVector;
		/** World seconds when it was there, and the draw scale there (a first-person tool is drawn pulled in toward the eye). */
		double Time = 0.0;
		float Scale = 1.f;
	};

	/** The smear's tint by the tool: the spade's ember cyan, the fan's gold, the gaff's pale iron, the piston's steam, the pick's teal. */
	AI_LOOTER_SHOOTER_API FLinearColor SmearColor(FName Kind);

	/** How long a smear sample lasts (Looter.Melee.Smear.Seconds, 0.12) and its strength at Age seconds (1 new, 0 gone). */
	AI_LOOTER_SHOOTER_API float SmearSeconds();
	AI_LOOTER_SHOOTER_API float SmearShare(float Age);

	/**
	 * Draws the smear this frame through Samples (oldest first) as Now: a glowing band from each sample to the next along the
	 * tip and along the inner edge, thinning and fading with age (FWeaponFX's streaks, drawn one frame each).
	 */
	AI_LOOTER_SHOOTER_API void DrawSmear(FWeaponFX& Effects, const TArray<FSmearSample>& Samples, double Now, const FLinearColor& Color);
}

#pragma once

#include "CoreMinimal.h"

class AActor;
class UDecalComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UWorld;

/** What a kill or a blast leaves on the ground (FDeathSplats). */
enum class EVfxSplat : uint8
{
	/** A spider's ichor: a wet yellow-green pool spreading out from under it. */
	Ichor,
	/** The Gravemother's dark ichor: darker, wider, wetter. */
	DarkIchor,
	/** A slime's gel: a glossy puddle in its own colour, with drops round it. */
	Gel,
	/** An Unpaid's or a miner's ash: a pale grey ring of it, dry. */
	Ash,
	/** A blast's scorch: a dark sooty star burned into the ground (a rocket's, Slag Bomb's). */
	Scorch
};

/** How a splat looks: its colour (linear, before the decal's lighting), opacity, wetness (glossier), size and spread. */
struct FVfxSplatLook
{
	FLinearColor Color = FLinearColor::Black;
	float Opacity = 0.8f;
	/** 0 dry and matte, 1 a wet sheen (lower roughness). */
	float Wet = 0.f;
	/** Its radius (cm) for a body or blast of size 1 (100 cm). */
	float Radius = 70.f;
	/** How long it takes to spread out to its full size (s): a pool seeps, a scorch is there at once. */
	float SpreadSeconds = 0.8f;
};

/**
 * Splats on the ground under kills and blasts (the VFX pass, Saved/Vfx/PLAN.md): a deferred decal each in M_FX_SplatDecal
 * (Tools/Unreal/build_vfx_materials.py: a procedural blob with a noisy edge and a ring of droplets, no texture), its colour
 * and opacity the decal's colour, its spread, seed and wetness a dynamic instance's. A pool of them on a transient actor (at
 * most Looter.Vfx.Splat.Max): a fresh one takes an ended one, else the oldest. Each spreads out over its look's seconds,
 * holds Looter.Vfx.Splat.Hold and fades over .Fade on the GPU (DecalLifetimeOpacity). Cheap on Medium: DBuffer decals as
 * the bullet marks, a few at once, each fading out with screen size. UVfxSubsystem owns one and ticks it while one spreads.
 */
class AI_LOOTER_SHOOTER_API FDeathSplats
{
public:
	/** The decal material; without it there are no splats (the code runs before the script has). */
	static const TCHAR* MaterialPath;

	/** A splat's look for Kind, Tint the creature's own (a slime's gel colour; ignored for the others). */
	static FVfxSplatLook LookFor(EVfxSplat Kind, const FLinearColor& Tint);

	/**
	 * A splat on the ground at Ground (its surface's Normal) for a body or blast of Size (cm; 100 = 1), holding Hold seconds
	 * and fading over Fade, at most Max at once. Pick (0..1) turns it and seeds its shape. False when none was laid.
	 */
	bool Add(UWorld& World, const FVector& Ground, const FVector& Normal, EVfxSplat Kind, const FLinearColor& Tint, float Size, float Hold,
		float Fade, int32 Max, float Pick);

	/** Spreads the fresh ones and hides the ended ones (Now: the world's time). True while any still spreads. */
	bool Advance(double Now);

	/** Splats showing now (spreading, holding or fading). */
	int32 NumLive(double Now) const;

	/** Lets go of the pool (its actor and decals go). */
	void Shutdown();

	/** The spread's curve: 0 at its start, 1 when spread (an ease-out: it seeps out fast, then slowly). Tests read it. */
	static float SpreadAt(float Seconds, float SpreadSeconds);

private:
	struct FSplat
	{
		TWeakObjectPtr<UDecalComponent> Decal;
		TWeakObjectPtr<UMaterialInstanceDynamic> Look;
		double Born = -1.0e9;
		double Ends = -1.0e9;
		float SpreadSeconds = 0.f;
		bool bSpreading = false;
	};

	bool Initialize(UWorld& World);

	TArray<FSplat> Splats;
	TWeakObjectPtr<AActor> Owner;
	TWeakObjectPtr<UMaterialInterface> Material;
	bool bMaterialMissing = false;
};

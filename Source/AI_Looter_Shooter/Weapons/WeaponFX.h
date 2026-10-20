#pragma once

#include "CoreMinimal.h"
#include "Player/DustLight.h"
#include "Weapons/CreatureBlood.h"
#include "Weapons/ImpactSurface.h"
#include "Weapons/WaterRings.h"

class AActor;
class UInstancedStaticMeshComponent;
class UWorld;

/** What a bullet hit, which decides how the impact looks. */
enum class EImpactSurface : uint8
{
	/** Ground, rocks, props: sparks, a puff of dust and a few chips. */
	World,
	/** Hard targets such as the training dummies: a burst of sparks and a flash. */
	Target,
	/** Creatures: a splash of what they spill (SpawnCreatureHit; SpawnImpact's is the spiders' ichor). */
	Flesh
};

/** How a creature's body goes when it dies (FWeaponFX::SpawnDeathBurst). */
enum class EDeathBurst : uint8
{
	None,
	/** A spider's shell cracking open: a puff of dust, chitin bits flung out and a splash of ichor. */
	Shell,
	/** A slime splatting: gel droplets flung out round it and a low wet spray. */
	Gel,
	/** An Unpaid dissolving: a flash of soul-light in its coal's color, motes rising off it, sparks and a pale wisp. */
	SoulLight
};

/**
 * Code-drawn weapon effects: tracer streaks, impact sparks, flashes, dust puffs and flying chips or droplets (and the
 * dust and grit a player's slide kicks up, FSlideDust, through SpawnDustPuff and SpawnGrit; and creatures' death bursts,
 * SpawnDeathBurst). It is all instanced quads and chunks on a few instanced static mesh components (redrawn every frame,
 * quads turned to face the camera), so there are no particle assets to author. The look comes from two small materials,
 * M_FX_Glow (additive) and M_FX_Smoke (translucent), which read each instance's color and strength from its custom data;
 * chunks are lit surfaces, one component per color (BitSetFor). WeaponFXWorld.cpp holds the effects that aren't a
 * gun's (grave dirt, the slide's dust and grit, the death bursts); WeaponFXImpacts.cpp the world's hits by surface,
 * WeaponFXCreatures.cpp the creatures' by what they spill, WeaponFXShots.cpp the shot's own (ricochets, the muzzle's
 * smoke, the tracer's smoke thread), WeaponFXDraw.cpp the moving and drawing.
 */
class AI_LOOTER_SHOOTER_API FWeaponFX
{
public:
	/** Creates the effect components on a transient actor in the world. Safe to call again (does nothing). */
	void Initialize(UWorld* World);
	void Shutdown();

	/** A glowing streak drawn this frame only; bullets redraw their tracers every frame as they move. */
	void AddStreak(const FVector& Tail, const FVector& Head, float Width, const FLinearColor& Color, float Intensity);

	/**
	 * A round glow Size cm across at Location, facing the camera, drawn this frame only (a flash's soft disc): what reads of
	 * a thing flying straight away from the eye, which a streak along its way shows end-on as nothing (Rail Spike's head).
	 */
	void AddGlow(const FVector& Location, float Size, const FLinearColor& Color, float Intensity);

	/** Sparks, flash, dust and chips (or an ichor splash) where a bullet hit, thrown off the surface. */
	void SpawnImpact(const FVector& Location, const FVector& Normal, const FVector& ShotDirection, EImpactSurface Surface, bool bCritical);

	/**
	 * A bullet's hit on the world in the look of what it struck (WeaponFXImpacts.cpp): dirt kicks up a brown puff and
	 * clods, grass bits of green, sand a pale spray, gravel and stone grit, chips and a few sparks, wood pale splinters
	 * and sawdust, metal a shower of hot sparks, sheet metal sparks and paint flakes, glass glinting shards, water a splash
	 * and mist, cloth lint, hay straws, bone pale chips. Scale sizes it to the gun (a shotgun's pellet is smaller and
	 * throws less); a critical hit adds its pop as SpawnImpact's does. A hit on water also leaves rings spreading over the
	 * surface (SpawnWaterRings: two for a shot, one for a pellet or a footstep).
	 *
	 * Given the Floor under the hit (the surface itself on the ground, what's under a wall's hit), the chunks thrown land
	 * on it: they bounce, skitter and lie there a moment or two before they shrink away; drops splat and are gone.
	 */
	void SpawnSurfaceImpact(const FVector& Location, const FVector& Normal, const FVector& ShotDirection, EImpactMaterial Material,
		float Scale, bool bCritical, const FPlane* Floor = nullptr);

	/**
	 * A bullet's hit on a creature in what it spills (WeaponFXCreatures.cpp; ECreatureBlood): a spider's ichor (SpawnImpact's
	 * Flesh), the Gravemother's thick dark ichor, a slime's gel in its Tint, an Unpaid's embers in its coal's color with a
	 * breath of grave dust, a dead miner's embers with rock dust and chips. Scale sizes it to the gun as SpawnSurfaceImpact.
	 */
	void SpawnCreatureHit(const FVector& Location, const FVector& Normal, const FVector& ShotDirection, ECreatureBlood Blood,
		const FLinearColor& Tint, float Scale, bool bCritical);

	/**
	 * Rings spreading over water from Location (WeaponFXImpacts.cpp; the rules and the pool are FWaterRings): Normal is the
	 * surface's (nothing on a waterfall's face, which doesn't lie flat), Size the hit's as SpawnSurfaceImpact's Scale (a
	 * rifle shot 1 leaves two concentric rings, a pellet or a wading step one), Light the light the foam is shown in
	 * (nullptr: the effects look it up, a trace toward the sun at most every second or so). Returns the rings made.
	 */
	int32 SpawnWaterRings(const FVector& Location, const FVector& Normal, float Size, const FLinearColor* Light = nullptr);

	/**
	 * One V of a wake at Location on water (Normal its surface): two arms trailing back from Direction, the way a body
	 * walking through it goes, at Speed cm/s, for a body of Scale. True if made (not on a surface that isn't flat).
	 */
	bool SpawnWake(const FVector& Location, const FVector& Normal, const FVector& Direction, float Speed, float Scale,
		const FLinearColor* Light = nullptr);

	/**
	 * A round glancing off (WeaponFXShots.cpp): a white-hot spark where it struck and a short tracer flung off along
	 * Direction (the shot's bounce), with a few sparks. Looks only: nothing flies on.
	 */
	void SpawnRicochet(const FVector& Location, const FVector& Direction, float Scale);

	/**
	 * One breath of a hot muzzle's smoke (AWeaponBase's wisp after a burst or a heavy shot): a small, faint puff drifting
	 * at Velocity, rising slowly and spreading as it thins.
	 */
	void SpawnWisp(const FVector& Location, const FVector& Velocity, float Opacity, float StartSize, float EndSize, float Life);

	/**
	 * A length of the faint smoke thread a tracer leaves behind it (UBulletSubsystem): a soft line from Tail to Head,
	 * widening from Width as it thins out over Life.
	 */
	void AddThread(const FVector& Tail, const FVector& Head, float Width, float Opacity, float Life);

	/**
	 * A shot's flash in the open, with no gun model to wear one (a scene's gunfire): a hot core, a wider burst a hair
	 * ahead along Direction, a few sparks thrown out with the shot and a breath of smoke left hanging. Scale sizes it.
	 */
	void SpawnFlash(const FVector& Location, const FVector& Direction, float Scale = 1.f);

	/**
	 * Grave dirt thrown up from Location along Up (the grave wake-up's claws): brown puffs that hang and spread, and clods
	 * tossed out that fall back. A Strength nearer 1 throws more, farther.
	 */
	void SpawnDirt(const FVector& Location, const FVector& Up, float Strength);

	/**
	 * One puff of dust drifting from Location at Velocity (air drag slows it, it rises a little), growing from StartSize
	 * to EndSize as it fades over Life seconds. The smoke material is unlit, so Color is the dust as the light where it
	 * is shows it (FSlideDust works that out): dust in shade or at dusk stays dim instead of glowing.
	 */
	void SpawnDustPuff(const FVector& Location, const FVector& Velocity, const FLinearColor& Color, float Opacity, float StartSize,
		float EndSize, float Life);

	/** One grain of grit or gravel thrown from Location: a tiny lit chip that tumbles and falls. */
	void SpawnGrit(const FVector& Location, const FVector& Velocity, float Size, float Life);

	/**
	 * A creature's death burst round Center: its body about Size cm across, the burst thrown a little along ShotDirection
	 * (the killing shot's way). Tint is the shell's chitin, the slime's gel, or the soul-light's color. 25 to 30 particles.
	 */
	void SpawnDeathBurst(EDeathBurst Kind, const FVector& Center, const FVector& ShotDirection, float Size, const FLinearColor& Tint);

	/** Advances the particles and redraws everything, turned to face the camera. */
	void Tick(float DeltaSeconds, const FVector& CameraLocation);

	/** Nothing flying and nothing drawn: ticking can stop. */
	bool IsIdle() const;

	/** Live particles (for tests and the particle cap). */
	int32 NumParticles() const { return Particles.Num(); }

	/** Rings and wakes on the water now (tests). */
	int32 NumWaterRings() const { return WaterRings.NumRings(); }
	int32 NumWakes() const { return WaterRings.NumWakes(); }

	/** Chunks lying where they landed on an impact's floor (tests). */
	int32 NumResting() const;

	/** How far the lowest chunk with a floor is above it (cm; negative: sunk into it). 0 with none (tests). */
	float LowestOverFloor() const;

private:
	/**
	 * Bit: a lit chunk of a color of its own (chitin, gel), drawn on its BitSet's component. Streak: a glowing line flying
	 * on, growing to its length (a ricochet's tracer). Thread: a still line of smoke (a tracer's smoke thread).
	 */
	enum class EParticle : uint8 { Spark, Flash, Smoke, Chip, Droplet, Bit, Streak, Thread };

	struct FParticle
	{
		EParticle Type = EParticle::Spark;
		FVector Location = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		FQuat Rotation = FQuat::Identity;
		/** Tumble axis times speed (rad/s), for chips and droplets. */
		FVector Spin = FVector::ZeroVector;
		float Age = 0.f;
		float Life = 0.3f;
		float StartSize = 1.f;
		float EndSize = 1.f;
		FLinearColor Color = FLinearColor::White;
		/** Glow strength, or smoke opacity. */
		float Intensity = 1.f;
		/** Multiplier of normal gravity (negative rises). */
		float Gravity = 1.f;
		/** Air drag per second. */
		float Drag = 0.f;
		float Roll = 0.f;
		/** A Bit's set (BitSets). */
		uint8 BitSet = 0;
		/** It has a floor to land on (Floor): it bounces off it, and lies on it once spent (bResting). */
		bool bFloor = false;
		bool bResting = false;
		uint8 Bounces = 0;
		FPlane Floor = FPlane(0.0, 0.0, 1.0, 0.0);
		/** A Thread's half length along its line. */
		FVector Extent = FVector::ZeroVector;
	};

	/** How a set's chunks are drawn: thin plates (chitin, chips of paint or glass), round drops (gel, water), long splinters (wood, straw). */
	enum class EBitShape : uint8 { Plate, Round, Splinter };

	/** The lit chunks of one color and shape: chitin bits (cubes), gel drops (spheres), splinters (stretched cubes). */
	struct FBitSet
	{
		TWeakObjectPtr<UInstancedStaticMeshComponent> Instances;
		FColor Key;
		EBitShape Shape = EBitShape::Plate;
	};
	/**
	 * Kept to a few (a component each, empty and free while unused): the creatures' chitin, gel and dark ichor, the grave
	 * dirt, and the world's impacts' clods, splinters, shards and drops. Past it, a new color shares the last set's.
	 */
	static constexpr int32 MaxBitSets = 20;

	struct FStreak
	{
		FVector Tail;
		FVector Head;
		float Width;
		FLinearColor Color;
		float Intensity;
		/** A round glow at Head, Width across (AddGlow), not a line. */
		bool bRound = false;
	};

	FParticle& AddParticle(EParticle Type, const FVector& Location, const FVector& Velocity, float Life);
	/** A chunk crossing its floor: a bounce, a skitter, then it lies still a moment (drops splat and are gone). WeaponFXDraw.cpp. */
	void LandOnFloor(FParticle& Particle);
	void Redraw(const FVector& CameraLocation);
	/** The set for chunks of this Color (round: spheres, else cubes) with Glow, made the first time it's asked for. */
	uint8 BitSetFor(bool bRound, const FLinearColor& Color, float Glow);
	uint8 BitSetFor(EBitShape Shape, const FLinearColor& Color, float Glow);
	/** One lit chunk of a set thrown from Location: tumbling, falling, shrinking away at the end of its life. */
	FParticle& AddBit(uint8 Set, const FVector& Location, const FVector& Velocity, float Size, float Life);
	/** The water foam's colour in the light at Where: the foam's albedo times the sun's or sky's (FDustLight), looked up again every WaterLightSeconds. */
	FLinearColor WaterTint(const FVector& Where);

	TWeakObjectPtr<AActor> Owner;
	TWeakObjectPtr<UInstancedStaticMeshComponent> Glows;
	TWeakObjectPtr<UInstancedStaticMeshComponent> Smoke;
	TWeakObjectPtr<UInstancedStaticMeshComponent> Chips;
	TWeakObjectPtr<UInstancedStaticMeshComponent> Droplets;

	TArray<FBitSet> BitSets;

	TArray<FParticle> Particles;
	TArray<FStreak> Streaks;
	/** The rings and wakes on the water, drawn with the smoke (WaterRings.h), and the light their foam is shown in. */
	FWaterRings WaterRings;
	FDustLight WaterDustLight;
	FLinearColor WaterLight = FLinearColor::White;
	double WaterLightAt = -1.0e9;
	static constexpr double WaterLightSeconds = 1.0;
	/** While an impact with a floor spawns (SpawnSurfaceImpact), the floor its chunks are given. */
	TOptional<FPlane> SpawnFloor;
	/** Instances drawn last frame, so an emptied component gets cleared exactly once. */
	bool bDrawnAnything = false;
	FRandomStream Random{ 0x5eed };
};

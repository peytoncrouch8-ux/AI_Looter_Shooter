#pragma once

#include "CoreMinimal.h"

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
	/** Creatures: a splash of ichor. */
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
 * gun's (grave dirt, the slide's dust and grit, the death bursts).
 */
class AI_LOOTER_SHOOTER_API FWeaponFX
{
public:
	/** Creates the effect components on a transient actor in the world. Safe to call again (does nothing). */
	void Initialize(UWorld* World);
	void Shutdown();

	/** A glowing streak drawn this frame only; bullets redraw their tracers every frame as they move. */
	void AddStreak(const FVector& Tail, const FVector& Head, float Width, const FLinearColor& Color, float Intensity);

	/** Sparks, flash, dust and chips (or an ichor splash) where a bullet hit, thrown off the surface. */
	void SpawnImpact(const FVector& Location, const FVector& Normal, const FVector& ShotDirection, EImpactSurface Surface, bool bCritical);

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

private:
	/** Bit: a lit chunk of a color of its own (chitin, gel), drawn on its BitSet's component. */
	enum class EParticle : uint8 { Spark, Flash, Smoke, Chip, Droplet, Bit };

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
	};

	/** The lit chunks of one color and shape: chitin bits (cubes), gel drops (spheres). */
	struct FBitSet
	{
		TWeakObjectPtr<UInstancedStaticMeshComponent> Instances;
		FColor Key;
		bool bRound = false;
	};
	/** Kept to a few: past it, a new color shares the last set's. */
	static constexpr int32 MaxBitSets = 6;

	struct FStreak
	{
		FVector Tail;
		FVector Head;
		float Width;
		FLinearColor Color;
		float Intensity;
	};

	FParticle& AddParticle(EParticle Type, const FVector& Location, const FVector& Velocity, float Life);
	void Redraw(const FVector& CameraLocation);
	/** The set for chunks of this Color (round: spheres, else cubes) with Glow, made the first time it's asked for. */
	uint8 BitSetFor(bool bRound, const FLinearColor& Color, float Glow);

	TWeakObjectPtr<AActor> Owner;
	TWeakObjectPtr<UInstancedStaticMeshComponent> Glows;
	TWeakObjectPtr<UInstancedStaticMeshComponent> Smoke;
	TWeakObjectPtr<UInstancedStaticMeshComponent> Chips;
	TWeakObjectPtr<UInstancedStaticMeshComponent> Droplets;

	TArray<FBitSet> BitSets;

	TArray<FParticle> Particles;
	TArray<FStreak> Streaks;
	/** Instances drawn last frame, so an emptied component gets cleared exactly once. */
	bool bDrawnAnything = false;
	FRandomStream Random{ 0x5eed };
};

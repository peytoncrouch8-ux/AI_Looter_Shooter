#pragma once

#include "CoreMinimal.h"
#include "Elements/LooterElement.h"
#include "Player/DustLight.h"
#include "Weapons/CreatureBlood.h"
#include "Weapons/ImpactSurface.h"
#include "Weapons/VfxTypes.h"
#include "Weapons/WaterRings.h"

class AActor;
class UInstancedStaticMeshComponent;
class UWorld;
struct FVfxTuning;

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
 * smoke, the tracer's smoke thread), WeaponFXMotion.cpp the moving, WeaponFXDraw.cpp the drawing.
 */
class AI_LOOTER_SHOOTER_API FWeaponFX
{
public:
	/** Creates the effect components on a transient actor in the world. Safe to call again (does nothing). */
	void Initialize(UWorld* World);
	void Shutdown();

	/**
	 * How a streak is drawn: a plain glow (M_FX_Glow's streak), or a tracer on the soft tapered streak (SoftStreaks, M_FX_Muzzle:
	 * a hot head thinning to its tail, never wider on screen than Looter.Vfx.Tracer.Width; the play-check, 2026-10-22: a tracer's
	 * head seen end-on near a close target was a flat yellow diamond over it).
	 */
	enum class EStreakLook : uint8 { Glow, Tracer };

	/** A glowing streak drawn this frame only; bullets redraw their tracers every frame as they move. */
	void AddStreak(const FVector& Tail, const FVector& Head, float Width, const FLinearColor& Color, float Intensity,
		EStreakLook Look = EStreakLook::Glow);

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
	 * Gunfeel: all of it Looter.Gunfeel.Spurt.Scale times, with an exit spray thrown on along the shot out behind the
	 * target (Spurt.Exit of the entry's); a shotgun blast's pellets share one budget between them (Spurt.BlastBudget).
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
	 * (the killing shot's way). Tint is the shell's chitin, the slime's gel, or the soul-light's color. 35 to 45 pieces since
	 * the VFX pass (a wet pop and splat, big plates; a gel crown; a soul-light fireball, starburst and ring).
	 */
	void SpawnDeathBurst(EDeathBurst Kind, const FVector& Center, const FVector& ShotDirection, float Size, const FLinearColor& Tint);

	// --- The elements, the part effects and the loot's drop moments (WeaponFXElements.cpp hits and auras; WeaponFXElementBursts.cpp
	// bursts, arcs, the pop, the drop's ring; Phase 12.3, Thunder and Rot 2026-10-29) ---

	/**
	 * An element gun's round landing on a creature, on top of its splash: Ember a spit of embers and a lick of flame,
	 * Soul-light white-gold motes and a few short rays, Grave-salt pale crystal glints and a puff of frost. bSet: its status
	 * was just set (a fuller burst); bWeak: the creature is weak to it (fuller again, brighter). Scale sizes it to the gun.
	 */
	void SpawnElementHit(ELooterElement Element, const FVector& Location, const FVector& Normal, const FVector& ShotDirection,
		float Scale, bool bSet, bool bWeak);

	/**
	 * Count pieces of a status's look on a body (its Center, Radius and HalfHeight): a burn's embers and flame licks rising,
	 * a smite's motes gathering in, the salt's crystals dropping off with a breath of frost. Strength: more of it (a third
	 * burn, a smite near bursting). The status component spawns them at its own rate.
	 */
	void SpawnElementAura(ELooterElement Element, const FVector& Center, float Radius, float HalfHeight, int32 Count, float Strength);

	/** A status's burst over a body Size cm across: a smite's light (rays and a flash), a burn's or salt's kill. */
	void SpawnElementBurst(ELooterElement Element, const FVector& Center, float Size, bool bKill);

	/** A line of the element's light from From to To (a smite leaping to the next of the dead), fading in a moment. */
	void SpawnElementArc(ELooterElement Element, const FVector& From, const FVector& To);

	// --- An element gun's own looks ("Elements matter" lane F, 2026-10-29; WeaponFXElementShots/Marks/Deaths.cpp;
	// strengths Looter.Vfx.Element.*, FElementVfxTuning) ---

	/** Muzzle.Element's sparks round a shot's flash: Cinder embers, Thunder jagged arcs, Rot green gobs, Wardbreak salt glints, Hallow gold motes. */
	void SpawnElementMuzzle(const FVfxMuzzle& Muzzle);
	/** One piece of the trail behind an element round's tracer, Tail to Head: heat shimmer and embers, a forked spark line, drips, a frost line, a light ribbon. Amount: its share (fading in). */
	void SpawnElementTrail(ELooterElement Element, const FVector& Tail, const FVector& Head, float Amount);
	/** One idle piece off a held element gun at Location (as the eye sees it), carried at Velocity (its holder's), Scale the gun's: an ember or smoke, a crackle, a drip, frost, a mote. */
	void SpawnElementIdle(ELooterElement Element, const FVector& Location, const FVector& Velocity, float Scale);
	/** The brief flare on a fresh element mark (Normal its surface's): embers and a glow, sparks skittering, drips, glints, motes. Scale the round's mark scale. */
	void SpawnElementMark(ELooterElement Element, const FVector& Location, const FVector& Normal, float Scale);
	/**
	 * The bigger element death over a body (its cylinder; Front: toward the eye, Radius long; Amount its share): Cinder ash falling
	 * and a heat burst, Thunder a shock pop and a ring of arcs, Rot a melt into a green puddle, Wardbreak crystallised then
	 * shattering, Hallow light lifting off it. On top of ElementDeathLook's aura and burst.
	 */
	void SpawnElementDeath(ELooterElement Element, const FVector& Center, float Radius, float HalfHeight, const FVector& Front, float Amount);

	/** An Explosive round's burst at Location (Normal its surface's): a hot flash, sparks, a ring of smoke Radius cm out. */
	void SpawnSplashPop(const FVector& Location, const FVector& Normal, float Radius);

	/** A ring of light flat on the ground round Center, Radius cm out, drawn this frame only (a drop's landing ring). */
	void AddGroundRing(const FVector& Center, float Radius, float Width, const FLinearColor& Color, float Intensity, int32 Segments = 32);

	/** Count motes of Color rising round Center within Radius (a Legendary's landing). */
	void SpawnLootMotes(const FVector& Center, float Radius, const FLinearColor& Color, int32 Count);

	// --- The VFX pass (Saved/Vfx/PLAN.md, 2026-10-21 night): shapes (WeaponFXShapes.cpp), kills and blasts (WeaponFXKills.cpp),
	// muzzles (WeaponFXShots.cpp). Their strength is Looter.Vfx.* (FVfxTuning). ---

	/**
	 * A shock ring growing from StartRadius to EndRadius over Life, Width cm thick, fading as it goes (VfxRules::RingGrowth,
	 * RingFade): square to Axis (a ring on the ground: Axis its normal), or turned to face the eye each frame (bFacing).
	 * One quad on M_FX_Shape; without it, pieces of streak on M_FX_Glow.
	 */
	void SpawnShockRing(const FVector& Center, const FVector& Axis, float StartRadius, float EndRadius, float Width, const FLinearColor& Color,
		float Intensity, float Life, bool bFacing = false);

	/** A ball of fire (or light) at Location drifting at Velocity, blooming from StartSize to EndSize and cooling over Life. */
	void SpawnFireball(const FVector& Location, const FVector& Velocity, float StartSize, float EndSize, const FLinearColor& Color, float Intensity,
		float Life);

	/** A starburst of Spikes uneven spikes round a hot core, Size cm across, over Life (a hit's pop, a crit's flare). */
	void SpawnStarburst(const FVector& Location, float Size, const FLinearColor& Color, float Intensity, float Life, int32 Spikes = 8);

	/** A kill's layers on top of its kind's death burst (gore by the gun, a mist, a dust skirt, rings, its tier's flare). */
	void SpawnKillLayers(const FVfxKill& Kill);

	/** A blast: a hot core, fire puffs rolling into soot (SpawnFlame), a faint ring, a dust skirt, a rising smoke column, embers, debris. */
	void SpawnExplosion(const FVfxExplosion& Blast);

	/**
	 * A puff of fire rolling into dark smoke (FVfxFlame; WeaponFXFlames.cpp): alpha-blended on M_FX_Fire, so it keeps its colour
	 * in daylight. Without the material, a smoke puff tinted by its heat.
	 */
	void SpawnFlame(const FVfxFlame& Flame);

	/** Flames alive (fire or its smoke; tests). */
	int32 NumFlames() const;

	/** What a shot's muzzle throws round its flash, by the gun's kind (VfxRules::MuzzleLook): smoke, sparks, side jets, a ring. */
	void SpawnMuzzleBurst(const FVfxMuzzle& Muzzle);

	// --- A world hit by its gun's kind (the gun feel pass, GF-IMPACT 2026-10-22; WeaponFXImpactsKinds.cpp) ---

	/**
	 * The round whose hit is shown while this lives (UBulletSubsystem::ResolveHit round its ShowHit): SpawnSurfaceImpact draws
	 * it by its gun's kind (VfxRules::ImpactLook: an SMG's small, a sniper's punch, an LMG's beat), and a shotgun's pellets
	 * gather into their blast's one dust sheet. Without one a hit is drawn by its mark scale alone, as before.
	 */
	struct FLandingScope
	{
		FLandingScope(FWeaponFX& InEffects, const FVfxRound& Round)
			: Effects(InEffects), Saved(InEffects.LandingRound)
		{
			Effects.LandingRound = Round;
		}
		~FLandingScope() { Effects.LandingRound = Saved; }
		FLandingScope(const FLandingScope&) = delete;
		FLandingScope& operator=(const FLandingScope&) = delete;

	private:
		FWeaponFX& Effects;
		FVfxRound Saved;
	};

	/** The volley whose pellets are gathering into a dust sheet (0: none). */
	int32 PendingSheetVolley() const { return Sheet.Hits > 0 ? Sheet.Volley : 0; }

	/**
	 * Lays the gathered blast's dust sheet across its pattern once its last pellet is down (bStillFlying false) or it has
	 * waited Looter.Vfx.Impact.SheetWait (UBulletSubsystem's tick, after the bullets moved). True when it was laid (or let go:
	 * too few pellets on the world).
	 */
	bool SettleBlastSheet(bool bStillFlying);

	// --- The hit-feel pass (HITS lane D, 2026-10-28; WeaponFXHitFeel.cpp): wounds' and kills' own looks. No blood: chitin, goo, embers, soul-light, gel ---

	/** Count chitin plates chipped off a spider's shell at Point, thrown out along Normal; Size is the body part's (cm). */
	void SpawnChitinChips(const FVector& Point, const FVector& Normal, float Size, int32 Count);

	/** One bit of goo (a wounded spider's ichor, a spitter's venom, a slime's gel in Tint) dripping down from Point, Size cm. */
	void SpawnGooDrip(const FVector& Point, const FLinearColor& Tint, float Size);

	/** Count ember streaks of Tint torn from From along Way (a unit vector), flung up to Length cm (a torn Unpaid). */
	void SpawnEmberTear(const FVector& From, const FVector& Way, float Length, const FLinearColor& Tint, int32 Count);

	/** Count soul tatters (ember-edged wisps) flung from Center along Way, spiralling up as they fade; Size is the body's. */
	void SpawnSoulTatters(const FVector& Center, const FVector& Way, const FLinearColor& Tint, float Size, int32 Count);

	/** A slime burst: gel in Tint flung out in a fan from Center along Way (the shot's), Size cm across, with a low wet spray. */
	void SpawnGelPop(const FVector& Center, const FVector& Way, const FLinearColor& Tint, float Size);

	/** A puff of ember smoke rising from Point (a wounded Unpaid's coal, a hat's dust), Size cm, warm at Tint. */
	void SpawnEmberPuff(const FVector& Point, const FLinearColor& Tint, float Size);

	/** The share of a look's pieces thrown at Where (VfxRules::DetailAt from the camera last drawn from; 1 before any). */
	float DetailAt(const FVector& Where) const;

	/** Rings, fireballs and starbursts alive (tests). */
	int32 NumShapes() const;

	/** Advances the particles and redraws everything, turned to face the camera (WeaponFXMotion.cpp moves, WeaponFXDraw.cpp draws). */
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
	enum class EParticle : uint8 { Spark, Flash, Smoke, Chip, Droplet, Bit, Streak, Thread, Ring, Fireball, Star, Flame };

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
		/** A Thread's half length along its line; a Ring's axis (unit). */
		FVector Extent = FVector::ZeroVector;
		/** A Ring's thickness (cm); a Star's spikes. */
		float Width = 0.f;
		/** A Ring turned to face the eye each frame (else square to its axis). */
		bool bFacing = false;
		/** A Flame's smoke colour, and the share of its life it burns. */
		FLinearColor Shade = FLinearColor::Black;
		float Burn = 0.f;
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
		EStreakLook Look = EStreakLook::Glow;
	};

	FParticle& AddParticle(EParticle Type, const FVector& Location, const FVector& Velocity, float Life);
	/** A chunk crossing its floor: a bounce, a skitter, then it lies still a moment (drops splat and are gone). WeaponFXDraw.cpp. */
	void LandOnFloor(FParticle& Particle);
	void Redraw(const FVector& CameraLocation);
	/** A component's instances made these (WeaponFXInstances.cpp: kept in blocks, so a particle born or gone rarely rebuilds it). */
	static void DrawInstances(UInstancedStaticMeshComponent* Instances, const TArray<FTransform>& Transforms, const TArray<float>& CustomData);
	/** The flames' quads and data, far to near (WeaponFXFire.cpp): on FireQuads, or as tinted puffs on the smoke without it. */
	void CollectFlames(const FVector& Camera, TArray<FTransform>& FireTransforms, TArray<float>& FireData, TArray<FTransform>& SmokeTransforms,
		TArray<float>& SmokeData, bool bPuffData);
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
	/**
	 * The light a world hit's dust shows in (VFX-2): FDustLight's at the hit over its full sun's level (1 in sun, about a
	 * third in shade), looked up again past ImpactLightSeconds or ImpactLightReach (a stream of fire into one spot asks once).
	 */
	FLinearColor ImpactShade(const FVector& Where);
	FDustLight ImpactDustLight;
	FLinearColor ImpactLight = FLinearColor::White;
	FVector ImpactLightWhere = FVector::ZeroVector;
	double ImpactLightAt = -1.0e9;
	static constexpr double ImpactLightSeconds = 0.25;
	static constexpr double ImpactLightReach = 400.0;
	/** While an impact with a floor spawns (SpawnSurfaceImpact), the floor its chunks are given. */
	TOptional<FPlane> SpawnFloor;
	/**
	 * A shotgun blast's pellets on creatures share one spurt budget (WeaponFXCreatures.cpp; Looter.Gunfeel.Spurt.BlastBudget):
	 * when and where its first pellet landed, and how much of the budget (in full hits' worth) its pellets have spent.
	 */
	double SpurtBlastAt = -1.0e9;
	FVector SpurtBlastWhere = FVector::ZeroVector;
	float SpurtBlastSpent = 0.f;
	/** The spurt a creature hit is given now (1: one full hit's), its blast's budget taken into account. */
	float SpurtAmountFor(const FVector& Location, float Scale, float SpurtScale, float BlastBudget);
	/** Instances drawn last frame, so an emptied component gets cleared exactly once. */
	bool bDrawnAnything = false;
	/** M_FX_Shape's quads (rings, fireballs, starbursts), when it's in the game and Looter.Vfx.Shapes is on (else stand-ins on Glows). */
	TWeakObjectPtr<UInstancedStaticMeshComponent> Shapes;
	/** Its master draws soft glows (EVfxShape::Glow: it has the SoftGlow knob), so flashes and round glows go on it. */
	bool bShapeGlows = false;
	/** M_FX_Fire's quads (SpawnFlame), drawn far to near; null without it (the flames go on the smoke). */
	TWeakObjectPtr<UInstancedStaticMeshComponent> FireQuads;
	/**
	 * Sparks and tracers as soft tapered streaks on M_FX_Muzzle (8 floats: colour, strength, shape 0, length share 1, seed, no
	 * hole), when it's in the game and Looter.Vfx.SoftStreaks is on; else they go on the glows (the old flat diamonds).
	 */
	TWeakObjectPtr<UInstancedStaticMeshComponent> SoftStreaks;
	/** The camera last drawn from (DetailAt), once a Tick has said where it is. */
	FVector LastCamera = FVector::ZeroVector;
	bool bHasCamera = false;
	/** Recent hits, so a stream of fire into one spot is thinned (VfxRules::StreamDetail). */
	FVfxHitStream HitStream;
	/** The share a hit at Location throws in its stream (1 alone), noting it. */
	float StreamDetailFor(const FVector& Location);
	/** Recent kills (VFX-2): a crowd killed at once, within KillCrowdWindow s and KillCrowdReach cm, throws less each. */
	FVfxHitStream KillStream;
	static constexpr float KillCrowdWindow = 0.6f;
	static constexpr float KillCrowdReach = 800.f;
	/** The share a kill at Location throws among the kills round it (1 alone), noting it. */
	float KillCrowdDetail(const FVector& Location);
	// --- The gun feel pass (GF-IMPACT): a hit by its gun's kind, a blast's sheet, a crit by what the body spills ---

	/** A world hit's place and sizes as SpawnSurfaceImpact worked them out (WeaponFXImpactLooks.h), for the kind's layers. */
	struct FImpactSpot;
	/** The round landing now (FLandingScope). */
	FVfxRound LandingRound;
	/** The shotgun blast gathering into its sheet. */
	FVfxBlastSheet Sheet;
	/** What a round of its kind adds to its hit (WeaponFXImpactsKinds.cpp): a punch, a chunk burst, a pellet into its blast's sheet. */
	void SpawnImpactKindLayers(const FImpactSpot& Spot, const FVfxImpactLook& Look);
	/** A burst of what it struck thrown high (Amount: its share, detail in), landing on the hit's floor. */
	void SpawnImpactChunks(const FImpactSpot& Spot, float Amount);
	/** The sniper's punch: a ring of dust over the surface, big chunks. */
	void SpawnImpactPunch(const FImpactSpot& Spot);
	/** Lays Sheet across its pattern (its puffs, and a fan, sparks or spray by its surface). */
	void SpawnBlastSheet(const FVfxTuning& Vfx);
	/**
	 * A crit's own look by what the body spills (WeaponFXCrits.cpp; Looter.Vfx.Hit.CritPop): a spider's shell shards, the
	 * Gravemother's thick gobs, a slime core's flash ring, an Unpaid's coal flare and soul-light gleam, a miner's rock chips,
	 * a placeholder's clods. Amount: its pieces' share (detail and a blast's budget in); Size the hit's; Light its glows' share of
	 * the wound's light (WoundGlowFor). Its glows are soft discs no wider on screen than a hit's may be (WoundFlashSize).
	 */
	void SpawnBloodCrit(ECreatureBlood Blood, const FVector& Point, const FVector& Up, const FVector& Shot, const FLinearColor& Tint, float Size,
		float Amount, float Light);
	/** Recent creature hits' glows (the play-check, 2026-10-23): glows at one wound share their light (VfxRules::GlowShare). */
	FVfxHitStream WoundGlowStream;
	/** The share of its light a creature hit's glows at Location give among the glows there (1 alone), noting it. */
	float WoundGlowFor(const FVector& Location);
	/** A creature hit's glow Size cm across at Location, as big as it may look from the camera (Looter.Vfx.Hit.FlashMax). */
	float WoundFlashSize(const FVector& Location, float Size) const;
	/** The effects' clock: their world's time (the wall clock without one). */
	double EffectsNow() const;
	/** The pool's size (Looter.Vfx.MaxParticles as the effects started). */
	int32 MaxParticleCount = 1000;
	FRandomStream Random{ 0x5eed };
};
